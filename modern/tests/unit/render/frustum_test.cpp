// mxh/tests/unit/render/frustum_test.cpp
// G5 M-R5 frustum culling: unit tests for the header-only Frustum class.
//
// The math under test (Gribb-Hartmann plane extraction, AABB p-vertex
// test) is the same as the legacy 4DyuchiGRX culling.inl but with a strict
// C++17 interface, so the tests pin the modern implementation against
// the legacy semantics: any AABB that intersects the visible half-space of
// every plane must be reported as visible, and any AABB that lies fully
// outside any single plane must be reported as culled.

#include "mxh/render/frustum.hpp"
#include "mxh/render/math.hpp"

#include <gtest/gtest.h>

#include <cmath>

namespace {

// Build a perspective view-projection matrix that points down -Z from the
// origin with a 90-degree vertical FOV, aspect 1, near 0.1, far 100. This
// is intentionally simple: a point at (0, 0, -50) should land near the
// center of the frustum, while (0, 0, -200) lies behind the far plane.
//
// The project uses the row-vector-times-matrix convention v' = v * M
// (see math.hpp). In row-major storage where m._ij is row-i, col-j,
// the i-th component of v' is the dot product of v with M's i-th column.
// For OpenGL [-1, 1] clip-space (z_v in [-zf, -zn] maps to ndc_z in [-1, 1])
// with a right-handed camera looking -Z, the perspective matrix is:
//
//   clip_z = -(zf+zn)/(zf-zn) * z_v - 2*zn*zf/(zf-zn) * w_v
//   clip_w = -1 * z_v + 0 * w_v
//
// so under v' = v * M:
//   m._33 (z-coeff of z')  = -(zf+zn)/(zf-zn)
//   m._43 (w-coeff of z')  = -2*zn*zf/(zf-zn)        (the "translation" in z')
//   m._34 (z-coeff of w')  = -1                       (the perspective-divide row's z entry)
//   m._44 (w-coeff of w')  = 0
//   m._13 = m._23 = m._14 = m._24 = m._44 = 0
//
// Sanity check at the near plane (z_v = -zn = -0.1):
//   clip_z = -1.002*(-0.1) - 0.2002 = 0.1002 - 0.2002 = -0.1
//   clip_w = -(-0.1) = 0.1
//   ndc_z  = -0.1 / 0.1 = -1
// At the far plane (z_v = -100):
//   clip_z = -1.002*(-100) - 0.2002 = 100.2 - 0.2002 = 100.0
//   clip_w = 100
//   ndc_z  = 100 / 100 = 1
// Both match the OpenGL [-1, 1] convention. The Gribb-Hartmann
// extraction in `Frustum::extractFromViewProj` is the column-based
// variant for v' = v * M (see frustum.hpp), so it expects exactly this
// layout.
mxh::gx::MATRIX4 makeSimpleViewProj() {
    using namespace mxh::gx;
    const float zn = 0.1f;
    const float zf = 100.0f;
    MATRIX4 p{};
    p._11 = 1.0f;  // sy=1/tan(45deg)=1, aspect=1
    p._22 = 1.0f;
    p._33 = -(zf + zn) / (zf - zn);
    p._34 = -1.0f;                                  // z-coefficient of clip_w
    p._43 = -2.0f * zn * zf / (zf - zn);            // w-coefficient of clip_z (translation)
    p._44 = 0.0f;
    return p;
}

mxh::gx::VECTOR3 v3(float x, float y, float z) {
    return mxh::gx::VECTOR3{x, y, z};
}

} // namespace

TEST(FrustumExtract, IdentityLikePlanesAreNormalized) {
    using namespace mxh::gx;
    Frustum f(makeSimpleViewProj());
    for (std::size_t i = 0; i < Frustum::kPlaneCount; ++i) {
        const auto& p = f.plane(i);
        const float len = std::sqrt(p.normal.x * p.normal.x +
                                    p.normal.y * p.normal.y +
                                    p.normal.z * p.normal.z);
        EXPECT_NEAR(len, 1.0f, 1e-4f)
            << "plane " << i << " normal is not unit length (len=" << len << ")";
    }
}

TEST(FrustumIntersectsAABB, BoxAtOriginInFrontOfCameraIsVisible) {
    using namespace mxh::gx;
    Frustum f(makeSimpleViewProj());
    // 1m cube centered at the origin, fully in front of the camera
    // (z = [-2, -1], well past the near plane at z = -0.1).
    const VECTOR3 mn{-0.5f, -0.5f, -2.0f};
    const VECTOR3 mx{ 0.5f,  0.5f, -1.0f};
    EXPECT_TRUE(f.intersectsAABB(mn, mx));
}

TEST(FrustumIntersectsAABB, BoxFarBehindFarPlaneIsCulled) {
    using namespace mxh::gx;
    Frustum f(makeSimpleViewProj());
    // 1m cube placed well beyond the far plane (zf=100).
    const VECTOR3 mn{-0.5f, -0.5f, -250.0f};
    const VECTOR3 mx{ 0.5f,  0.5f, -200.0f};
    EXPECT_FALSE(f.intersectsAABB(mn, mx));
}

TEST(FrustumIntersectsAABB, BoxBehindCameraIsCulled) {
    using namespace mxh::gx;
    Frustum f(makeSimpleViewProj());
    const VECTOR3 mn{-0.5f, -0.5f, 5.0f};
    const VECTOR3 mx{ 0.5f,  0.5f, 10.0f};
    EXPECT_FALSE(f.intersectsAABB(mn, mx));
}

TEST(FrustumIntersectsAABB, BoxStraddlingNearPlaneIsVisible) {
    using namespace mxh::gx;
    Frustum f(makeSimpleViewProj());
    // Near plane sits at z = -zn = -0.1. A box that straddles it is
    // partially inside, so the p-vertex test must still report visible.
    // The box z range goes from -0.3 (in front of near) to +0.1
    // (behind the camera). At least the front half is inside.
    const VECTOR3 mn{-0.5f, -0.5f, -0.3f};
    const VECTOR3 mx{ 0.5f,  0.5f,  0.1f};
    EXPECT_TRUE(f.intersectsAABB(mn, mx));
}

TEST(FrustumIntersectsAABB, BoxOffToTheLeftOfLeftPlaneIsCulled) {
    using namespace mxh::gx;
    Frustum f(makeSimpleViewProj());
    // Camera FOV is 90 deg vertical + square aspect, so the left plane
    // passes through (0, 0, -zn) and the point (-1, 0, -1) lies *outside*
    // the visible half-space. A 1m box centered at (-5, 0, -1) must be
    // fully culled.
    const VECTOR3 mn{-5.5f, -0.5f, -1.5f};
    const VECTOR3 mx{-4.5f,  0.5f, -0.5f};
    EXPECT_FALSE(f.intersectsAABB(mn, mx));
}

TEST(FrustumIntersectsAABB, EmptyPlanesDontCrash) {
    // Defensive: a default-constructed Frustum has all-normal planes,
    // which means the p-vertex test reduces to "dist >= 0" for every
    // plane, so any box should pass. This guards against degenerate
    // matrices producing NaNs.
    using namespace mxh::gx;
    Frustum f;
    EXPECT_TRUE(f.intersectsAABB(v3(-1.0f, -1.0f, -1.0f), v3(1.0f, 1.0f, 1.0f)));
}
