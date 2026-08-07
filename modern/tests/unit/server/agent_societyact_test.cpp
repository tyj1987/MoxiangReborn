// D4 AgentSocietyAct data plane tests.
//
// 1:1 port of the implicit default-branch behavior of
// [Server]Agent/AgentNetworkMsgParser.cpp for category MP_SOCIETYACT.

#include <mxh/server/agent_societyact.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(AgentSocietyActClassify, CategoryConstantMatchesProtocolHeader) {
    EXPECT_EQ(societyact_category, 55u);
}

TEST(AgentSocietyActClassify, SubProtocolConstantsAreUnique) {
    const std::uint8_t all[] = {
        societyact_act_syn,
        societyact_act
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

TEST(AgentSocietyActClassify, SubProtocolsAreContiguousFromZero) {
    EXPECT_EQ(societyact_act_syn, 0u);
    EXPECT_EQ(societyact_act, 1u);
}

TEST(AgentSocietyActClassify, UserFoundForwards) {
    AgentSocietyActRequest r;
    r.protocol = societyact_act_syn;
    r.user_found = true;
    r.object_id = 0xDEADBEEFu;
    EXPECT_EQ(classify_agent_societyact(r), AgentSocietyActOutcome::ForwardToUser);
}

TEST(AgentSocietyActClassify, UserNotFoundDrops) {
    AgentSocietyActRequest r;
    r.protocol = societyact_act_syn;
    r.user_found = false;
    EXPECT_EQ(classify_agent_societyact(r), AgentSocietyActOutcome::DropNoUser);
}

TEST(AgentSocietyActClassify, EverySubProtocolForwardsWhenUserFound) {
    const std::uint8_t all[] = {
        societyact_act_syn,
        societyact_act
    };
    for (std::uint8_t p : all) {
        AgentSocietyActRequest r;
        r.protocol = p;
        r.user_found = true;
        EXPECT_EQ(classify_agent_societyact(r), AgentSocietyActOutcome::ForwardToUser)
            << "protocol=" << +p;
    }
}

TEST(AgentSocietyActClassify, EverySubProtocolDropsWhenUserMissing) {
    const std::uint8_t all[] = {
        societyact_act_syn,
        societyact_act
    };
    for (std::uint8_t p : all) {
        AgentSocietyActRequest r;
        r.protocol = p;
        r.user_found = false;
        EXPECT_EQ(classify_agent_societyact(r), AgentSocietyActOutcome::DropNoUser)
            << "protocol=" << +p;
    }
}

TEST(AgentSocietyActClassify, ObjectIdIgnoredInClassification) {
    AgentSocietyActRequest a;
    a.user_found = true;
    a.object_id = 0xFFFFFFFFu;
    AgentSocietyActRequest b = a;
    b.object_id = 0u;
    EXPECT_EQ(classify_agent_societyact(a), classify_agent_societyact(b));
}

TEST(AgentSocietyActClassify, OutcomeIsDeterministic) {
    AgentSocietyActRequest r;
    r.user_found = true;
    EXPECT_EQ(classify_agent_societyact(r), AgentSocietyActOutcome::ForwardToUser);
    EXPECT_EQ(classify_agent_societyact(r), AgentSocietyActOutcome::ForwardToUser);
}

TEST(AgentSocietyActClassify, UnknownProtocolStillForwardsWhenUserFound) {
    // Legacy does not validate the protocol byte for this category at the agent;
    // any protocol gets forwarded if user is found. Preserved verbatim.
    AgentSocietyActRequest r;
    r.protocol = 200u;
    r.user_found = true;
    EXPECT_EQ(classify_agent_societyact(r), AgentSocietyActOutcome::ForwardToUser);
}
