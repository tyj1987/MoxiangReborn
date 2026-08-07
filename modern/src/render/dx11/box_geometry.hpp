#pragma once
#include <array>
#include <cstdint>
#include "mxh/render/render_typedef.hpp"
namespace mxh::gx::dx11 {
struct BoxLineVertex { VECTOR3 position{}; std::uint32_t color = 0u; };
inline std::array<BoxLineVertex, 24> make_box_line_vertices(const VECTOR3* oct, std::uint32_t color) noexcept {
    std::array<BoxLineVertex, 24> vertices{};
    if (oct == nullptr) return vertices;
    constexpr int edges[12][2] = {{0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}};
    for (int edge = 0; edge < 12; ++edge) {
        vertices[edge * 2] = {oct[edges[edge][0]], color};
        vertices[edge * 2 + 1] = {oct[edges[edge][1]], color};
    }
    return vertices;
}
}
