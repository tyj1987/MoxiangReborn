// TtbTileTable.cpp - legacy .ttb tile table parser.

#include "mxh/compat/ttb_tile_table.hpp"

#include <cstring>
#include <fstream>

namespace mxh::compat {

// Best-effort parse: try fixed-header format first, fall back to "raw u32 grid".
// Full format reverse from MHMap.cpp TBD in Phase 1.2.
TtbTileTable TtbTileTable::parse(std::span<const std::uint8_t> bytes) {
    TtbTileTable t;
    if (bytes.size() < 8) return t;

    // The shipped maps use {u32 width, u32 height, u16[N] tiles}.  Older
    // tools emitted a u32 tile array, so accept that byte-exact variant too;
    // both are normalized to the public u32 tile index vector.
    std::uint32_t w = 0, h = 0;
    std::memcpy(&w, bytes.data() + 0, 4);
    std::memcpy(&h, bytes.data() + 4, 4);

    const auto valid_dimensions = w > 0 && w < 10000 && h > 0 && h < 10000;
    const auto count = static_cast<std::size_t>(w) * h;
    const std::size_t expected16 = 8ull + count * 2ull;
    if (valid_dimensions && expected16 == bytes.size()) {
        t.width = w;
        t.height = h;
        t.tiles.resize(count);
        for (std::size_t i = 0; i < count; ++i) {
            const auto* p = bytes.data() + 8 + i * 2;
            t.tiles[i] = static_cast<std::uint32_t>(p[0]) |
                         (static_cast<std::uint32_t>(p[1]) << 8);
        }
        return t;
    }
    const std::size_t expected32 = 8ull + count * 4ull;
    if (valid_dimensions && expected32 == bytes.size()) {
        t.width = w;
        t.height = h;
        t.tiles.resize(count);
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
