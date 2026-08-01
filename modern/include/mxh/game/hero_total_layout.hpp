#pragma once

#include "mxh/game/item_types.hpp"

#include <cstddef>

namespace mxh::game {

inline constexpr std::size_t HERO_TOTAL_BASE_OBJECT_OFFSET = 0;
inline constexpr std::size_t HERO_TOTAL_CHARACTER_OFFSET = 35;
inline constexpr std::size_t HERO_TOTAL_HERO_OFFSET = 147;
inline constexpr std::size_t HERO_TOTAL_MOVE_OFFSET = 207;
inline constexpr std::size_t HERO_TOTAL_UNIQUE_AGENT_OFFSET = 221;
inline constexpr std::size_t HERO_TOTAL_SHOP_OPTION_OFFSET = 225;
inline constexpr std::size_t HERO_TOTAL_MUGONG_OFFSET = 345;
inline constexpr std::size_t HERO_TOTAL_ABILITY_OFFSET = 695;
inline constexpr std::size_t HERO_TOTAL_ITEM_OFFSET = 1019;
inline constexpr std::size_t HERO_TOTAL_OPTION_COUNTS_OFFSET =
    HERO_TOTAL_ITEM_OFFSET + sizeof(ItemTotalInfo);
inline constexpr std::size_t HERO_TOTAL_SERVER_TIME_OFFSET =
    HERO_TOTAL_OPTION_COUNTS_OFFSET + 10;
inline constexpr std::size_t HERO_TOTAL_ADDABLE_INFO_OFFSET =
    HERO_TOTAL_SERVER_TIME_OFFSET + 16;
inline constexpr std::size_t HERO_TOTAL_EMPTY_PAYLOAD_SIZE =
    HERO_TOTAL_ADDABLE_INFO_OFFSET + 2;

static_assert(HERO_TOTAL_OPTION_COUNTS_OFFSET == 3747);
static_assert(HERO_TOTAL_SERVER_TIME_OFFSET == 3757);
static_assert(HERO_TOTAL_ADDABLE_INFO_OFFSET == 3773);
static_assert(HERO_TOTAL_EMPTY_PAYLOAD_SIZE == 3775);

} // namespace mxh::game
