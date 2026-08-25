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

TEST(QuestRuntimeAdapter, RestrictsRewardsAndCostsToEndQuestSubquest) {
    const auto parsed = mxh::server::parse_quest_script_text(
        "$QUEST 11 { $SUBQUEST 0 { #TRIGGER @TALKTONPC 1 1 *GIVEQUESTITEM 99 1 *STARTSUB 11 1 }"
        " $SUBQUEST 1 { #TRIGGER @TALKTONPC 2 1 *GIVEQUESTITEM 100 2 *TAKEMONEY 7000 *ENDQUEST 0 } }");
    ASSERT_EQ(parsed.quests.size(), 1u);
    const auto runtime = mxh::server::make_runtime_quest_definition(parsed.quests[0]);
    EXPECT_EQ(runtime.reward_item_idx, 100u);
    EXPECT_EQ(runtime.reward_item_qty, 2u);
    EXPECT_EQ(runtime.money_cost, 7000u);
}

TEST(QuestRuntimeAdapter, MapsNpcTalkToAuthoritativeTalkSubcondition) {
    const auto parsed = mxh::server::parse_quest_script_text(
        "$QUEST 9 { $SUBQUEST 0 { #TRIGGER @TALKTONPC 77 1 *ENDQUEST 0 } }");
    ASSERT_EQ(parsed.quests.size(), 1u);
    const auto runtime = mxh::server::make_runtime_quest_definition(parsed.quests[0]);
    ASSERT_EQ(runtime.subs.size(), 1u);
    EXPECT_EQ(runtime.subs[0].kind, mxh::server::QuestSubKind::TalkNpc);
    EXPECT_EQ(runtime.subs[0].target_id, 77u);
    EXPECT_EQ(runtime.subs[0].target, 1u);
}

TEST(QuestRuntimeAdapter, MapsUseItemToAuthoritativeCollectSubcondition) {
    const auto parsed = mxh::server::parse_quest_script_text(
        "$QUEST 10 { $SUBQUEST 0 { #TRIGGER @USEITEM 147 2 *ENDQUEST 0 } }");
    ASSERT_EQ(parsed.quests.size(), 1u);
    const auto runtime = mxh::server::make_runtime_quest_definition(parsed.quests[0]);
    ASSERT_EQ(runtime.subs.size(), 1u);
    EXPECT_EQ(runtime.subs[0].kind, mxh::server::QuestSubKind::Collect);
    EXPECT_EQ(runtime.subs[0].target_id, 147u);
    EXPECT_EQ(runtime.subs[0].target, 2u);
}

TEST(QuestRuntimeAdapter, DispatchesOnlyCurrentSubquestStage) {
    const auto parsed = mxh::server::parse_quest_script_text(
        "$QUEST 12 { $SUBQUEST 0 { #TRIGGER @TALKTONPC 10 1 *STARTSUB 12 1 }"
        " $SUBQUEST 1 { #TRIGGER @HUNT 99 2 *ENDQUEST 0 } }");
    ASSERT_EQ(parsed.quests.size(), 1u);
    const auto definition = mxh::server::make_runtime_quest_definition(parsed.quests[0]);
    ASSERT_EQ(definition.subs.size(), 2u);
    EXPECT_EQ(definition.subs[0].stage, 0u);
    EXPECT_EQ(definition.subs[1].stage, 1u);
    mxh::server::QuestLog log;
    ASSERT_TRUE(mxh::server::accept_quest(log, definition, 1u));

    const auto blocked_kill = mxh::server::dispatch_quest_event(
        log, {mxh::server::QuestSubKind::Kill, 99u, 2u});
    EXPECT_TRUE(blocked_kill.empty());
    const auto talked = mxh::server::dispatch_quest_event(
        log, {mxh::server::QuestSubKind::TalkNpc, 10u, 1u});
    ASSERT_EQ(talked.size(), 1u);
    EXPECT_EQ(talked[0].state, mxh::server::QuestState::Accepted);

    const auto first_kill = mxh::server::dispatch_quest_event(
        log, {mxh::server::QuestSubKind::Kill, 99u, 1u});
    ASSERT_EQ(first_kill.size(), 1u);
    EXPECT_EQ(first_kill[0].state, mxh::server::QuestState::Accepted);
    const auto second_kill = mxh::server::dispatch_quest_event(
        log, {mxh::server::QuestSubKind::Kill, 99u, 1u});
    ASSERT_EQ(second_kill.size(), 1u);
    EXPECT_EQ(second_kill[0].state, mxh::server::QuestState::Complete);
}
