//
// 1:1 lock the implicit default-branch dispatch for MP_MP_QUICK in
// [Server]Agent/AgentNetworkMsgParser.cpp. Each test pins one branch
// of the legacy behavior to its modern side-effect plan output so
// future drift triggers a test failure.

#include <gtest/gtest.h>

#include "mxh/server/agent_quick.hpp"
#include "mxh/server/agent_quick_side_effect_plan.hpp"

using namespace mxh::server;

TEST(AgentQuickPlan, UserFoundEmitsForwardEffect) {
    AgentQuickRequest r;
    r.protocol = mp_quick_add_syn;
    r.user_found = true;
    r.object_id = 0xCAFE1234u;
    const auto plan = agent_quick_side_effect_plan(r, 17u);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentQuickSideEffectKind::ForwardToUser);
    EXPECT_EQ(plan.effects[0].reply_protocol, mp_quick_add_syn);
    EXPECT_EQ(plan.effects[0].connection_index, 17u);
    EXPECT_EQ(plan.effects[0].object_id, 0xCAFE1234u);
    EXPECT_TRUE(mp_quick_effect_targets_user(plan.effects[0]));
}

TEST(AgentQuickPlan, UserMissingEmitsDropEffect) {
    AgentQuickRequest r;
    r.protocol = mp_quick_add_syn;
    r.user_found = false;
    r.object_id = 0x12345678u;
    const auto plan = agent_quick_side_effect_plan(r, 17u);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentQuickSideEffectKind::Drop);
    EXPECT_FALSE(mp_quick_effect_targets_user(plan.effects[0]));
    EXPECT_EQ(plan.effects[0].connection_index, 0u);
    EXPECT_EQ(plan.effects[0].object_id, 0x12345678u);
}

TEST(AgentQuickPlan, ForwardPlanPreservesObjectId) {
    AgentQuickRequest r;
    r.protocol = mp_quick_add_syn;
    r.user_found = true;
    r.object_id = 0xABCDEF01u;
    const auto plan = agent_quick_side_effect_plan(r, 99u);
    EXPECT_EQ(plan.effects[0].object_id, 0xABCDEF01u);
    EXPECT_EQ(plan.effects[0].connection_index, 99u);
}

TEST(AgentQuickPlan, ForwardPlanPreservesProtocolByte) {
    const std::uint8_t all[] = {
        mp_quick_add_syn,
        mp_quick_add_ack,
        mp_quick_add_nack,
        mp_quick_use_syn,
        mp_quick_use_ack,
        mp_quick_use_nack,
        mp_quick_move_syn,
        mp_quick_move_ack,
        mp_quick_move_nack,
        mp_quick_rem_syn,
        mp_quick_rem_ack,
        mp_quick_rem_nack,
        mp_quick_set_syn,
        mp_quick_set_ack,
        mp_quick_set_nack
    };
    for (std::uint8_t p : all) {
        AgentQuickRequest r;
        r.protocol = p;
        r.user_found = true;
        const auto plan = agent_quick_side_effect_plan(r, 0u);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].reply_protocol, p)
            << "protocol=" << +p;
    }
}

TEST(AgentQuickPlan, DropEffectAlwaysEmittedWhenUserMissing) {
    const std::uint8_t all[] = {
        mp_quick_add_syn,
        mp_quick_add_ack,
        mp_quick_add_nack,
        mp_quick_use_syn,
        mp_quick_use_ack,
        mp_quick_use_nack,
        mp_quick_move_syn,
        mp_quick_move_ack,
        mp_quick_move_nack,
        mp_quick_rem_syn,
        mp_quick_rem_ack,
        mp_quick_rem_nack,
        mp_quick_set_syn,
        mp_quick_set_ack,
        mp_quick_set_nack
    };
    for (std::uint8_t p : all) {
        AgentQuickRequest r;
        r.protocol = p;
        r.user_found = false;
        const auto plan = agent_quick_side_effect_plan(r, 0u);
        EXPECT_TRUE(plan.drop);
        EXPECT_EQ(plan.effects[0].kind, AgentQuickSideEffectKind::Drop);
    }
}

TEST(AgentQuickPlan, DefaultPlanStructFieldsAreStable) {
    AgentQuickSideEffectPlan plan;
    EXPECT_FALSE(plan.dispatched);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(AgentQuickPlan, EffectTargetsUserPredicateMatchesForward) {
    AgentQuickSideEffect forward{};
    forward.kind = AgentQuickSideEffectKind::ForwardToUser;
    AgentQuickSideEffect drop{};
    drop.kind = AgentQuickSideEffectKind::Drop;
    EXPECT_TRUE(mp_quick_effect_targets_user(forward));
    EXPECT_FALSE(mp_quick_effect_targets_user(drop));
}

TEST(AgentQuickPlan, ForwardConnectionIndexEqualsResolvedConnection) {
    AgentQuickRequest r;
    r.user_found = true;
    r.object_id = 0x11223344u;
    const auto plan = agent_quick_side_effect_plan(r, 0xFFFFAA00u);
    EXPECT_EQ(plan.effects[0].connection_index, 0xFFFFAA00u);
}

TEST(AgentQuickPlan, DropEffectCarriesObjectIdEvenWhenConnectionZero) {
    AgentQuickRequest r;
    r.user_found = false;
    r.object_id = 0xCAFEBABEu;
    const auto plan = agent_quick_side_effect_plan(r, 1u);
    EXPECT_EQ(plan.effects[0].object_id, 0xCAFEBABEu);
    EXPECT_EQ(plan.effects[0].connection_index, 0u);
}
