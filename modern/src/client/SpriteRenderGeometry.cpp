#include "SpriteRenderGeometry.hpp"

#include <algorithm>
#include <cmath>

namespace mxh::client {

std::optional<SpriteRenderGeometry> compute_sprite_render_geometry(
    std::uint32_t texture_width,
    std::uint32_t texture_height,
    float target_width,
    float target_height,
    float u0,
    float v0,
    float u1,
    float v1) noexcept {
    if (texture_width == 0 || texture_height == 0 ||
        target_width <= 0.0f || target_height <= 0.0f) {
        return std::nullopt;
    }

    const auto pixel_x = [texture_width](float uv) {
        const float clamped = std::clamp(uv, 0.0f, 1.0f);
        return static_cast<std::int32_t>(
            std::lround(clamped * static_cast<float>(texture_width)));
    };
    const auto pixel_y = [texture_height](float uv) {
        const float clamped = std::clamp(uv, 0.0f, 1.0f);
        return static_cast<std::int32_t>(
            std::lround(clamped * static_cast<float>(texture_height)));
    };

    SpriteRenderGeometry geometry;
    geometry.source_left = pixel_x(u0);
    geometry.source_top = pixel_y(v0);
    geometry.source_right = pixel_x(u1);
    geometry.source_bottom = pixel_y(v1);
    const auto source_width = geometry.source_right - geometry.source_left;
    const auto source_height = geometry.source_bottom - geometry.source_top;
    if (source_width <= 0 || source_height <= 0) {
        return std::nullopt;
    }

    geometry.scale_x = target_width / static_cast<float>(source_width);
    geometry.scale_y = target_height / static_cast<float>(source_height);
    return geometry;
}

}  // namespace mxh::client
