// D4 AgentCharRevive data plane tests.
//
// 1:1 port of the implicit default-branch behavior of
// [Server]Agent/AgentNetworkMsgParser.cpp for category MP_CHARREVIVE.

#include <mxh/server/agent_charrevive.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(AgentCharReviveClassify, CategoryConstantMatchesProtocolHeader) {
    EXPECT_EQ(charrevive_category, 32u);
}

TEST(AgentCharReviveClassify, SubProtocolConstantsAreUnique) {
    const std::uint8_t all[] = {
        charrevive_Presentspot_syn,
        charrevive_Presentspot_ack,
        charrevive_Presentspot_nack,
        charrevive_loginspot_syn,
        charrevive_loginspot_ack,
        charrevive_loginspot_nack,
        charrevive_villagespot_syn,
        charrevive_villagespot_ack,
        charrevive_villagespot_nack
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

TEST(AgentCharReviveClassify, SubProtocolsAreContiguousFromZero) {
    EXPECT_EQ(charrevive_Presentspot_syn, 0u);
    EXPECT_EQ(charrevive_villagespot_nack, 8u);
}

TEST(AgentCharReviveClassify, UserFoundForwards) {
    AgentCharReviveRequest r;
    r.protocol = charrevive_Presentspot_syn;
    r.user_found = true;
    r.object_id = 0xDEADBEEFu;
    EXPECT_EQ(classify_agent_charrevive(r), AgentCharReviveOutcome::ForwardToUser);
}

TEST(AgentCharReviveClassify, UserNotFoundDrops) {
    AgentCharReviveRequest r;
    r.protocol = charrevive_Presentspot_syn;
    r.user_found = false;
    EXPECT_EQ(classify_agent_charrevive(r), AgentCharReviveOutcome::DropNoUser);
}

TEST(AgentCharReviveClassify, EverySubProtocolForwardsWhenUserFound) {
    const std::uint8_t all[] = {
        charrevive_Presentspot_syn,
        charrevive_Presentspot_ack,
        charrevive_Presentspot_nack,
        charrevive_loginspot_syn,
        charrevive_loginspot_ack,
        charrevive_loginspot_nack,
        charrevive_villagespot_syn,
        charrevive_villagespot_ack,
        charrevive_villagespot_nack
    };
    for (std::uint8_t p : all) {
        AgentCharReviveRequest r;
        r.protocol = p;
        r.user_found = true;
        EXPECT_EQ(classify_agent_charrevive(r), AgentCharReviveOutcome::ForwardToUser)
            << "protocol=" << +p;
    }
}

TEST(AgentCharReviveClassify, EverySubProtocolDropsWhenUserMissing) {
    const std::uint8_t all[] = {
        charrevive_Presentspot_syn,
        charrevive_Presentspot_ack,
        charrevive_Presentspot_nack,
        charrevive_loginspot_syn,
        charrevive_loginspot_ack,
        charrevive_loginspot_nack,
        charrevive_villagespot_syn,
        charrevive_villagespot_ack,
        charrevive_villagespot_nack
    };
    for (std::uint8_t p : all) {
        AgentCharReviveRequest r;
        r.protocol = p;
        r.user_found = false;
        EXPECT_EQ(classify_agent_charrevive(r), AgentCharReviveOutcome::DropNoUser)
            << "protocol=" << +p;
    }
}

TEST(AgentCharReviveClassify, ObjectIdIgnoredInClassification) {
    AgentCharReviveRequest a;
    a.user_found = true;
    a.object_id = 0xFFFFFFFFu;
    AgentCharReviveRequest b = a;
    b.object_id = 0u;
    EXPECT_EQ(classify_agent_charrevive(a), classify_agent_charrevive(b));
}

TEST(AgentCharReviveClassify, OutcomeIsDeterministic) {
    AgentCharReviveRequest r;
    r.user_found = true;
    EXPECT_EQ(classify_agent_charrevive(r), AgentCharReviveOutcome::ForwardToUser);
    EXPECT_EQ(classify_agent_charrevive(r), AgentCharReviveOutcome::ForwardToUser);
}

TEST(AgentCharReviveClassify, UnknownProtocolStillForwardsWhenUserFound) {
    // Legacy does not validate the protocol byte for this category at the agent;
    // any protocol gets forwarded if user is found. Preserved verbatim.
    AgentCharReviveRequest r;
    r.protocol = 200u;
    r.user_found = true;
    EXPECT_EQ(classify_agent_charrevive(r), AgentCharReviveOutcome::ForwardToUser);
}
