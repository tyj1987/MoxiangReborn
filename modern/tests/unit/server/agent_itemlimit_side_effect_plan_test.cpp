// D4.110 -- AgentItemLimit side-effect plan unit tests.
//
// 1:1 lock the legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_ITEMLIMITServerMsgParser (lines 5211-5235).
//

#include <gtest/gtest.h>

#include "mxh/server/agent_itemlimit.hpp"
#include "mxh/server/agent_itemlimit_side_effect_plan.hpp"

using namespace mxh::server;

TEST(ItemLimitPlan, BroadcastToOtherMapsEmitsBroadcastEffect) {
    ItemLimitAction a{};
    a.kind = ItemLimitActionKind::broadcast_to_other_maps;
    a.protocol = itemlimit_addcount_to_map;
    const auto plan = itemlimit_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, ItemLimitSideEffectKind::BroadcastToOtherMaps);
    EXPECT_EQ(plan.effects[0].protocol, itemlimit_addcount_to_map);
    EXPECT_TRUE(itemlimit_effect_targets_map(plan.effects[0]));
    EXPECT_FALSE(itemlimit_effect_targets_client(plan.effects[0]));
}

TEST(ItemLimitPlan, ForwardToClientEmitsClientForwardEffect) {
    ItemLimitAction a{};
    a.kind = ItemLimitActionKind::forward_to_client;
    a.protocol = itemlimit_full_to_client;
    const auto plan = itemlimit_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, ItemLimitSideEffectKind::ForwardRawToClient);
    EXPECT_EQ(plan.effects[0].protocol, itemlimit_full_to_client);
    EXPECT_TRUE(itemlimit_effect_targets_client(plan.effects[0]));
    EXPECT_FALSE(itemlimit_effect_targets_map(plan.effects[0]));
}

TEST(ItemLimitClassifierPlan, AddCountToMapEmitsBroadcastPlan) {
    ItemLimitRequest req{};
    req.protocol = itemlimit_addcount_to_map;
    const auto action = classify_itemlimit(req);
    EXPECT_EQ(action.kind, ItemLimitActionKind::broadcast_to_other_maps);
    EXPECT_EQ(action.protocol, itemlimit_addcount_to_map);
    const auto plan = itemlimit_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, ItemLimitSideEffectKind::BroadcastToOtherMaps);
}

TEST(ItemLimitClassifierPlan, FullToClientEmitsForwardPlan) {
    ItemLimitRequest req{};
    req.protocol = itemlimit_full_to_client;
    const auto action = classify_itemlimit(req);
    EXPECT_EQ(action.kind, ItemLimitActionKind::forward_to_client);
    EXPECT_EQ(action.protocol, itemlimit_full_to_client);
    const auto plan = itemlimit_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, ItemLimitSideEffectKind::ForwardRawToClient);
}

TEST(ItemLimitClassifierPlan, UnknownProtocolEmitsForwardToClient) {
    ItemLimitRequest req{};
    req.protocol = 99u;
    const auto action = classify_itemlimit(req);
    EXPECT_EQ(action.kind, ItemLimitActionKind::forward_to_client);
    EXPECT_EQ(action.protocol, 99u);
    const auto plan = itemlimit_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, ItemLimitSideEffectKind::ForwardRawToClient);
}

TEST(ItemLimitClassifierPlan, ActionPreservesProtocolByte) {
    ItemLimitRequest req_a{};
    req_a.protocol = itemlimit_addcount_to_map;
    const auto action_a = classify_itemlimit(req_a);
    EXPECT_EQ(action_a.protocol, itemlimit_addcount_to_map);
    const auto plan_a = itemlimit_side_effect_plan(action_a);
    EXPECT_EQ(plan_a.effects[0].protocol, itemlimit_addcount_to_map);
    ItemLimitRequest req_b{};
    req_b.protocol = itemlimit_full_to_client;
    const auto action_b = classify_itemlimit(req_b);
    EXPECT_EQ(action_b.protocol, itemlimit_full_to_client);
    const auto plan_b = itemlimit_side_effect_plan(action_b);
    EXPECT_EQ(plan_b.effects[0].protocol, itemlimit_full_to_client);
}