//
// 1:1 lock the implicit default-branch dispatch for MP_NPC in
// [Server]Agent/AgentNetworkMsgParser.cpp. Each test pins one branch
// of the legacy behavior to its modern side-effect plan output so
// future drift triggers a test failure.

#include <gtest/gtest.h>

#include "mxh/server/agent_npc.hpp"
#include "mxh/server/agent_npc_side_effect_plan.hpp"

using namespace mxh::server;

TEST(AgentNpcPlan, UserFoundEmitsForwardEffect) {
    AgentNpcRequest r;
    r.protocol = npc_speech_syn;
    r.user_found = true;
    r.object_id = 0xCAFE1234u;
    const auto plan = agent_npc_side_effect_plan(r, 17u);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentNpcSideEffectKind::ForwardToUser);
    EXPECT_EQ(plan.effects[0].reply_protocol, npc_speech_syn);
    EXPECT_EQ(plan.effects[0].connection_index, 17u);
    EXPECT_EQ(plan.effects[0].object_id, 0xCAFE1234u);
    EXPECT_TRUE(npc_effect_targets_user(plan.effects[0]));
}

TEST(AgentNpcPlan, UserMissingEmitsDropEffect) {
    AgentNpcRequest r;
    r.protocol = npc_speech_syn;
    r.user_found = false;
    r.object_id = 0x12345678u;
    const auto plan = agent_npc_side_effect_plan(r, 17u);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentNpcSideEffectKind::Drop);
    EXPECT_FALSE(npc_effect_targets_user(plan.effects[0]));
    EXPECT_EQ(plan.effects[0].connection_index, 0u);
    EXPECT_EQ(plan.effects[0].object_id, 0x12345678u);
}

TEST(AgentNpcPlan, ForwardPlanPreservesObjectId) {
    AgentNpcRequest r;
    r.protocol = npc_dojob_syn;
    r.user_found = true;
    r.object_id = 0xABCDEF01u;
    const auto plan = agent_npc_side_effect_plan(r, 99u);
    EXPECT_EQ(plan.effects[0].object_id, 0xABCDEF01u);
    EXPECT_EQ(plan.effects[0].connection_index, 99u);
}

TEST(AgentNpcPlan, ForwardPlanPreservesProtocolByte) {
    const std::uint8_t all[] = {
        npc_speech_syn, npc_speech_ack, npc_speech_nack,
        npc_closebomul_syn, npc_closebomul_ack, npc_closebomul_nack,
        npc_openbomul_syn, npc_openbomul_ack, npc_openbomul_nack,
        npc_dojob_syn, npc_dojob_ack, npc_dojob_nack,
        npc_die_ack,
    };
    for (std::uint8_t p : all) {
        AgentNpcRequest r;
        r.protocol = p;
        r.user_found = true;
        const auto plan = agent_npc_side_effect_plan(r, 0u);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].reply_protocol, p)
            << "protocol=" << +p;
    }
}

TEST(AgentNpcPlan, DropEffectAlwaysEmittedWhenUserMissing) {
    const std::uint8_t all[] = {
        npc_speech_syn, npc_speech_ack, npc_speech_nack,
        npc_closebomul_syn, npc_closebomul_ack, npc_closebomul_nack,
        npc_openbomul_syn, npc_openbomul_ack, npc_openbomul_nack,
        npc_dojob_syn, npc_dojob_ack, npc_dojob_nack,
        npc_die_ack,
    };
    for (std::uint8_t p : all) {
        AgentNpcRequest r;
        r.protocol = p;
        r.user_found = false;
        const auto plan = agent_npc_side_effect_plan(r, 0u);
        EXPECT_TRUE(plan.drop);
        EXPECT_EQ(plan.effects[0].kind, AgentNpcSideEffectKind::Drop);
    }
}

TEST(AgentNpcPlan, DefaultPlanStructFieldsAreStable) {
    AgentNpcSideEffectPlan plan;
    EXPECT_FALSE(plan.dispatched);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(AgentNpcPlan, NpcEffectTargetsUserPredicateMatchesForward) {
    AgentNpcSideEffect forward{};
    forward.kind = AgentNpcSideEffectKind::ForwardToUser;
    AgentNpcSideEffect drop{};
    drop.kind = AgentNpcSideEffectKind::Drop;
    EXPECT_TRUE(npc_effect_targets_user(forward));
    EXPECT_FALSE(npc_effect_targets_user(drop));
}

TEST(AgentNpcPlan, ForwardConnectionIndexEqualsResolvedConnection) {
    AgentNpcRequest r;
    r.user_found = true;
    r.object_id = 0x11223344u;
    const auto plan = agent_npc_side_effect_plan(r, 0xFFFFAA00u);
    EXPECT_EQ(plan.effects[0].connection_index, 0xFFFFAA00u);
}

TEST(AgentNpcPlan, DropEffectCarriesObjectIdEvenWhenConnectionZero) {
    AgentNpcRequest r;
    r.user_found = false;
    r.object_id = 0xCAFEBABEu;
    const auto plan = agent_npc_side_effect_plan(r, 1u);
    EXPECT_EQ(plan.effects[0].object_id, 0xCAFEBABEu);
    EXPECT_EQ(plan.effects[0].connection_index, 0u);
}

