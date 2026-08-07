// D4 AgentTactic data plane tests.
//
// 1:1 port of the implicit default-branch behavior of
// [Server]Agent/AgentNetworkMsgParser.cpp for category MP_MP_TACTIC.

#include <mxh/server/agent_tactic.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(AgentTacticClassify, CategoryConstantMatchesProtocolHeader) {
    EXPECT_EQ(mp_tactic_category, 20u);
}

TEST(AgentTacticClassify, SubProtocolConstantsAreUnique) {
    const std::uint8_t all[] = {
        mp_tactic_start_syn,
        mp_tactic_start_ack,
        mp_tactic_start_nack,
        mp_tactic_join_syn,
        mp_tactic_join_ack,
        mp_tactic_join_nack,
        mp_tactic_object_add,
        mp_tactic_fail,
        mp_tactic_execute
    };
    ASSERT_EQ(sizeof(all) / sizeof(all[0]), 9u);
    for (std::size_t i = 0; i < sizeof(all) / sizeof(all[0]); ++i) {
        for (std::size_t j = i + 1; j < sizeof(all) / sizeof(all[0]); ++j) {
            EXPECT_NE(all[i], all[j])
                << "duplicate protocol at i=" << i
                << " j=" << j;
        }
    }
}

TEST(AgentTacticClassify, SubProtocolsAreContiguousFromZero) {
    EXPECT_EQ(mp_tactic_start_syn, 0u);
    EXPECT_EQ(mp_tactic_execute, 8u);
}

TEST(AgentTacticClassify, UserFoundForwards) {
    AgentTacticRequest r;
    r.protocol = mp_tactic_start_syn;
    r.user_found = true;
    r.object_id = 0xDEADBEEFu;
    EXPECT_EQ(classify_agent_tactic(r), AgentTacticOutcome::ForwardToUser);
}

TEST(AgentTacticClassify, UserNotFoundDrops) {
    AgentTacticRequest r;
    r.protocol = mp_tactic_start_syn;
    r.user_found = false;
    EXPECT_EQ(classify_agent_tactic(r), AgentTacticOutcome::DropNoUser);
}

TEST(AgentTacticClassify, EverySubProtocolForwardsWhenUserFound) {
    const std::uint8_t all[] = {
        mp_tactic_start_syn,
        mp_tactic_start_ack,
        mp_tactic_start_nack,
        mp_tactic_join_syn,
        mp_tactic_join_ack,
        mp_tactic_join_nack,
        mp_tactic_object_add,
        mp_tactic_fail,
        mp_tactic_execute
    };
    for (std::uint8_t p : all) {
        AgentTacticRequest r;
        r.protocol = p;
        r.user_found = true;
        EXPECT_EQ(classify_agent_tactic(r), AgentTacticOutcome::ForwardToUser)
            << "protocol=" << +p;
    }
}

TEST(AgentTacticClassify, EverySubProtocolDropsWhenUserMissing) {
    const std::uint8_t all[] = {
        mp_tactic_start_syn,
        mp_tactic_start_ack,
        mp_tactic_start_nack,
        mp_tactic_join_syn,
        mp_tactic_join_ack,
        mp_tactic_join_nack,
        mp_tactic_object_add,
        mp_tactic_fail,
        mp_tactic_execute
    };
    for (std::uint8_t p : all) {
        AgentTacticRequest r;
        r.protocol = p;
        r.user_found = false;
        EXPECT_EQ(classify_agent_tactic(r), AgentTacticOutcome::DropNoUser)
            << "protocol=" << +p;
    }

    if (sizeof(all) / sizeof(all[0]) >= 2) {
        const std::uint8_t middle = all[sizeof(all) / sizeof(all[0]) / 2];
        AgentTacticRequest m;
        m.protocol = middle;
        m.user_found = true;
        EXPECT_EQ(classify_agent_tactic(m), AgentTacticOutcome::ForwardToUser);
    }
}

TEST(AgentTacticClassify, ObjectIdIgnoredInClassification) {
    AgentTacticRequest a;
    a.user_found = true;
    a.object_id = 0xFFFFFFFFu;
    AgentTacticRequest b = a;
    b.object_id = 0u;
    EXPECT_EQ(classify_agent_tactic(a), classify_agent_tactic(b));
}

TEST(AgentTacticClassify, OutcomeIsDeterministic) {
    AgentTacticRequest r;
    r.user_found = true;
    EXPECT_EQ(classify_agent_tactic(r), AgentTacticOutcome::ForwardToUser);
    EXPECT_EQ(classify_agent_tactic(r), AgentTacticOutcome::ForwardToUser);
}

TEST(AgentTacticClassify, UnknownProtocolStillForwardsWhenUserFound) {
    // Legacy does not validate the protocol byte for this category at the agent;
    // any protocol gets forwarded if user is found. Preserved verbatim.
    AgentTacticRequest r;
    r.protocol = 200u;
    r.user_found = true;
    EXPECT_EQ(classify_agent_tactic(r), AgentTacticOutcome::ForwardToUser);
}
