// D4.113 -- AgentHackCheck side-effect plan unit tests.
//
// 1:1 lock the legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_HACKCHECKMsgParser (lines 3683-3740).
//

#include <gtest/gtest.h>

#include "mxh/server/agent_hackcheck.hpp"
#include "mxh/server/agent_hackcheck_side_effect_plan.hpp"

using namespace mxh::server;

TEST(HackCheckPlan, DropNoUserEmitsDropEffect) {
    HackCheckAction a{};
    a.kind = HackCheckActionKind::drop_no_user;
    a.protocol = hackcheck_speedhack;
    a.object_id = 11u;
    const auto plan = hackcheck_side_effect_plan(a);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, HackCheckSideEffectKind::Drop);
    EXPECT_EQ(plan.effects[0].reply_protocol, hackcheck_speedhack);
    EXPECT_EQ(plan.effects[0].object_id, 11u);
    EXPECT_FALSE(hackcheck_effect_targets_user(plan.effects[0]));
}

TEST(HackCheckPlan, IgnoreEmitsDropEffect) {
    HackCheckAction a{};
    a.kind = HackCheckActionKind::ignore;
    a.protocol = hackcheck_speedhack;
    const auto plan = hackcheck_side_effect_plan(a);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, HackCheckSideEffectKind::Drop);
}

TEST(HackCheckPlan, DetectSpeedhackEmitsBanUserEffect) {
    HackCheckAction a{};
    a.kind = HackCheckActionKind::detect_speedhack_and_ban;
    a.protocol = hackcheck_ban_user;
    a.object_id = 17u;
    a.data = 1234u;
    const auto plan = hackcheck_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, HackCheckSideEffectKind::SendBanUserToUser);
    EXPECT_EQ(plan.effects[0].reply_protocol, hackcheck_ban_user);
    EXPECT_EQ(plan.effects[0].object_id, 17u);
    EXPECT_EQ(plan.effects[0].data, 1234u);
    EXPECT_TRUE(hackcheck_effect_targets_user(plan.effects[0]));
}

TEST(HackCheckPlan, BanUserToAgentEmitsBanUserEffect) {
    HackCheckAction a{};
    a.kind = HackCheckActionKind::ban_user_to_agent_always;
    a.protocol = hackcheck_ban_user;
    a.object_id = 17u;
    const auto plan = hackcheck_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_EQ(plan.effects[0].kind, HackCheckSideEffectKind::SendBanUserToUser);
    EXPECT_EQ(plan.effects[0].reply_protocol, hackcheck_ban_user);
    EXPECT_TRUE(hackcheck_effect_targets_user(plan.effects[0]));
}

TEST(HackCheckClassifierPlan, SpeedhackNoUserEmitsDropPlan) {
    HackCheckRequest req{};
    req.protocol = hackcheck_speedhack;
    req.user_found = false;
    const auto action = classify_hackcheck(req);
    EXPECT_EQ(action.kind, HackCheckActionKind::drop_no_user);
    const auto plan = hackcheck_side_effect_plan(action);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, HackCheckSideEffectKind::Drop);
}

TEST(HackCheckClassifierPlan, SpeedhackLegitClientEmitsIgnorePlan) {
    HackCheckRequest req{};
    req.protocol = hackcheck_speedhack;
    req.user_found = true;
    req.server_time = 10000u;
    req.client_time = 1000u;
    // delta = 9000ms, threshold = 7000ms -> legit (delta >= 7000)
    const auto action = classify_hackcheck(req);
    EXPECT_EQ(action.kind, HackCheckActionKind::ignore);
    const auto plan = hackcheck_side_effect_plan(action);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, HackCheckSideEffectKind::Drop);
}

TEST(HackCheckClassifierPlan, SpeedhackFastClientEmitsBanPlan) {
    HackCheckRequest req{};
    req.protocol = hackcheck_speedhack;
    req.user_found = true;
    req.server_time = 10000u;
    req.client_time = 5000u;
    // delta = 5000ms, threshold = 7000ms -> speedhack (delta < 7000)
    const auto action = classify_hackcheck(req);
    EXPECT_EQ(action.kind, HackCheckActionKind::detect_speedhack_and_ban);
    EXPECT_EQ(action.protocol, hackcheck_ban_user);
    EXPECT_EQ(action.data, 5000u);
    const auto plan = hackcheck_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, HackCheckSideEffectKind::SendBanUserToUser);
    EXPECT_EQ(plan.effects[0].data, 5000u);
}

TEST(HackCheckClassifierPlan, SpeedhackServerBehindClientEmitsIgnorePlan) {
    HackCheckRequest req{};
    req.protocol = hackcheck_speedhack;
    req.user_found = true;
    req.server_time = 1000u;
    req.client_time = 5000u;
    // server_time < client_time -> ignore
    const auto action = classify_hackcheck(req);
    EXPECT_EQ(action.kind, HackCheckActionKind::ignore);
}

TEST(HackCheckClassifierPlan, BanUserToAgentNoUserEmitsDropPlan) {
    HackCheckRequest req{};
    req.protocol = hackcheck_ban_user_toagent;
    req.user_found = false;
    const auto action = classify_hackcheck(req);
    EXPECT_EQ(action.kind, HackCheckActionKind::drop_no_user);
    const auto plan = hackcheck_side_effect_plan(action);
    EXPECT_TRUE(plan.drop);
}

TEST(HackCheckClassifierPlan, BanUserToAgentWithUserEmitsBanPlan) {
    HackCheckRequest req{};
    req.protocol = hackcheck_ban_user_toagent;
    req.user_found = true;
    const auto action = classify_hackcheck(req);
    EXPECT_EQ(action.kind, HackCheckActionKind::ban_user_to_agent_always);
    EXPECT_EQ(action.protocol, hackcheck_ban_user);
    const auto plan = hackcheck_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, HackCheckSideEffectKind::SendBanUserToUser);
    EXPECT_EQ(plan.effects[0].reply_protocol, hackcheck_ban_user);
}

TEST(HackCheckClassifierPlan, BanUserDirectEmitsIgnorePlan) {
    HackCheckRequest req{};
    req.protocol = hackcheck_ban_user;
    const auto action = classify_hackcheck(req);
    EXPECT_EQ(action.kind, HackCheckActionKind::ignore);
    const auto plan = hackcheck_side_effect_plan(action);
    EXPECT_TRUE(plan.drop);
}