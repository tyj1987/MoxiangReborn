#include "mxh/server/quest_runtime_adapter.hpp"
#include <array>
#include <filesystem>
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
    ASSERT_EQ(runtime.item_rewards.size(), 1u);
    EXPECT_EQ(runtime.item_rewards[0].item_idx, 100u);
    EXPECT_EQ(runtime.item_rewards[0].quantity, 2u);
    EXPECT_EQ(runtime.money_cost, 7000u);
}

TEST(QuestRuntimeAdapter, PreservesEveryFinalSubquestItemRewardInScriptOrder) {
    const auto parsed = mxh::server::parse_quest_script_text(
        "$QUEST 1 { $SUBQUEST 3 { #TRIGGER @TALKTONPC 71 1 "
        "*GIVEQUESTITEM 121 10 *GIVEQUESTITEM 148 1 *ENDQUEST 0 } }");
    ASSERT_EQ(parsed.quests.size(), 1u);
    const auto runtime = mxh::server::make_runtime_quest_definition(parsed.quests[0]);
    ASSERT_EQ(runtime.item_rewards.size(), 2u);
    EXPECT_EQ(runtime.item_rewards[0].item_idx, 121u);
    EXPECT_EQ(runtime.item_rewards[0].quantity, 10u);
    EXPECT_EQ(runtime.item_rewards[1].item_idx, 148u);
    EXPECT_EQ(runtime.item_rewards[1].quantity, 1u);
    EXPECT_EQ(runtime.reward_item_idx, 121u);
    EXPECT_EQ(runtime.reward_item_qty, 10u);
}

TEST(QuestRuntimeAdapter, ExtractsFinalSubquestItemCosts) {
    const auto parsed = mxh::server::parse_quest_script_text(
        "$QUEST 13 { $SUBQUEST 0 { #TRIGGER @TALKTONPC 1 1 *STARTSUB 13 1 }"
        " $SUBQUEST 1 { #TRIGGER @TALKTONPC 2 1 *TAKEQUESTITEM 147 2 10000 *ENDQUEST 0 } }");
    ASSERT_EQ(parsed.quests.size(), 1u);
    const auto runtime = mxh::server::make_runtime_quest_definition(parsed.quests[0]);
    ASSERT_EQ(runtime.item_costs.size(), 1u);
    EXPECT_EQ(runtime.item_costs[0].item_idx, 147u);
    EXPECT_EQ(runtime.item_costs[0].quantity, 2u);
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
        "$QUEST 10 { $SUBQUEST 0 { #TRIGGER @USEITEM 147 10 *ENDQUEST 0 } }");
    ASSERT_EQ(parsed.quests.size(), 1u);
    const auto runtime = mxh::server::make_runtime_quest_definition(parsed.quests[0]);
    ASSERT_EQ(runtime.subs.size(), 1u);
    EXPECT_EQ(runtime.subs[0].kind, mxh::server::QuestSubKind::Collect);
    EXPECT_EQ(runtime.subs[0].target_id, 147u);
    EXPECT_EQ(runtime.subs[0].target, 1u);
}

TEST(QuestRuntimeAdapter, TreatsTalkNpcQuestContextAsOneInteraction) {
    const auto parsed = mxh::server::parse_quest_script_text(
        "$QUEST 173 { $SUBQUEST 0 { #TRIGGER @TALKTONPC 38 173 *ENDQUEST 0 } }");
    ASSERT_EQ(parsed.quests.size(), 1u);
    const auto runtime = mxh::server::make_runtime_quest_definition(parsed.quests[0]);
    ASSERT_EQ(runtime.subs.size(), 1u);
    EXPECT_EQ(runtime.subs[0].target_id, 38u);
    EXPECT_EQ(runtime.subs[0].target, 1u);
}

TEST(QuestRuntimeAdapter, UsesCountTriggerForSingleHuntRequirement) {
    const auto parsed = mxh::server::parse_quest_script_text(
        "$QUEST 173 { $SUBQUEST 1 {\n#TRIGGER @HUNT 73 0\n"
        "#TRIGGER @COUNT 1 30 *ENDQUEST 0\n} }");
    ASSERT_EQ(parsed.quests.size(), 1u);
    const auto runtime = mxh::server::make_runtime_quest_definition(parsed.quests[0]);
    ASSERT_EQ(runtime.subs.size(), 1u);
    EXPECT_EQ(runtime.subs[0].target_id, 73u);
    EXPECT_EQ(runtime.subs[0].target, 30u);
}

TEST(QuestRuntimeAdapter, SharesCountAcrossMultipleHuntTargets) {
    const auto parsed = mxh::server::parse_quest_script_text(
        "$QUEST 200 { $SUBQUEST 2 {\n"
        "#TRIGGER @HUNT 70 0 *ADDCOUNT 2 5\n"
        "#TRIGGER @HUNT 71 0 *ADDCOUNT 2 5\n"
        "#TRIGGER @COUNT 2 5 *ENDQUEST 0\n} }");
    ASSERT_EQ(parsed.quests.size(), 1u);
    const auto runtime = mxh::server::make_runtime_quest_definition(parsed.quests[0]);
    ASSERT_EQ(runtime.subs.size(), 1u);
    EXPECT_EQ(runtime.subs[0].target, 5u);
    EXPECT_EQ(runtime.subs[0].accepted_target_ids,
              (std::vector<std::uint32_t>{70u, 71u}));

    mxh::server::QuestLog log;
    ASSERT_TRUE(mxh::server::accept_quest(log, runtime, 1u));
    for (std::uint32_t i = 0; i < 2u; ++i) {
        const auto changes = mxh::server::dispatch_quest_event(
            log, {mxh::server::QuestSubKind::Kill, 70u, 1u});
        ASSERT_EQ(changes.size(), 1u);
        EXPECT_EQ(changes[0].state, mxh::server::QuestState::Accepted);
    }
    for (std::uint32_t i = 0; i < 3u; ++i) {
        const auto changes = mxh::server::dispatch_quest_event(
            log, {mxh::server::QuestSubKind::Kill, 71u, 1u});
        ASSERT_EQ(changes.size(), 1u);
    }
    EXPECT_EQ(log.quests[0].subs[0].count, 5u);
    EXPECT_EQ(log.quests[0].state, mxh::server::QuestState::Complete);
    EXPECT_TRUE(mxh::server::dispatch_quest_event(
        log, {mxh::server::QuestSubKind::Kill, 72u, 1u}).empty());
}

TEST(QuestRuntimeAdapter, KeepsIndependentHuntCountersSeparate) {
    const auto parsed = mxh::server::parse_quest_script_text(
        "$QUEST 201 { $SUBQUEST 2 {\n"
        "#TRIGGER @HUNT 70 2 *ADDCOUNT 2 2\n"
        "#TRIGGER @HUNT 71 3 *ADDCOUNT 3 3\n"
        "#TRIGGER @COUNT 2 2\n"
        "#TRIGGER @COUNT 3 3 *ENDQUEST 0\n} }");
    ASSERT_EQ(parsed.quests.size(), 1u);
    const auto runtime = mxh::server::make_runtime_quest_definition(parsed.quests[0]);
    ASSERT_EQ(runtime.subs.size(), 2u);
    EXPECT_TRUE(runtime.subs[0].accepted_target_ids.empty());
    EXPECT_TRUE(runtime.subs[1].accepted_target_ids.empty());
    EXPECT_EQ(runtime.subs[0].target_id, 70u);
    EXPECT_EQ(runtime.subs[1].target_id, 71u);
}

TEST(QuestRuntimeAdapter, AppliesCanonicalFilteredAddCountConditions) {
    mxh::server::QuestScriptDefinition script;
    script.quest_idx = 202u;
    const std::array<std::string_view, 4> blocks{
        "#TRIGGER @HUNT 70 0 *ADDCOUNTFQW 0 2 9001\n#TRIGGER @COUNT 0 2",
        "#TRIGGER @HUNT 71 0 *ADDCOUNTFW 1 2 4\n#TRIGGER @COUNT 1 2",
        "#TRIGGER @HUNT 72 0 *ADDCOUNTLEVELGAP 2 2 3 2\n#TRIGGER @COUNT 2 2",
        "#TRIGGER @HUNT 73 0 *ADDCOUNTMONLEVEL 3 2 8 10\n"
        "#TRIGGER @COUNT 3 2 *ENDQUEST 0"};
    for (std::uint32_t index = 0; index < blocks.size(); ++index) {
        const auto subquest =
            mxh::server::parse_quest_subquest_block(blocks[index], 202u, index);
        ASSERT_TRUE(subquest.has_value()) << "subquest " << index;
        script.subquests.push_back(*subquest);
    }
    script.end_param = 3u;
    script.end_param_set = true;
    const auto runtime = mxh::server::make_runtime_quest_definition(script);
    ASSERT_EQ(runtime.subs.size(), 4u);
    EXPECT_EQ(runtime.subs[0].count_filter, mxh::server::QuestCountFilterKind::WeaponItem);
    EXPECT_EQ(runtime.subs[1].count_filter, mxh::server::QuestCountFilterKind::WeaponKind);
    EXPECT_EQ(runtime.subs[2].count_filter, mxh::server::QuestCountFilterKind::PlayerMonsterLevelGap);
    EXPECT_EQ(runtime.subs[3].count_filter, mxh::server::QuestCountFilterKind::MonsterLevel);

    mxh::server::QuestLog log;
    ASSERT_TRUE(mxh::server::accept_quest(log, runtime, 1u));
    EXPECT_TRUE(mxh::server::dispatch_quest_event(
        log, {mxh::server::QuestSubKind::Kill, 70u, 1u, 10u, 10u, 0u, 1u}).empty());
    EXPECT_EQ(mxh::server::dispatch_quest_event(
        log, {mxh::server::QuestSubKind::Kill, 70u, 1u, 10u, 10u, 0u, 9001u}).size(), 1u);
    EXPECT_EQ(mxh::server::dispatch_quest_event(
        log, {mxh::server::QuestSubKind::Kill, 70u, 1u, 10u, 10u, 0u, 9001u}).size(), 1u);
    EXPECT_TRUE(mxh::server::dispatch_quest_event(
        log, {mxh::server::QuestSubKind::Kill, 71u, 1u, 10u, 10u, 3u, 0u}).empty());
    EXPECT_EQ(mxh::server::dispatch_quest_event(
        log, {mxh::server::QuestSubKind::Kill, 71u, 2u, 10u, 10u, 4u, 0u}).size(), 1u);
    EXPECT_TRUE(mxh::server::dispatch_quest_event(
        log, {mxh::server::QuestSubKind::Kill, 72u, 1u, 20u, 10u}).empty());
    EXPECT_EQ(mxh::server::dispatch_quest_event(
        log, {mxh::server::QuestSubKind::Kill, 72u, 2u, 12u, 10u}).size(), 1u);
    EXPECT_TRUE(mxh::server::dispatch_quest_event(
        log, {mxh::server::QuestSubKind::Kill, 73u, 1u, 10u, 7u}).empty());
    const auto final = mxh::server::dispatch_quest_event(
        log, {mxh::server::QuestSubKind::Kill, 73u, 2u, 10u, 8u});
    ASSERT_EQ(final.size(), 1u);
    EXPECT_EQ(final[0].state, mxh::server::QuestState::Complete);
}

TEST(QuestRuntimeAdapter, MapsParsedLevelLimitIntoAcceptanceDefinition) {
    const auto parsed = mxh::server::parse_quest_script_text(
        "$QUEST 173 { $SUBQUEST 0 {\n#LIMIT &LEVEL 48 53\n"
        "#TRIGGER @TALKTONPC 38 173 *ENDQUEST 0\n} }");
    ASSERT_EQ(parsed.quests.size(), 1u);
    const auto runtime = mxh::server::make_runtime_quest_definition(parsed.quests[0]);
    EXPECT_EQ(runtime.min_level, 48u);
    EXPECT_EQ(runtime.max_level, 53u);
    mxh::server::QuestLog log;
    EXPECT_FALSE(mxh::server::accept_quest(log, runtime, 1u, 47u));
    EXPECT_TRUE(mxh::server::accept_quest(log, runtime, 1u, 48u));
}

TEST(QuestRuntimeAdapter, CanonicalQuest173KeepsLevelKillCountAndReward) {
    const auto parsed = mxh::server::load_quest_script(
        std::filesystem::path(MXH_SOURCE_DIR) / "data" / "PlayDH" /
        "Resource" / "QuestScript" / "QuestScript.bin");
    ASSERT_TRUE(parsed.error_message.empty()) << parsed.error_message;
    const auto* script = parsed.find_quest(173u);
    ASSERT_NE(script, nullptr);
    const auto runtime = mxh::server::make_runtime_quest_definition(*script);
    EXPECT_EQ(runtime.min_level, 48u);
    const auto hunt = std::find_if(runtime.subs.begin(), runtime.subs.end(),
        [](const auto& sub) {
            return sub.kind == mxh::server::QuestSubKind::Kill &&
                   sub.target_id == 73u;
        });
    ASSERT_NE(hunt, runtime.subs.end());
    EXPECT_EQ(hunt->target, 30u);
    ASSERT_EQ(runtime.item_rewards.size(), 1u);
    EXPECT_EQ(runtime.item_rewards[0].item_idx, 414u);
    EXPECT_EQ(runtime.item_rewards[0].quantity, 30u);
    EXPECT_EQ(runtime.reward_exp, 2000000u);
}

TEST(QuestRuntimeAdapter, CanonicalQuest1SharesTenKillsAcrossFiveMonsterKinds) {
    const auto parsed = mxh::server::load_quest_script(
        std::filesystem::path(MXH_SOURCE_DIR) / "data" / "PlayDH" /
        "Resource" / "QuestScript" / "QuestScript.bin");
    ASSERT_TRUE(parsed.error_message.empty()) << parsed.error_message;
    const auto* script = parsed.find_quest(1u);
    ASSERT_NE(script, nullptr);
    const auto runtime = mxh::server::make_runtime_quest_definition(*script);
    const auto shared = std::find_if(runtime.subs.begin(), runtime.subs.end(),
        [](const auto& sub) {
            return sub.stage == 2u &&
                   sub.kind == mxh::server::QuestSubKind::Kill;
        });
    ASSERT_NE(shared, runtime.subs.end());
    EXPECT_EQ(shared->target, 10u);
    EXPECT_EQ(shared->accepted_target_ids,
              (std::vector<std::uint32_t>{1u, 2u, 3u, 4u, 5u}));
    EXPECT_EQ(std::count_if(runtime.subs.begin(), runtime.subs.end(),
        [](const auto& sub) {
            return sub.stage == 2u &&
                   sub.kind == mxh::server::QuestSubKind::Kill;
        }), 1);
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
