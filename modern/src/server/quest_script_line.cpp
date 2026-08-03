#include "mxh/server/quest_script_line.hpp"

#include "mxh/server/quest_execute.hpp"

#include <charconv>
#include <cstddef>
#include <string_view>
#include <utility>

namespace mxh::server {
namespace {

struct LimitShape {
    std::string_view token;
    QuestLimitKind kind;
};

struct EventShape {
    std::string_view token;
    QuestEventKind kind;
};

constexpr LimitShape kLimits[] = {
    {"&LEVEL", QuestLimitKind::Level},
    {"&MONEY", QuestLimitKind::Money},
    {"&QUEST", QuestLimitKind::Quest},
    {"&SUBQUEST", QuestLimitKind::SubQuest},
    {"&STAGE", QuestLimitKind::Stage},
    {"&ATTR", QuestLimitKind::Attr},
};

constexpr EventShape kEvents[] = {
    {"@TALKTONPC", QuestEventKind::NpcTalk},
    {"@HUNT", QuestEventKind::Hunt},
    {"@COUNT", QuestEventKind::Count},
    {"@GAMEENTER", QuestEventKind::GameEnter},
    {"@LEVEL", QuestEventKind::Level},
    {"@USEITEM", QuestEventKind::UseItem},
    {"@MAPCHANGE", QuestEventKind::MapChange},
    {"@DIE", QuestEventKind::Die},
    {"@HUNTALL", QuestEventKind::HuntAll},
};

const LimitShape* find_limit(std::string_view token) noexcept {
    for (const auto& entry : kLimits)
        if (entry.token == token) return &entry;
    return nullptr;
}

const EventShape* find_event(std::string_view token) noexcept {
    for (const auto& entry : kEvents)
        if (entry.token == token) return &entry;
    return nullptr;
}

bool next_token(std::string_view line, std::size_t& cursor,
                std::string_view& token) noexcept {
    const auto is_space = [](char value){
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

bool parse_i32(std::string_view token, std::int32_t& value) noexcept {
    if (token.empty()) return false;
    const auto result = std::from_chars(token.data(), token.data() + token.size(), value);
    return result.ec == std::errc{} && result.ptr == token.data() + token.size();
}

}  // anonymous namespace

std::optional<QuestEventKind> quest_event_kind_from_token(
    std::string_view token) noexcept {
    const auto* entry = find_event(token);
    if (entry == nullptr) return std::nullopt;
    return entry->kind;
}

std::optional<QuestLimitKind> quest_limit_kind_from_token(
    std::string_view token) noexcept {
    const auto* entry = find_limit(token);
    if (entry == nullptr) return std::nullopt;
    return entry->kind;
}

std::optional<QuestScriptLine> parse_quest_script_line(
    std::string_view line, std::uint32_t quest_idx,
    std::uint32_t subquest_idx) noexcept {
    QuestScriptLine result;
    result.quest_idx = quest_idx;
    result.subquest_idx = subquest_idx;

    std::size_t cursor = 0;
    bool event_seen = false;

    while (true){
        std::string_view token;
        if (!next_token(line, cursor, token)) break;
        if (token.empty()) return std::nullopt;
        const char lead = token[0];
        if (lead != '&' && lead != '@' && lead != '*') return std::nullopt;

        if (lead == '&'){
            const auto* shape = find_limit(token);
            if (shape == nullptr) return std::nullopt;
            std::uint32_t v1 = 0;
            std::string_view param;
            if (!next_token(line, cursor, param)) return std::nullopt;
            if (!parse_u32(param, v1)) return std::nullopt;
            std::uint32_t v2 = v1;
            std::string_view param2;
            if (!next_token(line, cursor, param2)) return std::nullopt;
            if (!parse_u32(param2, v2)) return std::nullopt;
            result.limits.push_back({shape->kind, v1, v2});

        } else if (lead == '@'){
            if (event_seen) return std::nullopt;
            const auto* shape = find_event(token);
            if (shape == nullptr) return std::nullopt;
            std::uint32_t p1 = 0;
            std::int32_t p2 = 0;
            std::string_view param;
            if (!next_token(line, cursor, param)) return std::nullopt;
            if (!parse_u32(param, p1)) return std::nullopt;
            if (!next_token(line, cursor, param)) return std::nullopt;
            if (!parse_i32(param, p2)) return std::nullopt;
            result.event = {shape->kind, p1, p2};
            event_seen = true;

        } else {
            const auto token_start = cursor - token.size();
            const auto sub_line = line.substr(token_start);
            std::size_t scan = 0;
            std::string_view scan_tok;
            if (!next_token(sub_line, scan, scan_tok)) return std::nullopt;
            const auto kind = quest_execute_kind_from_token(scan_tok);
            if (!kind.has_value()) return std::nullopt;
            const auto range = quest_execute_arg_range(*kind);
            std::vector<std::uint32_t> args;
            std::size_t args_end_scan = scan;
            for (std::size_t i = 0; i < range.second; ++i){
                const auto before = scan;
                if (!next_token(sub_line, scan, scan_tok)) break;
                std::uint32_t value = 0;
                if (!parse_u32(scan_tok, value)){
                    scan = before;
                    break;
                }
                args.push_back(value);
                args_end_scan = scan;
            }
            if (args.size() < range.first) return std::nullopt;
            if (args.size() > range.second) return std::nullopt;
            QuestExecuteSpec execute;
            execute.kind = *kind;
            execute.quest_idx = quest_idx;
            execute.subquest_idx = subquest_idx;
            execute.args = std::move(args);
            if (*kind == QuestExecuteKind::RandomTakeItem){
                const auto max_count = execute.args[0];
                const auto random_count = execute.args[1];
                if (max_count > 0xffffu || random_count > max_count ||
                    execute.args.size() != 2u + static_cast<std::size_t>(max_count) * 3u)
                    return std::nullopt;
                for (std::size_t i = 0; i < max_count; ++i){
                    const auto offset = 2u + i * 3u;
                    execute.random_items.push_back({
                        static_cast<std::uint16_t>(execute.args[offset]),
                        static_cast<std::uint16_t>(execute.args[offset + 1u]),
                        static_cast<std::uint16_t>(execute.args[offset + 2u])});
                }
            }
            cursor = token_start + args_end_scan;
            result.executes.push_back(std::move(execute));
}
}

    if (!event_seen) return std::nullopt;
    return result;
}

bool quest_event_matches(const QuestEventSpec& condition,
                         const QuestEventSpec& runtime) noexcept {
    if (condition.kind == QuestEventKind::HuntAll){
        return runtime.kind == QuestEventKind::Hunt;
}
    return condition.kind == runtime.kind &&
           condition.param1 == runtime.param1 &&
           condition.param2 == runtime.param2;
}

}  // namespace mxh::server