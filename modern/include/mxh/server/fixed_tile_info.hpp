// fixed_tile_info.hpp - Phase D6 FixedTileInfo 1:1 port.
//
// Source-of-truth: legacy [Server]Map/FixedTileInfo.h + .cpp.
// Mirrors legacy CFixedTileInfo 2D tile-grid storage with bounds-safe
// GetFixedTile.  TILEINDEX is the legacy (nx, nz) pair struct.
// Modern uses raw contiguous vector; the layout is [z * W + x].
//
// IsInTile check guards against x/y out-of-range reads.

#pragma once

#include <cstdint>
#include <vector>

#include "mxh/server/fixed_tile.hpp"

namespace mxh::server {

// Mirror of legacy TILEINDEX (nx, nz pairs).
struct TileIndex {
    int nx = 0;
    int nz = 0;
};

struct FixedTileInfoState {
    int m_nTileWidth  = 0;
    int m_nTileHeight = 0;
    std::vector<FixedTile> m_tiles;  // contiguous, [z * W + x]
};

// Init: legacy SAFE_DELETE old array, new CFixedTile[w*h].
// Returns false if width or height <= 0.
bool fixed_tile_info_init(FixedTileInfoState& s, int width, int height);

// Bounds check: x,y in [0, w) x [0, h).
bool fixed_tile_info_in_tile(const FixedTileInfoState& s, int x, int y);

// GetFixedTile(x, z) returns pointer to the slot or nullptr.
FixedTile* fixed_tile_info_get(FixedTileInfoState& s, int x, int z);
const FixedTile* fixed_tile_info_get(const FixedTileInfoState& s, int x, int z);

// Legacy GetTileIndex (commented out in legacy but signature kept).
// Returns TileIndex with floor division by tile size; clamps to (0,0).
TileIndex fixed_tile_info_get_index(float fx, float fz, float tile_size);

}  // namespace mxh::server
