#include "mxh/compat/hfl_height_field.hpp"

#include <cstring>
#include <limits>

namespace mxh::compat {
namespace {
// sizeof(HFIELD_DESC) in the original Win32 build. Three serialized pointer
// slots are four bytes each; the modern 64-bit in-memory struct must not be
// used to determine this size.
constexpr std::size_t kDiskDescSize = 108;
constexpr std::size_t kTextureNameSize = 128;

void fail(std::string* error, const char* message) { if (error) *error = message; }

template <typename T>
bool read(std::span<const std::uint8_t> bytes, std::size_t offset, T& value) {
    if (offset > bytes.size() || sizeof(T) > bytes.size() - offset) return false;
    std::memcpy(&value, bytes.data() + offset, sizeof(T));
    return true;
}
}

bool parse_hfl(std::span<const std::uint8_t> bytes, HflHeightField& output,
               std::string* error) {
    output = {};
    if (bytes.size() < 4 + kDiskDescSize) { fail(error, "truncated HFL header"); return false; }
    read(bytes, 0, output.version);
    if (output.version == 0 || output.version > 0x10) { fail(error, "unsupported HFL version"); return false; }
    const std::size_t d = 4;
    auto& v = output.desc;
    read(bytes, d + 0, v.left); read(bytes, d + 4, v.top);
    read(bytes, d + 8, v.right); read(bytes, d + 12, v.bottom); read(bytes, d + 16, v.face_size);
    read(bytes, d + 20, v.faces_per_object_axis); read(bytes, d + 24, v.object_count_x);
    read(bytes, d + 28, v.object_count_z); read(bytes, d + 32, v.detail_level_count);
    std::uint8_t blend = 0; read(bytes, d + 35, blend); v.blend_enabled = blend != 0;
    read(bytes, d + 36, v.index_buffer_count_l0);
    std::uint32_t textureCount = 0; read(bytes, d + 44, textureCount);
    read(bytes, d + 52, v.height_count_x); read(bytes, d + 56, v.height_count_z);
    read(bytes, d + 60, v.width); read(bytes, d + 64, v.height);
    read(bytes, d + 68, v.face_count_x); read(bytes, d + 72, v.face_count_z);
    read(bytes, d + 76, v.triangles_per_object); read(bytes, d + 80, v.vertices_per_object);
    read(bytes, d + 88, v.faces_per_tile_axis); read(bytes, d + 92, v.tiles_per_object_axis);
    read(bytes, d + 96, v.tile_count_x); read(bytes, d + 100, v.tile_count_z);
    read(bytes, d + 104, v.tile_size);

    const std::uint64_t heightCount = static_cast<std::uint64_t>(v.height_count_x) * v.height_count_z;
    if (heightCount == 0 || heightCount > 16'000'000u || textureCount > 65536u) {
        fail(error, "implausible HFL dimensions"); return false;
    }
    std::size_t cursor = 4 + kDiskDescSize;
    const std::size_t heightBytes = static_cast<std::size_t>(heightCount) * sizeof(float);
    if (heightBytes > bytes.size() - cursor) { fail(error, "truncated HFL heights"); return false; }
    output.heights.resize(static_cast<std::size_t>(heightCount));
    std::memcpy(output.heights.data(), bytes.data() + cursor, heightBytes);
    cursor += heightBytes;

    std::uint32_t storedTextureCount = 0;
    if (!read(bytes, cursor, storedTextureCount) || storedTextureCount != textureCount) {
        fail(error, "invalid HFL texture table"); return false;
    }
    cursor += 4;
    output.textures.reserve(textureCount);
    for (std::uint32_t i = 0; i < textureCount; ++i) {
        if (cursor + 2 + kTextureNameSize > bytes.size()) { fail(error, "truncated HFL texture table"); return false; }
        HflTexture texture;
        read(bytes, cursor, texture.index);
        const char* name = reinterpret_cast<const char*>(bytes.data() + cursor + 2);
        const auto length = strnlen(name, kTextureNameSize);
        texture.name.assign(name, length);
        output.textures.push_back(std::move(texture));
        cursor += 2 + kTextureNameSize;
    }

    std::uint32_t tileX = 0, tileZ = 0;
    if (!read(bytes, cursor, tileX) || !read(bytes, cursor + 4, tileZ) ||
        tileX != v.tile_count_x || tileZ != v.tile_count_z) {
        fail(error, "invalid HFL tile table dimensions"); return false;
    }
    cursor += 8;
    const std::uint64_t tileCount = static_cast<std::uint64_t>(tileX) * tileZ;
    if (tileCount > 16'000'000u || tileCount * 2u > bytes.size() - cursor) {
        fail(error, "truncated HFL tile table"); return false;
    }
    output.tiles.resize(static_cast<std::size_t>(tileCount));
    std::memcpy(output.tiles.data(), bytes.data() + cursor, output.tiles.size() * 2u);
    cursor += output.tiles.size() * 2u;
    output.alpha_header.assign(bytes.begin() + cursor, bytes.end());
    return true;
}

} // namespace mxh::compat
