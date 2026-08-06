//
// agent_quest_side_effect_plan_test.cpp -- D4.123
//
// 1:1 lock the legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_QUEST handling. MP_QUEST is pass-through to map server (quest state lives on map).
//

#include <gtest/gtest.h>

#include "mxh/server/agent_quest.hpp"
#include "mxh/server/agent_quest_side_effect_plan.hpp"

using namespace mxh::server;

//
// agent_quest_side_effect_plan_test.cpp -- D4.123
//
// 1:1 lock the legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_QUEST handling. MP_QUEST is pass-through to map server (quest state lives on map).
//

#include <gtest/gtest.h>

#include "mxh/server/agent_quest.hpp"
#include "mxh/server/agent_quest_side_effect_plan.hpp"

using namespace mxh::server;

// ---------------------- plan-builder from QuestAction ----------------------

TEST(QuestPlan, ForwardToMapEmitsForwardEffect) {
    QuestAction a{};
    a.kind = QuestActionKind::forward_to_map;
    a.protocol = quest_totalinfo;
    a.object_id = 42u;
    const auto plan = agent_quest_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentQuestSideEffectKind::ForwardRawToMap);
    EXPECT_EQ(plan.effects[0].protocol, quest_totalinfo);
    EXPECT_EQ(plan.effects[0].object_id, 42u);
    EXPECT_TRUE(agent_quest_effect_targets_map(plan.effects[0]));
}

TEST(QuestPlan, SendNackNoQuestEmitsDrop) {
    QuestAction a{};
    a.kind = QuestActionKind::send_nack_no_quest;
    a.protocol = 99u;
    a.object_id = 42u;
    a.error_code = 7u;
    const auto plan = agent_quest_side_effect_plan(a);
    EXPECT_FALSE(plan.dispatched);
    EXPECT_TRUE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentQuestSideEffectKind::Drop);
    EXPECT_EQ(plan.effects[0].error_code, 7u);
    EXPECT_FALSE(agent_quest_effect_targets_map(plan.effects[0]));
}

// ---------------------- classify-style plan-builders ----------------------

TEST(QuestClassifyPlan, QuestTotalInfoForwards) {
    QuestRequest r{};
    r.protocol = quest_totalinfo;
    r.object_id = 100u;
    const auto plan = agent_quest_user_side_effect_plan(r);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, AgentQuestSideEffectKind::ForwardRawToMap);
}

TEST(QuestClassifyPlan, QuestStartSynForwards) {
    QuestRequest r{};
    r.protocol = quest_start_syn;
    const auto plan = agent_quest_user_side_effect_plan(r);
    EXPECT_EQ(plan.effects[0].kind, AgentQuestSideEffectKind::ForwardRawToMap);
}

TEST(QuestClassifyPlan, QuestEndNackForwards) {
    QuestRequest r{};
    r.protocol = quest_end_nack;
    const auto plan = agent_quest_user_side_effect_plan(r);
    EXPECT_EQ(plan.effects[0].kind, AgentQuestSideEffectKind::ForwardRawToMap);
}

TEST(QuestClassifyPlan, QuestFullForwards) {
    // Last sub-protocol (27) in MP_PROTOCOL_QUEST.
    QuestRequest r{};
    r.protocol = quest_full;
    const auto plan = agent_quest_user_side_effect_plan(r);
    EXPECT_EQ(plan.effects[0].kind, AgentQuestSideEffectKind::ForwardRawToMap);
}
TEST(QuestClassifyPlan, AllProtocolsForward) {
    // 1:1 mirror of legacy: every MP_QUEST sub-protocol routes to map.
    for (std::uint8_t p = 0; p <= 27; ++p) {
        QuestRequest r{};
        r.protocol = p;
        const auto plan = agent_quest_user_side_effect_plan(r);
        ASSERT_EQ(plan.effects.size(), 1u) << std::string("protocol ") + std::to_string(p);
        EXPECT_EQ(plan.effects[0].kind, AgentQuestSideEffectKind::ForwardRawToMap) << std::string("protocol ") + std::to_string(p);
    }
}

TEST(QuestClassifyPlan, UserWithoutQuestForwards) {
    // Edge: user_has_quest=false (legacy does not gate MP_QUEST on this).
    QuestRequest r{};
    r.protocol = quest_totalinfo;
    r.user_has_quest = false;
    const auto plan = agent_quest_user_side_effect_plan(r);
    EXPECT_EQ(plan.effects[0].kind, AgentQuestSideEffectKind::ForwardRawToMap);
}

// ---------------------- apply: state mutation (none) ----------------------

TEST(QuestApplyPlan, ForwardPlanReturnsTrue) {
    QuestAction a{};
    a.kind = QuestActionKind::forward_to_map;
    const auto plan = agent_quest_side_effect_plan(a);
    EXPECT_TRUE(apply_agent_quest_side_effect_plan(plan));
}

TEST(QuestApplyPlan, DropPlanReturnsFalse) {
    QuestAction a{};
    a.kind = QuestActionKind::send_nack_no_quest;
    const auto plan = agent_quest_side_effect_plan(a);
    EXPECT_FALSE(apply_agent_quest_side_effect_plan(plan));
}

TEST(QuestApplyPlan, EmptyEffectsPlanReturnsFalse) {
    AgentQuestSideEffectPlan plan;
    plan.dispatched = true;
    plan.drop = false;
    EXPECT_FALSE(apply_agent_quest_side_effect_plan(plan));
}

TEST(QuestApplyPlan, MultiEffectForwardPlanReturnsTrue) {
    AgentQuestSideEffectPlan plan;
    plan.dispatched = true;
    plan.drop = false;
    plan.effects.push_back({AgentQuestSideEffectKind::ForwardRawToMap, quest_totalinfo, 1u, 0u});
    plan.effects.push_back({AgentQuestSideEffectKind::ForwardRawToMap, quest_end_syn, 2u, 0u});
    EXPECT_TRUE(apply_agent_quest_side_effect_plan(plan));
}

// ---------------------- 1:1 mirror with classify_quest ----------------------

TEST(QuestApplyPlan, UserPlanMirrorsClassifyQuest) {
    QuestRequest r{};
    r.protocol = quest_takeitem_ack;
    r.object_id = 999u;
    const auto a = classify_quest(r);
    const auto plan = agent_quest_user_side_effect_plan(r);
    EXPECT_EQ(a.kind, QuestActionKind::forward_to_map);
    EXPECT_EQ(plan.effects[0].kind, AgentQuestSideEffectKind::ForwardRawToMap);
    EXPECT_EQ(plan.effects[0].protocol, a.protocol);
    EXPECT_EQ(plan.effects[0].object_id, a.object_id);
}
// ---------------------- predicate coverage ----------------------

TEST(QuestPlanPredicates, TargetsMapOnlyForward) {
    AgentQuestSideEffect fwd{AgentQuestSideEffectKind::ForwardRawToMap, 0u, 0u, 0u};
    AgentQuestSideEffect drop{AgentQuestSideEffectKind::Drop, 0u, 0u, 0u};
    EXPECT_TRUE(agent_quest_effect_targets_map(fwd));
    EXPECT_FALSE(agent_quest_effect_targets_map(drop));
}

// ---------------------- 1:1 lock: full sequence ----------------------

TEST(QuestApplyPlan, FullLifecycleSequenceAlwaysForwards) {
    // Legacy invariant: agent never gates MP_QUEST on any condition.
    // Verify across a representative sequence of sub-protocols.
    const std::uint8_t projs[] = {quest_totalinfo, quest_changestate, quest_remove_notify,
                               quest_maindata_load, quest_subdata_load, quest_item_load,
                               quest_start_syn, quest_start_ack, quest_start_nack,
                               quest_end_syn, quest_end_ack, quest_end_nack,
                               quest_takeitem_ack, quest_takemoney_ack,
                               quest_giveitem_ack, quest_givemoney_ack,
                               quest_delete_confirm_syn, quest_delete_confirm_ack,
                               quest_regist_checktime, quest_unregist_checktime,
                               quest_time_limit, quest_execute_error, quest_full};
    for (auto p : projs) {
        QuestRequest r{};
        r.protocol = p;
        r.object_id = 1u;
        const auto plan = agent_quest_user_side_effect_plan(r);
        EXPECT_TRUE(plan.dispatched);
        EXPECT_FALSE(plan.drop);
        EXPECT_EQ(plan.effects[0].kind, AgentQuestSideEffectKind::ForwardRawToMap);
        EXPECT_EQ(plan.effects[0].protocol, p);
    }
}

TEST(QuestCategory, QuestCategoryIs38) {
    // Lock the 1:1 mapping: MP_QUEST = 38 in MP_CATEGORY.
    EXPECT_EQ(quest_category, 38u);
}
