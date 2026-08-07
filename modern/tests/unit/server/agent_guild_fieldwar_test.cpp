//
// 1:1 port of MP_GUILD_FIELDWARUserMsgParser / MP_GUILD_FIELDWARServerMsgParser from
// legacy [Server]Agent/AgentNetworkMsgParser.cpp lines 4014-4090.

#include <mxh/server/agent_guild_fieldwar.hpp>

#include <gtest/gtest.h>

#include <array>
#include <set>

using namespace mxh::server;

namespace {
constexpr std::uint32_t kObjectId = 0xCAFEBABEu;
constexpr std::uint32_t kData = 0x11223344u;
constexpr std::uint32_t kData1 = 0x55667788u;
constexpr std::uint32_t kData2 = 0x99AABBCCu;
constexpr std::uint32_t kConnId = 0x200u;
}

TEST(AgentGuildFieldWarClassify, CategoryConstantMatchesProtocolHeader) {
    EXPECT_EQ(guild_fieldwar_category, 57u);
}

TEST(AgentGuildFieldWarClassify, ProtocolConstantsCoverZeroToTwentySeven) {
    EXPECT_EQ(guild_fieldwar_nack, 0u);
    EXPECT_EQ(guild_fieldwar_wait, 1u);
    EXPECT_EQ(guild_fieldwar_declare, 2u);
    EXPECT_EQ(guild_fieldwar_declare_nack, 3u);
    EXPECT_EQ(guild_fieldwar_declare_accept, 4u);
    EXPECT_EQ(guild_fieldwar_declare_deny, 5u);
    EXPECT_EQ(guild_fieldwar_declare_deny_notify_tomap, 6u);
    EXPECT_EQ(guild_fieldwar_start, 7u);
    EXPECT_EQ(guild_fieldwar_start_notify_tomap, 8u);
    EXPECT_EQ(guild_fieldwar_proc, 9u);
    EXPECT_EQ(guild_fieldwar_end, 10u);
    EXPECT_EQ(guild_fieldwar_end_notify_tomap, 11u);
    EXPECT_EQ(guild_fieldwar_suggestend, 12u);
    EXPECT_EQ(guild_fieldwar_suggestend_notify_tomap, 13u);
    EXPECT_EQ(guild_fieldwar_suggestend_nack, 14u);
    EXPECT_EQ(guild_fieldwar_suggestend_accept, 15u);
    EXPECT_EQ(guild_fieldwar_suggestend_accept_notify_tomap, 16u);
    EXPECT_EQ(guild_fieldwar_suggestend_deny, 17u);
    EXPECT_EQ(guild_fieldwar_suggestend_deny_notify_tomap, 18u);
    EXPECT_EQ(guild_fieldwar_surrender, 19u);
    EXPECT_EQ(guild_fieldwar_surrender_nack, 20u);
    EXPECT_EQ(guild_fieldwar_surrender_notify_tomap, 21u);
    EXPECT_EQ(guild_fieldwar_leveldown, 22u);
    EXPECT_EQ(guild_fieldwar_record, 23u);
    EXPECT_EQ(guild_fieldwar_addmoney, 24u);
    EXPECT_EQ(guild_fieldwar_removemoney, 25u);
    EXPECT_EQ(guild_fieldwar_addmoney_tomap, 26u);
    EXPECT_EQ(guild_fieldwar_result_toalluser, 27u);
}

TEST(AgentGuildFieldWarClassify, NotifyTomapPredicateIdentifiesAllSeven) {
    EXPECT_TRUE(is_guild_fieldwar_notify_tomap(guild_fieldwar_declare_deny_notify_tomap));
    EXPECT_TRUE(is_guild_fieldwar_notify_tomap(guild_fieldwar_start_notify_tomap));
    EXPECT_TRUE(is_guild_fieldwar_notify_tomap(guild_fieldwar_end_notify_tomap));
    EXPECT_TRUE(is_guild_fieldwar_notify_tomap(guild_fieldwar_suggestend_notify_tomap));
    EXPECT_TRUE(is_guild_fieldwar_notify_tomap(guild_fieldwar_suggestend_accept_notify_tomap));
    EXPECT_TRUE(is_guild_fieldwar_notify_tomap(guild_fieldwar_suggestend_deny_notify_tomap));
    EXPECT_TRUE(is_guild_fieldwar_notify_tomap(guild_fieldwar_surrender_notify_tomap));
    EXPECT_FALSE(is_guild_fieldwar_notify_tomap(guild_fieldwar_declare));
    EXPECT_FALSE(is_guild_fieldwar_notify_tomap(guild_fieldwar_nack));
    EXPECT_FALSE(is_guild_fieldwar_notify_tomap(guild_fieldwar_result_toalluser));
    EXPECT_FALSE(is_guild_fieldwar_notify_tomap(200u));
}

TEST(AgentGuildFieldWarUserClassify, DeclareWithoutUserIsNoUserForward) {
    GuildFieldWarUserRequest r;
    r.protocol = guild_fieldwar_declare;
    r.dw_object_id = kObjectId;
    r.user_found = false;
    auto a = classify_guild_fieldwar_user(r);
    EXPECT_EQ(a.kind, GuildFieldWarUserActionKind::forward_to_map_server_no_user);
    EXPECT_EQ(a.reply_protocol, guild_fieldwar_declare);
    EXPECT_EQ(a.dw_object_id, kObjectId);
}

TEST(AgentGuildFieldWarUserClassify, DeclareWithUserEmitsGuildMasterCheck) {
    GuildFieldWarUserRequest r;
    r.protocol = guild_fieldwar_declare;
    r.dw_object_id = kObjectId;
    r.dw_data = kData;
    r.user_found = true;
    auto a = classify_guild_fieldwar_user(r);
    EXPECT_EQ(a.kind, GuildFieldWarUserActionKind::check_guild_master_login);
    EXPECT_EQ(a.reply_protocol, guild_fieldwar_declare);
    EXPECT_EQ(a.dw_data, kData);
}

TEST(AgentGuildFieldWarUserClassify, SuggestEndWithUserEmitsGuildMasterCheck) {
    GuildFieldWarUserRequest r;
    r.protocol = guild_fieldwar_suggestend;
    r.user_found = true;
    auto a = classify_guild_fieldwar_user(r);
    EXPECT_EQ(a.kind, GuildFieldWarUserActionKind::check_guild_master_login);
}

TEST(AgentGuildFieldWarUserClassify, DeclareAcceptWithUserEmitsMoneyCheck) {
    GuildFieldWarUserRequest r;
    r.protocol = guild_fieldwar_declare_accept;
    r.dw_data1 = kData1;
    r.dw_data2 = kData2;
    r.user_found = true;
    auto a = classify_guild_fieldwar_user(r);
    EXPECT_EQ(a.kind, GuildFieldWarUserActionKind::check_guild_field_war_money);
    EXPECT_EQ(a.dw_data1, kData1);
    EXPECT_EQ(a.dw_data2, kData2);
}

TEST(AgentGuildFieldWarUserClassify, DefaultProtocolForwardsToMap) {
    GuildFieldWarUserRequest r;
    r.protocol = guild_fieldwar_start;
    r.user_found = true;
    auto a = classify_guild_fieldwar_user(r);
    EXPECT_EQ(a.kind, GuildFieldWarUserActionKind::forward_to_map_server);
    EXPECT_EQ(a.reply_protocol, guild_fieldwar_start);
}

TEST(AgentGuildFieldWarServerClassify, DeclareDenyNotifyTomapBroadcastsExceptSource) {
    GuildFieldWarUserSlot slots[2] = {{kConnId + 1, true}, {kConnId + 2, true}};
    GuildFieldWarServerRequest r;
    r.protocol = guild_fieldwar_declare_deny_notify_tomap;
    r.user_count = 2;
    r.users = slots;
    auto a = classify_guild_fieldwar_server(r);
    EXPECT_EQ(a.kind, GuildFieldWarServerActionKind::broadcast_to_map_servers_except_source);
    EXPECT_EQ(a.reply_protocol, guild_fieldwar_declare_deny_notify_tomap);
    EXPECT_EQ(a.broadcast_count, 2u);
}

TEST(AgentGuildFieldWarServerClassify, AllSevenNotifyTomapBroadcastExceptSource) {
    for (std::uint8_t p : {guild_fieldwar_declare_deny_notify_tomap,
                         guild_fieldwar_start_notify_tomap,
                         guild_fieldwar_end_notify_tomap,
                         guild_fieldwar_suggestend_notify_tomap,
                         guild_fieldwar_suggestend_accept_notify_tomap,
                         guild_fieldwar_suggestend_deny_notify_tomap,
                         guild_fieldwar_surrender_notify_tomap}) {
        GuildFieldWarServerRequest r;
        r.protocol = p;
        auto a = classify_guild_fieldwar_server(r);
        EXPECT_EQ(a.kind, GuildFieldWarServerActionKind::broadcast_to_map_servers_except_source);
    }
}

TEST(AgentGuildFieldWarServerClassify, NotifyTomapWithEmptyUsersReturnsZero) {
    GuildFieldWarServerRequest r;
    r.protocol = guild_fieldwar_start_notify_tomap;
    r.user_count = 0;
    r.users = nullptr;
    auto a = classify_guild_fieldwar_server(r);
    EXPECT_EQ(a.kind, GuildFieldWarServerActionKind::broadcast_to_map_servers_except_source);
    EXPECT_EQ(a.broadcast_count, 0u);
}

TEST(AgentGuildFieldWarServerClassify, DeclareNackResolvesTarget) {
    GuildFieldWarServerRequest r;
    r.protocol = guild_fieldwar_declare_nack;
    r.dword2.dw_object_id = kObjectId;
    r.dword2.dw_data1 = kData1;
    r.target_user_found = true;
    r.target_user_conn = kConnId;
    auto a = classify_guild_fieldwar_server(r);
    EXPECT_EQ(a.kind, GuildFieldWarServerActionKind::send_to_target_user_if_found);
    EXPECT_EQ(a.dw_data1, kData1);
    EXPECT_TRUE(a.target_resolved);
    EXPECT_EQ(a.target_user_conn, kConnId);
}

TEST(AgentGuildFieldWarServerClassify, DeclareNackWithoutTargetDrops) {
    GuildFieldWarServerRequest r;
    r.protocol = guild_fieldwar_declare_nack;
    r.target_user_found = false;
    auto a = classify_guild_fieldwar_server(r);
    EXPECT_EQ(a.kind, GuildFieldWarServerActionKind::send_to_target_user_if_found);
    EXPECT_FALSE(a.target_resolved);
}

TEST(AgentGuildFieldWarServerClassify, AddmoneyTomapEmitsMoneySideEffect) {
    GuildFieldWarServerRequest r;
    r.protocol = guild_fieldwar_addmoney_tomap;
    r.dword2.dw_data1 = kData1;
    r.dword2.dw_data2 = kData2;
    auto a = classify_guild_fieldwar_server(r);
    EXPECT_EQ(a.kind, GuildFieldWarServerActionKind::add_guild_field_war_money);
    EXPECT_EQ(a.dw_data1, kData1);
    EXPECT_EQ(a.dw_data2, kData2);
}

TEST(AgentGuildFieldWarServerClassify, ResultToAllUserBroadcastsAllUsers) {
    GuildFieldWarUserSlot slots[3] = {{kConnId + 1, true}, {kConnId + 2, true}, {kConnId + 3, true}};
    GuildFieldWarServerRequest r;
    r.protocol = guild_fieldwar_result_toalluser;
    r.name2.dw_data1 = kData1;
    r.name2.dw_data2 = kData2;
    r.user_count = 3;
    r.users = slots;
    auto a = classify_guild_fieldwar_server(r);
    EXPECT_EQ(a.kind, GuildFieldWarServerActionKind::broadcast_to_all_users);
    EXPECT_EQ(a.dw_data1, kData1);
    EXPECT_EQ(a.dw_data2, kData2);
    EXPECT_EQ(a.broadcast_count, 3u);
}

TEST(AgentGuildFieldWarServerClassify, ResultToAllUserSkipsUsersNotInTable) {
    GuildFieldWarUserSlot slots[3] = {{kConnId + 1, true}, {kConnId + 2, false}, {kConnId + 3, true}};
    GuildFieldWarServerRequest r;
    r.protocol = guild_fieldwar_result_toalluser;
    r.user_count = 3;
    r.users = slots;
    auto a = classify_guild_fieldwar_server(r);
    EXPECT_EQ(a.broadcast_count, 2u);
}

TEST(AgentGuildFieldWarServerClassify, DefaultServerProtocolForwardsToClient) {
    GuildFieldWarServerRequest r;
    r.protocol = guild_fieldwar_start;
    auto a = classify_guild_fieldwar_server(r);
    EXPECT_EQ(a.kind, GuildFieldWarServerActionKind::forward_to_originating_client);
    EXPECT_EQ(a.reply_protocol, guild_fieldwar_start);
}

TEST(AgentGuildFieldWarServerClassify, AllForwardProtocolsGoToClient) {
    for (std::uint8_t p : {guild_fieldwar_nack, guild_fieldwar_wait, guild_fieldwar_declare,
                         guild_fieldwar_declare_accept, guild_fieldwar_declare_deny,
                         guild_fieldwar_start, guild_fieldwar_proc, guild_fieldwar_end,
                         guild_fieldwar_suggestend, guild_fieldwar_suggestend_nack,
                         guild_fieldwar_suggestend_accept, guild_fieldwar_suggestend_deny,
                         guild_fieldwar_surrender, guild_fieldwar_surrender_nack,
                         guild_fieldwar_leveldown, guild_fieldwar_record,
                         guild_fieldwar_addmoney, guild_fieldwar_removemoney}) {
        GuildFieldWarServerRequest r;
        r.protocol = p;
        auto a = classify_guild_fieldwar_server(r);
        EXPECT_EQ(a.kind, GuildFieldWarServerActionKind::forward_to_originating_client);
        EXPECT_EQ(a.reply_protocol, p);
    }
}

TEST(AgentGuildFieldWarServerClassify, UnknownProtocolDrops) {
    GuildFieldWarServerRequest r;
    r.protocol = 200u;
    auto a = classify_guild_fieldwar_server(r);
    EXPECT_EQ(a.kind, GuildFieldWarServerActionKind::drop_unknown_protocol);
    EXPECT_EQ(a.reply_protocol, 200u);
    EXPECT_EQ(a.broadcast_count, 0u);
    EXPECT_FALSE(a.target_resolved);
}

TEST(AgentGuildFieldWarClassify, RequestAndActionDefaultsAreStable) {
    GuildFieldWarUserRequest ur;
    EXPECT_EQ(ur.protocol, 0u);
    EXPECT_EQ(ur.dw_object_id, 0u);
    EXPECT_TRUE(ur.user_found);
    GuildFieldWarServerRequest sr;
    EXPECT_EQ(sr.protocol, 0u);
    EXPECT_EQ(sr.user_count, 0u);
    EXPECT_EQ(sr.users, nullptr);
    EXPECT_FALSE(sr.target_user_found);
    GuildFieldWarUserAction ua;
    EXPECT_EQ(ua.kind, GuildFieldWarUserActionKind::forward_to_map_server);
    EXPECT_EQ(ua.reply_protocol, 0u);
    EXPECT_TRUE(ua.forward_payload);
    GuildFieldWarServerAction sa;
    EXPECT_EQ(sa.kind, GuildFieldWarServerActionKind::drop_unknown_protocol);
    EXPECT_FALSE(sa.target_resolved);
    EXPECT_EQ(sa.broadcast_count, 0u);
}

TEST(AgentGuildFieldWarUserClassify, EchoesProtocolOnNoUserForward) {
    GuildFieldWarUserRequest r;
    r.protocol = 220u;
    r.user_found = false;
    auto a = classify_guild_fieldwar_user(r);
    EXPECT_EQ(a.kind, GuildFieldWarUserActionKind::forward_to_map_server_no_user);
    EXPECT_EQ(a.reply_protocol, 220u);
}
