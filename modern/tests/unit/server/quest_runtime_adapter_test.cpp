#include "mxh/server/quest_runtime_adapter.hpp"
#include <gtest/gtest.h>
TEST(QuestRuntimeAdapter, MapsHuntAllAndParsedRewardsWithoutInventingValues) {
    const auto parsed = mxh::server::parse_quest_script_text(
        "$QUEST 7 { $SUBQUEST 1 { #TRIGGER @HUNTALL 0 3 *GIVEMONEY 50 *TAKEEXP 25 *GIVEITEM 8000 2 *ENDQUEST 1 } }");
    ASSERT_EQ(parsed.quests.size(), 1u);
    const auto runtime = mxh::server::make_runtime_quest_definition(parsed.quests[0]);
    ASSERT_EQ(runtime.subs.size(), 1u); EXPECT_EQ(runtime.subs[0].kind, mxh::server::QuestSubKind::Kill);
    EXPECT_EQ(runtime.subs[0].target_id, 0u); EXPECT_EQ(runtime.subs[0].target, 3u);
    EXPECT_EQ(runtime.reward_money, 50u); EXPECT_EQ(runtime.reward_exp, 25u);
    EXPECT_EQ(runtime.reward_item_idx, 8000u); EXPECT_EQ(runtime.reward_item_qty, 2u);
}
