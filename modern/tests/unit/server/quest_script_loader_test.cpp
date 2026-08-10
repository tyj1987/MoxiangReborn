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
