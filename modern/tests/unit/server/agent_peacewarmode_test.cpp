// D4 AgentPeaceWarMode data plane tests.
//
// 1:1 port of the implicit default-branch behavior of
// [Server]Agent/AgentNetworkMsgParser.cpp for category MP_MP_PEACEWARMODE.

#include <mxh/server/agent_peacewarmode.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(AgentPeaceWarModeClassify, CategoryConstantMatchesProtocolHeader) {
    EXPECT_EQ(mp_peacewarmode_category, 15u);
}

TEST(AgentPeaceWarModeClassify, SubProtocolConstantsAreUnique) {
    const std::uint8_t all[] = {
        mp_peacewarmode_peace,
        mp_peacewarmode_war
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

TEST(AgentPeaceWarModeClassify, SubProtocolsAreContiguousFromZero) {
    EXPECT_EQ(mp_peacewarmode_peace, 0u);
    EXPECT_EQ(mp_peacewarmode_war, 1u);
}

TEST(AgentPeaceWarModeClassify, UserFoundForwards) {
    AgentPeaceWarModeRequest r;
    r.protocol = mp_peacewarmode_peace;
    r.user_found = true;
    r.object_id = 0xDEADBEEFu;
    EXPECT_EQ(classify_agent_peacewarmode(r), AgentPeaceWarModeOutcome::ForwardToUser);
}

TEST(AgentPeaceWarModeClassify, UserNotFoundDrops) {
    AgentPeaceWarModeRequest r;
    r.protocol = mp_peacewarmode_peace;
    r.user_found = false;
    EXPECT_EQ(classify_agent_peacewarmode(r), AgentPeaceWarModeOutcome::DropNoUser);
}

TEST(AgentPeaceWarModeClassify, EverySubProtocolForwardsWhenUserFound) {
    const std::uint8_t all[] = {
        mp_peacewarmode_peace,
        mp_peacewarmode_war
    };
    for (std::uint8_t p : all) {
        AgentPeaceWarModeRequest r;
        r.protocol = p;
        r.user_found = true;
        EXPECT_EQ(classify_agent_peacewarmode(r), AgentPeaceWarModeOutcome::ForwardToUser)
            << "protocol=" << +p;
    }
}

TEST(AgentPeaceWarModeClassify, EverySubProtocolDropsWhenUserMissing) {
    const std::uint8_t all[] = {
        mp_peacewarmode_peace,
        mp_peacewarmode_war
    };
    for (std::uint8_t p : all) {
        AgentPeaceWarModeRequest r;
        r.protocol = p;
        r.user_found = false;
        EXPECT_EQ(classify_agent_peacewarmode(r), AgentPeaceWarModeOutcome::DropNoUser)
            << "protocol=" << +p;
    }

    if (sizeof(all) / sizeof(all[0]) >= 2) {
        const std::uint8_t middle = all[sizeof(all) / sizeof(all[0]) / 2];
        AgentPeaceWarModeRequest m;
        m.protocol = middle;
        m.user_found = true;
        EXPECT_EQ(classify_agent_peacewarmode(m), AgentPeaceWarModeOutcome::ForwardToUser);
    }
}

TEST(AgentPeaceWarModeClassify, ObjectIdIgnoredInClassification) {
    AgentPeaceWarModeRequest a;
    a.user_found = true;
    a.object_id = 0xFFFFFFFFu;
    AgentPeaceWarModeRequest b = a;
    b.object_id = 0u;
    EXPECT_EQ(classify_agent_peacewarmode(a), classify_agent_peacewarmode(b));
}

TEST(AgentPeaceWarModeClassify, OutcomeIsDeterministic) {
    AgentPeaceWarModeRequest r;
    r.user_found = true;
    EXPECT_EQ(classify_agent_peacewarmode(r), AgentPeaceWarModeOutcome::ForwardToUser);
    EXPECT_EQ(classify_agent_peacewarmode(r), AgentPeaceWarModeOutcome::ForwardToUser);
}

TEST(AgentPeaceWarModeClassify, UnknownProtocolStillForwardsWhenUserFound) {
    // Legacy does not validate the protocol byte for this category at the agent;
    // any protocol gets forwarded if user is found. Preserved verbatim.
    AgentPeaceWarModeRequest r;
    r.protocol = 200u;
    r.user_found = true;
    EXPECT_EQ(classify_agent_peacewarmode(r), AgentPeaceWarModeOutcome::ForwardToUser);
}
