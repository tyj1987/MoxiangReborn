// TtbTileTable.cpp - Skeleton .ttb parser.

#include "mxh/compat/ttb_tile_table.hpp"

#include <cstring>
#include <fstream>

namespace mxh::compat {

// Best-effort parse: try fixed-header format first, fall back to "raw u32 grid".
// Full format reverse from MHMap.cpp TBD in Phase 1.2.
TtbTileTable TtbTileTable::parse(std::span<const std::uint8_t> bytes) {
    TtbTileTable t;
    if (bytes.size() < 8) return t;

    // Try header: { u32 width, u32 height, u32[N] tiles }.
    std::uint32_t w = 0, h = 0;
    std::memcpy(&w, bytes.data() + 0, 4);
    std::memcpy(&h, bytes.data() + 4, 4);

    const std::size_t expected = 8ull + static_cast<std::size_t>(w) * h * 4ull;
    if (w > 0 && w < 10000 && h > 0 && h < 10000 && expected == bytes.size()) {
        t.width = w;
        t.height = h;
        t.tiles.resize(static_cast<std::size_t>(w) * h);
        std::memcpy(t.tiles.data(), bytes.data() + 8, t.tiles.size() * 4);
        return t;
    }

    // Fallback: assume pure u32 grid; size must be a multiple of 4.
    if (bytes.size() % 4 != 0) return t;
    const auto n = bytes.size() / 4;
    t.tiles.resize(n);
    std::memcpy(t.tiles.data(), bytes.data(), bytes.size());
    // Best-effort width/height: try square, then common aspect ratios.
    t.width = static_cast<std::uint32_t>(n);
    t.height = 1;
    return t;
}

TtbTileTable TtbTileTable::load(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return {};
    const auto size = static_cast<std::size_t>(f.tellg());
    f.seekg(0);
    std::vector<std::uint8_t> buf(size);
    f.read(reinterpret_cast<char*>(buf.data()), size);
    return parse(buf);
}

}  // namespace mxh::compat