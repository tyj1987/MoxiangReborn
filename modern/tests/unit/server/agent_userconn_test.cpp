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

}  // namespace