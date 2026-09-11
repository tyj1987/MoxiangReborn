#pragma once

#include "mxh/server/fixed_tile_info.hpp"
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace mxh::server {

struct FixedTileTrace {
    bool collision = true;
    float last_x = 25000.0f;
    float last_z = 25000.0f;
};

// Server Resource/Map/<map>.ttb: i32 width/height followed by WORD attributes.
// This is not a texture index table. Preserve all WORD bits even though ordinary
// ground collision uses the original CFixedTile low-byte TFA_COLLISON flag.
class FixedTileMap {
public:
    static constexpr float cell_size = 50.0f;
    static constexpr float world_extent = 51200.0f;
    static constexpr std::uint32_t max_dimension = 4096;
    static std::optional<FixedTileMap> decode(std::span<const std::uint8_t> bytes, std::string& error);
    static std::optional<FixedTileMap> load(const std::filesystem::path& path, std::string& error);

    std::uint32_t width() const noexcept { return static_cast<std::uint32_t>(tiles_.m_nTileWidth); }
    std::uint32_t height() const noexcept { return static_cast<std::uint32_t>(tiles_.m_nTileHeight); }
    std::span<const std::uint16_t> attributes() const noexcept { return attributes_; }
    bool blocked(float x, float z) const noexcept;
    // Exact normal-input scan from TileManager.cpp:557-699, including its first
    // cell skip and strict desc>0 tie rule. Caller must also test the endpoint;
    // the original OneTarget handler does both checks. last_x/last_z are only
    // meaningful on collision; successful scans do not reproduce legacy output writes.
    FixedTileTrace trace(float start_x, float start_z, float end_x, float end_z) const noexcept;

private:
    bool blocked_cell(int x, int z) const noexcept;
    FixedTileInfoState tiles_;
    std::vector<std::uint16_t> attributes_;
};

} // namespace mxh::server
