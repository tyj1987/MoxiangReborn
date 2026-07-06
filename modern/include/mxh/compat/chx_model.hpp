// ChxModel.hpp - .chx character model parser (skeleton).
// Original: 3ds Max Biped/Physique export via MAXEXP/MtlExp/anmexp plugins.

#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

namespace mxh::compat {

#pragma pack(push, 1)
struct ChxHeader {
    std::uint32_t magic;          // 'CHLX' or 'CHX\0'
    std::uint32_t version;
    std::uint32_t mesh_count;
    std::uint32_t bone_count;
    std::uint32_t material_count;
    std::uint32_t vertex_count;
    std::uint32_t index_count;
    std::uint32_t reserved;
};
#pragma pack(pop)

struct ChxModel {
    ChxHeader header{};
    std::vector<std::uint8_t> raw;       // entire file (for passthrough)
    std::vector<float> vertices;         // x,y,z triples
    std::vector<std::uint32_t> indices;
    // Mesh/material/bone decoded tables: TODO(Phase 1.3).

    [[nodiscard]] static bool is_chx(std::span<const std::uint8_t> bytes) noexcept;
    [[nodiscard]] static ChxModel parse(std::span<const std::uint8_t> bytes);
    [[nodiscard]] static ChxModel load(const std::filesystem::path& path);
};

}  // namespace mxh::compat