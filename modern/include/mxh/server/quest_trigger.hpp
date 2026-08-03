#pragma once

#include "mxh/server/quest_script_line.hpp"
#include "mxh/server/quest_execute.hpp"

#include <cstdint>
#include <vector>

namespace mxh::server {

// Bundles one parsed quest script line into a runtime trigger that can be
// checked against incoming events and fired to apply executes. Mirrors
// the legacy CQuestTrigger class which combined a CQuestCondition with a
// cPtrList of CQuestExecute plus the dwEndParam captured from the first
// *ENDQUEST execute.
struct QuestTrigger final {
    std::uint32_t quest_idx = 0;
    std::uint32_t subquest_idx = 0;
    QuestEventSpec event{};
    std::vector<QuestExecuteSpec> executes;
    std::uint32_t end_param = 0;
};

// Build a trigger from a parsed script line. The legacy loader only kept
// the subquest index of the first *ENDQUEST execute as end_param; we
// mirror that exactly.
QuestTrigger quest_trigger_from_script_line(
    const QuestScriptLine& line) noexcept;

// Check the trigger condition against a runtime event. Delegates to
// quest_event_matches (legacy CQuestCondition::CheckCondition semantics).
bool quest_trigger_check(const QuestTrigger& trigger,
                         const QuestEventSpec& runtime) noexcept;

enum class QuestTriggerApplyStatus : std::uint8_t {
    Applied = 0,
    ConditionFailed = 1,
    ExecuteFailed = 2,
};

struct QuestTriggerApplyResult final {
    QuestTriggerApplyStatus status = QuestTriggerApplyStatus::Applied;
    bool changed = false;
};

// Run all executes in order, but only if the trigger condition matches
// the runtime event (legacy CQuestTrigger::OnQuestEvent semantics).
// The legacy code stops on the first execute error and reports
// MP_QUEST_EXECUTE_ERROR to the player; we surface that as
// ExecuteFailed with changed=true if any execute ran.
// player_level and monster_level are forwarded to count executors that
// need them (legacy CQuestExecute_Count); now_ms is forwarded to quest
// executors (legacy CQuestExecute_Quest).
QuestTriggerApplyResult quest_trigger_run(
    const QuestTrigger& trigger,
    const QuestEventSpec& runtime_event,
    QuestGroupState& state,
    std::int32_t player_level = 0,
    std::int32_t monster_level = 0,
    std::uint32_t now_ms = 0) noexcept;

}  // namespace mxh::server
