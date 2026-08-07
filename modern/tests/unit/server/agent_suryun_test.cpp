// D4 AgentSuryun data plane tests.
//
// 1:1 port of the implicit default-branch behavior of
// [Server]Agent/AgentNetworkMsgParser.cpp for category MP_SURYUN.

#include <mxh/server/agent_suryun.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(AgentSuryunClassify, CategoryConstantMatchesProtocolHeader) {
    EXPECT_EQ(suryun_category, 54u);
}

TEST(AgentSuryunClassify, SubProtocolConstantsAreUnique) {
    const std::uint8_t all[] = {
        suryun_gosuryunmap_syn,
        suryun_gosuryunmap_ack,
        suryun_gosuryunmap_nack,
        suryun_enter_syn,
        suryun_enter_ack,
        suryun_enter_nack,
        suryun_start,
        suryun_returntorealworld,
        suryun_leave_syn,
        suryun_leave_ack,
        suryun_leave_nack
    };
    ASSERT_EQ(sizeof(all) / sizeof(all[0]), 11u);
    for (std::size_t i = 0; i < sizeof(all) / sizeof(all[0]); ++i) {
        for (std::size_t j = i + 1; j < sizeof(all) / sizeof(all[0]); ++j) {
            EXPECT_NE(all[i], all[j])
                << "duplicate protocol at i=" << i
                << " j=" << j;
        }
    }
}

TEST(AgentSuryunClassify, SubProtocolsAreContiguousFromZero) {
    EXPECT_EQ(suryun_gosuryunmap_syn, 0u);
    EXPECT_EQ(suryun_leave_nack, 10u);
}

TEST(AgentSuryunClassify, UserFoundForwards) {
    AgentSuryunRequest r;
    r.protocol = suryun_gosuryunmap_syn;
    r.user_found = true;
    r.object_id = 0xDEADBEEFu;
    EXPECT_EQ(classify_agent_suryun(r), AgentSuryunOutcome::ForwardToUser);
}

TEST(AgentSuryunClassify, UserNotFoundDrops) {
    AgentSuryunRequest r;
    r.protocol = suryun_gosuryunmap_syn;
    r.user_found = false;
    EXPECT_EQ(classify_agent_suryun(r), AgentSuryunOutcome::DropNoUser);
}

TEST(AgentSuryunClassify, EverySubProtocolForwardsWhenUserFound) {
    const std::uint8_t all[] = {
        suryun_gosuryunmap_syn,
        suryun_gosuryunmap_ack,
        suryun_gosuryunmap_nack,
        suryun_enter_syn,
        suryun_enter_ack,
        suryun_enter_nack,
        suryun_start,
        suryun_returntorealworld,
        suryun_leave_syn,
        suryun_leave_ack,
        suryun_leave_nack
    };
    for (std::uint8_t p : all) {
        AgentSuryunRequest r;
        r.protocol = p;
        r.user_found = true;
        EXPECT_EQ(classify_agent_suryun(r), AgentSuryunOutcome::ForwardToUser)
            << "protocol=" << +p;
    }
}

TEST(AgentSuryunClassify, EverySubProtocolDropsWhenUserMissing) {
    const std::uint8_t all[] = {
        suryun_gosuryunmap_syn,
        suryun_gosuryunmap_ack,
        suryun_gosuryunmap_nack,
        suryun_enter_syn,
        suryun_enter_ack,
        suryun_enter_nack,
        suryun_start,
        suryun_returntorealworld,
        suryun_leave_syn,
        suryun_leave_ack,
        suryun_leave_nack
    };
    for (std::uint8_t p : all) {
        AgentSuryunRequest r;
        r.protocol = p;
        r.user_found = false;
        EXPECT_EQ(classify_agent_suryun(r), AgentSuryunOutcome::DropNoUser)
            << "protocol=" << +p;
    }
}

TEST(AgentSuryunClassify, ObjectIdIgnoredInClassification) {
    AgentSuryunRequest a;
    a.user_found = true;
    a.object_id = 0xFFFFFFFFu;
    AgentSuryunRequest b = a;
    b.object_id = 0u;
    EXPECT_EQ(classify_agent_suryun(a), classify_agent_suryun(b));
}

TEST(AgentSuryunClassify, OutcomeIsDeterministic) {
    AgentSuryunRequest r;
    r.user_found = true;
    EXPECT_EQ(classify_agent_suryun(r), AgentSuryunOutcome::ForwardToUser);
    EXPECT_EQ(classify_agent_suryun(r), AgentSuryunOutcome::ForwardToUser);
}

TEST(AgentSuryunClassify, UnknownProtocolStillForwardsWhenUserFound) {
    // Legacy does not validate the protocol byte for this category at the agent;
    // any protocol gets forwarded if user is found. Preserved verbatim.
    AgentSuryunRequest r;
    r.protocol = 200u;
    r.user_found = true;
    EXPECT_EQ(classify_agent_suryun(r), AgentSuryunOutcome::ForwardToUser);
}
