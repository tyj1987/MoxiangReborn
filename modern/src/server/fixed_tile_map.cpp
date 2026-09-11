#include "mxh/server/fixed_tile_map.hpp"

#include <cmath>
#include <fstream>
#include <limits>

namespace mxh::server {
namespace {
std::uint32_t read_u32(const std::uint8_t* p) {
    return static_cast<std::uint32_t>(p[0]) | (static_cast<std::uint32_t>(p[1]) << 8) |
        (static_cast<std::uint32_t>(p[2]) << 16) | (static_cast<std::uint32_t>(p[3]) << 24);
}
bool valid_world(float x, float z) {
    return std::isfinite(x) && std::isfinite(z) && x >= 0 && z >= 0 &&
        x < FixedTileMap::world_extent && z < FixedTileMap::world_extent;
}
}

std::optional<FixedTileMap> FixedTileMap::decode(std::span<const std::uint8_t> bytes, std::string& error) {
    error.clear();
    if (bytes.size() < 8) { error = "fixed tile header is truncated"; return std::nullopt; }
    const auto width = read_u32(bytes.data()), height = read_u32(bytes.data() + 4);
    // Negative signed dimensions also exceed this bound; compute length in u64
    // before allocation so malformed headers cannot overflow on an x86 server.
    if (!width || !height || width > max_dimension || height > max_dimension) {
        error = "fixed tile dimensions are invalid or exceed the bounded loader";
        return std::nullopt;
    }
    const auto count = static_cast<std::uint64_t>(width) * height;
    if (8u + count * 2u != bytes.size()) {
        error = "fixed tile WORD payload length does not match dimensions";
        return std::nullopt;
    }
    FixedTileMap result;
    if (!fixed_tile_info_init(result.tiles_, static_cast<int>(width), static_cast<int>(height))) {
        error = "fixed tile grid initialization failed"; return std::nullopt;
    }
    result.attributes_.reserve(static_cast<std::size_t>(count));
    for (std::size_t i = 0; i < count; ++i) {
        const auto attr = static_cast<std::uint16_t>(bytes[8 + i * 2]) |
            (static_cast<std::uint16_t>(bytes[9 + i * 2]) << 8);
        result.attributes_.push_back(static_cast<std::uint16_t>(attr));
        fixed_tile_init(result.tiles_.m_tiles[i], static_cast<std::uint8_t>(attr));
    }
    return result;
}

std::optional<FixedTileMap> FixedTileMap::load(const std::filesystem::path& path, std::string& error) {
    error.clear();
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) { error = "fixed tile file cannot be opened"; return std::nullopt; }
    const auto size = file.tellg();
    constexpr std::uint64_t max_bytes = 8u + static_cast<std::uint64_t>(max_dimension) * max_dimension * 2u;
    if (size < 8 || static_cast<std::uint64_t>(size) > max_bytes) {
        error = "fixed tile file size is invalid"; return std::nullopt;
    }
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    file.seekg(0);
    if (!file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size())) ||
        file.peek() != std::char_traits<char>::eof()) {
        error = "fixed tile file changed or could not be fully read"; return std::nullopt;
    }
    return decode(bytes, error);
}

bool FixedTileMap::blocked_cell(int x, int z) const noexcept {
    const auto* tile = fixed_tile_info_get(tiles_, x, z);
    return !tile || fixed_tile_is_collision(*tile);
}

bool FixedTileMap::blocked(float x, float z) const noexcept {
    if (!valid_world(x, z)) return true;
    return blocked_cell(static_cast<int>(x / cell_size), static_cast<int>(z / cell_size));
}

FixedTileTrace FixedTileMap::trace(float start_x, float start_z, float end_x, float end_z) const noexcept {
    if (!valid_world(start_x, start_z)) return {};
    // Defined fail-closed extension for nonfinite/out-of-world destinations.
    if (!valid_world(end_x, end_z)) return {true, start_x, start_z};
    int x = static_cast<int>(start_x / cell_size), z = static_cast<int>(start_z / cell_size);
    const int x1 = static_cast<int>(end_x / cell_size), z1 = static_cast<int>(end_z / cell_size);
    if (x >= static_cast<int>(width()) || z >= static_cast<int>(height()) ||
        x1 >= static_cast<int>(width()) || z1 >= static_cast<int>(height()))
        return {true, start_x, start_z};
    const int sx = x1 >= x ? 1 : -1, sz = z1 >= z ? 1 : -1;
    const int dx = std::abs(x1 - x), dz = std::abs(z1 - z), ax = 2 * dx, az = 2 * dz;
    int last_x = x, last_z = z;
    bool first = dx != 0 || dz != 0;
    if (dx >= dz) {
        for (int desc = az - dx; ; x += sx, desc += az) {
            if (first) { first = false; continue; }
            if (blocked_cell(x, z)) return {true, last_x * cell_size, last_z * cell_size};
            if (x == x1) break;
            last_x = x; last_z = z;
            if (desc > 0) { z += sz; desc -= ax; }
        }
    } else {
        for (int desc = ax - dz; ; z += sz, desc += ax) {
            if (first) { first = false; continue; }
            if (blocked_cell(x, z)) return {true, last_x * cell_size, last_z * cell_size};
            if (z == z1) break;
            last_x = x; last_z = z;
            if (desc > 0) { x += sx; desc -= az; }
        }
    }
    return {false, end_x, end_z};
}

} // namespace mxh::server
