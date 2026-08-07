// D4 AgentAuction data plane tests.
//
// 1:1 port of the implicit default-branch behavior of
// [Server]Agent/AgentNetworkMsgParser.cpp for category MP_MP_AUCTION.

#include <mxh/server/agent_auction.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(AgentAuctionClassify, CategoryConstantMatchesProtocolHeader) {
    EXPECT_EQ(mp_auction_category, 17u);
}

TEST(AgentAuctionClassify, SubProtocolConstantsAreUnique) {
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
    ASSERT_EQ(sizeof(all) / sizeof(all[0]), 21u);
    for (std::size_t i = 0; i < sizeof(all) / sizeof(all[0]); ++i) {
        for (std::size_t j = i + 1; j < sizeof(all) / sizeof(all[0]); ++j) {
            EXPECT_NE(all[i], all[j])
                << "duplicate protocol at i=" << i
                << " j=" << j;
        }
    }
}

TEST(AgentAuctionClassify, SubProtocolsAreContiguousFromZero) {
    EXPECT_EQ(mp_auction_success_syn, 0u);
    EXPECT_EQ(mp_auction_cancel_nack, 20u);
}

TEST(AgentAuctionClassify, UserFoundForwards) {
    AgentAuctionRequest r;
    r.protocol = mp_auction_success_syn;
    r.user_found = true;
    r.object_id = 0xDEADBEEFu;
    EXPECT_EQ(classify_agent_auction(r), AgentAuctionOutcome::ForwardToUser);
}

TEST(AgentAuctionClassify, UserNotFoundDrops) {
    AgentAuctionRequest r;
    r.protocol = mp_auction_success_syn;
    r.user_found = false;
    EXPECT_EQ(classify_agent_auction(r), AgentAuctionOutcome::DropNoUser);
}

TEST(AgentAuctionClassify, EverySubProtocolForwardsWhenUserFound) {
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
        EXPECT_EQ(classify_agent_auction(r), AgentAuctionOutcome::ForwardToUser)
            << "protocol=" << +p;
    }
}

TEST(AgentAuctionClassify, EverySubProtocolDropsWhenUserMissing) {
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
        EXPECT_EQ(classify_agent_auction(r), AgentAuctionOutcome::DropNoUser)
            << "protocol=" << +p;
    }

    if (sizeof(all) / sizeof(all[0]) >= 2) {
        const std::uint8_t middle = all[sizeof(all) / sizeof(all[0]) / 2];
        AgentAuctionRequest m;
        m.protocol = middle;
        m.user_found = true;
        EXPECT_EQ(classify_agent_auction(m), AgentAuctionOutcome::ForwardToUser);
    }
}

TEST(AgentAuctionClassify, ObjectIdIgnoredInClassification) {
    AgentAuctionRequest a;
    a.user_found = true;
    a.object_id = 0xFFFFFFFFu;
    AgentAuctionRequest b = a;
    b.object_id = 0u;
    EXPECT_EQ(classify_agent_auction(a), classify_agent_auction(b));
}

TEST(AgentAuctionClassify, OutcomeIsDeterministic) {
    AgentAuctionRequest r;
    r.user_found = true;
    EXPECT_EQ(classify_agent_auction(r), AgentAuctionOutcome::ForwardToUser);
    EXPECT_EQ(classify_agent_auction(r), AgentAuctionOutcome::ForwardToUser);
}

TEST(AgentAuctionClassify, UnknownProtocolStillForwardsWhenUserFound) {
    // Legacy does not validate the protocol byte for this category at the agent;
    // any protocol gets forwarded if user is found. Preserved verbatim.
    AgentAuctionRequest r;
    r.protocol = 200u;
    r.user_found = true;
    EXPECT_EQ(classify_agent_auction(r), AgentAuctionOutcome::ForwardToUser);
}
