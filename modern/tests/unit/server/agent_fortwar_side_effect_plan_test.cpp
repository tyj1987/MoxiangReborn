// D4.104 -- AgentFortWar side-effect plan unit tests.
//
// 1:1 lock the legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_FORTWARServerMsgParser (lines 5290-5452). Each test pins one branch of the
// legacy dispatch to its modern side-effect plan output so future drift triggers
// a test failure.
//

#include <gtest/gtest.h>

#include "mxh/server/agent_fortwar.hpp"
#include "mxh/server/agent_fortwar_side_effect_plan.hpp"

using namespace mxh::server;

// ---------------------------------------------------------------------
// SERVER side-effects
// ---------------------------------------------------------------------

TEST(FortWarPlan, DropNoUserEmitsDropEffect) {
    FortWarAction action{};
    action.kind = FortWarActionKind::drop_no_user;
    action.protocol = fortwar_info;
    const auto plan = fortwar_side_effect_plan(action);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, FortWarSideEffectKind::Drop);
    EXPECT_EQ(plan.effects[0].protocol, fortwar_info);
}

TEST(FortWarPlan, StartBefore10MinBroadcastsToAllUsers) {
    FortWarAction action{};
    action.kind = FortWarActionKind::broadcast_to_all_users;
    action.protocol = fortwar_start_before10min;
    const auto plan = fortwar_side_effect_plan(action);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, FortWarSideEffectKind::BroadcastToAllUsers);
    EXPECT_TRUE(fortwar_effect_targets_all(plan.effects[0]));
    EXPECT_FALSE(fortwar_effect_targets_map(plan.effects[0]));
    EXPECT_FALSE(fortwar_effect_targets_user(plan.effects[0]));
}

TEST(FortWarPlan, StartBroadcastsToAllUsers) {
    FortWarAction action{};
    action.kind = FortWarActionKind::broadcast_to_all_users;
    action.protocol = fortwar_start;
    const auto plan = fortwar_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, FortWarSideEffectKind::BroadcastToAllUsers);
}

TEST(FortWarPlan, EndBroadcastsToAllUsers) {
    FortWarAction action{};
    action.kind = FortWarActionKind::broadcast_to_all_users;
    action.protocol = fortwar_end;
    const auto plan = fortwar_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, FortWarSideEffectKind::BroadcastToAllUsers);
}

TEST(FortWarPlan, StartBefore10MinToMapBroadcastsToOtherMaps) {
    FortWarAction action{};
    action.kind = FortWarActionKind::broadcast_to_other_maps;
    action.protocol = fortwar_start_before10min_to_map;
    const auto plan = fortwar_side_effect_plan(action);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, FortWarSideEffectKind::BroadcastToOtherMaps);
    EXPECT_TRUE(fortwar_effect_targets_map(plan.effects[0]));
}

TEST(FortWarPlan, StartToMapBroadcastsToOtherMaps) {
    FortWarAction action{};
    action.kind = FortWarActionKind::broadcast_to_other_maps;
    action.protocol = fortwar_start_to_map;
    const auto plan = fortwar_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, FortWarSideEffectKind::BroadcastToOtherMaps);
}

TEST(FortWarPlan, IngToMapBroadcastsToOtherMaps) {
    FortWarAction action{};
    action.kind = FortWarActionKind::broadcast_to_other_maps;
    action.protocol = fortwar_ing_to_map;
    const auto plan = fortwar_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, FortWarSideEffectKind::BroadcastToOtherMaps);
}

TEST(FortWarPlan, EndToMapBroadcastsToOtherMaps) {
    FortWarAction action{};
    action.kind = FortWarActionKind::broadcast_to_other_maps;
    action.protocol = fortwar_end_to_map;
    const auto plan = fortwar_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, FortWarSideEffectKind::BroadcastToOtherMaps);
}

TEST(FortWarPlan, InfoWithUserForwardsToUser) {
    FortWarAction action{};
    action.kind = FortWarActionKind::forward_to_user_if_found;
    action.protocol = fortwar_info;
    const auto plan = fortwar_side_effect_plan(action);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, FortWarSideEffectKind::ForwardRawToUser);
    EXPECT_TRUE(fortwar_effect_targets_user(plan.effects[0]));
}

TEST(FortWarPlan, IngWithUserForwardsToUser) {
    FortWarAction action{};
    action.kind = FortWarActionKind::forward_to_user_if_found;
    action.protocol = fortwar_ing;
    const auto plan = fortwar_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, FortWarSideEffectKind::ForwardRawToUser);
}

// ---------------------------------------------------------------------
// Classifier 1:1 tests
// ---------------------------------------------------------------------

TEST(FortWarClassifierPlan, StartBefore10MinEmitsBroadcastAllPlan) {
    FortWarRequest req{};
    req.protocol = fortwar_start_before10min;
    const auto action = classify_fortwar(req);
    EXPECT_EQ(action.kind, FortWarActionKind::broadcast_to_all_users);
    const auto plan = fortwar_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, FortWarSideEffectKind::BroadcastToAllUsers);
}

TEST(FortWarClassifierPlan, StartToMapEmitsBroadcastOtherMapsPlan) {
    FortWarRequest req{};
    req.protocol = fortwar_start_to_map;
    const auto action = classify_fortwar(req);
    EXPECT_EQ(action.kind, FortWarActionKind::broadcast_to_other_maps);
    const auto plan = fortwar_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, FortWarSideEffectKind::BroadcastToOtherMaps);
}

TEST(FortWarClassifierPlan, InfoWithUserEmitsForwardToUserPlan) {
    FortWarRequest req{};
    req.protocol = fortwar_info;
    req.user_object_found = true;
    const auto action = classify_fortwar(req);
    EXPECT_EQ(action.kind, FortWarActionKind::forward_to_user_if_found);
    const auto plan = fortwar_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, FortWarSideEffectKind::ForwardRawToUser);
}

TEST(FortWarClassifierPlan, InfoNoUserEmitsDropPlan) {
    FortWarRequest req{};
    req.protocol = fortwar_info;
    req.user_object_found = false;
    const auto action = classify_fortwar(req);
    EXPECT_EQ(action.kind, FortWarActionKind::drop_no_user);
    const auto plan = fortwar_side_effect_plan(action);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, FortWarSideEffectKind::Drop);
}

TEST(FortWarClassifierPlan, IngWithUserEmitsForwardToUserPlan) {
    FortWarRequest req{};
    req.protocol = fortwar_ing;
    req.user_object_found = true;
    const auto action = classify_fortwar(req);
    EXPECT_EQ(action.kind, FortWarActionKind::forward_to_user_if_found);
    const auto plan = fortwar_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, FortWarSideEffectKind::ForwardRawToUser);
}

TEST(FortWarClassifierPlan, UnknownProtocolWithUserEmitsForwardToUserPlan) {
    FortWarRequest req{};
    req.protocol = 99u;
    req.user_object_found = true;
    const auto action = classify_fortwar(req);
    EXPECT_EQ(action.kind, FortWarActionKind::forward_to_user_if_found);
    const auto plan = fortwar_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, FortWarSideEffectKind::ForwardRawToUser);
}

TEST(FortWarClassifierPlan, UnknownProtocolNoUserEmitsDropPlan) {
    FortWarRequest req{};
    req.protocol = 99u;
    req.user_object_found = false;
    const auto action = classify_fortwar(req);
    EXPECT_EQ(action.kind, FortWarActionKind::drop_no_user);
    const auto plan = fortwar_side_effect_plan(action);
    EXPECT_TRUE(plan.drop);
}

TEST(FortWarClassifierPlan, AllThreeBroadcastAllProtocolsRouteCorrectly) {
    for (auto proto : {fortwar_start_before10min, fortwar_start, fortwar_end}) {
        FortWarRequest req{};
        req.protocol = proto;
        const auto action = classify_fortwar(req);
        EXPECT_EQ(action.kind, FortWarActionKind::broadcast_to_all_users);
        const auto plan = fortwar_side_effect_plan(action);
        EXPECT_EQ(plan.effects[0].kind, FortWarSideEffectKind::BroadcastToAllUsers);
    }
}

TEST(FortWarClassifierPlan, AllFourBroadcastToMapProtocolsRouteCorrectly) {
    for (auto proto : {fortwar_start_before10min_to_map, fortwar_start_to_map, fortwar_ing_to_map, fortwar_end_to_map}) {
        FortWarRequest req{};
        req.protocol = proto;
        const auto action = classify_fortwar(req);
        EXPECT_EQ(action.kind, FortWarActionKind::broadcast_to_other_maps);
        const auto plan = fortwar_side_effect_plan(action);
        EXPECT_EQ(plan.effects[0].kind, FortWarSideEffectKind::BroadcastToOtherMaps);
    }
}