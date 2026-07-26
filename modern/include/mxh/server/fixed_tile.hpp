// fixed_tile.hpp - Phase D6 FixedTile 1:1 port.
//
// Source-of-truth: legacy [Server]Map/FixedTile.h + .cpp.
// Mirrors legacy CFixedTile collision bitfield + TFA_COLLISON mask.

#pragma once

#include <cstdint>

namespace mxh::server {

// Mirror of legacy FIXEDTILEATTR (DWORD bitfield storage).
using FixedTileAttr = std::uint32_t;

// Mirror of legacy FIXEDTILE_ATTR bitfield struct.
struct FixedTileAttrBits {
    std::uint8_t  uFixedAttr = 0;   // 0..255 collision flag
};

// Mirror of legacy TFA_COLLISON mask.
inline constexpr std::uint8_t TFA_COLLISON = 0x01u;

struct FixedTile {
    FixedTileAttrBits m_FixedAttr{};
};

// Init copies the input attribute byte.
void fixed_tile_init(FixedTile& t, std::uint8_t attr);

// Legacy IsCollisonTile: returns true when uFixedAttr has TFA_COLLISON set.
bool fixed_tile_is_collision(const FixedTile& t);

// TFA_FLYCOLLISON (separate mask present in Tile.h).  Re-exposed here
// so callers do not need to include Tile.h.
inline constexpr std::uint8_t TFA_FLYCOLLISON = 0x10u;
inline constexpr std::uint8_t TFA_PEACEZONE  = 0x08u;
inline constexpr std::uint8_t TFA_JSAZONE    = 0x40u;

}  // namespace mxh::server
