//
// 1:1 lock the implicit default-branch dispatch for MP_PK in
// [Server]Agent/AgentNetworkMsgParser.cpp. Each test pins one branch
// of the legacy behavior to its modern side-effect plan output so
// future drift triggers a test failure.

#include <gtest/gtest.h>

#include "mxh/server/agent_pk.hpp"
#include "mxh/server/agent_pk_side_effect_plan.hpp"

using namespace mxh::server;

TEST(AgentPkPlan, UserFoundEmitsForwardEffect) {
    AgentPkRequest r;
    r.protocol = pk_pkon_syn;
    r.user_found = true;
    r.object_id = 0xCAFE1234u;
    const auto plan = agent_pk_side_effect_plan(r, 17u);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentPkSideEffectKind::ForwardToUser);
    EXPECT_EQ(plan.effects[0].reply_protocol, pk_pkon_syn);
    EXPECT_EQ(plan.effects[0].connection_index, 17u);
    EXPECT_EQ(plan.effects[0].object_id, 0xCAFE1234u);
    EXPECT_TRUE(pk_effect_targets_user(plan.effects[0]));
}

TEST(AgentPkPlan, UserMissingEmitsDropEffect) {
    AgentPkRequest r;
    r.protocol = pk_pkon_syn;
    r.user_found = false;
    r.object_id = 0x12345678u;
    const auto plan = agent_pk_side_effect_plan(r, 17u);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentPkSideEffectKind::Drop);
    EXPECT_FALSE(pk_effect_targets_user(plan.effects[0]));
    EXPECT_EQ(plan.effects[0].connection_index, 0u);
    EXPECT_EQ(plan.effects[0].object_id, 0x12345678u);
}

TEST(AgentPkPlan, ForwardPlanPreservesObjectId) {
    AgentPkRequest r;
    r.protocol = pk_looting_select_syn;
    r.user_found = true;
    r.object_id = 0xABCDEF01u;
    const auto plan = agent_pk_side_effect_plan(r, 99u);
    EXPECT_EQ(plan.effects[0].object_id, 0xABCDEF01u);
    EXPECT_EQ(plan.effects[0].connection_index, 99u);
}

TEST(AgentPkPlan, ForwardPlanPreservesProtocolByte) {
    const std::uint8_t all[] = {
        pk_pkon_syn, pk_pkon_ack, pk_pkon_nack,
        pk_pkoff_syn, pk_pkoff_ack, pk_pkoff_nack,
        pk_looting_start, pk_looting_beinglooted,
        pk_looting_select_syn, pk_looting_select_ack, pk_looting_select_nack,
        pk_looting_itemlooting, pk_looting_itemlooted,
        pk_looting_moenylooting, pk_looting_moenylooted,
        pk_looting_explooting, pk_looting_explooted,
        pk_looting_nolooting, pk_looting_noinvenspace,
        pk_looting_endlooting, pk_destroy_item, pk_looting_error,
    };
    for (std::uint8_t p : all) {
        AgentPkRequest r;
        r.protocol = p;
        r.user_found = true;
        const auto plan = agent_pk_side_effect_plan(r, 0u);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].reply_protocol, p)
            << "protocol=" << +p;
    }
}

TEST(AgentPkPlan, DropEffectAlwaysEmittedWhenUserMissing) {
    const std::uint8_t all[] = {
        pk_pkon_syn, pk_pkon_ack, pk_pkon_nack,
        pk_pkoff_syn, pk_pkoff_ack, pk_pkoff_nack,
        pk_looting_start, pk_looting_beinglooted,
        pk_looting_select_syn, pk_looting_select_ack, pk_looting_select_nack,
        pk_looting_itemlooting, pk_looting_itemlooted,
        pk_looting_moenylooting, pk_looting_moenylooted,
        pk_looting_explooting, pk_looting_explooted,
        pk_looting_nolooting, pk_looting_noinvenspace,
        pk_looting_endlooting, pk_destroy_item, pk_looting_error,
    };
    for (std::uint8_t p : all) {
        AgentPkRequest r;
        r.protocol = p;
        r.user_found = false;
        const auto plan = agent_pk_side_effect_plan(r, 0u);
        EXPECT_TRUE(plan.drop);
        EXPECT_EQ(plan.effects[0].kind, AgentPkSideEffectKind::Drop);
    }
}

TEST(AgentPkPlan, DefaultPlanStructFieldsAreStable) {
    AgentPkSideEffectPlan plan;
    EXPECT_FALSE(plan.dispatched);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(AgentPkPlan, PkEffectTargetsUserPredicateMatchesForward) {
    AgentPkSideEffect forward{};
    forward.kind = AgentPkSideEffectKind::ForwardToUser;
    AgentPkSideEffect drop{};
    drop.kind = AgentPkSideEffectKind::Drop;
    EXPECT_TRUE(pk_effect_targets_user(forward));
    EXPECT_FALSE(pk_effect_targets_user(drop));
}

TEST(AgentPkPlan, ForwardConnectionIndexEqualsResolvedConnection) {
    AgentPkRequest r;
    r.user_found = true;
    r.object_id = 0x11223344u;
    const auto plan = agent_pk_side_effect_plan(r, 0xFFFFAA00u);
    EXPECT_EQ(plan.effects[0].connection_index, 0xFFFFAA00u);
}

TEST(AgentPkPlan, DropEffectCarriesObjectIdEvenWhenConnectionZero) {
    AgentPkRequest r;
    r.user_found = false;
    r.object_id = 0xCAFEBABEu;
    const auto plan = agent_pk_side_effect_plan(r, 1u);
    EXPECT_EQ(plan.effects[0].object_id, 0xCAFEBABEu);
    EXPECT_EQ(plan.effects[0].connection_index, 0u);
}

