// D4 AgentMonster data plane tests.
//
// 1:1 port of the implicit default-branch behavior of
// [Server]Agent/AgentNetworkMsgParser.cpp for category MP_MONSTER.

#include <mxh/server/agent_monster.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(AgentMonsterClassify, CategoryConstantMatchesProtocolHeader) {
    EXPECT_EQ(monster_category, 35u);
}

TEST(AgentMonsterClassify, SubProtocolConstantsAreUnique) {
    const std::uint8_t all[] = {
        monster_life_notify,
        monster_reststart_notify,
        monster_restend_notify,
        monster_recall_notify
    };
    ASSERT_EQ(sizeof(all) / sizeof(all[0]), 4u);
    for (std::size_t i = 0; i < sizeof(all) / sizeof(all[0]); ++i) {
        for (std::size_t j = i + 1; j < sizeof(all) / sizeof(all[0]); ++j) {
            EXPECT_NE(all[i], all[j])
                << "duplicate protocol at i=" << i
                << " j=" << j;
        }
    }
}

TEST(AgentMonsterClassify, SubProtocolsAreContiguousFromZero) {
    EXPECT_EQ(monster_life_notify, 0u);
    EXPECT_EQ(monster_recall_notify, 3u);
}

TEST(AgentMonsterClassify, UserFoundForwards) {
    AgentMonsterRequest r;
    r.protocol = monster_life_notify;
    r.user_found = true;
    r.object_id = 0xDEADBEEFu;
    EXPECT_EQ(classify_agent_monster(r), AgentMonsterOutcome::ForwardToUser);
}

TEST(AgentMonsterClassify, UserNotFoundDrops) {
    AgentMonsterRequest r;
    r.protocol = monster_life_notify;
    r.user_found = false;
    EXPECT_EQ(classify_agent_monster(r), AgentMonsterOutcome::DropNoUser);
}

TEST(AgentMonsterClassify, EverySubProtocolForwardsWhenUserFound) {
    const std::uint8_t all[] = {
        monster_life_notify,
        monster_reststart_notify,
        monster_restend_notify,
        monster_recall_notify
    };
    for (std::uint8_t p : all) {
        AgentMonsterRequest r;
        r.protocol = p;
        r.user_found = true;
        EXPECT_EQ(classify_agent_monster(r), AgentMonsterOutcome::ForwardToUser)
            << "protocol=" << +p;
    }
}

TEST(AgentMonsterClassify, EverySubProtocolDropsWhenUserMissing) {
    const std::uint8_t all[] = {
        monster_life_notify,
        monster_reststart_notify,
        monster_restend_notify,
        monster_recall_notify
    };
    for (std::uint8_t p : all) {
        AgentMonsterRequest r;
        r.protocol = p;
        r.user_found = false;
        EXPECT_EQ(classify_agent_monster(r), AgentMonsterOutcome::DropNoUser)
            << "protocol=" << +p;
    }
}

TEST(AgentMonsterClassify, ObjectIdIgnoredInClassification) {
    AgentMonsterRequest a;
    a.user_found = true;
    a.object_id = 0xFFFFFFFFu;
    AgentMonsterRequest b = a;
    b.object_id = 0u;
    EXPECT_EQ(classify_agent_monster(a), classify_agent_monster(b));
}

TEST(AgentMonsterClassify, OutcomeIsDeterministic) {
    AgentMonsterRequest r;
    r.user_found = true;
    EXPECT_EQ(classify_agent_monster(r), AgentMonsterOutcome::ForwardToUser);
    EXPECT_EQ(classify_agent_monster(r), AgentMonsterOutcome::ForwardToUser);
}

TEST(AgentMonsterClassify, UnknownProtocolStillForwardsWhenUserFound) {
    // Legacy does not validate the protocol byte for this category at the agent;
    // any protocol gets forwarded if user is found. Preserved verbatim.
    AgentMonsterRequest r;
    r.protocol = 200u;
    r.user_found = true;
    EXPECT_EQ(classify_agent_monster(r), AgentMonsterOutcome::ForwardToUser);
}
