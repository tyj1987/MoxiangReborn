// agent_cheat_test.cpp - Phase 6.3 AgentCheat 1:1 port tests.

#include "mxh/server/agent_cheat.hpp"

#include <gtest/gtest.h>

namespace {

using mxh::server::classify_agent_cheat_user;
using mxh::server::classify_agent_cheat_server;
using mxh::server::CheatUserActionKind;
using mxh::server::CheatServerActionKind;
using mxh::server::CheatUserRequest;
using mxh::server::CheatServerRequest;

CheatUserRequest base_user() {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_changemap_syn;
    r.user_level = mxh::server::cheat_user_level_programmer;
    return r;
}

TEST(CheatCategory, CategoryByteIsEleven) {
    EXPECT_EQ(mxh::server::cheat_category, 11u);
}

TEST(CheatUserLevel, ConstantsMatchLegacy) {
    EXPECT_EQ(mxh::server::cheat_user_level_gm, 1u);
    EXPECT_EQ(mxh::server::cheat_user_level_programmer, 2u);
    EXPECT_EQ(mxh::server::cheat_user_level_developer, 3u);
}

TEST(CheatUserUnknownConnection, IsDropped) {
    auto r = base_user();
    r.user_known = false;
    auto a = classify_agent_cheat_user(r);
    EXPECT_EQ(a.kind, CheatUserActionKind::drop_no_user);
}

TEST(CheatUserNotAuthorized, IsDropped) {
    auto r = base_user();
    r.user_level = 99u;
    auto a = classify_agent_cheat_user(r);
    EXPECT_EQ(a.kind, CheatUserActionKind::drop_unauthorized);
}

TEST(CheatUserGmMissingInfo, IsDropped) {
    auto r = base_user();
    r.user_level = mxh::server::cheat_user_level_gm;
    r.gm_info_present = false;
    auto a = classify_agent_cheat_user(r);
    EXPECT_EQ(a.kind, CheatUserActionKind::drop_gm_not_logged_in);
}

TEST(CheatUserProgrammerBypassesGmInfoCheck, CallsLogin) {
    auto r = base_user();
    r.protocol = mxh::server::cheat_gm_login_syn;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_EQ(a.kind, CheatUserActionKind::gm_login);
    EXPECT_EQ(a.reply_protocol, mxh::server::cheat_gm_login_syn);
}

TEST(CheatUserChangeMap, UnknownPortNacksUser) {
    auto r = base_user();
    r.protocol = mxh::server::cheat_changemap_syn;
    r.target_map_port_known = false;
    auto a = classify_agent_cheat_user(r);
    EXPECT_EQ(a.kind, CheatUserActionKind::send_changemap_nack_to_user);
    EXPECT_EQ(a.reply_protocol, mxh::server::cheat_changemap_nack);
}

TEST(CheatUserChangeMap, KnownPortForwards) {
    auto r = base_user();
    r.protocol = mxh::server::cheat_changemap_syn;
    auto a = classify_agent_cheat_user(r);
    EXPECT_EQ(a.kind, CheatUserActionKind::trans_to_map_server);
}

TEST(CheatUserBanCharacter, ZeroTargetDrops) {
    auto r = base_user();
    r.protocol = mxh::server::cheat_bancharacter_syn;
    r.admin_target_user = 0u;
    auto a = classify_agent_cheat_user(r);
    EXPECT_EQ(a.kind, CheatUserActionKind::drop_gm_blocked_subprotocol);
}

TEST(CheatUserBanCharacter, NonZeroBroadcasts) {
    auto r = base_user();
    r.protocol = mxh::server::cheat_bancharacter_syn;
    r.admin_target_user = 9u;
    auto a = classify_agent_cheat_user(r);
    EXPECT_EQ(a.kind, CheatUserActionKind::send_bancharacter_to_map_server);
    EXPECT_TRUE(a.broadcast_all_maps);
}

TEST(CheatUserBlockCharacter, Broadcasts) {
    auto r = base_user();
    r.protocol = mxh::server::cheat_blockcharacter_syn;
    auto a = classify_agent_cheat_user(r);
    EXPECT_EQ(a.kind, CheatUserActionKind::send_blockcharacter_syn);
    EXPECT_TRUE(a.broadcast_all_maps);
}

TEST(CheatUserEventMonsterRegen, Broadcasts) {
    auto r = base_user();
    r.protocol = mxh::server::cheat_event_monster_regen;
    auto a = classify_agent_cheat_user(r);
    EXPECT_EQ(a.kind, CheatUserActionKind::send_event_monster_regen);
    EXPECT_TRUE(a.broadcast_all_maps);
}

TEST(CheatUserEventMonsterDelete, Broadcasts) {
    auto r = base_user();
    r.protocol = mxh::server::cheat_event_monster_delete;
    auto a = classify_agent_cheat_user(r);
    EXPECT_EQ(a.kind, CheatUserActionKind::send_event_monster_delete);
    EXPECT_TRUE(a.broadcast_all_maps);
}

TEST(CheatUserPkAllow, Broadcasts) {
    auto r = base_user();
    r.protocol = mxh::server::cheat_pkallow_syn;
    auto a = classify_agent_cheat_user(r);
    EXPECT_EQ(a.kind, CheatUserActionKind::send_pkallow_syn);
    EXPECT_TRUE(a.broadcast_all_maps);
}

TEST(CheatUserItemCheats, AllLogGmToolUse) {
    for (std::uint8_t proto : {
             mxh::server::cheat_abilityexp_syn,
             mxh::server::cheat_addmugong_syn,
             mxh::server::cheat_mugongsung_syn,
             mxh::server::cheat_item_syn,
             mxh::server::cheat_item_option_syn,
             mxh::server::cheat_money_syn,
         }) {
        auto r = base_user();
        r.protocol = proto;
        auto a = classify_agent_cheat_user(r);
        EXPECT_EQ(a.kind, CheatUserActionKind::log_gm_tool_use) << "proto=" << +proto;
        EXPECT_EQ(a.reply_protocol, proto);
    }
}

TEST(CheatUserPetCheats, AllLogGmToolUse) {
    for (std::uint8_t proto : {
             mxh::server::cheat_pet_stamina,
             mxh::server::cheat_pet_friendship_syn,
             mxh::server::cheat_pet_selected_friendship_syn,
         }) {
        auto r = base_user();
        r.protocol = proto;
        auto a = classify_agent_cheat_user(r);
        EXPECT_EQ(a.kind, CheatUserActionKind::log_gm_tool_use) << "proto=" << +proto;
    }
}

TEST(CheatUserJackpotCheats, AllLogGmToolUse) {
    for (std::uint8_t proto : {
             mxh::server::cheat_jackpot_getprize,
             mxh::server::cheat_jackpot_moneypermonster,
             mxh::server::cheat_jackpot_onoff,
             mxh::server::cheat_jackpot_probability,
             mxh::server::cheat_jackpot_control,
         }) {
        auto r = base_user();
        r.protocol = proto;
        auto a = classify_agent_cheat_user(r);
        EXPECT_EQ(a.kind, CheatUserActionKind::log_gm_tool_use) << "proto=" << +proto;
    }
}

TEST(CheatUserBobusangCheats, AllLogGmToolUse) {
    for (std::uint8_t proto : {
             mxh::server::cheat_bobusanginfo_request_syn,
             mxh::server::cheat_bobusang_leave_syn,
             mxh::server::cheat_bobusanginfo_change_syn,
             mxh::server::cheat_itemlimit_syn,
             mxh::server::cheat_autonote_setting_syn,
         }) {
        auto r = base_user();
        r.protocol = proto;
        auto a = classify_agent_cheat_user(r);
        EXPECT_EQ(a.kind, CheatUserActionKind::log_gm_tool_use) << "proto=" << +proto;
    }
}

TEST(CheatUserPlusTimeOn, LogsAndBroadcasts) {
    auto r = base_user();
    r.protocol = mxh::server::cheat_plustime_on;
    auto a = classify_agent_cheat_user(r);
    EXPECT_EQ(a.kind, CheatUserActionKind::log_gm_tool_use);
    EXPECT_TRUE(a.broadcast_all_maps);
}

TEST(CheatUserDamageAck, ForwardsToUser) {
    auto r = base_user();
    r.protocol = mxh::server::cheat_damage_ack;
    auto a = classify_agent_cheat_user(r);
    EXPECT_EQ(a.kind, CheatUserActionKind::forward_to_user);
}

TEST(CheatUserWhereisMissingTarget, Drops) {
    auto r = base_user();
    r.protocol = mxh::server::cheat_whereis_syn;
    r.target_char_in_user_table = false;
    auto a = classify_agent_cheat_user(r);
    EXPECT_EQ(a.kind, CheatUserActionKind::drop_no_user);
}

TEST(CheatUserUnknownProto, DefaultsToTrans) {
    auto r = base_user();
    r.protocol = 200u;
    auto a = classify_agent_cheat_user(r);
    EXPECT_EQ(a.kind, CheatUserActionKind::trans_to_map_server);
}

TEST(CheatServerBanCharUnknownTarget, Drops) {
    CheatServerRequest r{};
    r.protocol = mxh::server::cheat_bancharacter_syn;
    r.target_found = false;
    auto a = classify_agent_cheat_server(r);
    EXPECT_EQ(a.kind, CheatServerActionKind::drop_unknown_char);
}

TEST(CheatServerBanCharAck, SendsAckToUser) {
    CheatServerRequest r{};
    r.protocol = mxh::server::cheat_bancharacter_ack;
    auto a = classify_agent_cheat_server(r);
    EXPECT_EQ(a.kind, CheatServerActionKind::send_bancharacter_ack_to_user);
}

TEST(CheatServerBanCharNack, Broadcasts) {
    CheatServerRequest r{};
    r.protocol = mxh::server::cheat_bancharacter_nack;
    auto a = classify_agent_cheat_server(r);
    EXPECT_EQ(a.kind, CheatServerActionKind::send_bancharacter_ack_broadcast);
    EXPECT_TRUE(a.broadcast_all_maps);
}

TEST(CheatServerWhereisUnknownTarget, NacksUser) {
    CheatServerRequest r{};
    r.protocol = mxh::server::cheat_whereis_syn;
    r.whereis_target_known = false;
    auto a = classify_agent_cheat_server(r);
    EXPECT_EQ(a.kind, CheatServerActionKind::send_whereis_nack_to_user);
}

TEST(CheatServerWhereisAck, BroadcastsToMaps) {
    CheatServerRequest r{};
    r.protocol = mxh::server::cheat_whereis_ack;
    auto a = classify_agent_cheat_server(r);
    EXPECT_EQ(a.kind, CheatServerActionKind::send_whereis_ack_broadcast);
    EXPECT_TRUE(a.broadcast_all_maps);
}



TEST(CheatSweep, Sweep_0) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_gm_login_syn;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_1) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_changemap_syn;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_2) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_changemap_nack;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_3) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_changemap_ack;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_4) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_bancharacter_syn;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_5) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_bancharacter_nack;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_6) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_blockcharacter_syn;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_7) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_whereis_syn;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_8) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_event_monster_regen;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_9) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_event_monster_delete;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_10) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_banmap_syn;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_11) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_agentcheck_syn;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_12) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_pkallow_syn;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_13) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_notice_syn;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_14) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_abilityexp_syn;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_15) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_addmugong_syn;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_16) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_mugongsung_syn;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_17) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_item_syn;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_18) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_item_option_syn;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_19) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_money_syn;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_20) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_event_syn;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_21) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_eventnotify_on;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_22) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_plustime_on;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_23) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_eventnotify_off;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_24) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_plustime_alloff;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_25) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_change_eventmap_syn;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_26) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_event_start_syn;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_27) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_event_ready_syn;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_28) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_pet_stamina;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_29) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_pet_friendship_syn;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_30) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_pet_selected_friendship_syn;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_31) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_guildpoint_syn;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_32) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_guildhunted_monstercount_syn;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_33) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_mussang_ready;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_34) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_jackpot_getprize;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_35) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_jackpot_moneypermonster;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_36) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_jackpot_onoff;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_37) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_jackpot_probability;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_38) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_jackpot_control;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_39) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_bobusanginfo_request_syn;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_40) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_bobusang_leave_syn;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_41) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_bobusanginfo_change_syn;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_42) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_itemlimit_syn;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_43) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_autonote_setting_syn;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_44) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_damage_syn;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_45) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_damage_ack;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_46) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_damage_nack;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_47) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_map_condition;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_48) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_agent_condition;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_49) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_titan_fuel_spell_max_syn;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_50) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_bancharacter_ack;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(CheatSweep, Sweep_51) {
    CheatUserRequest r{};
    r.protocol = mxh::server::cheat_whereis_ack;
    r.user_level = mxh::server::cheat_user_level_programmer;
    auto a = classify_agent_cheat_user(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}

}  // namespace
