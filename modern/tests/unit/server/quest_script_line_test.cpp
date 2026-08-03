#include "mxh/server/quest_script_line.hpp"
#include "mxh/server/quest_execute.hpp"

#include <gtest/gtest.h>

namespace {
using mxh::server::QuestEventKind;
using mxh::server::QuestLimitKind;
using mxh::server::quest_event_kind_from_token;
using mxh::server::quest_limit_kind_from_token;
using mxh::server::parse_quest_script_line;
using mxh::server::quest_event_matches;
}

TEST(QuestEventKindFromToken, MapsAllScriptLoadableTokens) {
    EXPECT_EQ(quest_event_kind_from_token("@TALKTONPC"), QuestEventKind::NpcTalk);
    EXPECT_EQ(quest_event_kind_from_token("@HUNT"), QuestEventKind::Hunt);
    EXPECT_EQ(quest_event_kind_from_token("@COUNT"), QuestEventKind::Count);
    EXPECT_EQ(quest_event_kind_from_token("@GAMEENTER"), QuestEventKind::GameEnter);
    EXPECT_EQ(quest_event_kind_from_token("@LEVEL"), QuestEventKind::Level);
    EXPECT_EQ(quest_event_kind_from_token("@USEITEM"), QuestEventKind::UseItem);
    EXPECT_EQ(quest_event_kind_from_token("@MAPCHANGE"), QuestEventKind::MapChange);
    EXPECT_EQ(quest_event_kind_from_token("@DIE"), QuestEventKind::Die);
    EXPECT_EQ(quest_event_kind_from_token("@HUNTALL"), QuestEventKind::HuntAll);
}

TEST(QuestEventKindFromToken, RejectsRuntimeOnlyAndUnknownTokens) {
    EXPECT_FALSE(quest_event_kind_from_token("@END_SUB").has_value());
    EXPECT_FALSE(quest_event_kind_from_token("@TIME").has_value());
    EXPECT_FALSE(quest_event_kind_from_token("@UNKNOWN").has_value());
    EXPECT_FALSE(quest_event_kind_from_token("HUNT").has_value());
    EXPECT_FALSE(quest_event_kind_from_token("").has_value());
}

TEST(QuestLimitKindFromToken, MapsAllRegisteredLimitTokens) {
    EXPECT_EQ(quest_limit_kind_from_token("&LEVEL"), QuestLimitKind::Level);
    EXPECT_EQ(quest_limit_kind_from_token("&MONEY"), QuestLimitKind::Money);
    EXPECT_EQ(quest_limit_kind_from_token("&QUEST"), QuestLimitKind::Quest);
    EXPECT_EQ(quest_limit_kind_from_token("&SUBQUEST"), QuestLimitKind::SubQuest);
    EXPECT_EQ(quest_limit_kind_from_token("&STAGE"), QuestLimitKind::Stage);
    EXPECT_EQ(quest_limit_kind_from_token("&ATTR"), QuestLimitKind::Attr);
}

TEST(QuestLimitKindFromToken, RejectsUnknownTokens) {
    EXPECT_FALSE(quest_limit_kind_from_token("&RANK").has_value());
    EXPECT_FALSE(quest_limit_kind_from_token("LEVEL").has_value());
    EXPECT_FALSE(quest_limit_kind_from_token("").has_value());
}

TEST(ParseQuestScriptLine, RejectsEmptyAndMissingEvent) {
    EXPECT_FALSE(parse_quest_script_line("", 1u, 0u).has_value());
    EXPECT_FALSE(parse_quest_script_line("&LEVEL 1 99", 1u, 0u).has_value());
    EXPECT_FALSE(parse_quest_script_line("garbage", 1u, 0u).has_value());
}

TEST(ParseQuestScriptLine, ParsesSingleLimitEventAndExecute) {
    const auto line = parse_quest_script_line(
        "&LEVEL 1 99 @HUNT 1 10 *ADDCOUNT 1 5", 7u, 3u);
    ASSERT_TRUE(line.has_value());
    EXPECT_EQ(line->quest_idx, 7u);
    EXPECT_EQ(line->subquest_idx, 3u);
    ASSERT_EQ(line->limits.size(), 1u);
    EXPECT_EQ(line->limits[0].kind, QuestLimitKind::Level);
    EXPECT_EQ(line->limits[0].value1, 1u);
    EXPECT_EQ(line->limits[0].value2, 99u);
    EXPECT_EQ(line->event.kind, QuestEventKind::Hunt);
    EXPECT_EQ(line->event.param1, 1u);
    EXPECT_EQ(line->event.param2, 10);
    ASSERT_EQ(line->executes.size(), 1u);
}

TEST(ParseQuestScriptLine, ParsesMultipleLimitsAndExecutes) {
    const auto line = parse_quest_script_line(
        "&LEVEL 1 99 &MONEY 1000 5000 &QUEST 1 0 @HUNT 1 10 "
        "*ADDCOUNT 1 5 *ENDSUB *GIVEMONEY 500",
        12u, 4u);
    ASSERT_TRUE(line.has_value());
    EXPECT_EQ(line->limits.size(), 3u);
    EXPECT_EQ(line->limits[0].kind, QuestLimitKind::Level);
    EXPECT_EQ(line->limits[1].kind, QuestLimitKind::Money);
    EXPECT_EQ(line->limits[1].value1, 1000u);
    EXPECT_EQ(line->limits[1].value2, 5000u);
    EXPECT_EQ(line->limits[2].kind, QuestLimitKind::Quest);
    EXPECT_EQ(line->event.kind, QuestEventKind::Hunt);
    ASSERT_EQ(line->executes.size(), 3u);
}

TEST(ParseQuestScriptLine, PreservesHuntAllEvent) {
    const auto line = parse_quest_script_line(
        "&LEVEL 1 99 @HUNTALL 0 0 *ENDSUB", 1u, 0u);
    ASSERT_TRUE(line.has_value());
    EXPECT_EQ(line->event.kind, QuestEventKind::HuntAll);
    EXPECT_EQ(line->event.param1, 0u);
    EXPECT_EQ(line->event.param2, 0);
}

TEST(ParseQuestScriptLine, AcceptsTabAndCrlfWhitespace) {
    const auto line = parse_quest_script_line(
        "&LEVEL\t1\t99\r\n@HUNT\t1\t10\r\n*ADDCOUNT\t1\t5",
        1u, 0u);
    ASSERT_TRUE(line.has_value());
    EXPECT_EQ(line->event.kind, QuestEventKind::Hunt);
    ASSERT_EQ(line->executes.size(), 1u);
}

TEST(ParseQuestScriptLine, AcceptsNegativeParamTwo) {
    const auto line = parse_quest_script_line(
        "&LEVEL 1 99 @COUNT 1 -5 *ADDCOUNT 1 5", 1u, 0u);
    ASSERT_TRUE(line.has_value());
    EXPECT_EQ(line->event.param1, 1u);
    EXPECT_EQ(line->event.param2, -5);
}

TEST(ParseQuestScriptLine, RejectsRepeatedEventAndMalformed) {
    EXPECT_FALSE(parse_quest_script_line(
        "&LEVEL 1 99 @HUNT 1 10 @COUNT 2 3 *ADDCOUNT 1 1", 1u, 0u).has_value());
    EXPECT_FALSE(parse_quest_script_line(
        "&LEVEL 1 @HUNT 1 10 *ADDCOUNT 1 1", 1u, 0u).has_value());
    EXPECT_FALSE(parse_quest_script_line(
        "&LEVEL 1 99 @HUNT 1 *ADDCOUNT 1 1", 1u, 0u).has_value());
    EXPECT_FALSE(parse_quest_script_line(
        "&UNKNOWN 1 99 @HUNT 1 10 *ADDCOUNT 1 1", 1u, 0u).has_value());
    EXPECT_FALSE(parse_quest_script_line(
        "@UNKNOWN 1 10 *ADDCOUNT 1 1", 1u, 0u).has_value());
    EXPECT_FALSE(parse_quest_script_line(
        "&LEVEL 1 99 @HUNT 1 10 *BOGUS a b", 1u, 0u).has_value());
}

TEST(QuestEventMatches, HuntAllMatchesAnyHunt) {
    const mxh::server::QuestEventSpec condition{
        QuestEventKind::HuntAll, 0u, 0};
    const mxh::server::QuestEventSpec runtime{
        QuestEventKind::Hunt, 999u, 999};
    EXPECT_TRUE(quest_event_matches(condition, runtime));
}

TEST(QuestEventMatches, HuntAllRejectsNonHuntKinds) {
    const mxh::server::QuestEventSpec condition{
        QuestEventKind::HuntAll, 0u, 0};
    EXPECT_FALSE(quest_event_matches(condition,
        {QuestEventKind::Count, 0u, 0}));
    EXPECT_FALSE(quest_event_matches(condition,
        {QuestEventKind::NpcTalk, 0u, 0}));
    EXPECT_FALSE(quest_event_matches(condition,
        {QuestEventKind::Die, 0u, 0}));
}

TEST(QuestEventMatches, ExactMatchForNonHuntAllKinds) {
    const mxh::server::QuestEventSpec cond{QuestEventKind::Hunt, 5u, 10};
    EXPECT_TRUE(quest_event_matches(cond, {QuestEventKind::Hunt, 5u, 10}));
    EXPECT_FALSE(quest_event_matches(cond, {QuestEventKind::Hunt, 6u, 10}));
    EXPECT_FALSE(quest_event_matches(cond, {QuestEventKind::Hunt, 5u, 11}));
    EXPECT_FALSE(quest_event_matches(cond, {QuestEventKind::Count, 5u, 10}));
    const mxh::server::QuestEventSpec lvl{QuestEventKind::Level, 1u, 99};
    EXPECT_TRUE(quest_event_matches(lvl, {QuestEventKind::Level, 1u, 99}));
    EXPECT_FALSE(quest_event_matches(lvl, {QuestEventKind::Level, 2u, 99}));
}
