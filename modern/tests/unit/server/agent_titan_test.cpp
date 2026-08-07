// D4 AgentTitan data plane tests.
//
// 1:1 port of the implicit default-branch behavior of
// [Server]Agent/AgentNetworkMsgParser.cpp for category MP_TITAN.

#include <mxh/server/agent_titan.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(AgentTitanClassify, CategoryConstantMatchesProtocolHeader) {
    EXPECT_EQ(titan_category, 72u);
}

TEST(AgentTitanClassify, SubProtocolConstantsAreUnique) {
    const std::uint8_t all[] = {
        titan_valueinfo,
        titan_fuel_ack,
        titan_spell_ack,
        titan_recall_syn,
        titan_recall_ack,
        titan_recall_nack,
        titan_recall_cancel_syn,
        titan_recall_cancel_ack,
        titan_recall_cancel_nack,
        titan_ridein_syn,
        titan_ridein_ack,
        titan_getoff_ack,
        titan_make_syn,
        titan_make_ack,
        titan_make_nack,
        titan_addnew_fromitem,
        titan_addnew_equip_fromitem,
        titan_statinfo,
        titan_endurance_update
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

TEST(AgentTitanClassify, SubProtocolsAreContiguousFromZero) {
    EXPECT_EQ(titan_valueinfo, 0u);
    EXPECT_EQ(titan_endurance_update, 18u);
}

TEST(AgentTitanClassify, UserFoundForwards) {
    AgentTitanRequest r;
    r.protocol = titan_valueinfo;
    r.user_found = true;
    r.object_id = 0xDEADBEEFu;
    EXPECT_EQ(classify_agent_titan(r), AgentTitanOutcome::ForwardToUser);
}

TEST(AgentTitanClassify, UserNotFoundDrops) {
    AgentTitanRequest r;
    r.protocol = titan_valueinfo;
    r.user_found = false;
    EXPECT_EQ(classify_agent_titan(r), AgentTitanOutcome::DropNoUser);
}

TEST(AgentTitanClassify, EverySubProtocolForwardsWhenUserFound) {
    const std::uint8_t all[] = {
        titan_valueinfo,
        titan_fuel_ack,
        titan_spell_ack,
        titan_recall_syn,
        titan_recall_ack,
        titan_recall_nack,
        titan_recall_cancel_syn,
        titan_recall_cancel_ack,
        titan_recall_cancel_nack,
        titan_ridein_syn,
        titan_ridein_ack,
        titan_getoff_ack,
        titan_make_syn,
        titan_make_ack,
        titan_make_nack,
        titan_addnew_fromitem,
        titan_addnew_equip_fromitem,
        titan_statinfo,
        titan_endurance_update
    };
    for (std::uint8_t p : all) {
        AgentTitanRequest r;
        r.protocol = p;
        r.user_found = true;
        EXPECT_EQ(classify_agent_titan(r), AgentTitanOutcome::ForwardToUser)
            << "protocol=" << +p;
    }
}

TEST(AgentTitanClassify, EverySubProtocolDropsWhenUserMissing) {
    const std::uint8_t all[] = {
        titan_valueinfo,
        titan_fuel_ack,
        titan_spell_ack,
        titan_recall_syn,
        titan_recall_ack,
        titan_recall_nack,
        titan_recall_cancel_syn,
        titan_recall_cancel_ack,
        titan_recall_cancel_nack,
        titan_ridein_syn,
        titan_ridein_ack,
        titan_getoff_ack,
        titan_make_syn,
        titan_make_ack,
        titan_make_nack,
        titan_addnew_fromitem,
        titan_addnew_equip_fromitem,
        titan_statinfo,
        titan_endurance_update
    };
    for (std::uint8_t p : all) {
        AgentTitanRequest r;
        r.protocol = p;
        r.user_found = false;
        EXPECT_EQ(classify_agent_titan(r), AgentTitanOutcome::DropNoUser)
            << "protocol=" << +p;
    }
}

TEST(AgentTitanClassify, ObjectIdIgnoredInClassification) {
    AgentTitanRequest a;
    a.user_found = true;
    a.object_id = 0xFFFFFFFFu;
    AgentTitanRequest b = a;
    b.object_id = 0u;
    EXPECT_EQ(classify_agent_titan(a), classify_agent_titan(b));
}

TEST(AgentTitanClassify, OutcomeIsDeterministic) {
    AgentTitanRequest r;
    r.user_found = true;
    EXPECT_EQ(classify_agent_titan(r), AgentTitanOutcome::ForwardToUser);
    EXPECT_EQ(classify_agent_titan(r), AgentTitanOutcome::ForwardToUser);
}

TEST(AgentTitanClassify, UnknownProtocolStillForwardsWhenUserFound) {
    // Legacy does not validate the protocol byte for this category at the agent;
    // any protocol gets forwarded if user is found. Preserved verbatim.
    AgentTitanRequest r;
    r.protocol = 200u;
    r.user_found = true;
    EXPECT_EQ(classify_agent_titan(r), AgentTitanOutcome::ForwardToUser);
}
