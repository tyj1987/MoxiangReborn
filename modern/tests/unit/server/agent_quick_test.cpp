// D4 AgentQuick data plane tests.
//
// 1:1 port of the implicit default-branch behavior of
// [Server]Agent/AgentNetworkMsgParser.cpp for category MP_MP_QUICK.

#include <mxh/server/agent_quick.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(AgentQuickClassify, CategoryConstantMatchesProtocolHeader) {
    EXPECT_EQ(mp_quick_category, 12u);
}

TEST(AgentQuickClassify, SubProtocolConstantsAreUnique) {
    const std::uint8_t all[] = {
        mp_quick_add_syn,
        mp_quick_add_ack,
        mp_quick_add_nack,
        mp_quick_use_syn,
        mp_quick_use_ack,
        mp_quick_use_nack,
        mp_quick_move_syn,
        mp_quick_move_ack,
        mp_quick_move_nack,
        mp_quick_rem_syn,
        mp_quick_rem_ack,
        mp_quick_rem_nack,
        mp_quick_set_syn,
        mp_quick_set_ack,
        mp_quick_set_nack
    };
    ASSERT_EQ(sizeof(all) / sizeof(all[0]), 15u);
    for (std::size_t i = 0; i < sizeof(all) / sizeof(all[0]); ++i) {
        for (std::size_t j = i + 1; j < sizeof(all) / sizeof(all[0]); ++j) {
            EXPECT_NE(all[i], all[j])
                << "duplicate protocol at i=" << i
                << " j=" << j;
        }
    }
}

TEST(AgentQuickClassify, SubProtocolsAreContiguousFromZero) {
    EXPECT_EQ(mp_quick_add_syn, 0u);
    EXPECT_EQ(mp_quick_set_nack, 14u);
}

TEST(AgentQuickClassify, UserFoundForwards) {
    AgentQuickRequest r;
    r.protocol = mp_quick_add_syn;
    r.user_found = true;
    r.object_id = 0xDEADBEEFu;
    EXPECT_EQ(classify_agent_quick(r), AgentQuickOutcome::ForwardToUser);
}

TEST(AgentQuickClassify, UserNotFoundDrops) {
    AgentQuickRequest r;
    r.protocol = mp_quick_add_syn;
    r.user_found = false;
    EXPECT_EQ(classify_agent_quick(r), AgentQuickOutcome::DropNoUser);
}

TEST(AgentQuickClassify, EverySubProtocolForwardsWhenUserFound) {
    const std::uint8_t all[] = {
        mp_quick_add_syn,
        mp_quick_add_ack,
        mp_quick_add_nack,
        mp_quick_use_syn,
        mp_quick_use_ack,
        mp_quick_use_nack,
        mp_quick_move_syn,
        mp_quick_move_ack,
        mp_quick_move_nack,
        mp_quick_rem_syn,
        mp_quick_rem_ack,
        mp_quick_rem_nack,
        mp_quick_set_syn,
        mp_quick_set_ack,
        mp_quick_set_nack
    };
    for (std::uint8_t p : all) {
        AgentQuickRequest r;
        r.protocol = p;
        r.user_found = true;
        EXPECT_EQ(classify_agent_quick(r), AgentQuickOutcome::ForwardToUser)
            << "protocol=" << +p;
    }
}

TEST(AgentQuickClassify, EverySubProtocolDropsWhenUserMissing) {
    const std::uint8_t all[] = {
        mp_quick_add_syn,
        mp_quick_add_ack,
        mp_quick_add_nack,
        mp_quick_use_syn,
        mp_quick_use_ack,
        mp_quick_use_nack,
        mp_quick_move_syn,
        mp_quick_move_ack,
        mp_quick_move_nack,
        mp_quick_rem_syn,
        mp_quick_rem_ack,
        mp_quick_rem_nack,
        mp_quick_set_syn,
        mp_quick_set_ack,
        mp_quick_set_nack
    };
    for (std::uint8_t p : all) {
        AgentQuickRequest r;
        r.protocol = p;
        r.user_found = false;
        EXPECT_EQ(classify_agent_quick(r), AgentQuickOutcome::DropNoUser)
            << "protocol=" << +p;
    }

    if (sizeof(all) / sizeof(all[0]) >= 2) {
        const std::uint8_t middle = all[sizeof(all) / sizeof(all[0]) / 2];
        AgentQuickRequest m;
        m.protocol = middle;
        m.user_found = true;
        EXPECT_EQ(classify_agent_quick(m), AgentQuickOutcome::ForwardToUser);
    }
}

TEST(AgentQuickClassify, ObjectIdIgnoredInClassification) {
    AgentQuickRequest a;
    a.user_found = true;
    a.object_id = 0xFFFFFFFFu;
    AgentQuickRequest b = a;
    b.object_id = 0u;
    EXPECT_EQ(classify_agent_quick(a), classify_agent_quick(b));
}

TEST(AgentQuickClassify, OutcomeIsDeterministic) {
    AgentQuickRequest r;
    r.user_found = true;
    EXPECT_EQ(classify_agent_quick(r), AgentQuickOutcome::ForwardToUser);
    EXPECT_EQ(classify_agent_quick(r), AgentQuickOutcome::ForwardToUser);
}

TEST(AgentQuickClassify, UnknownProtocolStillForwardsWhenUserFound) {
    // Legacy does not validate the protocol byte for this category at the agent;
    // any protocol gets forwarded if user is found. Preserved verbatim.
    AgentQuickRequest r;
    r.protocol = 200u;
    r.user_found = true;
    EXPECT_EQ(classify_agent_quick(r), AgentQuickOutcome::ForwardToUser);
}
