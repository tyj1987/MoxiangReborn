//
// 1:1 lock the implicit default-branch dispatch for MP_MP_SIGNAL in
// [Server]Agent/AgentNetworkMsgParser.cpp. Each test pins one branch
// of the legacy behavior to its modern side-effect plan output so
// future drift triggers a test failure.

#include <gtest/gtest.h>

#include "mxh/server/agent_signal.hpp"
#include "mxh/server/agent_signal_side_effect_plan.hpp"

using namespace mxh::server;

TEST(AgentSignalPlan, UserFoundEmitsForwardEffect) {
    AgentSignalRequest r;
    r.protocol = mp_signal_commonuser;
    r.user_found = true;
    r.object_id = 0xCAFE1234u;
    const auto plan = agent_signal_side_effect_plan(r, 17u);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentSignalSideEffectKind::ForwardToUser);
    EXPECT_EQ(plan.effects[0].reply_protocol, mp_signal_commonuser);
    EXPECT_EQ(plan.effects[0].connection_index, 17u);
    EXPECT_EQ(plan.effects[0].object_id, 0xCAFE1234u);
    EXPECT_TRUE(mp_signal_effect_targets_user(plan.effects[0]));
}

TEST(AgentSignalPlan, UserMissingEmitsDropEffect) {
    AgentSignalRequest r;
    r.protocol = mp_signal_commonuser;
    r.user_found = false;
    r.object_id = 0x12345678u;
    const auto plan = agent_signal_side_effect_plan(r, 17u);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentSignalSideEffectKind::Drop);
    EXPECT_FALSE(mp_signal_effect_targets_user(plan.effects[0]));
    EXPECT_EQ(plan.effects[0].connection_index, 0u);
    EXPECT_EQ(plan.effects[0].object_id, 0x12345678u);
}

TEST(AgentSignalPlan, ForwardPlanPreservesObjectId) {
    AgentSignalRequest r;
    r.protocol = mp_signal_commonuser;
    r.user_found = true;
    r.object_id = 0xABCDEF01u;
    const auto plan = agent_signal_side_effect_plan(r, 99u);
    EXPECT_EQ(plan.effects[0].object_id, 0xABCDEF01u);
    EXPECT_EQ(plan.effects[0].connection_index, 99u);
}

TEST(AgentSignalPlan, ForwardPlanPreservesProtocolByte) {
    const std::uint8_t all[] = {
        mp_signal_commonuser,
        mp_signal_oneuser,
        mp_signal_system,
        mp_signal_battle,
        mp_signal_vimu_result
    };
    for (std::uint8_t p : all) {
        AgentSignalRequest r;
        r.protocol = p;
        r.user_found = true;
        const auto plan = agent_signal_side_effect_plan(r, 0u);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].reply_protocol, p)
            << "protocol=" << +p;
    }
}

TEST(AgentSignalPlan, DropEffectAlwaysEmittedWhenUserMissing) {
    const std::uint8_t all[] = {
        mp_signal_commonuser,
        mp_signal_oneuser,
        mp_signal_system,
        mp_signal_battle,
        mp_signal_vimu_result
    };
    for (std::uint8_t p : all) {
        AgentSignalRequest r;
        r.protocol = p;
        r.user_found = false;
        const auto plan = agent_signal_side_effect_plan(r, 0u);
        EXPECT_TRUE(plan.drop);
        EXPECT_EQ(plan.effects[0].kind, AgentSignalSideEffectKind::Drop);
    }
}

TEST(AgentSignalPlan, DefaultPlanStructFieldsAreStable) {
    AgentSignalSideEffectPlan plan;
    EXPECT_FALSE(plan.dispatched);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(AgentSignalPlan, EffectTargetsUserPredicateMatchesForward) {
    AgentSignalSideEffect forward{};
    forward.kind = AgentSignalSideEffectKind::ForwardToUser;
    AgentSignalSideEffect drop{};
    drop.kind = AgentSignalSideEffectKind::Drop;
    EXPECT_TRUE(mp_signal_effect_targets_user(forward));
    EXPECT_FALSE(mp_signal_effect_targets_user(drop));
}

TEST(AgentSignalPlan, ForwardConnectionIndexEqualsResolvedConnection) {
    AgentSignalRequest r;
    r.user_found = true;
    r.object_id = 0x11223344u;
    const auto plan = agent_signal_side_effect_plan(r, 0xFFFFAA00u);
    EXPECT_EQ(plan.effects[0].connection_index, 0xFFFFAA00u);
}

TEST(AgentSignalPlan, DropEffectCarriesObjectIdEvenWhenConnectionZero) {
    AgentSignalRequest r;
    r.user_found = false;
    r.object_id = 0xCAFEBABEu;
    const auto plan = agent_signal_side_effect_plan(r, 1u);
    EXPECT_EQ(plan.effects[0].object_id, 0xCAFEBABEu);
    EXPECT_EQ(plan.effects[0].connection_index, 0u);
}
