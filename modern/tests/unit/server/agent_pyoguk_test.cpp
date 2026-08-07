// D4 AgentPyoguk data plane tests.
//
// 1:1 port of the implicit default-branch behavior of
// [Server]Agent/AgentNetworkMsgParser.cpp for category MP_PYOGUK.

#include <mxh/server/agent_pyoguk.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(AgentPyogukClassify, CategoryConstantMatchesProtocolHeader) {
    EXPECT_EQ(pyoguk_category, 30u);
}

TEST(AgentPyogukClassify, SubProtocolConstantsAreUnique) {
    const std::uint8_t all[] = {
        pyoguk_listinfo_syn,
        pyoguk_listinfo_ack,
        pyoguk_listinfo_nack,
        pyoguk_buy_syn,
        pyoguk_buy_ack,
        pyoguk_buy_nack,
        pyoguk_del_syn,
        pyoguk_del_ack,
        pyoguk_del_nack,
        pyoguk_putin_money_syn,
        pyoguk_putin_money_ack,
        pyoguk_putin_money_nack,
        pyoguk_putout_money_syn,
        pyoguk_putout_money_ack,
        pyoguk_putout_money_nack,
        pyoguk_info
    };
    ASSERT_EQ(sizeof(all) / sizeof(all[0]), 16u);
    for (std::size_t i = 0; i < sizeof(all) / sizeof(all[0]); ++i) {
        for (std::size_t j = i + 1; j < sizeof(all) / sizeof(all[0]); ++j) {
            EXPECT_NE(all[i], all[j])
                << "duplicate protocol at i=" << i
                << " j=" << j;
        }
    }
}

TEST(AgentPyogukClassify, SubProtocolsAreContiguousFromZero) {
    EXPECT_EQ(pyoguk_listinfo_syn, 0u);
    EXPECT_EQ(pyoguk_info, 15u);
}

TEST(AgentPyogukClassify, UserFoundForwards) {
    AgentPyogukRequest r;
    r.protocol = pyoguk_listinfo_syn;
    r.user_found = true;
    r.object_id = 0xDEADBEEFu;
    EXPECT_EQ(classify_agent_pyoguk(r), AgentPyogukOutcome::ForwardToUser);
}

TEST(AgentPyogukClassify, UserNotFoundDrops) {
    AgentPyogukRequest r;
    r.protocol = pyoguk_listinfo_syn;
    r.user_found = false;
    EXPECT_EQ(classify_agent_pyoguk(r), AgentPyogukOutcome::DropNoUser);
}

TEST(AgentPyogukClassify, EverySubProtocolForwardsWhenUserFound) {
    const std::uint8_t all[] = {
        pyoguk_listinfo_syn,
        pyoguk_listinfo_ack,
        pyoguk_listinfo_nack,
        pyoguk_buy_syn,
        pyoguk_buy_ack,
        pyoguk_buy_nack,
        pyoguk_del_syn,
        pyoguk_del_ack,
        pyoguk_del_nack,
        pyoguk_putin_money_syn,
        pyoguk_putin_money_ack,
        pyoguk_putin_money_nack,
        pyoguk_putout_money_syn,
        pyoguk_putout_money_ack,
        pyoguk_putout_money_nack,
        pyoguk_info
    };
    for (std::uint8_t p : all) {
        AgentPyogukRequest r;
        r.protocol = p;
        r.user_found = true;
        EXPECT_EQ(classify_agent_pyoguk(r), AgentPyogukOutcome::ForwardToUser)
            << "protocol=" << +p;
    }
}

TEST(AgentPyogukClassify, EverySubProtocolDropsWhenUserMissing) {
    const std::uint8_t all[] = {
        pyoguk_listinfo_syn,
        pyoguk_listinfo_ack,
        pyoguk_listinfo_nack,
        pyoguk_buy_syn,
        pyoguk_buy_ack,
        pyoguk_buy_nack,
        pyoguk_del_syn,
        pyoguk_del_ack,
        pyoguk_del_nack,
        pyoguk_putin_money_syn,
        pyoguk_putin_money_ack,
        pyoguk_putin_money_nack,
        pyoguk_putout_money_syn,
        pyoguk_putout_money_ack,
        pyoguk_putout_money_nack,
        pyoguk_info
    };
    for (std::uint8_t p : all) {
        AgentPyogukRequest r;
        r.protocol = p;
        r.user_found = false;
        EXPECT_EQ(classify_agent_pyoguk(r), AgentPyogukOutcome::DropNoUser)
            << "protocol=" << +p;
    }
}

TEST(AgentPyogukClassify, ObjectIdIgnoredInClassification) {
    AgentPyogukRequest a;
    a.user_found = true;
    a.object_id = 0xFFFFFFFFu;
    AgentPyogukRequest b = a;
    b.object_id = 0u;
    EXPECT_EQ(classify_agent_pyoguk(a), classify_agent_pyoguk(b));
}

TEST(AgentPyogukClassify, OutcomeIsDeterministic) {
    AgentPyogukRequest r;
    r.user_found = true;
    EXPECT_EQ(classify_agent_pyoguk(r), AgentPyogukOutcome::ForwardToUser);
    EXPECT_EQ(classify_agent_pyoguk(r), AgentPyogukOutcome::ForwardToUser);
}

TEST(AgentPyogukClassify, UnknownProtocolStillForwardsWhenUserFound) {
    // Legacy does not validate the protocol byte for this category at the agent;
    // any protocol gets forwarded if user is found. Preserved verbatim.
    AgentPyogukRequest r;
    r.protocol = 200u;
    r.user_found = true;
    EXPECT_EQ(classify_agent_pyoguk(r), AgentPyogukOutcome::ForwardToUser);
}
