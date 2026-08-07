//
// 1:1 lock the implicit default-branch dispatch for MP_SIMBUB in
// [Server]Agent/AgentNetworkMsgParser.cpp. Each test pins one branch
// of the legacy behavior to its modern side-effect plan output so
// future drift triggers a test failure.

#include <gtest/gtest.h>

#include "mxh/server/agent_simbub.hpp"
#include "mxh/server/agent_simbub_side_effect_plan.hpp"

using namespace mxh::server;

TEST(AgentSimBubPlan, UserFoundEmitsForwardEffect) {
    AgentSimBubRequest r;
    r.protocol = simbub_change_syn;
    r.user_found = true;
    r.object_id = 0xCAFE1234u;
    const auto plan = agent_simbub_side_effect_plan(r, 17u);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentSimBubSideEffectKind::ForwardToUser);
    EXPECT_EQ(plan.effects[0].reply_protocol, simbub_change_syn);
    EXPECT_EQ(plan.effects[0].connection_index, 17u);
    EXPECT_EQ(plan.effects[0].object_id, 0xCAFE1234u);
    EXPECT_TRUE(simbub_effect_targets_user(plan.effects[0]));
}

TEST(AgentSimBubPlan, UserMissingEmitsDropEffect) {
    AgentSimBubRequest r;
    r.protocol = simbub_change_syn;
    r.user_found = false;
    r.object_id = 0x12345678u;
    const auto plan = agent_simbub_side_effect_plan(r, 17u);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentSimBubSideEffectKind::Drop);
    EXPECT_FALSE(simbub_effect_targets_user(plan.effects[0]));
    EXPECT_EQ(plan.effects[0].connection_index, 0u);
    EXPECT_EQ(plan.effects[0].object_id, 0x12345678u);
}

TEST(AgentSimBubPlan, ForwardPlanPreservesObjectId) {
    AgentSimBubRequest r;
    r.protocol = simbub_change_syn;
    r.user_found = true;
    r.object_id = 0xABCDEF01u;
    const auto plan = agent_simbub_side_effect_plan(r, 99u);
    EXPECT_EQ(plan.effects[0].object_id, 0xABCDEF01u);
    EXPECT_EQ(plan.effects[0].connection_index, 99u);
}

TEST(AgentSimBubPlan, ForwardPlanPreservesProtocolByte) {
    const std::uint8_t all[] = {
        simbub_change_syn,
        simbub_change_ack,
        simbub_change_nack
    };
    for (std::uint8_t p : all) {
        AgentSimBubRequest r;
        r.protocol = p;
        r.user_found = true;
        const auto plan = agent_simbub_side_effect_plan(r, 0u);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].reply_protocol, p)
            << "protocol=" << +p;
    }
}

TEST(AgentSimBubPlan, DropEffectAlwaysEmittedWhenUserMissing) {
    const std::uint8_t all[] = {
        simbub_change_syn,
        simbub_change_ack,
        simbub_change_nack
    };
    for (std::uint8_t p : all) {
        AgentSimBubRequest r;
        r.protocol = p;
        r.user_found = false;
        const auto plan = agent_simbub_side_effect_plan(r, 0u);
        EXPECT_TRUE(plan.drop);
        EXPECT_EQ(plan.effects[0].kind, AgentSimBubSideEffectKind::Drop);
    }
}

TEST(AgentSimBubPlan, DefaultPlanStructFieldsAreStable) {
    AgentSimBubSideEffectPlan plan;
    EXPECT_FALSE(plan.dispatched);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(AgentSimBubPlan, EffectTargetsUserPredicateMatchesForward) {
    AgentSimBubSideEffect forward{};
    forward.kind = AgentSimBubSideEffectKind::ForwardToUser;
    AgentSimBubSideEffect drop{};
    drop.kind = AgentSimBubSideEffectKind::Drop;
    EXPECT_TRUE(simbub_effect_targets_user(forward));
    EXPECT_FALSE(simbub_effect_targets_user(drop));
}

TEST(AgentSimBubPlan, ForwardConnectionIndexEqualsResolvedConnection) {
    AgentSimBubRequest r;
    r.user_found = true;
    r.object_id = 0x11223344u;
    const auto plan = agent_simbub_side_effect_plan(r, 0xFFFFAA00u);
    EXPECT_EQ(plan.effects[0].connection_index, 0xFFFFAA00u);
}

TEST(AgentSimBubPlan, DropEffectCarriesObjectIdEvenWhenConnectionZero) {
    AgentSimBubRequest r;
    r.user_found = false;
    r.object_id = 0xCAFEBABEu;
    const auto plan = agent_simbub_side_effect_plan(r, 1u);
    EXPECT_EQ(plan.effects[0].object_id, 0xCAFEBABEu);
    EXPECT_EQ(plan.effects[0].connection_index, 0u);
}
