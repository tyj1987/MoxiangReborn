// D4.174 AgentPk data plane tests.
//
// 1:1 port of the implicit default-branch behavior of
// [Server]Agent/AgentNetworkMsgParser.cpp for category MP_PK.

#include <mxh/server/agent_pk.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(AgentPkClassify, CategoryConstantMatchesProtocolHeader) {
    EXPECT_EQ(pk_category, 41u);
}

TEST(AgentPkClassify, SubProtocolConstantsAreUnique) {
    const std::uint8_t all[] = {
        pk_pkon_syn, pk_pkon_ack, pk_pkon_nack,
        pk_pkoff_syn, pk_pkoff_ack, pk_pkoff_nack,
        pk_looting_start, pk_looting_beinglooted,
        pk_looting_select_syn, pk_looting_select_ack, pk_looting_select_nack,
        pk_looting_itemlooting, pk_looting_itemlooted,
        pk_looting_moenylooting, pk_looting_moenylooted,
        pk_looting_explooting, pk_looting_explooted,
        pk_looting_nolooting, pk_looting_noinvenspace,
        pk_looting_endlooting, pk_destroy_item, pk_looting_error,
    };
    ASSERT_EQ(sizeof(all) / sizeof(all[0]), 22u);
    for (std::size_t i = 0; i < sizeof(all) / sizeof(all[0]); ++i) {
        for (std::size_t j = i + 1; j < sizeof(all) / sizeof(all[0]); ++j) {
            EXPECT_NE(all[i], all[j])
                << "duplicate protocol at i=" << i
                << " j=" << j;
        }
    }
}

TEST(AgentPkClassify, SubProtocolsAreContiguousFromZero) {
    EXPECT_EQ(pk_pkon_syn, 0u);
    EXPECT_EQ(pk_looting_error, 21u);
}

TEST(AgentPkClassify, UserFoundForwards) {
    AgentPkRequest r;
    r.protocol = pk_pkon_syn;
    r.user_found = true;
    r.object_id = 0xDEADBEEFu;
    EXPECT_EQ(classify_agent_pk(r), AgentPkOutcome::ForwardToUser);
}

TEST(AgentPkClassify, UserNotFoundDrops) {
    AgentPkRequest r;
    r.protocol = pk_looting_select_syn;
    r.user_found = false;
    EXPECT_EQ(classify_agent_pk(r), AgentPkOutcome::DropNoUser);
}

TEST(AgentPkClassify, EverySubProtocolForwardsWhenUserFound) {
    const std::uint8_t all[] = {
        pk_pkon_syn, pk_pkon_ack, pk_pkon_nack,
        pk_pkoff_syn, pk_pkoff_ack, pk_pkoff_nack,
        pk_looting_start, pk_looting_beinglooted,
        pk_looting_select_syn, pk_looting_select_ack, pk_looting_select_nack,
        pk_looting_itemlooting, pk_looting_itemlooted,
        pk_looting_moenylooting, pk_looting_moenylooted,
        pk_looting_explooting, pk_looting_explooted,
        pk_looting_nolooting, pk_looting_noinvenspace,
        pk_looting_endlooting, pk_destroy_item, pk_looting_error,
    };
    for (std::uint8_t p : all) {
        AgentPkRequest r;
        r.protocol = p;
        r.user_found = true;
        EXPECT_EQ(classify_agent_pk(r), AgentPkOutcome::ForwardToUser)
            << "protocol=" << +p;
    }
}

TEST(AgentPkClassify, EverySubProtocolDropsWhenUserMissing) {
    const std::uint8_t all[] = {
        pk_pkon_syn, pk_pkon_ack, pk_pkon_nack,
        pk_pkoff_syn, pk_pkoff_ack, pk_pkoff_nack,
        pk_looting_start, pk_looting_beinglooted,
        pk_looting_select_syn, pk_looting_select_ack, pk_looting_select_nack,
        pk_looting_itemlooting, pk_looting_itemlooted,
        pk_looting_moenylooting, pk_looting_moenylooted,
        pk_looting_explooting, pk_looting_explooted,
        pk_looting_nolooting, pk_looting_noinvenspace,
        pk_looting_endlooting, pk_destroy_item, pk_looting_error,
    };
    for (std::uint8_t p : all) {
        AgentPkRequest r;
        r.protocol = p;
        r.user_found = false;
        EXPECT_EQ(classify_agent_pk(r), AgentPkOutcome::DropNoUser)
            << "protocol=" << +p;
    }
}

TEST(AgentPkClassify, ObjectIdIgnoredInClassification) {
    AgentPkRequest a;
    a.user_found = true;
    a.object_id = 0xFFFFFFFFu;
    AgentPkRequest b = a;
    b.object_id = 0u;
    EXPECT_EQ(classify_agent_pk(a), classify_agent_pk(b));
}

TEST(AgentPkClassify, OutcomeIsDeterministic) {
    AgentPkRequest r;
    r.user_found = true;
    EXPECT_EQ(classify_agent_pk(r), AgentPkOutcome::ForwardToUser);
    EXPECT_EQ(classify_agent_pk(r), AgentPkOutcome::ForwardToUser);
}

TEST(AgentPkClassify, UnknownProtocolStillForwardsWhenUserFound) {
    // Legacy does not validate the protocol byte for MP_PK at the agent;
    // any protocol gets forwarded if user is found. Preserved verbatim.
    AgentPkRequest r;
    r.protocol = 200u;
    r.user_found = true;
    EXPECT_EQ(classify_agent_pk(r), AgentPkOutcome::ForwardToUser);
}

