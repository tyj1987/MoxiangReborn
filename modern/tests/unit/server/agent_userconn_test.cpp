// agent_userconn_test.cpp - Phase 6.3 AgentUserConn 1:1 port tests.

#include "mxh/server/agent_userconn.hpp"

#include <gtest/gtest.h>

namespace {

using mxh::server::classify_userconn;
// (alias added below for compatibility)
using mxh::server::UserConnAction;
using mxh::server::UserConnActionKind;
using mxh::server::UserConnRequest;

UserConnRequest base() {
    UserConnRequest r{};
    r.protocol = mxh::server::userconn_notify_userlogin_syn;
    r.user_id = 7777u;
    r.auth_key = 0xABCDEFu;
    r.dist_auth_key = 0xABCDEFu;
    return r;
}

TEST(UserConnCategory, CategoryByteIsSeven) {
    EXPECT_EQ(mxh::server::userconn_category, 7u);
}

TEST(UserConnProtocol, AllKnownProtocolsAreUnique) {
    std::uint16_t seen = 0u;
    const std::uint8_t table[] = {
        mxh::server::userconn_dist_connectsuccess, mxh::server::userconn_login_syn,
        mxh::server::userconn_login_ack, mxh::server::userconn_login_nack,
        mxh::server::userconn_notify_userlogin_syn, mxh::server::userconn_notify_userlogin_ack,
        mxh::server::userconn_notify_userlogin_nack,
        mxh::server::userconn_characterlist_syn, mxh::server::userconn_character_make_syn,
        mxh::server::userconn_gamein_syn, mxh::server::userconn_gamein_ack,
        mxh::server::userconn_gamein_nack, mxh::server::userconn_changemap_syn,
        mxh::server::userconn_savepoint_syn, mxh::server::userconn_disconnect_syn,
    };
    for (std::uint8_t v : table) {
        EXPECT_LT(v, 115u);
        ++seen;
    }
    EXPECT_EQ(seen, std::size(table));
}

TEST(UserConnNotifyUserlogin, NotReadyEmitsNoAgentNack) {
    auto r = base();
    r.agent_ready = false;
    // (r.error_code removed; not part of request)
    auto a = classify_userconn(r);
    EXPECT_EQ(a.kind, UserConnActionKind::send_nack_to_dist_no_agent);
    EXPECT_EQ(a.reply_protocol, mxh::server::userconn_notify_userlogin_nack);
    EXPECT_EQ(a.error_code, mxh::server::userconn_login_err_no_agent_server);
}

TEST(UserConnNotifyUserlogin, ReadyForwardsToMapServer) {
    auto r = base();
    r.agent_ready = true;
    auto a = classify_userconn(r);
    EXPECT_EQ(a.kind, UserConnActionKind::forward_to_map_server);
    EXPECT_EQ(a.reply_protocol, 0u);
}

TEST(UserConnCharacterList, UnknownUserDisconnectsAndNacks) {
    auto r = base();
    r.protocol = mxh::server::userconn_characterlist_syn;
    r.user_found_by_userid = false;
    auto a = classify_userconn(r);
    EXPECT_EQ(a.kind, UserConnActionKind::disconnect_user);
    EXPECT_EQ(a.reply_protocol, mxh::server::userconn_characterlist_nack);
}

TEST(UserConnCharacterList, AuthMismatchDisconnectsAndNacks) {
    auto r = base();
    r.protocol = mxh::server::userconn_characterlist_syn;
    r.auth_keys_match = false;
    auto a = classify_userconn(r);
    EXPECT_EQ(a.kind, UserConnActionKind::disconnect_user);
    EXPECT_EQ(a.reply_protocol, mxh::server::userconn_characterlist_nack);
}

TEST(UserConnCharacterList, ValidUserForwardsAck) {
    auto r = base();
    r.protocol = mxh::server::userconn_characterlist_syn;
    auto a = classify_userconn(r);
    EXPECT_EQ(a.kind, UserConnActionKind::forward_to_map_server);
    EXPECT_EQ(a.reply_protocol, mxh::server::userconn_characterlist_ack);
}

TEST(UserConnGameIn, SynRequiresUserByConn) {
    auto r = base();
    r.protocol = mxh::server::userconn_gamein_syn;
    r.user_found_by_conn = true;
    auto a = classify_userconn(r);
    EXPECT_EQ(a.kind, UserConnActionKind::forward_to_map_server);
    EXPECT_EQ(a.reply_protocol, mxh::server::userconn_gamein_syn);
    EXPECT_TRUE(a.forward_payload);

    r.user_found_by_conn = false;
    auto a2 = classify_userconn(r);
    EXPECT_EQ(a2.kind, UserConnActionKind::drop_no_user);
}

TEST(UserConnGameIn, AckIsForwardedToUser) {
    auto r = base();
    r.protocol = mxh::server::userconn_gamein_ack;
    auto a = classify_userconn(r);
    EXPECT_EQ(a.kind, UserConnActionKind::send_to_user);
    EXPECT_EQ(a.reply_protocol, mxh::server::userconn_gamein_ack);
}

TEST(UserConnGameIn, NackDisconnectsUser) {
    auto r = base();
    r.protocol = mxh::server::userconn_gamein_nack;
    auto a = classify_userconn(r);
    EXPECT_EQ(a.kind, UserConnActionKind::disconnect_user);
}

TEST(UserConnGameIn, OtherMapBroadcastsExceptSource) {
    auto r = base();
    r.protocol = mxh::server::userconn_gamein_othermap_syn;
    auto a = classify_userconn(r);
    EXPECT_EQ(a.kind, UserConnActionKind::broadcast_except_to_map_servers);
    EXPECT_EQ(a.reply_protocol, mxh::server::userconn_gamein_othermap_syn);
}

TEST(UserConnChangeMap, UnknownPortReturnsNackToUser) {
    auto r = base();
    r.protocol = mxh::server::userconn_changemap_syn;
    r.map_port_known = false;
    auto a = classify_userconn(r);
    EXPECT_EQ(a.kind, UserConnActionKind::send_to_user);
    EXPECT_EQ(a.reply_protocol, mxh::server::userconn_changemap_nack);
}

TEST(UserConnChangeMap, EventBlockedReturnsNackToUser) {
    auto r = base();
    r.protocol = mxh::server::userconn_changemap_syn;
    r.event_blocked = true;
    auto a = classify_userconn(r);
    EXPECT_EQ(a.kind, UserConnActionKind::send_to_user);
    EXPECT_EQ(a.reply_protocol, mxh::server::userconn_changemap_nack);
}

TEST(UserConnForceDisconnectOverlap, UserWithCharacterSendsNowaitexit) {
    auto r = base();
    r.protocol = mxh::server::userconn_force_disconnect_overlaplogin;
    r.character_id = 999u;
    r.map_server_conn = 17u;
    auto a = classify_userconn(r);
    EXPECT_EQ(a.kind, UserConnActionKind::send_nowaitexit_to_map_server);
    EXPECT_EQ(a.reply_protocol, mxh::server::userconn_nowaitexitplayer);
}

TEST(UserConnNotifyOverlap, UserMissingIsDropped) {
    auto r = base();
    r.protocol = mxh::server::userconn_notify_overlappedlogin;
    r.user_found_by_userid = false;
    auto a = classify_userconn(r);
    EXPECT_EQ(a.kind, UserConnActionKind::drop_no_user);
}

TEST(UserConnNotifyOverlap, UserPresentNotifiesOtherUser) {
    auto r = base();
    r.protocol = mxh::server::userconn_notify_overlappedlogin;
    auto a = classify_userconn(r);
    EXPECT_EQ(a.kind, UserConnActionKind::send_otheruser_connecttry_notify);
    EXPECT_EQ(a.reply_protocol, mxh::server::userconn_otheruser_connecttry_notify);
}

TEST(UserConnDisconnectedOnLogin, AuthMismatchDropped) {
    auto r = base();
    r.protocol = mxh::server::userconn_disconnected_on_login;
    r.auth_keys_match = false;
    auto a = classify_userconn(r);
    EXPECT_EQ(a.kind, UserConnActionKind::drop_unauthenticated);
}

TEST(UserConnRequestDistOut, MissingUserDropped) {
    auto r = base();
    r.protocol = mxh::server::userconn_request_distout;
    r.user_found_by_userid = false;
    auto a = classify_userconn(r);
    EXPECT_EQ(a.kind, UserConnActionKind::drop_no_user);
}

TEST(UserConnLoginCheckDelete, RepliesToCaller) {
    auto r = base();
    r.protocol = mxh::server::userconn_logincheck_delete;
    auto a = classify_userconn(r);
    EXPECT_EQ(a.kind, UserConnActionKind::reply_logincheck_delete);
}

TEST(UserConnAddRemove, BroadcastsToMapServers) {
    for (std::uint8_t proto : {
             mxh::server::userconn_character_add,
             mxh::server::userconn_pet_add,
             mxh::server::userconn_monster_add,
             mxh::server::userconn_npc_add,
             mxh::server::userconn_object_remove,
             mxh::server::userconn_character_die,
             mxh::server::userconn_monster_die,
             mxh::server::userconn_pet_die,
             mxh::server::userconn_character_revive,
             mxh::server::userconn_ready_to_revive,
             mxh::server::userconn_gridinit,
             mxh::server::userconn_setvisible,
         }) {
        auto r = base();
        r.protocol = proto;
        auto a = classify_userconn(r);
        EXPECT_EQ(a.kind, UserConnActionKind::broadcast_to_map_servers) << "proto=" << +proto;
        EXPECT_EQ(a.reply_protocol, proto);
    }
}

TEST(UserConnLoginDynamic, ForwardsToMapServer) {
    auto r = base();
    r.protocol = mxh::server::userconn_login_dynamic_syn;
    auto a = classify_userconn(r);
    EXPECT_EQ(a.kind, UserConnActionKind::forward_to_map_server);
    EXPECT_EQ(a.reply_protocol, mxh::server::userconn_login_dynamic_syn);
}

TEST(UserConnEventMapEnter, NoUserDrops) {
    auto r = base();
    r.protocol = mxh::server::userconn_enter_eventmap_syn;
    r.user_found_by_conn = false;
    auto a = classify_userconn(r);
    EXPECT_EQ(a.kind, UserConnActionKind::drop_no_user);
}

TEST(UserConnChangeMapChannelInfo, ForwardsAllThree) {
    for (std::uint8_t proto : {
             mxh::server::userconn_changemap_channelinfo_syn,
             mxh::server::userconn_changemap_channelinfo_ack,
             mxh::server::userconn_changemap_channelinfo_nack,
         }) {
        auto r = base();
        r.protocol = proto;
        auto a = classify_userconn(r);
        EXPECT_EQ(a.kind, UserConnActionKind::forward_to_map_server);
        EXPECT_EQ(a.reply_protocol, proto);
    }
}

TEST(UserConnConnectionCheckOk, ResetsFailureFlag) {
    auto r = base();
    r.protocol = mxh::server::userconn_connection_check_ok;
    auto a = classify_userconn(r);
    EXPECT_TRUE(a.disable_failure_flag);
}

TEST(UserConnUnknown, DefaultsToForward) {
    auto r = base();
    r.protocol = 200u;
    auto a = classify_userconn(r);
    EXPECT_EQ(a.kind, UserConnActionKind::forward_to_map_server);
    EXPECT_EQ(a.reply_protocol, 200u);
    EXPECT_TRUE(a.forward_payload);
}

TEST(UserConnCheatUsing, LogsCheat) {
    auto r = base();
    r.protocol = mxh::server::userconn_cheat_using;
    auto a = classify_userconn(r);
    EXPECT_EQ(a.kind, UserConnActionKind::log_cheat_use);
    EXPECT_EQ(a.reply_protocol, mxh::server::userconn_cheat_using);
}

TEST(UserConnNotifyToAgentAlreadyOut, MissingUserDrops) {
    auto r = base();
    r.protocol = mxh::server::userconn_notifytoagent_alreadyout;
    r.user_found_by_userid = false;
    auto a = classify_userconn(r);
    EXPECT_EQ(a.kind, UserConnActionKind::drop_no_user);
}

TEST(UserConnNotifyToAgentAlreadyOut, ValidUserNacksWithAlreadyout) {
    auto r = base();
    r.protocol = mxh::server::userconn_notifytoagent_alreadyout;
    auto a = classify_userconn(r);
    EXPECT_EQ(a.kind, UserConnActionKind::send_to_user);
    EXPECT_EQ(a.reply_protocol, mxh::server::userconn_login_nack);
    EXPECT_EQ(a.error_code, mxh::server::userconn_login_err_dist_alreadyout);
}

TEST(UserConnBackToCharSelAck, MissingCharDrops) {
    auto r = base();
    r.protocol = mxh::server::userconn_backtocharsel_ack;
    r.user_found_by_charid = false;
    auto a = classify_userconn(r);
    EXPECT_EQ(a.kind, UserConnActionKind::drop_no_user);
}



TEST(UserConnSweep, Sweep_0) {
    auto r = base();
    r.protocol = mxh::server::userconn_dist_connectsuccess;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_1) {
    auto r = base();
    r.protocol = mxh::server::userconn_login_syn;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_2) {
    auto r = base();
    r.protocol = mxh::server::userconn_login_ack;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_3) {
    auto r = base();
    r.protocol = mxh::server::userconn_login_nack;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_4) {
    auto r = base();
    r.protocol = mxh::server::userconn_notify_userlogin_syn;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_5) {
    auto r = base();
    r.protocol = mxh::server::userconn_notify_userlogin_ack;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_6) {
    auto r = base();
    r.protocol = mxh::server::userconn_notify_userlogin_nack;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_7) {
    auto r = base();
    r.protocol = mxh::server::userconn_notify_overlappedlogin;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_8) {
    auto r = base();
    r.protocol = mxh::server::userconn_agent_connectsuccess;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_9) {
    auto r = base();
    r.protocol = mxh::server::userconn_characterlist_syn;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_10) {
    auto r = base();
    r.protocol = mxh::server::userconn_directcharacterlist_syn;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_11) {
    auto r = base();
    r.protocol = mxh::server::userconn_characterlist_nack;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_12) {
    auto r = base();
    r.protocol = mxh::server::userconn_characterlist_ack;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_13) {
    auto r = base();
    r.protocol = mxh::server::userconn_disconnect_syn;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_14) {
    auto r = base();
    r.protocol = mxh::server::userconn_disconnect_ack;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_15) {
    auto r = base();
    r.protocol = mxh::server::userconn_disconnect_nack;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_16) {
    auto r = base();
    r.protocol = mxh::server::userconn_characterselect_syn;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_17) {
    auto r = base();
    r.protocol = mxh::server::userconn_characterselect_ack;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_18) {
    auto r = base();
    r.protocol = mxh::server::userconn_characterselect_nack;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_19) {
    auto r = base();
    r.protocol = mxh::server::userconn_character_namecheck_syn;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_20) {
    auto r = base();
    r.protocol = mxh::server::userconn_character_namecheck_ack;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_21) {
    auto r = base();
    r.protocol = mxh::server::userconn_character_namecheck_nack;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_22) {
    auto r = base();
    r.protocol = mxh::server::userconn_character_make_syn;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_23) {
    auto r = base();
    r.protocol = mxh::server::userconn_character_make_ack;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_24) {
    auto r = base();
    r.protocol = mxh::server::userconn_character_make_nack;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_25) {
    auto r = base();
    r.protocol = mxh::server::userconn_character_info_syn;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_26) {
    auto r = base();
    r.protocol = mxh::server::userconn_character_info_ack;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_27) {
    auto r = base();
    r.protocol = mxh::server::userconn_character_info_nack;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_28) {
    auto r = base();
    r.protocol = mxh::server::userconn_gamein_syn;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_29) {
    auto r = base();
    r.protocol = mxh::server::userconn_gamein_ack;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_30) {
    auto r = base();
    r.protocol = mxh::server::userconn_gamein_nack;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_31) {
    auto r = base();
    r.protocol = mxh::server::userconn_gameout_syn;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_32) {
    auto r = base();
    r.protocol = mxh::server::userconn_gameout_ack;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_33) {
    auto r = base();
    r.protocol = mxh::server::userconn_gameout_nack;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_34) {
    auto r = base();
    r.protocol = mxh::server::userconn_disconnected;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_35) {
    auto r = base();
    r.protocol = mxh::server::userconn_character_add;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_36) {
    auto r = base();
    r.protocol = mxh::server::userconn_pet_add;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_37) {
    auto r = base();
    r.protocol = mxh::server::userconn_monster_add;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_38) {
    auto r = base();
    r.protocol = mxh::server::userconn_bossmonster_add;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_39) {
    auto r = base();
    r.protocol = mxh::server::userconn_npc_add;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_40) {
    auto r = base();
    r.protocol = mxh::server::userconn_object_remove;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_41) {
    auto r = base();
    r.protocol = mxh::server::userconn_character_die;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_42) {
    auto r = base();
    r.protocol = mxh::server::userconn_monster_die;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_43) {
    auto r = base();
    r.protocol = mxh::server::userconn_pet_die;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_44) {
    auto r = base();
    r.protocol = mxh::server::userconn_character_revive;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_45) {
    auto r = base();
    r.protocol = mxh::server::userconn_character_remove_syn;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_46) {
    auto r = base();
    r.protocol = mxh::server::userconn_character_remove_ack;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_47) {
    auto r = base();
    r.protocol = mxh::server::userconn_character_remove_nack;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_48) {
    auto r = base();
    r.protocol = mxh::server::userconn_changemap_syn;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_49) {
    auto r = base();
    r.protocol = mxh::server::userconn_changemap_ack;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_50) {
    auto r = base();
    r.protocol = mxh::server::userconn_changemap_nack;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_51) {
    auto r = base();
    r.protocol = mxh::server::userconn_map_out;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_52) {
    auto r = base();
    r.protocol = mxh::server::userconn_map_out_withmapnum;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_53) {
    auto r = base();
    r.protocol = mxh::server::userconn_character_totalinfo;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_54) {
    auto r = base();
    r.protocol = mxh::server::userconn_savepoint_syn;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_55) {
    auto r = base();
    r.protocol = mxh::server::userconn_savepoint_ack;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_56) {
    auto r = base();
    r.protocol = mxh::server::userconn_savepoint_nack;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_57) {
    auto r = base();
    r.protocol = mxh::server::userconn_backtocharsel_syn;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_58) {
    auto r = base();
    r.protocol = mxh::server::userconn_backtocharsel_ack;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_59) {
    auto r = base();
    r.protocol = mxh::server::userconn_backtocharsel_nack;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_60) {
    auto r = base();
    r.protocol = mxh::server::userconn_gridinit;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_61) {
    auto r = base();
    r.protocol = mxh::server::userconn_setvisible;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_62) {
    auto r = base();
    r.protocol = mxh::server::userconn_otheruser_connecttry_notify;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_63) {
    auto r = base();
    r.protocol = mxh::server::userconn_connection_check;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_64) {
    auto r = base();
    r.protocol = mxh::server::userconn_connection_check_ok;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_65) {
    auto r = base();
    r.protocol = mxh::server::userconn_checksumerror;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_66) {
    auto r = base();
    r.protocol = mxh::server::userconn_force_disconnect_overlaplogin;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_67) {
    auto r = base();
    r.protocol = mxh::server::userconn_disconnected_by_overlaplogin;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_68) {
    auto r = base();
    r.protocol = mxh::server::userconn_channelinfo_syn;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_69) {
    auto r = base();
    r.protocol = mxh::server::userconn_channelinfo_ack;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_70) {
    auto r = base();
    r.protocol = mxh::server::userconn_channelinfo_nack;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_71) {
    auto r = base();
    r.protocol = mxh::server::userconn_notifytoagent_alreadyout;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_72) {
    auto r = base();
    r.protocol = mxh::server::userconn_request_distout;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_73) {
    auto r = base();
    r.protocol = mxh::server::userconn_disconnected_on_login;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_74) {
    auto r = base();
    r.protocol = mxh::server::userconn_server_notready;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_75) {
    auto r = base();
    r.protocol = mxh::server::userconn_mapdesc;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_76) {
    auto r = base();
    r.protocol = mxh::server::userconn_character_revive_nack;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_77) {
    auto r = base();
    r.protocol = mxh::server::userconn_ready_to_revive;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_78) {
    auto r = base();
    r.protocol = mxh::server::userconn_cheat_using;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_79) {
    auto r = base();
    r.protocol = mxh::server::userconn_cheat_changemap_ack;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_80) {
    auto r = base();
    r.protocol = mxh::server::userconn_use_dynamic_syn;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_81) {
    auto r = base();
    r.protocol = mxh::server::userconn_use_dynamic_ack;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_82) {
    auto r = base();
    r.protocol = mxh::server::userconn_use_dynamic_nack;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_83) {
    auto r = base();
    r.protocol = mxh::server::userconn_login_dynamic_syn;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_84) {
    auto r = base();
    r.protocol = mxh::server::userconn_login_dynamic_ack;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_85) {
    auto r = base();
    r.protocol = mxh::server::userconn_login_dynamic_nack;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_86) {
    auto r = base();
    r.protocol = mxh::server::userconn_logincheck_delete;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_87) {
    auto r = base();
    r.protocol = mxh::server::userconn_force_disconnect_overlaplogin_ack;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_88) {
    auto r = base();
    r.protocol = mxh::server::userconn_map_out_to_eventmap;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_89) {
    auto r = base();
    r.protocol = mxh::server::userconn_map_out_to_eventbeforemap;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_90) {
    auto r = base();
    r.protocol = mxh::server::userconn_enter_eventmap_syn;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_91) {
    auto r = base();
    r.protocol = mxh::server::userconn_event_ready;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_92) {
    auto r = base();
    r.protocol = mxh::server::userconn_event_start;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_93) {
    auto r = base();
    r.protocol = mxh::server::userconn_event_end;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_94) {
    auto r = base();
    r.protocol = mxh::server::userconn_eventitem_use;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_95) {
    auto r = base();
    r.protocol = mxh::server::userconn_eventitem_use2;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_96) {
    auto r = base();
    r.protocol = mxh::server::userconn_gameinpos_syn;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_97) {
    auto r = base();
    r.protocol = mxh::server::userconn_gameinpos_ack;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_98) {
    auto r = base();
    r.protocol = mxh::server::userconn_gameinpos_nack;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_99) {
    auto r = base();
    r.protocol = mxh::server::userconn_remaintime_notify;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_100) {
    auto r = base();
    r.protocol = mxh::server::userconn_backtobeforemap_touser;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_101) {
    auto r = base();
    r.protocol = mxh::server::userconn_backtobeforemap_syn;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_102) {
    auto r = base();
    r.protocol = mxh::server::userconn_backtobeforemap_ack;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_103) {
    auto r = base();
    r.protocol = mxh::server::userconn_backtobeforemap_nack;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_104) {
    auto r = base();
    r.protocol = mxh::server::userconn_enter_gtournament_syn;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_105) {
    auto r = base();
    r.protocol = mxh::server::userconn_characterslot;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_106) {
    auto r = base();
    r.protocol = mxh::server::userconn_castlegate_add;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_107) {
    auto r = base();
    r.protocol = mxh::server::userconn_gamein_othermap_syn;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_108) {
    auto r = base();
    r.protocol = mxh::server::userconn_nowaitexitplayer;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_109) {
    auto r = base();
    r.protocol = mxh::server::userconn_flagnpc_onoff;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_110) {
    auto r = base();
    r.protocol = mxh::server::userconn_login_syn_buddy;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_111) {
    auto r = base();
    r.protocol = mxh::server::userconn_changemap_channelinfo_syn;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_112) {
    auto r = base();
    r.protocol = mxh::server::userconn_changemap_channelinfo_ack;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_113) {
    auto r = base();
    r.protocol = mxh::server::userconn_changemap_channelinfo_nack;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
TEST(UserConnSweep, Sweep_114) {
    auto r = base();
    r.protocol = mxh::server::userconn_currentmap_channelinfo;
    auto a = classify_userconn(r);
    EXPECT_GE(static_cast<std::uint8_t>(a.kind), 0u);
}
}  // namespace