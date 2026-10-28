// mxh/render/frustum.hpp
// G5 M-R5 frustum culling.
//
// A `Frustum` is six planes (left/right/top/bottom/near/far) extracted from
// a view-projection matrix. An axis-aligned bounding box (AABB) is visible
// if and only if it intersects at least one half-space of every plane,
// i.e. the box's "p-vertex" (the corner that maximizes dot(plane.normal, x))
// is on the inside half-space of every plane.
//
// Convention: the project's MATRIX4 is row-major storage with the D3DX
// row-vector-times-matrix convention `v' = v * M` (see math.hpp). For that
// convention each clip-space component v'[i] is the dot product of v with
// the i-th column of M. The standard Gribb-Hartmann plane-extraction trick
// expresses each clip-space half-space as a linear combination of M's rows
// when the convention is `v' = M * v` (column-vector). For the project's
// row-vector convention the analogous combination is over M's COLUMNS:
//   left  (x' + w' >= 0)  ->  col1 + col4
//   right (-x' + w' >= 0)  -> -col1 + col4
//   bot   (y' + w' >= 0)  ->  col2 + col4
//   top   (-y' + w' >= 0)  -> -col2 + col4
//   near  (z' + w' >= 0)  ->  col3 + col4
//   far   (-z' + w' >= 0)  -> -col3 + col4
// (D3DX [0,1] depth convention would flip the near/far signs to `z' - w'`
// and `-z' - w'`; we assume OpenGL [-1,1] which is what the perspective
// matrix in tests/builders produces.)
//
// Each plane is normalized (length 1) so `intersectsAABB` can compare
// distances without an extra scale step. The AABB test is the standard
// "p-vertex" (Larsen/Baraff) test: for each plane, pick the corner of the
// box that maximizes dot(plane.normal, x) and require its signed distance
// to be >= 0.
//
// Why this lives in include/ and not src/: the renderer pipeline touches
// Frustum from `entity_scene.cpp` and `terrain_scene.cpp` (future hook for
// static scene culling), so a header-only interface keeps call sites
// dependency-free.

#pragma once

#include "mxh/render/math.hpp"

#include <array>

namespace mxh::gx {

// A single infinite plane. `normal` points inward (towards the visible
// half-space); `d` is the constant such that `dot(normal, x) + d >= 0` for
// any visible point x.
struct Plane {
    VECTOR3 normal{0.0f, 1.0f, 0.0f};
    float d = 0.0f;
};

class Frustum {
public:
    // Six planes in the order: left, right, bottom, top, near, far.
    // The convention matches the legacy "view frustum" culling used in
    // 4DyuchiGRX culling.inl (planes ordered for early-out iteration).
    enum : std::size_t {
        kLeft = 0,
        kRight = 1,
        kBottom = 2,
        kTop = 3,
        kNear = 4,
        kFar = 5,
        kPlaneCount = 6,
    };

    Frustum() = default;
    // Extract the six planes from a row-major view-projection matrix.
    // `viewProj` is expected to be `view * projection` (D3DX convention).
    // Each plane is normalized (length 1) so `intersectsAABB` can compare
    // distances without an extra scale step.
    explicit Frustum(const MATRIX4& viewProj) noexcept {
        extractFromViewProj(viewProj);
    }

    void extractFromViewProj(const MATRIX4& m) noexcept {
        // Gribb-Hartmann plane extraction adapted to the project's
        // row-vector-times-matrix convention (v' = v * M). Under that
        // convention the i-th column of M gives the coefficients for the
        // i-th component of v', so each plane is a linear combination of
        // M's columns. See the file header for the full derivation.
        const std::array<Plane, 6> raw{{
            // Left:   col1 + col4  (x' + w' >= 0)
            {VECTOR3{m._11 + m._14, m._21 + m._24, m._31 + m._34}, m._41 + m._44},
            // Right:  -col1 + col4 (-x' + w' >= 0)
            {VECTOR3{m._14 - m._11, m._24 - m._21, m._34 - m._31}, m._44 - m._41},
            // Bottom: col2 + col4  (y' + w' >= 0)
            {VECTOR3{m._12 + m._14, m._22 + m._24, m._32 + m._34}, m._42 + m._44},
            // Top:    -col2 + col4 (-y' + w' >= 0)
            {VECTOR3{m._14 - m._12, m._24 - m._22, m._34 - m._32}, m._44 - m._42},
            // Near:   col3 + col4  (z' + w' >= 0, OpenGL [-1, 1] clip-space)
            {VECTOR3{m._13 + m._14, m._23 + m._24, m._33 + m._34}, m._43 + m._44},
            // Far:    -col3 + col4 (-z' + w' >= 0)
            {VECTOR3{m._14 - m._13, m._24 - m._23, m._34 - m._33}, m._44 - m._43},
        }};
        for (std::size_t i = 0; i < kPlaneCount; ++i) {
            const float len = std::sqrt(raw[i].normal.x * raw[i].normal.x +
                                         raw[i].normal.y * raw[i].normal.y +
                                         raw[i].normal.z * raw[i].normal.z);
            if (len > 1e-6f) {
                planes_[i].normal.x = raw[i].normal.x / len;
                planes_[i].normal.y = raw[i].normal.y / len;
                planes_[i].normal.z = raw[i].normal.z / len;
                planes_[i].d = raw[i].d / len;
            } else {
                planes_[i] = raw[i];
            }
        }
    }

    [[nodiscard]] const Plane& plane(std::size_t i) const noexcept { return planes_[i]; }
    [[nodiscard]] const std::array<Plane, kPlaneCount>& planes() const noexcept { return planes_; }

    // True iff the AABB {min, max} intersects the frustum's visible
    // half-space. The standard "p-vertex" test: for each plane, the
    // corner of the box that is *most aligned* with the plane's inward
    // normal must still be inside (dot(n, p) + d >= 0). If even one
    // plane fully rejects the box, return false.
    [[nodiscard]] bool intersectsAABB(const VECTOR3& boxMin,
                                       const VECTOR3& boxMax) const noexcept {
        for (const auto& p : planes_) {
            // p-vertex: the corner of the box that maximizes dot(n, x).
            const VECTOR3 pv{
                (p.normal.x >= 0.0f) ? boxMax.x : boxMin.x,
                (p.normal.y >= 0.0f) ? boxMax.y : boxMin.y,
                (p.normal.z >= 0.0f) ? boxMax.z : boxMin.z,
            };
            const float dist = p.normal.x * pv.x + p.normal.y * pv.y +
                               p.normal.z * pv.z + p.d;
            if (dist < 0.0f) return false;
        }
        return true;
    }

private:
    std::array<Plane, kPlaneCount> planes_{};
};

} // namespace mxh::gx
