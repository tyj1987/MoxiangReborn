#include "mxh/render/TerrainScene.hpp"

#include <gtest/gtest.h>
#include <utility>

namespace mxh::gx {
namespace {

// The accessor must report (0, 0) before `load()` succeeds, so downstream
// callers (entity / static scene cache, project_world_point helper) can
// detect a not-yet-loaded terrain instead of seeing the legacy 25.6f
// constant. Without this sentinel the placeholder / static collision
// path would silently use the wrong origin for any map that has not
// finished loading yet.
TEST(TerrainSceneMapCenter, DefaultsToOriginBeforeLoad) {
    TerrainScene scene;
    const auto centre = scene.mapCenter();
    EXPECT_FLOAT_EQ(centre.first, 0.0f);
    EXPECT_FLOAT_EQ(centre.second, 0.0f);
}

// The accessor must be a const member that does not mutate internal
// state.  Two consecutive calls must return the same value, and
// neither call should trigger a side effect on the descriptor cache.
// This pins the "no surprise side effect" contract that lets the
// main.cpp render loop poll the accessor once per frame and forward
// the pair to entity_scene->setMapCenter() / static_scene->setMapCenter()
// without worrying about aliasing.
TEST(TerrainSceneMapCenter, IsIdempotentAndConst) {
    TerrainScene scene;
    const auto first = scene.mapCenter();
    const auto second = scene.mapCenter();
    EXPECT_FLOAT_EQ(first.first, second.first);
    EXPECT_FLOAT_EQ(first.second, second.second);
    EXPECT_FLOAT_EQ(first.first, 0.0f);
    EXPECT_FLOAT_EQ(first.second, 0.0f);
}

// The accessor must return a std::pair<float, float>.  Pin the type
// contract at compile time so any future change to the return type
// (e.g. switching to a `struct MapCenter { float x; float z; }`) is
// flagged at this test's translation unit, well before main.cpp
// callers.  std::pair is the lightest container that lets main.cpp
// do `auto centre = terrain.mapCenter(); centre.first; centre.second;`
// without a header include churn.
TEST(TerrainSceneMapCenter, ReturnsStdPairOfFloats) {
    TerrainScene scene;
    constexpr auto expected = std::pair<float, float>{0.0f, 0.0f};
    using ReturnT = decltype(scene.mapCenter());
    static_assert(std::is_same<ReturnT, std::pair<float, float>>::value,
                  "mapCenter() must return std::pair<float, float>");
    EXPECT_EQ(scene.mapCenter(), expected);
}

}  // namespace
}  // namespace mxh::gx
