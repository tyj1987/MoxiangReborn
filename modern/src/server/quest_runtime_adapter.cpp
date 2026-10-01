#include "mxh/server/quest_runtime_adapter.hpp"
#include <algorithm>
#include <limits>
namespace mxh::server {
QuestDefinition make_runtime_quest_definition(const QuestScriptDefinition& script) {
    QuestDefinition out; out.quest_id = script.quest_idx; out.title = "quest_" + std::to_string(script.quest_idx);
    bool level_limit_set = false;
    for (const auto& subquest : script.subquests) {
        if (!level_limit_set) {
            const auto level_limit = std::find_if(
                subquest.limits.begin(), subquest.limits.end(), [](const auto& limit) {
                    return limit.kind == QuestLimitKind::Level;
                });
            if (level_limit != subquest.limits.end()) {
                out.min_level = static_cast<std::uint16_t>(std::min<std::uint32_t>(
                    level_limit->value1, std::numeric_limits<std::uint16_t>::max()));
                out.max_level = static_cast<std::uint16_t>(std::min<std::uint32_t>(
                    level_limit->value2, std::numeric_limits<std::uint16_t>::max()));
                level_limit_set = true;
            }
        }
        const bool final_subquest = !script.end_param_set ||
            subquest.subquest_idx == script.end_param;
        const auto hunt_triggers = static_cast<std::size_t>(std::count_if(
            subquest.triggers.begin(), subquest.triggers.end(), [](const auto& trigger) {
                return trigger.event.kind == QuestEventKind::Hunt ||
                       trigger.event.kind == QuestEventKind::HuntAll;
            }));
        std::uint32_t single_hunt_count_target = 0u;
        if (hunt_triggers == 1u) {
            for (const auto& trigger : subquest.triggers) {
                if (trigger.event.kind == QuestEventKind::Count &&
                    trigger.event.param1 == subquest.subquest_idx && trigger.event.param2 > 0) {
                    single_hunt_count_target = static_cast<std::uint32_t>(trigger.event.param2);
                    break;
                }
            }
        }
        std::uint32_t shared_hunt_count_target = 0u;
        std::vector<std::uint32_t> shared_hunt_targets;
        if (hunt_triggers > 1u) {
            std::optional<std::uint32_t> shared_counter;
            bool every_hunt_uses_shared_add_count = true;
            bool any_hunt_uses_add_count = false;
            for (const auto& trigger : subquest.triggers) {
                if (trigger.event.kind != QuestEventKind::Hunt) continue;
                const auto add_count = std::find_if(
                    trigger.executes.begin(), trigger.executes.end(), [](const auto& execute) {
                        return execute.kind == QuestExecuteKind::AddCount &&
                               execute.args.size() >= 2u;
                    });
                if (add_count == trigger.executes.end()) {
                    every_hunt_uses_shared_add_count = false;
                    continue;
                }
                any_hunt_uses_add_count = true;
                const auto counter = add_count->args[0];
                if (shared_counter && *shared_counter != counter) {
                    every_hunt_uses_shared_add_count = false;
                    break;
                }
                shared_counter = counter;
                shared_hunt_targets.push_back(trigger.event.param1);
            }
            if (!any_hunt_uses_add_count) {
                shared_counter = subquest.subquest_idx;
                every_hunt_uses_shared_add_count = true;
                shared_hunt_targets.clear();
                for (const auto& trigger : subquest.triggers) {
                    if (trigger.event.kind == QuestEventKind::Hunt) {
                        shared_hunt_targets.push_back(trigger.event.param1);
                    }
                }
            }
            if (every_hunt_uses_shared_add_count && shared_counter) {
                const auto count_trigger = std::find_if(
                    subquest.triggers.begin(), subquest.triggers.end(),
                    [&](const auto& trigger) {
                        return trigger.event.kind == QuestEventKind::Count &&
                               trigger.event.param1 == *shared_counter &&
                               trigger.event.param2 > 0;
                    });
                if (count_trigger != subquest.triggers.end()) {
                    shared_hunt_count_target =
                        static_cast<std::uint32_t>(count_trigger->event.param2);
                }
            }
            std::sort(shared_hunt_targets.begin(), shared_hunt_targets.end());
            shared_hunt_targets.erase(
                std::unique(shared_hunt_targets.begin(), shared_hunt_targets.end()),
                shared_hunt_targets.end());
            if (shared_hunt_count_target != 0u && !shared_hunt_targets.empty()) {
                QuestSub shared;
                shared.stage = subquest.subquest_idx;
                shared.kind = QuestSubKind::Kill;
                shared.target_id = shared_hunt_targets.front();
                shared.target = shared_hunt_count_target;
                shared.accepted_target_ids = shared_hunt_targets;
                out.subs.push_back(std::move(shared));
            }
        }
        for (const auto& trigger : subquest.triggers) {
            if (trigger.event.kind == QuestEventKind::Hunt || trigger.event.kind == QuestEventKind::HuntAll) {
                if (shared_hunt_count_target != 0u &&
                    trigger.event.kind == QuestEventKind::Hunt) {
                    continue;
                }
                QuestSub sub; sub.stage = subquest.subquest_idx; sub.kind = QuestSubKind::Kill;
                sub.target_id = trigger.event.kind == QuestEventKind::HuntAll ? 0u : trigger.event.param1;
                sub.target = single_hunt_count_target != 0u
                    ? single_hunt_count_target
                    : static_cast<std::uint32_t>(std::max(1, trigger.event.param2));
                const auto count_execute = std::find_if(
                    trigger.executes.begin(), trigger.executes.end(), [](const auto& execute) {
                        return execute.kind == QuestExecuteKind::AddCount ||
                               execute.kind == QuestExecuteKind::AddCountFQW ||
                               execute.kind == QuestExecuteKind::AddCountFW ||
                               execute.kind == QuestExecuteKind::LevelGap ||
                               execute.kind == QuestExecuteKind::MonLevel;
                    });
                if (count_execute != trigger.executes.end() &&
                    count_execute->args.size() >= 2u) {
                    const auto count_trigger = std::find_if(
                        subquest.triggers.begin(), subquest.triggers.end(),
                        [&](const auto& candidate) {
                            return candidate.event.kind == QuestEventKind::Count &&
                                   candidate.event.param1 == count_execute->args[0] &&
                                   candidate.event.param2 > 0;
                        });
                    sub.target = count_trigger != subquest.triggers.end()
                        ? static_cast<std::uint32_t>(count_trigger->event.param2)
                        : count_execute->args[1];
                    if (count_execute->kind == QuestExecuteKind::AddCountFQW &&
                        count_execute->args.size() >= 3u) {
                        sub.count_filter = QuestCountFilterKind::WeaponItem;
                        sub.filter_value1 = count_execute->args[2];
                    } else if (count_execute->kind == QuestExecuteKind::AddCountFW &&
                               count_execute->args.size() >= 3u) {
                        sub.count_filter = QuestCountFilterKind::WeaponKind;
                        sub.filter_value1 = count_execute->args[2];
                    } else if (count_execute->kind == QuestExecuteKind::LevelGap &&
                               count_execute->args.size() >= 4u) {
                        sub.count_filter = QuestCountFilterKind::PlayerMonsterLevelGap;
                        sub.filter_value1 = count_execute->args[2];
                        sub.filter_value2 = count_execute->args[3];
                    } else if (count_execute->kind == QuestExecuteKind::MonLevel &&
                               count_execute->args.size() >= 4u) {
                        sub.count_filter = QuestCountFilterKind::MonsterLevel;
                        sub.filter_value1 = count_execute->args[2];
                        sub.filter_value2 = count_execute->args[3];
                    }
                }
                out.subs.push_back(sub);
            }
            if (trigger.event.kind == QuestEventKind::NpcTalk) {
                QuestSub sub; sub.stage = subquest.subquest_idx;
                sub.kind = QuestSubKind::TalkNpc;
                sub.target_id = trigger.event.param1;
                // Legacy @TALKTONPC stores quest/script context in param2.
                sub.target = 1u;
                out.subs.push_back(sub);
            }
            if (trigger.event.kind == QuestEventKind::UseItem) {
                QuestSub sub; sub.stage = subquest.subquest_idx;
                sub.kind = QuestSubKind::Collect;
                sub.target_id = trigger.event.param1;
                // Legacy @USEITEM is a one-shot event; param2 is quest context.
                sub.target = 1u;
                out.subs.push_back(sub);
            }
            for (const auto& execute : trigger.executes) {
                if (!final_subquest) continue;
                if (execute.kind == QuestExecuteKind::TakeQuestItem && execute.args.size() >= 2) {
                    out.item_costs.push_back(QuestItemCost{
                        static_cast<std::uint16_t>(execute.args[0]), execute.args[1]});
                } else if (execute.kind == QuestExecuteKind::TakeQuestItemFQW && execute.args.size() >= 2) {
                    out.item_costs.push_back(QuestItemCost{
                        static_cast<std::uint16_t>(execute.args[0]), execute.args[1]});
                }
                if (execute.kind == QuestExecuteKind::GiveMoney && !execute.args.empty()) out.reward_money += execute.args[0];
                else if (execute.kind == QuestExecuteKind::TakeMoney && !execute.args.empty()) out.money_cost += execute.args[0];
                else if ((execute.kind == QuestExecuteKind::TakeExp || execute.kind == QuestExecuteKind::TakeSExp) && !execute.args.empty()) out.reward_exp += execute.args[0];
                else if ((execute.kind == QuestExecuteKind::GiveItem || execute.kind == QuestExecuteKind::GiveQuestItem) && execute.args.size() >= 2) {
                    out.item_rewards.push_back(QuestItemReward{execute.args[0], execute.args[1]});
                    if (out.reward_item_idx == 0) {
                        out.reward_item_idx = execute.args[0]; out.reward_item_qty = execute.args[1];
                    }
                }
            }
        }
    }
    if (out.subs.empty()) { QuestSub talk; talk.kind = QuestSubKind::TalkNpc; talk.target = 1; out.subs.push_back(talk); }
    return out;
}
} // namespace mxh::server
