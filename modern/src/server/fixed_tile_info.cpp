// fixed_tile_info.cpp - Phase D6 FixedTileInfo 1:1 port.

#include "mxh/server/fixed_tile_info.hpp"

namespace mxh::server {

bool fixed_tile_info_init(FixedTileInfoState& s, int width, int height) {
    if (width <= 0 || height <= 0) return false;
    s.m_nTileWidth  = width;
    s.m_nTileHeight = height;
    s.m_tiles.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height),
                     FixedTile{});
    return true;
}

bool fixed_tile_info_in_tile(const FixedTileInfoState& s, int x, int y) {
    if (x < 0 || y < 0 || x >= s.m_nTileWidth || y >= s.m_nTileHeight) return false;
    return true;
}

FixedTile* fixed_tile_info_get(FixedTileInfoState& s, int x, int z) {
    if (!fixed_tile_info_in_tile(s, x, z)) return nullptr;
    const std::size_t idx = static_cast<std::size_t>(z) *
                            static_cast<std::size_t>(s.m_nTileWidth) +
                            static_cast<std::size_t>(x);
    return &s.m_tiles[idx];
}

const FixedTile* fixed_tile_info_get(const FixedTileInfoState& s, int x, int z) {
    if (!fixed_tile_info_in_tile(s, x, z)) return nullptr;
    const std::size_t idx = static_cast<std::size_t>(z) *
                            static_cast<std::size_t>(s.m_nTileWidth) +
                            static_cast<std::size_t>(x);
    return &s.m_tiles[idx];
}

TileIndex fixed_tile_info_get_index(float fx, float fz, float tile_size) {
    TileIndex out{};
    if (tile_size <= 0.0f) return out;
    out.nx = static_cast<int>(fx / tile_size);
    out.nz = static_cast<int>(fz / tile_size);
    return out;
}

}  // namespace mxh::server

namespace {
[[maybe_unused]] constexpr int fixed_tile_info_translation_unit_anchor = 0;
}
