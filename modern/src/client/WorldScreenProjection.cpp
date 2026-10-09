#include "WorldScreenProjection.hpp"

#include <cmath>

namespace mxh::client {
namespace {
constexpr double kSceneScale = 0.001;
constexpr int kLogicalWidth = 800;
constexpr int kLogicalHeight = 600;

struct ScenePoint { double x, y, z; };

bool invert(const mxh::gx::MATRIX4& matrix, double out[4][4]) noexcept {
    double work[4][8]{};
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            if (!std::isfinite(matrix.m[row][col])) return false;
            work[row][col] = matrix.m[row][col];
        }
        work[row][row + 4] = 1.0;
    }
    for (int col = 0; col < 4; ++col) {
        int pivot = col;
        for (int row = col + 1; row < 4; ++row) {
            if (std::abs(work[row][col]) > std::abs(work[pivot][col])) pivot = row;
        }
        if (std::abs(work[pivot][col]) < 1e-12) return false;
        if (pivot != col) {
            for (int at = 0; at < 8; ++at) {
                const auto value = work[col][at];
                work[col][at] = work[pivot][at];
                work[pivot][at] = value;
            }
        }
        const double divisor = work[col][col];
        for (auto& value : work[col]) value /= divisor;
        for (int row = 0; row < 4; ++row) {
            if (row == col) continue;
            const double factor = work[row][col];
            for (int at = 0; at < 8; ++at) {
                work[row][at] -= factor * work[col][at];
            }
        }
    }
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            out[row][col] = work[row][col + 4];
            if (!std::isfinite(out[row][col])) return false;
        }
    }
    return true;
}

std::optional<ScenePoint> unproject(double inverse[4][4],
                                    double ndc_x, double ndc_y,
                                    double ndc_z) noexcept {
    const double clip[4]{ndc_x, ndc_y, ndc_z, 1.0};
    double world[4]{};
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            world[col] += clip[row] * inverse[row][col];
        }
    }
    if (!std::isfinite(world[3]) || std::abs(world[3]) < 1e-12) return std::nullopt;
    ScenePoint point{world[0] / world[3], world[1] / world[3], world[2] / world[3]};
    if (!std::isfinite(point.x) || !std::isfinite(point.y) ||
        !std::isfinite(point.z)) return std::nullopt;
    return point;
}
}  // namespace

std::optional<ScreenPoint> project_world_point(
    const mxh::gx::MATRIX4& matrix, std::pair<float, float> center,
    const TerrainHeightQuery& height_at,
    float world_x, float world_z, float height_offset_raw) noexcept {
    if (!height_at || !std::isfinite(world_x) || !std::isfinite(world_z) ||
        !std::isfinite(height_offset_raw) ||
        !std::isfinite(center.first) || !std::isfinite(center.second)) return std::nullopt;
    const double raw_height = height_at(world_x, world_z);
    if (!std::isfinite(raw_height)) return std::nullopt;
    const double x = world_x * kSceneScale - center.first;
    const double y = (raw_height + height_offset_raw) * kSceneScale;
    const double z = world_z * kSceneScale - center.second;
    const double clip_x = x * matrix._11 + y * matrix._21 + z * matrix._31 + matrix._41;
    const double clip_y = x * matrix._12 + y * matrix._22 + z * matrix._32 + matrix._42;
    const double clip_w = x * matrix._14 + y * matrix._24 + z * matrix._34 + matrix._44;
    if (!std::isfinite(clip_x) || !std::isfinite(clip_y) ||
        !std::isfinite(clip_w) || clip_w <= 1e-6) return std::nullopt;
    const double sx = (clip_x / clip_w + 1.0) * (kLogicalWidth * 0.5);
    const double sy = (1.0 - clip_y / clip_w) * (kLogicalHeight * 0.5);
    if (!std::isfinite(sx) || !std::isfinite(sy)) return std::nullopt;
    return ScreenPoint{static_cast<float>(sx), static_cast<float>(sy)};
}

std::optional<GroundPoint> pick_ground_point(
    const mxh::gx::MATRIX4& matrix, std::pair<float, float> center,
    const TerrainHeightQuery& height_at,
    float screen_x, float screen_y,
    float world_limit_x, float world_limit_z) noexcept {
    if (!height_at || !std::isfinite(screen_x) || !std::isfinite(screen_y) ||
        !std::isfinite(center.first) || !std::isfinite(center.second) ||
        !std::isfinite(world_limit_x) || !std::isfinite(world_limit_z) ||
        screen_x < 0.0f || screen_x >= kLogicalWidth ||
        screen_y < 0.0f || screen_y >= kLogicalHeight ||
        world_limit_x <= 0.0f || world_limit_z <= 0.0f) return std::nullopt;
    double inverse[4][4]{};
    if (!invert(matrix, inverse)) return std::nullopt;
    const double ndc_x = screen_x / (kLogicalWidth * 0.5) - 1.0;
    const double ndc_y = 1.0 - screen_y / (kLogicalHeight * 0.5);
    const auto near = unproject(inverse, ndc_x, ndc_y, 0.0);
    const auto far = unproject(inverse, ndc_x, ndc_y, 1.0);
    if (!near || !far) return std::nullopt;
    const auto raw_at = [&](double t) -> GroundPoint {
        return {static_cast<float>(((near->x + (far->x - near->x) * t) + center.first) / kSceneScale),
                static_cast<float>(((near->z + (far->z - near->z) * t) + center.second) / kSceneScale)};
    };
    const auto delta = [&](double t) -> std::optional<double> {
        const auto point = raw_at(t);
        if (!std::isfinite(point.x) || !std::isfinite(point.z) ||
            point.x < 0.0f || point.z < 0.0f ||
            point.x > world_limit_x || point.z > world_limit_z) return std::nullopt;
        const double height = height_at(point.x, point.z);
        if (!std::isfinite(height)) return std::nullopt;
        return near->y + (far->y - near->y) * t - height * kSceneScale;
    };
    constexpr int kRaySteps = 1024;
    auto previous = delta(0.0);
    double previous_t = 0.0;
    for (int step = 1; step <= kRaySteps; ++step) {
        const double t = static_cast<double>(step) / kRaySteps;
        const auto current = delta(t);
        if (previous && current && *previous > 0.0 && *current <= 0.0) {
            double low = previous_t, high = t;
            for (int iteration = 0; iteration < 24; ++iteration) {
                const double middle = (low + high) * 0.5;
                const auto probe = delta(middle);
                if (!probe) return std::nullopt;
                if (*probe > 0.0) low = middle;
                else high = middle;
            }
            return raw_at(high);
        }
        previous = current;
        previous_t = t;
    }
    return std::nullopt;
}

}  // namespace mxh::client
