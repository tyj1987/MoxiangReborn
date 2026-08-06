//
// D4.100 -- AgentSiegeWar side-effect plan unit tests.
//
// 1:1 lock the legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_SIEGEWARUserMsgParser (lines 4788-4920) and MP_SIEGEWARServerMsgParser (lines
// 4923-5010). Each test pins one branch of the legacy dispatch to its modern
// side-effect plan output so future drift triggers a test failure.
//

#include <gtest/gtest.h>

#include "mxh/server/agent_siegewar.hpp"
#include "mxh/server/agent_siegewar_server.hpp"
#include "mxh/server/agent_siegewar_side_effect_plan.hpp"

using namespace mxh::server;

TEST(SiegeWarUserPlan, DropNoUserEmitsDropEffect) {
    SiegeWarUserAction action{};
    action.kind = SiegeWarUserActionKind::drop_no_user;
    action.protocol = siegewar_movein_syn;
    const auto plan = siegewar_user_side_effect_plan(action, false, false);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, SiegeWarUserSideEffectKind::Drop);
}

TEST(SiegeWarUserPlan, CheatFansOutToTwoMaps) {
    SiegeWarUserAction action{};
    action.kind = SiegeWarUserActionKind::cheat_fanout_to_map_servers;
    action.protocol = siegewar_cheat;
    action.data2_target_map = 5u;
    action.data3_target_map = 7u;
    const auto plan = siegewar_user_side_effect_plan(action, true, false);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, SiegeWarUserSideEffectKind::FanoutToMapServers);
    EXPECT_EQ(plan.effects[0].data2_target_map, 5u);
    EXPECT_EQ(plan.effects[0].data3_target_map, 7u);
    EXPECT_TRUE(plan.effects[0].data2_map_found);
    EXPECT_FALSE(plan.effects[0].data3_map_found);
}

TEST(SiegeWarUserPlan, CheatAlwaysFansOutEvenWhenNoUser) {
    SiegeWarUserAction action{};
    action.kind = SiegeWarUserActionKind::cheat_fanout_to_map_servers;
    action.protocol = siegewar_cheat;
    action.data2_target_map = 1u;
    action.data3_target_map = 2u;
    const auto plan = siegewar_user_side_effect_plan(action, false, false);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, SiegeWarUserSideEffectKind::FanoutToMapServers);
}

TEST(SiegeWarUserPlan, MoveInSynForwardsToUserMap) {
    SiegeWarUserAction action{};
    action.kind = SiegeWarUserActionKind::movein_to_user_map;
    action.protocol = siegewar_movein_syn;
    action.unique_connect_idx = 42u;
    const auto plan = siegewar_user_side_effect_plan(action, false, false);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, SiegeWarUserSideEffectKind::ForwardRawToUserMap);
    EXPECT_EQ(plan.effects[0].unique_connect_idx, 42u);
    EXPECT_TRUE(siegewar_user_effect_targets_map(plan.effects[0]));
    EXPECT_FALSE(siegewar_user_effect_targets_user(plan.effects[0]));
}

TEST(SiegeWarUserPlan, BattleJoinWithTargetSendsToMapAndUpdatesUserSlot) {
    SiegeWarUserAction action{};
    action.kind = SiegeWarUserActionKind::battlejoin_to_target_map_or_nack;
    action.protocol = siegewar_battlejoin_syn;
    action.guild_idx = 11u;
    action.return_map_num = 12u;
    action.observer_flag = 0u;
    action.unique_connect_idx = 99u;
    action.user_level = 50u;
    action.channel = 3u;
    const auto plan = siegewar_user_side_effect_plan(action, false, false);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    EXPECT_TRUE(plan.update_user_map_slot);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, SiegeWarUserSideEffectKind::ForwardBattleJoinToMap);
    EXPECT_TRUE(plan.effects[0].target_map_found);
    EXPECT_EQ(plan.effects[0].guild_idx, 11u);
    EXPECT_EQ(plan.effects[0].return_map_num, 12u);
    EXPECT_EQ(plan.effects[0].observer_flag, 0u);
    EXPECT_TRUE(siegewar_user_effect_targets_map(plan.effects[0]));
    EXPECT_FALSE(siegewar_user_effect_targets_user(plan.effects[0]));
}

TEST(SiegeWarUserPlan, ObserverJoinWithTargetSendsToMapWithObserverFlag) {
    SiegeWarUserAction action{};
    action.kind = SiegeWarUserActionKind::battlejoin_to_target_map_or_nack;
    action.protocol = siegewar_observerjoin_syn;
    action.guild_idx = 13u;
    action.return_map_num = 14u;
    action.observer_flag = 1u;
    const auto plan = siegewar_user_side_effect_plan(action, false, false);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_TRUE(plan.update_user_map_slot);
    EXPECT_EQ(plan.effects[0].kind, SiegeWarUserSideEffectKind::ForwardBattleJoinToMap);
    EXPECT_EQ(plan.effects[0].observer_flag, 1u);
}

TEST(SiegeWarUserPlan, BattleJoinWithoutTargetSendsNackToUser) {
    SiegeWarUserAction action{};
    action.kind = SiegeWarUserActionKind::battlejoin_to_target_map_or_nack;
    action.protocol = siegewar_battlejoin_nack;
    action.unique_connect_idx = 7u;
    const auto plan = siegewar_user_side_effect_plan(action, false, false);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    EXPECT_FALSE(plan.update_user_map_slot);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, SiegeWarUserSideEffectKind::SendBattleJoinNackToUser);
    EXPECT_FALSE(plan.effects[0].target_map_found);
    EXPECT_TRUE(siegewar_user_effect_targets_user(plan.effects[0]));
    EXPECT_FALSE(siegewar_user_effect_targets_map(plan.effects[0]));
}

TEST(SiegeWarUserPlan, LeaveSynForwardsToUserMapWithIdentityFields) {
    SiegeWarUserAction action{};
    action.kind = SiegeWarUserActionKind::leave_syn_to_user_map;
    action.protocol = siegewar_leave_syn;
    action.unique_connect_idx = 21u;
    action.user_level = 60u;
    action.channel = 5u;
    const auto plan = siegewar_user_side_effect_plan(action, false, false);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, SiegeWarUserSideEffectKind::ForwardLeaveSynToUserMap);
    EXPECT_EQ(plan.effects[0].unique_connect_idx, 21u);
    EXPECT_EQ(plan.effects[0].user_level, 60u);
    EXPECT_EQ(plan.effects[0].channel, 5u);
    EXPECT_TRUE(siegewar_user_effect_targets_map(plan.effects[0]));
}

TEST(SiegeWarUserPlan, DefaultForwardsToMap) {
    SiegeWarUserAction action{};
    action.kind = SiegeWarUserActionKind::default_forward_to_map;
    action.protocol = 99u;
    const auto plan = siegewar_user_side_effect_plan(action, false, false);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, SiegeWarUserSideEffectKind::ForwardRawToMap);
    EXPECT_TRUE(siegewar_user_effect_targets_map(plan.effects[0]));
}

// ---------------------------------------------------------------------
// SERVER side-effects
// ---------------------------------------------------------------------

TEST(SiegeWarServerPlan, DropNoUserEmitsDropEffect) {
    SiegeWarServerAction action{};
    action.kind = SiegeWarServerActionKind::drop_no_user;
    action.protocol = siegewar_returntomap;
    const auto plan = siegewar_server_side_effect_plan(action, 0u, 0u);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, SiegeWarServerSideEffectKind::Drop);
    EXPECT_FALSE(siegewar_server_effect_targets_map(plan.effects[0]));
    EXPECT_FALSE(siegewar_server_effect_targets_client(plan.effects[0]));
}

TEST(SiegeWarServerPlan, TaxrateBroadcastsWithParamAndAffectedCount) {
    SiegeWarServerAction action{};
    action.kind = SiegeWarServerActionKind::broadcast_taxrate_to_affected_maps;
    action.protocol = siegewar_taxrate;
    const auto plan = siegewar_server_side_effect_plan(action, 123u, 4u);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    EXPECT_FALSE(plan.forward_to_client);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, SiegeWarServerSideEffectKind::BroadcastTaxrateToAffectedMaps);
    EXPECT_EQ(plan.effects[0].taxrate_param, 123u);
    EXPECT_EQ(plan.effects[0].affected_count, 4u);
    EXPECT_TRUE(siegewar_server_effect_targets_map(plan.effects[0]));
    EXPECT_FALSE(siegewar_server_effect_targets_client(plan.effects[0]));
}

TEST(SiegeWarServerPlan, ReturnToMapWithTargetUpdatesUserSlotAndForwardsToClient) {
    SiegeWarServerAction action{};
    action.kind = SiegeWarServerActionKind::update_user_map_and_forward_to_client;
    action.protocol = siegewar_returntomap;
    action.target_map = 17u;
    const auto plan = siegewar_server_side_effect_plan(action, 0u, 0u);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    EXPECT_TRUE(plan.forward_to_client);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, SiegeWarServerSideEffectKind::UpdateUserMapAndForwardToClient);
    EXPECT_EQ(plan.effects[0].target_map, 17u);
    EXPECT_TRUE(plan.effects[0].target_map_found);
    EXPECT_TRUE(plan.effects[0].update_user_map_slot);
    EXPECT_TRUE(siegewar_server_effect_targets_client(plan.effects[0]));
    EXPECT_FALSE(siegewar_server_effect_targets_map(plan.effects[0]));
}

TEST(SiegeWarServerPlan, ReturnToMapWithNoTargetStillForwardsToClient) {
    SiegeWarServerAction action{};
    action.kind = SiegeWarServerActionKind::default_forward_to_client;
    action.protocol = siegewar_returntomap;
    action.target_map = 0u;
    const auto plan = siegewar_server_side_effect_plan(action, 0u, 0u);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_TRUE(plan.forward_to_client);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, SiegeWarServerSideEffectKind::ForwardRawToClient);
    EXPECT_FALSE(plan.effects[0].target_map_found);
    EXPECT_TRUE(siegewar_server_effect_targets_client(plan.effects[0]));
}

TEST(SiegeWarServerPlan, FlagChangeBroadcastsToAllUsers) {
    SiegeWarServerAction action{};
    action.kind = SiegeWarServerActionKind::broadcast_to_all_users;
    action.protocol = siegewar_flagchange;
    const auto plan = siegewar_server_side_effect_plan(action, 0u, 0u);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    EXPECT_FALSE(plan.forward_to_client);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, SiegeWarServerSideEffectKind::BroadcastToAllUsers);
    EXPECT_FALSE(siegewar_server_effect_targets_map(plan.effects[0]));
    EXPECT_FALSE(siegewar_server_effect_targets_client(plan.effects[0]));
}

TEST(SiegeWarServerPlan, DefaultForwardsToClient) {
    SiegeWarServerAction action{};
    action.kind = SiegeWarServerActionKind::default_forward_to_client;
    action.protocol = 99u;
    const auto plan = siegewar_server_side_effect_plan(action, 0u, 0u);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_TRUE(plan.forward_to_client);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, SiegeWarServerSideEffectKind::ForwardRawToClient);
    EXPECT_TRUE(siegewar_server_effect_targets_client(plan.effects[0]));
}

// ---------------------------------------------------------------------
// Coupling tests: classifier -> plan integration.
// ---------------------------------------------------------------------

TEST(SiegeWarUserPlanCoupling, MoveInClassifierProducesMoveInPlan) {
    SiegeWarUserRequest req{};
    req.protocol = siegewar_movein_syn;
    req.user_found = true;
    const auto action = classify_siegewar_user(req);
    const auto plan = siegewar_user_side_effect_plan(action, false, false);
    EXPECT_EQ(action.kind, SiegeWarUserActionKind::movein_to_user_map);
    EXPECT_EQ(plan.effects[0].kind, SiegeWarUserSideEffectKind::ForwardRawToUserMap);
}

TEST(SiegeWarUserPlanCoupling, NoUserMoveInClassifierProducesDropPlan) {
    SiegeWarUserRequest req{};
    req.protocol = siegewar_movein_syn;
    req.user_found = false;
    const auto action = classify_siegewar_user(req);
    const auto plan = siegewar_user_side_effect_plan(action, false, false);
    EXPECT_EQ(action.kind, SiegeWarUserActionKind::drop_no_user);
    EXPECT_EQ(plan.effects[0].kind, SiegeWarUserSideEffectKind::Drop);
}

TEST(SiegeWarUserPlanCoupling, BattleJoinNoTargetClassifierProducesNackPlan) {
    SiegeWarUserRequest req{};
    req.protocol = siegewar_battlejoin_syn;
    req.user_found = true;
    req.target_map_found = false;
    const auto action = classify_siegewar_user(req);
    const auto plan = siegewar_user_side_effect_plan(action, false, false);
    EXPECT_EQ(action.kind, SiegeWarUserActionKind::battlejoin_to_target_map_or_nack);
    EXPECT_EQ(action.protocol, siegewar_battlejoin_nack);
    EXPECT_EQ(plan.effects[0].kind, SiegeWarUserSideEffectKind::SendBattleJoinNackToUser);
}

TEST(SiegeWarServerPlanCoupling, TaxrateClassifierProducesBroadcastPlan) {
    SiegeWarServerRequest req{};
    req.protocol = siegewar_taxrate;
    req.affected_count = 3u;
    const auto action = classify_siegewar_server(req);
    const auto plan = siegewar_server_side_effect_plan(action, 99u, req.affected_count);
    EXPECT_EQ(action.kind, SiegeWarServerActionKind::broadcast_taxrate_to_affected_maps);
    EXPECT_EQ(plan.effects[0].kind, SiegeWarServerSideEffectKind::BroadcastTaxrateToAffectedMaps);
    EXPECT_EQ(plan.effects[0].affected_count, 3u);
}
