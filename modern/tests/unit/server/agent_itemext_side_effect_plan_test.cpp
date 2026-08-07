//
// 1:1 lock the implicit default-branch dispatch for MP_ITEMEXT in
// [Server]Agent/AgentNetworkMsgParser.cpp. Each test pins one branch
// of the legacy behavior to its modern side-effect plan output so
// future drift triggers a test failure.

#include <gtest/gtest.h>

#include "mxh/server/agent_itemext.hpp"
#include "mxh/server/agent_itemext_side_effect_plan.hpp"

using namespace mxh::server;

TEST(AgentItemExtPlan, UserFoundEmitsForwardEffect) {
    AgentItemExtRequest r;
    r.protocol = itemext_shopitem_curse_cancellation_additem_syn;
    r.user_found = true;
    r.object_id = 0xCAFE1234u;
    const auto plan = agent_itemext_side_effect_plan(r, 17u);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentItemExtSideEffectKind::ForwardToUser);
    EXPECT_EQ(plan.effects[0].reply_protocol, itemext_shopitem_curse_cancellation_additem_syn);
    EXPECT_EQ(plan.effects[0].connection_index, 17u);
    EXPECT_EQ(plan.effects[0].object_id, 0xCAFE1234u);
    EXPECT_TRUE(itemext_effect_targets_user(plan.effects[0]));
}

TEST(AgentItemExtPlan, UserMissingEmitsDropEffect) {
    AgentItemExtRequest r;
    r.protocol = itemext_shopitem_curse_cancellation_additem_syn;
    r.user_found = false;
    r.object_id = 0x12345678u;
    const auto plan = agent_itemext_side_effect_plan(r, 17u);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentItemExtSideEffectKind::Drop);
    EXPECT_FALSE(itemext_effect_targets_user(plan.effects[0]));
    EXPECT_EQ(plan.effects[0].connection_index, 0u);
    EXPECT_EQ(plan.effects[0].object_id, 0x12345678u);
}

TEST(AgentItemExtPlan, ForwardPlanPreservesObjectId) {
    AgentItemExtRequest r;
    r.protocol = itemext_shopitem_curse_cancellation_additem_syn;
    r.user_found = true;
    r.object_id = 0xABCDEF01u;
    const auto plan = agent_itemext_side_effect_plan(r, 99u);
    EXPECT_EQ(plan.effects[0].object_id, 0xABCDEF01u);
    EXPECT_EQ(plan.effects[0].connection_index, 99u);
}

TEST(AgentItemExtPlan, ForwardPlanPreservesProtocolByte) {
    const std::uint8_t all[] = {
        itemext_shopitem_curse_cancellation_additem_syn,
        itemext_shopitem_curse_cancellation_additem_ack,
        itemext_shopitem_curse_cancellation_additem_nack,
        itemext_shopitem_curse_cancellation_release,
        itemext_shopitem_curse_cancellation_deleteitem,
        itemext_shopitem_curse_cancellation_syn,
        itemext_shopitem_curse_cancellation_ack,
        itemext_shopitem_curse_cancellation_nack,
        itemext_uniqueitem_mix_additem_syn,
        itemext_uniqueitem_mix_additem_ack,
        itemext_uniqueitem_mix_additem_nack,
        itemext_uniqueitem_mix_release,
        itemext_uniqueitem_mix_deleteitem,
        itemext_uniqueitem_mix_syn,
        itemext_uniqueitem_mix_ack,
        itemext_uniqueitem_mix_nack,
        itemext_shopitem_decoration_on,
        itemext_skinitem_select_syn,
        itemext_skinitem_select_ack,
        itemext_skinitem_select_nack,
        itemext_skinitem_discard_ack
    };
    for (std::uint8_t p : all) {
        AgentItemExtRequest r;
        r.protocol = p;
        r.user_found = true;
        const auto plan = agent_itemext_side_effect_plan(r, 0u);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].reply_protocol, p)
            << "protocol=" << +p;
    }
}

TEST(AgentItemExtPlan, DropEffectAlwaysEmittedWhenUserMissing) {
    const std::uint8_t all[] = {
        itemext_shopitem_curse_cancellation_additem_syn,
        itemext_shopitem_curse_cancellation_additem_ack,
        itemext_shopitem_curse_cancellation_additem_nack,
        itemext_shopitem_curse_cancellation_release,
        itemext_shopitem_curse_cancellation_deleteitem,
        itemext_shopitem_curse_cancellation_syn,
        itemext_shopitem_curse_cancellation_ack,
        itemext_shopitem_curse_cancellation_nack,
        itemext_uniqueitem_mix_additem_syn,
        itemext_uniqueitem_mix_additem_ack,
        itemext_uniqueitem_mix_additem_nack,
        itemext_uniqueitem_mix_release,
        itemext_uniqueitem_mix_deleteitem,
        itemext_uniqueitem_mix_syn,
        itemext_uniqueitem_mix_ack,
        itemext_uniqueitem_mix_nack,
        itemext_shopitem_decoration_on,
        itemext_skinitem_select_syn,
        itemext_skinitem_select_ack,
        itemext_skinitem_select_nack,
        itemext_skinitem_discard_ack
    };
    for (std::uint8_t p : all) {
        AgentItemExtRequest r;
        r.protocol = p;
        r.user_found = false;
        const auto plan = agent_itemext_side_effect_plan(r, 0u);
        EXPECT_TRUE(plan.drop);
        EXPECT_EQ(plan.effects[0].kind, AgentItemExtSideEffectKind::Drop);
    }
}

TEST(AgentItemExtPlan, DefaultPlanStructFieldsAreStable) {
    AgentItemExtSideEffectPlan plan;
    EXPECT_FALSE(plan.dispatched);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(AgentItemExtPlan, EffectTargetsUserPredicateMatchesForward) {
    AgentItemExtSideEffect forward{};
    forward.kind = AgentItemExtSideEffectKind::ForwardToUser;
    AgentItemExtSideEffect drop{};
    drop.kind = AgentItemExtSideEffectKind::Drop;
    EXPECT_TRUE(itemext_effect_targets_user(forward));
    EXPECT_FALSE(itemext_effect_targets_user(drop));
}

TEST(AgentItemExtPlan, ForwardConnectionIndexEqualsResolvedConnection) {
    AgentItemExtRequest r;
    r.user_found = true;
    r.object_id = 0x11223344u;
    const auto plan = agent_itemext_side_effect_plan(r, 0xFFFFAA00u);
    EXPECT_EQ(plan.effects[0].connection_index, 0xFFFFAA00u);
}

TEST(AgentItemExtPlan, DropEffectCarriesObjectIdEvenWhenConnectionZero) {
    AgentItemExtRequest r;
    r.user_found = false;
    r.object_id = 0xCAFEBABEu;
    const auto plan = agent_itemext_side_effect_plan(r, 1u);
    EXPECT_EQ(plan.effects[0].object_id, 0xCAFEBABEu);
    EXPECT_EQ(plan.effects[0].connection_index, 0u);
}
