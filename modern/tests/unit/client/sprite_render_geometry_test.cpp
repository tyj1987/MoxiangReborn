#include <gtest/gtest.h>

#include "SpriteRenderGeometry.hpp"

namespace mxh::client {
namespace {

TEST(SpriteRenderGeometryTest, PreservesAtlasSourceRectAndTargetSize) {
    const auto geometry = compute_sprite_render_geometry(
        1024, 512, 197.0f, 345.0f,
        505.0f / 1024.0f, 0.0f, 702.0f / 1024.0f, 345.0f / 512.0f);
    ASSERT_TRUE(geometry.has_value());
    EXPECT_EQ(geometry->source_left, 505);
    EXPECT_EQ(geometry->source_top, 0);
    EXPECT_EQ(geometry->source_right, 702);
    EXPECT_EQ(geometry->source_bottom, 345);
    EXPECT_FLOAT_EQ(geometry->scale_x, 1.0f);
    EXPECT_FLOAT_EQ(geometry->scale_y, 1.0f);
}

TEST(SpriteRenderGeometryTest, ScalesBySourceRegionRatherThanTextureSize) {
    const auto geometry = compute_sprite_render_geometry(
        64, 64, 128.0f, 32.0f, 0.25f, 0.25f, 0.75f, 0.75f);
    ASSERT_TRUE(geometry.has_value());
    EXPECT_EQ(geometry->source_left, 16);
    EXPECT_EQ(geometry->source_top, 16);
    EXPECT_EQ(geometry->source_right, 48);
    EXPECT_EQ(geometry->source_bottom, 48);
    EXPECT_FLOAT_EQ(geometry->scale_x, 4.0f);
    EXPECT_FLOAT_EQ(geometry->scale_y, 1.0f);
}

TEST(SpriteRenderGeometryTest, RejectsEmptyOrReversedSourceRegions) {
    EXPECT_FALSE(compute_sprite_render_geometry(
        64, 64, 20.0f, 20.0f, 0.5f, 0.0f, 0.5f, 1.0f));
    EXPECT_FALSE(compute_sprite_render_geometry(
        64, 64, 20.0f, 20.0f, 0.75f, 0.0f, 0.25f, 1.0f));
}

}  // namespace
}  // namespace mxh::client
