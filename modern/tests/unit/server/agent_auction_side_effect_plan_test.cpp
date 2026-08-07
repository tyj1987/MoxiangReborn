//
// 1:1 lock the implicit default-branch dispatch for MP_MP_AUCTION in
// [Server]Agent/AgentNetworkMsgParser.cpp. Each test pins one branch
// of the legacy behavior to its modern side-effect plan output so
// future drift triggers a test failure.

#include <gtest/gtest.h>

#include "mxh/server/agent_auction.hpp"
#include "mxh/server/agent_auction_side_effect_plan.hpp"

using namespace mxh::server;

TEST(AgentAuctionPlan, UserFoundEmitsForwardEffect) {
    AgentAuctionRequest r;
    r.protocol = mp_auction_success_syn;
    r.user_found = true;
    r.object_id = 0xCAFE1234u;
    const auto plan = agent_auction_side_effect_plan(r, 17u);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentAuctionSideEffectKind::ForwardToUser);
    EXPECT_EQ(plan.effects[0].reply_protocol, mp_auction_success_syn);
    EXPECT_EQ(plan.effects[0].connection_index, 17u);
    EXPECT_EQ(plan.effects[0].object_id, 0xCAFE1234u);
    EXPECT_TRUE(mp_auction_effect_targets_user(plan.effects[0]));
}

TEST(AgentAuctionPlan, UserMissingEmitsDropEffect) {
    AgentAuctionRequest r;
    r.protocol = mp_auction_success_syn;
    r.user_found = false;
    r.object_id = 0x12345678u;
    const auto plan = agent_auction_side_effect_plan(r, 17u);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentAuctionSideEffectKind::Drop);
    EXPECT_FALSE(mp_auction_effect_targets_user(plan.effects[0]));
    EXPECT_EQ(plan.effects[0].connection_index, 0u);
    EXPECT_EQ(plan.effects[0].object_id, 0x12345678u);
}

TEST(AgentAuctionPlan, ForwardPlanPreservesObjectId) {
    AgentAuctionRequest r;
    r.protocol = mp_auction_success_syn;
    r.user_found = true;
    r.object_id = 0xABCDEF01u;
    const auto plan = agent_auction_side_effect_plan(r, 99u);
    EXPECT_EQ(plan.effects[0].object_id, 0xABCDEF01u);
    EXPECT_EQ(plan.effects[0].connection_index, 99u);
}

TEST(AgentAuctionPlan, ForwardPlanPreservesProtocolByte) {
    const std::uint8_t all[] = {
        mp_auction_success_syn,
        mp_auction_success_ack,
        mp_auction_success_nack,
        mp_auction_search_syn,
        mp_auction_search_ack,
        mp_auction_search_nack,
        mp_auction_sort_syn,
        mp_auction_sort_ack,
        mp_auction_sort_nack,
        mp_auction_register_ok_syn,
        mp_auction_register_ok_ack,
        mp_auction_register_ok_nack,
        mp_auction_registser_cancel_syn,
        mp_auction_registser_cancel_ack,
        mp_auction_registser_cancel_nack,
        mp_auction_join_ok_syn,
        mp_auction_join_ok_ack,
        mp_auction_join_ok_nack,
        mp_auction_cancel_syn,
        mp_auction_cancel_ack,
        mp_auction_cancel_nack
    };
    for (std::uint8_t p : all) {
        AgentAuctionRequest r;
        r.protocol = p;
        r.user_found = true;
        const auto plan = agent_auction_side_effect_plan(r, 0u);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].reply_protocol, p)
            << "protocol=" << +p;
    }
}

TEST(AgentAuctionPlan, DropEffectAlwaysEmittedWhenUserMissing) {
    const std::uint8_t all[] = {
        mp_auction_success_syn,
        mp_auction_success_ack,
        mp_auction_success_nack,
        mp_auction_search_syn,
        mp_auction_search_ack,
        mp_auction_search_nack,
        mp_auction_sort_syn,
        mp_auction_sort_ack,
        mp_auction_sort_nack,
        mp_auction_register_ok_syn,
        mp_auction_register_ok_ack,
        mp_auction_register_ok_nack,
        mp_auction_registser_cancel_syn,
        mp_auction_registser_cancel_ack,
        mp_auction_registser_cancel_nack,
        mp_auction_join_ok_syn,
        mp_auction_join_ok_ack,
        mp_auction_join_ok_nack,
        mp_auction_cancel_syn,
        mp_auction_cancel_ack,
        mp_auction_cancel_nack
    };
    for (std::uint8_t p : all) {
        AgentAuctionRequest r;
        r.protocol = p;
        r.user_found = false;
        const auto plan = agent_auction_side_effect_plan(r, 0u);
        EXPECT_TRUE(plan.drop);
        EXPECT_EQ(plan.effects[0].kind, AgentAuctionSideEffectKind::Drop);
    }
}

TEST(AgentAuctionPlan, DefaultPlanStructFieldsAreStable) {
    AgentAuctionSideEffectPlan plan;
    EXPECT_FALSE(plan.dispatched);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(AgentAuctionPlan, EffectTargetsUserPredicateMatchesForward) {
    AgentAuctionSideEffect forward{};
    forward.kind = AgentAuctionSideEffectKind::ForwardToUser;
    AgentAuctionSideEffect drop{};
    drop.kind = AgentAuctionSideEffectKind::Drop;
    EXPECT_TRUE(mp_auction_effect_targets_user(forward));
    EXPECT_FALSE(mp_auction_effect_targets_user(drop));
}

TEST(AgentAuctionPlan, ForwardConnectionIndexEqualsResolvedConnection) {
    AgentAuctionRequest r;
    r.user_found = true;
    r.object_id = 0x11223344u;
    const auto plan = agent_auction_side_effect_plan(r, 0xFFFFAA00u);
    EXPECT_EQ(plan.effects[0].connection_index, 0xFFFFAA00u);
}

TEST(AgentAuctionPlan, DropEffectCarriesObjectIdEvenWhenConnectionZero) {
    AgentAuctionRequest r;
    r.user_found = false;
    r.object_id = 0xCAFEBABEu;
    const auto plan = agent_auction_side_effect_plan(r, 1u);
    EXPECT_EQ(plan.effects[0].object_id, 0xCAFEBABEu);
    EXPECT_EQ(plan.effects[0].connection_index, 0u);
}
