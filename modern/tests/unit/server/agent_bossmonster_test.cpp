// D4 AgentBossMonster data plane tests.
//
// 1:1 port of the implicit default-branch behavior of
// [Server]Agent/AgentNetworkMsgParser.cpp for category MP_BOSSMONSTER.

#include <mxh/server/agent_bossmonster.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(AgentBossMonsterClassify, CategoryConstantMatchesProtocolHeader) {
    EXPECT_EQ(bossmonster_category, 34u);
}

TEST(AgentBossMonsterClassify, SubProtocolConstantsAreUnique) {
    const std::uint8_t all[] = {
        boss_rest_start_notify,
        boss_recall_notify,
        boss_life_notify,
        boss_shield_notify,
        boss_stand_notify,
        boss_stand_end_notify,
        field_life_notify,
        field_shield_notify
    };
    ASSERT_EQ(sizeof(all) / sizeof(all[0]), 8u);
    for (std::size_t i = 0; i < sizeof(all) / sizeof(all[0]); ++i) {
        for (std::size_t j = i + 1; j < sizeof(all) / sizeof(all[0]); ++j) {
            EXPECT_NE(all[i], all[j])
                << "duplicate protocol at i=" << i
                << " j=" << j;
        }
    }
}

TEST(AgentBossMonsterClassify, SubProtocolsAreContiguousFromZero) {
    EXPECT_EQ(boss_rest_start_notify, 0u);
    EXPECT_EQ(field_shield_notify, 7u);
}

TEST(AgentBossMonsterClassify, UserFoundForwards) {
    AgentBossMonsterRequest r;
    r.protocol = boss_rest_start_notify;
    r.user_found = true;
    r.object_id = 0xDEADBEEFu;
    EXPECT_EQ(classify_agent_bossmonster(r), AgentBossMonsterOutcome::ForwardToUser);
}

TEST(AgentBossMonsterClassify, UserNotFoundDrops) {
    AgentBossMonsterRequest r;
    r.protocol = boss_rest_start_notify;
    r.user_found = false;
    EXPECT_EQ(classify_agent_bossmonster(r), AgentBossMonsterOutcome::DropNoUser);
}

TEST(AgentBossMonsterClassify, EverySubProtocolForwardsWhenUserFound) {
    const std::uint8_t all[] = {
        boss_rest_start_notify,
        boss_recall_notify,
        boss_life_notify,
        boss_shield_notify,
        boss_stand_notify,
        boss_stand_end_notify,
        field_life_notify,
        field_shield_notify
    };
    for (std::uint8_t p : all) {
        AgentBossMonsterRequest r;
        r.protocol = p;
        r.user_found = true;
        EXPECT_EQ(classify_agent_bossmonster(r), AgentBossMonsterOutcome::ForwardToUser)
            << "protocol=" << +p;
    }
}

TEST(AgentBossMonsterClassify, EverySubProtocolDropsWhenUserMissing) {
    const std::uint8_t all[] = {
        boss_rest_start_notify,
        boss_recall_notify,
        boss_life_notify,
        boss_shield_notify,
        boss_stand_notify,
        boss_stand_end_notify,
        field_life_notify,
        field_shield_notify
    };
    for (std::uint8_t p : all) {
        AgentBossMonsterRequest r;
        r.protocol = p;
        r.user_found = false;
        EXPECT_EQ(classify_agent_bossmonster(r), AgentBossMonsterOutcome::DropNoUser)
            << "protocol=" << +p;
    }
}

TEST(AgentBossMonsterClassify, ObjectIdIgnoredInClassification) {
    AgentBossMonsterRequest a;
    a.user_found = true;
    a.object_id = 0xFFFFFFFFu;
    AgentBossMonsterRequest b = a;
    b.object_id = 0u;
    EXPECT_EQ(classify_agent_bossmonster(a), classify_agent_bossmonster(b));
}

TEST(AgentBossMonsterClassify, OutcomeIsDeterministic) {
    AgentBossMonsterRequest r;
    r.user_found = true;
    EXPECT_EQ(classify_agent_bossmonster(r), AgentBossMonsterOutcome::ForwardToUser);
    EXPECT_EQ(classify_agent_bossmonster(r), AgentBossMonsterOutcome::ForwardToUser);
}

TEST(AgentBossMonsterClassify, UnknownProtocolStillForwardsWhenUserFound) {
    // Legacy does not validate the protocol byte for this category at the agent;
    // any protocol gets forwarded if user is found. Preserved verbatim.
    AgentBossMonsterRequest r;
    r.protocol = 200u;
    r.user_found = true;
    EXPECT_EQ(classify_agent_bossmonster(r), AgentBossMonsterOutcome::ForwardToUser);
}
