//
// 1:1 lock the implicit default-branch dispatch for MP_MP_AUTOPATCH in
// [Server]Agent/AgentNetworkMsgParser.cpp. Each test pins one branch
// of the legacy behavior to its modern side-effect plan output so
// future drift triggers a test failure.

#include <gtest/gtest.h>

#include "mxh/server/agent_autopatch.hpp"
#include "mxh/server/agent_autopatch_side_effect_plan.hpp"

using namespace mxh::server;

TEST(AgentAutoPatchPlan, UserFoundEmitsForwardEffect) {
    AgentAutoPatchRequest r;
    r.protocol = mp_autopatch_traffic_syn;
    r.user_found = true;
    r.object_id = 0xCAFE1234u;
    const auto plan = agent_autopatch_side_effect_plan(r, 17u);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentAutoPatchSideEffectKind::ForwardToUser);
    EXPECT_EQ(plan.effects[0].reply_protocol, mp_autopatch_traffic_syn);
    EXPECT_EQ(plan.effects[0].connection_index, 17u);
    EXPECT_EQ(plan.effects[0].object_id, 0xCAFE1234u);
    EXPECT_TRUE(mp_autopatch_effect_targets_user(plan.effects[0]));
}

TEST(AgentAutoPatchPlan, UserMissingEmitsDropEffect) {
    AgentAutoPatchRequest r;
    r.protocol = mp_autopatch_traffic_syn;
    r.user_found = false;
    r.object_id = 0x12345678u;
    const auto plan = agent_autopatch_side_effect_plan(r, 17u);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentAutoPatchSideEffectKind::Drop);
    EXPECT_FALSE(mp_autopatch_effect_targets_user(plan.effects[0]));
    EXPECT_EQ(plan.effects[0].connection_index, 0u);
    EXPECT_EQ(plan.effects[0].object_id, 0x12345678u);
}

TEST(AgentAutoPatchPlan, ForwardPlanPreservesObjectId) {
    AgentAutoPatchRequest r;
    r.protocol = mp_autopatch_traffic_syn;
    r.user_found = true;
    r.object_id = 0xABCDEF01u;
    const auto plan = agent_autopatch_side_effect_plan(r, 99u);
    EXPECT_EQ(plan.effects[0].object_id, 0xABCDEF01u);
    EXPECT_EQ(plan.effects[0].connection_index, 99u);
}

TEST(AgentAutoPatchPlan, ForwardPlanPreservesProtocolByte) {
    const std::uint8_t all[] = {
        mp_autopatch_traffic_syn,
        mp_autopatch_traffic_ack,
        mp_autopatch_traffic_nack
    };
    for (std::uint8_t p : all) {
        AgentAutoPatchRequest r;
        r.protocol = p;
        r.user_found = true;
        const auto plan = agent_autopatch_side_effect_plan(r, 0u);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].reply_protocol, p)
            << "protocol=" << +p;
    }
}

TEST(AgentAutoPatchPlan, DropEffectAlwaysEmittedWhenUserMissing) {
    const std::uint8_t all[] = {
        mp_autopatch_traffic_syn,
        mp_autopatch_traffic_ack,
        mp_autopatch_traffic_nack
    };
    for (std::uint8_t p : all) {
        AgentAutoPatchRequest r;
        r.protocol = p;
        r.user_found = false;
        const auto plan = agent_autopatch_side_effect_plan(r, 0u);
        EXPECT_TRUE(plan.drop);
        EXPECT_EQ(plan.effects[0].kind, AgentAutoPatchSideEffectKind::Drop);
    }
}

TEST(AgentAutoPatchPlan, DefaultPlanStructFieldsAreStable) {
    AgentAutoPatchSideEffectPlan plan;
    EXPECT_FALSE(plan.dispatched);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(AgentAutoPatchPlan, EffectTargetsUserPredicateMatchesForward) {
    AgentAutoPatchSideEffect forward{};
    forward.kind = AgentAutoPatchSideEffectKind::ForwardToUser;
    AgentAutoPatchSideEffect drop{};
    drop.kind = AgentAutoPatchSideEffectKind::Drop;
    EXPECT_TRUE(mp_autopatch_effect_targets_user(forward));
    EXPECT_FALSE(mp_autopatch_effect_targets_user(drop));
}

TEST(AgentAutoPatchPlan, ForwardConnectionIndexEqualsResolvedConnection) {
    AgentAutoPatchRequest r;
    r.user_found = true;
    r.object_id = 0x11223344u;
    const auto plan = agent_autopatch_side_effect_plan(r, 0xFFFFAA00u);
    EXPECT_EQ(plan.effects[0].connection_index, 0xFFFFAA00u);
}

TEST(AgentAutoPatchPlan, DropEffectCarriesObjectIdEvenWhenConnectionZero) {
    AgentAutoPatchRequest r;
    r.user_found = false;
    r.object_id = 0xCAFEBABEu;
    const auto plan = agent_autopatch_side_effect_plan(r, 1u);
    EXPECT_EQ(plan.effects[0].object_id, 0xCAFEBABEu);
    EXPECT_EQ(plan.effects[0].connection_index, 0u);
}
