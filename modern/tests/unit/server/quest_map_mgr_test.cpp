#include <mxh/server/quest_map_mgr.hpp>
#include <gtest/gtest.h>

using namespace mxh::server;

TEST(QuestMapMgrConstants, PreserveHistoricalMapNumbers) {
    EXPECT_EQ(QUESTMAPNUM1, 73u);
    EXPECT_EQ(QUESTMAPNUM2, 37u);
    EXPECT_EQ(QUESTMAPNUM3, 95u);
}

TEST(QuestMapMgrInit, DefaultIsNotQuestMap) {
    const auto state = make_quest_map_mgr();
    EXPECT_FALSE(is_quest_map(state));
    EXPECT_FALSE(state.m_bQuestChannelInitialized);
}

TEST(QuestMapMgrInit, QuestRoomInitializesQuestChannel) {
    auto state = make_quest_map_mgr();
    EXPECT_TRUE(quest_map_mgr_init(state, 500, true));
    EXPECT_TRUE(is_quest_map(state));
    EXPECT_TRUE(state.m_bQuestChannelInitialized);
}

TEST(QuestMapMgrInit, OrdinaryMapDoesNotInitializeQuestChannel) {
    auto state = make_quest_map_mgr();
    EXPECT_FALSE(quest_map_mgr_init(state, 73, false));
    EXPECT_FALSE(state.m_bQuestChannelInitialized);
}

TEST(QuestMapMgrInit, MapKindOverridesHistoricalHardcodedNumbers) {
    auto state = make_quest_map_mgr();
    EXPECT_FALSE(quest_map_mgr_init(state, QUESTMAPNUM1, false));
    EXPECT_TRUE(quest_map_mgr_init(state, 999, true));
}

TEST(QuestMapMgrInit, ReinitFromQuestToOrdinaryClearsFlags) {
    auto state = make_quest_map_mgr();
    quest_map_mgr_init(state, 1, true);
    quest_map_mgr_init(state, 2, false);
    EXPECT_FALSE(state.m_bQuestMap);
    EXPECT_FALSE(state.m_bQuestChannelInitialized);
}

TEST(QuestMapMgrRemove, OrdinaryMapHasNoSideEffects) {
    const auto state = make_quest_map_mgr();
    const auto result = quest_map_remove_player(state, 42);
    EXPECT_FALSE(result.deleteQuestRecallMonster);
    EXPECT_FALSE(result.destroyQuestMapChannel);
    EXPECT_EQ(result.channelId, 0u);
}

TEST(QuestMapMgrRemove, QuestMapDeletesRecallAndDestroysChannel) {
    auto state = make_quest_map_mgr();
    quest_map_mgr_init(state, 1, true);
    const auto result = quest_map_remove_player(state, 42);
    EXPECT_TRUE(result.deleteQuestRecallMonster);
    EXPECT_TRUE(result.destroyQuestMapChannel);
    EXPECT_EQ(result.channelId, 42u);
}

TEST(QuestMapMgrRemove, ZeroChannelIsStillForwarded) {
    auto state = make_quest_map_mgr();
    quest_map_mgr_init(state, 1, true);
    const auto result = quest_map_remove_player(state, 0);
    EXPECT_TRUE(result.destroyQuestMapChannel);
    EXPECT_EQ(result.channelId, 0u);
}

TEST(QuestMapMgrDeath, OrdinaryMapPreservesReadyFlag) {
    const auto state = make_quest_map_mgr();
    EXPECT_TRUE(quest_map_die_player(state, true).readyToRevive);
    EXPECT_FALSE(quest_map_die_player(state, true).handled);
    EXPECT_FALSE(quest_map_die_player(state, false).readyToRevive);
}

TEST(QuestMapMgrDeath, QuestMapForcesReadyToReviveFalse) {
    auto state = make_quest_map_mgr();
    quest_map_mgr_init(state, 1, true);
    const auto result = quest_map_die_player(state, true);
    EXPECT_TRUE(result.handled);
    EXPECT_FALSE(result.readyToRevive);
}

TEST(QuestMapMgrDeath, QuestEventEmissionRemainsDisabled) {
    auto state = make_quest_map_mgr();
    quest_map_mgr_init(state, 1, true);
    const auto result = quest_map_die_player(state, false);
    EXPECT_TRUE(result.handled);
    EXPECT_FALSE(result.readyToRevive);
}
