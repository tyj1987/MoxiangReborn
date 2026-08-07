//
// 1:1 lock the implicit default-branch dispatch for MP_MP_PEACEWARMODE in
// [Server]Agent/AgentNetworkMsgParser.cpp. Each test pins one branch
// of the legacy behavior to its modern side-effect plan output so
// future drift triggers a test failure.

#include <gtest/gtest.h>

#include "mxh/server/agent_peacewarmode.hpp"
#include "mxh/server/agent_peacewarmode_side_effect_plan.hpp"

using namespace mxh::server;

TEST(AgentPeaceWarModePlan, UserFoundEmitsForwardEffect) {
    AgentPeaceWarModeRequest r;
    r.protocol = mp_peacewarmode_peace;
    r.user_found = true;
    r.object_id = 0xCAFE1234u;
    const auto plan = agent_peacewarmode_side_effect_plan(r, 17u);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentPeaceWarModeSideEffectKind::ForwardToUser);
    EXPECT_EQ(plan.effects[0].reply_protocol, mp_peacewarmode_peace);
    EXPECT_EQ(plan.effects[0].connection_index, 17u);
    EXPECT_EQ(plan.effects[0].object_id, 0xCAFE1234u);
    EXPECT_TRUE(mp_peacewarmode_effect_targets_user(plan.effects[0]));
}

TEST(AgentPeaceWarModePlan, UserMissingEmitsDropEffect) {
    AgentPeaceWarModeRequest r;
    r.protocol = mp_peacewarmode_peace;
    r.user_found = false;
    r.object_id = 0x12345678u;
    const auto plan = agent_peacewarmode_side_effect_plan(r, 17u);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentPeaceWarModeSideEffectKind::Drop);
    EXPECT_FALSE(mp_peacewarmode_effect_targets_user(plan.effects[0]));
    EXPECT_EQ(plan.effects[0].connection_index, 0u);
    EXPECT_EQ(plan.effects[0].object_id, 0x12345678u);
}

TEST(AgentPeaceWarModePlan, ForwardPlanPreservesObjectId) {
    AgentPeaceWarModeRequest r;
    r.protocol = mp_peacewarmode_peace;
    r.user_found = true;
    r.object_id = 0xABCDEF01u;
    const auto plan = agent_peacewarmode_side_effect_plan(r, 99u);
    EXPECT_EQ(plan.effects[0].object_id, 0xABCDEF01u);
    EXPECT_EQ(plan.effects[0].connection_index, 99u);
}

TEST(AgentPeaceWarModePlan, ForwardPlanPreservesProtocolByte) {
    const std::uint8_t all[] = {
        mp_peacewarmode_peace,
        mp_peacewarmode_war
    };
    for (std::uint8_t p : all) {
        AgentPeaceWarModeRequest r;
        r.protocol = p;
        r.user_found = true;
        const auto plan = agent_peacewarmode_side_effect_plan(r, 0u);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].reply_protocol, p)
            << "protocol=" << +p;
    }
}

TEST(AgentPeaceWarModePlan, DropEffectAlwaysEmittedWhenUserMissing) {
    const std::uint8_t all[] = {
        mp_peacewarmode_peace,
        mp_peacewarmode_war
    };
    for (std::uint8_t p : all) {
        AgentPeaceWarModeRequest r;
        r.protocol = p;
        r.user_found = false;
        const auto plan = agent_peacewarmode_side_effect_plan(r, 0u);
        EXPECT_TRUE(plan.drop);
        EXPECT_EQ(plan.effects[0].kind, AgentPeaceWarModeSideEffectKind::Drop);
    }
}

TEST(AgentPeaceWarModePlan, DefaultPlanStructFieldsAreStable) {
    AgentPeaceWarModeSideEffectPlan plan;
    EXPECT_FALSE(plan.dispatched);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(AgentPeaceWarModePlan, EffectTargetsUserPredicateMatchesForward) {
    AgentPeaceWarModeSideEffect forward{};
    forward.kind = AgentPeaceWarModeSideEffectKind::ForwardToUser;
    AgentPeaceWarModeSideEffect drop{};
    drop.kind = AgentPeaceWarModeSideEffectKind::Drop;
    EXPECT_TRUE(mp_peacewarmode_effect_targets_user(forward));
    EXPECT_FALSE(mp_peacewarmode_effect_targets_user(drop));
}

TEST(AgentPeaceWarModePlan, ForwardConnectionIndexEqualsResolvedConnection) {
    AgentPeaceWarModeRequest r;
    r.user_found = true;
    r.object_id = 0x11223344u;
    const auto plan = agent_peacewarmode_side_effect_plan(r, 0xFFFFAA00u);
    EXPECT_EQ(plan.effects[0].connection_index, 0xFFFFAA00u);
}

TEST(AgentPeaceWarModePlan, DropEffectCarriesObjectIdEvenWhenConnectionZero) {
    AgentPeaceWarModeRequest r;
    r.user_found = false;
    r.object_id = 0xCAFEBABEu;
    const auto plan = agent_peacewarmode_side_effect_plan(r, 1u);
    EXPECT_EQ(plan.effects[0].object_id, 0xCAFEBABEu);
    EXPECT_EQ(plan.effects[0].connection_index, 0u);
}
