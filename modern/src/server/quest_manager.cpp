// quest_manager.cpp - implementation for QuestManager port.
// 1:1 semantics with legacy [Server]Map/QuestManager.cpp + QuestGroup.cpp
// (state machine + tracker + quest event dispatch).

#include "mxh/server/quest_manager.hpp"
#include <algorithm>
#include <limits>

namespace mxh::server {

namespace {
bool count_filter_matches(const QuestSub& sub, const QuestEvent& event) noexcept {
    switch (sub.count_filter) {
    case QuestCountFilterKind::None:
        return true;
    case QuestCountFilterKind::WeaponItem:
        return event.weapon_item == sub.filter_value1;
    case QuestCountFilterKind::WeaponKind:
        return event.weapon_kind == sub.filter_value1;
    case QuestCountFilterKind::MonsterLevel:
        return event.monster_level >= sub.filter_value1 &&
               event.monster_level <= sub.filter_value2;
    case QuestCountFilterKind::PlayerMonsterLevelGap: {
        const auto player = static_cast<std::int32_t>(event.player_level);
        const auto monster = static_cast<std::int32_t>(event.monster_level);
        const auto player_above = player - monster;
        const auto monster_above = monster - player;
        return (player_above <= 0 ||
                static_cast<std::uint32_t>(player_above) <= sub.filter_value1) &&
               (monster_above <= 0 ||
                static_cast<std::uint32_t>(monster_above) <= sub.filter_value2);
    }
    }
    return false;
}
}  // namespace

QuestProgress start_quest(std::uint32_t player_id,
                          const QuestDefinition& def,
                          std::uint32_t now_ms) noexcept {
    QuestProgress p;
    p.player_id = player_id;
    p.quest_id  = def.quest_id;
    p.state     = QuestState::Accepted;
    p.accepted_time_ms = now_ms;
    p.subs.reserve(def.subs.size());
    for (const auto& s : def.subs) p.subs.push_back(s);
    return p;
}

bool increment_sub(QuestProgress& progress,
                    QuestSubKind kind,
                    std::uint32_t target_id,
                    std::uint32_t delta) noexcept {
    for (auto& s : progress.subs) {
        const bool accepted_target = s.accepted_target_ids.empty()
            ? (s.target_id == target_id || s.target_id == 0u)
            : std::find(s.accepted_target_ids.begin(),
                        s.accepted_target_ids.end(), target_id) !=
                  s.accepted_target_ids.end();
        if (s.kind == kind && accepted_target) {
            const auto remaining = std::numeric_limits<std::uint32_t>::max() - s.count;
            s.count += std::min(delta, remaining);
            if (s.count >= s.target) {
                s.count = s.target;
                return true;
            }
            return false;
        }
    }
    return false;
}

QuestState evaluate_quest_state(QuestProgress& progress) noexcept {
    if (progress.state == QuestState::Complete) return QuestState::Complete;
    if (progress.state == QuestState::Rewarded) return QuestState::Rewarded;
    if (progress.state == QuestState::Failed)   return QuestState::Failed;
    std::uint32_t active_stage = std::numeric_limits<std::uint32_t>::max();
    for (const auto& s : progress.subs) {
        if (s.count < s.target) active_stage = std::min(active_stage, s.stage);
    }
    if (active_stage != std::numeric_limits<std::uint32_t>::max()) {
        return QuestState::Accepted;
    }
    progress.state = QuestState::Complete;
    return QuestState::Complete;
}

QuestState complete_quest(QuestProgress& progress) noexcept {
    progress.state = QuestState::Complete;
    return QuestState::Complete;
}

QuestState fail_quest(QuestProgress& progress) noexcept {
    progress.state = QuestState::Failed;
    return QuestState::Failed;
}

QuestState reward_quest(QuestProgress& progress) noexcept {
    if (progress.state != QuestState::Complete) return progress.state;
    progress.state = QuestState::Rewarded;
    return QuestState::Rewarded;
}

QuestRewardResult claim_quest_reward(QuestProgress& progress,
                                      const QuestDefinition& def,
                                      Player& player,
                                      std::uint32_t next_level_exp) noexcept {
    QuestRewardResult result;
    if (progress.state != QuestState::Complete) return result;
    if (!player.is_active()) {
        result.status = QuestRewardStatus::PlayerInactive;
        return result;
    }
    if (def.money_cost != 0u && player.state().progress.money < def.money_cost) {
        return result;
    }
    for (const auto& cost : def.item_costs) {
        if (cost.item_idx == 0u || cost.quantity == 0u ||
            player.count_inventory_item(cost.item_idx) < cost.quantity) {
            result.status = QuestRewardStatus::MissingItem;
            return result;
        }
    }
    for (const auto& cost : def.item_costs) {
        if (!player.remove_inventory_item_by_icon(cost.item_idx, cost.quantity)) {
            result.status = QuestRewardStatus::MissingItem;
            return result;
        }
    }
    if (def.money_cost != 0u && !player.spend_money(def.money_cost)) {
        return result;
    }
    const auto money_before = player.state().progress.money;
    const auto exp_before = player.state().progress.total_exp;
    if (def.reward_money != 0u) player.add_money(def.reward_money);
    if (def.reward_exp != 0u) player.add_experience(def.reward_exp, next_level_exp);
    result.status = QuestRewardStatus::Granted;
    result.money = player.state().progress.money - money_before;
    result.experience = player.state().progress.total_exp - exp_before;
    result.item_idx = def.reward_item_idx;
    result.item_qty = def.reward_item_qty;
    progress.state = QuestState::Rewarded;
    return result;
}

QuestTickResult tick_quest(QuestProgress& progress,
                           const QuestDefinition& def,
                           std::uint32_t now_ms) noexcept {
    QuestTickResult result;
    result.state = progress.state;
    if (progress.state != QuestState::Accepted) return result;
    const auto before = progress.state;
    if (def.timer_seconds != 0u) {
        const auto elapsed = static_cast<std::uint32_t>(now_ms - progress.accepted_time_ms);
        const auto limit = static_cast<std::uint64_t>(def.timer_seconds) * 1000u;
        if (static_cast<std::uint64_t>(elapsed) > limit) {
            progress.state = QuestState::Failed;
            result.state = progress.state;
            result.expired = true;
            result.changed = true;
            return result;
        }
    }
    result.state = evaluate_quest_state(progress);
    result.changed = result.state != before;
    return result;
}

std::size_t active_quest_count(const QuestLog& log) noexcept {
    std::size_t count = 0;
    for (const auto& quest : log.quests) {
        if (quest.state == QuestState::Accepted) ++count;
    }
    return count;
}

std::vector<QuestEventChange> dispatch_quest_event(
    QuestLog& log,
    const QuestEvent& event) noexcept {
    std::vector<QuestEventChange> changes;
    if (event.kind == QuestSubKind::None || event.delta == 0u) return changes;

    for (auto& quest : log.quests) {
        // Legacy CQuestGroup::AddQuestEvent only updates sub-progress on
        // accepted quests.  Once a quest is Complete / Rewarded / Failed
        // the legacy group refuses further mutations, so we mirror that
        // here to keep behavior byte-equal.
        if (quest.state != QuestState::Accepted) continue;
        if (event.quest_id != 0u && event.quest_id != quest.quest_id) continue;

        QuestEventChange change;
        change.quest_id = quest.quest_id;
        change.previous_state = quest.state;
        std::uint32_t active_stage = std::numeric_limits<std::uint32_t>::max();
        for (const auto& sub : quest.subs) {
            if (sub.count < sub.target) active_stage = std::min(active_stage, sub.stage);
        }
        for (auto& sub : quest.subs) {
            if (active_stage != std::numeric_limits<std::uint32_t>::max() &&
                sub.stage != active_stage) continue;
            const bool accepted_target = sub.accepted_target_ids.empty()
                ? sub.target_id == event.target_id
                : std::find(sub.accepted_target_ids.begin(),
                            sub.accepted_target_ids.end(), event.target_id) !=
                      sub.accepted_target_ids.end();
            if (sub.kind != event.kind || !accepted_target ||
                !count_filter_matches(sub, event)) continue;
            const auto previous_count = sub.count;
            const auto remaining = std::numeric_limits<std::uint32_t>::max() - sub.count;
            sub.count += std::min(event.delta, remaining);
            if (sub.count > sub.target) sub.count = sub.target;
            if (sub.count != previous_count) ++change.updated_subs;
        }
        if (change.updated_subs == 0u) continue;

        change.state = evaluate_quest_state(quest);
        changes.push_back(change);
    }
    return changes;
}

bool can_accept_quest(const QuestDefinition& def,
                      std::uint16_t player_level) noexcept {
    return player_level >= def.min_level && player_level <= def.max_level;
}
std::optional<QuestProgress*> find_quest(QuestLog& log, std::uint32_t quest_id) noexcept {
    for (auto& q : log.quests) {
        if (q.quest_id == quest_id) return &q;
    }
    return std::nullopt;
}

bool accept_quest(QuestLog& log, const QuestDefinition& def, std::uint32_t now_ms) noexcept {
    if (find_quest(log, def.quest_id).has_value()) return false;
    if (active_quest_count(log) >= LIMIT_PROCESS_QUEST) return false;
    log.quests.push_back(start_quest(log.player_id, def, now_ms));
    return true;
}

bool accept_quest(QuestLog& log,
                  const QuestDefinition& def,
                  std::uint32_t now_ms,
                  std::uint16_t player_level) noexcept {
    if (!can_accept_quest(def, player_level)) return false;
    return accept_quest(log, def, now_ms);
}

// drop_quest - player gives up an active quest.
// Legacy: CQuestGroup::DeleteQuest - removes from quest list.
// Returns true if removed, false if not present or already Rewarded.
bool drop_quest(QuestLog& log, std::uint32_t quest_id) noexcept {
    for (auto it = log.quests.begin(); it != log.quests.end(); ++it) {
        if (it->quest_id != quest_id) continue;
        if (it->state == QuestState::Rewarded) return false;
        log.quests.erase(it);
        return true;
    }
    return false;
}

// quest_summary - count quests by state for a player log.
QuestSummary summarize(const QuestLog& log) noexcept {
    QuestSummary s;
    for (const auto& q : log.quests) {
        switch (q.state) {
            case QuestState::Accepted: ++s.accepted; break;
            case QuestState::Complete: ++s.complete; break;
            case QuestState::Rewarded: ++s.rewarded; break;
            case QuestState::Failed:   ++s.failed;   break;
            default: break;
        }
    }
    return s;
}

}  // namespace mxh::server
