#include "mxh/server/quest_script_loader.hpp"

#include <gtest/gtest.h>

TEST(QuestScriptLoader, ParsesSingleSubquestWithTrigger) {
    const char* text = "$QUEST 1 { $SUBQUEST 1 { #TRIGGER &LEVEL 1 99 @HUNT 1 10 *ADDCOUNT 1 5 } }";
    auto parsed = mxh::server::parse_quest_script_text(text);
    ASSERT_EQ(parsed.quests.size(), 1u);
    EXPECT_EQ(parsed.quests[0].quest_idx, 1u);
    ASSERT_EQ(parsed.quests[0].subquests.size(), 1u);
    EXPECT_EQ(parsed.quests[0].subquests[0].subquest_idx, 1u);
    EXPECT_EQ(parsed.quests[0].subquests[0].triggers.size(), 1u);
    EXPECT_FALSE(parsed.quests[0].end_param_set);
}

TEST(QuestScriptLoader, CapturesEndParamFromFirstEndQuest) {
    const char* text = "$QUEST 2 { $SUBQUEST 1 { #TRIGGER @HUNT 1 10 *ADDCOUNT 1 1 *ENDQUEST 1 } $SUBQUEST 2 { #TRIGGER @HUNT 1 10 *ADDCOUNT 1 1 } }";
    auto parsed = mxh::server::parse_quest_script_text(text);
    ASSERT_EQ(parsed.quests.size(), 1u);
    EXPECT_TRUE(parsed.quests[0].end_param_set);
    EXPECT_EQ(parsed.quests[0].end_param, 1u);
}

TEST(QuestScriptLoader, RejectsMalformedStanza) {
    const char* text = "QUEST 1 { $SUBQUEST 1 { #TRIGGER @HUNT 1 10 } }";
    auto parsed = mxh::server::parse_quest_script_text(text);
    EXPECT_EQ(parsed.quests.size(), 0u);
    EXPECT_EQ(parsed.parse_errors, 1u);
}

TEST(QuestScriptLoader, ParsesMultiLineRealFileFormat) {
    // The shipped QuestScript.bin stanzas span multiple lines with tab
    // indentation; the loader must accumulate until the brace depth closes.
    const char* text =
        "$QUEST\t1\r\n"
        "{\r\n"
        "\t$SUBQUEST\t0\r\n"
        "\t{\r\n"
        "\t\t#LIMIT\t\t&LEVEL 1 99\r\n"
        "\t\t#TRIGGER\t@TALKTONPC 77 1\t\t*TAKEQUESTITEM 147 1 10000\t*STARTSUB 1 1\r\n"
        "\t}\r\n"
        "\t$SUBQUEST\t1\r\n"
        "\t{\r\n"
        "\t\t#TRIGGER\t@HUNT 1 0\t\t*TAKEQUESTITEM 121 1 2500\t*ENDQUEST 1\r\n"
        "\t}\r\n"
        "}\r\n";
    auto parsed = mxh::server::parse_quest_script_text(text);
    ASSERT_EQ(parsed.quests.size(), 1u);
    EXPECT_EQ(parsed.quests[0].quest_idx, 1u);
    ASSERT_EQ(parsed.quests[0].subquests.size(), 2u);
    EXPECT_EQ(parsed.quests[0].subquests[0].subquest_idx, 0u);
    EXPECT_EQ(parsed.quests[0].subquests[1].subquest_idx, 1u);
    EXPECT_TRUE(parsed.quests[0].end_param_set);
    EXPECT_EQ(parsed.quests[0].end_param, 1u);
    EXPECT_EQ(parsed.parse_errors, 0u);
}
