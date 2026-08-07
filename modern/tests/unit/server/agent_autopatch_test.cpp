// D4 AgentAutoPatch data plane tests.
//
// 1:1 port of the implicit default-branch behavior of
// [Server]Agent/AgentNetworkMsgParser.cpp for category MP_MP_AUTOPATCH.

#include <mxh/server/agent_autopatch.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(AgentAutoPatchClassify, CategoryConstantMatchesProtocolHeader) {
    EXPECT_EQ(mp_autopatch_category, 18u);
}

TEST(AgentAutoPatchClassify, SubProtocolConstantsAreUnique) {
    const std::uint8_t all[] = {
        mp_autopatch_traffic_syn,
        mp_autopatch_traffic_ack,
        mp_autopatch_traffic_nack
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

TEST(AgentAutoPatchClassify, SubProtocolsAreContiguousFromZero) {
    EXPECT_EQ(mp_autopatch_traffic_syn, 0u);
    EXPECT_EQ(mp_autopatch_traffic_nack, 2u);
}

TEST(AgentAutoPatchClassify, UserFoundForwards) {
    AgentAutoPatchRequest r;
    r.protocol = mp_autopatch_traffic_syn;
    r.user_found = true;
    r.object_id = 0xDEADBEEFu;
    EXPECT_EQ(classify_agent_autopatch(r), AgentAutoPatchOutcome::ForwardToUser);
}

TEST(AgentAutoPatchClassify, UserNotFoundDrops) {
    AgentAutoPatchRequest r;
    r.protocol = mp_autopatch_traffic_syn;
    r.user_found = false;
    EXPECT_EQ(classify_agent_autopatch(r), AgentAutoPatchOutcome::DropNoUser);
}

TEST(AgentAutoPatchClassify, EverySubProtocolForwardsWhenUserFound) {
    const std::uint8_t all[] = {
        mp_autopatch_traffic_syn,
        mp_autopatch_traffic_ack,
        mp_autopatch_traffic_nack
    };
    for (std::uint8_t p : all) {
        AgentAutoPatchRequest r;
        r.protocol = p;
        r.user_found = true;
        EXPECT_EQ(classify_agent_autopatch(r), AgentAutoPatchOutcome::ForwardToUser)
            << "protocol=" << +p;
    }
}

TEST(AgentAutoPatchClassify, EverySubProtocolDropsWhenUserMissing) {
    const std::uint8_t all[] = {
        mp_autopatch_traffic_syn,
        mp_autopatch_traffic_ack,
        mp_autopatch_traffic_nack
    };
    for (std::uint8_t p : all) {
        AgentAutoPatchRequest r;
        r.protocol = p;
        r.user_found = false;
        EXPECT_EQ(classify_agent_autopatch(r), AgentAutoPatchOutcome::DropNoUser)
            << "protocol=" << +p;
    }

    if (sizeof(all) / sizeof(all[0]) >= 2) {
        const std::uint8_t middle = all[sizeof(all) / sizeof(all[0]) / 2];
        AgentAutoPatchRequest m;
        m.protocol = middle;
        m.user_found = true;
        EXPECT_EQ(classify_agent_autopatch(m), AgentAutoPatchOutcome::ForwardToUser);
    }
}

TEST(AgentAutoPatchClassify, ObjectIdIgnoredInClassification) {
    AgentAutoPatchRequest a;
    a.user_found = true;
    a.object_id = 0xFFFFFFFFu;
    AgentAutoPatchRequest b = a;
    b.object_id = 0u;
    EXPECT_EQ(classify_agent_autopatch(a), classify_agent_autopatch(b));
}

TEST(AgentAutoPatchClassify, OutcomeIsDeterministic) {
    AgentAutoPatchRequest r;
    r.user_found = true;
    EXPECT_EQ(classify_agent_autopatch(r), AgentAutoPatchOutcome::ForwardToUser);
    EXPECT_EQ(classify_agent_autopatch(r), AgentAutoPatchOutcome::ForwardToUser);
}

TEST(AgentAutoPatchClassify, UnknownProtocolStillForwardsWhenUserFound) {
    // Legacy does not validate the protocol byte for this category at the agent;
    // any protocol gets forwarded if user is found. Preserved verbatim.
    AgentAutoPatchRequest r;
    r.protocol = 200u;
    r.user_found = true;
    EXPECT_EQ(classify_agent_autopatch(r), AgentAutoPatchOutcome::ForwardToUser);
}
