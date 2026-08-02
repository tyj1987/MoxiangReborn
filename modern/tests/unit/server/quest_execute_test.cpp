#include "mxh/server/quest_execute.hpp"
#include "mxh/server/quest_group.hpp"

#include <gtest/gtest.h>

namespace {
using mxh::server::QuestExecuteKind;
using mxh::server::QuestExecuteApplyStatus;
using mxh::server::apply_count_execute;
using mxh::server::apply_quest_execute;
using mxh::server::apply_time_execute;
using mxh::server::apply_item_execute;
using mxh::server::make_quest_group;
using mxh::server::quest_group_create_quest;
using mxh::server::quest_group_get_quest;
using mxh::server::quest_group_set_item;
using mxh::server::quest_group_get_quest;
using mxh::server::quest_group_start_subquest;
using mxh::server::quest_group_end_subquest;
using mxh::server::quest_group_end_quest;
using mxh::server::quest_group_register_check_time;
using mxh::server::quest_group_save_login_point;
using mxh::server::quest_group_add_count;
using mxh::server::quest_group_change_stage;
using mxh::server::parse_quest_execute;
using mxh::server::quest_execute_kind_from_token;
}

TEST(QuestExecuteParser, MapsAllLoaderCommands) {
    EXPECT_EQ(quest_execute_kind_from_token("*ENDQUEST"), QuestExecuteKind::EndQuest);
    EXPECT_EQ(quest_execute_kind_from_token("*ADDCOUNTLEVELGAP"), QuestExecuteKind::LevelGap);
    EXPECT_EQ(quest_execute_kind_from_token("*TAKEQUESTITEMFQW"), QuestExecuteKind::TakeQuestItemFQW);
    EXPECT_EQ(quest_execute_kind_from_token("*REGISTTIME"), QuestExecuteKind::RegistTime);
    EXPECT_EQ(quest_execute_kind_from_token("*SAVELOGINPOINT"), QuestExecuteKind::SaveLoginPoint);
    EXPECT_FALSE(quest_execute_kind_from_token("*STARTQUEST").has_value());
    EXPECT_FALSE(quest_execute_kind_from_token("*MINUSCOUNT").has_value());
}

TEST(QuestExecuteParser, AcceptsEveryCommandRegisteredByLegacyLoader) {
    const std::vector<const char*> tokens = {
        "*ENDQUEST", "*STARTSUB", "*ENDSUB", "*ENDOTHERSUB",
        "*ADDCOUNT", "*ADDCOUNTFQW", "*ADDCOUNTFW", "*ADDCOUNTLEVELGAP",
        "*ADDCOUNTMONLEVEL", "*GIVEQUESTITEM", "*TAKEQUESTITEM", "*GIVEITEM",
        "*GIVEMONEY", "*TAKEITEM", "*TAKEMONEY", "*TAKEEXP", "*TAKESEXP",
        "*RANDOMTAKEITEM", "*TAKEQUESTITEMFQW", "*TAKEQUESTITEMFW",
        "*TAKEMONEYPERCOUNT", "*REGENMONSTER", "*MAPCHANGE", "*CHANGESTAGE",
        "*REGISTTIME", "*ENDOTHERQUEST", "*SAVELOGINPOINT"};
    for (const auto* token : tokens)
        EXPECT_TRUE(quest_execute_kind_from_token(token).has_value()) << token;
}

TEST(QuestExecuteParser, ParsesCountAndPreservesContext) {
    const auto parsed = parse_quest_execute("*ADDCOUNT\t17 25\r\n", 1001u, 3u);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->kind, QuestExecuteKind::AddCount);
    EXPECT_EQ(parsed->quest_idx, 1001u);
    EXPECT_EQ(parsed->subquest_idx, 3u);
    ASSERT_EQ(parsed->args, (std::vector<std::uint32_t>{17u, 25u}));
}

TEST(QuestExecuteParser, ParsesWeaponAndTimeArgumentShapes) {
    const auto weapon = parse_quest_execute("*TAKEQUESTITEMFW 500 2 7500 9", 1u, 2u);
    ASSERT_TRUE(weapon.has_value());
    EXPECT_EQ(weapon->kind, QuestExecuteKind::TakeQuestItemFW);
    EXPECT_EQ(weapon->args[3], 9u);
    const auto time = parse_quest_execute("*REGISTTIME 1 2 3 4", 1u, 2u);
    ASSERT_TRUE(time.has_value());
    EXPECT_EQ(time->kind, QuestExecuteKind::RegistTime);
    EXPECT_EQ(time->args, (std::vector<std::uint32_t>{1u, 2u, 3u, 4u}));
}

TEST(QuestExecuteParser, ParsesRandomItemTriplesIntoWordFields) {
    const auto parsed = parse_quest_execute(
        "*RANDOMTAKEITEM 2 1 100 3 2500 200 1 7500", 9u, 4u);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->kind, QuestExecuteKind::RandomTakeItem);
    ASSERT_EQ(parsed->random_items.size(), 2u);
    EXPECT_EQ(parsed->random_items[0].item_idx, 100u);
    EXPECT_EQ(parsed->random_items[0].item_num, 3u);
    EXPECT_EQ(parsed->random_items[0].percent, 2500u);
    EXPECT_EQ(parsed->random_items[1].item_idx, 200u);
    EXPECT_EQ(parsed->random_items[1].percent, 7500u);
}

TEST(QuestExecuteParser, RejectsMissingExtraMalformedAndOutOfRangeArgs) {
    EXPECT_FALSE(parse_quest_execute("*ADDCOUNT 1", 1u, 1u).has_value());
    EXPECT_FALSE(parse_quest_execute("*ENDSUB 1", 1u, 1u).has_value());
    EXPECT_FALSE(parse_quest_execute("*GIVEMONEY nope", 1u, 1u).has_value());
    EXPECT_FALSE(parse_quest_execute("*RANDOMTAKEITEM 1 2 1 1 10000", 1u, 1u).has_value());
    EXPECT_FALSE(parse_quest_execute("*RANDOMTAKEITEM 1 1 70000 1 1", 1u, 1u).has_value());
    EXPECT_FALSE(parse_quest_execute("unknown", 1u, 1u).has_value());
}

TEST(QuestExecuteApply, AddCountUsesParsedSubquestAndClamps) {
    auto state = make_quest_group();
    ASSERT_TRUE(mxh::server::quest_group_create_quest(state, 7u));
    const auto spec = parse_quest_execute("*ADDCOUNT 2 3", 7u, 0u);
    ASSERT_TRUE(spec.has_value());
    EXPECT_EQ(apply_count_execute(state, *spec).status, QuestExecuteApplyStatus::Applied);
    EXPECT_EQ(state.m_QuestTable.at(7u).subQuestData.at(2u), 1u);
    EXPECT_EQ(apply_count_execute(state, *spec).status, QuestExecuteApplyStatus::Applied);
    EXPECT_EQ(state.m_QuestTable.at(7u).subQuestData.at(2u), 2u);
}

TEST(QuestExecuteApply, LevelGapAndMonsterLevelForwardBounds) {
    auto state = make_quest_group();
    ASSERT_TRUE(mxh::server::quest_group_create_quest(state, 7u));
    const auto gap = parse_quest_execute("*ADDCOUNTLEVELGAP 1 5 3 2", 7u, 0u);
    ASSERT_TRUE(gap.has_value());
    EXPECT_EQ(apply_count_execute(state, *gap, 10, 8).status,
              QuestExecuteApplyStatus::Applied);
    const auto level = parse_quest_execute("*ADDCOUNTMONLEVEL 1 5 8 10", 7u, 0u);
    ASSERT_TRUE(level.has_value());
    EXPECT_EQ(apply_count_execute(state, *level, 0, 9).status,
              QuestExecuteApplyStatus::Applied);
    EXPECT_EQ(state.m_QuestTable.at(7u).subQuestData.at(1u), 2u);
}

TEST(QuestExecuteApply, MissingQuestAndWeaponContextAreExplicit) {
    auto state = make_quest_group();
    const auto missing = parse_quest_execute("*ADDCOUNT 1 2", 99u, 0u);
    ASSERT_TRUE(missing.has_value());
    EXPECT_EQ(apply_count_execute(state, *missing).status,
              QuestExecuteApplyStatus::MissingQuest);
    ASSERT_TRUE(mxh::server::quest_group_create_quest(state, 99u));
    const auto weapon = parse_quest_execute("*ADDCOUNTFW 1 2 3", 99u, 0u);
    ASSERT_TRUE(weapon.has_value());
    EXPECT_EQ(apply_count_execute(state, *weapon).status,
              QuestExecuteApplyStatus::UnsupportedContext);
}

TEST(QuestGroupStartSub, FirstCallInsertsAndSecondIsNoOp) {
    auto state = make_quest_group();
    ASSERT_TRUE(quest_group_create_quest(state, 7u));
    EXPECT_TRUE(quest_group_start_subquest(state, 7u, 2u, 1000u));
    EXPECT_FALSE(quest_group_start_subquest(state, 7u, 2u, 2000u));
    const auto* quest = quest_group_get_quest(state, 7u);
    ASSERT_NE(quest, nullptr);
    EXPECT_EQ(quest->subQuestTime.at(2u), 1000u);
}

TEST(QuestGroupEndSub, SetsLegacyFlagBitAndClearsActiveSlot) {
    auto state = make_quest_group();
    ASSERT_TRUE(quest_group_create_quest(state, 7u));
    ASSERT_TRUE(quest_group_start_subquest(state, 7u, 3u, 0u));
    EXPECT_TRUE(quest_group_end_subquest(state, 7u, 3u, 1234u));
    const auto* quest = quest_group_get_quest(state, 7u);
    ASSERT_NE(quest, nullptr);
    EXPECT_EQ(quest->subQuestFlag, 1u << (31u - 3u));
    EXPECT_EQ(quest->subQuestData.at(3u), 0u);
}

TEST(QuestGroupEndQuest, RepeatRebuildsAndFinalMarksComplete) {
    auto state = make_quest_group();
    ASSERT_TRUE(quest_group_create_quest(state, 7u));
    ASSERT_TRUE(quest_group_start_subquest(state, 7u, 1u, 0u));
    EXPECT_TRUE(quest_group_end_quest(state, 7u, /*repeat=*/1u, 500u));
    const auto* quest = quest_group_get_quest(state, 7u);
    ASSERT_NE(quest, nullptr);
    EXPECT_EQ(quest->data, 0u);
    EXPECT_FALSE(quest->complete);
    EXPECT_TRUE(quest->activeSubquests.empty());
    EXPECT_TRUE(quest_group_end_quest(state, 7u, /*repeat=*/0u, 900u));
    EXPECT_TRUE(quest->complete);
    EXPECT_EQ(quest->data, 1u);
}

TEST(QuestGroupCheckTime, StoresLegacyFieldsAndToggles) {
    auto state = make_quest_group();
    ASSERT_TRUE(quest_group_create_quest(state, 7u));
    EXPECT_TRUE(quest_group_register_check_time(state, 7u, 2u, 1u, 3u, 4u, 5u));
    const auto* quest = quest_group_get_quest(state, 7u);
    ASSERT_NE(quest, nullptr);
    EXPECT_TRUE(quest->checkTimeActive);
    EXPECT_EQ(quest->checkType, 1u);
    EXPECT_EQ(quest->checkDay, 3u);
    EXPECT_EQ(quest->checkHour, 4u);
    EXPECT_EQ(quest->checkMinute, 5u);
    EXPECT_TRUE(quest_group_end_quest(state, 7u, 0u, 100u));
    EXPECT_FALSE(quest->checkTimeActive);
}

TEST(QuestGroupSaveLoginPoint, RejectsOutOfRangeAndStoresPair) {
    auto state = make_quest_group();
    EXPECT_FALSE(quest_group_save_login_point(state, 2001u));
    EXPECT_TRUE(quest_group_save_login_point(state, 73u));
    EXPECT_EQ(state.m_savePoint, 73u);
    EXPECT_EQ(state.m_loginPoint, 2073u);
}

TEST(QuestExecuteApply, StartSubAndEndSubForwardIntoQuestGroup) {
    auto state = make_quest_group();
    ASSERT_TRUE(quest_group_create_quest(state, 7u));
    const auto start = parse_quest_execute("*STARTSUB 2 1", 7u, 0u);
    ASSERT_TRUE(start.has_value());
    EXPECT_EQ(apply_quest_execute(state, *start, 1000u).status,
              QuestExecuteApplyStatus::Applied);
    const auto end = parse_quest_execute("*ENDSUB", 7u, 2u);
    ASSERT_TRUE(end.has_value());
    EXPECT_EQ(apply_quest_execute(state, *end, 2000u).status,
              QuestExecuteApplyStatus::Applied);
    EXPECT_EQ(state.m_QuestTable.at(7u).subQuestFlag, 1u << (31u - 2u));
}

TEST(QuestExecuteApply, EndOtherSubAndEndOtherQuestRespectArgs) {
    auto state = make_quest_group();
    ASSERT_TRUE(quest_group_create_quest(state, 7u));
    ASSERT_TRUE(quest_group_start_subquest(state, 7u, 3u, 0u));
    const auto end_other_sub = parse_quest_execute("*ENDOTHERSUB 3 0", 7u, 0u);
    ASSERT_TRUE(end_other_sub.has_value());
    EXPECT_EQ(apply_quest_execute(state, *end_other_sub, 1000u).status,
              QuestExecuteApplyStatus::Applied);
    ASSERT_TRUE(quest_group_start_subquest(state, 7u, 0u, 0u));
    const auto end_other_quest = parse_quest_execute("*ENDOTHERQUEST 1 0", 7u, 0u);
    ASSERT_TRUE(end_other_quest.has_value());
    EXPECT_EQ(apply_quest_execute(state, *end_other_quest, 1500u).status,
              QuestExecuteApplyStatus::Applied);
    EXPECT_TRUE(state.m_QuestTable.at(7u).complete);
}

TEST(QuestExecuteApply, RegistTimeAndSaveLoginPointPersist) {
    auto state = make_quest_group();
    ASSERT_TRUE(quest_group_create_quest(state, 7u));
    const auto time = parse_quest_execute("*REGISTTIME 1 2 3 4", 7u, 5u);
    ASSERT_TRUE(time.has_value());
    EXPECT_EQ(apply_time_execute(state, *time).status, QuestExecuteApplyStatus::Applied);
    EXPECT_TRUE(state.m_QuestTable.at(7u).checkTimeActive);
    const auto save = parse_quest_execute("*SAVELOGINPOINT 12", 7u, 0u);
    ASSERT_TRUE(save.has_value());
    EXPECT_EQ(apply_quest_execute(state, *save, 0u).status,
              QuestExecuteApplyStatus::Applied);
    EXPECT_EQ(state.m_savePoint, 12u);
    EXPECT_EQ(state.m_loginPoint, 2012u);
}

TEST(QuestExecuteApply, MissingSubquestReturnsExplicitStatus) {
    auto state = make_quest_group();
    ASSERT_TRUE(quest_group_create_quest(state, 7u));
    const auto end = parse_quest_execute("*ENDSUB", 7u, 3u);
    ASSERT_TRUE(end.has_value());
    EXPECT_EQ(apply_quest_execute(state, *end, 1000u).status,
              QuestExecuteApplyStatus::MissingSubquest);
}

TEST(QuestExecuteApply, GiveQuestItemRemovesAndTakeRefillsTable) {
    auto state = make_quest_group();
    ASSERT_TRUE(quest_group_create_quest(state, 7u));
    quest_group_set_item(state, 7u, 100u, 2u);
    const auto give = parse_quest_execute("*GIVEQUESTITEM 100 0", 7u, 0u);
    ASSERT_TRUE(give.has_value());
    EXPECT_EQ(apply_item_execute(state, *give).status,
              QuestExecuteApplyStatus::Applied);
    EXPECT_EQ(state.m_QuestItemTable.count(100u), 0u);

    const auto take = parse_quest_execute("*TAKEQUESTITEM 100 4 9000", 7u, 0u);
    ASSERT_TRUE(take.has_value());
    EXPECT_EQ(apply_item_execute(state, *take).status,
              QuestExecuteApplyStatus::Applied);
    EXPECT_EQ(state.m_QuestItemTable.at(100u).dwItemNum, 4u);
    EXPECT_EQ(state.m_QuestTable.at(7u).subQuestData.at(0u), 1u);
}

TEST(QuestExecuteApply, TakeMoneyPerCountEmptiesAndReturnsTrue) {
    auto state = make_quest_group();
    quest_group_set_item(state, 7u, 100u, 3u);
    const auto spec = parse_quest_execute("*TAKEMONEYPERCOUNT 100 17", 7u, 0u);
    ASSERT_TRUE(spec.has_value());
    EXPECT_EQ(apply_item_execute(state, *spec).status,
              QuestExecuteApplyStatus::Applied);
    EXPECT_EQ(state.m_QuestItemTable.count(100u), 0u);
}

TEST(QuestExecuteApply, GiveMoneyAndTakeItemRequireUnsupportedContext) {
    auto state = make_quest_group();
    const auto give_money = parse_quest_execute("*GIVEMONEY 100", 7u, 0u);
    ASSERT_TRUE(give_money.has_value());
    EXPECT_EQ(apply_item_execute(state, *give_money).status,
              QuestExecuteApplyStatus::UnsupportedContext);
    const auto take_item = parse_quest_execute("*TAKEITEM 1 1 10000", 7u, 0u);
    ASSERT_TRUE(take_item.has_value());
    EXPECT_EQ(apply_item_execute(state, *take_item).status,
              QuestExecuteApplyStatus::UnsupportedContext);
}
