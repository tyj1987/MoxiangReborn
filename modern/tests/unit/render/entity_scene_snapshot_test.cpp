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
    snap.local_player = ScenePlayer{7u, 0, 0, 0, {}, 12.0f, 3.0f, 8.0f};
    snap.entities.push_back(
        SceneEntity{42u, 9999u, 1.0f, 2.0f, 3.0f, SceneEntityType::Npc});
    scene.synchronize(snap);

    EXPECT_EQ(scene.playerInstanceCount(), 1u);
    EXPECT_EQ(scene.npcInstanceCount(), 1u);
    EXPECT_GE(scene.failedModelCount(), 1u);
    EXPECT_GE(scene.placeholderCount(), 1u);
    EXPECT_EQ(scene.loadedModelCount(), 0u);

    bool saw_npc = false;
    bool saw_player = false;
    for (const auto& placeholder : scene.placeholders()) {
        EXPECT_GT(placeholder.radius, 0.0f);
        if (placeholder.object_id == 42u) {
            saw_npc = true;
            EXPECT_EQ(placeholder.world_x, 1.0f);
            EXPECT_EQ(placeholder.world_z, 3.0f);
            EXPECT_EQ(placeholder.kind, PlaceholderKind::Npc);
        }
        if (placeholder.object_id == 7u) {
            saw_player = true;
            EXPECT_EQ(placeholder.kind, PlaceholderKind::Player);
        }
    }
    EXPECT_TRUE(saw_npc);
    EXPECT_TRUE(saw_player);
}

TEST(EntityScenePlaceholder, FillOctUsesSceneScaleAndKindColor) {
    EXPECT_EQ(placeholderArgb(PlaceholderKind::Player), 0xFF40E0FFu);
    EXPECT_EQ(placeholderArgb(PlaceholderKind::Npc), 0xFFFFD700u);
    EXPECT_EQ(placeholderArgb(PlaceholderKind::Monster), 0xFFFF4040u);

    // Monster_10.bin group 1 spawn 0: kind 105 at (44653, 7829).
    PlaceholderVisual visual{
        100001u, 44653.0f, 0.0f, 7829.0f, 0.5f, PlaceholderKind::Monster};
    VECTOR3 oct[8]{};
    fillPlaceholderOct(visual, oct);

    const float tx = 44653.0f * kEntitySceneScale - kEntityMapCenter;
    const float tz = 7829.0f * kEntitySceneScale - kEntityMapCenter;
    EXPECT_NEAR(oct[0].x, tx - 0.5f, 1.0e-4f);
    EXPECT_NEAR(oct[0].y, 0.0f, 1.0e-4f);
    EXPECT_NEAR(oct[0].z, tz - 0.5f, 1.0e-4f);
    EXPECT_NEAR(oct[6].x, tx + 0.5f, 1.0e-4f);
    EXPECT_NEAR(oct[6].y, 1.0f, 1.0e-4f);
    EXPECT_NEAR(oct[6].z, tz + 0.5f, 1.0e-4f);
    EXPECT_GT(oct[6].x - oct[0].x, 0.0f);
    EXPECT_GT(oct[6].y - oct[0].y, 0.0f);
    EXPECT_GT(oct[6].z - oct[0].z, 0.0f);
}

TEST(EntityScenePlaceholder, MonsterSpawnStaysDrawableWithoutCatalog) {
    EntityScene scene;
    WorldSnapshot snap;
    snap.entities.push_back(SceneEntity{
        100001u, 105u, 44653.0f, 0.0f, 7829.0f, SceneEntityType::Monster});
    scene.synchronize(snap);

    ASSERT_EQ(scene.placeholderCount(), 1u);
    const auto& placeholder = scene.placeholders().front();
    EXPECT_EQ(placeholder.object_id, 100001u);
    EXPECT_EQ(placeholder.kind, PlaceholderKind::Monster);
    EXPECT_GT(placeholder.radius, 0.0f);

    VECTOR3 oct[8]{};
    fillPlaceholderOct(placeholder, oct);
    EXPECT_GT(oct[6].y - oct[0].y, 0.0f);
    scene.render();
}

} // namespace
} // namespace mxh::gx
