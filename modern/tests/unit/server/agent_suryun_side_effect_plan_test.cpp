//
// 1:1 lock the implicit default-branch dispatch for MP_SURYUN in
// [Server]Agent/AgentNetworkMsgParser.cpp. Each test pins one branch
// of the legacy behavior to its modern side-effect plan output so
// future drift triggers a test failure.

#include <gtest/gtest.h>

#include "mxh/server/agent_suryun.hpp"
#include "mxh/server/agent_suryun_side_effect_plan.hpp"

using namespace mxh::server;

TEST(AgentSuryunPlan, UserFoundEmitsForwardEffect) {
    AgentSuryunRequest r;
    r.protocol = suryun_gosuryunmap_syn;
    r.user_found = true;
    r.object_id = 0xCAFE1234u;
    const auto plan = agent_suryun_side_effect_plan(r, 17u);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentSuryunSideEffectKind::ForwardToUser);
    EXPECT_EQ(plan.effects[0].reply_protocol, suryun_gosuryunmap_syn);
    EXPECT_EQ(plan.effects[0].connection_index, 17u);
    EXPECT_EQ(plan.effects[0].object_id, 0xCAFE1234u);
    EXPECT_TRUE(suryun_effect_targets_user(plan.effects[0]));
}

TEST(AgentSuryunPlan, UserMissingEmitsDropEffect) {
    AgentSuryunRequest r;
    r.protocol = suryun_gosuryunmap_syn;
    r.user_found = false;
    r.object_id = 0x12345678u;
    const auto plan = agent_suryun_side_effect_plan(r, 17u);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentSuryunSideEffectKind::Drop);
    EXPECT_FALSE(suryun_effect_targets_user(plan.effects[0]));
    EXPECT_EQ(plan.effects[0].connection_index, 0u);
    EXPECT_EQ(plan.effects[0].object_id, 0x12345678u);
}

TEST(AgentSuryunPlan, ForwardPlanPreservesObjectId) {
    AgentSuryunRequest r;
    r.protocol = suryun_gosuryunmap_syn;
    r.user_found = true;
    r.object_id = 0xABCDEF01u;
    const auto plan = agent_suryun_side_effect_plan(r, 99u);
    EXPECT_EQ(plan.effects[0].object_id, 0xABCDEF01u);
    EXPECT_EQ(plan.effects[0].connection_index, 99u);
}

TEST(AgentSuryunPlan, ForwardPlanPreservesProtocolByte) {
    const std::uint8_t all[] = {
        suryun_gosuryunmap_syn,
        suryun_gosuryunmap_ack,
        suryun_gosuryunmap_nack,
        suryun_enter_syn,
        suryun_enter_ack,
        suryun_enter_nack,
        suryun_start,
        suryun_returntorealworld,
        suryun_leave_syn,
        suryun_leave_ack,
        suryun_leave_nack
    };
    for (std::uint8_t p : all) {
        AgentSuryunRequest r;
        r.protocol = p;
        r.user_found = true;
        const auto plan = agent_suryun_side_effect_plan(r, 0u);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].reply_protocol, p)
            << "protocol=" << +p;
    }
}

TEST(AgentSuryunPlan, DropEffectAlwaysEmittedWhenUserMissing) {
    const std::uint8_t all[] = {
        suryun_gosuryunmap_syn,
        suryun_gosuryunmap_ack,
        suryun_gosuryunmap_nack,
        suryun_enter_syn,
        suryun_enter_ack,
        suryun_enter_nack,
        suryun_start,
        suryun_returntorealworld,
        suryun_leave_syn,
        suryun_leave_ack,
        suryun_leave_nack
    };
    for (std::uint8_t p : all) {
        AgentSuryunRequest r;
        r.protocol = p;
        r.user_found = false;
        const auto plan = agent_suryun_side_effect_plan(r, 0u);
        EXPECT_TRUE(plan.drop);
        EXPECT_EQ(plan.effects[0].kind, AgentSuryunSideEffectKind::Drop);
    }
}

TEST(AgentSuryunPlan, DefaultPlanStructFieldsAreStable) {
    AgentSuryunSideEffectPlan plan;
    EXPECT_FALSE(plan.dispatched);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(AgentSuryunPlan, EffectTargetsUserPredicateMatchesForward) {
    AgentSuryunSideEffect forward{};
    forward.kind = AgentSuryunSideEffectKind::ForwardToUser;
    AgentSuryunSideEffect drop{};
    drop.kind = AgentSuryunSideEffectKind::Drop;
    EXPECT_TRUE(suryun_effect_targets_user(forward));
    EXPECT_FALSE(suryun_effect_targets_user(drop));
}

TEST(AgentSuryunPlan, ForwardConnectionIndexEqualsResolvedConnection) {
    AgentSuryunRequest r;
    r.user_found = true;
    r.object_id = 0x11223344u;
    const auto plan = agent_suryun_side_effect_plan(r, 0xFFFFAA00u);
    EXPECT_EQ(plan.effects[0].connection_index, 0xFFFFAA00u);
}

TEST(AgentSuryunPlan, DropEffectCarriesObjectIdEvenWhenConnectionZero) {
    AgentSuryunRequest r;
    r.user_found = false;
    r.object_id = 0xCAFEBABEu;
    const auto plan = agent_suryun_side_effect_plan(r, 1u);
    EXPECT_EQ(plan.effects[0].object_id, 0xCAFEBABEu);
    EXPECT_EQ(plan.effects[0].connection_index, 0u);
}
