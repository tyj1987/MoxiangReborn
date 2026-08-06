#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace mxh::server {

struct QuestGroupState;

// Values mirror legacy eQuestExecute. Unsupported legacy enum values are
// intentionally not accepted by parse_quest_execute because the original
// QuestScriptLoader did not construct an executor for them.
enum class QuestExecuteKind : std::uint32_t {
    EndQuest = 0,
    StartQuest = 1,
    EndSub = 2,
    EndOtherSub = 3,
    StartSub = 4,
    AddCount = 5,
    MinusCount = 6,
    GiveQuestItem = 7,
    TakeQuestItem = 8,
    GiveItem = 9,
    GiveMoney = 10,
    TakeItem = 11,
    TakeMoney = 12,
    TakeExp = 13,
    TakeSExp = 14,
    RandomTakeItem = 15,
    TakeQuestItemFQW = 16,
    AddCountFQW = 17,
    TakeQuestItemFW = 18,
    AddCountFW = 19,
    TakeMoneyPerCount = 20,
    RegenMonster = 21,
    MapChange = 22,
    ChangeStage = 23,
    ChangeSubAttr = 24,
    RegistTime = 25,
    LevelGap = 26,
    MonLevel = 27,
    EndOtherQuest = 28,
    SaveLoginPoint = 29,
};

struct QuestRandomItem final {
    std::uint16_t item_idx = 0;
    std::uint16_t item_num = 0;
    std::uint16_t percent = 0;
};

struct QuestExecuteSpec final {
    QuestExecuteKind kind = QuestExecuteKind::EndSub;
    std::uint32_t quest_idx = 0;
    std::uint32_t subquest_idx = 0;
    std::vector<std::uint32_t> args;
    std::vector<QuestRandomItem> random_items;
};

enum class QuestExecuteApplyStatus : std::uint8_t {
    Applied = 0,
    MissingQuest = 1,
    InvalidSpec = 2,
    UnsupportedContext = 3,
    MissingSubquest = 4,
};

struct QuestExecuteApplyResult final {
    QuestExecuteApplyStatus status = QuestExecuteApplyStatus::InvalidSpec;
    bool changed = false;
};

// Parse one legacy QuestScriptLoader execution line. The command must be the
// first whitespace-delimited token (for example "*ADDCOUNT 3 10"). Numeric
// values are decimal DWORDs, and extra or missing parameters reject the line.
// RandomTakeItem follows the legacy layout: max_count, random_count, then
// max_count repetitions of WORD item_idx, WORD item_num, WORD percent.
std::optional<QuestExecuteSpec> parse_quest_execute(
    std::string_view line, std::uint32_t quest_idx,
    std::uint32_t subquest_idx) noexcept;



// Return the [min,max] argument count the legacy QuestScriptLoader expects
// for a given command kind. Used by the line parser to trim a sub-line
// that may be followed by additional clauses (event, limit, execute).
std::pair<std::size_t, std::size_t> quest_execute_arg_range(
    QuestExecuteKind kind) noexcept;

std::optional<QuestExecuteKind> quest_execute_kind_from_token(
    std::string_view token) noexcept;

// Apply the count-family executors whose legacy inputs are fully represented
// by QuestGroupState. Weapon-filtered variants (AddCountFQW / AddCountFW)
// route through quest_group_add_count_from_q_weapon /
// quest_group_add_count_from_weapon using player_weapon_kind /
// player_weapon_item; default 0 disables the gate (no weapon ever matches
// so the call is a no-op, preserving legacy "missing context" semantics).
QuestExecuteApplyResult apply_count_execute(
    QuestGroupState& state, const QuestExecuteSpec& spec,
    std::int32_t player_level = 0,
    std::int32_t monster_level = 0,
    std::uint32_t player_weapon_kind = 0,
    std::uint32_t player_weapon_item = 0) noexcept;

// Apply the quest-family executors (StartSub / EndSub / EndQuest / EndOther
// Sub / EndOtherQuest / MapChange / SaveLoginPoint). Each entry requires the
// referenced quest to exist; subquest operations additionally require the
// caller to have started the subquest.
QuestExecuteApplyResult apply_quest_execute(
    QuestGroupState& state, const QuestExecuteSpec& spec,
    std::uint32_t now_ms) noexcept;

// Apply the time-family executor (RegistTime). Records the legacy day/hour/
// minute tuple on the target quest.
QuestExecuteApplyResult apply_time_execute(
    QuestGroupState& state, const QuestExecuteSpec& spec) noexcept;

// Apply the item-family executors whose legacy inputs are fully represented
// by QuestGroupState (GiveQuestItem, TakeQuestItem, TakeMoneyPerCount). The
// weapon-filtered TakeQuestItem variants (TakeQuestItemFW / FQW) route
// through quest_group_take_quest_item_from_weapon / _from_q_weapon using
// player_weapon_kind / player_weapon_item; default 0 keeps the legacy "no
// context" no-op behavior.  All other Item executors (GiveItem / TakeItem /
// GiveMoney / TakeMoney / TakeExp / TakeSExp / RandomTakeItem) still
// return UnsupportedContext until a real player inventory / network /
// random context is available.
QuestExecuteApplyResult apply_item_execute(
    QuestGroupState& state, const QuestExecuteSpec& spec,
    std::uint32_t player_weapon_kind = 0,
    std::uint32_t player_weapon_item = 0) noexcept;

}  // namespace mxh::server
