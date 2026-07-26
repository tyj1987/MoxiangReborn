// tile.hpp - Phase D6 Tile 1:1 port.
//
// Source-of-truth: legacy [Server]Map/Tile.h + .cpp.
// Mirrors legacy CTile preoccupied counter (0..255).  Tile-collision
// state (TMA_*) and dynamic-collision counter (uMuCollisonNum) are
// exposed as a separate bitfield pair so callers can manage the
// 16-bit legacy attribute layout.

#pragma once

#include <cstdint>

namespace mxh::server {

// Mirror of legacy TILE_ATTR bitfield: 8-bit preoccupied + 8-bit collision num.
struct TileAttr {
    std::uint8_t uCharPreoccupied = 0;  // 0..255
    std::uint8_t uMuCollisonNum   = 0;
};

// Mirror of legacy TMA_* collision masks.
inline constexpr std::uint8_t TMA_COLLISON      = 0x01u;
inline constexpr std::uint8_t TMA_FIRSTEFFECT   = 0x10u;
inline constexpr std::uint8_t TMA_CONTEFFECT    = 0x08u;
inline constexpr std::uint8_t TMA_OBJECT        = 0x40u;

struct Tile {
    TileAttr m_Attr{};
};

void tile_init(Tile& t);

// Preoccupied counter: legacy increments/decrements uCharPreoccupied.
// Saturates at 0 on decrement (legacy wrapped to 255).
void tile_increase_preoccupied(Tile& t);
void tile_decrease_preoccupied(Tile& t);
std::uint8_t tile_get_preoccupied(const Tile& t);

// MuCollision counter (legacy uMuCollisonNum).
void tile_inc_mu_collision(Tile& t);
void tile_dec_mu_collision(Tile& t);
std::uint8_t tile_get_mu_collision(const Tile& t);

// IsCollisonTile: occupied > 0 OR mu_collision_num > 0.
bool tile_is_collision(const Tile& t);

}  // namespace mxh::server
