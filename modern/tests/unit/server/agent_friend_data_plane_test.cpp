// agent_friend_data_plane_test.cpp
//
// Comprehensive data plane tests for mxh::server::classify_friend (D4.147).
// Augments the legacy 19-test agent_friend_test.cpp with deeper coverage of:
//   - friend_category constant = 33 (MP_FRIEND)
//   - 35 sub-protocol constants (friend_add_syn=0 .. friend_add_accept_nack_to_agent=34)
//   - 2 error codes (friend_err_add_deny=4, friend_err_option_no_friend=5)
//   - 15-value FriendActionKind enum
//   - FriendRequest struct defaults (protocol=0, object_id=0, user_found=true,
//     no_friend_option=false, from_invalid_char=false)
//   - FriendAction struct defaults
//   - classify_friend full switch dispatch (16 explicit cases + default):
//       !user_found -> drop_no_user (any protocol)
//       friend_login -> db_notify_login
//       friend_add_syn + invalid char -> drop_with_invalid_filter
//       friend_add_syn + valid -> db_add_syn
//       friend_add_accept -> db_add_syn
//       friend_add_deny -> send_add_denied_nack_to_user + friend_add_nack + friend_err_add_deny
//       friend_del_syn -> db_del_syn
//       friend_delid_syn -> db_del_id_syn
//       friend_addid_syn -> db_add_id_syn_validate
//       friend_logout_notify_to_agent -> send_logout_to_client_if_user_or_broadcast_to_agents
//       friend_logout_notify_agent_to_agent -> send_logout_to_client_if_user
//       friend_list_syn -> db_get_list
//       friend_add_ack_to_agent -> send_to_user (with friend_add_ack)
//       friend_add_nack_to_agent -> send_to_user (with friend_add_nack)
//       friend_add_accept_to_agent -> send_to_user (with friend_add_accept_ack)
//       friend_add_accept_nack_to_agent -> send_to_user (with friend_add_accept_nack)
//       friend_login_notify_to_agent -> send_to_user_login_notify
//       friend_add_invite_to_agent + no_friend_option -> nack with friend_err_option_no_friend
//       friend_add_invite_to_agent + ok -> invite with friend_add_invite
//       friend_add_nack -> send_to_user
//       default -> forward_to_client (protocol preserved)
//
// 1:1 invariants (locked):
//   - friend_category = 33
//   - friend_err_add_deny = 4, friend_err_option_no_friend = 5
//   - user_found=false always drops (overrides any case)
//   - add_syn with invalid char drops with invalid_filter (legacy filter)
//   - add_deny converts to friend_add_nack + error_code=4
//   - add_invite_to_agent + no_friend_option converts to friend_add_nack + error_code=5
//   - logout_notify_to_agent converts to friend_logout_notify_to_client
//   - logout_notify_agent_to_agent converts to friend_logout_notify_to_client

#pragma once

#include "mxh/server/agent_friend.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <set>
#include <type_traits>

namespace {

using mxh::server::classify_friend;
using mxh::server::friend_add_accept;
using mxh::server::friend_add_accept_ack;
using mxh::server::friend_add_accept_nack;
using mxh::server::friend_add_accept_nack_to_agent;
using mxh::server::friend_add_accept_to_agent;
using mxh::server::friend_add_ack;
using mxh::server::friend_add_ack_to_agent;
using mxh::server::friend_add_deny;
using mxh::server::friend_add_invite;
using mxh::server::friend_add_invite_to_agent;
using mxh::server::friend_add_nack;
using mxh::server::friend_add_nack_to_agent;
using mxh::server::friend_add_syn;
using mxh::server::friend_addid_ack;
using mxh::server::friend_addid_nack;
using mxh::server::friend_addid_syn;
using mxh::server::friend_category;
using mxh::server::friend_del_ack;
using mxh::server::friend_del_nack;
using mxh::server::friend_del_syn;
using mxh::server::friend_delid_ack;
using mxh::server::friend_delid_syn;
using mxh::server::friend_err_add_deny;
using mxh::server::friend_err_option_no_friend;
using mxh::server::friend_list_ack;
using mxh::server::friend_list_nack;
using mxh::server::friend_list_syn;
using mxh::server::friend_login;
using mxh::server::friend_login_friend;
using mxh::server::friend_login_notify;
using mxh::server::friend_login_notify_to_agent;
using mxh::server::friend_logout_notify;
using mxh::server::friend_logout_notify_agent_to_agent;
using mxh::server::friend_logout_notify_to_agent;
using mxh::server::friend_logout_notify_to_client;
using mxh::server::friend_show_list_ack;
using mxh::server::friend_show_list_nack;
using mxh::server::friend_show_list_syn;
using mxh::server::FriendAction;
using mxh::server::FriendActionKind;
using mxh::server::FriendRequest;

}  // namespace


// ===========================================================================
// friend_category constant
// ===========================================================================

TEST(FriendDataPlane, CategoryIsThirtyThree) {
    EXPECT_EQ(friend_category, 33u);
}


// ===========================================================================
// Error codes
// ===========================================================================

TEST(FriendDataPlane, ErrorCodeAddDenyIsFour) {
    EXPECT_EQ(friend_err_add_deny, 4u);
}

TEST(FriendDataPlane, ErrorCodeOptionNoFriendIsFive) {
    EXPECT_EQ(friend_err_option_no_friend, 5u);
}


// ===========================================================================
// Protocol constants -- subset (35 total)
// ===========================================================================

TEST(FriendDataPlane, ProtocolAddSynIsZero) { EXPECT_EQ(friend_add_syn, 0u); }
TEST(FriendDataPlane, ProtocolAddAckIsOne) { EXPECT_EQ(friend_add_ack, 1u); }
TEST(FriendDataPlane, ProtocolAddNackIsTwo) { EXPECT_EQ(friend_add_nack, 2u); }
TEST(FriendDataPlane, ProtocolAddInviteIsThree) { EXPECT_EQ(friend_add_invite, 3u); }
TEST(FriendDataPlane, ProtocolAddAcceptIsFour) { EXPECT_EQ(friend_add_accept, 4u); }
TEST(FriendDataPlane, ProtocolAddAcceptAckIsFive) { EXPECT_EQ(friend_add_accept_ack, 5u); }
TEST(FriendDataPlane, ProtocolAddAcceptNackIsSix) { EXPECT_EQ(friend_add_accept_nack, 6u); }
TEST(FriendDataPlane, ProtocolAddDenyIsSeven) { EXPECT_EQ(friend_add_deny, 7u); }
TEST(FriendDataPlane, ProtocolDelSynIsEight) { EXPECT_EQ(friend_del_syn, 8u); }
TEST(FriendDataPlane, ProtocolDelAckIsNine) { EXPECT_EQ(friend_del_ack, 9u); }
TEST(FriendDataPlane, ProtocolDelNackIsTen) { EXPECT_EQ(friend_del_nack, 10u); }
TEST(FriendDataPlane, ProtocolDelidSynIsEleven) { EXPECT_EQ(friend_delid_syn, 11u); }
TEST(FriendDataPlane, ProtocolDelidAckIsTwelve) { EXPECT_EQ(friend_delid_ack, 12u); }
TEST(FriendDataPlane, ProtocolShowListSynIsThirteen) { EXPECT_EQ(friend_show_list_syn, 13u); }
TEST(FriendDataPlane, ProtocolShowListAckIsFourteen) { EXPECT_EQ(friend_show_list_ack, 14u); }
TEST(FriendDataPlane, ProtocolShowListNackIsFifteen) { EXPECT_EQ(friend_show_list_nack, 15u); }
TEST(FriendDataPlane, ProtocolLoginIsSixteen) { EXPECT_EQ(friend_login, 16u); }
TEST(FriendDataPlane, ProtocolLoginNotifyIsSeventeen) { EXPECT_EQ(friend_login_notify, 17u); }
TEST(FriendDataPlane, ProtocolLoginFriendIsEighteen) { EXPECT_EQ(friend_login_friend, 18u); }
TEST(FriendDataPlane, ProtocolLogoutNotifyIsNineteen) { EXPECT_EQ(friend_logout_notify, 19u); }
TEST(FriendDataPlane, ProtocolLogoutNotifyToAgentIsTwenty) { EXPECT_EQ(friend_logout_notify_to_agent, 20u); }
TEST(FriendDataPlane, ProtocolLogoutNotifyToClientIsTwentyOne) { EXPECT_EQ(friend_logout_notify_to_client, 21u); }
TEST(FriendDataPlane, ProtocolLogoutNotifyAgentToAgentIsTwentyTwo) { EXPECT_EQ(friend_logout_notify_agent_to_agent, 22u); }
TEST(FriendDataPlane, ProtocolAddidSynIsTwentyThree) { EXPECT_EQ(friend_addid_syn, 23u); }
TEST(FriendDataPlane, ProtocolAddidAckIsTwentyFour) { EXPECT_EQ(friend_addid_ack, 24u); }
TEST(FriendDataPlane, ProtocolAddidNackIsTwentyFive) { EXPECT_EQ(friend_addid_nack, 25u); }
TEST(FriendDataPlane, ProtocolListSynIsTwentySix) { EXPECT_EQ(friend_list_syn, 26u); }
TEST(FriendDataPlane, ProtocolListAckIsTwentySeven) { EXPECT_EQ(friend_list_ack, 27u); }
TEST(FriendDataPlane, ProtocolListNackIsTwentyEight) { EXPECT_EQ(friend_list_nack, 28u); }
TEST(FriendDataPlane, ProtocolAddAcceptToAgentIsTwentyNine) { EXPECT_EQ(friend_add_accept_to_agent, 29u); }
TEST(FriendDataPlane, ProtocolLoginNotifyToAgentIsThirty) { EXPECT_EQ(friend_login_notify_to_agent, 30u); }
TEST(FriendDataPlane, ProtocolAddInviteToAgentIsThirtyOne) { EXPECT_EQ(friend_add_invite_to_agent, 31u); }
TEST(FriendDataPlane, ProtocolAddAckToAgentIsThirtyTwo) { EXPECT_EQ(friend_add_ack_to_agent, 32u); }
TEST(FriendDataPlane, ProtocolAddNackToAgentIsThirtyThree) { EXPECT_EQ(friend_add_nack_to_agent, 33u); }
TEST(FriendDataPlane, ProtocolAddAcceptNackToAgentIsThirtyFour) { EXPECT_EQ(friend_add_accept_nack_to_agent, 34u); }

TEST(FriendDataPlane, ProtocolConstantsAllDistinct) {
    std::set<std::uint8_t> seen = {
        friend_add_syn, friend_add_ack, friend_add_nack, friend_add_invite,
        friend_add_accept, friend_add_accept_ack, friend_add_accept_nack,
        friend_add_deny, friend_del_syn, friend_del_ack, friend_del_nack,
        friend_delid_syn, friend_delid_ack, friend_show_list_syn,
        friend_show_list_ack, friend_show_list_nack, friend_login,
        friend_login_notify, friend_login_friend, friend_logout_notify,
        friend_logout_notify_to_agent, friend_logout_notify_to_client,
        friend_logout_notify_agent_to_agent, friend_addid_syn, friend_addid_ack,
        friend_addid_nack, friend_list_syn, friend_list_ack, friend_list_nack,
        friend_add_accept_to_agent, friend_login_notify_to_agent,
        friend_add_invite_to_agent, friend_add_ack_to_agent,
        friend_add_nack_to_agent, friend_add_accept_nack_to_agent,
    };
    EXPECT_EQ(seen.size(), 35u);
}


// ===========================================================================
// Enum types
// ===========================================================================

TEST(FriendDataPlane, ActionKindHasFifteenValues) {
    auto all = {
        FriendActionKind::db_notify_login, FriendActionKind::db_add_syn,
        FriendActionKind::db_del_syn, FriendActionKind::db_del_id_syn,
        FriendActionKind::db_add_id_syn_validate, FriendActionKind::db_get_list,
        FriendActionKind::send_to_user, FriendActionKind::send_to_user_login_notify,
        FriendActionKind::send_invite_to_user_or_broadcast_nack,
        FriendActionKind::send_add_denied_nack_to_user,
        FriendActionKind::send_logout_to_client_if_user_or_broadcast_to_agents,
        FriendActionKind::send_logout_to_client_if_user,
        FriendActionKind::forward_to_client, FriendActionKind::drop_no_user,
        FriendActionKind::drop_with_invalid_filter,
    };
    EXPECT_EQ(all.size(), 15u);
}


// ===========================================================================
// Struct defaults
// ===========================================================================

TEST(FriendDataPlane, RequestDefaults) {
    FriendRequest r{};
    EXPECT_EQ(r.protocol, 0u);
    EXPECT_EQ(r.object_id, 0u);
    EXPECT_TRUE(r.user_found);
    EXPECT_FALSE(r.no_friend_option);
    EXPECT_FALSE(r.from_invalid_char);
}

TEST(FriendDataPlane, ActionDefaults) {
    FriendAction a{};
    EXPECT_EQ(a.kind, FriendActionKind::forward_to_client);
    EXPECT_EQ(a.protocol, 0u);
    EXPECT_EQ(a.object_id, 0u);
    EXPECT_EQ(a.error_code, 0u);
}


// ===========================================================================
// classify_friend -- !user_found wins
// ===========================================================================

TEST(FriendDataPlane, ClassifyUserNotFoundDropsLogin) {
    FriendRequest r;
    r.protocol = friend_login;
    r.user_found = false;
    EXPECT_EQ(classify_friend(r).kind, FriendActionKind::drop_no_user);
}

TEST(FriendDataPlane, ClassifyUserNotFoundDropsAddSyn) {
    FriendRequest r;
    r.protocol = friend_add_syn;
    r.user_found = false;
    EXPECT_EQ(classify_friend(r).kind, FriendActionKind::drop_no_user);
}

TEST(FriendDataPlane, ClassifyUserNotFoundPreservesProtocol) {
    FriendRequest r;
    r.protocol = friend_add_syn;
    r.user_found = false;
    EXPECT_EQ(classify_friend(r).protocol, friend_add_syn);
}


// ===========================================================================
// classify_friend -- switch dispatch (all 16 cases + default)
// ===========================================================================

TEST(FriendDataPlane, ClassifyLoginNotifiesLoginAndNotes) {
    FriendRequest r;
    r.protocol = friend_login;
    auto a = classify_friend(r);
    EXPECT_EQ(a.kind, FriendActionKind::db_notify_login);
    EXPECT_EQ(a.protocol, friend_login);
    EXPECT_EQ(a.error_code, 0u);
}

TEST(FriendDataPlane, ClassifyAddSynValidCallsAddFriend) {
    FriendRequest r;
    r.protocol = friend_add_syn;
    EXPECT_EQ(classify_friend(r).kind, FriendActionKind::db_add_syn);
}

TEST(FriendDataPlane, ClassifyAddSynInvalidCharDrops) {
    FriendRequest r;
    r.protocol = friend_add_syn;
    r.from_invalid_char = true;
    EXPECT_EQ(classify_friend(r).kind, FriendActionKind::drop_with_invalid_filter);
}

TEST(FriendDataPlane, ClassifyAddAcceptCommitsAdd) {
    FriendRequest r;
    r.protocol = friend_add_accept;
    EXPECT_EQ(classify_friend(r).kind, FriendActionKind::db_add_syn);
}

TEST(FriendDataPlane, ClassifyAddDenySendsNackWithAddDenyCode) {
    FriendRequest r;
    r.protocol = friend_add_deny;
    auto a = classify_friend(r);
    EXPECT_EQ(a.kind, FriendActionKind::send_add_denied_nack_to_user);
    EXPECT_EQ(a.protocol, friend_add_nack);
    EXPECT_EQ(a.error_code, friend_err_add_deny);
}

TEST(FriendDataPlane, ClassifyDelSynDeletesFriend) {
    FriendRequest r;
    r.protocol = friend_del_syn;
    EXPECT_EQ(classify_friend(r).kind, FriendActionKind::db_del_syn);
}

TEST(FriendDataPlane, ClassifyDelIdSynDeletesById) {
    FriendRequest r;
    r.protocol = friend_delid_syn;
    EXPECT_EQ(classify_friend(r).kind, FriendActionKind::db_del_id_syn);
}

TEST(FriendDataPlane, ClassifyAddIdSynValidatesTarget) {
    FriendRequest r;
    r.protocol = friend_addid_syn;
    EXPECT_EQ(classify_friend(r).kind, FriendActionKind::db_add_id_syn_validate);
}

TEST(FriendDataPlane, ClassifyLogoutNotifyToAgentConvertsToClient) {
    FriendRequest r;
    r.protocol = friend_logout_notify_to_agent;
    auto a = classify_friend(r);
    EXPECT_EQ(a.kind, FriendActionKind::send_logout_to_client_if_user_or_broadcast_to_agents);
    EXPECT_EQ(a.protocol, friend_logout_notify_to_client);
}

TEST(FriendDataPlane, ClassifyLogoutNotifyAgentToAgentForwardsClientOnly) {
    FriendRequest r;
    r.protocol = friend_logout_notify_agent_to_agent;
    EXPECT_EQ(classify_friend(r).kind, FriendActionKind::send_logout_to_client_if_user);
}

TEST(FriendDataPlane, ClassifyListSynFetchesFriendList) {
    FriendRequest r;
    r.protocol = friend_list_syn;
    EXPECT_EQ(classify_friend(r).kind, FriendActionKind::db_get_list);
}

TEST(FriendDataPlane, ClassifyAddAckToAgentSendsToUser) {
    FriendRequest r;
    r.protocol = friend_add_ack_to_agent;
    auto a = classify_friend(r);
    EXPECT_EQ(a.kind, FriendActionKind::send_to_user);
    EXPECT_EQ(a.protocol, friend_add_ack);
}

TEST(FriendDataPlane, ClassifyAddNackToAgentSendsToUser) {
    FriendRequest r;
    r.protocol = friend_add_nack_to_agent;
    EXPECT_EQ(classify_friend(r).kind, FriendActionKind::send_to_user);
    EXPECT_EQ(classify_friend(r).protocol, friend_add_nack);
}

TEST(FriendDataPlane, ClassifyAddAcceptToAgentSendsAcceptAckToUser) {
    FriendRequest r;
    r.protocol = friend_add_accept_to_agent;
    auto a = classify_friend(r);
    EXPECT_EQ(a.kind, FriendActionKind::send_to_user);
    EXPECT_EQ(a.protocol, friend_add_accept_ack);
}

TEST(FriendDataPlane, ClassifyAddAcceptNackToAgentSendsAcceptNackToUser) {
    FriendRequest r;
    r.protocol = friend_add_accept_nack_to_agent;
    auto a = classify_friend(r);
    EXPECT_EQ(a.kind, FriendActionKind::send_to_user);
    EXPECT_EQ(a.protocol, friend_add_accept_nack);
}

TEST(FriendDataPlane, ClassifyLoginNotifyToAgentSendsLoginNotifyToUser) {
    FriendRequest r;
    r.protocol = friend_login_notify_to_agent;
    auto a = classify_friend(r);
    EXPECT_EQ(a.protocol, friend_login_notify);
    EXPECT_EQ(a.kind, FriendActionKind::send_to_user_login_notify);
}

TEST(FriendDataPlane, ClassifyAddInviteNoFriendOptionSendsNoFriendNack) {
    FriendRequest r;
    r.protocol = friend_add_invite_to_agent;
    r.no_friend_option = true;
    auto a = classify_friend(r);
    EXPECT_EQ(a.kind, FriendActionKind::send_invite_to_user_or_broadcast_nack);
    EXPECT_EQ(a.protocol, friend_add_nack);
    EXPECT_EQ(a.error_code, friend_err_option_no_friend);
}

TEST(FriendDataPlane, ClassifyAddInviteNormalSendsInvite) {
    FriendRequest r;
    r.protocol = friend_add_invite_to_agent;
    r.no_friend_option = false;
    auto a = classify_friend(r);
    EXPECT_EQ(a.kind, FriendActionKind::send_invite_to_user_or_broadcast_nack);
    EXPECT_EQ(a.protocol, friend_add_invite);
    EXPECT_EQ(a.error_code, 0u);
}

TEST(FriendDataPlane, ClassifyAddNackSendsToUser) {
    FriendRequest r;
    r.protocol = friend_add_nack;
    EXPECT_EQ(classify_friend(r).kind, FriendActionKind::send_to_user);
}


// ===========================================================================
// classify_friend -- default path
// ===========================================================================

TEST(FriendDataPlane, ClassifyUnknownProtocolForwardsToClient) {
    FriendRequest r;
    r.protocol = 99;
    EXPECT_EQ(classify_friend(r).kind, FriendActionKind::forward_to_client);
}

TEST(FriendDataPlane, ClassifyUnknownProtocolPreservesProtocol) {
    FriendRequest r;
    r.protocol = 99;
    auto a = classify_friend(r);
    EXPECT_EQ(a.protocol, 99u);
}

TEST(FriendDataPlane, ClassifyProtocol255ForwardsToClient) {
    FriendRequest r;
    r.protocol = 255;
    EXPECT_EQ(classify_friend(r).kind, FriendActionKind::forward_to_client);
}


// ===========================================================================
// Object_id preservation
// ===========================================================================

TEST(FriendDataPlane, ClassifyPreservesObjectIdLogin) {
    FriendRequest r;
    r.protocol = friend_login;
    r.object_id = 0xDEADBEEFu;
    auto a = classify_friend(r);
    EXPECT_EQ(a.object_id, 0xDEADBEEFu);
}

TEST(FriendDataPlane, ClassifyPreservesObjectIdAddDeny) {
    FriendRequest r;
    r.protocol = friend_add_deny;
    r.object_id = 0xCAFEBABEu;
    auto a = classify_friend(r);
    EXPECT_EQ(a.object_id, 0xCAFEBABEu);
}

TEST(FriendDataPlane, ClassifyPreservesObjectIdDefault) {
    FriendRequest r;
    r.protocol = 99;
    r.object_id = 0x12345678u;
    auto a = classify_friend(r);
    EXPECT_EQ(a.object_id, 0x12345678u);
}

TEST(FriendDataPlane, ClassifyPreservesObjectIdUserNotFound) {
    FriendRequest r;
    r.protocol = friend_add_syn;
    r.user_found = false;
    r.object_id = 0xFEEDFACEu;
    auto a = classify_friend(r);
    EXPECT_EQ(a.object_id, 0xFEEDFACEu);
}
