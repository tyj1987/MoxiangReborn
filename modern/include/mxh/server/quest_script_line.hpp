#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace mxh::server {

// Numeric values mirror the Korean legacy eQuestEvent enum
// ([CC]Quest/QuestDefines.h). Script-loadable tokens cover 9 of the 11
// values; EndSub and Time are runtime-generated only.
enum class QuestEventKind : std::uint32_t {
    NpcTalk = 1,
    Hunt = 2,
    EndSub = 3,
    Count = 4,
    GameEnter = 5,
    Level = 6,
    UseItem = 7,
    MapChange = 8,
    Die = 9,
    Time = 10,
    HuntAll = 11,
};

// Numeric values mirror the Korean legacy eQuestLimitKind enum
// ([CC]Quest/QuestDefines.h). 6 kinds; only 5 are script-loadable by
// default (Attr is _JAPAN_LOCAL_ only).
enum class QuestLimitKind : std::uint32_t {
    Level = 0,
    Money = 1,
    Quest = 2,
    SubQuest = 3,
    Stage = 4,
    Attr = 5,
};

struct QuestLimitSpec final {
    QuestLimitKind kind = QuestLimitKind::Level;
    std::uint32_t value1 = 0;
    std::uint32_t value2 = 0;
};

struct QuestEventSpec final {
    QuestEventKind kind = QuestEventKind::NpcTalk;
    std::uint32_t param1 = 0;
    std::int32_t param2 = 0;
};

struct QuestExecuteSpec;
struct QuestScriptLine final {
    std::uint32_t quest_idx = 0;
    std::uint32_t subquest_idx = 0;
    std::vector<QuestLimitSpec> limits;
    QuestEventSpec event{};
    std::vector<QuestExecuteSpec> executes;
};

// Map the script-loadable event tokens. EndSub and Time are deliberately
// rejected (no LOADUNIT for them in the legacy default QuestScriptLoader).
std::optional<QuestEventKind> quest_event_kind_from_token(
    std::string_view token) noexcept;

// Map the script-loadable limit tokens. Attr is rejected (only
// registered in legacy _JAPAN_LOCAL_ builds).
std::optional<QuestLimitKind> quest_limit_kind_from_token(
    std::string_view token) noexcept;

// Parse one legacy quest script line containing any number of &Limit
// clauses, at most one @Event clause, and any number of *Execute clauses.
// The classic layout matches CQuestTrigger::ReadTrigger:
//   &LEVEL 1 99 &MONEY 1000 @HUNT 1 10 *ADDCOUNT 1 5 *ENDSUB
std::optional<QuestScriptLine> parse_quest_script_line(
    std::string_view line, std::uint32_t quest_idx,
    std::uint32_t subquest_idx) noexcept;

// Legacy CQuestCondition::CheckCondition semantics: HuntAll matches any
// Hunt; everything else requires kind + param1 + param2 to be equal.
bool quest_event_matches(const QuestEventSpec& condition,
                         const QuestEventSpec& runtime) noexcept;

}  // namespace mxh::server
