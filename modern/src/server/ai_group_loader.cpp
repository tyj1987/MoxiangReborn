#include "mxh/server/ai_group_loader.hpp"

#include "mxh/compat/mh_file_ex.hpp"

#include <charconv>
#include <limits>

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

std::vector<std::string_view> tokenize(std::string_view line) {
    std::vector<std::string_view> tokens;
    std::size_t cursor = 0;
    while (cursor < line.size()) {
        while (cursor < line.size() && is_space(line[cursor])) ++cursor;
        if (cursor == line.size()) break;
        const auto start = cursor;
        while (cursor < line.size() && !is_space(line[cursor])) ++cursor;
        tokens.push_back(line.substr(start, cursor - start));
    }
    return tokens;
}

bool token_equals(std::string_view left, std::string_view right) noexcept {
    if (left.size() != right.size()) return false;
    for (std::size_t index = 0; index < left.size(); ++index) {
        char a = left[index];
        char b = right[index];
        if (a >= 'a' && a <= 'z') a = static_cast<char>(a - 'a' + 'A');
        if (b >= 'a' && b <= 'z') b = static_cast<char>(b - 'a' + 'A');
        if (a != b) return false;
    }
    return true;
}

bool parse_u32(std::string_view token, std::uint32_t& value) noexcept {
    if (token.empty()) return false;
    const auto result = std::from_chars(
        token.data(), token.data() + token.size(), value);
    return result.ec == std::errc{} &&
           result.ptr == token.data() + token.size();
}

bool parse_float(std::string_view token, float& value) noexcept {
    if (token.empty()) return false;
    const auto result = std::from_chars(
        token.data(), token.data() + token.size(), value);
    return result.ec == std::errc{} &&
           result.ptr == token.data() + token.size();
}

bool parse_condition(const std::vector<std::string_view>& tokens,
                     AiGroupCondition& condition) noexcept {
    std::uint32_t regen = 0;
    return tokens.size() == 5 &&
           parse_u32(tokens[1], condition.target_group_id) &&
           parse_float(tokens[2], condition.remainder_ratio) &&
           parse_u32(tokens[3], condition.regen_delay) &&
           parse_u32(tokens[4], regen) &&
           ((condition.regen = regen != 0), true);
}

bool parse_spawn(const std::vector<std::string_view>& tokens,
                 AiSpawnDefinition& spawn) noexcept {
    std::uint32_t object_kind = 0;
    std::uint32_t monster_kind = 0;
    std::uint32_t initially_dead = 0;
    if (tokens.size() != 7 ||
        !parse_u32(tokens[1], object_kind) ||
        !parse_u32(tokens[2], spawn.source_object_id) ||
        !parse_u32(tokens[3], monster_kind) ||
        !parse_float(tokens[4], spawn.pos_x) ||
        !parse_float(tokens[5], spawn.pos_z) ||
        !parse_u32(tokens[6], initially_dead) ||
        object_kind > std::numeric_limits<std::uint8_t>::max() ||
        monster_kind > std::numeric_limits<std::uint16_t>::max()) {
        return false;
    }
    spawn.object_kind = static_cast<std::uint8_t>(object_kind);
    spawn.monster_kind = static_cast<std::uint16_t>(monster_kind);
    spawn.initially_dead = initially_dead != 0;
    return true;
}

}  // namespace

std::size_t AiGroupList::spawn_count() const noexcept {
    std::size_t count = 0;
    for (const auto& group : groups) count += group.spawns.size();
    return count;
}

const AiGroupDefinition* AiGroupList::find_group(
    std::uint32_t group_id) const noexcept {
    for (const auto& group : groups) {
        if (group.group_id == group_id) return &group;
    }
    return nullptr;
}

std::optional<AiGroupList> parse_ai_group_list(std::string_view text) noexcept {
    AiGroupList result;
    AiGroupDefinition* current = nullptr;
    std::size_t cursor = 0;
    while (cursor < text.size()) {
        const auto line_end = text.find(static_cast<char>(10), cursor);
        const auto count = line_end == std::string_view::npos
            ? text.size() - cursor : line_end - cursor;
        const auto line = trim(text.substr(cursor, count));
        cursor = line_end == std::string_view::npos
            ? text.size() : line_end + 1;
        if (line.empty() || line.front() == ';' ||
            (line.size() >= 2 && line[0] == '/' && line[1] == '/')) {
            continue;
        }
        if (line == "{") continue;
        if (line == "}") {
            current = nullptr;
            continue;
        }

        const auto tokens = tokenize(line);
        if (tokens.empty()) continue;
        const auto command = tokens.front();
        if (token_equals(command, "$GROUP") ||
            token_equals(command, "$UNIQUE")) {
            std::uint32_t group_id = 0;
            if (tokens.size() != 2 || !parse_u32(tokens[1], group_id)) {
                return std::nullopt;
            }
            result.groups.push_back({});
            current = &result.groups.back();
            current->group_id = group_id;
            current->unique = token_equals(command, "$UNIQUE");
            continue;
        }
        if (token_equals(command, "#FILEDBOSSREGENPOSITION")) {
            AiFieldBossPosition position;
            if (tokens.size() != 3 || !parse_float(tokens[1], position.x) ||
                !parse_float(tokens[2], position.z)) {
                return std::nullopt;
            }
            result.field_boss_positions.push_back(position);
            continue;
        }
        if (current == nullptr) return std::nullopt;

        if (token_equals(command, "#MAXOBJECT")) {
            if (tokens.size() != 2 ||
                !parse_u32(tokens[1], current->max_object)) return std::nullopt;
        } else if (token_equals(command, "#PROPERTY")) {
            std::uint32_t property = 0;
            if (tokens.size() != 2 || !parse_u32(tokens[1], property) ||
                property > std::numeric_limits<std::uint16_t>::max()) {
                return std::nullopt;
            }
            current->property = static_cast<std::uint16_t>(property);
        } else if (token_equals(command, "#GROUPNAME")) {
            if (tokens.size() > 2) return std::nullopt;
            current->group_name = tokens.size() == 2
                ? std::string(tokens[1]) : std::string{};
        } else if (token_equals(command, "#ADDCONDITION") ||
                   token_equals(command, "#UNIQUEADDCONDITION")) {
            AiGroupCondition condition;
            if (!parse_condition(tokens, condition)) return std::nullopt;
            current->conditions.push_back(condition);
        } else if (token_equals(command, "#ADD") ||
                   token_equals(command, "#UNIQUEADD")) {
            AiSpawnDefinition spawn;
            if (!parse_spawn(tokens, spawn)) return std::nullopt;
            current->spawns.push_back(spawn);
        } else if (token_equals(command, "#UNIQUEREGENDELAY")) {
            if (tokens.size() != 2 || !parse_u32(
                    tokens[1], current->regen_delay.min_minutes)) {
                return std::nullopt;
            }
            current->regen_delay.kind = AiRegenDelayKind::FixedUnique;
            current->regen_delay.max_minutes =
                current->regen_delay.min_minutes;
        } else if (token_equals(command, "#RANDOMREGENDELAY") ||
                   token_equals(command, "#UNIQUERANDOMREGENDELAY") ||
                   token_equals(command, "#UNIQUERANDOMREGENDELAY2")) {
            if (tokens.size() != 3 ||
                !parse_u32(tokens[1], current->regen_delay.min_minutes) ||
                !parse_u32(tokens[2], current->regen_delay.max_minutes)) {
                return std::nullopt;
            }
            current->regen_delay.kind = token_equals(command, "#RANDOMREGENDELAY")
                ? AiRegenDelayKind::RandomGroup
                : (token_equals(command, "#UNIQUERANDOMREGENDELAY2")
                    ? AiRegenDelayKind::RandomUniqueRange
                    : AiRegenDelayKind::RandomUnique);
        } else {
            return std::nullopt;
        }
    }
    return result;
}

std::optional<AiGroupList> load_ai_group_list_bin(
    const std::filesystem::path& path) noexcept {
    const auto file = mxh::compat::read_mh_bin(path);
    if (!file.ok()) return std::nullopt;
    const auto* data = reinterpret_cast<const char*>(file.value.data.data());
    return parse_ai_group_list(std::string_view(data, file.value.data.size()));
}

}  // namespace mxh::server
