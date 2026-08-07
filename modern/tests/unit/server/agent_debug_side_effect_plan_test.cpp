//
// 1:1 lock the legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_DebugMsgParser (lines 2837-2854). Each test pins one branch of the legacy
// dispatch to its modern side-effect plan output so future drift triggers a
// test failure.

#include <gtest/gtest.h>

#include "mxh/server/agent_debug.hpp"
#include "mxh/server/agent_debug_side_effect_plan.hpp"

using namespace mxh::server;

TEST(AgentDebugPlan, LoggedOutcomeEmitsLogAssertEffect) {
    AgentDebugRequest r;
    r.protocol = debug_clientassert;
    r.payload_present = true;
    auto outcome = classify_agent_debug(r);
    const auto plan = agent_debug_side_effect_plan(outcome);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    EXPECT_TRUE(plan.log_assert);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentDebugSideEffectKind::LogAssert);
    EXPECT_EQ(plan.effects[0].reply_protocol, debug_clientassert);
    EXPECT_TRUE(plan.effects[0].payload_present);
}

TEST(AgentDebugPlan, DroppedForUnknownProtocol) {
    AgentDebugRequest r;
    r.protocol = 99u;
    r.payload_present = true;
    auto outcome = classify_agent_debug(r);
    EXPECT_EQ(outcome, AgentDebugOutcome::Dropped);
    const auto plan = agent_debug_side_effect_plan(outcome);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
    EXPECT_FALSE(plan.log_assert);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentDebugSideEffectKind::Drop);
}

TEST(AgentDebugPlan, DroppedForMissingPayload) {
    AgentDebugRequest r;
    r.protocol = debug_clientassert;
    r.payload_present = false;
    auto outcome = classify_agent_debug(r);
    EXPECT_EQ(outcome, AgentDebugOutcome::Dropped);
    const auto plan = agent_debug_side_effect_plan(outcome);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, AgentDebugSideEffectKind::Drop);
    EXPECT_FALSE(plan.effects[0].payload_present);
}

TEST(AgentDebugPlan, UserSidePlanMirrorsClassify) {
    // agent_debug_user_side_effect_plan is the convenience wrapper
    // around classify_agent_debug + plan-build.
    AgentDebugRequest r;
    r.protocol = debug_clientassert;
    r.payload_present = true;
    const auto plan = agent_debug_user_side_effect_plan(r);
    EXPECT_EQ(plan.effects[0].kind, AgentDebugSideEffectKind::LogAssert);
}

TEST(AgentDebugPlan, UserSidePlanDroppedForUnknown) {
    AgentDebugRequest r;
    r.protocol = 250u;
    const auto plan = agent_debug_user_side_effect_plan(r);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, AgentDebugSideEffectKind::Drop);
}

TEST(AgentDebugPlan, LogAssertEffectNeverEchoesProtocolAsReply) {
    // legacy WriteAssertMsg does not emit a network response; the reply_protocol
    // here is purely the source protocol byte for orchestrator logging.
    AgentDebugRequest r;
    r.protocol = debug_clientassert;
    r.payload_present = true;
    const auto plan = agent_debug_user_side_effect_plan(r);
    EXPECT_EQ(plan.effects[0].reply_protocol, debug_clientassert);
}

TEST(AgentDebugPlan, DefaultPlanStructFieldsAreStable) {
    AgentDebugSideEffectPlan plan;
    EXPECT_FALSE(plan.dispatched);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.log_assert);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(AgentDebugPlan, LoggedPlanNeverDrops) {
    // Sanity: any outcome that produced Logged must have drop=false.
    AgentDebugRequest r;
    r.protocol = debug_clientassert;
    r.payload_present = true;
    const auto plan = agent_debug_user_side_effect_plan(r);
    EXPECT_FALSE(plan.drop);
}
