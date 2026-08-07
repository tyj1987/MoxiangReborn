//
// 1:1 lock the implicit default-branch dispatch for MP_MP_TACTIC in
// [Server]Agent/AgentNetworkMsgParser.cpp. Each test pins one branch
// of the legacy behavior to its modern side-effect plan output so
// future drift triggers a test failure.

#include <gtest/gtest.h>

#include "mxh/server/agent_tactic.hpp"
#include "mxh/server/agent_tactic_side_effect_plan.hpp"

using namespace mxh::server;

TEST(AgentTacticPlan, UserFoundEmitsForwardEffect) {
    AgentTacticRequest r;
    r.protocol = mp_tactic_start_syn;
    r.user_found = true;
    r.object_id = 0xCAFE1234u;
    const auto plan = agent_tactic_side_effect_plan(r, 17u);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentTacticSideEffectKind::ForwardToUser);
    EXPECT_EQ(plan.effects[0].reply_protocol, mp_tactic_start_syn);
    EXPECT_EQ(plan.effects[0].connection_index, 17u);
    EXPECT_EQ(plan.effects[0].object_id, 0xCAFE1234u);
    EXPECT_TRUE(mp_tactic_effect_targets_user(plan.effects[0]));
}

TEST(AgentTacticPlan, UserMissingEmitsDropEffect) {
    AgentTacticRequest r;
    r.protocol = mp_tactic_start_syn;
    r.user_found = false;
    r.object_id = 0x12345678u;
    const auto plan = agent_tactic_side_effect_plan(r, 17u);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentTacticSideEffectKind::Drop);
    EXPECT_FALSE(mp_tactic_effect_targets_user(plan.effects[0]));
    EXPECT_EQ(plan.effects[0].connection_index, 0u);
    EXPECT_EQ(plan.effects[0].object_id, 0x12345678u);
}

TEST(AgentTacticPlan, ForwardPlanPreservesObjectId) {
    AgentTacticRequest r;
    r.protocol = mp_tactic_start_syn;
    r.user_found = true;
    r.object_id = 0xABCDEF01u;
    const auto plan = agent_tactic_side_effect_plan(r, 99u);
    EXPECT_EQ(plan.effects[0].object_id, 0xABCDEF01u);
    EXPECT_EQ(plan.effects[0].connection_index, 99u);
}

TEST(AgentTacticPlan, ForwardPlanPreservesProtocolByte) {
    const std::uint8_t all[] = {
        mp_tactic_start_syn,
        mp_tactic_start_ack,
        mp_tactic_start_nack,
        mp_tactic_join_syn,
        mp_tactic_join_ack,
        mp_tactic_join_nack,
        mp_tactic_object_add,
        mp_tactic_fail,
        mp_tactic_execute
    };
    for (std::uint8_t p : all) {
        AgentTacticRequest r;
        r.protocol = p;
        r.user_found = true;
        const auto plan = agent_tactic_side_effect_plan(r, 0u);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].reply_protocol, p)
            << "protocol=" << +p;
    }
}

TEST(AgentTacticPlan, DropEffectAlwaysEmittedWhenUserMissing) {
    const std::uint8_t all[] = {
        mp_tactic_start_syn,
        mp_tactic_start_ack,
        mp_tactic_start_nack,
        mp_tactic_join_syn,
        mp_tactic_join_ack,
        mp_tactic_join_nack,
        mp_tactic_object_add,
        mp_tactic_fail,
        mp_tactic_execute
    };
    for (std::uint8_t p : all) {
        AgentTacticRequest r;
        r.protocol = p;
        r.user_found = false;
        const auto plan = agent_tactic_side_effect_plan(r, 0u);
        EXPECT_TRUE(plan.drop);
        EXPECT_EQ(plan.effects[0].kind, AgentTacticSideEffectKind::Drop);
    }
}

TEST(AgentTacticPlan, DefaultPlanStructFieldsAreStable) {
    AgentTacticSideEffectPlan plan;
    EXPECT_FALSE(plan.dispatched);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(AgentTacticPlan, EffectTargetsUserPredicateMatchesForward) {
    AgentTacticSideEffect forward{};
    forward.kind = AgentTacticSideEffectKind::ForwardToUser;
    AgentTacticSideEffect drop{};
    drop.kind = AgentTacticSideEffectKind::Drop;
    EXPECT_TRUE(mp_tactic_effect_targets_user(forward));
    EXPECT_FALSE(mp_tactic_effect_targets_user(drop));
}

TEST(AgentTacticPlan, ForwardConnectionIndexEqualsResolvedConnection) {
    AgentTacticRequest r;
    r.user_found = true;
    r.object_id = 0x11223344u;
    const auto plan = agent_tactic_side_effect_plan(r, 0xFFFFAA00u);
    EXPECT_EQ(plan.effects[0].connection_index, 0xFFFFAA00u);
}

TEST(AgentTacticPlan, DropEffectCarriesObjectIdEvenWhenConnectionZero) {
    AgentTacticRequest r;
    r.user_found = false;
    r.object_id = 0xCAFEBABEu;
    const auto plan = agent_tactic_side_effect_plan(r, 1u);
    EXPECT_EQ(plan.effects[0].object_id, 0xCAFEBABEu);
    EXPECT_EQ(plan.effects[0].connection_index, 0u);
}
