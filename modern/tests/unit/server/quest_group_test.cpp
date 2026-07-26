#include <mxh/server/quest_group.hpp>
#include <gtest/gtest.h>

using namespace mxh::server;

TEST(QuestGroupConstants, MatchLegacy) {
    EXPECT_EQ(MAX_QUEST, 1000u);
    EXPECT_EQ(MAX_QUESTEVENT_PLAYER, 100u);
    EXPECT_EQ(MAX_QUESTITEM, 100u);
    EXPECT_EQ(MAX_QUEST_PROBABILITY, 10000u);
}

TEST(QuestGroupInit, DefaultHasNoPlayerAndEmptyTables) {
    const auto state = make_quest_group();
    EXPECT_FALSE(state.m_hasPlayer);
    EXPECT_TRUE(state.m_QuestTable.empty());
    EXPECT_TRUE(state.m_QuestItemTable.empty());
    EXPECT_TRUE(state.m_QuestEvent.empty());
}

TEST(QuestGroupInit, InitializeSetsPlayerAndClearsEventsOnly) {
    auto state = make_quest_group();
    quest_group_create_quest(state, 1);
    quest_group_add_event(state, {1, 2, 3, 4});
    quest_group_initialize(state, 99);
    EXPECT_TRUE(state.m_hasPlayer);
    EXPECT_EQ(state.m_playerId, 99u);
    EXPECT_EQ(state.m_QuestTable.size(), 1u);
    EXPECT_TRUE(state.m_QuestEvent.empty());
}

TEST(QuestGroupRelease, ClearsEverythingAndPlayer) {
    auto state = make_quest_group();
    quest_group_initialize(state, 99);
    quest_group_create_quest(state, 1);
    quest_group_set_item(state, 1, 10, 2);
    quest_group_release(state);
    EXPECT_FALSE(state.m_hasPlayer);
    EXPECT_TRUE(state.m_QuestTable.empty());
    EXPECT_TRUE(state.m_QuestItemTable.empty());
}

TEST(QuestGroupQuest, CreateDuplicateIsNoOp) {
    auto state = make_quest_group();
    EXPECT_TRUE(quest_group_create_quest(state, 1));
    EXPECT_FALSE(quest_group_create_quest(state, 1));
    EXPECT_EQ(state.m_QuestTable.size(), 1u);
}

TEST(QuestGroupQuest, MissingMainDataReturnsLegacyTrue) {
    auto state = make_quest_group();
    EXPECT_TRUE(quest_group_set_main_data(state, 999, 1, 2, 3, 4, 5));
}

TEST(QuestGroupQuest, MainAndSubDataRoundTrip) {
    auto state = make_quest_group();
    quest_group_create_quest(state, 1);
    quest_group_set_main_data(state, 1, 7, 8, 9, 2, 10);
    EXPECT_TRUE(quest_group_set_subquest_data(state, 1, 3, 11, 12));
    const auto* quest = quest_group_get_quest(state, 1);
    ASSERT_NE(quest, nullptr);
    EXPECT_EQ(quest->subQuestFlag, 7u);
    EXPECT_EQ(quest->subQuestData.at(3), 11u);
    EXPECT_EQ(quest->subQuestTime.at(3), 12u);
}

TEST(QuestGroupItem, ItemIndexIsUniqueKeyAndReplacementDeletesOldRecord) {
    auto state = make_quest_group();
    quest_group_set_item(state, 1, 100, 2);
    quest_group_set_item(state, 2, 100, 5);
    ASSERT_EQ(state.m_QuestItemTable.size(), 1u);
    EXPECT_EQ(state.m_QuestItemTable.at(100).dwQuestIdx, 2u);
    EXPECT_EQ(state.m_QuestItemTable.at(100).dwItemNum, 5u);
}

TEST(QuestGroupEvent, CapacityIsExactlyOneHundred) {
    auto state = make_quest_group();
    for (std::uint32_t i = 0; i < 100; ++i)
        EXPECT_TRUE(quest_group_add_event(state, {i, 1, 2, 3}));
    EXPECT_FALSE(quest_group_add_event(state, {101, 1, 2, 3}));
    EXPECT_EQ(state.m_QuestEvent.size(), 100u);
}

TEST(QuestGroupComplete, MissingAndIncompleteReturnFalse) {
    auto state = make_quest_group();
    EXPECT_FALSE(quest_group_is_complete(state, 1));
    quest_group_create_quest(state, 1);
    EXPECT_FALSE(quest_group_is_complete(state, 1));
    quest_group_get_quest(state, 1)->complete = true;
    EXPECT_TRUE(quest_group_is_complete(state, 1));
}

TEST(QuestGroupDelete, RemovesOnlyItemsOwnedByQuest) {
    auto state = make_quest_group();
    quest_group_create_quest(state, 1);
    quest_group_set_item(state, 1, 10, 1);
    quest_group_set_item(state, 1, 11, 1);
    quest_group_set_item(state, 2, 12, 1);
    EXPECT_EQ(quest_group_delete_quest(state, 1), 2u);
    EXPECT_TRUE(quest_group_get_quest(state, 1)->deleteRequested);
    EXPECT_EQ(state.m_QuestItemTable.size(), 1u);
}

TEST(QuestGroupDelete, DoesNotRemoveQuestRecord) {
    auto state = make_quest_group();
    quest_group_create_quest(state, 1);
    quest_group_delete_quest(state, 1);
    EXPECT_NE(quest_group_get_quest(state, 1), nullptr);
}

TEST(QuestGroupProcessCount, RequiresFlagAndIncompleteState) {
    auto state = make_quest_group();
    quest_group_create_quest(state, 1);
    quest_group_create_quest(state, 2);
    quest_group_create_quest(state, 3);
    quest_group_get_quest(state, 1)->subQuestFlag = 1;
    quest_group_get_quest(state, 2)->subQuestFlag = 0;
    quest_group_get_quest(state, 3)->subQuestFlag = 2;
    quest_group_get_quest(state, 3)->complete = true;
    EXPECT_EQ(quest_group_process_quest_count(state), 1);
}

TEST(QuestGroupProcess, NoPlayerOrNoEventsDoesNothing) {
    auto state = make_quest_group();
    quest_group_create_quest(state, 1);
    quest_group_add_event(state, {0, 2, 3, 4});
    EXPECT_TRUE(quest_group_process(state).empty());
    EXPECT_EQ(state.m_QuestEvent.size(), 1u);
    quest_group_initialize(state, 9);
    EXPECT_TRUE(quest_group_process(state).empty());
}

TEST(QuestGroupProcess, FansEveryEventToEveryIncompleteQuestThenClears) {
    auto state = make_quest_group();
    quest_group_initialize(state, 9);
    quest_group_create_quest(state, 1);
    quest_group_create_quest(state, 2);
    quest_group_create_quest(state, 3);
    quest_group_get_quest(state, 3)->complete = true;
    quest_group_add_event(state, {77, 1, 2, 3});
    quest_group_add_event(state, {88, 4, 5, 6});
    const auto dispatches = quest_group_process(state);
    EXPECT_EQ(dispatches.size(), 4u);
    EXPECT_TRUE(state.m_QuestEvent.empty());
}

TEST(QuestGroupProbability, ZeroNeverAndTenThousandAlways) {
    EXPECT_FALSE(check_quest_probability(0, 0));
    EXPECT_TRUE(check_quest_probability(10000, 999999));
}

TEST(QuestGroupProbability, UsesRandomModuloTenThousand) {
    EXPECT_TRUE(check_quest_probability(5000, 4999));
    EXPECT_FALSE(check_quest_probability(5000, 5000));
    EXPECT_TRUE(check_quest_probability(1, 10000));
}

TEST(QuestGroupLoginPoint, RejectsMapsAboveTwoThousand) {
    std::uint16_t save = 1, login = 2;
    EXPECT_FALSE(quest_group_save_login_point(2001, save, login));
    EXPECT_EQ(save, 1u);
    EXPECT_EQ(login, 2u);
}

TEST(QuestGroupLoginPoint, AddsTwoThousandToLoginPoint) {
    std::uint16_t save = 0, login = 0;
    EXPECT_TRUE(quest_group_save_login_point(73, save, login));
    EXPECT_EQ(save, 73u);
    EXPECT_EQ(login, 2073u);
}


TEST(QuestGroupExecution, AddCountClampsAtMaximum) {
    auto state = make_quest_group();
    quest_group_create_quest(state, 7);
    EXPECT_TRUE(quest_group_add_count(state, 7, 2, 2));
    EXPECT_TRUE(quest_group_add_count(state, 7, 2, 2));
    EXPECT_EQ(quest_group_get_quest(state, 7)->subQuestData[2], 2u);
}

TEST(QuestGroupExecution, LevelGapUsesDirectionalLegacyBounds) {
    auto state = make_quest_group();
    quest_group_create_quest(state, 7);
    EXPECT_TRUE(quest_group_add_count_from_level_gap(state, 7, 1, 5, 3, 2, 10, 8));
    EXPECT_FALSE(quest_group_add_count_from_level_gap(state, 7, 1, 5, 1, 2, 10, 8));
    EXPECT_TRUE(quest_group_add_count_from_monster_level(state, 7, 1, 5, 8, 10, 9));
    EXPECT_FALSE(quest_group_add_count_from_monster_level(state, 7, 1, 5, 8, 10, 11));
}

TEST(QuestGroupExecution, StageTransitionsFollowLegacyGraph) {
    std::uint8_t stage = 0;
    EXPECT_TRUE(quest_group_change_stage(0, 1, stage));
    EXPECT_EQ(stage, 1);
    EXPECT_TRUE(quest_group_change_stage(stage, 2, stage));
    EXPECT_FALSE(quest_group_change_stage(stage, 4, stage));
    EXPECT_TRUE(quest_group_change_stage(0, 3, stage));
    EXPECT_TRUE(quest_group_change_stage(3, 4, stage));
}

TEST(QuestGroupExecution, MoneyPerCountConsumesQuestItem) {
    auto state = make_quest_group();
    quest_group_set_item(state, 9, 42, 3);
    EXPECT_EQ(quest_group_take_money_per_count(state, 42, 17), 51u);
    EXPECT_EQ(state.m_QuestItemTable.count(42), 0u);
    EXPECT_EQ(quest_group_take_money_per_count(state, 42, 17), 0u);
}
