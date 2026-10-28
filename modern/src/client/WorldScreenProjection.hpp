#pragma once

#include "mxh/render/math.hpp"

#include <functional>
#include <optional>
#include <utility>

namespace mxh::client {

struct ScreenPoint {
    float x = 0.0f;
    float y = 0.0f;
};

struct GroundPoint {
    float x = 0.0f;
    float z = 0.0f;
};

using TerrainHeightQuery = std::function<float(float, float)>;

// The terrain and entity renderer use raw map units scaled by 0.001, then
// subtract TerrainScene::mapCenter() from X/Z before applying viewProj.
// These functions share exactly that transform with world interaction.
std::optional<ScreenPoint> project_world_point(
    const mxh::gx::MATRIX4& view_proj,
    std::pair<float, float> map_center,
    const TerrainHeightQuery& height_at,
    float world_x, float world_z, float height_offset_raw = 0.0f) noexcept;

// Cast a ray through the 800x600 logical canvas and return the first sampled
// crossing with the actual HFL terrain. Sampling is bounded to quarter-cell
// horizontal steps; sub-cell spikes and tangential contacts are not exact.
// No fallback camera approximation is used.
std::optional<GroundPoint> pick_ground_point(
    const mxh::gx::MATRIX4& view_proj,
    std::pair<float, float> map_center,
    const TerrainHeightQuery& height_at,
    float screen_x, float screen_y,
    float world_limit_x, float world_limit_z,
    float terrain_sample_spacing_raw) noexcept;

}  // namespace mxh::client
