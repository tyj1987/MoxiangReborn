// D4.116 -- AgentPowerUp side-effect plan unit tests.
//
// 1:1 lock the legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_POWERUPMsgParser (206-208) + ServerSystem self-init (309-318) +
// MP_AGENTSERVERMsgParser (230-280) + MP_SERVER_REGISTMAP_ACK (265-280).
//

#include <gtest/gtest.h>

#include "mxh/server/agent_powerup.hpp"
#include "mxh/server/agent_powerup_side_effect_plan.hpp"

using namespace mxh::server;

TEST(PowerUpPlan, ForwardToBootManagerEmitsForwardEffect) {
    PowerUpAction a{};
    a.kind = PowerUpActionKind::forward_to_boot_manager;
    a.forward_payload = true;
    const auto plan = powerup_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, PowerUpSideEffectKind::ForwardToBootManager);
    EXPECT_TRUE(plan.effects[0].forward_payload);
    EXPECT_TRUE(powerup_effect_targets_boot_manager(plan.effects[0]));
}

TEST(PowerUpPlan, AddSelfBootListEmitsAddEffect) {
    PowerUpAction a{};
    a.kind = PowerUpActionKind::add_self_boot_list;
    a.target_port = 9000u;
    const auto plan = powerup_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, PowerUpSideEffectKind::AddSelfBootList);
    EXPECT_EQ(plan.effects[0].target_port, 9000u);
    EXPECT_TRUE(powerup_effect_targets_boot_manager(plan.effects[0]));
}

TEST(PowerUpPlan, StartServerEmitsStartEffect) {
    PowerUpAction a{};
    a.kind = PowerUpActionKind::start_server;
    a.need_assert = true;
    const auto plan = powerup_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, PowerUpSideEffectKind::StartServer);
    EXPECT_TRUE(plan.effects[0].need_assert);
}

TEST(PowerUpPlan, ConnectToMSEmitsConnectEffect) {
    PowerUpAction a{};
    a.kind = PowerUpActionKind::connect_to_ms;
    const auto plan = powerup_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_EQ(plan.effects[0].kind, PowerUpSideEffectKind::ConnectToMS);
}

TEST(PowerUpPlan, SendRegistMapAckToMSEmitsAckEffect) {
    PowerUpAction a{};
    a.kind = PowerUpActionKind::send_registmap_ack_to_ms;
    a.reply_protocol = 1u;
    a.target_port = 9000u;
    const auto plan = powerup_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_EQ(plan.effects[0].kind, PowerUpSideEffectKind::SendRegistMapAckToMS);
    EXPECT_EQ(plan.effects[0].reply_protocol, 1u);
    EXPECT_TRUE(powerup_effect_targets_remote(plan.effects[0]));
}

TEST(PowerUpPlan, SendUserCountToDistributeEmitsUserCntEffect) {
    PowerUpAction a{};
    a.kind = PowerUpActionKind::send_user_count_to_distribute;
    a.reply_protocol = 6u;
    a.user_count = 100u;
    const auto plan = powerup_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_EQ(plan.effects[0].kind, PowerUpSideEffectKind::SendUserCountToDistribute);
    EXPECT_EQ(plan.effects[0].user_count, 100u);
}

TEST(PowerUpPlan, SendRegistMapSynToMapEmitsSynEffect) {
    PowerUpAction a{};
    a.kind = PowerUpActionKind::send_registmap_syn_to_map;
    a.reply_protocol = 5u;
    a.map_num = 17u;
    const auto plan = powerup_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_EQ(plan.effects[0].kind, PowerUpSideEffectKind::SendRegistMapSynToMap);
    EXPECT_EQ(plan.effects[0].map_num, 17u);
}

TEST(PowerUpPlan, SetMapRegistEmitsSetEffect) {
    PowerUpAction a{};
    a.kind = PowerUpActionKind::set_map_regist;
    a.map_num = 25u;
    a.target_port = 9500u;
    const auto plan = powerup_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_EQ(plan.effects[0].kind, PowerUpSideEffectKind::SetMapRegist);
    EXPECT_TRUE(powerup_effect_targets_server_table(plan.effects[0]));
}

TEST(PowerUpPlan, MapUserUnRegistLoginEmitsUnregistEffect) {
    PowerUpAction a{};
    a.kind = PowerUpActionKind::map_user_unregist_login;
    const auto plan = powerup_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_EQ(plan.effects[0].kind, PowerUpSideEffectKind::MapUserUnRegistLogin);
    EXPECT_TRUE(powerup_effect_targets_server_table(plan.effects[0]));
}

TEST(PowerUpPlan, DropUnknownServerKindEmitsDrop) {
    PowerUpAction a{};
    a.kind = PowerUpActionKind::drop_unknown_server_kind;
    const auto plan = powerup_side_effect_plan(a);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, PowerUpSideEffectKind::Drop);
}

TEST(PowerUpPlan, DropUnreachableMSEmitsDrop) {
    PowerUpAction a{};
    a.kind = PowerUpActionKind::drop_unreachable_ms;
    const auto plan = powerup_side_effect_plan(a);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, PowerUpSideEffectKind::Drop);
}

// Classifier 1:1

TEST(PowerUpClassifierPlan, BootingNotifyEmitsForwardPlan) {
    PowerUpRequest req{};
    req.protocol = powerup_booting_notify;
    const auto action = classify_powerup(req);
    EXPECT_EQ(action.kind, PowerUpActionKind::forward_to_boot_manager);
    const auto plan = powerup_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, PowerUpSideEffectKind::ForwardToBootManager);
}

TEST(PowerUpClassifierPlan, BootListSynEmitsForwardPlan) {
    PowerUpRequest req{};
    req.protocol = powerup_bootlist_syn;
    const auto action = classify_powerup(req);
    EXPECT_EQ(action.kind, PowerUpActionKind::forward_to_boot_manager);
}

TEST(PowerUpClassifierPlan, ConnectSynMSUnreachableDrops) {
    PowerUpRequest req{};
    req.protocol = powerup_connect_syn;
    req.ms_reachable = false;
    const auto action = classify_powerup(req);
    EXPECT_EQ(action.kind, PowerUpActionKind::drop_unreachable_ms);
    const auto plan = powerup_side_effect_plan(action);
    EXPECT_TRUE(plan.drop);
}

TEST(PowerUpClassifierPlan, ConnectSynMSReachableForwards) {
    PowerUpRequest req{};
    req.protocol = powerup_connect_syn;
    req.ms_reachable = true;
    const auto action = classify_powerup(req);
    EXPECT_EQ(action.kind, PowerUpActionKind::forward_to_boot_manager);
}

TEST(PowerUpSelfInitClassifierPlan, MSUnreachableDrops) {
    PowerUpRequest req{};
    req.ms_reachable = false;
    const auto action = classify_powerup_self_init(req);
    EXPECT_EQ(action.kind, PowerUpActionKind::drop_unreachable_ms);
    const auto plan = powerup_side_effect_plan(action);
    EXPECT_TRUE(plan.drop);
}

TEST(PowerUpSelfInitClassifierPlan, SmallServerNumEmitsAddSelfBootList) {
    PowerUpRequest req{};
    req.server_num = 50u;
    req.self_port = 9000u;
    const auto action = classify_powerup_self_init(req);
    EXPECT_EQ(action.kind, PowerUpActionKind::add_self_boot_list);
    EXPECT_EQ(action.target_port, 9000u);
    const auto plan = powerup_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, PowerUpSideEffectKind::AddSelfBootList);
}

TEST(PowerUpSelfInitClassifierPlan, LargeServerNumEmitsStartServer) {
    PowerUpRequest req{};
    req.server_num = 200u;
    req.self_port = 9000u;
    const auto action = classify_powerup_self_init(req);
    EXPECT_EQ(action.kind, PowerUpActionKind::start_server);
    const auto plan = powerup_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, PowerUpSideEffectKind::StartServer);
}

TEST(PowerUpServerKindDispatch, AgentKindEmitsRegistMapAck) {
    PowerUpRequest req{};
    req.is_agent_kind = true;
    req.self_port = 9000u;
    const auto action = classify_powerup_server_kind_dispatch(req);
    EXPECT_EQ(action.kind, PowerUpActionKind::send_registmap_ack_to_ms);
    EXPECT_EQ(action.reply_protocol, 1u);
    EXPECT_EQ(action.target_port, 9000u);
    const auto plan = powerup_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, PowerUpSideEffectKind::SendRegistMapAckToMS);
}

TEST(PowerUpServerKindDispatch, MonitorKindEmitsRegistMapAck) {
    PowerUpRequest req{};
    req.is_monitor_kind = true;
    req.self_port = 9000u;
    const auto action = classify_powerup_server_kind_dispatch(req);
    EXPECT_EQ(action.kind, PowerUpActionKind::send_registmap_ack_to_ms);
}

TEST(PowerUpServerKindDispatch, DistributeKindEmitsUserCount) {
    PowerUpRequest req{};
    req.is_distribute_kind = true;
    req.self_port = 9000u;
    req.agent_user_count = 42u;
    const auto action = classify_powerup_server_kind_dispatch(req);
    EXPECT_EQ(action.kind, PowerUpActionKind::send_user_count_to_distribute);
    EXPECT_EQ(action.user_count, 42u);
    const auto plan = powerup_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, PowerUpSideEffectKind::SendUserCountToDistribute);
    EXPECT_EQ(plan.effects[0].user_count, 42u);
}

TEST(PowerUpServerKindDispatch, MapKindEmitsRegistMapSyn) {
    PowerUpRequest req{};
    req.is_map_kind = true;
    req.target_port = 8000u;
    const auto action = classify_powerup_server_kind_dispatch(req);
    EXPECT_EQ(action.kind, PowerUpActionKind::send_registmap_syn_to_map);
    EXPECT_EQ(action.target_port, 8000u);
    EXPECT_EQ(action.reply_protocol, 5u);
    const auto plan = powerup_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, PowerUpSideEffectKind::SendRegistMapSynToMap);
}

TEST(PowerUpServerKindDispatch, NoServerFoundDrops) {
    PowerUpRequest req{};
    req.target_server_found = false;
    const auto action = classify_powerup_server_kind_dispatch(req);
    EXPECT_EQ(action.kind, PowerUpActionKind::drop_unknown_server_kind);
    const auto plan = powerup_side_effect_plan(action);
    EXPECT_TRUE(plan.drop);
}

TEST(PowerUpRegistMapAck, ZeroMapNumEmitsForwardPlan) {
    PowerUpRequest req{};
    req.self_port = 5000u;
    const auto action = classify_powerup_registmap_ack(req, 0u);
    EXPECT_EQ(action.kind, PowerUpActionKind::forward_to_boot_manager);
    const auto plan = powerup_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, PowerUpSideEffectKind::ForwardToBootManager);
}

TEST(PowerUpRegistMapAck, NonMapPortEmitsSetMapRegist) {
    PowerUpRequest req{};
    req.self_port = 5000u;
    const auto action = classify_powerup_registmap_ack(req, 25u);
    EXPECT_EQ(action.kind, PowerUpActionKind::set_map_regist);
    EXPECT_EQ(action.map_num, 25u);
    const auto plan = powerup_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, PowerUpSideEffectKind::SetMapRegist);
}

TEST(PowerUpRegistMapAck, MapPortEmitsUnregistLogin) {
    PowerUpRequest req{};
    req.self_port = 8500u;
    const auto action = classify_powerup_registmap_ack(req, 25u);
    EXPECT_EQ(action.kind, PowerUpActionKind::map_user_unregist_login);
    const auto plan = powerup_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, PowerUpSideEffectKind::MapUserUnRegistLogin);
}