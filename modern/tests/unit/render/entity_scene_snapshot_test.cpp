#include "mxh/render/EntityScene.hpp"

#include <gtest/gtest.h>

namespace mxh::gx {
namespace {

TEST(EntitySceneMotion, MapsLegacyOneBasedActionSlotsToChxIndices) {
    EXPECT_EQ(chooseSceneMotionIndex(
        SceneEntityType::Monster, false, SceneAction::Idle, 12), 0u);
    EXPECT_EQ(chooseSceneMotionIndex(
        SceneEntityType::Monster, false, SceneAction::Moving, 12), 1u);
    EXPECT_EQ(chooseSceneMotionIndex(
        SceneEntityType::Monster, false, SceneAction::Attack, 12), 2u);
    EXPECT_EQ(chooseSceneMotionIndex(
        SceneEntityType::Monster, false, SceneAction::Dead, 12), 8u);
    EXPECT_EQ(chooseSceneMotionIndex(
        SceneEntityType::Npc, false, SceneAction::Moving, 3), 0u);
    EXPECT_EQ(chooseSceneMotionIndex(
        SceneEntityType::Monster, true, SceneAction::Moving, 30), 2u);
    EXPECT_EQ(chooseSceneMotionIndex(
        SceneEntityType::Monster, true, SceneAction::Dead, 30), 25u);
    EXPECT_EQ(chooseSceneMotionIndex(
        SceneEntityType::Monster, true, SceneAction::Dead, 4), 0u);
}

TEST(EntitySceneSnapshot, SynchronizeReplacesAllRuntimeEntityClasses) {
    EntityScene scene;
    WorldSnapshot first;
    first.local_player = ScenePlayer{100u};
    first.remote_players.push_back(ScenePlayer{200u});
    first.remote_players.push_back(ScenePlayer{300u});
    first.entities.push_back(SceneEntity{400u, 7u});
    first.entities.push_back(
        SceneEntity{500u, 12u, 0, 0, 0, SceneEntityType::Npc});
    first.remote_players.back().facing_yaw = 1.25f;
    first.remote_players.back().current_life = 80u;
    first.remote_players.back().max_life = 100u;
    first.remote_players.back().action = SceneAction::Moving;

    scene.synchronize(first);

    EXPECT_EQ(scene.playerInstanceCount(), 3u);
    EXPECT_EQ(scene.instanceCount(), 2u);
    EXPECT_EQ(scene.npcInstanceCount(), 1u);

    WorldSnapshot second;
    second.local_player = ScenePlayer{100u};
    second.remote_players.push_back(ScenePlayer{300u});
    scene.synchronize(second);

    EXPECT_EQ(scene.playerInstanceCount(), 2u);
    EXPECT_EQ(scene.instanceCount(), 0u);
    EXPECT_EQ(scene.npcInstanceCount(), 0u);
}

TEST(EntitySceneSnapshot, EmptySnapshotRemovesLocalAndRemotePlayers) {
    EntityScene scene;
    WorldSnapshot populated;
    populated.local_player = ScenePlayer{10u};
    populated.remote_players.push_back(ScenePlayer{20u});
    scene.synchronize(populated);
    ASSERT_EQ(scene.playerInstanceCount(), 2u);

    scene.synchronize(WorldSnapshot{});

    EXPECT_EQ(scene.playerInstanceCount(), 0u);
}

TEST(EntitySceneSnapshot, MissingVisualIsCountedAsFailedLoadWithPlaceholder) {
    EntityScene scene;
    WorldSnapshot snap;
    snap.local_player = ScenePlayer{7u};
    snap.entities.push_back(
        SceneEntity{42u, 9999u, 0, 0, 0, SceneEntityType::Npc});
    scene.synchronize(snap);

    EXPECT_GE(scene.failedModelCount(), 1u);
    EXPECT_GE(scene.placeholderCount(), 1u);
    EXPECT_EQ(scene.loadedModelCount(), 0u);
}

} // namespace
} // namespace mxh::gx
