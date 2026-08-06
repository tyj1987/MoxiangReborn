// D4.111 -- AgentItem side-effect plan unit tests.
//
// 1:1 lock the legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_ITEMUserMsgParser (lines 4148-4206) + Ext + MP_ITEMServerMsgParser (4214-4286) + Ext.
//

#include <gtest/gtest.h>

#include "mxh/server/agent_item.hpp"
#include "mxh/server/agent_item_side_effect_plan.hpp"

using namespace mxh::server;

// USER side-effects

TEST(ItemUserPlan, ForwardToMapEmitsRawForward) {
    ItemUserAction a{};
    a.kind = ItemUserActionKind::forward_to_map;
    a.protocol = item_shopitem_changemap_syn;
    a.object_id = 7u;
    a.drop_payload = true;
    const auto plan = item_user_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, ItemUserSideEffectKind::ForwardRawToMap);
    EXPECT_EQ(plan.effects[0].reply_protocol, item_shopitem_changemap_syn);
    EXPECT_TRUE(plan.effects[0].drop_payload);
    EXPECT_TRUE(item_user_effect_targets_map(plan.effects[0]));
}

TEST(ItemUserPlan, ForwardNChangeSynToMapEmitsValidForward) {
    ItemUserAction a{};
    a.kind = ItemUserActionKind::forward_to_map_if_name_valid;
    a.protocol = item_shopitem_nchange_syn;
    const auto plan = item_user_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, ItemUserSideEffectKind::ForwardNChangeSynToMap);
    EXPECT_TRUE(item_user_effect_targets_map(plan.effects[0]));
}

TEST(ItemUserPlan, SendNackToUserEmitsNackEffect) {
    ItemUserAction a{};
    a.kind = ItemUserActionKind::send_nack_to_user;
    a.protocol = item_shopitem_nchange_nack;
    a.object_id = 11u;
    a.error_code = item_name_nack_code;
    const auto plan = item_user_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, ItemUserSideEffectKind::SendNackToUser);
    EXPECT_EQ(plan.effects[0].reply_protocol, item_shopitem_nchange_nack);
    EXPECT_EQ(plan.effects[0].error_code, item_name_nack_code);
    EXPECT_TRUE(item_user_effect_targets_user(plan.effects[0]));
}

TEST(ItemUserPlan, SendChaseLookupEmitsChaseEffect) {
    ItemUserAction a{};
    a.kind = ItemUserActionKind::send_chase_lookup;
    a.protocol = item_shopitem_chase_syn;
    a.object_id = 17u;
    const auto plan = item_user_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, ItemUserSideEffectKind::SendChaseLookup);
    EXPECT_TRUE(item_user_effect_targets_map(plan.effects[0]));
}

// USER Classifier 1:1

TEST(ItemUserClassifierPlan, ChangeMapSynForwardsToMap) {
    ItemUserRequest req{};
    req.protocol = item_shopitem_changemap_syn;
    const auto action = classify_item_user(req);
    EXPECT_EQ(action.kind, ItemUserActionKind::forward_to_map);
    EXPECT_TRUE(action.drop_payload);
    const auto plan = item_user_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, ItemUserSideEffectKind::ForwardRawToMap);
}

TEST(ItemUserClassifierPlan, NChangeNoUserEmitsNackPlan) {
    ItemUserRequest req{};
    req.protocol = item_shopitem_nchange_syn;
    req.user_found = false;
    const auto action = classify_item_user(req);
    EXPECT_EQ(action.kind, ItemUserActionKind::send_nack_to_user);
    EXPECT_EQ(action.error_code, item_name_nack_code);
    const auto plan = item_user_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, ItemUserSideEffectKind::SendNackToUser);
}

TEST(ItemUserClassifierPlan, NChangeShortNameEmitsNackPlan) {
    ItemUserRequest req{};
    req.protocol = item_shopitem_nchange_syn;
    req.user_found = true;
    req.name_length = 2u;
    const auto action = classify_item_user(req);
    EXPECT_EQ(action.kind, ItemUserActionKind::send_nack_to_user);
    const auto plan = item_user_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, ItemUserSideEffectKind::SendNackToUser);
}

TEST(ItemUserClassifierPlan, NChangeLongNameEmitsNackPlan) {
    ItemUserRequest req{};
    req.protocol = item_shopitem_nchange_syn;
    req.user_found = true;
    req.name_length = 25u;
    const auto action = classify_item_user(req);
    EXPECT_EQ(action.kind, ItemUserActionKind::send_nack_to_user);
}

TEST(ItemUserClassifierPlan, NChangeInvalidCharEmitsNackPlan) {
    ItemUserRequest req{};
    req.protocol = item_shopitem_nchange_syn;
    req.user_found = true;
    req.name_length = 8u;
    req.name_has_invalid_char = true;
    const auto action = classify_item_user(req);
    EXPECT_EQ(action.kind, ItemUserActionKind::send_nack_to_user);
    const auto plan = item_user_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, ItemUserSideEffectKind::SendNackToUser);
}

TEST(ItemUserClassifierPlan, NChangeValidEmitsForwardPlan) {
    ItemUserRequest req{};
    req.protocol = item_shopitem_nchange_syn;
    req.user_found = true;
    req.name_length = 8u;
    req.name_usable = true;
    const auto action = classify_item_user(req);
    EXPECT_EQ(action.kind, ItemUserActionKind::forward_to_map_if_name_valid);
    EXPECT_FALSE(action.drop_payload);
    const auto plan = item_user_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, ItemUserSideEffectKind::ForwardNChangeSynToMap);
}

TEST(ItemUserClassifierPlan, ChaseSynEmitsChaseLookupPlan) {
    ItemUserRequest req{};
    req.protocol = item_shopitem_chase_syn;
    const auto action = classify_item_user(req);
    EXPECT_EQ(action.kind, ItemUserActionKind::send_chase_lookup);
    const auto plan = item_user_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, ItemUserSideEffectKind::SendChaseLookup);
}

TEST(ItemUserClassifierPlan, UnknownUserProtocolEmitsForwardPlan) {
    ItemUserRequest req{};
    req.protocol = 99u;
    const auto action = classify_item_user(req);
    EXPECT_EQ(action.kind, ItemUserActionKind::forward_to_map);
    const auto plan = item_user_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, ItemUserSideEffectKind::ForwardRawToMap);
}

TEST(ItemUserClassifierPlan, ExtIsUnconditionalForwardToMap) {
    const auto action = classify_item_user_ext(99u);
    EXPECT_EQ(action.kind, ItemUserActionKind::forward_to_map);
    const auto plan = item_user_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, ItemUserSideEffectKind::ForwardRawToMap);
}

// SERVER side-effects

TEST(ItemServerPlan, ForwardToUserEmitsRawForward) {
    ItemServerAction a{};
    a.kind = ItemServerActionKind::forward_to_user;
    a.protocol = item_shopitem_chase_ack;
    a.object_id = 11u;
    const auto plan = item_server_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, ItemServerSideEffectKind::ForwardRawToUser);
    EXPECT_TRUE(item_server_effect_targets_user(plan.effects[0]));
}

TEST(ItemServerPlan, SendChaseNackToUserEmitsChaseNackEffect) {
    ItemServerAction a{};
    a.kind = ItemServerActionKind::send_chase_nack_to_user;
    a.protocol = item_shopitem_chase_nack;
    a.object_id = 11u;
    a.alternate_data = item_chase_nack_data;
    const auto plan = item_server_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, ItemServerSideEffectKind::SendChaseNackToUser);
    EXPECT_EQ(plan.effects[0].alternate_data, item_chase_nack_data);
    EXPECT_TRUE(item_server_effect_targets_user(plan.effects[0]));
}

TEST(ItemServerPlan, ShoutAckWithBroadcastSetsBroadcastFlag) {
    ItemServerAction a{};
    a.kind = ItemServerActionKind::shout_ack_with_broadcast;
    a.protocol = item_shopitem_shout_ack;
    a.broadcast_shout = true;
    const auto plan = item_server_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_EQ(plan.effects[0].kind, ItemServerSideEffectKind::ShoutAckWithBroadcast);
    EXPECT_TRUE(plan.effects[0].broadcast_shout);
    EXPECT_TRUE(item_server_effect_targets_client(plan.effects[0]));
}

TEST(ItemServerPlan, ShoutAddOnlyEmitsAddEffect) {
    ItemServerAction a{};
    a.kind = ItemServerActionKind::shout_add_only;
    a.protocol = item_shopitem_shout_sendserver;
    const auto plan = item_server_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_EQ(plan.effects[0].kind, ItemServerSideEffectKind::ShoutAddOnly);
    EXPECT_FALSE(item_server_effect_targets_user(plan.effects[0]));
    EXPECT_FALSE(item_server_effect_targets_client(plan.effects[0]));
}

TEST(ItemServerPlan, ForwardToClientEmitsClientForward) {
    ItemServerAction a{};
    a.kind = ItemServerActionKind::forward_to_client;
    a.protocol = item_shopitem_changemap_syn;
    const auto plan = item_server_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_EQ(plan.effects[0].kind, ItemServerSideEffectKind::ForwardRawToClient);
    EXPECT_TRUE(item_server_effect_targets_client(plan.effects[0]));
}

// SERVER Classifier 1:1

TEST(ItemServerClassifierPlan, ChangeMapSynForwardsToClient) {
    ItemServerRequest req{};
    req.protocol = item_shopitem_changemap_syn;
    const auto action = classify_item_server(req);
    EXPECT_EQ(action.kind, ItemServerActionKind::forward_to_client);
    const auto plan = item_server_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, ItemServerSideEffectKind::ForwardRawToClient);
}

TEST(ItemServerClassifierPlan, ChaseAckForwardsToUser) {
    ItemServerRequest req{};
    req.protocol = item_shopitem_chase_ack;
    req.data = 11u;
    const auto action = classify_item_server(req);
    EXPECT_EQ(action.kind, ItemServerActionKind::forward_to_user);
    EXPECT_EQ(action.object_id, 11u);
    const auto plan = item_server_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, ItemServerSideEffectKind::ForwardRawToUser);
}

TEST(ItemServerClassifierPlan, ChaseNackSendsChaseNack) {
    ItemServerRequest req{};
    req.protocol = item_shopitem_chase_nack;
    const auto action = classify_item_server(req);
    EXPECT_EQ(action.kind, ItemServerActionKind::send_chase_nack_to_user);
    EXPECT_EQ(action.alternate_data, item_chase_nack_data);
    const auto plan = item_server_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, ItemServerSideEffectKind::SendChaseNackToUser);
}

TEST(ItemServerClassifierPlan, ShoutAckWithBroadcastFlag) {
    ItemServerRequest req{};
    req.protocol = item_shopitem_shout_ack;
    req.shout_buffer_full = false;
    const auto action = classify_item_server(req);
    EXPECT_EQ(action.kind, ItemServerActionKind::shout_ack_with_broadcast);
    EXPECT_FALSE(action.broadcast_shout);
    const auto plan = item_server_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, ItemServerSideEffectKind::ShoutAckWithBroadcast);
}

TEST(ItemServerClassifierPlan, ShoutAckBufferFullSetsBroadcastTrue) {
    ItemServerRequest req{};
    req.protocol = item_shopitem_shout_ack;
    req.shout_buffer_full = true;
    const auto action = classify_item_server(req);
    EXPECT_EQ(action.kind, ItemServerActionKind::shout_ack_with_broadcast);
    EXPECT_TRUE(action.broadcast_shout);
}

TEST(ItemServerClassifierPlan, ShoutSendServerEmitsAddOnly) {
    ItemServerRequest req{};
    req.protocol = item_shopitem_shout_sendserver;
    const auto action = classify_item_server(req);
    EXPECT_EQ(action.kind, ItemServerActionKind::shout_add_only);
    const auto plan = item_server_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, ItemServerSideEffectKind::ShoutAddOnly);
}

TEST(ItemServerClassifierPlan, UnknownServerProtocolEmitsForwardToClient) {
    ItemServerRequest req{};
    req.protocol = 99u;
    const auto action = classify_item_server(req);
    EXPECT_EQ(action.kind, ItemServerActionKind::forward_to_client);
    const auto plan = item_server_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, ItemServerSideEffectKind::ForwardRawToClient);
}

TEST(ItemServerClassifierPlan, ExtIsUnconditionalForwardToClient) {
    const auto action = classify_item_server_ext(99u);
    EXPECT_EQ(action.kind, ItemServerActionKind::forward_to_client);
    const auto plan = item_server_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, ItemServerSideEffectKind::ForwardRawToClient);
}