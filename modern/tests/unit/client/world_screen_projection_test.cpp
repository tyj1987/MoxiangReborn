#include "WorldScreenProjection.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

mxh::gx::MATRIX4 follow_camera(float far_plane = 100.0f) {
    const mxh::gx::VECTOR3 eye{25.0f, 7.0f, 18.0f};
    const mxh::gx::VECTOR3 at{25.0f, 0.0f, 25.0f};
    const mxh::gx::VECTOR3 up{0.0f, 1.0f, 0.0f};
    mxh::gx::MATRIX4 view{};
    mxh::gx::MatrixLookAtLH(&view, &eye, &at, &up);
    mxh::gx::MATRIX4 projection{};
    const float f = 1.0f / std::tan(mxh::gx::PI / 6.0f);
    projection._11 = f / (4.0f / 3.0f);
    projection._22 = f;
    projection._33 = far_plane / (far_plane - 0.1f);
    projection._34 = 1.0f;
    projection._43 = -0.1f * far_plane / (far_plane - 0.1f);
    mxh::gx::MATRIX4 result{};
    mxh::gx::MatrixMultiply2(&result, &view, &projection);
    return result;
}

TEST(WorldScreenProjection, VisibleLowerHalfGroundRoundTripsThroughSameCamera) {
    const auto camera = follow_camera();
    const mxh::client::TerrainHeightQuery flat = [](float, float) { return 0.0f; };
    const auto screen = mxh::client::project_world_point(
        camera, {0.0f, 0.0f}, flat, 25000.0f, 22000.0f);
    ASSERT_TRUE(screen);
    ASSERT_GT(screen->y, 300.0f);
    ASSERT_LT(screen->y, 600.0f);
    const auto picked = mxh::client::pick_ground_point(
        camera, {0.0f, 0.0f}, flat,
        screen->x, screen->y, 50000.0f, 50000.0f, 100.0f);
    ASSERT_TRUE(picked);
    EXPECT_NEAR(picked->x, 25000.0f, 2.0f);
    EXPECT_NEAR(picked->z, 22000.0f, 2.0f);
}

TEST(WorldScreenProjection, TerrainHeightAndMapCenterUseRendererUnits) {
    const auto camera = follow_camera();
    const mxh::client::TerrainHeightQuery raised = [](float, float) { return 300.0f; };
    const auto screen = mxh::client::project_world_point(
        camera, {5.0f, 7.0f}, raised, 30000.0f, 29000.0f, 100.0f);
    ASSERT_TRUE(screen);
    const auto picked = mxh::client::pick_ground_point(
        camera, {5.0f, 7.0f}, raised,
        screen->x, screen->y, 60000.0f, 60000.0f, 100.0f);
    ASSERT_TRUE(picked);
    // The projected point is 100 raw units above ground, so the actual ray
    // intersects the ground slightly farther away rather than inventing an
    // exact inverse for a head-height label.
    EXPECT_NEAR(picked->x, 30000.0f, 2.0f);
    EXPECT_GT(picked->z, 29000.0f);
}

TEST(WorldScreenProjection, UpperLeftAndRightGroundUseActualRay) {
    const auto camera = follow_camera();
    const mxh::client::TerrainHeightQuery flat = [](float, float) { return 0.0f; };
    for (const float x : {23000.0f, 27000.0f}) {
        const auto screen = mxh::client::project_world_point(
            camera, {0.0f, 0.0f}, flat, x, 30000.0f);
        ASSERT_TRUE(screen);
        EXPECT_LT(screen->y, 300.0f);
        EXPECT_GT(screen->x, 0.0f);
        EXPECT_LT(screen->x, 800.0f);
        const auto picked = mxh::client::pick_ground_point(
            camera, {0.0f, 0.0f}, flat,
            screen->x, screen->y, 50000.0f, 50000.0f, 100.0f);
        ASSERT_TRUE(picked);
        EXPECT_NEAR(picked->x, x, 2.0f);
        EXPECT_NEAR(picked->z, 30000.0f, 2.0f);
    }
}

TEST(WorldScreenProjection, FirstHflGridRidgeWinsOverFartherFlatGround) {
    const auto camera = follow_camera(800.0f);
    // One 100-unit HFL sample raised above its two adjacent Z samples.
    // The long far plane must not skip this narrow, first visible crossing.
    const mxh::client::TerrainHeightQuery ridge = [](float, float z) {
        return std::max(0.0f,
            6000.0f * (1.0f - std::abs(z - 22000.0f) / 100.0f));
    };
    const auto picked = mxh::client::pick_ground_point(
        camera, {0.0f, 0.0f}, ridge, 400.0f, 300.0f,
        50000.0f, 50000.0f, 100.0f);
    ASSERT_TRUE(picked);
    EXPECT_GT(picked->z, 21900.0f);
    EXPECT_LT(picked->z, 22100.0f);
}

TEST(WorldScreenProjection, InvalidViewportMatrixOrTerrainFailsClosed) {
    const auto camera = follow_camera();
    const mxh::client::TerrainHeightQuery flat = [](float, float) { return 0.0f; };
    EXPECT_FALSE(mxh::client::pick_ground_point(
        camera, {0.0f, 0.0f}, flat, -1.0f, 300.0f, 50000.0f, 50000.0f, 100.0f));
    EXPECT_FALSE(mxh::client::pick_ground_point(
        camera, {0.0f, 0.0f}, flat, 400.0f, 600.0f, 50000.0f, 50000.0f, 100.0f));
    EXPECT_FALSE(mxh::client::pick_ground_point(
        camera, {0.0f, 0.0f}, flat, 800.0f, 300.0f, 50000.0f, 50000.0f, 100.0f));
    EXPECT_FALSE(mxh::client::pick_ground_point(
        camera, {0.0f, 0.0f}, flat,
        std::numeric_limits<float>::quiet_NaN(), 300.0f, 50000.0f, 50000.0f, 100.0f));
    EXPECT_FALSE(mxh::client::pick_ground_point(
        mxh::gx::MATRIX4{}, {0.0f, 0.0f}, flat,
        400.0f, 350.0f, 50000.0f, 50000.0f, 100.0f));
    EXPECT_FALSE(mxh::client::pick_ground_point(
        camera, {0.0f, 0.0f},
        [](float, float) { return std::numeric_limits<float>::quiet_NaN(); },
        400.0f, 350.0f, 50000.0f, 50000.0f, 100.0f));
    EXPECT_FALSE(mxh::client::pick_ground_point(
        camera, {0.0f, 0.0f}, flat,
        400.0f, 350.0f, 50000.0f, 50000.0f, 0.0f));
    EXPECT_FALSE(mxh::client::project_world_point(
        camera, {0.0f, 0.0f}, flat,
        std::numeric_limits<float>::infinity(), 22000.0f));
    EXPECT_FALSE(mxh::client::project_world_point(
        camera, {0.0f, 0.0f}, flat, 25000.0f, 10000.0f));
    const mxh::client::TerrainHeightQuery near_eye = [](float, float) {
        return 6950.0f;
    };
    EXPECT_FALSE(mxh::client::project_world_point(
        camera, {0.0f, 0.0f}, near_eye, 25000.0f, 18050.0f));
    EXPECT_FALSE(mxh::client::project_world_point(
        camera, {0.0f, 0.0f}, flat, 25000.0f, 200000.0f));
}

}  // namespace
