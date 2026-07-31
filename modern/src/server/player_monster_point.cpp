#include "mxh/server/player_monster_point.hpp"

#include "mxh/compat/mh_file_ex.hpp"

#include <charconv>
#include <stdexcept>
#include <string>

namespace mxh::server {

namespace {

constexpr std::size_t kValueCount =
    static_cast<std::size_t>(MAX_PLAYER_LEVEL_NUM) *
    PLAYER_MONSTER_POINT_COLUMN_COUNT;

std::uint32_t parse_value(std::string_view token) {
    std::uint32_t value = 0;
    const auto result = std::from_chars(token.data(), token.data() + token.size(), value);
    if (result.ec != std::errc{} || result.ptr != token.data() + token.size())
        throw std::runtime_error("invalid PlayerxMonsterPoint value");
    return value;
}

std::array<std::uint32_t, kValueCount> parse_values(std::string_view text) {
    std::array<std::uint32_t, kValueCount> values{};
    std::size_t cursor = 0;
    for (std::size_t index = 0; index < kValueCount; ++index) {
        while (cursor < text.size() && text[cursor] <= ' ') ++cursor;
        if (cursor == text.size()) break;
        const auto begin = cursor;
        while (cursor < text.size() && text[cursor] > ' ') ++cursor;
        values[index] = parse_value(text.substr(begin, cursor - begin));
    }
    return values;
}

}  // namespace

PlayerMonsterPointTable PlayerMonsterPointTable::load_from_bin(
    const std::filesystem::path& path) {
    const auto result = mxh::compat::read_mh_bin(path);
    if (!result) throw std::runtime_error("cannot read PlayerxMonsterPoint.bin");
    const std::string text(result.value.data.begin(), result.value.data.end());
    return PlayerMonsterPointTable(parse_values(text));
}

PlayerMonsterPointTable PlayerMonsterPointTable::load_from_text(std::string_view text) {
    return PlayerMonsterPointTable(parse_values(text));
}

std::uint32_t PlayerMonsterPointTable::get(std::uint16_t level,
                                           std::int32_t level_gap) const {
    if (level == 0 || level > MAX_PLAYER_LEVEL_NUM ||
        level_gap < -MONSTER_LEVEL_RESTRICT_LOW_START_NUM ||
        level_gap > MAX_MONSTER_LEVEL_POINT_RESTRICT_NUM) {
        throw std::out_of_range("PlayerxMonsterPoint index out of range");
    }
    const auto row = static_cast<std::size_t>(level - 1);
    const auto column = static_cast<std::size_t>(
        level_gap + MONSTER_LEVEL_RESTRICT_LOW_START_NUM);
    return values_[row * PLAYER_MONSTER_POINT_COLUMN_COUNT + column];
}

std::uint32_t PlayerMonsterPointTable::get_player_point(
    std::uint16_t level, std::int32_t level_gap) const {
    if (level == MAX_PLAYER_LEVEL_NUM) return 0;
    if (level_gap < -MONSTER_LEVEL_RESTRICT_LOW_START_NUM)
        level_gap = -MONSTER_LEVEL_RESTRICT_LOW_START_NUM;
    else if (level_gap >= MAX_MONSTER_LEVEL_POINT_RESTRICT_NUM)
        level_gap = MAX_MONSTER_LEVEL_POINT_RESTRICT_NUM;
    return get(level, level_gap);
}

}  // namespace mxh::server
