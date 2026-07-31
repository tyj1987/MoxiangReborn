#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string_view>

namespace mxh::server {

inline constexpr std::uint16_t MAX_PLAYER_LEVEL_NUM = 121;
inline constexpr std::int32_t MAX_MONSTER_LEVEL_POINT_RESTRICT_NUM = 9;
inline constexpr std::int32_t MONSTER_LEVEL_RESTRICT_LOW_START_NUM = 6;
inline constexpr std::size_t PLAYER_MONSTER_POINT_COLUMN_COUNT =
    static_cast<std::size_t>(MAX_MONSTER_LEVEL_POINT_RESTRICT_NUM +
                             MONSTER_LEVEL_RESTRICT_LOW_START_NUM + 1);

class PlayerMonsterPointTable {
public:
    static PlayerMonsterPointTable load_from_bin(const std::filesystem::path& path);
    static PlayerMonsterPointTable load_from_text(std::string_view text);

    std::uint32_t get(std::uint16_t level, std::int32_t level_gap) const;
    std::uint32_t get_player_point(std::uint16_t level, std::int32_t level_gap) const;

private:
    explicit PlayerMonsterPointTable(
        std::array<std::uint32_t,
                   static_cast<std::size_t>(MAX_PLAYER_LEVEL_NUM) *
                       PLAYER_MONSTER_POINT_COLUMN_COUNT> values)
        : values_(values) {}

    std::array<std::uint32_t,
               static_cast<std::size_t>(MAX_PLAYER_LEVEL_NUM) *
                   PLAYER_MONSTER_POINT_COLUMN_COUNT>
        values_{};
};

}  // namespace mxh::server
