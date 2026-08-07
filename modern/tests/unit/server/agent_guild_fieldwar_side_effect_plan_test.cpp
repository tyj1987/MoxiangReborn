// D4.169 -- 1:1 lock the legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_GUILD_FIELDWARUserMsgParser (lines 4014-4041) and MP_GUILD_FIELDWARServerMsgParser
// (lines 4043-4090). Each test pins one branch of the legacy dispatch to its
// modern side-effect plan output so future drift triggers a test failure.

#include <gtest/gtest.h>

#include "mxh/server/agent_guild_fieldwar.hpp"
#include "mxh/server/agent_guild_fieldwar_side_effect_plan.hpp"

using namespace mxh::server;

namespace {
constexpr std::uint32_t kObjectId = 0xCAFEBABEu;
constexpr std::uint32_t kData = 0x11223344u;
constexpr std::uint32_t kData1 = 0x55667788u;
constexpr std::uint32_t kData2 = 0x99AABBCCu;
constexpr std::uint32_t kConnId = 0x200u;
}

TEST(GuildFieldWarPlan, UserDeclareWithoutUserForwardsToMapAsNoUser) {
    GuildFieldWarUserRequest r;
    r.protocol = guild_fieldwar_declare;
    r.dw_object_id = kObjectId;
    r.user_found = false;
    auto a = classify_guild_fieldwar_user(r);
    EXPECT_EQ(a.kind, GuildFieldWarUserActionKind::forward_to_map_server_no_user);
    const auto plan = guild_fieldwar_user_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    EXPECT_TRUE(plan.forward_to_map_server);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, GuildFieldWarSideEffectKind::ForwardToMapServerNoUser);
    EXPECT_EQ(plan.effects[0].reply_protocol, guild_fieldwar_declare);
    EXPECT_EQ(plan.effects[0].dw_object_id, kObjectId);
}

TEST(GuildFieldWarPlan, UserDeclareWithUserEmitsGuildMasterLoginCheck) {
    GuildFieldWarUserRequest r;
    r.protocol = guild_fieldwar_declare;
    r.dw_object_id = kObjectId;
    r.dw_data = kData;
    r.user_found = true;
    auto a = classify_guild_fieldwar_user(r);
    EXPECT_EQ(a.kind, GuildFieldWarUserActionKind::check_guild_master_login);
    const auto plan = guild_fieldwar_user_side_effect_plan(a);
    EXPECT_TRUE(plan.check_guild_master_login);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, GuildFieldWarSideEffectKind::CheckGuildMasterLogin);
    EXPECT_EQ(plan.effects[0].dw_data, kData);
    EXPECT_EQ(plan.effects[0].reply_protocol, guild_fieldwar_declare);
}

TEST(GuildFieldWarPlan, UserSuggestEndWithUserEmitsGuildMasterLoginCheck) {
    GuildFieldWarUserRequest r;
    r.protocol = guild_fieldwar_suggestend;
    r.user_found = true;
    auto a = classify_guild_fieldwar_user(r);
    EXPECT_EQ(a.kind, GuildFieldWarUserActionKind::check_guild_master_login);
    const auto plan = guild_fieldwar_user_side_effect_plan(a);
    EXPECT_TRUE(plan.check_guild_master_login);
}

TEST(GuildFieldWarPlan, UserDeclareAcceptWithUserEmitsMoneyCheck) {
    GuildFieldWarUserRequest r;
    r.protocol = guild_fieldwar_declare_accept;
    r.dw_data1 = kData1;
    r.dw_data2 = kData2;
    r.user_found = true;
    auto a = classify_guild_fieldwar_user(r);
    EXPECT_EQ(a.kind, GuildFieldWarUserActionKind::check_guild_field_war_money);
    const auto plan = guild_fieldwar_user_side_effect_plan(a);
    EXPECT_TRUE(plan.check_guild_field_war_money);
    EXPECT_FALSE(plan.check_guild_master_login);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, GuildFieldWarSideEffectKind::CheckGuildFieldWarMoney);
    EXPECT_EQ(plan.effects[0].dw_data1, kData1);
    EXPECT_EQ(plan.effects[0].dw_data2, kData2);
}

TEST(GuildFieldWarPlan, UserDefaultProtocolForwardsToMap) {
    GuildFieldWarUserRequest r;
    r.protocol = guild_fieldwar_start;
    r.user_found = true;
    auto a = classify_guild_fieldwar_user(r);
    EXPECT_EQ(a.kind, GuildFieldWarUserActionKind::forward_to_map_server);
    const auto plan = guild_fieldwar_user_side_effect_plan(a);
    EXPECT_TRUE(plan.forward_to_map_server);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, GuildFieldWarSideEffectKind::ForwardToMapServer);
    EXPECT_EQ(plan.effects[0].reply_protocol, guild_fieldwar_start);
}

TEST(GuildFieldWarPlan, ServerNotifyTomapBroadcastsExceptSource) {
    GuildFieldWarUserSlot slots[2] = {{kConnId + 1, true}, {kConnId + 2, true}};
    GuildFieldWarServerRequest r;
    r.protocol = guild_fieldwar_start_notify_tomap;
    r.user_count = 2;
    r.users = slots;
    auto a = classify_guild_fieldwar_server(r);
    EXPECT_EQ(a.kind, GuildFieldWarServerActionKind::broadcast_to_map_servers_except_source);
    const auto plan = guild_fieldwar_server_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_TRUE(plan.broadcast_to_map_servers_except_source);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, GuildFieldWarSideEffectKind::BroadcastToMapServersExceptSource);
    EXPECT_EQ(plan.effects[0].broadcast_count, 2u);
    EXPECT_EQ(plan.effects[0].reply_protocol, guild_fieldwar_start_notify_tomap);
}

TEST(GuildFieldWarPlan, ServerDeclareNackResolvesTargetUser) {
    GuildFieldWarServerRequest r;
    r.protocol = guild_fieldwar_declare_nack;
    r.dword2.dw_object_id = kObjectId;
    r.dword2.dw_data1 = kData1;
    r.target_user_found = true;
    r.target_user_conn = kConnId;
    auto a = classify_guild_fieldwar_server(r);
    EXPECT_EQ(a.kind, GuildFieldWarServerActionKind::send_to_target_user_if_found);
    EXPECT_TRUE(a.target_resolved);
    const auto plan = guild_fieldwar_server_side_effect_plan(a);
    EXPECT_TRUE(plan.send_to_target_user_if_found);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, GuildFieldWarSideEffectKind::SendToTargetUserIfFound);
    EXPECT_EQ(plan.effects[0].target_user_conn, kConnId);
    EXPECT_TRUE(plan.effects[0].target_resolved);
    EXPECT_EQ(plan.effects[0].dw_data1, kData1);
}

TEST(GuildFieldWarPlan, ServerDeclareNackWithoutTargetDrops) {
    GuildFieldWarServerRequest r;
    r.protocol = guild_fieldwar_declare_nack;
    r.target_user_found = false;
    auto a = classify_guild_fieldwar_server(r);
    EXPECT_FALSE(a.target_resolved);
    const auto plan = guild_fieldwar_server_side_effect_plan(a);
    EXPECT_TRUE(plan.send_to_target_user_if_found);
    EXPECT_TRUE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, GuildFieldWarSideEffectKind::SendToTargetUserIfFound);
    EXPECT_FALSE(plan.effects[0].target_resolved);
}

TEST(GuildFieldWarPlan, ServerAddmoneyTomapEmitsMoneyEffect) {
    GuildFieldWarServerRequest r;
    r.protocol = guild_fieldwar_addmoney_tomap;
    r.dword2.dw_data1 = kData1;
    r.dword2.dw_data2 = kData2;
    auto a = classify_guild_fieldwar_server(r);
    EXPECT_EQ(a.kind, GuildFieldWarServerActionKind::add_guild_field_war_money);
    const auto plan = guild_fieldwar_server_side_effect_plan(a);
    EXPECT_TRUE(plan.add_guild_field_war_money);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, GuildFieldWarSideEffectKind::AddGuildFieldWarMoney);
    EXPECT_EQ(plan.effects[0].dw_data1, kData1);
    EXPECT_EQ(plan.effects[0].dw_data2, kData2);
}

TEST(GuildFieldWarPlan, ServerResultToAllUserBroadcastsAllUsers) {
    GuildFieldWarUserSlot slots[3] = {{kConnId + 1, true}, {kConnId + 2, true}, {kConnId + 3, true}};
    GuildFieldWarServerRequest r;
    r.protocol = guild_fieldwar_result_toalluser;
    r.name2.dw_data1 = kData1;
    r.name2.dw_data2 = kData2;
    r.user_count = 3;
    r.users = slots;
    auto a = classify_guild_fieldwar_server(r);
    EXPECT_EQ(a.kind, GuildFieldWarServerActionKind::broadcast_to_all_users);
    const auto plan = guild_fieldwar_server_side_effect_plan(a);
    EXPECT_TRUE(plan.broadcast_to_all_users);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, GuildFieldWarSideEffectKind::BroadcastToAllUsers);
    EXPECT_EQ(plan.effects[0].broadcast_count, 3u);
    EXPECT_EQ(plan.effects[0].dw_data1, kData1);
}

TEST(GuildFieldWarPlan, ServerDefaultProtocolForwardsToOriginatingClient) {
    GuildFieldWarServerRequest r;
    r.protocol = guild_fieldwar_start;
    auto a = classify_guild_fieldwar_server(r);
    EXPECT_EQ(a.kind, GuildFieldWarServerActionKind::forward_to_originating_client);
    const auto plan = guild_fieldwar_server_side_effect_plan(a);
    EXPECT_TRUE(plan.forward_to_originating_client);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, GuildFieldWarSideEffectKind::ForwardToOriginatingClient);
    EXPECT_EQ(plan.effects[0].reply_protocol, guild_fieldwar_start);
}

TEST(GuildFieldWarPlan, ServerUnknownProtocolEmitsDrop) {
    GuildFieldWarServerRequest r;
    r.protocol = 200u;
    auto a = classify_guild_fieldwar_server(r);
    EXPECT_EQ(a.kind, GuildFieldWarServerActionKind::drop_unknown_protocol);
    const auto plan = guild_fieldwar_server_side_effect_plan(a);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, GuildFieldWarSideEffectKind::Drop);
    EXPECT_EQ(plan.effects[0].reply_protocol, 200u);
}

TEST(GuildFieldWarPlan, PlanDefaultsAreConservative) {
    GuildFieldWarSideEffectPlan plan;
    EXPECT_FALSE(plan.dispatched);
    EXPECT_TRUE(plan.drop);
    EXPECT_TRUE(plan.effects.empty());
}

