//
// agent_battle_side_effect_plan_test.cpp -- D4.121
//
// 1:1 lock the legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_BATTLE handling. MP_BATTLE is pass-through to map server (battle state lives
// on map). No state mutation on agent side; only raw forward.
//

#include <gtest/gtest.h>

#include "mxh/server/agent_battle.hpp"
#include "mxh/server/agent_battle_side_effect_plan.hpp"

using namespace mxh::server;

//
// agent_battle_side_effect_plan_test.cpp -- D4.121
//
// 1:1 lock the legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_BATTLE handling. MP_BATTLE is pass-through to map server (battle state lives
// on map). No state mutation on agent side; only raw forward.
//

#include <gtest/gtest.h>

#include "mxh/server/agent_battle.hpp"
#include "mxh/server/agent_battle_side_effect_plan.hpp"

using namespace mxh::server;

// ---------------------- plan-builder from BattleAction ----------------------

TEST(BattlePlan, ForwardToMapEmitsForwardEffect) {
    BattleAction a{};
    a.kind = BattleActionKind::forward_to_map;
    a.protocol = battle_info;
    a.object_id = 42u;
    const auto plan = agent_battle_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentBattleSideEffectKind::ForwardRawToMap);
    EXPECT_EQ(plan.effects[0].protocol, battle_info);
    EXPECT_EQ(plan.effects[0].object_id, 42u);
    EXPECT_TRUE(agent_battle_effect_targets_map(plan.effects[0]));
}

TEST(BattlePlan, DropProtocolEmitsDrop) {
    BattleAction a{};
    a.kind = BattleActionKind::drop_protocol;
    a.protocol = 99u;
    a.object_id = 42u;
    const auto plan = agent_battle_side_effect_plan(a);
    EXPECT_FALSE(plan.dispatched);
    EXPECT_TRUE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentBattleSideEffectKind::Drop);
    EXPECT_EQ(plan.effects[0].protocol, 99u);
    EXPECT_FALSE(agent_battle_effect_targets_map(plan.effects[0]));
}

// ---------------------- classify-style plan-builders ----------------------

TEST(BattleClassifyPlan, BattleInfoAlwaysForwards) {
    BattleRequest r{};
    r.protocol = battle_info;
    r.object_id = 100u;
    const auto plan = agent_battle_user_side_effect_plan(r);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentBattleSideEffectKind::ForwardRawToMap);
    EXPECT_EQ(plan.effects[0].protocol, battle_info);
}

TEST(BattleClassifyPlan, BattleChatTeamSynForwards) {
    BattleRequest r{};
    r.protocol = battle_chat_team_syn;
    const auto plan = agent_battle_user_side_effect_plan(r);
    EXPECT_EQ(plan.effects[0].kind, AgentBattleSideEffectKind::ForwardRawToMap);
}

TEST(BattleClassifyPlan, BattleStartNotifyForwards) {
    BattleRequest r{};
    r.protocol = battle_start_notify;
    const auto plan = agent_battle_user_side_effect_plan(r);
    EXPECT_EQ(plan.effects[0].kind, AgentBattleSideEffectKind::ForwardRawToMap);
}
TEST(BattleClassifyPlan, BattleVimuWaitingCancelNackForwards) {
    // Last sub-protocol (30) in MP_PROTOCOL_BATTLE.
    BattleRequest r{};
    r.protocol = battle_vimu_waiting_cancel_nack;
    const auto plan = agent_battle_user_side_effect_plan(r);
    EXPECT_EQ(plan.effects[0].kind, AgentBattleSideEffectKind::ForwardRawToMap);
}

TEST(BattleClassifyPlan, AllProtocolsForward) {
    // 1:1 mirror of legacy: every MP_BATTLE sub-protocol routes to map.
    for (std::uint8_t p = 0; p <= 30; ++p) {
        BattleRequest r{};
        r.protocol = p;
        const auto plan = agent_battle_user_side_effect_plan(r);
        ASSERT_EQ(plan.effects.size(), 1u) << std::string("protocol ") + std::to_string(p);
        EXPECT_EQ(plan.effects[0].kind, AgentBattleSideEffectKind::ForwardRawToMap) << std::string("protocol ") + std::to_string(p);
    }
}

TEST(BattleClassifyPlan, ZeroObjectIdForwards) {
    // Edge: object_id=0 (legacy allows forward with no object).
    BattleRequest r{};
    r.protocol = battle_result;
    const auto plan = agent_battle_user_side_effect_plan(r);
    EXPECT_EQ(plan.effects[0].kind, AgentBattleSideEffectKind::ForwardRawToMap);
    EXPECT_EQ(plan.effects[0].object_id, 0u);
}

// ---------------------- apply: state mutation (none) ----------------------

TEST(BattleApplyPlan, ForwardPlanReturnsTrue) {
    BattleAction a{};
    a.kind = BattleActionKind::forward_to_map;
    a.protocol = battle_info;
    const auto plan = agent_battle_side_effect_plan(a);
    EXPECT_TRUE(apply_agent_battle_side_effect_plan(plan));
}

TEST(BattleApplyPlan, DropPlanReturnsFalse) {
    BattleAction a{};
    a.kind = BattleActionKind::drop_protocol;
    const auto plan = agent_battle_side_effect_plan(a);
    EXPECT_FALSE(apply_agent_battle_side_effect_plan(plan));
}

TEST(BattleApplyPlan, EmptyEffectsPlanReturnsFalse) {
    AgentBattleSideEffectPlan plan;
    plan.dispatched = true;
    plan.drop = false;
    EXPECT_FALSE(apply_agent_battle_side_effect_plan(plan));
}

TEST(BattleApplyPlan, MultiEffectForwardPlanReturnsTrue) {
    AgentBattleSideEffectPlan plan;
    plan.dispatched = true;
    plan.drop = false;
    plan.effects.push_back({AgentBattleSideEffectKind::ForwardRawToMap, battle_info, 1u});
    plan.effects.push_back({AgentBattleSideEffectKind::ForwardRawToMap, battle_result, 2u});
    EXPECT_TRUE(apply_agent_battle_side_effect_plan(plan));
}

// ---------------------- 1:1 mirror with classify_battle ----------------------

TEST(BattleApplyPlan, UserPlanMirrorsClassifyBattle) {
    BattleRequest r{};
    r.protocol = battle_battleobject_create_notify;
    r.object_id = 123u;
    const auto a = classify_battle(r);
    const auto plan = agent_battle_user_side_effect_plan(r);
    EXPECT_EQ(a.kind, BattleActionKind::forward_to_map);
    EXPECT_EQ(plan.effects[0].kind, AgentBattleSideEffectKind::ForwardRawToMap);
    EXPECT_EQ(plan.effects[0].protocol, a.protocol);
    EXPECT_EQ(plan.effects[0].object_id, a.object_id);
}
TEST(BattleApplyPlan, UserPlanMirrorsClassifyBattleDefaultProtocol) {
    // Protocol not in the named MP_PROTOCOL_BATTLE enum (e.g. 17 = battle_change_objectbattle)
    BattleRequest r{};
    r.protocol = battle_change_objectbattle;
    r.object_id = 999u;
    const auto a = classify_battle(r);
    const auto plan = agent_battle_user_side_effect_plan(r);
    EXPECT_EQ(a.kind, BattleActionKind::forward_to_map);
    EXPECT_EQ(plan.effects[0].kind, AgentBattleSideEffectKind::ForwardRawToMap);
    EXPECT_EQ(plan.effects[0].protocol, battle_change_objectbattle);
    EXPECT_EQ(plan.effects[0].object_id, 999u);
}

// ---------------------- predicate coverage ----------------------

TEST(BattlePlanPredicates, TargetsMapOnlyForward) {
    AgentBattleSideEffect fwd{AgentBattleSideEffectKind::ForwardRawToMap, 0u, 0u};
    AgentBattleSideEffect drop{AgentBattleSideEffectKind::Drop, 0u, 0u};
    EXPECT_TRUE(agent_battle_effect_targets_map(fwd));
    EXPECT_FALSE(agent_battle_effect_targets_map(drop));
}

// ---------------------- 1:1 lock: full sequence ----------------------

TEST(BattleApplyPlan, FullSequenceAlwaysForwards) {
    // Legacy invariant: agent never gates MP_BATTLE on any condition.
    // Verify across a representative sequence of sub-protocols.
    const std::uint8_t projs[] = {battle_info, battle_chat_team_syn, battle_chat_master_syn,
                               battle_start_notify, battle_teammember_add_notify,
                               battle_victory_notify, battle_destroy_notify,
                               battle_vimu_request_syn, battle_vimu_apply_syn,
                               battle_vimu_waiting_cancel_nack};
    for (auto p : projs) {
        BattleRequest r{};
        r.protocol = p;
        r.object_id = 1u;
        const auto plan = agent_battle_user_side_effect_plan(r);
        EXPECT_TRUE(plan.dispatched);
        EXPECT_FALSE(plan.drop);
        EXPECT_EQ(plan.effects[0].kind, AgentBattleSideEffectKind::ForwardRawToMap);
        EXPECT_EQ(plan.effects[0].protocol, p);
    }
}

TEST(BattleApplyPlan, BattleCategoryIs31) {
    // Lock the 1:1 mapping: MP_BATTLE = 31 in MP_CATEGORY.
    EXPECT_EQ(battle_category, 31u);
}

