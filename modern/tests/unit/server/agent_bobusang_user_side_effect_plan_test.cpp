// D4.107 -- AgentBobusang USER side-effect plan unit tests.
//
// 1:1 lock the legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_BOBUSANGUserMsgParser (lines 5159-5190). Each test pins one branch of the
// legacy dispatch to its modern side-effect plan output so future drift triggers
// a test failure.
//

#include <gtest/gtest.h>

#include "mxh/server/agent_bobusang_user.hpp"
#include "mxh/server/agent_bobusang_user_side_effect_plan.hpp"

using namespace mxh::server;

TEST(BobusangUserPlan, DropNoUserEmitsDropEffect) {
    BobusangUserAction a{};
    a.kind = BobusangUserActionKind::drop_no_user;
    a.object_id = 11u;
    const auto plan = bobusang_user_side_effect_plan(a);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, BobusangUserSideEffectKind::Drop);
    EXPECT_EQ(plan.effects[0].object_id, 11u);
}

TEST(BobusangUserPlan, DropWrongGmPowerEmitsDropEffect) {
    BobusangUserAction a{};
    a.kind = BobusangUserActionKind::drop_wrong_gm_power;
    a.object_id = 11u;
    const auto plan = bobusang_user_side_effect_plan(a);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, BobusangUserSideEffectKind::Drop);
    EXPECT_FALSE(bobusang_user_effect_targets_map(plan.effects[0]));
}

TEST(BobusangUserPlan, ForwardToMapEmitsRawForward) {
    BobusangUserAction a{};
    a.kind = BobusangUserActionKind::forward_to_map;
    a.protocol = 1u;
    a.object_id = 11u;
    const auto plan = bobusang_user_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, BobusangUserSideEffectKind::ForwardRawToMap);
    EXPECT_EQ(plan.effects[0].protocol, 1u);
    EXPECT_EQ(plan.effects[0].object_id, 11u);
    EXPECT_TRUE(bobusang_user_effect_targets_map(plan.effects[0]));
}

// Classifier 1:1 tests

TEST(BobusangUserClassifierPlan, NoUserEmitsDropPlan) {
    BobusangUserRequest req{};
    req.user_found = false;
    const auto action = classify_bobusang_user(req);
    EXPECT_EQ(action.kind, BobusangUserActionKind::drop_no_user);
    const auto plan = bobusang_user_side_effect_plan(action);
    EXPECT_TRUE(plan.drop);
}

TEST(BobusangUserClassifierPlan, GmAboveMasterEmitsDropPlan) {
    BobusangUserRequest req{};
    req.user_found = true;
    req.is_gm = true;
    req.gm_master_or_below = false;
    const auto action = classify_bobusang_user(req);
    EXPECT_EQ(action.kind, BobusangUserActionKind::drop_wrong_gm_power);
    const auto plan = bobusang_user_side_effect_plan(action);
    EXPECT_TRUE(plan.drop);
}

TEST(BobusangUserClassifierPlan, GmMasterOrBelowEmitsForwardPlan) {
    BobusangUserRequest req{};
    req.user_found = true;
    req.is_gm = true;
    req.gm_master_or_below = true;
    const auto action = classify_bobusang_user(req);
    EXPECT_EQ(action.kind, BobusangUserActionKind::forward_to_map);
    const auto plan = bobusang_user_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, BobusangUserSideEffectKind::ForwardRawToMap);
}

TEST(BobusangUserClassifierPlan, NonGmUserEmitsForwardPlan) {
    BobusangUserRequest req{};
    req.user_found = true;
    req.is_gm = false;
    const auto action = classify_bobusang_user(req);
    EXPECT_EQ(action.kind, BobusangUserActionKind::forward_to_map);
    const auto plan = bobusang_user_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, BobusangUserSideEffectKind::ForwardRawToMap);
}