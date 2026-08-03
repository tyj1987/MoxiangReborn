#include "mxh/server/quest_trigger.hpp"

#include "mxh/server/quest_group.hpp"

namespace mxh::server {
QuestTrigger quest_trigger_from_script_line(
    const QuestScriptLine& line) noexcept {
    QuestTrigger trigger;
    trigger.quest_idx = line.quest_idx;
    trigger.subquest_idx = line.subquest_idx;
    trigger.event = line.event;
    trigger.executes = line.executes;
    for (const auto& execute : line.executes){
        if (execute.kind == QuestExecuteKind::EndQuest && !execute.args.empty()){
            trigger.end_param = execute.args[0];
            break;
        }
    }
    return trigger;
}
bool quest_trigger_check(const QuestTrigger& trigger,
                         const QuestEventSpec& runtime) noexcept {
    return quest_event_matches(trigger.event, runtime);
}
QuestTriggerApplyResult quest_trigger_run(
    const QuestTrigger& trigger,
    const QuestEventSpec& runtime_event,
    QuestGroupState& state,
    std::int32_t player_level,
    std::int32_t monster_level,
    std::uint32_t now_ms) noexcept {
    if (!quest_trigger_check(trigger, runtime_event))
        return {QuestTriggerApplyStatus::ConditionFailed, false};
    bool any_changed = false;
    for (const auto& execute : trigger.executes){
        QuestExecuteApplyResult result{
            QuestExecuteApplyStatus::UnsupportedContext, false};
        switch (execute.kind){
        case QuestExecuteKind::AddCount:
        case QuestExecuteKind::AddCountFQW:
        case QuestExecuteKind::AddCountFW:
        case QuestExecuteKind::LevelGap:
        case QuestExecuteKind::MonLevel:
            result = apply_count_execute(
                state, execute, player_level, monster_level);
            break;
        case QuestExecuteKind::RegistTime:
            result = apply_time_execute(state, execute);
            break;
        case QuestExecuteKind::GiveQuestItem:
        case QuestExecuteKind::TakeQuestItem:
        case QuestExecuteKind::TakeMoneyPerCount:
        case QuestExecuteKind::GiveItem:
        case QuestExecuteKind::TakeItem:
        case QuestExecuteKind::GiveMoney:
        case QuestExecuteKind::TakeMoney:
        case QuestExecuteKind::TakeExp:
        case QuestExecuteKind::TakeSExp:
        case QuestExecuteKind::TakeQuestItemFQW:
        case QuestExecuteKind::TakeQuestItemFW:
        case QuestExecuteKind::RandomTakeItem:
            result = apply_item_execute(state, execute);
            break;
        default:
            result = apply_quest_execute(state, execute, now_ms);
            break;
        }
        if (result.status != QuestExecuteApplyStatus::Applied)
            return {QuestTriggerApplyStatus::ExecuteFailed, any_changed};
        any_changed = any_changed || result.changed;
    }
    return {QuestTriggerApplyStatus::Applied, any_changed};
}
}  // namespace mxh::server
