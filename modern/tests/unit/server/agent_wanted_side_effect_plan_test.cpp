// D4.102 -- AgentWanted side-effect plan unit tests.
//
// 1:1 lock the legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_WANTEDServerMsgParser (around line 2397). Each test pins one branch of the
// legacy dispatch to its modern side-effect plan output so future drift triggers
// a test failure.
//

#include <gtest/gtest.h>

#include "mxh/server/agent_wanted.hpp"
#include "mxh/server/agent_wanted_side_effect_plan.hpp"

using namespace mxh::server;

// ---------------------------------------------------------------------
// SERVER side-effects
// ---------------------------------------------------------------------

TEST(WantedServerPlan, DropNoUserEmitsDropEffect) {
    WantedAction action{};
    action.kind = WantedServerActionKind::drop_no_user;
    action.protocol = wanted_notcomplete_to_agent;
    action.object_id = 42u;
    const auto plan = wanted_side_effect_plan(action);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, WantedSideEffectKind::Drop);
    EXPECT_EQ(plan.effects[0].object_id, 42u);
    EXPECT_EQ(plan.effects[0].target_map_connection_index, 0u);
}

TEST(WantedServerPlan, NotifyDeleteToMapBroadcastsToOtherMaps) {
    WantedAction action{};
    action.kind = WantedServerActionKind::broadcast_to_other_maps;
    action.protocol = wanted_notify_delete_to_map;
    const auto plan = wanted_side_effect_plan(action);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, WantedSideEffectKind::BroadcastToOtherMaps);
    EXPECT_TRUE(wanted_effect_targets_map(plan.effects[0]));
    EXPECT_FALSE(wanted_effect_targets_client(plan.effects[0]));
}

TEST(WantedServerPlan, NotifyRegistToMapBroadcastsToOtherMaps) {
    WantedAction action{};
    action.kind = WantedServerActionKind::broadcast_to_other_maps;
    action.protocol = wanted_notify_regist_to_map;
    const auto plan = wanted_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, WantedSideEffectKind::BroadcastToOtherMaps);
}

TEST(WantedServerPlan, NotifyNotCompleteToMapBroadcastsToOtherMaps) {
    WantedAction action{};
    action.kind = WantedServerActionKind::broadcast_to_other_maps;
    action.protocol = wanted_notify_notcomplete_to_map;
    const auto plan = wanted_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, WantedSideEffectKind::BroadcastToOtherMaps);
}

TEST(WantedServerPlan, DestroyedToMapBroadcastsToOtherMaps) {
    WantedAction action{};
    action.kind = WantedServerActionKind::broadcast_to_other_maps;
    action.protocol = wanted_destroyed_to_map;
    const auto plan = wanted_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, WantedSideEffectKind::BroadcastToOtherMaps);
}

TEST(WantedServerPlan, NotCompleteToAgentWithUserSendsByDelChrToMap) {
    WantedAction action{};
    action.kind = WantedServerActionKind::complete_notcomplete_send_to_map;
    action.protocol = wanted_notcomplete_by_delchr;
    action.object_id = 7u;
    action.target_map_connection_index = 3u;
    const auto plan = wanted_side_effect_plan(action);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, WantedSideEffectKind::SendNotCompleteByDelChrToMap);
    EXPECT_EQ(plan.effects[0].object_id, 7u);
    EXPECT_EQ(plan.effects[0].target_map_connection_index, 3u);
    EXPECT_TRUE(wanted_effect_targets_map(plan.effects[0]));
}

TEST(WantedServerPlan, NotCompleteToAgentNoUserDrops) {
    WantedAction action{};
    action.kind = WantedServerActionKind::drop_no_user;
    action.protocol = wanted_notcomplete_to_agent;
    action.object_id = 7u;
    const auto plan = wanted_side_effect_plan(action);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, WantedSideEffectKind::Drop);
    EXPECT_FALSE(wanted_effect_targets_map(plan.effects[0]));
    EXPECT_FALSE(wanted_effect_targets_client(plan.effects[0]));
}

TEST(WantedServerPlan, UnknownProtocolForwardsToClient) {
    WantedAction action{};
    action.kind = WantedServerActionKind::default_forward_to_client;
    action.protocol = 99u;
    const auto plan = wanted_side_effect_plan(action);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, WantedSideEffectKind::ForwardRawToClient);
    EXPECT_TRUE(wanted_effect_targets_client(plan.effects[0]));
    EXPECT_FALSE(wanted_effect_targets_map(plan.effects[0]));
}

// ---------------------------------------------------------------------
// Classifier 1:1 tests (data plane -> plan)
// ---------------------------------------------------------------------

TEST(WantedClassifierPlan, NotifyDeleteEmitsBroadcastPlan) {
    WantedRequest req{};
    req.protocol = wanted_notify_delete_to_map;
    req.user_found = true;
    req.object_id = 11u;
    const auto action = classify_wanted(req);
    const auto plan = wanted_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, WantedSideEffectKind::BroadcastToOtherMaps);
}

TEST(WantedClassifierPlan, NotCompleteToAgentWithUserEmitsDelChrPlan) {
    WantedRequest req{};
    req.protocol = wanted_notcomplete_to_agent;
    req.user_found = true;
    req.object_id = 11u;
    req.target_map_connection_index = 5u;
    const auto action = classify_wanted(req);
    EXPECT_EQ(action.kind, WantedServerActionKind::complete_notcomplete_send_to_map);
    EXPECT_EQ(action.target_map_connection_index, 5u);
    const auto plan = wanted_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, WantedSideEffectKind::SendNotCompleteByDelChrToMap);
    EXPECT_EQ(plan.effects[0].target_map_connection_index, 5u);
}

TEST(WantedClassifierPlan, NotCompleteToAgentNoUserEmitsDropPlan) {
    WantedRequest req{};
    req.protocol = wanted_notcomplete_to_agent;
    req.user_found = false;
    req.object_id = 11u;
    const auto action = classify_wanted(req);
    EXPECT_EQ(action.kind, WantedServerActionKind::drop_no_user);
    const auto plan = wanted_side_effect_plan(action);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, WantedSideEffectKind::Drop);
}

TEST(WantedClassifierPlan, UnknownProtocolEmitsForwardToClientPlan) {
    WantedRequest req{};
    req.protocol = 200u;
    const auto action = classify_wanted(req);
    EXPECT_EQ(action.kind, WantedServerActionKind::default_forward_to_client);
    const auto plan = wanted_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, WantedSideEffectKind::ForwardRawToClient);
}

TEST(WantedClassifierPlan, AllFourBroadcastProtocolsRouteToBroadcast) {
    for (auto proto : {wanted_notify_delete_to_map, wanted_notify_regist_to_map, wanted_notify_notcomplete_to_map, wanted_destroyed_to_map}) {
        WantedRequest req{};
        req.protocol = proto;
        const auto action = classify_wanted(req);
        EXPECT_EQ(action.kind, WantedServerActionKind::broadcast_to_other_maps);
        const auto plan = wanted_side_effect_plan(action);
        EXPECT_EQ(plan.effects[0].kind, WantedSideEffectKind::BroadcastToOtherMaps);
    }
}