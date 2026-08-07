// D4.173 AgentNpc data plane tests.
//
// 1:1 port of the implicit default-branch behavior of
// [Server]Agent/AgentNetworkMsgParser.cpp for category MP_NPC.

#include <mxh/server/agent_npc.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(AgentNpcClassify, CategoryConstantMatchesProtocolHeader) {
    EXPECT_EQ(npc_category, 37u);
}

TEST(AgentNpcClassify, SubProtocolConstantsAreUnique) {
    const std::uint8_t all[] = {
        npc_speech_syn, npc_speech_ack, npc_speech_nack,
        npc_closebomul_syn, npc_closebomul_ack, npc_closebomul_nack,
        npc_openbomul_syn, npc_openbomul_ack, npc_openbomul_nack,
        npc_dojob_syn, npc_dojob_ack, npc_dojob_nack,
        npc_die_ack,
    };
    ASSERT_EQ(sizeof(all) / sizeof(all[0]), 13u);
    for (std::size_t i = 0; i < sizeof(all) / sizeof(all[0]); ++i) {
        for (std::size_t j = i + 1; j < sizeof(all) / sizeof(all[0]); ++j) {
            EXPECT_NE(all[i], all[j])
                << "duplicate protocol at i=" << i
                << " j=" << j;
        }
    }
}

TEST(AgentNpcClassify, SubProtocolsAreContiguousFromZero) {
    EXPECT_EQ(npc_speech_syn, 0u);
    EXPECT_EQ(npc_speech_ack, 1u);
    EXPECT_EQ(npc_speech_nack, 2u);
    EXPECT_EQ(npc_die_ack, 12u);
}

TEST(AgentNpcClassify, UserFoundForwards) {
    AgentNpcRequest r;
    r.protocol = npc_speech_syn;
    r.user_found = true;
    r.object_id = 0xDEADBEEFu;
    EXPECT_EQ(classify_agent_npc(r), AgentNpcOutcome::ForwardToUser);
}

TEST(AgentNpcClassify, UserNotFoundDrops) {
    AgentNpcRequest r;
    r.protocol = npc_speech_syn;
    r.user_found = false;
    EXPECT_EQ(classify_agent_npc(r), AgentNpcOutcome::DropNoUser);
}

TEST(AgentNpcClassify, EverySubProtocolForwardsWhenUserFound) {
    const std::uint8_t all[] = {
        npc_speech_syn, npc_speech_ack, npc_speech_nack,
        npc_closebomul_syn, npc_closebomul_ack, npc_closebomul_nack,
        npc_openbomul_syn, npc_openbomul_ack, npc_openbomul_nack,
        npc_dojob_syn, npc_dojob_ack, npc_dojob_nack,
        npc_die_ack,
    };
    for (std::uint8_t p : all) {
        AgentNpcRequest r;
        r.protocol = p;
        r.user_found = true;
        EXPECT_EQ(classify_agent_npc(r), AgentNpcOutcome::ForwardToUser)
            << "protocol=" << +p;
    }
}

TEST(AgentNpcClassify, EverySubProtocolDropsWhenUserMissing) {
    const std::uint8_t all[] = {
        npc_speech_syn, npc_speech_ack, npc_speech_nack,
        npc_closebomul_syn, npc_closebomul_ack, npc_closebomul_nack,
        npc_openbomul_syn, npc_openbomul_ack, npc_openbomul_nack,
        npc_dojob_syn, npc_dojob_ack, npc_dojob_nack,
        npc_die_ack,
    };
    for (std::uint8_t p : all) {
        AgentNpcRequest r;
        r.protocol = p;
        r.user_found = false;
        EXPECT_EQ(classify_agent_npc(r), AgentNpcOutcome::DropNoUser)
            << "protocol=" << +p;
    }
}

TEST(AgentNpcClassify, ObjectIdIgnoredInClassification) {
    AgentNpcRequest a;
    a.user_found = true;
    a.object_id = 0xFFFFFFFFu;
    AgentNpcRequest b = a;
    b.object_id = 0u;
    EXPECT_EQ(classify_agent_npc(a), classify_agent_npc(b));
}

TEST(AgentNpcClassify, OutcomeIsDeterministic) {
    AgentNpcRequest r;
    r.user_found = true;
    EXPECT_EQ(classify_agent_npc(r), AgentNpcOutcome::ForwardToUser);
    EXPECT_EQ(classify_agent_npc(r), AgentNpcOutcome::ForwardToUser);
}

TEST(AgentNpcClassify, UnknownProtocolStillForwardsWhenUserFound) {
    // Legacy does not validate the protocol byte for MP_NPC at the agent;
    // any protocol gets forwarded if user is found. Preserved verbatim.
    AgentNpcRequest r;
    r.protocol = 200u;
    r.user_found = true;
    EXPECT_EQ(classify_agent_npc(r), AgentNpcOutcome::ForwardToUser);
}

