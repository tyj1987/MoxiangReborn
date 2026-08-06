//
// D4.101 -- AgentGuildUnion side-effect plan unit tests.
//
// 1:1 lock the legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_GUILD_UNIONUserMsgParser (lines 4735-4763) and MP_GUILD_UNIONServerMsgParser
// (lines 4765-4790). Each test pins one branch of the legacy dispatch to its modern
// side-effect plan output so future drift triggers a test failure.
//

#include <gtest/gtest.h>

#include "mxh/server/agent_guild_union.hpp"
#include "mxh/server/agent_guild_union_side_effect_plan.hpp"

using namespace mxh::server;

// ---------------------------------------------------------------------
// USER side-effects
// ---------------------------------------------------------------------

TEST(GuildUnionUserPlan, DropNoUserEmitsDropEffect) {
    GuildUnionAction action{};
    action.kind = GuildUnionActionKind::drop_no_user;
    action.protocol = guild_union_create_syn;
    const auto plan = guild_union_user_side_effect_plan(action);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, GuildUnionUserSideEffectKind::Drop);
    EXPECT_EQ(plan.effects[0].error_code, 0u);
}

TEST(GuildUnionUserPlan, CreateNackInvalidNameEmitsNackWithErrorCode) {
    GuildUnionAction action{};
    action.kind = GuildUnionActionKind::send_create_nack_to_user;
    action.protocol = guild_union_create_nack;
    action.error_code = guild_union_err_not_valid_name;
    const auto plan = guild_union_user_side_effect_plan(action);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, GuildUnionUserSideEffectKind::SendCreateNackToUser);
    EXPECT_EQ(plan.effects[0].error_code, guild_union_err_not_valid_name);
    EXPECT_TRUE(guild_union_user_effect_targets_user(plan.effects[0]));
    EXPECT_FALSE(guild_union_user_effect_targets_map(plan.effects[0]));
}

TEST(GuildUnionUserPlan, CreateSynForwardsToMap) {
    GuildUnionAction action{};
    action.kind = GuildUnionActionKind::forward_to_map;
    action.protocol = guild_union_create_syn;
    const auto plan = guild_union_user_side_effect_plan(action);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, GuildUnionUserSideEffectKind::ForwardRawToMap);
    EXPECT_TRUE(guild_union_user_effect_targets_map(plan.effects[0]));
    EXPECT_FALSE(guild_union_user_effect_targets_user(plan.effects[0]));
}

TEST(GuildUnionUserPlan, DefaultProtocolForwardsToMap) {
    GuildUnionAction action{};
    action.kind = GuildUnionActionKind::forward_to_map;
    action.protocol = 99u;
    const auto plan = guild_union_user_side_effect_plan(action);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, GuildUnionUserSideEffectKind::ForwardRawToMap);
}

// ---------------------------------------------------------------------
// SERVER side-effects
// ---------------------------------------------------------------------

TEST(GuildUnionServerPlan, CreateNotifyBroadcastsToOtherMaps) {
    GuildUnionServerAction action{};
    action.kind = GuildUnionServerActionKind::broadcast_to_other_maps;
    action.protocol = guild_union_create_notify_to_map;
    const auto plan = guild_union_server_side_effect_plan(action);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, GuildUnionServerSideEffectKind::BroadcastToOtherMaps);
    EXPECT_TRUE(guild_union_server_effect_targets_map(plan.effects[0]));
}

TEST(GuildUnionServerPlan, DestroyNotifyBroadcastsToOtherMaps) {
    GuildUnionServerAction action{};
    action.kind = GuildUnionServerActionKind::broadcast_to_other_maps;
    action.protocol = guild_union_destroy_notify_to_map;
    const auto plan = guild_union_server_side_effect_plan(action);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, GuildUnionServerSideEffectKind::BroadcastToOtherMaps);
}

TEST(GuildUnionServerPlan, InviteAcceptNotifyBroadcastsToOtherMaps) {
    GuildUnionServerAction action{};
    action.kind = GuildUnionServerActionKind::broadcast_to_other_maps;
    action.protocol = guild_union_invite_accept_notify_to_map;
    const auto plan = guild_union_server_side_effect_plan(action);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, GuildUnionServerSideEffectKind::BroadcastToOtherMaps);
}

TEST(GuildUnionServerPlan, AddNotifyBroadcastsToOtherMaps) {
    GuildUnionServerAction action{};
    action.kind = GuildUnionServerActionKind::broadcast_to_other_maps;
    action.protocol = guild_union_add_notify_to_map;
    const auto plan = guild_union_server_side_effect_plan(action);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, GuildUnionServerSideEffectKind::BroadcastToOtherMaps);
}

TEST(GuildUnionServerPlan, RemoveNotifyBroadcastsToOtherMaps) {
    GuildUnionServerAction action{};
    action.kind = GuildUnionServerActionKind::broadcast_to_other_maps;
    action.protocol = guild_union_remove_notify_to_map;
    const auto plan = guild_union_server_side_effect_plan(action);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, GuildUnionServerSideEffectKind::BroadcastToOtherMaps);
}

TEST(GuildUnionServerPlan, SecedeNotifyBroadcastsToOtherMaps) {
    GuildUnionServerAction action{};
    action.kind = GuildUnionServerActionKind::broadcast_to_other_maps;
    action.protocol = guild_union_secede_notify_to_map;
    const auto plan = guild_union_server_side_effect_plan(action);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, GuildUnionServerSideEffectKind::BroadcastToOtherMaps);
}

TEST(GuildUnionServerPlan, MarkRegistNotifyBroadcastsToOtherMaps) {
    GuildUnionServerAction action{};
    action.kind = GuildUnionServerActionKind::broadcast_to_other_maps;
    action.protocol = guild_union_mark_regist_notify_to_map;
    const auto plan = guild_union_server_side_effect_plan(action);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, GuildUnionServerSideEffectKind::BroadcastToOtherMaps);
}

TEST(GuildUnionServerPlan, UnknownProtocolDrops) {
    GuildUnionServerAction action{};
    action.kind = GuildUnionServerActionKind::drop_unknown;
    action.protocol = 99u;
    const auto plan = guild_union_server_side_effect_plan(action);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, GuildUnionServerSideEffectKind::Drop);
    EXPECT_FALSE(guild_union_server_effect_targets_map(plan.effects[0]));
}

// ---------------------------------------------------------------------
// Coupling: classifier -> plan integration.
// ---------------------------------------------------------------------

TEST(GuildUnionCoupling, CreateSynInvalidNameProducesNackPlan) {
    GuildUnionRequest req{};
    req.protocol = guild_union_create_syn;
    req.user_found = true;
    req.name_usable = false;
    req.has_invalid_char = true;
    const auto action = classify_guild_union_user(req);
    const auto plan = guild_union_user_side_effect_plan(action);
    EXPECT_EQ(action.kind, GuildUnionActionKind::send_create_nack_to_user);
    EXPECT_EQ(action.error_code, guild_union_err_not_valid_name);
    EXPECT_EQ(plan.effects[0].kind, GuildUnionUserSideEffectKind::SendCreateNackToUser);
    EXPECT_EQ(plan.effects[0].error_code, guild_union_err_not_valid_name);
}

TEST(GuildUnionCoupling, CreateSynValidNameProducesForwardPlan) {
    GuildUnionRequest req{};
    req.protocol = guild_union_create_syn;
    req.user_found = true;
    req.name_usable = true;
    req.has_invalid_char = false;
    const auto action = classify_guild_union_user(req);
    const auto plan = guild_union_user_side_effect_plan(action);
    EXPECT_EQ(action.kind, GuildUnionActionKind::forward_to_map);
    EXPECT_EQ(action.protocol, guild_union_create_syn);
    EXPECT_EQ(plan.effects[0].kind, GuildUnionUserSideEffectKind::ForwardRawToMap);
}

TEST(GuildUnionCoupling, NoUserProducesDropPlan) {
    GuildUnionRequest req{};
    req.protocol = guild_union_create_syn;
    req.user_found = false;
    const auto action = classify_guild_union_user(req);
    const auto plan = guild_union_user_side_effect_plan(action);
    EXPECT_EQ(action.kind, GuildUnionActionKind::drop_no_user);
    EXPECT_EQ(plan.effects[0].kind, GuildUnionUserSideEffectKind::Drop);
}

TEST(GuildUnionCoupling, AddNotifyServerProducesBroadcastPlan) {
    GuildUnionServerRequest req{};
    req.protocol = guild_union_add_notify_to_map;
    const auto action = classify_guild_union_server(req);
    const auto plan = guild_union_server_side_effect_plan(action);
    EXPECT_EQ(action.kind, GuildUnionServerActionKind::broadcast_to_other_maps);
    EXPECT_EQ(plan.effects[0].kind, GuildUnionServerSideEffectKind::BroadcastToOtherMaps);
}

TEST(GuildUnionCoupling, UnknownServerProtocolProducesDropPlan) {
    GuildUnionServerRequest req{};
    req.protocol = 99u;
    const auto action = classify_guild_union_server(req);
    const auto plan = guild_union_server_side_effect_plan(action);
    EXPECT_EQ(action.kind, GuildUnionServerActionKind::drop_unknown);
    EXPECT_EQ(plan.effects[0].kind, GuildUnionServerSideEffectKind::Drop);
}
