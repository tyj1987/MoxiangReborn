#include "mxh/server/quest_execute.hpp"
#include "mxh/server/quest_group.hpp"

#include <gtest/gtest.h>

namespace {
using mxh::server::QuestExecuteKind;
using mxh::server::QuestExecuteApplyStatus;
using mxh::server::apply_count_execute;
using mxh::server::make_quest_group;
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
