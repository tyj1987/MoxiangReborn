// D4.103 -- AgentGTournament side-effect plan unit tests.
//
// 1:1 lock the legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_GTOURNAMENTUserMsgParser (lines 4295-4497) and MP_GTOURNAMENTServerMsgParser
// (lines 4498-4543). Each test pins one branch of the legacy dispatch to its modern
// side-effect plan output so future drift triggers a test failure.
//

#include <gtest/gtest.h>

#include "mxh/server/agent_gtournament_server.hpp"
#include "mxh/server/agent_gtournament_server_side_effect_plan.hpp"

using namespace mxh::server;

// ---------------------------------------------------------------------
// USER side-effects
// ---------------------------------------------------------------------

TEST(GTournamentUserPlan, DropNoUserByCharidEmitsDrop) {
    GTournamentUserAction a{};
    a.kind = GTournamentActionKind::drop_no_user_by_charid;
    const auto plan = gtournament_user_side_effect_plan(a);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, GTournamentUserSideEffectKind::Drop);
}

TEST(GTournamentUserPlan, DropNoUserByConnEmitsDrop) {
    GTournamentUserAction a{};
    a.kind = GTournamentActionKind::drop_no_user_by_conn;
    const auto plan = gtournament_user_side_effect_plan(a);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, GTournamentUserSideEffectKind::Drop);
}

TEST(GTournamentUserPlan, DropEventStartNotGmEmitsDrop) {
    GTournamentUserAction a{};
    a.kind = GTournamentActionKind::drop_event_start_not_gm;
    const auto plan = gtournament_user_side_effect_plan(a);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, GTournamentUserSideEffectKind::Drop);
}

TEST(GTournamentUserPlan, ForwardToUserMapEmitsForwardRawToUserMap) {
    GTournamentUserAction a{};
    a.kind = GTournamentActionKind::forward_to_user_map_server;
    a.reply_protocol = gtournament_leave_syn;
    const auto plan = gtournament_user_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, GTournamentUserSideEffectKind::ForwardRawToUserMap);
    EXPECT_EQ(plan.effects[0].reply_protocol, gtournament_leave_syn);
    EXPECT_TRUE(gtournament_user_effect_targets_map(plan.effects[0]));
}

TEST(GTournamentUserPlan, ForwardToTournamentMapEmitsForwardRawToTournamentMap) {
    GTournamentUserAction a{};
    a.kind = GTournamentActionKind::forward_to_tournament_map_server;
    a.reply_protocol = gtournament_movetobattlemap_syn;
    a.gt_map_num = gtournament_map_num;
    const auto plan = gtournament_user_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_EQ(plan.effects[0].kind, GTournamentUserSideEffectKind::ForwardRawToTournamentMap);
    EXPECT_EQ(plan.effects[0].gt_map_num, gtournament_map_num);
    EXPECT_TRUE(gtournament_user_effect_targets_map(plan.effects[0]));
}

TEST(GTournamentUserPlan, SendMoveToBattleMapNackEmitsNackEffect) {
    GTournamentUserAction a{};
    a.kind = GTournamentActionKind::send_movetobattlemap_nack_to_user;
    a.reply_protocol = gtournament_movetobattlemap_nack;
    a.gt_map_num = gtournament_map_num;
    const auto plan = gtournament_user_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_EQ(plan.effects[0].kind, GTournamentUserSideEffectKind::SendMoveToBattleMapNackToUser);
    EXPECT_EQ(plan.effects[0].reply_protocol, gtournament_movetobattlemap_nack);
    EXPECT_TRUE(gtournament_user_effect_targets_user(plan.effects[0]));
}

TEST(GTournamentUserPlan, SendStandingInfoNackEmitsNackEffect) {
    GTournamentUserAction a{};
    a.kind = GTournamentActionKind::send_standinginfo_nack_to_user;
    a.reply_protocol = gtournament_standinginfo_nack;
    const auto plan = gtournament_user_side_effect_plan(a);
    EXPECT_EQ(plan.effects[0].kind, GTournamentUserSideEffectKind::SendStandingInfoNackToUser);
    EXPECT_EQ(plan.effects[0].reply_protocol, gtournament_standinginfo_nack);
    EXPECT_TRUE(gtournament_user_effect_targets_user(plan.effects[0]));
}

TEST(GTournamentUserPlan, SendBattleJoinNackEmitsNackEffect) {
    GTournamentUserAction a{};
    a.kind = GTournamentActionKind::send_battlejoin_nack_to_user;
    a.reply_protocol = gtournament_battlejoin_nack;
    const auto plan = gtournament_user_side_effect_plan(a);
    EXPECT_EQ(plan.effects[0].kind, GTournamentUserSideEffectKind::SendBattleJoinNackToUser);
    EXPECT_EQ(plan.effects[0].reply_protocol, gtournament_battlejoin_nack);
    EXPECT_TRUE(gtournament_user_effect_targets_user(plan.effects[0]));
}

TEST(GTournamentUserPlan, DefaultToTransToUserEmitsDefaultTrans) {
    GTournamentUserAction a{};
    a.kind = GTournamentActionKind::default_to_trans_to_user;
    a.reply_protocol = 99u;
    const auto plan = gtournament_user_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_EQ(plan.effects[0].kind, GTournamentUserSideEffectKind::DefaultTransToUser);
    EXPECT_TRUE(gtournament_user_effect_targets_user(plan.effects[0]));
}

// ---------------------------------------------------------------------
// SERVER side-effects
// ---------------------------------------------------------------------

TEST(GTournamentServerPlan, DropNoUserByCharidEmitsDrop) {
    GTournamentServerAction a{};
    a.kind = GTournamentActionKind::drop_no_user_by_charid;
    const auto plan = gtournament_server_side_effect_plan(a);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
    EXPECT_EQ(plan.effects[0].kind, GTournamentServerSideEffectKind::Drop);
}

TEST(GTournamentServerPlan, SendToUserEmitsSendRawToUser) {
    GTournamentServerAction a{};
    a.kind = GTournamentActionKind::send_to_user;
    a.reply_protocol = gtournament_cheat;
    const auto plan = gtournament_server_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, GTournamentServerSideEffectKind::SendRawToUser);
    EXPECT_EQ(plan.effects[0].reply_protocol, gtournament_cheat);
    EXPECT_TRUE(gtournament_server_effect_targets_user(plan.effects[0]));
}

TEST(GTournamentServerPlan, BroadcastToClientEmitsBroadcastToMapServers) {
    GTournamentServerAction a{};
    a.kind = GTournamentActionKind::broadcast_to_client;
    a.reply_protocol = gtournament_notify_winlose;
    const auto plan = gtournament_server_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_EQ(plan.effects[0].kind, GTournamentServerSideEffectKind::BroadcastToMapServers);
    EXPECT_TRUE(gtournament_server_effect_targets_map(plan.effects[0]));
}

TEST(GTournamentServerPlan, SetUserMapStateAndForwardEmitsUpdateAndForward) {
    GTournamentServerAction a{};
    a.kind = GTournamentActionKind::set_user_map_state_and_forward;
    a.reply_protocol = gtournament_returntomap;
    a.update_map_state = true;
    const auto plan = gtournament_server_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_EQ(plan.effects[0].kind, GTournamentServerSideEffectKind::SetUserMapStateAndForwardToUser);
    EXPECT_TRUE(plan.effects[0].update_map_state);
    EXPECT_TRUE(gtournament_server_effect_targets_user(plan.effects[0]));
}

// ---------------------------------------------------------------------
// Classifier 1:1 tests
// ---------------------------------------------------------------------

TEST(GTournamentUserClassifierPlan, MoveToBattleMapNoPortEmitsNackPlan) {
    GTournamentUserRequest req{};
    req.protocol = gtournament_movetobattlemap_syn;
    req.gt_map_port = 0u;
    req.user_known_by_charid = true;
    const auto action = classify_agent_gtournament_user(req);
    EXPECT_EQ(action.kind, GTournamentActionKind::send_movetobattlemap_nack_to_user);
    const auto plan = gtournament_user_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, GTournamentUserSideEffectKind::SendMoveToBattleMapNackToUser);
}

TEST(GTournamentUserClassifierPlan, MoveToBattleMapNoUserByCharidEmitsDrop) {
    GTournamentUserRequest req{};
    req.protocol = gtournament_movetobattlemap_syn;
    req.user_known_by_charid = false;
    const auto action = classify_agent_gtournament_user(req);
    EXPECT_EQ(action.kind, GTournamentActionKind::drop_no_user_by_charid);
    const auto plan = gtournament_user_side_effect_plan(action);
    EXPECT_TRUE(plan.drop);
}

TEST(GTournamentUserClassifierPlan, BattleJoinWithPortEmitsForwardToTournamentMap) {
    GTournamentUserRequest req{};
    req.protocol = gtournament_battlejoin_syn;
    req.gt_map_port = 9060u;
    req.user_known_by_charid = true;
    const auto action = classify_agent_gtournament_user(req);
    EXPECT_EQ(action.kind, GTournamentActionKind::forward_to_tournament_map_server);
    EXPECT_EQ(action.gt_map_num, gtournament_map_num);
    const auto plan = gtournament_user_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, GTournamentUserSideEffectKind::ForwardRawToTournamentMap);
}

TEST(GTournamentUserClassifierPlan, CheatUserMapEmitsForwardToUserMap) {
    GTournamentUserRequest req{};
    req.protocol = gtournament_cheat;
    req.user_known_by_charid = true;
    req.target_map_port = 9017u;
    req.gt_map_port = 0u;
    const auto action = classify_agent_gtournament_user(req);
    EXPECT_EQ(action.kind, GTournamentActionKind::forward_to_user_map_server);
    const auto plan = gtournament_user_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, GTournamentUserSideEffectKind::ForwardRawToUserMap);
}

TEST(GTournamentUserClassifierPlan, EventStartNonGmEmitsDrop) {
    GTournamentUserRequest req{};
    req.protocol = gtournament_event_start;
    req.user_known_by_conn = true;
    req.user_level = 99u;
    const auto action = classify_agent_gtournament_user(req);
    EXPECT_EQ(action.kind, GTournamentActionKind::drop_event_start_not_gm);
    const auto plan = gtournament_user_side_effect_plan(action);
    EXPECT_TRUE(plan.drop);
}

TEST(GTournamentServerClassifierPlan, ReturnToMapWithPortEmitsUpdatePlan) {
    GTournamentServerRequest req{};
    req.protocol = gtournament_returntomap;
    req.target_user_found = true;
    req.return_port_known = true;
    const auto action = classify_agent_gtournament_server(req);
    EXPECT_EQ(action.kind, GTournamentActionKind::set_user_map_state_and_forward);
    EXPECT_TRUE(action.update_map_state);
    const auto plan = gtournament_server_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, GTournamentServerSideEffectKind::SetUserMapStateAndForwardToUser);
}

TEST(GTournamentServerClassifierPlan, ReturnToMapNoPortEmitsSendToUser) {
    GTournamentServerRequest req{};
    req.protocol = gtournament_returntomap;
    req.target_user_found = true;
    req.return_port_known = false;
    const auto action = classify_agent_gtournament_server(req);
    EXPECT_EQ(action.kind, GTournamentActionKind::send_to_user);
    const auto plan = gtournament_server_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, GTournamentServerSideEffectKind::SendRawToUser);
}

TEST(GTournamentServerClassifierPlan, NotifyWinLoseNoUserEmitsDrop) {
    GTournamentServerRequest req{};
    req.protocol = gtournament_notify_winlose;
    req.target_user_found = false;
    const auto action = classify_agent_gtournament_server(req);
    EXPECT_EQ(action.kind, GTournamentActionKind::drop_no_user_by_charid);
    const auto plan = gtournament_server_side_effect_plan(action);
    EXPECT_TRUE(plan.drop);
}

TEST(GTournamentServerClassifierPlan, UnknownServerProtocolEmitsBroadcast) {
    GTournamentServerRequest req{};
    req.protocol = 200u;
    const auto action = classify_agent_gtournament_server(req);
    EXPECT_EQ(action.kind, GTournamentActionKind::broadcast_to_client);
    const auto plan = gtournament_server_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, GTournamentServerSideEffectKind::BroadcastToMapServers);
}