// D4 AgentSimBub data plane tests.
//
// 1:1 port of the implicit default-branch behavior of
// [Server]Agent/AgentNetworkMsgParser.cpp for category MP_SIMBUB.

#include <mxh/server/agent_simbub.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(AgentSimBubClassify, CategoryConstantMatchesProtocolHeader) {
    EXPECT_EQ(simbub_category, 24u);
}

TEST(AgentSimBubClassify, SubProtocolConstantsAreUnique) {
    const std::uint8_t all[] = {
        simbub_change_syn,
        simbub_change_ack,
        simbub_change_nack
    };
    ASSERT_EQ(sizeof(all) / sizeof(all[0]), 3u);
    for (std::size_t i = 0; i < sizeof(all) / sizeof(all[0]); ++i) {
        for (std::size_t j = i + 1; j < sizeof(all) / sizeof(all[0]); ++j) {
            EXPECT_NE(all[i], all[j])
                << "duplicate protocol at i=" << i
                << " j=" << j;
        }
    }
}

TEST(AgentSimBubClassify, SubProtocolsAreContiguousFromZero) {
    EXPECT_EQ(simbub_change_syn, 0u);
    EXPECT_EQ(simbub_change_nack, 2u);
}

TEST(AgentSimBubClassify, UserFoundForwards) {
    AgentSimBubRequest r;
    r.protocol = simbub_change_syn;
    r.user_found = true;
    r.object_id = 0xDEADBEEFu;
    EXPECT_EQ(classify_agent_simbub(r), AgentSimBubOutcome::ForwardToUser);
}

TEST(AgentSimBubClassify, UserNotFoundDrops) {
    AgentSimBubRequest r;
    r.protocol = simbub_change_syn;
    r.user_found = false;
    EXPECT_EQ(classify_agent_simbub(r), AgentSimBubOutcome::DropNoUser);
}

TEST(AgentSimBubClassify, EverySubProtocolForwardsWhenUserFound) {
    const std::uint8_t all[] = {
        simbub_change_syn,
        simbub_change_ack,
        simbub_change_nack
    };
    for (std::uint8_t p : all) {
        AgentSimBubRequest r;
        r.protocol = p;
        r.user_found = true;
        EXPECT_EQ(classify_agent_simbub(r), AgentSimBubOutcome::ForwardToUser)
            << "protocol=" << +p;
    }
}

TEST(AgentSimBubClassify, EverySubProtocolDropsWhenUserMissing) {
    const std::uint8_t all[] = {
        simbub_change_syn,
        simbub_change_ack,
        simbub_change_nack
    };
    for (std::uint8_t p : all) {
        AgentSimBubRequest r;
        r.protocol = p;
        r.user_found = false;
        EXPECT_EQ(classify_agent_simbub(r), AgentSimBubOutcome::DropNoUser)
            << "protocol=" << +p;
    }
}

TEST(AgentSimBubClassify, ObjectIdIgnoredInClassification) {
    AgentSimBubRequest a;
    a.user_found = true;
    a.object_id = 0xFFFFFFFFu;
    AgentSimBubRequest b = a;
    b.object_id = 0u;
    EXPECT_EQ(classify_agent_simbub(a), classify_agent_simbub(b));
}

TEST(AgentSimBubClassify, OutcomeIsDeterministic) {
    AgentSimBubRequest r;
    r.user_found = true;
    EXPECT_EQ(classify_agent_simbub(r), AgentSimBubOutcome::ForwardToUser);
    EXPECT_EQ(classify_agent_simbub(r), AgentSimBubOutcome::ForwardToUser);
}

TEST(AgentSimBubClassify, UnknownProtocolStillForwardsWhenUserFound) {
    // Legacy does not validate the protocol byte for this category at the agent;
    // any protocol gets forwarded if user is found. Preserved verbatim.
    AgentSimBubRequest r;
    r.protocol = 200u;
    r.user_found = true;
    EXPECT_EQ(classify_agent_simbub(r), AgentSimBubOutcome::ForwardToUser);
}
