#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace mxh::compat {

struct StmMaterial {
    std::uint32_t index = 0;
    std::uint32_t diffuse = 0;
    float transparency = 0;
    std::string texture_name;
    std::uint32_t flags = 0;
};

struct StmFaceGroup {
    std::uint32_t material_index = 0;
    std::vector<std::uint16_t> indices;
};

struct StmInfluence {
    std::uint32_t bone_index = 0;
    float weight = 0;
    std::array<float, 3> offset{};
    std::array<float, 3> normal_offset{};
};

struct StmBone {
    std::uint32_t index = 0;
    std::uint32_t parent_index = 0xffffffffu;
    std::string name;
    float rotation_angle = 0;
    std::array<float, 3> position{};
    std::array<float, 3> rotation_axis{};
    std::array<float, 3> scale{1, 1, 1};
    std::array<float, 16> transform{};
};

struct StmMesh {
    std::uint32_t index = 0;
    std::string name;
    std::array<float, 16> transform{};
    std::uint32_t mesh_flags = 0;
    std::vector<std::array<float, 3>> positions;
    std::vector<std::array<float, 3>> normals;
    std::vector<std::array<float, 2>> texcoords;
    std::vector<StmFaceGroup> face_groups;
    std::vector<std::vector<StmInfluence>> physique;
};

struct StmStaticModel {
    std::uint32_t version = 0;
    std::vector<StmMaterial> materials;
    std::vector<StmMesh> meshes;
    std::vector<StmBone> bones;
    std::size_t collision_offset = 0;
};

// Reads the original 32-bit STATIC_MODEL_VERSION=1 disk representation.
// Runtime pointers serialized by the old engine are treated as 4-byte slots.
[[nodiscard]] bool parse_stm(std::span<const std::uint8_t> bytes,
                             StmStaticModel& output,
                             std::string* error = nullptr);

// Reads a regular 4Dyuchi .mod scene and exposes its mesh/material subset in
// the same representation used by static map scenes.
[[nodiscard]] bool parse_mod(std::span<const std::uint8_t> bytes,
                             StmStaticModel& output,
                             std::string* error = nullptr);

} // namespace mxh::compat
