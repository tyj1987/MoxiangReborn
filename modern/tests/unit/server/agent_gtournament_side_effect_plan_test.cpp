#pragma once

//
// agent_gtournament_side_effect_plan_test.cpp -- D4.124
//
// 1:1 lock the legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_GTOURNAMENTUserMsgParser (lines 4286-4295).
//
// Routing matrix (legacy classify_gtournament_user):
//   if (!user_found)                                  -> drop_no_user
//   movetobattlemap_syn + user_map_found              -> send_movetobattle_to_user_map
//   movetobattlemap_syn + !user_map_found            -> send_movetobattle_nack_to_user
//   standinginfo_syn + gt_map_found                  -> send_standing_info_to_gt_map (target=28)
//   standinginfo_syn + !gt_map_found                 -> send_standing_info_nack_to_user
//   battlejoin_syn/observerjoin_syn + gt_map_found   -> send_standing_info_to_gt_map (target=28)
//   battlejoin_syn/observerjoin_syn + !gt_map_found  -> send_battlejoin_nack_to_user
//   leave_syn                                        -> send_leave_syn_to_user_map
//   cheat + cheat_data==1                            -> send_cheat_to_user_map
//   cheat + cheat_data!=1 + gt_map_found            -> send_cheat_to_gt_map
//   cheat + cheat_data!=1 + !gt_map_found           -> drop_no_user
//   event_start/event_end + user_level>8             -> drop_no_user
//   event_start/event_end + user_level<=8 + gt_map   -> send_event_to_gt_map
//   event_start/event_end + !gt_map_found            -> drop_no_user
//   default                                          -> forward_to_map_server
//

#include <gtest/gtest.h>

#include "mxh/server/agent_gtournament.hpp"
#include "mxh/server/agent_gtournament_side_effect_plan.hpp"

using namespace mxh::server;

// ---------------------- plan-builder from GtournamentAction ----------------------

TEST(GtournamentPlan, ForwardToMapEmitsForwardEffect) {
    GtournamentAction a{};
    a.kind = GtournamentActionKind::forward_to_map_server;
    a.protocol = 200u;
    a.object_id = 42u;
    const auto plan = agent_gtournament_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentGtournamentSideEffectKind::ForwardRawToMap);
    EXPECT_EQ(plan.effects[0].protocol, 200u);
    EXPECT_EQ(plan.effects[0].object_id, 42u);
    EXPECT_TRUE(agent_gtournament_effect_targets_map(plan.effects[0]));
}

TEST(GtournamentPlan, SendMovetoBattleToUserMapTargetsUserMap) {
    GtournamentAction a{};
    a.kind = GtournamentActionKind::send_movetobattle_to_user_map;
    a.protocol = gtournament_movetobattlemap_syn;
    a.object_id = 7u;
    a.target_map = 0u;
    const auto plan = agent_gtournament_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, AgentGtournamentSideEffectKind::ForwardRawToMap);
    EXPECT_EQ(plan.effects[0].target_map, 0u);
}

TEST(GtournamentPlan, SendStandingInfoToGtMapTargets28) {
    GtournamentAction a{};
    a.kind = GtournamentActionKind::send_standing_info_to_gt_map;
    a.protocol = gtournament_standinginfo_syn;
    a.target_map = gt_map_num;
    const auto plan = agent_gtournament_side_effect_plan(a);
    EXPECT_EQ(plan.effects[0].kind, AgentGtournamentSideEffectKind::ForwardRawToMap);
    EXPECT_EQ(plan.effects[0].target_map, 28u);
}

TEST(GtournamentPlan, SendBattleJoinNackEmitsNackToUser) {
    GtournamentAction a{};
    a.kind = GtournamentActionKind::send_battlejoin_nack_to_user;
    a.protocol = gtournament_battlejoin_nack;
    a.object_id = 11u;
    a.error_code = gt_error_code_error;
    const auto plan = agent_gtournament_side_effect_plan(a);
    EXPECT_EQ(plan.effects[0].kind, AgentGtournamentSideEffectKind::SendNackToUser);
    EXPECT_EQ(plan.effects[0].error_code, 0u);
    EXPECT_TRUE(agent_gtournament_effect_targets_user(plan.effects[0]));
    EXPECT_FALSE(agent_gtournament_effect_targets_map(plan.effects[0]));
}

TEST(GtournamentPlan, SendStandingInfoNackEmitsNackToUser) {
    GtournamentAction a{};
    a.kind = GtournamentActionKind::send_standing_info_nack_to_user;
    a.protocol = gtournament_standinginfo_nack;
    a.error_code = 1u;
    const auto plan = agent_gtournament_side_effect_plan(a);
    EXPECT_EQ(plan.effects[0].kind, AgentGtournamentSideEffectKind::SendNackToUser);
    EXPECT_EQ(plan.effects[0].error_code, 1u);
    EXPECT_TRUE(agent_gtournament_effect_targets_user(plan.effects[0]));
}

TEST(GtournamentPlan, SendMoveToBattleNackEmitsNackToUser) {
    GtournamentAction a{};
    a.kind = GtournamentActionKind::send_movetobattle_nack_to_user;
    a.protocol = gtournament_movetobattlemap_nack;
    a.error_code = 2u;
    const auto plan = agent_gtournament_side_effect_plan(a);
    EXPECT_EQ(plan.effects[0].kind, AgentGtournamentSideEffectKind::SendNackToUser);
    EXPECT_EQ(plan.effects[0].error_code, 2u);
    EXPECT_TRUE(agent_gtournament_effect_targets_user(plan.effects[0]));
}

TEST(GtournamentPlan, SendLeaveSynToUserMapForwards) {
    GtournamentAction a{};
    a.kind = GtournamentActionKind::send_leave_syn_to_user_map;
    a.protocol = gtournament_leave_syn;
    a.target_map = 0u;
    const auto plan = agent_gtournament_side_effect_plan(a);
    EXPECT_EQ(plan.effects[0].kind, AgentGtournamentSideEffectKind::ForwardRawToMap);
    EXPECT_EQ(plan.effects[0].target_map, 0u);
}

TEST(GtournamentPlan, SendCheatToUserMapTargetsUserMap) {
    GtournamentAction a{};
    a.kind = GtournamentActionKind::send_cheat_to_user_map;
    a.protocol = gtournament_cheat;
    a.target_map = 0u;
    const auto plan = agent_gtournament_side_effect_plan(a);
    EXPECT_EQ(plan.effects[0].kind, AgentGtournamentSideEffectKind::ForwardRawToMap);
    EXPECT_EQ(plan.effects[0].target_map, 0u);
}

TEST(GtournamentPlan, SendCheatToGtMapTargets28) {
    GtournamentAction a{};
    a.kind = GtournamentActionKind::send_cheat_to_gt_map;
    a.protocol = gtournament_cheat;
    a.target_map = gt_map_num;
    const auto plan = agent_gtournament_side_effect_plan(a);
    EXPECT_EQ(plan.effects[0].kind, AgentGtournamentSideEffectKind::ForwardRawToMap);
    EXPECT_EQ(plan.effects[0].target_map, 28u);
}

TEST(GtournamentPlan, SendEventToGtMapTargets28) {
    GtournamentAction a{};
    a.kind = GtournamentActionKind::send_event_to_gt_map;
    a.protocol = gtournament_event_start;
    a.target_map = gt_map_num;
    const auto plan = agent_gtournament_side_effect_plan(a);
    EXPECT_EQ(plan.effects[0].kind, AgentGtournamentSideEffectKind::ForwardRawToMap);
    EXPECT_EQ(plan.effects[0].target_map, 28u);
}

TEST(GtournamentPlan, DropNoUserEmitsDrop) {
    GtournamentAction a{};
    a.kind = GtournamentActionKind::drop_no_user;
    a.protocol = 200u;
    const auto plan = agent_gtournament_side_effect_plan(a);
    EXPECT_FALSE(plan.dispatched);
    EXPECT_TRUE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, AgentGtournamentSideEffectKind::Drop);
    EXPECT_FALSE(agent_gtournament_effect_targets_map(plan.effects[0]));
    EXPECT_FALSE(agent_gtournament_effect_targets_user(plan.effects[0]));
}

TEST(GtournamentPlan, EveryActionKindProducesExactlyOneEffect) {
    // 1:1 invariant: plan-builder produces exactly 1 effect per action kind.
    const GtournamentActionKind kinds[] = {
        GtournamentActionKind::forward_to_map_server,
        GtournamentActionKind::send_movetobattle_to_user_map,
        GtournamentActionKind::send_standing_info_to_gt_map,
        GtournamentActionKind::send_battlejoin_nack_to_user,
        GtournamentActionKind::send_standing_info_nack_to_user,
        GtournamentActionKind::send_movetobattle_nack_to_user,
        GtournamentActionKind::send_leave_syn_to_user_map,
        GtournamentActionKind::send_cheat_to_user_map,
        GtournamentActionKind::send_cheat_to_gt_map,
        GtournamentActionKind::send_event_to_gt_map,
        GtournamentActionKind::drop_no_user,
    };
    for (auto k : kinds) {
        GtournamentAction a{};
        a.kind = k;
        const auto plan = agent_gtournament_side_effect_plan(a);
        EXPECT_EQ(plan.effects.size(), 1u) << (int)k;
    }
}

// ---------------------- classify-style plan-builders ----------------------

TEST(GtournamentClassifyPlan, NoUserFoundDrops) {
    GtournamentRequest r{};
    r.user_found = false;
    r.protocol = gtournament_movetobattlemap_syn;
    r.user_map_found = true;
    const auto plan = agent_gtournament_user_side_effect_plan(r);
    EXPECT_FALSE(plan.dispatched);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, AgentGtournamentSideEffectKind::Drop);
}

TEST(GtournamentClassifyPlan, NoUserFoundDropsForStandingInfo) {
    GtournamentRequest r{};
    r.user_found = false;
    r.protocol = gtournament_standinginfo_syn;
    r.gt_map_found = true;
    const auto plan = agent_gtournament_user_side_effect_plan(r);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, AgentGtournamentSideEffectKind::Drop);
}

TEST(GtournamentClassifyPlan, NoUserFoundDropsForCheat) {
    GtournamentRequest r{};
    r.user_found = false;
    r.protocol = gtournament_cheat;
    r.cheat_data = 1u;
    const auto plan = agent_gtournament_user_side_effect_plan(r);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, AgentGtournamentSideEffectKind::Drop);
}

TEST(GtournamentClassifyPlan, NoUserFoundDropsForEventStart) {
    GtournamentRequest r{};
    r.user_found = false;
    r.protocol = gtournament_event_start;
    const auto plan = agent_gtournament_user_side_effect_plan(r);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, AgentGtournamentSideEffectKind::Drop);
}

TEST(GtournamentClassifyPlan, MovetoBattleUserMapFoundForwards) {
    GtournamentRequest r{};
    r.protocol = gtournament_movetobattlemap_syn;
    r.user_map_found = true;
    const auto plan = agent_gtournament_user_side_effect_plan(r);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, AgentGtournamentSideEffectKind::ForwardRawToMap);
    EXPECT_EQ(plan.effects[0].protocol, gtournament_movetobattlemap_syn);
    EXPECT_EQ(plan.effects[0].target_map, 0u);
}

TEST(GtournamentClassifyPlan, MovetoBattleUserMapMissingNacks) {
    GtournamentRequest r{};
    r.protocol = gtournament_movetobattlemap_syn;
    r.user_map_found = false;
    const auto plan = agent_gtournament_user_side_effect_plan(r);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, AgentGtournamentSideEffectKind::SendNackToUser);
    EXPECT_EQ(plan.effects[0].protocol, gtournament_movetobattlemap_nack);
    EXPECT_EQ(plan.effects[0].error_code, gt_error_code_error);
}

TEST(GtournamentClassifyPlan, StandingInfoGtMapFoundForwardsTo28) {
    GtournamentRequest r{};
    r.protocol = gtournament_standinginfo_syn;
    r.gt_map_found = true;
    const auto plan = agent_gtournament_user_side_effect_plan(r);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, AgentGtournamentSideEffectKind::ForwardRawToMap);
    EXPECT_EQ(plan.effects[0].target_map, 28u);
}

TEST(GtournamentClassifyPlan, StandingInfoGtMapMissingNacks) {
    GtournamentRequest r{};
    r.protocol = gtournament_standinginfo_syn;
    r.gt_map_found = false;
    const auto plan = agent_gtournament_user_side_effect_plan(r);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, AgentGtournamentSideEffectKind::SendNackToUser);
    EXPECT_EQ(plan.effects[0].protocol, gtournament_standinginfo_nack);
}

TEST(GtournamentClassifyPlan, BattleJoinGtMapFoundForwardsTo28) {
    GtournamentRequest r{};
    r.protocol = gtournament_battlejoin_syn;
    r.gt_map_found = true;
    const auto plan = agent_gtournament_user_side_effect_plan(r);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_EQ(plan.effects[0].kind, AgentGtournamentSideEffectKind::ForwardRawToMap);
    EXPECT_EQ(plan.effects[0].protocol, gtournament_battlejoin_syn);
    EXPECT_EQ(plan.effects[0].target_map, 28u);
}

TEST(GtournamentClassifyPlan, BattleJoinGtMapMissingNacks) {
    GtournamentRequest r{};
    r.protocol = gtournament_battlejoin_syn;
    r.gt_map_found = false;
    const auto plan = agent_gtournament_user_side_effect_plan(r);
    EXPECT_EQ(plan.effects[0].kind, AgentGtournamentSideEffectKind::SendNackToUser);
    EXPECT_EQ(plan.effects[0].protocol, gtournament_battlejoin_nack);
}

TEST(GtournamentClassifyPlan, ObserverJoinGtMapFoundForwardsTo28) {
    GtournamentRequest r{};
    r.protocol = gtournament_observerjoin_syn;
    r.gt_map_found = true;
    const auto plan = agent_gtournament_user_side_effect_plan(r);
    EXPECT_EQ(plan.effects[0].kind, AgentGtournamentSideEffectKind::ForwardRawToMap);
    EXPECT_EQ(plan.effects[0].protocol, gtournament_observerjoin_syn);
    EXPECT_EQ(plan.effects[0].target_map, 28u);
}

TEST(GtournamentClassifyPlan, ObserverJoinGtMapMissingNacks) {
    GtournamentRequest r{};
    r.protocol = gtournament_observerjoin_syn;
    r.gt_map_found = false;
    const auto plan = agent_gtournament_user_side_effect_plan(r);
    EXPECT_EQ(plan.effects[0].kind, AgentGtournamentSideEffectKind::SendNackToUser);
    EXPECT_EQ(plan.effects[0].protocol, gtournament_battlejoin_nack);
}

TEST(GtournamentClassifyPlan, LeaveSynAlwaysForwards) {
    GtournamentRequest r{};
    r.protocol = gtournament_leave_syn;
    const auto plan = agent_gtournament_user_side_effect_plan(r);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, AgentGtournamentSideEffectKind::ForwardRawToMap);
    EXPECT_EQ(plan.effects[0].protocol, gtournament_leave_syn);
    EXPECT_EQ(plan.effects[0].target_map, 0u);
}

TEST(GtournamentClassifyPlan, CheatData1ForwardsToUserMap) {
    GtournamentRequest r{};
    r.protocol = gtournament_cheat;
    r.cheat_data = 1u;
    const auto plan = agent_gtournament_user_side_effect_plan(r);
    EXPECT_EQ(plan.effects[0].kind, AgentGtournamentSideEffectKind::ForwardRawToMap);
    EXPECT_EQ(plan.effects[0].target_map, 0u);
    EXPECT_EQ(plan.effects[0].protocol, gtournament_cheat);
}

TEST(GtournamentClassifyPlan, CheatDataNot1GtMapFoundForwardsTo28) {
    GtournamentRequest r{};
    r.protocol = gtournament_cheat;
    r.cheat_data = 2u;
    r.gt_map_found = true;
    const auto plan = agent_gtournament_user_side_effect_plan(r);
    EXPECT_EQ(plan.effects[0].kind, AgentGtournamentSideEffectKind::ForwardRawToMap);
    EXPECT_EQ(plan.effects[0].target_map, 28u);
}

TEST(GtournamentClassifyPlan, CheatDataNot1GtMapMissingDrops) {
    GtournamentRequest r{};
    r.protocol = gtournament_cheat;
    r.cheat_data = 0u;
    r.gt_map_found = false;
    const auto plan = agent_gtournament_user_side_effect_plan(r);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, AgentGtournamentSideEffectKind::Drop);
}

TEST(GtournamentClassifyPlan, EventStartLevel9Drops) {
    GtournamentRequest r{};
    r.protocol = gtournament_event_start;
    r.user_level = 9u;
    r.gt_map_found = true;
    const auto plan = agent_gtournament_user_side_effect_plan(r);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, AgentGtournamentSideEffectKind::Drop);
}

TEST(GtournamentClassifyPlan, EventStartLevel8ForwardsToGtMap) {
    GtournamentRequest r{};
    r.protocol = gtournament_event_start;
    r.user_level = 8u;
    r.gt_map_found = true;
    const auto plan = agent_gtournament_user_side_effect_plan(r);
    EXPECT_FALSE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, AgentGtournamentSideEffectKind::ForwardRawToMap);
    EXPECT_EQ(plan.effects[0].target_map, 28u);
}

TEST(GtournamentClassifyPlan, EventStartLevel8GtMapMissingDrops) {
    GtournamentRequest r{};
    r.protocol = gtournament_event_start;
    r.user_level = 8u;
    r.gt_map_found = false;
    const auto plan = agent_gtournament_user_side_effect_plan(r);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, AgentGtournamentSideEffectKind::Drop);
}

TEST(GtournamentClassifyPlan, EventEndLevel9Drops) {
    GtournamentRequest r{};
    r.protocol = gtournament_event_end;
    r.user_level = 9u;
    const auto plan = agent_gtournament_user_side_effect_plan(r);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, AgentGtournamentSideEffectKind::Drop);
}

TEST(GtournamentClassifyPlan, EventEndLevel0GtMapFoundForwards) {
    GtournamentRequest r{};
    r.protocol = gtournament_event_end;
    r.user_level = 0u;
    r.gt_map_found = true;
    const auto plan = agent_gtournament_user_side_effect_plan(r);
    EXPECT_FALSE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, AgentGtournamentSideEffectKind::ForwardRawToMap);
    EXPECT_EQ(plan.effects[0].protocol, gtournament_event_end);
    EXPECT_EQ(plan.effects[0].target_map, 28u);
}

TEST(GtournamentClassifyPlan, EventEndLevel0GtMapMissingDrops) {
    GtournamentRequest r{};
    r.protocol = gtournament_event_end;
    r.user_level = 0u;
    r.gt_map_found = false;
    const auto plan = agent_gtournament_user_side_effect_plan(r);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, AgentGtournamentSideEffectKind::Drop);
}

TEST(GtournamentClassifyPlan, DefaultProtocolForwardsToMapServer) {
    // Any unrecognized protocol falls into the legacy default branch.
    GtournamentRequest r{};
    r.protocol = 200u;
    const auto plan = agent_gtournament_user_side_effect_plan(r);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, AgentGtournamentSideEffectKind::ForwardRawToMap);
    EXPECT_EQ(plan.effects[0].protocol, 200u);
    EXPECT_EQ(plan.effects[0].target_map, 0u);
}

TEST(GtournamentClassifyPlan, DefaultProtocolZeroForwards) {
    GtournamentRequest r{};
    r.protocol = 0u;
    const auto plan = agent_gtournament_user_side_effect_plan(r);
    EXPECT_EQ(plan.effects[0].kind, AgentGtournamentSideEffectKind::ForwardRawToMap);
}

// ---------------------- apply: pure routing, no state mutation ----------------------

TEST(GtournamentApplyPlan, ForwardPlanReturnsTrue) {
    GtournamentAction a{};
    a.kind = GtournamentActionKind::forward_to_map_server;
    a.protocol = 200u;
    const auto plan = agent_gtournament_side_effect_plan(a);
    EXPECT_TRUE(apply_agent_gtournament_side_effect_plan(plan));
}

TEST(GtournamentApplyPlan, NackPlanReturnsTrue) {
    GtournamentAction a{};
    a.kind = GtournamentActionKind::send_battlejoin_nack_to_user;
    const auto plan = agent_gtournament_side_effect_plan(a);
    EXPECT_TRUE(apply_agent_gtournament_side_effect_plan(plan));
}

TEST(GtournamentApplyPlan, DropPlanReturnsFalse) {
    GtournamentAction a{};
    a.kind = GtournamentActionKind::drop_no_user;
    const auto plan = agent_gtournament_side_effect_plan(a);
    EXPECT_FALSE(apply_agent_gtournament_side_effect_plan(plan));
}

TEST(GtournamentApplyPlan, EmptyEffectsPlanReturnsFalse) {
    AgentGtournamentSideEffectPlan plan;
    plan.dispatched = true;
    plan.drop = false;
    EXPECT_FALSE(apply_agent_gtournament_side_effect_plan(plan));
}

TEST(GtournamentApplyPlan, MultiEffectForwardPlanReturnsTrue) {
    AgentGtournamentSideEffectPlan plan;
    plan.dispatched = true;
    plan.drop = false;
    plan.effects.push_back({AgentGtournamentSideEffectKind::ForwardRawToMap, gtournament_movetobattlemap_syn, 1u, 0u, 0u});
    plan.effects.push_back({AgentGtournamentSideEffectKind::ForwardRawToMap, gtournament_standinginfo_syn, 2u, 0u, 28u});
    EXPECT_TRUE(apply_agent_gtournament_side_effect_plan(plan));
}

TEST(GtournamentApplyPlan, MixedForwardAndDropEffectsReturnsTrue) {
    AgentGtournamentSideEffectPlan plan;
    plan.dispatched = true;
    plan.drop = false;
    plan.effects.push_back({AgentGtournamentSideEffectKind::Drop, 0u, 0u, 0u, 0u});
    plan.effects.push_back({AgentGtournamentSideEffectKind::ForwardRawToMap, 200u, 1u, 0u, 0u});
    EXPECT_TRUE(apply_agent_gtournament_side_effect_plan(plan));
}

// ---------------------- predicate coverage ----------------------

TEST(GtournamentPlanPredicates, TargetsMapOnlyForward) {
    AgentGtournamentSideEffect fwd{AgentGtournamentSideEffectKind::ForwardRawToMap, 0u, 0u, 0u, 0u};
    AgentGtournamentSideEffect drop{AgentGtournamentSideEffectKind::Drop, 0u, 0u, 0u, 0u};
    EXPECT_TRUE(agent_gtournament_effect_targets_map(fwd));
    EXPECT_FALSE(agent_gtournament_effect_targets_map(drop));
}

TEST(GtournamentPlanPredicates, TargetsUserOnlyNack) {
    AgentGtournamentSideEffect nack{AgentGtournamentSideEffectKind::SendNackToUser, 0u, 0u, 0u, 0u};
    AgentGtournamentSideEffect fwd{AgentGtournamentSideEffectKind::ForwardRawToMap, 0u, 0u, 0u, 0u};
    EXPECT_TRUE(agent_gtournament_effect_targets_user(nack));
    EXPECT_FALSE(agent_gtournament_effect_targets_user(fwd));
}

TEST(GtournamentPlanPredicates, DropTargetsNeither) {
    AgentGtournamentSideEffect drop{AgentGtournamentSideEffectKind::Drop, 0u, 0u, 0u, 0u};
    EXPECT_FALSE(agent_gtournament_effect_targets_map(drop));
    EXPECT_FALSE(agent_gtournament_effect_targets_user(drop));
}

TEST(GtournamentPlanPredicates, NackDoesNotTargetMap) {
    AgentGtournamentSideEffect nack{AgentGtournamentSideEffectKind::SendNackToUser, 0u, 0u, 0u, 0u};
    EXPECT_FALSE(agent_gtournament_effect_targets_map(nack));
}

// ---------------------- 1:1 mirror with classify_gtournament_user ----------------------

TEST(GtournamentApplyPlan, UserPlanMirrorsClassifyGtournamentUser) {
    // For each branch the user-side plan must produce the same effect kind as classify.
    struct Case { GtournamentRequest req; AgentGtournamentSideEffectKind expected_kind; };
    const Case cases[] = {
        // !user_found always drops.
        {{0u, 0u, false, false, false, 0u, 0u, 0u, 0u, 0u, 0u, 0u}, AgentGtournamentSideEffectKind::Drop},
        // movetobattlemap_syn + user_map_found -> ForwardRawToMap.
        {{gtournament_movetobattlemap_syn, 0u, true, false, true, 0u, 0u, 0u, 0u, 0u, 0u, 0u}, AgentGtournamentSideEffectKind::ForwardRawToMap},
        // movetobattlemap_syn + !user_map_found -> SendNackToUser.
        {{gtournament_movetobattlemap_syn, 0u, true, false, false, 0u, 0u, 0u, 0u, 0u, 0u, 0u}, AgentGtournamentSideEffectKind::SendNackToUser},
        // standinginfo_syn + gt_map_found -> ForwardRawToMap (target=28).
        {{gtournament_standinginfo_syn, 0u, true, true, false, 0u, 0u, 0u, 0u, 0u, 0u, 0u}, AgentGtournamentSideEffectKind::ForwardRawToMap},
        // standinginfo_syn + !gt_map_found -> SendNackToUser.
        {{gtournament_standinginfo_syn, 0u, true, false, false, 0u, 0u, 0u, 0u, 0u, 0u, 0u}, AgentGtournamentSideEffectKind::SendNackToUser},
        // battlejoin_syn + gt_map_found -> ForwardRawToMap (target=28).
        {{gtournament_battlejoin_syn, 0u, true, true, false, 0u, 0u, 0u, 0u, 0u, 0u, 0u}, AgentGtournamentSideEffectKind::ForwardRawToMap},
        // battlejoin_syn + !gt_map_found -> SendNackToUser.
        {{gtournament_battlejoin_syn, 0u, true, false, false, 0u, 0u, 0u, 0u, 0u, 0u, 0u}, AgentGtournamentSideEffectKind::SendNackToUser},
        // observerjoin_syn + gt_map_found -> ForwardRawToMap.
        {{gtournament_observerjoin_syn, 0u, true, true, false, 0u, 0u, 0u, 0u, 0u, 0u, 0u}, AgentGtournamentSideEffectKind::ForwardRawToMap},
        // observerjoin_syn + !gt_map_found -> SendNackToUser.
        {{gtournament_observerjoin_syn, 0u, true, false, false, 0u, 0u, 0u, 0u, 0u, 0u, 0u}, AgentGtournamentSideEffectKind::SendNackToUser},
        // leave_syn -> ForwardRawToMap.
        {{gtournament_leave_syn, 0u, true, false, false, 0u, 0u, 0u, 0u, 0u, 0u, 0u}, AgentGtournamentSideEffectKind::ForwardRawToMap},
        // cheat + cheat_data==1 -> ForwardRawToMap (target=0).
        {{gtournament_cheat, 0u, true, false, false, 0u, 0u, 0u, 0u, 1u, 0u, 0u}, AgentGtournamentSideEffectKind::ForwardRawToMap},
        // cheat + cheat_data!=1 + gt_map_found -> ForwardRawToMap (target=28).
        {{gtournament_cheat, 0u, true, true, false, 0u, 0u, 0u, 0u, 0u, 0u, 0u}, AgentGtournamentSideEffectKind::ForwardRawToMap},
        // cheat + cheat_data!=1 + !gt_map_found -> Drop.
        {{gtournament_cheat, 0u, true, false, false, 0u, 0u, 0u, 0u, 0u, 0u, 0u}, AgentGtournamentSideEffectKind::Drop},
        // event_start + user_level>8 -> Drop.
        {{gtournament_event_start, 0u, true, true, false, 9u, 0u, 0u, 0u, 0u, 0u, 0u}, AgentGtournamentSideEffectKind::Drop},
        // event_start + user_level<=8 + gt_map_found -> ForwardRawToMap.
        {{gtournament_event_start, 0u, true, true, false, 0u, 0u, 0u, 0u, 0u, 0u, 0u}, AgentGtournamentSideEffectKind::ForwardRawToMap},
        // event_start + user_level<=8 + !gt_map_found -> Drop.
        {{gtournament_event_start, 0u, true, false, false, 0u, 0u, 0u, 0u, 0u, 0u, 0u}, AgentGtournamentSideEffectKind::Drop},
        // event_end + user_level>8 -> Drop.
        {{gtournament_event_end, 0u, true, true, false, 9u, 0u, 0u, 0u, 0u, 0u, 0u}, AgentGtournamentSideEffectKind::Drop},
        // default protocol -> ForwardRawToMap.
        {{200u, 0u, true, false, false, 0u, 0u, 0u, 0u, 0u, 0u, 0u}, AgentGtournamentSideEffectKind::ForwardRawToMap},
    };
    for (const auto& c : cases) {
        const auto plan = agent_gtournament_user_side_effect_plan(c.req);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].kind, c.expected_kind) << (int)c.req.protocol;
        // Mirror check: classify action kind must match plan effect kind.
        const auto a = classify_gtournament_user(c.req);
        // action.kind -> effect.kind mapping via plan-builder.
        const auto mirror = agent_gtournament_side_effect_plan(a);
        EXPECT_EQ(mirror.effects[0].kind, c.expected_kind);
    }
}

TEST(GtournamentApplyPlan, UserPlanAndClassifyProtocolAgree) {
    // For every protocol + key boolean combination, the plan must agree with classify.
    const std::uint8_t protocols[] = {
        gtournament_movetobattlemap_syn,
        gtournament_standinginfo_syn,
        gtournament_battlejoin_syn,
        gtournament_observerjoin_syn,
        gtournament_leave_syn,
        gtournament_cheat,
        gtournament_event_start,
        gtournament_event_end,
        200u,
    };
    for (auto p : protocols) {
        for (bool gf : {false, true}) {
            for (bool umf : {false, true}) {
                for (std::uint8_t lvl : {static_cast<std::uint8_t>(0), static_cast<std::uint8_t>(8), static_cast<std::uint8_t>(9)}) {
                    GtournamentRequest r{};
                    r.protocol = p;
                    r.user_found = true;
                    r.gt_map_found = gf;
                    r.user_map_found = umf;
                    r.user_level = lvl;
                    r.cheat_data = 0u;
                    const auto a = classify_gtournament_user(r);
                    const auto plan = agent_gtournament_side_effect_plan(a);
                    const auto user_plan = agent_gtournament_user_side_effect_plan(r);
                    ASSERT_EQ(plan.effects.size(), 1u) << (int)p;
                    ASSERT_EQ(user_plan.effects.size(), 1u) << (int)p;
                    EXPECT_EQ(plan.effects[0].kind, user_plan.effects[0].kind) << (int)p;
                    EXPECT_EQ(plan.effects[0].protocol, user_plan.effects[0].protocol) << (int)p;
                }
            }
        }
    }
}

TEST(GtournamentApplyPlan, DefaultPlanMirrorsClassifyForUnrecognizedProtocol) {
    GtournamentRequest r{};
    r.protocol = 99u;
    r.object_id = 1234u;
    const auto a = classify_gtournament_user(r);
    const auto plan = agent_gtournament_user_side_effect_plan(r);
    EXPECT_EQ(a.kind, GtournamentActionKind::forward_to_map_server);
    EXPECT_EQ(plan.effects[0].kind, AgentGtournamentSideEffectKind::ForwardRawToMap);
    EXPECT_EQ(plan.effects[0].protocol, a.protocol);
    EXPECT_EQ(plan.effects[0].object_id, a.object_id);
    EXPECT_EQ(plan.effects[0].target_map, a.target_map);
}

// ---------------------- 1:1 lock: protocol constants ----------------------

TEST(GtournamentCategory, GtournamentCategoryIs59) {
    // Lock the 1:1 mapping: MP_GTOURNAMENT = 59 in MP_CATEGORY.
    EXPECT_EQ(gtournament_category, 59u);
}

TEST(GtournamentCategory, GtMapNumIs28) {
    // Lock the 1:1 mapping: #define GTMAPNUM 28.
    EXPECT_EQ(gt_map_num, 28u);
}

TEST(GtournamentCategory, ProtocolConstantsLocked) {
    // Lock 1:1 wire byte values for the 8 sub-protocols the routing branches read.
    EXPECT_EQ(gtournament_movetobattlemap_syn, 7u);
    EXPECT_EQ(gtournament_movetobattlemap_nack, 9u);
    EXPECT_EQ(gtournament_observerjoin_syn, 10u);
    EXPECT_EQ(gtournament_battlejoin_syn, 13u);
    EXPECT_EQ(gtournament_battlejoin_nack, 15u);
    EXPECT_EQ(gtournament_leave_syn, 17u);
    EXPECT_EQ(gtournament_standinginfo_syn, 18u);
    EXPECT_EQ(gtournament_standinginfo_nack, 20u);
    EXPECT_EQ(gtournament_cheat, 41u);
    EXPECT_EQ(gtournament_event_start, 43u);
    EXPECT_EQ(gtournament_event_end, 46u);
    EXPECT_EQ(gt_error_code_error, 0u);
}

TEST(GtournamentCategory, GtMapNumLiteralMatchesEffectTargetMap) {
    // Lock 1:1: standinginfo_syn + gt_map_found -> target_map = gt_map_num.
    GtournamentRequest r{};
    r.protocol = gtournament_standinginfo_syn;
    r.gt_map_found = true;
    const auto plan = agent_gtournament_user_side_effect_plan(r);
    EXPECT_EQ(plan.effects[0].target_map, gt_map_num);
    EXPECT_EQ(plan.effects[0].target_map, 28u);
}

TEST(GtournamentPlan, CheatGtMapForwardMatchesEffectTargetMap) {
    GtournamentRequest r{};
    r.protocol = gtournament_cheat;
    r.cheat_data = 7u;
    r.gt_map_found = true;
    const auto plan = agent_gtournament_user_side_effect_plan(r);
    EXPECT_EQ(plan.effects[0].target_map, gt_map_num);
}

TEST(GtournamentPlan, EventStartLevelLowGtMapForwardMatchesEffectTargetMap) {
    GtournamentRequest r{};
    r.protocol = gtournament_event_start;
    r.user_level = 5u;
    r.gt_map_found = true;
    const auto plan = agent_gtournament_user_side_effect_plan(r);
    EXPECT_EQ(plan.effects[0].target_map, gt_map_num);
}

TEST(GtournamentPlan, NackEffectsAlwaysHaveZeroTargetMap) {
    // 1:1: NACK effects always target the user (target_map=0 is meaningless).
    GtournamentAction a{};
    a.kind = GtournamentActionKind::send_battlejoin_nack_to_user;
    const auto plan = agent_gtournament_side_effect_plan(a);
    EXPECT_EQ(plan.effects[0].target_map, 0u);
    a.kind = GtournamentActionKind::send_standing_info_nack_to_user;
    const auto plan2 = agent_gtournament_side_effect_plan(a);
    EXPECT_EQ(plan2.effects[0].target_map, 0u);
    a.kind = GtournamentActionKind::send_movetobattle_nack_to_user;
    const auto plan3 = agent_gtournament_side_effect_plan(a);
    EXPECT_EQ(plan3.effects[0].target_map, 0u);
}
