//
// 1:1 lock the implicit default-branch dispatch for MP_CHARREVIVE in
// [Server]Agent/AgentNetworkMsgParser.cpp. Each test pins one branch
// of the legacy behavior to its modern side-effect plan output so
// future drift triggers a test failure.

#include <gtest/gtest.h>

#include "mxh/server/agent_charrevive.hpp"
#include "mxh/server/agent_charrevive_side_effect_plan.hpp"

using namespace mxh::server;

TEST(AgentCharRevivePlan, UserFoundEmitsForwardEffect) {
    AgentCharReviveRequest r;
    r.protocol = charrevive_Presentspot_syn;
    r.user_found = true;
    r.object_id = 0xCAFE1234u;
    const auto plan = agent_charrevive_side_effect_plan(r, 17u);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentCharReviveSideEffectKind::ForwardToUser);
    EXPECT_EQ(plan.effects[0].reply_protocol, charrevive_Presentspot_syn);
    EXPECT_EQ(plan.effects[0].connection_index, 17u);
    EXPECT_EQ(plan.effects[0].object_id, 0xCAFE1234u);
    EXPECT_TRUE(charrevive_effect_targets_user(plan.effects[0]));
}

TEST(AgentCharRevivePlan, UserMissingEmitsDropEffect) {
    AgentCharReviveRequest r;
    r.protocol = charrevive_Presentspot_syn;
    r.user_found = false;
    r.object_id = 0x12345678u;
    const auto plan = agent_charrevive_side_effect_plan(r, 17u);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentCharReviveSideEffectKind::Drop);
    EXPECT_FALSE(charrevive_effect_targets_user(plan.effects[0]));
    EXPECT_EQ(plan.effects[0].connection_index, 0u);
    EXPECT_EQ(plan.effects[0].object_id, 0x12345678u);
}

TEST(AgentCharRevivePlan, ForwardPlanPreservesObjectId) {
    AgentCharReviveRequest r;
    r.protocol = charrevive_Presentspot_syn;
    r.user_found = true;
    r.object_id = 0xABCDEF01u;
    const auto plan = agent_charrevive_side_effect_plan(r, 99u);
    EXPECT_EQ(plan.effects[0].object_id, 0xABCDEF01u);
    EXPECT_EQ(plan.effects[0].connection_index, 99u);
}

TEST(AgentCharRevivePlan, ForwardPlanPreservesProtocolByte) {
    const std::uint8_t all[] = {
        charrevive_Presentspot_syn,
        charrevive_Presentspot_ack,
        charrevive_Presentspot_nack,
        charrevive_loginspot_syn,
        charrevive_loginspot_ack,
        charrevive_loginspot_nack,
        charrevive_villagespot_syn,
        charrevive_villagespot_ack,
        charrevive_villagespot_nack
    };
    for (std::uint8_t p : all) {
        AgentCharReviveRequest r;
        r.protocol = p;
        r.user_found = true;
        const auto plan = agent_charrevive_side_effect_plan(r, 0u);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].reply_protocol, p)
            << "protocol=" << +p;
    }
}

TEST(AgentCharRevivePlan, DropEffectAlwaysEmittedWhenUserMissing) {
    const std::uint8_t all[] = {
        charrevive_Presentspot_syn,
        charrevive_Presentspot_ack,
        charrevive_Presentspot_nack,
        charrevive_loginspot_syn,
        charrevive_loginspot_ack,
        charrevive_loginspot_nack,
        charrevive_villagespot_syn,
        charrevive_villagespot_ack,
        charrevive_villagespot_nack
    };
    for (std::uint8_t p : all) {
        AgentCharReviveRequest r;
        r.protocol = p;
        r.user_found = false;
        const auto plan = agent_charrevive_side_effect_plan(r, 0u);
        EXPECT_TRUE(plan.drop);
        EXPECT_EQ(plan.effects[0].kind, AgentCharReviveSideEffectKind::Drop);
    }
}

TEST(AgentCharRevivePlan, DefaultPlanStructFieldsAreStable) {
    AgentCharReviveSideEffectPlan plan;
    EXPECT_FALSE(plan.dispatched);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(AgentCharRevivePlan, EffectTargetsUserPredicateMatchesForward) {
    AgentCharReviveSideEffect forward{};
    forward.kind = AgentCharReviveSideEffectKind::ForwardToUser;
    AgentCharReviveSideEffect drop{};
    drop.kind = AgentCharReviveSideEffectKind::Drop;
    EXPECT_TRUE(charrevive_effect_targets_user(forward));
    EXPECT_FALSE(charrevive_effect_targets_user(drop));
}

TEST(AgentCharRevivePlan, ForwardConnectionIndexEqualsResolvedConnection) {
    AgentCharReviveRequest r;
    r.user_found = true;
    r.object_id = 0x11223344u;
    const auto plan = agent_charrevive_side_effect_plan(r, 0xFFFFAA00u);
    EXPECT_EQ(plan.effects[0].connection_index, 0xFFFFAA00u);
}

TEST(AgentCharRevivePlan, DropEffectCarriesObjectIdEvenWhenConnectionZero) {
    AgentCharReviveRequest r;
    r.user_found = false;
    r.object_id = 0xCAFEBABEu;
    const auto plan = agent_charrevive_side_effect_plan(r, 1u);
    EXPECT_EQ(plan.effects[0].object_id, 0xCAFEBABEu);
    EXPECT_EQ(plan.effects[0].connection_index, 0u);
}
