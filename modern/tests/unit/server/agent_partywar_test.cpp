// D4 AgentPartyWar data plane tests.
//
// 1:1 port of the implicit default-branch behavior of
// [Server]Agent/AgentNetworkMsgParser.cpp for category MP_PARTYWAR.

#include <mxh/server/agent_partywar.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(AgentPartyWarClassify, CategoryConstantMatchesProtocolHeader) {
    EXPECT_EQ(partywar_category, 59u);
}

TEST(AgentPartyWarClassify, SubProtocolConstantsAreUnique) {
    const std::uint8_t all[] = {
        partywar_nack,
        partywar_suggest,
        partywar_suggest_wait,
        partywar_suggest_accept,
        partywar_suggest_deny,
        partywar_addmember_syn,
        partywar_addmember_ack,
        partywar_addmember_nack,
        partywar_removemember_syn,
        partywar_removemember_ack,
        partywar_removemember_nack,
        partywar_lock,
        partywar_unlock,
        partywar_start,
        partywar_cancel,
        partywar_ready,
        partywar_fight,
        partywar_result,
        partywar_end
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

TEST(AgentPartyWarClassify, SubProtocolsAreContiguousFromZero) {
    EXPECT_EQ(partywar_nack, 0u);
    EXPECT_EQ(partywar_end, 18u);
}

TEST(AgentPartyWarClassify, UserFoundForwards) {
    AgentPartyWarRequest r;
    r.protocol = partywar_nack;
    r.user_found = true;
    r.object_id = 0xDEADBEEFu;
    EXPECT_EQ(classify_agent_partywar(r), AgentPartyWarOutcome::ForwardToUser);
}

TEST(AgentPartyWarClassify, UserNotFoundDrops) {
    AgentPartyWarRequest r;
    r.protocol = partywar_nack;
    r.user_found = false;
    EXPECT_EQ(classify_agent_partywar(r), AgentPartyWarOutcome::DropNoUser);
}

TEST(AgentPartyWarClassify, EverySubProtocolForwardsWhenUserFound) {
    const std::uint8_t all[] = {
        partywar_nack,
        partywar_suggest,
        partywar_suggest_wait,
        partywar_suggest_accept,
        partywar_suggest_deny,
        partywar_addmember_syn,
        partywar_addmember_ack,
        partywar_addmember_nack,
        partywar_removemember_syn,
        partywar_removemember_ack,
        partywar_removemember_nack,
        partywar_lock,
        partywar_unlock,
        partywar_start,
        partywar_cancel,
        partywar_ready,
        partywar_fight,
        partywar_result,
        partywar_end
    };
    for (std::uint8_t p : all) {
        AgentPartyWarRequest r;
        r.protocol = p;
        r.user_found = true;
        EXPECT_EQ(classify_agent_partywar(r), AgentPartyWarOutcome::ForwardToUser)
            << "protocol=" << +p;
    }
}

TEST(AgentPartyWarClassify, EverySubProtocolDropsWhenUserMissing) {
    const std::uint8_t all[] = {
        partywar_nack,
        partywar_suggest,
        partywar_suggest_wait,
        partywar_suggest_accept,
        partywar_suggest_deny,
        partywar_addmember_syn,
        partywar_addmember_ack,
        partywar_addmember_nack,
        partywar_removemember_syn,
        partywar_removemember_ack,
        partywar_removemember_nack,
        partywar_lock,
        partywar_unlock,
        partywar_start,
        partywar_cancel,
        partywar_ready,
        partywar_fight,
        partywar_result,
        partywar_end
    };
    for (std::uint8_t p : all) {
        AgentPartyWarRequest r;
        r.protocol = p;
        r.user_found = false;
        EXPECT_EQ(classify_agent_partywar(r), AgentPartyWarOutcome::DropNoUser)
            << "protocol=" << +p;
    }
}

TEST(AgentPartyWarClassify, ObjectIdIgnoredInClassification) {
    AgentPartyWarRequest a;
    a.user_found = true;
    a.object_id = 0xFFFFFFFFu;
    AgentPartyWarRequest b = a;
    b.object_id = 0u;
    EXPECT_EQ(classify_agent_partywar(a), classify_agent_partywar(b));
}

TEST(AgentPartyWarClassify, OutcomeIsDeterministic) {
    AgentPartyWarRequest r;
    r.user_found = true;
    EXPECT_EQ(classify_agent_partywar(r), AgentPartyWarOutcome::ForwardToUser);
    EXPECT_EQ(classify_agent_partywar(r), AgentPartyWarOutcome::ForwardToUser);
}

TEST(AgentPartyWarClassify, UnknownProtocolStillForwardsWhenUserFound) {
    // Legacy does not validate the protocol byte for this category at the agent;
    // any protocol gets forwarded if user is found. Preserved verbatim.
    AgentPartyWarRequest r;
    r.protocol = 200u;
    r.user_found = true;
    EXPECT_EQ(classify_agent_partywar(r), AgentPartyWarOutcome::ForwardToUser);
}
