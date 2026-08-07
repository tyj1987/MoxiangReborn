//
// 1:1 lock the implicit default-branch dispatch for MP_TITAN in
// [Server]Agent/AgentNetworkMsgParser.cpp. Each test pins one branch
// of the legacy behavior to its modern side-effect plan output so
// future drift triggers a test failure.

#include <gtest/gtest.h>

#include "mxh/server/agent_titan.hpp"
#include "mxh/server/agent_titan_side_effect_plan.hpp"

using namespace mxh::server;

TEST(AgentTitanPlan, UserFoundEmitsForwardEffect) {
    AgentTitanRequest r;
    r.protocol = titan_valueinfo;
    r.user_found = true;
    r.object_id = 0xCAFE1234u;
    const auto plan = agent_titan_side_effect_plan(r, 17u);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentTitanSideEffectKind::ForwardToUser);
    EXPECT_EQ(plan.effects[0].reply_protocol, titan_valueinfo);
    EXPECT_EQ(plan.effects[0].connection_index, 17u);
    EXPECT_EQ(plan.effects[0].object_id, 0xCAFE1234u);
    EXPECT_TRUE(titan_effect_targets_user(plan.effects[0]));
}

TEST(AgentTitanPlan, UserMissingEmitsDropEffect) {
    AgentTitanRequest r;
    r.protocol = titan_valueinfo;
    r.user_found = false;
    r.object_id = 0x12345678u;
    const auto plan = agent_titan_side_effect_plan(r, 17u);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentTitanSideEffectKind::Drop);
    EXPECT_FALSE(titan_effect_targets_user(plan.effects[0]));
    EXPECT_EQ(plan.effects[0].connection_index, 0u);
    EXPECT_EQ(plan.effects[0].object_id, 0x12345678u);
}

TEST(AgentTitanPlan, ForwardPlanPreservesObjectId) {
    AgentTitanRequest r;
    r.protocol = titan_valueinfo;
    r.user_found = true;
    r.object_id = 0xABCDEF01u;
    const auto plan = agent_titan_side_effect_plan(r, 99u);
    EXPECT_EQ(plan.effects[0].object_id, 0xABCDEF01u);
    EXPECT_EQ(plan.effects[0].connection_index, 99u);
}

TEST(AgentTitanPlan, ForwardPlanPreservesProtocolByte) {
    const std::uint8_t all[] = {
        titan_valueinfo,
        titan_fuel_ack,
        titan_spell_ack,
        titan_recall_syn,
        titan_recall_ack,
        titan_recall_nack,
        titan_recall_cancel_syn,
        titan_recall_cancel_ack,
        titan_recall_cancel_nack,
        titan_ridein_syn,
        titan_ridein_ack,
        titan_getoff_ack,
        titan_make_syn,
        titan_make_ack,
        titan_make_nack,
        titan_addnew_fromitem,
        titan_addnew_equip_fromitem,
        titan_statinfo,
        titan_endurance_update
    };
    for (std::uint8_t p : all) {
        AgentTitanRequest r;
        r.protocol = p;
        r.user_found = true;
        const auto plan = agent_titan_side_effect_plan(r, 0u);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].reply_protocol, p)
            << "protocol=" << +p;
    }
}

TEST(AgentTitanPlan, DropEffectAlwaysEmittedWhenUserMissing) {
    const std::uint8_t all[] = {
        titan_valueinfo,
        titan_fuel_ack,
        titan_spell_ack,
        titan_recall_syn,
        titan_recall_ack,
        titan_recall_nack,
        titan_recall_cancel_syn,
        titan_recall_cancel_ack,
        titan_recall_cancel_nack,
        titan_ridein_syn,
        titan_ridein_ack,
        titan_getoff_ack,
        titan_make_syn,
        titan_make_ack,
        titan_make_nack,
        titan_addnew_fromitem,
        titan_addnew_equip_fromitem,
        titan_statinfo,
        titan_endurance_update
    };
    for (std::uint8_t p : all) {
        AgentTitanRequest r;
        r.protocol = p;
        r.user_found = false;
        const auto plan = agent_titan_side_effect_plan(r, 0u);
        EXPECT_TRUE(plan.drop);
        EXPECT_EQ(plan.effects[0].kind, AgentTitanSideEffectKind::Drop);
    }
}

TEST(AgentTitanPlan, DefaultPlanStructFieldsAreStable) {
    AgentTitanSideEffectPlan plan;
    EXPECT_FALSE(plan.dispatched);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(AgentTitanPlan, EffectTargetsUserPredicateMatchesForward) {
    AgentTitanSideEffect forward{};
    forward.kind = AgentTitanSideEffectKind::ForwardToUser;
    AgentTitanSideEffect drop{};
    drop.kind = AgentTitanSideEffectKind::Drop;
    EXPECT_TRUE(titan_effect_targets_user(forward));
    EXPECT_FALSE(titan_effect_targets_user(drop));
}

TEST(AgentTitanPlan, ForwardConnectionIndexEqualsResolvedConnection) {
    AgentTitanRequest r;
    r.user_found = true;
    r.object_id = 0x11223344u;
    const auto plan = agent_titan_side_effect_plan(r, 0xFFFFAA00u);
    EXPECT_EQ(plan.effects[0].connection_index, 0xFFFFAA00u);
}

TEST(AgentTitanPlan, DropEffectCarriesObjectIdEvenWhenConnectionZero) {
    AgentTitanRequest r;
    r.user_found = false;
    r.object_id = 0xCAFEBABEu;
    const auto plan = agent_titan_side_effect_plan(r, 1u);
    EXPECT_EQ(plan.effects[0].object_id, 0xCAFEBABEu);
    EXPECT_EQ(plan.effects[0].connection_index, 0u);
}
