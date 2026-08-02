#include "mxh/server/quest_execute.hpp"
#include "mxh/server/quest_group.hpp"

#include <charconv>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

namespace mxh::server {
namespace {

struct CommandShape {
    std::string_view token;
    QuestExecuteKind kind;
    std::size_t min_args;
    std::size_t max_args;
};

constexpr CommandShape kCommands[] = {
    {"*ENDQUEST", QuestExecuteKind::EndQuest, 1, 1},
    {"*ENDSUB", QuestExecuteKind::EndSub, 0, 0},
    {"*ENDOTHERSUB", QuestExecuteKind::EndOtherSub, 2, 2},
    {"*STARTSUB", QuestExecuteKind::StartSub, 2, 2},
    {"*ADDCOUNT", QuestExecuteKind::AddCount, 2, 2},
    {"*ADDCOUNTFQW", QuestExecuteKind::AddCountFQW, 3, 3},
    {"*ADDCOUNTFW", QuestExecuteKind::AddCountFW, 3, 3},
    {"*ADDCOUNTLEVELGAP", QuestExecuteKind::LevelGap, 4, 4},
    {"*ADDCOUNTMONLEVEL", QuestExecuteKind::MonLevel, 4, 4},
    {"*GIVEQUESTITEM", QuestExecuteKind::GiveQuestItem, 2, 2},
    {"*TAKEQUESTITEM", QuestExecuteKind::TakeQuestItem, 3, 3},
    {"*GIVEITEM", QuestExecuteKind::GiveItem, 2, 2},
    {"*GIVEMONEY", QuestExecuteKind::GiveMoney, 1, 1},
    {"*TAKEITEM", QuestExecuteKind::TakeItem, 3, 3},
    {"*TAKEMONEY", QuestExecuteKind::TakeMoney, 1, 1},
    {"*TAKEEXP", QuestExecuteKind::TakeExp, 1, 1},
    {"*TAKESEXP", QuestExecuteKind::TakeSExp, 1, 1},
    {"*RANDOMTAKEITEM", QuestExecuteKind::RandomTakeItem, 2, SIZE_MAX},
    {"*TAKEQUESTITEMFQW", QuestExecuteKind::TakeQuestItemFQW, 4, 4},
    {"*TAKEQUESTITEMFW", QuestExecuteKind::TakeQuestItemFW, 4, 4},
    {"*TAKEMONEYPERCOUNT", QuestExecuteKind::TakeMoneyPerCount, 2, 2},
    {"*REGENMONSTER", QuestExecuteKind::RegenMonster, 1, 1},
    {"*MAPCHANGE", QuestExecuteKind::MapChange, 2, 2},
    {"*CHANGESTAGE", QuestExecuteKind::ChangeStage, 1, 1},
    {"*REGISTTIME", QuestExecuteKind::RegistTime, 4, 4},
    {"*ENDOTHERQUEST", QuestExecuteKind::EndOtherQuest, 2, 2},
    {"*SAVELOGINPOINT", QuestExecuteKind::SaveLoginPoint, 1, 1},
};

const CommandShape* find_command(std::string_view token) noexcept {
    for (const auto& command : kCommands)
        if (command.token == token) return &command;
    return nullptr;
}

bool next_token(std::string_view line, std::size_t& cursor,
                std::string_view& token) noexcept {
    const auto is_space = [](char value) {
        return value == ' ' || value == '\t' || value == '\r' || value == '\n';
    };
    while (cursor < line.size() && is_space(line[cursor])) ++cursor;
    if (cursor == line.size()) return false;
    const auto start = cursor;
    while (cursor < line.size() && !is_space(line[cursor])) ++cursor;
    token = line.substr(start, cursor - start);
    return true;
}

bool parse_u32(std::string_view token, std::uint32_t& value) noexcept {
    if (token.empty()) return false;
    const auto result = std::from_chars(token.data(), token.data() + token.size(), value);
    return result.ec == std::errc{} && result.ptr == token.data() + token.size();
}

}  // namespace

std::optional<QuestExecuteKind> quest_execute_kind_from_token(
    std::string_view token) noexcept {
    const auto* command = find_command(token);
    if (command == nullptr) return std::nullopt;
    return command->kind;
}

std::optional<QuestExecuteSpec> parse_quest_execute(
    std::string_view line, std::uint32_t quest_idx,
    std::uint32_t subquest_idx) noexcept {
    std::size_t cursor = 0;
    std::string_view token;
    if (!next_token(line, cursor, token)) return std::nullopt;
    const auto* command = find_command(token);
    if (command == nullptr) return std::nullopt;

    std::vector<std::uint32_t> values;
    while (next_token(line, cursor, token)) {
        std::uint32_t value = 0;
        if (!parse_u32(token, value)) return std::nullopt;
        values.push_back(value);
    }
    if (values.size() < command->min_args || values.size() > command->max_args)
        return std::nullopt;

    QuestExecuteSpec result{command->kind, quest_idx, subquest_idx,
                            std::move(values), {}};
    if (command->kind != QuestExecuteKind::RandomTakeItem) return result;
    const auto max_count = result.args[0];
    const auto random_count = result.args[1];
    if (max_count > 0xffffu || random_count > max_count ||
        result.args.size() != 2u + static_cast<std::size_t>(max_count) * 3u)
        return std::nullopt;
    result.random_items.reserve(max_count);
    for (std::size_t i = 0; i < max_count; ++i) {
        const auto offset = 2u + i * 3u;
        if (result.args[offset] > 0xffffu || result.args[offset + 1] > 0xffffu ||
            result.args[offset + 2] > 0xffffu)
            return std::nullopt;
        result.random_items.push_back({
            static_cast<std::uint16_t>(result.args[offset]),
            static_cast<std::uint16_t>(result.args[offset + 1]),
            static_cast<std::uint16_t>(result.args[offset + 2])});
    }
    return result;
}

QuestExecuteApplyResult apply_count_execute(
    QuestGroupState& state, const QuestExecuteSpec& spec,
    std::int32_t player_level, std::int32_t monster_level) noexcept {
    const auto* quest = quest_group_get_quest(state, spec.quest_idx);
    if (quest == nullptr)
        return {QuestExecuteApplyStatus::MissingQuest, false};

    switch (spec.kind) {
    case QuestExecuteKind::AddCount:
        if (spec.args.size() != 2u) return {QuestExecuteApplyStatus::InvalidSpec, false};
        return {QuestExecuteApplyStatus::Applied,
                quest_group_add_count(state, spec.quest_idx, spec.args[0], spec.args[1])};
    case QuestExecuteKind::LevelGap:
        if (spec.args.size() != 4u) return {QuestExecuteApplyStatus::InvalidSpec, false};
        return {QuestExecuteApplyStatus::Applied,
                quest_group_add_count_from_level_gap(
                    state, spec.quest_idx, spec.args[0], spec.args[1], spec.args[2],
                    spec.args[3], player_level, monster_level)};
    case QuestExecuteKind::MonLevel:
        if (spec.args.size() != 4u) return {QuestExecuteApplyStatus::InvalidSpec, false};
        return {QuestExecuteApplyStatus::Applied,
                quest_group_add_count_from_monster_level(
                    state, spec.quest_idx, spec.args[0], spec.args[1], spec.args[2],
                    spec.args[3], static_cast<std::uint32_t>(monster_level))};
    case QuestExecuteKind::AddCountFQW:
    case QuestExecuteKind::AddCountFW:
        return {QuestExecuteApplyStatus::UnsupportedContext, false};
    default:
        return {QuestExecuteApplyStatus::InvalidSpec, false};
    }
}

QuestExecuteApplyResult apply_quest_execute(
    QuestGroupState& state, const QuestExecuteSpec& spec,
    std::uint32_t now_ms) noexcept {
    auto report = [](bool applied) {
        return QuestExecuteApplyResult{
            applied ? QuestExecuteApplyStatus::Applied
                    : QuestExecuteApplyStatus::MissingSubquest,
            applied};
    };
    switch (spec.kind) {
    case QuestExecuteKind::StartSub:
        if (spec.args.size() != 2u) return {QuestExecuteApplyStatus::InvalidSpec, false};
        return report(quest_group_start_subquest(
            state, spec.quest_idx, spec.args[0], now_ms));
    case QuestExecuteKind::EndSub:
        return report(quest_group_end_subquest(
            state, spec.quest_idx, spec.subquest_idx, now_ms));
    case QuestExecuteKind::EndOtherSub:
        if (spec.args.size() != 2u) return {QuestExecuteApplyStatus::InvalidSpec, false};
        return report(quest_group_end_subquest(
            state, spec.quest_idx, spec.args[0], now_ms));
    case QuestExecuteKind::EndQuest:
    case QuestExecuteKind::EndOtherQuest:
        if (spec.args.size() != 2u)
            return {QuestExecuteApplyStatus::InvalidSpec, false};
        return report(quest_group_end_quest(
            state, spec.quest_idx, spec.args[1], now_ms));
    case QuestExecuteKind::SaveLoginPoint:
        if (spec.args.size() != 1u) return {QuestExecuteApplyStatus::InvalidSpec, false};
        return {QuestExecuteApplyStatus::Applied,
                quest_group_save_login_point(state, spec.args[0])};
    case QuestExecuteKind::MapChange:
    case QuestExecuteKind::RegenMonster:
    case QuestExecuteKind::ChangeStage:
    case QuestExecuteKind::ChangeSubAttr:
        return {QuestExecuteApplyStatus::UnsupportedContext, false};
    default:
        return {QuestExecuteApplyStatus::InvalidSpec, false};
    }
}

QuestExecuteApplyResult apply_time_execute(
    QuestGroupState& state, const QuestExecuteSpec& spec) noexcept {
    if (spec.kind != QuestExecuteKind::RegistTime)
        return {QuestExecuteApplyStatus::InvalidSpec, false};
    if (spec.args.size() != 4u)
        return {QuestExecuteApplyStatus::InvalidSpec, false};
    return {QuestExecuteApplyStatus::Applied,
            quest_group_register_check_time(
                state, spec.quest_idx, spec.subquest_idx,
                spec.args[0], spec.args[1], spec.args[2], spec.args[3])};
}

}  // namespace mxh::server
