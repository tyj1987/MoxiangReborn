// D4.112 -- AgentGuild side-effect plan unit tests.
//
// 1:1 lock the legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_GUILDUserMsgParser.
//

#include <gtest/gtest.h>

#include "mxh/server/agent_guild.hpp"
#include "mxh/server/agent_guild_side_effect_plan.hpp"

using namespace mxh::server;

// USER

TEST(GuildUserPlan, ForwardEmitsRawForward) {
    GuildAction a{};
    a.kind = GuildActionKind::forward;
    a.protocol = guild_create_syn;
    a.object_id = 11u;
    const auto plan = guild_user_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, GuildUserSideEffectKind::ForwardRawToMap);
    EXPECT_EQ(plan.effects[0].reply_protocol, guild_create_syn);
    EXPECT_TRUE(guild_user_effect_targets_map(plan.effects[0]));
    EXPECT_FALSE(guild_user_effect_targets_user(plan.effects[0]));
}

TEST(GuildUserPlan, SendNackEmitsNackEffect) {
    GuildAction a{};
    a.kind = GuildActionKind::send_nack;
    a.protocol = guild_create_nack;
    a.object_id = 11u;
    a.error_code = guild_err_create_name;
    const auto plan = guild_user_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, GuildUserSideEffectKind::SendNackToUser);
    EXPECT_EQ(plan.effects[0].reply_protocol, guild_create_nack);
    EXPECT_EQ(plan.effects[0].error_code, guild_err_create_name);
    EXPECT_TRUE(guild_user_effect_targets_user(plan.effects[0]));
}

TEST(GuildUserPlan, GiveNicknameNackEmitsNickFilterError) {
    GuildAction a{};
    a.kind = GuildActionKind::send_nack;
    a.protocol = guild_givenickname_nack;
    a.object_id = 13u;
    a.error_code = guild_err_nick_filter;
    const auto plan = guild_user_side_effect_plan(a);
    EXPECT_EQ(plan.effects[0].kind, GuildUserSideEffectKind::SendNackToUser);
    EXPECT_EQ(plan.effects[0].reply_protocol, guild_givenickname_nack);
    EXPECT_EQ(plan.effects[0].error_code, guild_err_nick_filter);
}

// SERVER (simple pass-through)

TEST(GuildServerPlan, ForwardEmitsRawForwardToClient) {
    GuildAction a{};
    a.kind = GuildActionKind::forward;
    a.protocol = 99u;
    const auto plan = guild_server_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, GuildServerSideEffectKind::ForwardRawToClient);
    EXPECT_EQ(plan.effects[0].protocol, 99u);
    EXPECT_TRUE(guild_server_effect_targets_client(plan.effects[0]));
}

// Classifier 1:1

TEST(GuildUserClassifierPlan, CreateSynWithValidNameForwardsToMap) {
    GuildUserRequest req{};
    req.object_id = 11u;
    req.usable_name = true;
    req.is_nickname_path = false;
    const auto action = classify_guild_user(req);
    EXPECT_EQ(action.kind, GuildActionKind::forward);
    EXPECT_EQ(action.protocol, guild_create_syn);
    EXPECT_EQ(action.error_code, 0u);
    const auto plan = guild_user_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, GuildUserSideEffectKind::ForwardRawToMap);
}

TEST(GuildUserClassifierPlan, CreateSynInvalidCharEmitsNack) {
    GuildUserRequest req{};
    req.object_id = 11u;
    req.has_invalid_char = true;
    const auto action = classify_guild_user(req);
    EXPECT_EQ(action.kind, GuildActionKind::send_nack);
    EXPECT_EQ(action.protocol, guild_create_nack);
    EXPECT_EQ(action.error_code, guild_err_create_name);
    const auto plan = guild_user_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, GuildUserSideEffectKind::SendNackToUser);
}

TEST(GuildUserClassifierPlan, CreateSynUnusableNameEmitsNack) {
    GuildUserRequest req{};
    req.object_id = 11u;
    req.usable_name = false;
    const auto action = classify_guild_user(req);
    EXPECT_EQ(action.kind, GuildActionKind::send_nack);
    EXPECT_EQ(action.error_code, guild_err_create_name);
}

TEST(GuildUserClassifierPlan, GiveNicknameSynWithValidNameForwardsToMap) {
    GuildUserRequest req{};
    req.object_id = 11u;
    req.usable_name = true;
    req.is_nickname_path = true;
    const auto action = classify_guild_user(req);
    EXPECT_EQ(action.kind, GuildActionKind::forward);
    EXPECT_EQ(action.protocol, guild_givenickname_syn);
    const auto plan = guild_user_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, GuildUserSideEffectKind::ForwardRawToMap);
}

TEST(GuildUserClassifierPlan, GiveNicknameInvalidCharEmitsNackWithNickFilter) {
    GuildUserRequest req{};
    req.object_id = 11u;
    req.has_invalid_char = true;
    req.is_nickname_path = true;
    const auto action = classify_guild_user(req);
    EXPECT_EQ(action.kind, GuildActionKind::send_nack);
    EXPECT_EQ(action.protocol, guild_givenickname_nack);
    EXPECT_EQ(action.error_code, guild_err_nick_filter);
    const auto plan = guild_user_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, GuildUserSideEffectKind::SendNackToUser);
    EXPECT_EQ(plan.effects[0].error_code, guild_err_nick_filter);
}

TEST(GuildServerClassifierPlan, DefaultServerIsForwardToClient) {
    const auto action = classify_guild_server_default(99u);
    EXPECT_EQ(action.kind, GuildActionKind::forward);
    EXPECT_EQ(action.protocol, 99u);
    const auto plan = guild_server_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, GuildServerSideEffectKind::ForwardRawToClient);
}