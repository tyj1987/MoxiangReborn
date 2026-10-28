#include "mxh/server/quest_runtime_adapter.hpp"
#include <algorithm>
namespace mxh::server {
QuestDefinition make_runtime_quest_definition(const QuestScriptDefinition& script) {
    QuestDefinition out; out.quest_id = script.quest_idx; out.title = "quest_" + std::to_string(script.quest_idx);
    for (const auto& subquest : script.subquests) {
        const bool final_subquest = !script.end_param_set ||
            subquest.subquest_idx == script.end_param;
        for (const auto& trigger : subquest.triggers) {
            if (trigger.event.kind == QuestEventKind::Hunt || trigger.event.kind == QuestEventKind::HuntAll) {
                QuestSub sub; sub.stage = subquest.subquest_idx; sub.kind = QuestSubKind::Kill;
                sub.target_id = trigger.event.kind == QuestEventKind::HuntAll ? 0u : trigger.event.param1;
                sub.target = static_cast<std::uint32_t>(std::max(1, trigger.event.param2));
                out.subs.push_back(sub);
            }
            if (trigger.event.kind == QuestEventKind::NpcTalk) {
                QuestSub sub; sub.stage = subquest.subquest_idx;
                sub.kind = QuestSubKind::TalkNpc;
                sub.target_id = trigger.event.param1;
                sub.target = static_cast<std::uint32_t>(
                    std::max(1, trigger.event.param2));
                out.subs.push_back(sub);
            }
            if (trigger.event.kind == QuestEventKind::UseItem) {
                QuestSub sub; sub.stage = subquest.subquest_idx;
                sub.kind = QuestSubKind::Collect;
                sub.target_id = trigger.event.param1;
                sub.target = static_cast<std::uint32_t>(
                    std::max(1, trigger.event.param2));
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
                else if ((execute.kind == QuestExecuteKind::GiveItem || execute.kind == QuestExecuteKind::GiveQuestItem) && execute.args.size() >= 2 && out.reward_item_idx == 0) {
                    out.reward_item_idx = execute.args[0]; out.reward_item_qty = execute.args[1];
                }
            }
        }
    }
    if (out.subs.empty()) { QuestSub talk; talk.kind = QuestSubKind::TalkNpc; talk.target = 1; out.subs.push_back(talk); }
    return out;
}
} // namespace mxh::server
