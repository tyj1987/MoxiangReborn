#include <mxh/server/quest_group.hpp>
#include <gtest/gtest.h>

#include <mxh/server/quest_trigger.hpp>

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

// ---- D3.9 subquest counter access (legacy GetSubQuestValue + ChangeSubQuestValue) ----
TEST(QuestGroupSubQuestValue, GetMissingQuestReturnsSentinel) {
    auto state = make_quest_group();
    EXPECT_EQ(quest_group_get_sub_quest_value(state, 99, 0), QUEST_SUB_QUEST_VALUE_NOT_FOUND);
}

TEST(QuestGroupSubQuestValue, GetExistingButUnsetSubQuestReturnsZero) {
    auto state = make_quest_group();
    quest_group_create_quest(state, 7);
    // CQuest::GetSubQuestData on an un-set index reads as 0 in legacy.
    EXPECT_EQ(quest_group_get_sub_quest_value(state, 7, 2), 0u);
}

TEST(QuestGroupSubQuestValue, GetReturnsStoredCount) {
    auto state = make_quest_group();
    quest_group_create_quest(state, 7);
    quest_group_start_subquest(state, 7, 2, 0);
    // add_count(state, quest, sub, max) increments by 1 each call until
    // the count reaches max -- mirrors legacy CQuestGroup::AddCount's
    // bound-clamp semantics. Three Add calls == count 3.
    quest_group_add_count(state, 7, 2, 5);
    quest_group_add_count(state, 7, 2, 5);
    quest_group_add_count(state, 7, 2, 5);
    EXPECT_EQ(quest_group_get_sub_quest_value(state, 7, 2), 3u);
}

TEST(QuestGroupChangeSubQuestValue, AddIncrementsCount) {
    auto state = make_quest_group();
    quest_group_create_quest(state, 7);
    EXPECT_TRUE(quest_group_change_sub_quest_value(state, 7, 2, QUEST_VALUE_ADD));
    EXPECT_EQ(quest_group_get_sub_quest_value(state, 7, 2), 1u);
    EXPECT_TRUE(quest_group_change_sub_quest_value(state, 7, 2, QUEST_VALUE_ADD));
    EXPECT_EQ(quest_group_get_sub_quest_value(state, 7, 2), 2u);
}

TEST(QuestGroupChangeSubQuestValue, MinusDecrementsCount) {
    auto state = make_quest_group();
    quest_group_create_quest(state, 7);
    quest_group_change_sub_quest_value(state, 7, 2, QUEST_VALUE_ADD);
    quest_group_change_sub_quest_value(state, 7, 2, QUEST_VALUE_ADD);
    EXPECT_TRUE(quest_group_change_sub_quest_value(state, 7, 2, QUEST_VALUE_MINUS));
    EXPECT_EQ(quest_group_get_sub_quest_value(state, 7, 2), 1u);
}

TEST(QuestGroupChangeSubQuestValue, MinusClampsAtZero) {
    auto state = make_quest_group();
    quest_group_create_quest(state, 7);
    // No prior Add -- count is 0. Legacy CQuest::ChangeSubQuestValue Minus branch
    // takes the `else pSubQuest->dwData = 0` path, leaving the value at 0.
    EXPECT_TRUE(quest_group_change_sub_quest_value(state, 7, 2, QUEST_VALUE_MINUS));
    EXPECT_EQ(quest_group_get_sub_quest_value(state, 7, 2), 0u);
    // Twice in a row stays at 0 (no underflow).
    EXPECT_TRUE(quest_group_change_sub_quest_value(state, 7, 2, QUEST_VALUE_MINUS));
    EXPECT_EQ(quest_group_get_sub_quest_value(state, 7, 2), 0u);
}

TEST(QuestGroupChangeSubQuestValue, UnknownKindIsRejected) {
    auto state = make_quest_group();
    quest_group_create_quest(state, 7);
    EXPECT_FALSE(quest_group_change_sub_quest_value(state, 7, 2, 99u));
    // Counter unchanged on rejection.
    EXPECT_EQ(quest_group_get_sub_quest_value(state, 7, 2), 0u);
}

TEST(QuestGroupChangeSubQuestValue, MissingQuestReturnsFalse) {
    auto state = make_quest_group();
    EXPECT_FALSE(quest_group_change_sub_quest_value(state, 99, 2, QUEST_VALUE_ADD));
}



// ---- D3.10 quest end lifecycle tests (lock legacy EndQuest + EndSubQuest semantics) ----
TEST(QuestGroupEndQuest, MissingQuestReturnsFalse) {
    auto state = make_quest_group();
    EXPECT_FALSE(quest_group_end_quest(state, 99, 0, 12345u));
}

TEST(QuestGroupEndQuest, NonRepeatMarksCompleteAndClearsSubquests) {
    auto state = make_quest_group();
    quest_group_create_quest(state, 7);
    quest_group_start_subquest(state, 7, 2, 100);
    quest_group_start_subquest(state, 7, 3, 100);
    quest_group_set_main_data(state, 7, 0xFFu, 0, 100, 0, 0);
    EXPECT_TRUE(quest_group_end_quest(state, 7, 0, 5000u));
    const auto* quest = quest_group_get_quest(state, 7);
    ASSERT_NE(quest, nullptr);
    EXPECT_TRUE(quest->complete);
    EXPECT_EQ(quest->data, 1u);
    EXPECT_EQ(quest->time, 5000u);
    EXPECT_EQ(quest->subQuestFlag, 0u);
    EXPECT_TRUE(quest->activeSubquests.empty());
    EXPECT_TRUE(quest->subQuestData.empty());
}

TEST(QuestGroupEndQuest, RepeatLeavesQuestOpenAndResetsCounter) {
    auto state = make_quest_group();
    quest_group_create_quest(state, 7);
    EXPECT_TRUE(quest_group_end_quest(state, 7, 1, 9000u));
    const auto* quest = quest_group_get_quest(state, 7);
    ASSERT_NE(quest, nullptr);
    EXPECT_FALSE(quest->complete);
    EXPECT_EQ(quest->data, 0u);
    EXPECT_EQ(quest->time, 9000u);
}

TEST(QuestGroupEndSubQuest, MissingQuestReturnsFalse) {
    auto state = make_quest_group();
    EXPECT_FALSE(quest_group_end_subquest(state, 99, 2, 12345u));
}

TEST(QuestGroupEndSubQuest, NotActiveReturnsFalse) {
    auto state = make_quest_group();
    quest_group_create_quest(state, 7);
    // Subquest 2 was never started, so erase(2) returns 0.
    EXPECT_FALSE(quest_group_end_subquest(state, 7, 2, 12345u));
}

TEST(QuestGroupEndSubQuest, ActiveSetsFlagBitAndClearsTime) {
    auto state = make_quest_group();
    quest_group_create_quest(state, 7);
    quest_group_start_subquest(state, 7, 2, 100);
    EXPECT_TRUE(quest_group_end_subquest(state, 7, 2, 5000u));
    const auto* quest = quest_group_get_quest(state, 7);
    ASSERT_NE(quest, nullptr);
    EXPECT_EQ(quest->subQuestTime.at(2), 5000u);
    EXPECT_EQ(quest->subQuestData.at(2), 0u);
    // subQuestFlag bit for idx 2 is bit (31-2) = 29, so 0x20000000.
    EXPECT_EQ(quest->subQuestFlag, 0x20000000u);
    EXPECT_FALSE(quest->activeSubquests.count(2));
}

TEST(QuestGroupEndSubQuest, ZeroIdxResetsQuestDataAndTime) {
    auto state = make_quest_group();
    quest_group_create_quest(state, 7);
    quest_group_set_main_data(state, 7, 0, 9999, 100, 0, 0);
    quest_group_start_subquest(state, 7, 0, 100);
    EXPECT_TRUE(quest_group_end_subquest(state, 7, 0, 5000u));
    const auto* quest = quest_group_get_quest(state, 7);
    ASSERT_NE(quest, nullptr);
    // Ending the main (idx 0) subquest also resets quest->data / quest->time,
    // matching the legacy `if (dwSubQuestIdx == 0) { dwData=0; dwTime=... }`
    // branch in CQuest::EndQuest / CQuestGroup::EndSubQuest.
    EXPECT_EQ(quest->data, 0u);
    EXPECT_EQ(quest->time, 5000u);
}


// ---- D3.11 weapon-filtered AddCount (legacy AddCountFromWeapon + AddCountFromQWeapon) ----
TEST(QuestGroupWeaponFilter, AddCountFromWeaponMismatchedKindDoesNothing) {
    auto state = make_quest_group();
    quest_group_create_quest(state, 7);
    quest_group_start_subquest(state, 7, 2, 0);
    EXPECT_FALSE(quest_group_add_count_from_weapon(state, 7, 2, 3, 5u, 99u));
    EXPECT_EQ(quest_group_get_sub_quest_value(state, 7, 2), 0u);
}

TEST(QuestGroupWeaponFilter, AddCountFromWeaponMatchingKindIncrements) {
    auto state = make_quest_group();
    quest_group_create_quest(state, 7);
    quest_group_start_subquest(state, 7, 2, 0);
    EXPECT_TRUE(quest_group_add_count_from_weapon(state, 7, 2, 5u, 17u, 17u));
    EXPECT_EQ(quest_group_get_sub_quest_value(state, 7, 2), 1u);
    EXPECT_TRUE(quest_group_add_count_from_weapon(state, 7, 2, 5u, 17u, 17u));
    EXPECT_EQ(quest_group_get_sub_quest_value(state, 7, 2), 2u);
}

TEST(QuestGroupWeaponFilter, AddCountFromWeaponClampsAtMax) {
    auto state = make_quest_group();
    quest_group_create_quest(state, 7);
    quest_group_start_subquest(state, 7, 2, 0);
    quest_group_add_count_from_weapon(state, 7, 2, 2u, 5u, 5u);
    quest_group_add_count_from_weapon(state, 7, 2, 2u, 5u, 5u);
    EXPECT_EQ(quest_group_get_sub_quest_value(state, 7, 2), 2u);
    // Third call still passes the gate but max already reached -- returns
    // true but does not further increment (quest_group_add_count returns
    // true with no-op clamp).
    EXPECT_TRUE(quest_group_add_count_from_weapon(state, 7, 2, 2u, 5u, 5u));
    EXPECT_EQ(quest_group_get_sub_quest_value(state, 7, 2), 2u);
}

TEST(QuestGroupWeaponFilter, AddCountFromQWeaponMismatchedItemDoesNothing) {
    auto state = make_quest_group();
    quest_group_create_quest(state, 7);
    quest_group_start_subquest(state, 7, 2, 0);
    EXPECT_FALSE(quest_group_add_count_from_q_weapon(state, 7, 2, 3u, 50001u, 99999u));
    EXPECT_EQ(quest_group_get_sub_quest_value(state, 7, 2), 0u);
}

TEST(QuestGroupWeaponFilter, AddCountFromQWeaponMatchingItemIncrements) {
    auto state = make_quest_group();
    quest_group_create_quest(state, 7);
    quest_group_start_subquest(state, 7, 2, 0);
    EXPECT_TRUE(quest_group_add_count_from_q_weapon(state, 7, 2, 5u, 50001u, 50001u));
    EXPECT_EQ(quest_group_get_sub_quest_value(state, 7, 2), 1u);
}

TEST(QuestGroupWeaponFilter, AddCountFromWeaponMissingQuestReturnsFalse) {
    auto state = make_quest_group();
    EXPECT_FALSE(quest_group_add_count_from_weapon(state, 99, 2, 3u, 5u, 5u));
    EXPECT_FALSE(quest_group_add_count_from_q_weapon(state, 99, 2, 3u, 5u, 5u));
}



// ---- D3.8 quest_group_run_pending (legacy CQuestGroup::Process link) ----
TEST(QuestGroupRunPending, NoEventsProducesEmptyResult) {
    auto state = make_quest_group();
    quest_group_initialize(state, 9);
    quest_group_create_quest(state, 1);
    std::unordered_map<std::uint32_t, std::vector<QuestTrigger>> table;
    const auto out = quest_group_run_pending(state, table);
    EXPECT_TRUE(out.empty());
}

TEST(QuestGroupRunPending, AppliesFirstMatchingTriggerAndAdvancesSubCount) {
    auto state = make_quest_group();
    quest_group_initialize(state, 9);
    quest_group_create_quest(state, 7);
    quest_group_start_subquest(state, 7, 2, 0);
    const std::string src = "&LEVEL 1 99 @HUNT 1 10 *ADDCOUNT 2 3";
    const auto line = parse_quest_script_line(src, 7, 2);
    ASSERT_TRUE(line.has_value());
    QuestTrigger trigger = quest_trigger_from_script_line(*line);

    std::unordered_map<std::uint32_t, std::vector<QuestTrigger>> table;
    table[7] = {trigger};

    // Stage the runtime event: legacy eQuestEvent_Hunt with kind=2 param1=1 param2=10.
    state.m_QuestEvent.push_back({7u, static_cast<std::uint32_t>(QuestEventKind::Hunt), 1u, 10});

    const auto out = quest_group_run_pending(state, table);
    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].quest_idx, 7u);
    EXPECT_EQ(out[0].subquest_idx, 2u);
    EXPECT_EQ(out[0].status, QuestTriggerApplyStatus::Applied);
    EXPECT_TRUE(out[0].changed);
    EXPECT_EQ(state.m_QuestTable.at(7u).subQuestData.at(2u), 1u);
    // Events consumed exactly once (legacy Process clears m_QuestEvent).
    EXPECT_TRUE(state.m_QuestEvent.empty());
}

TEST(QuestGroupRunPending, SkipsQuestWithNoTriggerEntry) {
    auto state = make_quest_group();
    quest_group_initialize(state, 9);
    quest_group_create_quest(state, 7);
    std::unordered_map<std::uint32_t, std::vector<QuestTrigger>> table;  // empty
    state.m_QuestEvent.push_back({7u, static_cast<std::uint32_t>(QuestEventKind::Hunt), 1u, 10});
    const auto out = quest_group_run_pending(state, table);
    EXPECT_TRUE(out.empty());
    EXPECT_TRUE(state.m_QuestEvent.empty());  // still drained
}

TEST(QuestGroupRunPending, FiresFirstConditionMatchAndIgnoresTheRest) {
    auto state = make_quest_group();
    quest_group_initialize(state, 9);
    quest_group_create_quest(state, 7);
    quest_group_start_subquest(state, 7, 2, 0);

    QuestTrigger non_matching;
    non_matching.quest_idx = 7;
    non_matching.subquest_idx = 2;
    non_matching.event = {QuestEventKind::NpcTalk, 99u, 99};  // won't match
    const std::string match_src = "&LEVEL 1 99 @HUNT 1 10 *ADDCOUNT 2 3";
    const auto match_line = parse_quest_script_line(match_src, 7, 2);
    QuestTrigger matching = quest_trigger_from_script_line(*match_line);

    std::unordered_map<std::uint32_t, std::vector<QuestTrigger>> table;
    table[7] = {non_matching, matching};

    state.m_QuestEvent.push_back({7u, static_cast<std::uint32_t>(QuestEventKind::Hunt), 1u, 10});

    const auto out = quest_group_run_pending(state, table);
    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].subquest_idx, 2u);
    EXPECT_EQ(state.m_QuestTable.at(7u).subQuestData.at(2u), 1u);
}

TEST(QuestGroupRunPending, FansOutAcrossMultipleQuests) {
    auto state = make_quest_group();
    quest_group_initialize(state, 9);
    quest_group_create_quest(state, 7);
    quest_group_create_quest(state, 8);
    quest_group_start_subquest(state, 7, 2, 0);
    quest_group_start_subquest(state, 8, 3, 0);

    const std::string src7 = "&LEVEL 1 99 @HUNT 1 10 *ADDCOUNT 2 3";
    const std::string src8 = "&LEVEL 1 99 @HUNT 1 10 *ADDCOUNT 3 3";
    QuestTrigger t7 = quest_trigger_from_script_line(*parse_quest_script_line(src7, 7, 2));
    QuestTrigger t8 = quest_trigger_from_script_line(*parse_quest_script_line(src8, 8, 3));

    std::unordered_map<std::uint32_t, std::vector<QuestTrigger>> table;
    table[7] = {t7};
    table[8] = {t8};

    state.m_QuestEvent.push_back({0u, static_cast<std::uint32_t>(QuestEventKind::Hunt), 1u, 10});

    const auto out = quest_group_run_pending(state, table);
    EXPECT_EQ(out.size(), 2u);
    EXPECT_EQ(state.m_QuestTable.at(7u).subQuestData.at(2u), 1u);
    EXPECT_EQ(state.m_QuestTable.at(8u).subQuestData.at(3u), 1u);
}

TEST(QuestGroupRunPending, ConditionFailedLeavesQuestUntouched) {
    auto state = make_quest_group();
    quest_group_initialize(state, 9);
    quest_group_create_quest(state, 7);
    quest_group_start_subquest(state, 7, 2, 0);
    // start_subquest initializes subQuestData[2]=0 -- that is the legacy
    // baseline. After a condition-failed dispatch, that 0 must stay 0
    // (no ADDCOUNT ever ran).
    EXPECT_EQ(state.m_QuestTable.at(7u).subQuestData[2u], 0u);
    const std::string src = "&LEVEL 1 99 @HUNT 5 5 *ADDCOUNT 2 3";
    QuestTrigger t = quest_trigger_from_script_line(*parse_quest_script_line(src, 7, 2));
    std::unordered_map<std::uint32_t, std::vector<QuestTrigger>> table;
    table[7] = {t};
    // Mismatched runtime: kind matches, but param1 differs.
    state.m_QuestEvent.push_back({7u, static_cast<std::uint32_t>(QuestEventKind::Hunt), 1u, 5});
    const auto out = quest_group_run_pending(state, table);
    EXPECT_TRUE(out.empty());
    EXPECT_EQ(state.m_QuestTable.at(7u).subQuestData[2u], 0u);
    EXPECT_EQ(state.m_QuestEvent.empty(), true);
}

TEST(QuestGroupRunPending, IdempotentAfterEventsDrained) {
    auto state = make_quest_group();
    quest_group_initialize(state, 9);
    quest_group_create_quest(state, 7);
    quest_group_start_subquest(state, 7, 2, 0);
    const std::string src = "&LEVEL 1 99 @HUNT 1 10 *ADDCOUNT 2 3";
    QuestTrigger t = quest_trigger_from_script_line(*parse_quest_script_line(src, 7, 2));
    std::unordered_map<std::uint32_t, std::vector<QuestTrigger>> table;
    table[7] = {t};
    state.m_QuestEvent.push_back({7u, static_cast<std::uint32_t>(QuestEventKind::Hunt), 1u, 10});
    auto out1 = quest_group_run_pending(state, table);
    auto out2 = quest_group_run_pending(state, table);
    EXPECT_EQ(out1.size(), 1u);
    EXPECT_TRUE(out2.empty());
    EXPECT_EQ(state.m_QuestTable.at(7u).subQuestData.at(2u), 1u);  // not re-applied
}
