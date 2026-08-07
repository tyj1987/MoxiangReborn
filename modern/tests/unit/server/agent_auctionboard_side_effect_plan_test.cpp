//
// 1:1 lock the implicit default-branch dispatch for MP_MP_AUCTIONBOARD in
// [Server]Agent/AgentNetworkMsgParser.cpp. Each test pins one branch
// of the legacy behavior to its modern side-effect plan output so
// future drift triggers a test failure.

#include <gtest/gtest.h>

#include "mxh/server/agent_auctionboard.hpp"
#include "mxh/server/agent_auctionboard_side_effect_plan.hpp"

using namespace mxh::server;

TEST(AgentAuctionBoardPlan, UserFoundEmitsForwardEffect) {
    AgentAuctionBoardRequest r;
    r.protocol = mp_auctionboard_open_syn;
    r.user_found = true;
    r.object_id = 0xCAFE1234u;
    const auto plan = agent_auctionboard_side_effect_plan(r, 17u);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentAuctionBoardSideEffectKind::ForwardToUser);
    EXPECT_EQ(plan.effects[0].reply_protocol, mp_auctionboard_open_syn);
    EXPECT_EQ(plan.effects[0].connection_index, 17u);
    EXPECT_EQ(plan.effects[0].object_id, 0xCAFE1234u);
    EXPECT_TRUE(mp_auctionboard_effect_targets_user(plan.effects[0]));
}

TEST(AgentAuctionBoardPlan, UserMissingEmitsDropEffect) {
    AgentAuctionBoardRequest r;
    r.protocol = mp_auctionboard_open_syn;
    r.user_found = false;
    r.object_id = 0x12345678u;
    const auto plan = agent_auctionboard_side_effect_plan(r, 17u);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentAuctionBoardSideEffectKind::Drop);
    EXPECT_FALSE(mp_auctionboard_effect_targets_user(plan.effects[0]));
    EXPECT_EQ(plan.effects[0].connection_index, 0u);
    EXPECT_EQ(plan.effects[0].object_id, 0x12345678u);
}

TEST(AgentAuctionBoardPlan, ForwardPlanPreservesObjectId) {
    AgentAuctionBoardRequest r;
    r.protocol = mp_auctionboard_open_syn;
    r.user_found = true;
    r.object_id = 0xABCDEF01u;
    const auto plan = agent_auctionboard_side_effect_plan(r, 99u);
    EXPECT_EQ(plan.effects[0].object_id, 0xABCDEF01u);
    EXPECT_EQ(plan.effects[0].connection_index, 99u);
}

TEST(AgentAuctionBoardPlan, ForwardPlanPreservesProtocolByte) {
    const std::uint8_t all[] = {
        mp_auctionboard_open_syn,
        mp_auctionboard_open_ack,
        mp_auctionboard_open_nack,
        mp_auctionboard_list_syn,
        mp_auctionboard_list_ack,
        mp_auctionboard_list_nack,
        mp_auctionboard_contents_syn,
        mp_auctionboard_contents_ack,
        mp_auctionboard_contents_nack,
        mp_auctionboard_write_syn,
        mp_auctionboard_write_ack,
        mp_auctionboard_write_nack,
        mp_auctionboard_delete_syn,
        mp_auctionboard_delete_ack,
        mp_auctionboard_delete_nack,
        mp_auctionboard_bid_syn,
        mp_auctionboard_bid_ack,
        mp_auctionboard_bid_nack,
        mp_auctionboard_closecontents
    };
    for (std::uint8_t p : all) {
        AgentAuctionBoardRequest r;
        r.protocol = p;
        r.user_found = true;
        const auto plan = agent_auctionboard_side_effect_plan(r, 0u);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].reply_protocol, p)
            << "protocol=" << +p;
    }
}

TEST(AgentAuctionBoardPlan, DropEffectAlwaysEmittedWhenUserMissing) {
    const std::uint8_t all[] = {
        mp_auctionboard_open_syn,
        mp_auctionboard_open_ack,
        mp_auctionboard_open_nack,
        mp_auctionboard_list_syn,
        mp_auctionboard_list_ack,
        mp_auctionboard_list_nack,
        mp_auctionboard_contents_syn,
        mp_auctionboard_contents_ack,
        mp_auctionboard_contents_nack,
        mp_auctionboard_write_syn,
        mp_auctionboard_write_ack,
        mp_auctionboard_write_nack,
        mp_auctionboard_delete_syn,
        mp_auctionboard_delete_ack,
        mp_auctionboard_delete_nack,
        mp_auctionboard_bid_syn,
        mp_auctionboard_bid_ack,
        mp_auctionboard_bid_nack,
        mp_auctionboard_closecontents
    };
    for (std::uint8_t p : all) {
        AgentAuctionBoardRequest r;
        r.protocol = p;
        r.user_found = false;
        const auto plan = agent_auctionboard_side_effect_plan(r, 0u);
        EXPECT_TRUE(plan.drop);
        EXPECT_EQ(plan.effects[0].kind, AgentAuctionBoardSideEffectKind::Drop);
    }
}

TEST(AgentAuctionBoardPlan, DefaultPlanStructFieldsAreStable) {
    AgentAuctionBoardSideEffectPlan plan;
    EXPECT_FALSE(plan.dispatched);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(AgentAuctionBoardPlan, EffectTargetsUserPredicateMatchesForward) {
    AgentAuctionBoardSideEffect forward{};
    forward.kind = AgentAuctionBoardSideEffectKind::ForwardToUser;
    AgentAuctionBoardSideEffect drop{};
    drop.kind = AgentAuctionBoardSideEffectKind::Drop;
    EXPECT_TRUE(mp_auctionboard_effect_targets_user(forward));
    EXPECT_FALSE(mp_auctionboard_effect_targets_user(drop));
}

TEST(AgentAuctionBoardPlan, ForwardConnectionIndexEqualsResolvedConnection) {
    AgentAuctionBoardRequest r;
    r.user_found = true;
    r.object_id = 0x11223344u;
    const auto plan = agent_auctionboard_side_effect_plan(r, 0xFFFFAA00u);
    EXPECT_EQ(plan.effects[0].connection_index, 0xFFFFAA00u);
}

TEST(AgentAuctionBoardPlan, DropEffectCarriesObjectIdEvenWhenConnectionZero) {
    AgentAuctionBoardRequest r;
    r.user_found = false;
    r.object_id = 0xCAFEBABEu;
    const auto plan = agent_auctionboard_side_effect_plan(r, 1u);
    EXPECT_EQ(plan.effects[0].object_id, 0xCAFEBABEu);
    EXPECT_EQ(plan.effects[0].connection_index, 0u);
}
