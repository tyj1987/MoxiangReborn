//
// 1:1 lock the implicit default-branch dispatch for MP_SOCIETYACT in
// [Server]Agent/AgentNetworkMsgParser.cpp. Each test pins one branch
// of the legacy behavior to its modern side-effect plan output so
// future drift triggers a test failure.

#include <gtest/gtest.h>

#include "mxh/server/agent_societyact.hpp"
#include "mxh/server/agent_societyact_side_effect_plan.hpp"

using namespace mxh::server;

TEST(AgentSocietyActPlan, UserFoundEmitsForwardEffect) {
    AgentSocietyActRequest r;
    r.protocol = societyact_act_syn;
    r.user_found = true;
    r.object_id = 0xCAFE1234u;
    const auto plan = agent_societyact_side_effect_plan(r, 17u);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentSocietyActSideEffectKind::ForwardToUser);
    EXPECT_EQ(plan.effects[0].reply_protocol, societyact_act_syn);
    EXPECT_EQ(plan.effects[0].connection_index, 17u);
    EXPECT_EQ(plan.effects[0].object_id, 0xCAFE1234u);
    EXPECT_TRUE(societyact_effect_targets_user(plan.effects[0]));
}

TEST(AgentSocietyActPlan, UserMissingEmitsDropEffect) {
    AgentSocietyActRequest r;
    r.protocol = societyact_act_syn;
    r.user_found = false;
    r.object_id = 0x12345678u;
    const auto plan = agent_societyact_side_effect_plan(r, 17u);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentSocietyActSideEffectKind::Drop);
    EXPECT_FALSE(societyact_effect_targets_user(plan.effects[0]));
    EXPECT_EQ(plan.effects[0].connection_index, 0u);
    EXPECT_EQ(plan.effects[0].object_id, 0x12345678u);
}

TEST(AgentSocietyActPlan, ForwardPlanPreservesObjectId) {
    AgentSocietyActRequest r;
    r.protocol = societyact_act_syn;
    r.user_found = true;
    r.object_id = 0xABCDEF01u;
    const auto plan = agent_societyact_side_effect_plan(r, 99u);
    EXPECT_EQ(plan.effects[0].object_id, 0xABCDEF01u);
    EXPECT_EQ(plan.effects[0].connection_index, 99u);
}

TEST(AgentSocietyActPlan, ForwardPlanPreservesProtocolByte) {
    const std::uint8_t all[] = {
        societyact_act_syn,
        societyact_act
    };
    for (std::uint8_t p : all) {
        AgentSocietyActRequest r;
        r.protocol = p;
        r.user_found = true;
        const auto plan = agent_societyact_side_effect_plan(r, 0u);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].reply_protocol, p)
            << "protocol=" << +p;
    }
}

TEST(AgentSocietyActPlan, DropEffectAlwaysEmittedWhenUserMissing) {
    const std::uint8_t all[] = {
        societyact_act_syn,
        societyact_act
    };
    for (std::uint8_t p : all) {
        AgentSocietyActRequest r;
        r.protocol = p;
        r.user_found = false;
        const auto plan = agent_societyact_side_effect_plan(r, 0u);
        EXPECT_TRUE(plan.drop);
        EXPECT_EQ(plan.effects[0].kind, AgentSocietyActSideEffectKind::Drop);
    }
}

TEST(AgentSocietyActPlan, DefaultPlanStructFieldsAreStable) {
    AgentSocietyActSideEffectPlan plan;
    EXPECT_FALSE(plan.dispatched);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(AgentSocietyActPlan, EffectTargetsUserPredicateMatchesForward) {
    AgentSocietyActSideEffect forward{};
    forward.kind = AgentSocietyActSideEffectKind::ForwardToUser;
    AgentSocietyActSideEffect drop{};
    drop.kind = AgentSocietyActSideEffectKind::Drop;
    EXPECT_TRUE(societyact_effect_targets_user(forward));
    EXPECT_FALSE(societyact_effect_targets_user(drop));
}

TEST(AgentSocietyActPlan, ForwardConnectionIndexEqualsResolvedConnection) {
    AgentSocietyActRequest r;
    r.user_found = true;
    r.object_id = 0x11223344u;
    const auto plan = agent_societyact_side_effect_plan(r, 0xFFFFAA00u);
    EXPECT_EQ(plan.effects[0].connection_index, 0xFFFFAA00u);
}

TEST(AgentSocietyActPlan, DropEffectCarriesObjectIdEvenWhenConnectionZero) {
    AgentSocietyActRequest r;
    r.user_found = false;
    r.object_id = 0xCAFEBABEu;
    const auto plan = agent_societyact_side_effect_plan(r, 1u);
    EXPECT_EQ(plan.effects[0].object_id, 0xCAFEBABEu);
    EXPECT_EQ(plan.effects[0].connection_index, 0u);
}
