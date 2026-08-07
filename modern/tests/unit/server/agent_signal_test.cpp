// D4 AgentSignal data plane tests.
//
// 1:1 port of the implicit default-branch behavior of
// [Server]Agent/AgentNetworkMsgParser.cpp for category MP_MP_SIGNAL.

#include <mxh/server/agent_signal.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(AgentSignalClassify, CategoryConstantMatchesProtocolHeader) {
    EXPECT_EQ(mp_signal_category, 19u);
}

TEST(AgentSignalClassify, SubProtocolConstantsAreUnique) {
    const std::uint8_t all[] = {
        mp_signal_commonuser,
        mp_signal_oneuser,
        mp_signal_system,
        mp_signal_battle,
        mp_signal_vimu_result
    };
    ASSERT_EQ(sizeof(all) / sizeof(all[0]), 5u);
    for (std::size_t i = 0; i < sizeof(all) / sizeof(all[0]); ++i) {
        for (std::size_t j = i + 1; j < sizeof(all) / sizeof(all[0]); ++j) {
            EXPECT_NE(all[i], all[j])
                << "duplicate protocol at i=" << i
                << " j=" << j;
        }
    }
}

TEST(AgentSignalClassify, SubProtocolsAreContiguousFromZero) {
    EXPECT_EQ(mp_signal_commonuser, 0u);
    EXPECT_EQ(mp_signal_vimu_result, 4u);
}

TEST(AgentSignalClassify, UserFoundForwards) {
    AgentSignalRequest r;
    r.protocol = mp_signal_commonuser;
    r.user_found = true;
    r.object_id = 0xDEADBEEFu;
    EXPECT_EQ(classify_agent_signal(r), AgentSignalOutcome::ForwardToUser);
}

TEST(AgentSignalClassify, UserNotFoundDrops) {
    AgentSignalRequest r;
    r.protocol = mp_signal_commonuser;
    r.user_found = false;
    EXPECT_EQ(classify_agent_signal(r), AgentSignalOutcome::DropNoUser);
}

TEST(AgentSignalClassify, EverySubProtocolForwardsWhenUserFound) {
    const std::uint8_t all[] = {
        mp_signal_commonuser,
        mp_signal_oneuser,
        mp_signal_system,
        mp_signal_battle,
        mp_signal_vimu_result
    };
    for (std::uint8_t p : all) {
        AgentSignalRequest r;
        r.protocol = p;
        r.user_found = true;
        EXPECT_EQ(classify_agent_signal(r), AgentSignalOutcome::ForwardToUser)
            << "protocol=" << +p;
    }
}

TEST(AgentSignalClassify, EverySubProtocolDropsWhenUserMissing) {
    const std::uint8_t all[] = {
        mp_signal_commonuser,
        mp_signal_oneuser,
        mp_signal_system,
        mp_signal_battle,
        mp_signal_vimu_result
    };
    for (std::uint8_t p : all) {
        AgentSignalRequest r;
        r.protocol = p;
        r.user_found = false;
        EXPECT_EQ(classify_agent_signal(r), AgentSignalOutcome::DropNoUser)
            << "protocol=" << +p;
    }

    if (sizeof(all) / sizeof(all[0]) >= 2) {
        const std::uint8_t middle = all[sizeof(all) / sizeof(all[0]) / 2];
        AgentSignalRequest m;
        m.protocol = middle;
        m.user_found = true;
        EXPECT_EQ(classify_agent_signal(m), AgentSignalOutcome::ForwardToUser);
    }
}

TEST(AgentSignalClassify, ObjectIdIgnoredInClassification) {
    AgentSignalRequest a;
    a.user_found = true;
    a.object_id = 0xFFFFFFFFu;
    AgentSignalRequest b = a;
    b.object_id = 0u;
    EXPECT_EQ(classify_agent_signal(a), classify_agent_signal(b));
}

TEST(AgentSignalClassify, OutcomeIsDeterministic) {
    AgentSignalRequest r;
    r.user_found = true;
    EXPECT_EQ(classify_agent_signal(r), AgentSignalOutcome::ForwardToUser);
    EXPECT_EQ(classify_agent_signal(r), AgentSignalOutcome::ForwardToUser);
}

TEST(AgentSignalClassify, UnknownProtocolStillForwardsWhenUserFound) {
    // Legacy does not validate the protocol byte for this category at the agent;
    // any protocol gets forwarded if user is found. Preserved verbatim.
    AgentSignalRequest r;
    r.protocol = 200u;
    r.user_found = true;
    EXPECT_EQ(classify_agent_signal(r), AgentSignalOutcome::ForwardToUser);
}
