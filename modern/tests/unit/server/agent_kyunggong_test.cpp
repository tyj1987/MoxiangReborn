// D4 AgentKyungGong data plane tests.
//
// 1:1 port of the implicit default-branch behavior of
// [Server]Agent/AgentNetworkMsgParser.cpp for category MP_KYUNGGONG.

#include <mxh/server/agent_kyunggong.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(AgentKyungGongClassify, CategoryConstantMatchesProtocolHeader) {
    EXPECT_EQ(kyunggong_category, 23u);
}

TEST(AgentKyungGongClassify, SubProtocolConstantsAreUnique) {
    const std::uint8_t all[] = {
        kyunggong_change_notify,
        kyunggong_ability_change_notify
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

TEST(AgentKyungGongClassify, SubProtocolsAreContiguousFromZero) {
    EXPECT_EQ(kyunggong_change_notify, 0u);
    EXPECT_EQ(kyunggong_ability_change_notify, 1u);
}

TEST(AgentKyungGongClassify, UserFoundForwards) {
    AgentKyungGongRequest r;
    r.protocol = kyunggong_change_notify;
    r.user_found = true;
    r.object_id = 0xDEADBEEFu;
    EXPECT_EQ(classify_agent_kyunggong(r), AgentKyungGongOutcome::ForwardToUser);
}

TEST(AgentKyungGongClassify, UserNotFoundDrops) {
    AgentKyungGongRequest r;
    r.protocol = kyunggong_change_notify;
    r.user_found = false;
    EXPECT_EQ(classify_agent_kyunggong(r), AgentKyungGongOutcome::DropNoUser);
}

TEST(AgentKyungGongClassify, EverySubProtocolForwardsWhenUserFound) {
    const std::uint8_t all[] = {
        kyunggong_change_notify,
        kyunggong_ability_change_notify
    };
    for (std::uint8_t p : all) {
        AgentKyungGongRequest r;
        r.protocol = p;
        r.user_found = true;
        EXPECT_EQ(classify_agent_kyunggong(r), AgentKyungGongOutcome::ForwardToUser)
            << "protocol=" << +p;
    }
}

TEST(AgentKyungGongClassify, EverySubProtocolDropsWhenUserMissing) {
    const std::uint8_t all[] = {
        kyunggong_change_notify,
        kyunggong_ability_change_notify
    };
    for (std::uint8_t p : all) {
        AgentKyungGongRequest r;
        r.protocol = p;
        r.user_found = false;
        EXPECT_EQ(classify_agent_kyunggong(r), AgentKyungGongOutcome::DropNoUser)
            << "protocol=" << +p;
    }
}

TEST(AgentKyungGongClassify, ObjectIdIgnoredInClassification) {
    AgentKyungGongRequest a;
    a.user_found = true;
    a.object_id = 0xFFFFFFFFu;
    AgentKyungGongRequest b = a;
    b.object_id = 0u;
    EXPECT_EQ(classify_agent_kyunggong(a), classify_agent_kyunggong(b));
}

TEST(AgentKyungGongClassify, OutcomeIsDeterministic) {
    AgentKyungGongRequest r;
    r.user_found = true;
    EXPECT_EQ(classify_agent_kyunggong(r), AgentKyungGongOutcome::ForwardToUser);
    EXPECT_EQ(classify_agent_kyunggong(r), AgentKyungGongOutcome::ForwardToUser);
}

TEST(AgentKyungGongClassify, UnknownProtocolStillForwardsWhenUserFound) {
    // Legacy does not validate the protocol byte for this category at the agent;
    // any protocol gets forwarded if user is found. Preserved verbatim.
    AgentKyungGongRequest r;
    r.protocol = 200u;
    r.user_found = true;
    EXPECT_EQ(classify_agent_kyunggong(r), AgentKyungGongOutcome::ForwardToUser);
}
