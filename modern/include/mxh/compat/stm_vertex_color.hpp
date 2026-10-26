// mxh/compat/stm_vertex_color.hpp
// DX-free helper for the legacy <map>.vcl static-model vertex-color file.
//
// Legacy contract (D:\MX original source):
//  * CoStaticModel.cpp:1151-1155 / 1184-1186 and geom_obj.cpp:962-980 read
//    one D3DCOLOR (4 bytes) per vertex, in STM mesh order, for every mesh
//    that is neither SHADE_TYPE_LIGHT_MAP (3) nor TRANSFORM_TYPE_ALIGN_VIEW
//    (0x30, billboard). A missing file means memset(0xff) -> white.
//  * VLSMeshObject.cpp:75-78 stores the colour in the VB diffuse slot and
//    VLSMeshObject.cpp:154 renders COLOROP=MODULATE (1x), with
//    CoD3DDevice.cpp:2102-2107 defaults ARG1=TEXTURE ARG2=DIFFUSE,
//    ALPHAOP=SELECTARG1(TEXTURE). So: rgb = tex.rgb * vcl.rgb, a = tex.a.
//
// A D3DCOLOR is an ARGB DWORD stored little-endian: bytes on disk are
// B, G, R, A.
#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace mxh::compat {

inline constexpr std::uint32_t kStmShadeMask      = 0x0000000fu;
inline constexpr std::uint32_t kStmShadeLightMap  = 0x00000003u;
inline constexpr std::uint32_t kStmTransformMask  = 0x000000f0u;
inline constexpr std::uint32_t kStmTransformAlign = 0x00000030u;
inline constexpr std::uint32_t kD3DColorWhite     = 0xffffffffu;

// True when the legacy loader consumes .vcl entries for this mesh.
inline constexpr bool stm_mesh_uses_vcl(std::uint32_t mesh_flags) noexcept {
    return (mesh_flags & kStmShadeMask) != kStmShadeLightMap &&
           (mesh_flags & kStmTransformMask) != kStmTransformAlign;
}

// Read one D3DCOLOR (ARGB DWORD) from 4 little-endian bytes B,G,R,A.
inline constexpr std::uint32_t read_d3dcolor(const std::uint8_t* p) noexcept {
    return static_cast<std::uint32_t>(p[0]) |
           (static_cast<std::uint32_t>(p[1]) << 8) |
           (static_cast<std::uint32_t>(p[2]) << 16) |
           (static_cast<std::uint32_t>(p[3]) << 24);
}

// Convert a D3DCOLOR (0xAARRGGBB) to a DXGI_FORMAT_R8G8B8A8_UNORM packed
// value (memory bytes R,G,B,A).
inline constexpr std::uint32_t d3dcolor_to_rgba8(std::uint32_t argb) noexcept {
    const std::uint32_t a = (argb >> 24) & 0xffu;
    const std::uint32_t r = (argb >> 16) & 0xffu;
    const std::uint32_t g = (argb >> 8) & 0xffu;
    const std::uint32_t b = argb & 0xffu;
    return r | (g << 8) | (b << 16) | (a << 24);
}

struct StmVclMeshInfo {
    std::uint32_t mesh_flags = 0;
    std::size_t vertex_count = 0;
};

enum class StmVclStatus { Missing, Matched, Mismatch };

struct StmVclAssignment {
    StmVclStatus status = StmVclStatus::Missing;
    std::size_t loaded_bytes = 0;
    std::size_t expected_vertices = 0;   // sum over vcl-consuming meshes
    // One entry per input mesh. D3DCOLOR values. Empty for meshes that do
    // not consume .vcl (lightmap / billboard). White unless Matched.
    std::vector<std::vector<std::uint32_t>> per_mesh;
    bool matched() const noexcept { return status == StmVclStatus::Matched; }
};

inline std::size_t stm_vcl_expected_vertices(std::span<const StmVclMeshInfo> meshes) noexcept {
    std::size_t n = 0;
    for (const auto& m : meshes) if (stm_mesh_uses_vcl(m.mesh_flags)) n += m.vertex_count;
    return n;
}

// Assign .vcl colours to meshes. `vcl` == nullptr means the file is missing.
// Size must be exactly expected_vertices * 4; otherwise status=Mismatch and
// every mesh gets white (misaligned colours are never applied).
inline StmVclAssignment assign_stm_vertex_colors(std::span<const StmVclMeshInfo> meshes,
                                                 const std::vector<std::uint8_t>* vcl) {
    StmVclAssignment out;
    out.expected_vertices = stm_vcl_expected_vertices(meshes);
    out.loaded_bytes = vcl ? vcl->size() : 0;
    if (!vcl) out.status = StmVclStatus::Missing;
    else if (vcl->size() == out.expected_vertices * 4u) out.status = StmVclStatus::Matched;
    else out.status = StmVclStatus::Mismatch;

    out.per_mesh.resize(meshes.size());
    std::size_t cursor = 0;
    for (std::size_t i = 0; i < meshes.size(); ++i) {
        if (!stm_mesh_uses_vcl(meshes[i].mesh_flags)) continue;
        auto& dst = out.per_mesh[i];
        dst.assign(meshes[i].vertex_count, kD3DColorWhite);
        if (out.matched()) {
            for (std::size_t v = 0; v < dst.size(); ++v)
                dst[v] = read_d3dcolor(vcl->data() + (cursor + v) * 4u);
        }
        cursor += meshes[i].vertex_count;
    }
    return out;
}

} // namespace mxh::compat
