#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace mxh::compat {

struct HflDesc {
    float left = 0, top = 0, right = 0, bottom = 0, face_size = 0;
    std::uint32_t faces_per_object_axis = 0;
    std::uint32_t object_count_x = 0, object_count_z = 0;
    std::uint8_t detail_level_count = 0;
    bool blend_enabled = false;
    std::uint32_t index_buffer_count_l0 = 0;
    std::uint32_t height_count_x = 0, height_count_z = 0;
    float width = 0, height = 0;
    std::uint32_t face_count_x = 0, face_count_z = 0;
    std::uint32_t triangles_per_object = 0, vertices_per_object = 0;
    std::uint32_t faces_per_tile_axis = 0, tiles_per_object_axis = 0;
    std::uint32_t tile_count_x = 0, tile_count_z = 0;
    float tile_size = 0;
};

struct HflTexture {
    std::uint16_t index = 0;
    std::string name;
};

struct HflHeightField {
    std::uint32_t version = 0;
    HflDesc desc;
    std::vector<float> heights;
    std::vector<HflTexture> textures;
    std::vector<std::uint16_t> tiles;
    std::vector<std::uint8_t> alpha_header;
};

// Parses the original 32-bit 4Dyuchi .hfl disk layout. Pointer fields stored
// in HFIELD_DESC are skipped; they are process addresses and always ignored.
[[nodiscard]] bool parse_hfl(std::span<const std::uint8_t> bytes,
                             HflHeightField& output,
                             std::string* error = nullptr);

} // namespace mxh::compat
