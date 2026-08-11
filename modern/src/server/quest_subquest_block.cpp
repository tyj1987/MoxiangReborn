#include "mxh/server/quest_subquest_block.hpp"
#include "mxh/server/quest_execute.hpp"

#include <charconv>

namespace mxh::server {
namespace {
bool is_space(char value) noexcept {
    return value == ' ' || value == static_cast<char>(9) ||
           value == static_cast<char>(10) || value == static_cast<char>(13);
}

std::string_view trim(std::string_view value) noexcept {
    while (!value.empty() && is_space(value.front())) value.remove_prefix(1);
    while (!value.empty() && is_space(value.back())) value.remove_suffix(1);
    return value;
}

bool next_token(std::string_view line, std::size_t& cursor,
                std::string_view& token) noexcept {
    while (cursor < line.size() && is_space(line[cursor])) ++cursor;
    if (cursor == line.size()) return false;
    const auto start = cursor;
    while (cursor < line.size() && !is_space(line[cursor])) ++cursor;
    token = line.substr(start, cursor - start);
    return true;
}

bool parse_u32(std::string_view token, std::uint32_t& value) noexcept {
    if (token.empty()) return false;
    const auto result = std::from_chars(
        token.data(), token.data() + token.size(), value);
    return result.ec == std::errc{} &&
           result.ptr == token.data() + token.size();
}
}  // namespace

std::optional<std::vector<QuestLimitSpec>> parse_quest_subquest_limit_line(
    std::string_view line) noexcept {
    std::vector<QuestLimitSpec> limits;
    std::size_t cursor = 0;
    std::string_view token;
    while (next_token(line, cursor, token)) {
        if (token.empty() || token.front() != '&') return std::nullopt;
        const auto kind = quest_limit_kind_from_token(token);
        if (!kind.has_value()) return std::nullopt;

        std::string_view value1_token;
        std::string_view value2_token;
        std::uint32_t value1 = 0;
        std::uint32_t value2 = 0;
        if (!next_token(line, cursor, value1_token) ||
            !next_token(line, cursor, value2_token) ||
            !parse_u32(value1_token, value1) ||
            !parse_u32(value2_token, value2)) {
            return std::nullopt;
        }
        limits.push_back({*kind, value1, value2});
    }
    if (limits.empty()) return std::nullopt;
    return limits;
}

std::optional<QuestSubquestEntry> parse_quest_subquest_block(
    std::string_view block, std::uint32_t quest_idx,
    std::uint32_t subquest_idx) noexcept {
    QuestSubquestEntry entry;
    entry.subquest_idx = subquest_idx;

    std::size_t cursor = 0;
    while (cursor < block.size()) {
        const auto line_end = block.find(static_cast<char>(10), cursor);
        const auto count = line_end == std::string_view::npos
            ? block.size() - cursor
            : line_end - cursor;
        const auto line = trim(block.substr(cursor, count));
        cursor = line_end == std::string_view::npos
            ? block.size()
            : line_end + 1;
        if (line.empty()) continue;

        std::size_t line_cursor = 0;
        std::string_view directive;
        if (!next_token(line, line_cursor, directive)) return std::nullopt;
        const auto body = trim(line.substr(line_cursor));
        if (directive == "#LIMIT") {
            const auto limits = parse_quest_subquest_limit_line(body);
            if (!limits.has_value()) return std::nullopt;
            entry.limits.insert(
                entry.limits.end(), limits->begin(), limits->end());
        } else if (directive == "#TRIGGER") {
            const auto trigger =
                parse_quest_script_line(body, quest_idx, subquest_idx);
            if (!trigger.has_value()) return std::nullopt;
            entry.triggers.push_back(*trigger);
        } else {
            // Unknown directives (e.g. #NPCSCRIPT / #REWARD) are
            // client-side presentation data in the shipped QuestScript.bin;
            // skip them instead of failing the whole quest.
            continue;
        }
    }
    return entry;
}

std::optional<QuestSubquestEntry> parse_quest_subquest_stanza(
    std::string_view stanza, std::uint32_t quest_idx) noexcept {
    std::size_t cursor = 0;
    std::string_view token;
    if (!next_token(stanza, cursor, token) || token != "$SUBQUEST") {
        return std::nullopt;
    }

    std::string_view subquest_token;
    std::uint32_t subquest_idx = 0;
    if (!next_token(stanza, cursor, subquest_token) ||
        !parse_u32(subquest_token, subquest_idx)) {
        return std::nullopt;
    }
    if (!next_token(stanza, cursor, token) || token != "{") {
        return std::nullopt;
    }

    auto body = trim(stanza.substr(cursor));
    if (body.empty() || body.back() != '}') return std::nullopt;
    body.remove_suffix(1);
    return parse_quest_subquest_block(body, quest_idx, subquest_idx);
}
}  // namespace mxh::server
