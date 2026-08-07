//
// 1:1 lock the implicit default-branch dispatch for MP_PARTYWAR in
// [Server]Agent/AgentNetworkMsgParser.cpp. Each test pins one branch
// of the legacy behavior to its modern side-effect plan output so
// future drift triggers a test failure.

#include <gtest/gtest.h>

#include "mxh/server/agent_partywar.hpp"
#include "mxh/server/agent_partywar_side_effect_plan.hpp"

using namespace mxh::server;

TEST(AgentPartyWarPlan, UserFoundEmitsForwardEffect) {
    AgentPartyWarRequest r;
    r.protocol = partywar_nack;
    r.user_found = true;
    r.object_id = 0xCAFE1234u;
    const auto plan = agent_partywar_side_effect_plan(r, 17u);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentPartyWarSideEffectKind::ForwardToUser);
    EXPECT_EQ(plan.effects[0].reply_protocol, partywar_nack);
    EXPECT_EQ(plan.effects[0].connection_index, 17u);
    EXPECT_EQ(plan.effects[0].object_id, 0xCAFE1234u);
    EXPECT_TRUE(partywar_effect_targets_user(plan.effects[0]));
}

TEST(AgentPartyWarPlan, UserMissingEmitsDropEffect) {
    AgentPartyWarRequest r;
    r.protocol = partywar_nack;
    r.user_found = false;
    r.object_id = 0x12345678u;
    const auto plan = agent_partywar_side_effect_plan(r, 17u);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentPartyWarSideEffectKind::Drop);
    EXPECT_FALSE(partywar_effect_targets_user(plan.effects[0]));
    EXPECT_EQ(plan.effects[0].connection_index, 0u);
    EXPECT_EQ(plan.effects[0].object_id, 0x12345678u);
}

TEST(AgentPartyWarPlan, ForwardPlanPreservesObjectId) {
    AgentPartyWarRequest r;
    r.protocol = partywar_nack;
    r.user_found = true;
    r.object_id = 0xABCDEF01u;
    const auto plan = agent_partywar_side_effect_plan(r, 99u);
    EXPECT_EQ(plan.effects[0].object_id, 0xABCDEF01u);
    EXPECT_EQ(plan.effects[0].connection_index, 99u);
}

TEST(AgentPartyWarPlan, ForwardPlanPreservesProtocolByte) {
    const std::uint8_t all[] = {
        partywar_nack,
        partywar_suggest,
        partywar_suggest_wait,
        partywar_suggest_accept,
        partywar_suggest_deny,
        partywar_addmember_syn,
        partywar_addmember_ack,
        partywar_addmember_nack,
        partywar_removemember_syn,
        partywar_removemember_ack,
        partywar_removemember_nack,
        partywar_lock,
        partywar_unlock,
        partywar_start,
        partywar_cancel,
        partywar_ready,
        partywar_fight,
        partywar_result,
        partywar_end
    };
    for (std::uint8_t p : all) {
        AgentPartyWarRequest r;
        r.protocol = p;
        r.user_found = true;
        const auto plan = agent_partywar_side_effect_plan(r, 0u);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].reply_protocol, p)
            << "protocol=" << +p;
    }
}

TEST(AgentPartyWarPlan, DropEffectAlwaysEmittedWhenUserMissing) {
    const std::uint8_t all[] = {
        partywar_nack,
        partywar_suggest,
        partywar_suggest_wait,
        partywar_suggest_accept,
        partywar_suggest_deny,
        partywar_addmember_syn,
        partywar_addmember_ack,
        partywar_addmember_nack,
        partywar_removemember_syn,
        partywar_removemember_ack,
        partywar_removemember_nack,
        partywar_lock,
        partywar_unlock,
        partywar_start,
        partywar_cancel,
        partywar_ready,
        partywar_fight,
        partywar_result,
        partywar_end
    };
    for (std::uint8_t p : all) {
        AgentPartyWarRequest r;
        r.protocol = p;
        r.user_found = false;
        const auto plan = agent_partywar_side_effect_plan(r, 0u);
        EXPECT_TRUE(plan.drop);
        EXPECT_EQ(plan.effects[0].kind, AgentPartyWarSideEffectKind::Drop);
    }
}

TEST(AgentPartyWarPlan, DefaultPlanStructFieldsAreStable) {
    AgentPartyWarSideEffectPlan plan;
    EXPECT_FALSE(plan.dispatched);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(AgentPartyWarPlan, EffectTargetsUserPredicateMatchesForward) {
    AgentPartyWarSideEffect forward{};
    forward.kind = AgentPartyWarSideEffectKind::ForwardToUser;
    AgentPartyWarSideEffect drop{};
    drop.kind = AgentPartyWarSideEffectKind::Drop;
    EXPECT_TRUE(partywar_effect_targets_user(forward));
    EXPECT_FALSE(partywar_effect_targets_user(drop));
}

TEST(AgentPartyWarPlan, ForwardConnectionIndexEqualsResolvedConnection) {
    AgentPartyWarRequest r;
    r.user_found = true;
    r.object_id = 0x11223344u;
    const auto plan = agent_partywar_side_effect_plan(r, 0xFFFFAA00u);
    EXPECT_EQ(plan.effects[0].connection_index, 0xFFFFAA00u);
}

TEST(AgentPartyWarPlan, DropEffectCarriesObjectIdEvenWhenConnectionZero) {
    AgentPartyWarRequest r;
    r.user_found = false;
    r.object_id = 0xCAFEBABEu;
    const auto plan = agent_partywar_side_effect_plan(r, 1u);
    EXPECT_EQ(plan.effects[0].object_id, 0xCAFEBABEu);
    EXPECT_EQ(plan.effects[0].connection_index, 0u);
}
