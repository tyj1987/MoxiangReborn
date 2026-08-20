#pragma once

#include <cstdint>
#include <optional>

namespace mxh::client {

struct SpriteRenderGeometry {
    std::int32_t source_left = 0;
    std::int32_t source_top = 0;
    std::int32_t source_right = 0;
    std::int32_t source_bottom = 0;
    float scale_x = 1.0f;
    float scale_y = 1.0f;
};

// Converts normalized cImage UVs back into the pixel source rectangle used by
// the legacy IDISpriteObject API, then derives scale from target/source size.
[[nodiscard]] std::optional<SpriteRenderGeometry> compute_sprite_render_geometry(
    std::uint32_t texture_width,
    std::uint32_t texture_height,
    float target_width,
    float target_height,
    float u0,
    float v0,
    float u1,
    float v1) noexcept;

}  // namespace mxh::client
