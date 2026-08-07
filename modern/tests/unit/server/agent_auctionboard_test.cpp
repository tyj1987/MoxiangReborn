// D4 AgentAuctionBoard data plane tests.
//
// 1:1 port of the implicit default-branch behavior of
// [Server]Agent/AgentNetworkMsgParser.cpp for category MP_MP_AUCTIONBOARD.

#include <mxh/server/agent_auctionboard.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(AgentAuctionBoardClassify, CategoryConstantMatchesProtocolHeader) {
    EXPECT_EQ(mp_auctionboard_category, 10u);
}

TEST(AgentAuctionBoardClassify, SubProtocolConstantsAreUnique) {
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
    ASSERT_EQ(sizeof(all) / sizeof(all[0]), 19u);
    for (std::size_t i = 0; i < sizeof(all) / sizeof(all[0]); ++i) {
        for (std::size_t j = i + 1; j < sizeof(all) / sizeof(all[0]); ++j) {
            EXPECT_NE(all[i], all[j])
                << "duplicate protocol at i=" << i
                << " j=" << j;
        }
    }
}

TEST(AgentAuctionBoardClassify, SubProtocolsAreContiguousFromZero) {
    EXPECT_EQ(mp_auctionboard_open_syn, 0u);
    EXPECT_EQ(mp_auctionboard_closecontents, 18u);
}

TEST(AgentAuctionBoardClassify, UserFoundForwards) {
    AgentAuctionBoardRequest r;
    r.protocol = mp_auctionboard_open_syn;
    r.user_found = true;
    r.object_id = 0xDEADBEEFu;
    EXPECT_EQ(classify_agent_auctionboard(r), AgentAuctionBoardOutcome::ForwardToUser);
}

TEST(AgentAuctionBoardClassify, UserNotFoundDrops) {
    AgentAuctionBoardRequest r;
    r.protocol = mp_auctionboard_open_syn;
    r.user_found = false;
    EXPECT_EQ(classify_agent_auctionboard(r), AgentAuctionBoardOutcome::DropNoUser);
}

TEST(AgentAuctionBoardClassify, EverySubProtocolForwardsWhenUserFound) {
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
        EXPECT_EQ(classify_agent_auctionboard(r), AgentAuctionBoardOutcome::ForwardToUser)
            << "protocol=" << +p;
    }
}

TEST(AgentAuctionBoardClassify, EverySubProtocolDropsWhenUserMissing) {
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
        EXPECT_EQ(classify_agent_auctionboard(r), AgentAuctionBoardOutcome::DropNoUser)
            << "protocol=" << +p;
    }

    if (sizeof(all) / sizeof(all[0]) >= 2) {
        const std::uint8_t middle = all[sizeof(all) / sizeof(all[0]) / 2];
        AgentAuctionBoardRequest m;
        m.protocol = middle;
        m.user_found = true;
        EXPECT_EQ(classify_agent_auctionboard(m), AgentAuctionBoardOutcome::ForwardToUser);
    }
}

TEST(AgentAuctionBoardClassify, ObjectIdIgnoredInClassification) {
    AgentAuctionBoardRequest a;
    a.user_found = true;
    a.object_id = 0xFFFFFFFFu;
    AgentAuctionBoardRequest b = a;
    b.object_id = 0u;
    EXPECT_EQ(classify_agent_auctionboard(a), classify_agent_auctionboard(b));
}

TEST(AgentAuctionBoardClassify, OutcomeIsDeterministic) {
    AgentAuctionBoardRequest r;
    r.user_found = true;
    EXPECT_EQ(classify_agent_auctionboard(r), AgentAuctionBoardOutcome::ForwardToUser);
    EXPECT_EQ(classify_agent_auctionboard(r), AgentAuctionBoardOutcome::ForwardToUser);
}

TEST(AgentAuctionBoardClassify, UnknownProtocolStillForwardsWhenUserFound) {
    // Legacy does not validate the protocol byte for this category at the agent;
    // any protocol gets forwarded if user is found. Preserved verbatim.
    AgentAuctionBoardRequest r;
    r.protocol = 200u;
    r.user_found = true;
    EXPECT_EQ(classify_agent_auctionboard(r), AgentAuctionBoardOutcome::ForwardToUser);
}
