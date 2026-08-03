#include "mxh/server/quest_subquest_block.hpp"

#include <gtest/gtest.h>

namespace {
using mxh::server::QuestEventKind;
using mxh::server::QuestLimitKind;
using mxh::server::parse_quest_subquest_block;
using mxh::server::parse_quest_subquest_limit_line;
using mxh::server::parse_quest_subquest_stanza;
}

TEST(QuestSubquestLimitLine, ParsesSingleLimit) {
    const auto limits = parse_quest_subquest_limit_line(
        "&LEVEL 1 99");
    ASSERT_TRUE(limits.has_value());
    ASSERT_EQ(limits->size(), 1u);
    EXPECT_EQ((*limits)[0].kind, QuestLimitKind::Level);
    EXPECT_EQ((*limits)[0].value1, 1u);
    EXPECT_EQ((*limits)[0].value2, 99u);
}

TEST(QuestSubquestLimitLine, ParsesMultipleLimits) {
    const auto limits = parse_quest_subquest_limit_line(
        "&LEVEL 1 99 &MONEY 100 500 &QUEST 7 2");
    ASSERT_TRUE(limits.has_value());
    ASSERT_EQ(limits->size(), 3u);
    EXPECT_EQ((*limits)[1].kind, QuestLimitKind::Money);
    EXPECT_EQ((*limits)[1].value1, 100u);
    EXPECT_EQ((*limits)[1].value2, 500u);
    EXPECT_EQ((*limits)[2].kind, QuestLimitKind::Quest);
}

TEST(QuestSubquestLimitLine, RejectsEventAndExecuteTokens) {
    EXPECT_FALSE(parse_quest_subquest_limit_line(
        "&LEVEL 1 99 @HUNT 1 10").has_value());
    EXPECT_FALSE(parse_quest_subquest_limit_line(
        "&LEVEL 1 99 *ENDSUB").has_value());
}

TEST(QuestSubquestLimitLine, RejectsUnknownAndMalformedClauses) {
    EXPECT_FALSE(parse_quest_subquest_limit_line("").has_value());
    EXPECT_FALSE(parse_quest_subquest_limit_line("&RANK 1 2").has_value());
    EXPECT_FALSE(parse_quest_subquest_limit_line("&LEVEL 1").has_value());
    EXPECT_FALSE(parse_quest_subquest_limit_line("&LEVEL one 99").has_value());
}

TEST(QuestSubquestBlock, ParsesLimitDirectivesByLine) {
    const auto entry = parse_quest_subquest_block(
        "#LIMIT &LEVEL 1 99\n#LIMIT &MONEY 100 500", 7u, 3u);
    ASSERT_TRUE(entry.has_value());
    EXPECT_EQ(entry->subquest_idx, 3u);
    ASSERT_EQ(entry->limits.size(), 2u);
    EXPECT_TRUE(entry->triggers.empty());
}

TEST(QuestSubquestBlock, ParsesTriggerDirective) {
    const auto entry = parse_quest_subquest_block(
        "#TRIGGER @HUNT 1 10 *ADDCOUNT 1 5", 7u, 3u);
    ASSERT_TRUE(entry.has_value());
    ASSERT_EQ(entry->triggers.size(), 1u);
    EXPECT_EQ(entry->triggers[0].quest_idx, 7u);
    EXPECT_EQ(entry->triggers[0].subquest_idx, 3u);
    EXPECT_EQ(entry->triggers[0].event.kind, QuestEventKind::Hunt);
}

TEST(QuestSubquestBlock, ParsesMixedDirectivesAndCrlf) {
    const auto entry = parse_quest_subquest_block(
        "#LIMIT &LEVEL 1 99\r\n"
        "#TRIGGER &MONEY 100 500 @COUNT 2 -3 *ADDCOUNT 2 1\r\n",
        12u, 4u);
    ASSERT_TRUE(entry.has_value());
    ASSERT_EQ(entry->limits.size(), 1u);
    ASSERT_EQ(entry->triggers.size(), 1u);
    ASSERT_EQ(entry->triggers[0].limits.size(), 1u);
    EXPECT_EQ(entry->triggers[0].event.kind, QuestEventKind::Count);
}

TEST(QuestSubquestBlock, RejectsUnknownAndMalformedDirectives) {
    EXPECT_FALSE(parse_quest_subquest_block(
        "#NPCSCRIPT @NPC 1", 1u, 0u).has_value());
    EXPECT_FALSE(parse_quest_subquest_block(
        "#LIMIT", 1u, 0u).has_value());
    EXPECT_FALSE(parse_quest_subquest_block(
        "#TRIGGER @UNKNOWN 1 2 *ENDSUB", 1u, 0u).has_value());
}

TEST(QuestSubquestStanza, ParsesHeaderBodyAndClosingBrace) {
    const auto entry = parse_quest_subquest_stanza(
        "$SUBQUEST 6 {\n"
        "#LIMIT &LEVEL 1 99\n"
        "#TRIGGER @HUNTALL 0 0 *ENDSUB\n"
        "}",
        20u);
    ASSERT_TRUE(entry.has_value());
    EXPECT_EQ(entry->subquest_idx, 6u);
    ASSERT_EQ(entry->triggers.size(), 1u);
    EXPECT_EQ(entry->triggers[0].quest_idx, 20u);
    EXPECT_EQ(entry->triggers[0].subquest_idx, 6u);
}

TEST(QuestSubquestStanza, AcceptsOpeningBraceOnNextLine) {
    const auto entry = parse_quest_subquest_stanza(
        "$SUBQUEST 2\r\n{\r\n#LIMIT &STAGE 1 3\r\n}", 9u);
    ASSERT_TRUE(entry.has_value());
    EXPECT_EQ(entry->subquest_idx, 2u);
    ASSERT_EQ(entry->limits.size(), 1u);
    EXPECT_EQ(entry->limits[0].kind, QuestLimitKind::Stage);
}

TEST(QuestSubquestStanza, RejectsMalformedHeaders) {
    EXPECT_FALSE(parse_quest_subquest_stanza(
        "SUBQUEST 1 { }", 1u).has_value());
    EXPECT_FALSE(parse_quest_subquest_stanza(
        "$SUBQUEST nope { }", 1u).has_value());
    EXPECT_FALSE(parse_quest_subquest_stanza(
        "$SUBQUEST 1 #LIMIT &LEVEL 1 2 }", 1u).has_value());
}

TEST(QuestSubquestStanza, RejectsMissingClosingBrace) {
    EXPECT_FALSE(parse_quest_subquest_stanza(
        "$SUBQUEST 1 { #LIMIT &LEVEL 1 2", 1u).has_value());
}
