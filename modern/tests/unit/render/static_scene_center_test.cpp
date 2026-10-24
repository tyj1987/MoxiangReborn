// Regression: Map 10 STM statics were baked with the legacy 25.6 centre while
// terrain uses desc.width * 0.001 * 0.5 = 25.0, shifting statics + STM
// collision by 0.6 scene units. Server collision ([Server]Map TileManager /
// mxh::server::fixed_tile_info_get_index) uses absolute world coords, so a
// world point must map to the same scene point for terrain and statics.
#include "mxh/render/StaticSceneCenter.hpp"

#include <gtest/gtest.h>

namespace sc = mxh::gx::static_center;

namespace {
// Mirrors TerrainScene::load: centre = desc.width * kSceneScale * 0.5.
constexpr sc::Center terrainCenter(float width, float height) {
    return {width * sc::kSceneScale * 0.5f, height * sc::kSceneScale * 0.5f};
}
}

TEST(StaticSceneCenter, Map10TerrainCenterIs25) {
    const auto c = terrainCenter(50000.0f, 50000.0f);
    EXPECT_FLOAT_EQ(c.x, 25.0f);
    EXPECT_FLOAT_EQ(c.z, 25.0f);
}

TEST(StaticSceneCenter, Map10StaticMatchesTerrainScenePoint) {
    const auto c = terrainCenter(50000.0f, 50000.0f);
    // Server world point (absolute, tile = int(x / 50)) at the map middle.
    const float wx = 25000.0f, wz = 25000.0f;
    EXPECT_FLOAT_EQ(sc::worldToSceneX(wx, c), 0.0f);
    EXPECT_FLOAT_EQ(sc::worldToSceneZ(wz, c), 0.0f);
    // A static vertex at world (12345, 40000) lands where terrain puts it.
    const float terrain_x = 12345.0f * 0.001f - 25.0f;
    const float terrain_z = 40000.0f * 0.001f - 25.0f;
    EXPECT_NEAR(sc::worldToSceneX(12345.0f, c), terrain_x, 1e-5f);
    EXPECT_NEAR(sc::worldToSceneZ(40000.0f, c), terrain_z, 1e-5f);
}

TEST(StaticSceneCenter, LegacyDefaultWouldDriftBy0_6OnMap10) {
    const sc::Center legacy{};  // 25.6 fallback
    const auto map10 = terrainCenter(50000.0f, 50000.0f);
    EXPECT_FALSE(sc::sameCenter(legacy, map10));
    EXPECT_NEAR(sc::worldToSceneX(25000.0f, map10) - sc::worldToSceneX(25000.0f, legacy),
                0.6f, 1e-5f);
}

TEST(StaticSceneCenter, Map12StillUses25_6) {
    const auto c = terrainCenter(51200.0f, 51200.0f);
    EXPECT_TRUE(sc::sameCenter(c, sc::Center{}));
}
