// D4.108 -- AgentMurimNet side-effect plan unit tests.
//
// 1:1 lock the legacy side-effects applied by [Server]Agent/MNNetworkMsgParser.cpp.
// Each test pins one branch of the legacy dispatch to its modern side-effect plan
// output so future drift triggers a test failure.
//

#include <gtest/gtest.h>

#include "mxh/server/agent_murimnet.hpp"
#include "mxh/server/agent_murimnet_side_effect_plan.hpp"

using namespace mxh::server;

// ---------------------------------------------------------------------
// Plan unit tests
// ---------------------------------------------------------------------

TEST(MurimNetPlan, DropUnknownEmitsDropEffect) {
    MurimNetAction a{};
    a.kind = MurimNetActionKind::drop_unknown;
    a.protocol = 99u;
    a.character_id = 7u;
    const auto plan = murimnet_side_effect_plan(a);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, MurimNetSideEffectKind::Drop);
    EXPECT_EQ(plan.effects[0].reply_protocol, 99u);
    EXPECT_EQ(plan.effects[0].character_id, 7u);
    EXPECT_FALSE(murimnet_effect_targets_map(plan.effects[0]));
}

TEST(MurimNetPlan, ForwardToMapEmitsRawForwardEffect) {
    MurimNetAction a{};
    a.kind = MurimNetActionKind::forward_to_map;
    a.protocol = murimnet_connect_syn;
    a.character_id = 11u;
    const auto plan = murimnet_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, MurimNetSideEffectKind::ForwardRawToMap);
    EXPECT_EQ(plan.effects[0].reply_protocol, murimnet_connect_syn);
    EXPECT_TRUE(murimnet_effect_targets_map(plan.effects[0]));
}

TEST(MurimNetPlan, SendAckEmitsAckEffect) {
    MurimNetAction a{};
    a.kind = MurimNetActionKind::send_ack;
    a.protocol = murimnet_changetomurimnet_ack;
    a.character_id = 13u;
    const auto plan = murimnet_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, MurimNetSideEffectKind::SendAckToMap);
    EXPECT_EQ(plan.effects[0].reply_protocol, murimnet_changetomurimnet_ack);
    EXPECT_TRUE(murimnet_effect_targets_map(plan.effects[0]));
}

TEST(MurimNetPlan, SendNackEmitsNackEffect) {
    MurimNetAction a{};
    a.kind = MurimNetActionKind::send_nack;
    a.protocol = murimnet_changetomurimnet_nack;
    a.character_id = 17u;
    const auto plan = murimnet_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, MurimNetSideEffectKind::SendNackToMap);
    EXPECT_EQ(plan.effects[0].reply_protocol, murimnet_changetomurimnet_nack);
    EXPECT_TRUE(murimnet_effect_targets_map(plan.effects[0]));
}

// ---------------------------------------------------------------------
// Classifier 1:1 tests (USER)
// ---------------------------------------------------------------------

TEST(MurimNetUserClassifierPlan, ChangeToMurimNetWithPortForwardsToMap) {
    MurimNetUserRequest req{};
    req.protocol = murimnet_changetomurimnet_syn;
    req.character_id = 11u;
    req.lookup.port = 9060u;
    const auto action = classify_murimnet_user(req);
    EXPECT_EQ(action.kind, MurimNetActionKind::forward_to_map);
    EXPECT_EQ(action.protocol, murimnet_changetomurimnet_syn);
    const auto plan = murimnet_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, MurimNetSideEffectKind::ForwardRawToMap);
}

TEST(MurimNetUserClassifierPlan, ChangeToMurimNetNoPortEmitsNackPlan) {
    MurimNetUserRequest req{};
    req.protocol = murimnet_changetomurimnet_syn;
    req.character_id = 11u;
    req.lookup.port.reset();
    const auto action = classify_murimnet_user(req);
    EXPECT_EQ(action.kind, MurimNetActionKind::send_nack);
    EXPECT_EQ(action.protocol, murimnet_changetomurimnet_nack);
    const auto plan = murimnet_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, MurimNetSideEffectKind::SendNackToMap);
    EXPECT_EQ(plan.effects[0].reply_protocol, murimnet_changetomurimnet_nack);
}

TEST(MurimNetUserClassifierPlan, ConnectSynForwardsToMap) {
    MurimNetUserRequest req{};
    req.protocol = murimnet_connect_syn;
    const auto action = classify_murimnet_user(req);
    EXPECT_EQ(action.kind, MurimNetActionKind::forward_to_map);
    const auto plan = murimnet_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, MurimNetSideEffectKind::ForwardRawToMap);
}

TEST(MurimNetUserClassifierPlan, ReconnectSynForwardsToMap) {
    MurimNetUserRequest req{};
    req.protocol = murimnet_reconnect_syn;
    const auto action = classify_murimnet_user(req);
    EXPECT_EQ(action.kind, MurimNetActionKind::forward_to_map);
    const auto plan = murimnet_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, MurimNetSideEffectKind::ForwardRawToMap);
}

TEST(MurimNetUserClassifierPlan, PrTeamChangeForwardsToMap) {
    MurimNetUserRequest req{};
    req.protocol = murimnet_pr_teamchange_syn;
    const auto action = classify_murimnet_user(req);
    EXPECT_EQ(action.kind, MurimNetActionKind::forward_to_map);
    const auto plan = murimnet_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, MurimNetSideEffectKind::ForwardRawToMap);
}

TEST(MurimNetUserClassifierPlan, ChnlModeChangeForwardsToMap) {
    MurimNetUserRequest req{};
    req.protocol = murimnet_chnl_modechange;
    const auto action = classify_murimnet_user(req);
    EXPECT_EQ(action.kind, MurimNetActionKind::forward_to_map);
    const auto plan = murimnet_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, MurimNetSideEffectKind::ForwardRawToMap);
}

TEST(MurimNetUserClassifierPlan, ChatAllForwardsToMap) {
    MurimNetUserRequest req{};
    req.protocol = murimnet_chat_all;
    const auto action = classify_murimnet_user(req);
    EXPECT_EQ(action.kind, MurimNetActionKind::forward_to_map);
    const auto plan = murimnet_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, MurimNetSideEffectKind::ForwardRawToMap);
}

TEST(MurimNetUserClassifierPlan, NotifyToMnPlayerLogoutForwardsToMap) {
    MurimNetUserRequest req{};
    req.protocol = murimnet_notifytomn_player_logout;
    const auto action = classify_murimnet_user(req);
    EXPECT_EQ(action.kind, MurimNetActionKind::forward_to_map);
    const auto plan = murimnet_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, MurimNetSideEffectKind::ForwardRawToMap);
}

TEST(MurimNetUserClassifierPlan, UnknownProtocolDrops) {
    MurimNetUserRequest req{};
    req.protocol = 200u;
    const auto action = classify_murimnet_user(req);
    EXPECT_EQ(action.kind, MurimNetActionKind::drop_unknown);
    const auto plan = murimnet_side_effect_plan(action);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, MurimNetSideEffectKind::Drop);
}

// ---------------------------------------------------------------------
// Classifier 1:1 tests (SERVER)
// ---------------------------------------------------------------------

TEST(MurimNetServerClassifierPlan, ChangeToMurimNetAckWithPortEmitsAckPlan) {
    MurimNetServerRequest req{};
    req.protocol = murimnet_changetomurimnet_ack;
    req.lookup.port = 9060u;
    const auto action = classify_murimnet_server(req);
    EXPECT_EQ(action.kind, MurimNetActionKind::send_ack);
    const auto plan = murimnet_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, MurimNetSideEffectKind::SendAckToMap);
}

TEST(MurimNetServerClassifierPlan, ChangeToMurimNetAckNoPortEmitsNackPlan) {
    MurimNetServerRequest req{};
    req.protocol = murimnet_changetomurimnet_ack;
    req.lookup.port.reset();
    const auto action = classify_murimnet_server(req);
    EXPECT_EQ(action.kind, MurimNetActionKind::send_nack);
    EXPECT_EQ(action.protocol, murimnet_changetomurimnet_nack);
    const auto plan = murimnet_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, MurimNetSideEffectKind::SendNackToMap);
    EXPECT_EQ(plan.effects[0].reply_protocol, murimnet_changetomurimnet_nack);
}

TEST(MurimNetServerClassifierPlan, ReturnToMurimNetAckWithPortEmitsAckPlan) {
    MurimNetServerRequest req{};
    req.protocol = murimnet_returntomurimnet_ack;
    req.lookup.port = 9060u;
    const auto action = classify_murimnet_server(req);
    EXPECT_EQ(action.kind, MurimNetActionKind::send_ack);
    const auto plan = murimnet_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, MurimNetSideEffectKind::SendAckToMap);
}

TEST(MurimNetServerClassifierPlan, ReturnToMurimNetAckNoPortEmitsNackPlan) {
    MurimNetServerRequest req{};
    req.protocol = murimnet_returntomurimnet_ack;
    req.lookup.port.reset();
    const auto action = classify_murimnet_server(req);
    EXPECT_EQ(action.kind, MurimNetActionKind::send_nack);
    EXPECT_EQ(action.protocol, murimnet_returntomurimnet_nack);
    const auto plan = murimnet_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, MurimNetSideEffectKind::SendNackToMap);
}

TEST(MurimNetServerClassifierPlan, PrStartForwardsToMap) {
    MurimNetServerRequest req{};
    req.protocol = murimnet_pr_start;
    const auto action = classify_murimnet_server(req);
    EXPECT_EQ(action.kind, MurimNetActionKind::forward_to_map);
    const auto plan = murimnet_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, MurimNetSideEffectKind::ForwardRawToMap);
}

TEST(MurimNetServerClassifierPlan, DisconnectAckForwardsToMap) {
    MurimNetServerRequest req{};
    req.protocol = murimnet_disconnect_ack;
    const auto action = classify_murimnet_server(req);
    EXPECT_EQ(action.kind, MurimNetActionKind::forward_to_map);
    const auto plan = murimnet_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, MurimNetSideEffectKind::ForwardRawToMap);
}

TEST(MurimNetServerClassifierPlan, NotifyToMnGameEndForwardsToMap) {
    MurimNetServerRequest req{};
    req.protocol = murimnet_notifytomn_gameend;
    const auto action = classify_murimnet_server(req);
    EXPECT_EQ(action.kind, MurimNetActionKind::forward_to_map);
    const auto plan = murimnet_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, MurimNetSideEffectKind::ForwardRawToMap);
}

TEST(MurimNetServerClassifierPlan, UnknownServerProtocolDrops) {
    MurimNetServerRequest req{};
    req.protocol = 200u;
    const auto action = classify_murimnet_server(req);
    EXPECT_EQ(action.kind, MurimNetActionKind::drop_unknown);
    const auto plan = murimnet_side_effect_plan(action);
    EXPECT_TRUE(plan.drop);
}

// ---------------------------------------------------------------------
// Combined user+server forward protocols coverage
// ---------------------------------------------------------------------

TEST(MurimNetClassifierPlan, AllUserForwardProtocolsRouteToForwardToMap) {
    for (auto proto : {murimnet_connect_syn, murimnet_reconnect_syn, murimnet_pr_teamchange_syn, murimnet_chnl_modechange, murimnet_chat_all, murimnet_notifytomn_player_logout}) {
        MurimNetUserRequest req{};
        req.protocol = proto;
        const auto action = classify_murimnet_user(req);
        EXPECT_EQ(action.kind, MurimNetActionKind::forward_to_map);
        const auto plan = murimnet_side_effect_plan(action);
        EXPECT_EQ(plan.effects[0].kind, MurimNetSideEffectKind::ForwardRawToMap);
    }
}

TEST(MurimNetClassifierPlan, AllServerForwardProtocolsRouteToForwardToMap) {
    for (auto proto : {murimnet_pr_start, murimnet_disconnect_ack, murimnet_chat_all, murimnet_notifytomn_gameend}) {
        MurimNetServerRequest req{};
        req.protocol = proto;
        const auto action = classify_murimnet_server(req);
        EXPECT_EQ(action.kind, MurimNetActionKind::forward_to_map);
        const auto plan = murimnet_side_effect_plan(action);
        EXPECT_EQ(plan.effects[0].kind, MurimNetSideEffectKind::ForwardRawToMap);
    }
}