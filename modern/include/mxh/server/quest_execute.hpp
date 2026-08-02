#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace mxh::server {

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

// Parse one legacy QuestScriptLoader execution line. The command must be the
// first whitespace-delimited token (for example "*ADDCOUNT 3 10"). Numeric
// values are decimal DWORDs, and extra or missing parameters reject the line.
// RandomTakeItem follows the legacy layout: max_count, random_count, then
// max_count repetitions of WORD item_idx, WORD item_num, WORD percent.
std::optional<QuestExecuteSpec> parse_quest_execute(
    std::string_view line, std::uint32_t quest_idx,
    std::uint32_t subquest_idx) noexcept;

std::optional<QuestExecuteKind> quest_execute_kind_from_token(
    std::string_view token) noexcept;

}  // namespace mxh::server
