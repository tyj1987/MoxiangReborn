// quest_manager.cpp - implementation for QuestManager port.

#include "mxh/server/quest_manager.hpp"
#include <algorithm>

namespace mxh::server {

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
        if (s.kind == kind && s.target_id == target_id) {
            s.count += delta;
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
    for (auto& s : progress.subs) {
        if (s.count < s.target) return QuestState::Accepted;
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

std::optional<QuestProgress*> find_quest(QuestLog& log, std::uint32_t quest_id) noexcept {
    for (auto& q : log.quests) {
        if (q.quest_id == quest_id) return &q;
    }
    return std::nullopt;
}

bool accept_quest(QuestLog& log, const QuestDefinition& def, std::uint32_t now_ms) noexcept {
    if (find_quest(log, def.quest_id).has_value()) return false;
    log.quests.push_back(start_quest(log.player_id, def, now_ms));
    return true;
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
