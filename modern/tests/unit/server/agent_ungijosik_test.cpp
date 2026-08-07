// D4 AgentUngiJosik data plane tests.
//
// 1:1 port of the implicit default-branch behavior of
// [Server]Agent/AgentNetworkMsgParser.cpp for category MP_MP_UNGIJOSIK.

#include <mxh/server/agent_ungijosik.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(AgentUngiJosikClassify, CategoryConstantMatchesProtocolHeader) {
    EXPECT_EQ(mp_ungijosik_category, 16u);
}

TEST(AgentUngiJosikClassify, SubProtocolConstantsAreUnique) {
    const std::uint8_t all[] = {
        mp_ungijosik_start,
        mp_ungijosik_end
    };
    ASSERT_EQ(sizeof(all) / sizeof(all[0]), 2u);
    for (std::size_t i = 0; i < sizeof(all) / sizeof(all[0]); ++i) {
        for (std::size_t j = i + 1; j < sizeof(all) / sizeof(all[0]); ++j) {
            EXPECT_NE(all[i], all[j])
                << "duplicate protocol at i=" << i
                << " j=" << j;
        }
    }
}

TEST(AgentUngiJosikClassify, SubProtocolsAreContiguousFromZero) {
    EXPECT_EQ(mp_ungijosik_start, 0u);
    EXPECT_EQ(mp_ungijosik_end, 1u);
}

TEST(AgentUngiJosikClassify, UserFoundForwards) {
    AgentUngiJosikRequest r;
    r.protocol = mp_ungijosik_start;
    r.user_found = true;
    r.object_id = 0xDEADBEEFu;
    EXPECT_EQ(classify_agent_ungijosik(r), AgentUngiJosikOutcome::ForwardToUser);
}

TEST(AgentUngiJosikClassify, UserNotFoundDrops) {
    AgentUngiJosikRequest r;
    r.protocol = mp_ungijosik_start;
    r.user_found = false;
    EXPECT_EQ(classify_agent_ungijosik(r), AgentUngiJosikOutcome::DropNoUser);
}

TEST(AgentUngiJosikClassify, EverySubProtocolForwardsWhenUserFound) {
    const std::uint8_t all[] = {
        mp_ungijosik_start,
        mp_ungijosik_end
    };
    for (std::uint8_t p : all) {
        AgentUngiJosikRequest r;
        r.protocol = p;
        r.user_found = true;
        EXPECT_EQ(classify_agent_ungijosik(r), AgentUngiJosikOutcome::ForwardToUser)
            << "protocol=" << +p;
    }
}

TEST(AgentUngiJosikClassify, EverySubProtocolDropsWhenUserMissing) {
    const std::uint8_t all[] = {
        mp_ungijosik_start,
        mp_ungijosik_end
    };
    for (std::uint8_t p : all) {
        AgentUngiJosikRequest r;
        r.protocol = p;
        r.user_found = false;
        EXPECT_EQ(classify_agent_ungijosik(r), AgentUngiJosikOutcome::DropNoUser)
            << "protocol=" << +p;
    }

    if (sizeof(all) / sizeof(all[0]) >= 2) {
        const std::uint8_t middle = all[sizeof(all) / sizeof(all[0]) / 2];
        AgentUngiJosikRequest m;
        m.protocol = middle;
        m.user_found = true;
        EXPECT_EQ(classify_agent_ungijosik(m), AgentUngiJosikOutcome::ForwardToUser);
    }
}

TEST(AgentUngiJosikClassify, ObjectIdIgnoredInClassification) {
    AgentUngiJosikRequest a;
    a.user_found = true;
    a.object_id = 0xFFFFFFFFu;
    AgentUngiJosikRequest b = a;
    b.object_id = 0u;
    EXPECT_EQ(classify_agent_ungijosik(a), classify_agent_ungijosik(b));
}

TEST(AgentUngiJosikClassify, OutcomeIsDeterministic) {
    AgentUngiJosikRequest r;
    r.user_found = true;
    EXPECT_EQ(classify_agent_ungijosik(r), AgentUngiJosikOutcome::ForwardToUser);
    EXPECT_EQ(classify_agent_ungijosik(r), AgentUngiJosikOutcome::ForwardToUser);
}

TEST(AgentUngiJosikClassify, UnknownProtocolStillForwardsWhenUserFound) {
    // Legacy does not validate the protocol byte for this category at the agent;
    // any protocol gets forwarded if user is found. Preserved verbatim.
    AgentUngiJosikRequest r;
    r.protocol = 200u;
    r.user_found = true;
    EXPECT_EQ(classify_agent_ungijosik(r), AgentUngiJosikOutcome::ForwardToUser);
}
