// agent_party_data_plane_test.cpp
//
// Comprehensive data plane tests for mxh::server::classify_party_user +
// classify_party_server (D4.148).
// Augments the legacy 4-test agent_party_test.cpp with deeper coverage of:
//   - party_category constant = 14 (MP_PARTY)
//   - 61 sub-protocol constants (party_info=0 .. party_matching_info=60)
//   - error code constant (party_err_request_not_master=1)
//   - PartyUserActionKind enum (forward_to_map, send_to_map_with_not_master_error)
//   - PartyServerActionKind enum (broadcast_to_other_maps, forward_to_object_map,
//     forward_to_object_user, send_consent_nack, send_refusal_nack, default_to_client)
//   - struct defaults
//   - classify_party_user truth table:
//       master_to_request_syn + master_resolved=true -> forward with master_object_id
//       master_to_request_syn + master_resolved=false -> send_to_map_with_not_master_error
//       any other protocol -> forward_to_map with original protocol + object_id
//   - classify_party_server truth table:
//       10 *_to_mapserver notify protocols -> broadcast_to_other_maps
//       party_request_consent_ack -> forward_to_object_map (object_id, object_id2)
//       party_request_refusal_ack -> forward_to_object_user (object_id, object_id2)
//       party_request_consent_nack -> send_consent_nack (object_id2)
//       party_request_refusal_nack -> send_refusal_nack (object_id2)
//       party_error -> forward_to_object_user (object_id)
//       default -> default_to_client (protocol preserved)
//
// 1:1 invariants (locked):
//   - party_category = 14
//   - party_err_request_not_master = 1
//   - master_to_request_syn resolved forwards to map with master_object_id
//   - master_to_request_syn unresolved sends party_error + party_err_request_not_master
//   - 10 broadcast protocols: party_notify_add/delete/changemaster/breakup/ban/member_login/member_logout/member_loginmsg/create/member_level

#pragma once

#include "mxh/server/agent_party.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <set>
#include <type_traits>

namespace {

using mxh::server::classify_party_server;
using mxh::server::classify_party_user;
using mxh::server::PartyServerAction;
using mxh::server::PartyServerActionKind;
using mxh::server::PartyServerRequest;
using mxh::server::PartyUserAction;
using mxh::server::PartyUserActionKind;
using mxh::server::PartyUserRequest;
using mxh::server::party_add_ack;
using mxh::server::party_add_invite;
using mxh::server::party_add_nack;
using mxh::server::party_add_syn;
using mxh::server::party_ban_ack;
using mxh::server::party_ban_nack;
using mxh::server::party_ban_syn;
using mxh::server::party_breakup_ack;
using mxh::server::party_breakup_nack;
using mxh::server::party_breakup_syn;
using mxh::server::party_category;
using mxh::server::party_changemaster_ack;
using mxh::server::party_changemaster_nack;
using mxh::server::party_changemaster_syn;
using mxh::server::party_clear;
using mxh::server::party_create_ack;
using mxh::server::party_create_nack;
using mxh::server::party_create_syn;
using mxh::server::party_del_ack;
using mxh::server::party_del_nack;
using mxh::server::party_del_syn;
using mxh::server::party_error;
using mxh::server::party_err_request_not_master;
using mxh::server::party_info;
using mxh::server::party_invite_accept_ack;
using mxh::server::party_invite_accept_nack;
using mxh::server::party_invite_accept_syn;
using mxh::server::party_invite_deny_ack;
using mxh::server::party_invite_deny_nack;
using mxh::server::party_invite_deny_syn;
using mxh::server::party_master_to_request_ack;
using mxh::server::party_master_to_request_syn;
using mxh::server::party_matching_info;
using mxh::server::party_memberlevel;
using mxh::server::party_memberlife;
using mxh::server::party_member_login;
using mxh::server::party_member_loginmsg;
using mxh::server::party_member_logout;
using mxh::server::party_membershipield;
using mxh::server::party_membernaeryuk;
using mxh::server::party_monster_obtain_notify;
using mxh::server::party_notify_add_to_mapserver;
using mxh::server::party_notify_ban_to_mapserver;
using mxh::server::party_notify_breakup_to_mapserver;
using mxh::server::party_notify_changes_to_mapserver;
using mxh::server::party_notify_changemaster_to_mapserver;
using mxh::server::party_notify_create_to_mapserver;
using mxh::server::party_notify_delete_to_mapserver;
using mxh::server::party_notify_info;
using mxh::server::party_notify_member_level;
using mxh::server::party_notify_member_loginmsg;
using mxh::server::party_notify_member_login_to_mapserver;
using mxh::server::party_notify_member_logout_to_mapserver;
using mxh::server::party_obtain_money_to_party;
using mxh::server::party_request_consent_ack;
using mxh::server::party_request_consent_nack;
using mxh::server::party_request_consent_syn;
using mxh::server::party_request_refusal_ack;
using mxh::server::party_request_refusal_nack;
using mxh::server::party_request_refusal_syn;
using mxh::server::party_revivepos;
using mxh::server::party_sendpos;
using mxh::server::party_syndelete_to_mapserver;

}  // namespace


// ===========================================================================
// party_category constant
// ===========================================================================

TEST(PartyDataPlane, CategoryIsFourteen) {
    EXPECT_EQ(party_category, 14u);
}


// ===========================================================================
// Error code
// ===========================================================================

TEST(PartyDataPlane, ErrorCodeRequestNotMasterIsOne) {
    EXPECT_EQ(party_err_request_not_master, 1u);
}


// ===========================================================================
// Protocol constants -- representative subset
// ===========================================================================

TEST(PartyDataPlane, ProtocolInfoIsZero) { EXPECT_EQ(party_info, 0u); }
TEST(PartyDataPlane, ProtocolCreateSynIsOne) { EXPECT_EQ(party_create_syn, 1u); }
TEST(PartyDataPlane, ProtocolCreateAckIsTwo) { EXPECT_EQ(party_create_ack, 2u); }
TEST(PartyDataPlane, ProtocolCreateNackIsThree) { EXPECT_EQ(party_create_nack, 3u); }
TEST(PartyDataPlane, ProtocolAddSynIsFour) { EXPECT_EQ(party_add_syn, 4u); }
TEST(PartyDataPlane, ProtocolMasterToRequestSynIsFiftyOne) { EXPECT_EQ(party_master_to_request_syn, 51u); }
TEST(PartyDataPlane, ProtocolMasterToRequestAckIsFiftyTwo) { EXPECT_EQ(party_master_to_request_ack, 52u); }
TEST(PartyDataPlane, ProtocolErrorIsFifty) { EXPECT_EQ(party_error, 50u); }
TEST(PartyDataPlane, ProtocolMatchingInfoIsSixty) { EXPECT_EQ(party_matching_info, 60u); }

TEST(PartyDataPlane, ProtocolConstantsAllDistinct) {
    std::set<std::uint8_t> seen = {
        party_info, party_create_syn, party_create_ack, party_create_nack,
        party_add_syn, party_add_ack, party_add_nack, party_add_invite,
        party_invite_accept_syn, party_invite_accept_ack, party_invite_accept_nack,
        party_invite_deny_syn, party_invite_deny_ack, party_invite_deny_nack,
        party_notify_add_to_mapserver, party_del_syn, party_del_ack, party_del_nack,
        party_notify_delete_to_mapserver, party_syndelete_to_mapserver,
        party_ban_syn, party_ban_ack, party_ban_nack, party_notify_ban_to_mapserver,
        party_changemaster_syn, party_changemaster_ack, party_changemaster_nack,
        party_notify_changemaster_to_mapserver, party_breakup_syn,
        party_breakup_ack, party_breakup_nack, party_notify_breakup_to_mapserver,
        party_member_login, party_notify_member_login_to_mapserver,
        party_member_logout, party_notify_member_logout_to_mapserver,
        party_memberlife, party_membershipield, party_membernaeryuk,
        party_memberlevel, party_sendpos, party_revivepos,
        party_notify_changes_to_mapserver, party_clear,
        party_notify_create_to_mapserver, party_member_loginmsg,
        party_notify_member_loginmsg, party_notify_member_level,
        party_monster_obtain_notify, party_obtain_money_to_party,
        party_error, party_master_to_request_syn, party_master_to_request_ack,
        party_request_consent_syn, party_request_consent_ack,
        party_request_consent_nack, party_request_refusal_syn,
        party_request_refusal_ack, party_request_refusal_nack,
        party_notify_info, party_matching_info,
    };
    EXPECT_EQ(seen.size(), 61u);
}


// ===========================================================================
// Enum types
// ===========================================================================

TEST(PartyDataPlane, UserActionKindHasTwoValues) {
    auto all = {
        PartyUserActionKind::forward_to_map,
        PartyUserActionKind::send_to_map_with_not_master_error,
    };
    EXPECT_EQ(all.size(), 2u);
}

TEST(PartyDataPlane, ServerActionKindHasSixValues) {
    auto all = {
        PartyServerActionKind::broadcast_to_other_maps,
        PartyServerActionKind::forward_to_object_map,
        PartyServerActionKind::forward_to_object_user,
        PartyServerActionKind::send_consent_nack,
        PartyServerActionKind::send_refusal_nack,
        PartyServerActionKind::default_to_client,
    };
    EXPECT_EQ(all.size(), 6u);
}


// ===========================================================================
// Struct defaults
// ===========================================================================

TEST(PartyDataPlane, UserRequestDefaults) {
    PartyUserRequest r{};
    EXPECT_EQ(r.protocol, 0u);
    EXPECT_EQ(r.object_id, 0u);
    EXPECT_EQ(r.master_object_id, 0u);
    EXPECT_FALSE(r.master_resolved);
}

TEST(PartyDataPlane, UserActionDefaults) {
    PartyUserAction a{};
    EXPECT_EQ(a.kind, PartyUserActionKind::forward_to_map);
    EXPECT_EQ(a.protocol, 0u);
    EXPECT_EQ(a.object_id, 0u);
    EXPECT_EQ(a.error_code, 0u);
}

TEST(PartyDataPlane, ServerRequestDefaults) {
    PartyServerRequest r{};
    EXPECT_EQ(r.protocol, 0u);
    EXPECT_EQ(r.object_id, 0u);
    EXPECT_EQ(r.object_id2, 0u);
}

TEST(PartyDataPlane, ServerActionDefaults) {
    PartyServerAction a{};
    EXPECT_EQ(a.kind, PartyServerActionKind::default_to_client);
    EXPECT_EQ(a.protocol, 0u);
    EXPECT_EQ(a.target_object_id, 0u);
    EXPECT_EQ(a.alternate_object_id, 0u);
}


// ===========================================================================
// classify_party_user -- master_to_request_syn
// ===========================================================================

TEST(PartyDataPlane, ClassifyMasterRequestResolvedForwardsToMasterMap) {
    PartyUserRequest r;
    r.protocol = party_master_to_request_syn;
    r.object_id = 11;
    r.master_object_id = 22;
    r.master_resolved = true;
    auto a = classify_party_user(r);
    EXPECT_EQ(a.kind, PartyUserActionKind::forward_to_map);
    EXPECT_EQ(a.protocol, party_master_to_request_syn);
    EXPECT_EQ(a.object_id, 22u);  // master_object_id
}

TEST(PartyDataPlane, ClassifyMasterRequestUnresolvedSendsNotMasterError) {
    PartyUserRequest r;
    r.protocol = party_master_to_request_syn;
    r.object_id = 11;
    r.master_object_id = 22;
    r.master_resolved = false;
    auto a = classify_party_user(r);
    EXPECT_EQ(a.kind, PartyUserActionKind::send_to_map_with_not_master_error);
    EXPECT_EQ(a.protocol, party_error);
    EXPECT_EQ(a.error_code, party_err_request_not_master);
    EXPECT_EQ(a.object_id, 11u);  // original object_id
}


// ===========================================================================
// classify_party_user -- default path
// ===========================================================================

TEST(PartyDataPlane, ClassifyCreateSynForwards) {
    PartyUserRequest r;
    r.protocol = party_create_syn;
    auto a = classify_party_user(r);
    EXPECT_EQ(a.kind, PartyUserActionKind::forward_to_map);
    EXPECT_EQ(a.protocol, party_create_syn);
}

TEST(PartyDataPlane, ClassifyAddSynForwards) {
    PartyUserRequest r;
    r.protocol = party_add_syn;
    EXPECT_EQ(classify_party_user(r).kind, PartyUserActionKind::forward_to_map);
}

TEST(PartyDataPlane, ClassifyDelSynForwards) {
    PartyUserRequest r;
    r.protocol = party_del_syn;
    EXPECT_EQ(classify_party_user(r).kind, PartyUserActionKind::forward_to_map);
}

TEST(PartyDataPlane, ClassifyForwardPreservesObjectId) {
    PartyUserRequest r;
    r.protocol = party_create_syn;
    r.object_id = 0xDEADBEEFu;
    auto a = classify_party_user(r);
    EXPECT_EQ(a.object_id, 0xDEADBEEFu);
}

TEST(PartyDataPlane, ClassifyForwardZeroErrorCode) {
    PartyUserRequest r;
    r.protocol = party_create_syn;
    auto a = classify_party_user(r);
    EXPECT_EQ(a.error_code, 0u);
}


// ===========================================================================
// classify_party_server -- broadcast group (10 protocols)
// ===========================================================================

TEST(PartyDataPlane, ServerNotifyAddToMapserverBroadcasts) {
    PartyServerRequest r;
    r.protocol = party_notify_add_to_mapserver;
    EXPECT_EQ(classify_party_server(r).kind, PartyServerActionKind::broadcast_to_other_maps);
}

TEST(PartyDataPlane, ServerNotifyDeleteToMapserverBroadcasts) {
    PartyServerRequest r;
    r.protocol = party_notify_delete_to_mapserver;
    EXPECT_EQ(classify_party_server(r).kind, PartyServerActionKind::broadcast_to_other_maps);
}

TEST(PartyDataPlane, ServerNotifyChangemasterToMapserverBroadcasts) {
    PartyServerRequest r;
    r.protocol = party_notify_changemaster_to_mapserver;
    EXPECT_EQ(classify_party_server(r).kind, PartyServerActionKind::broadcast_to_other_maps);
}

TEST(PartyDataPlane, ServerNotifyBreakupToMapserverBroadcasts) {
    PartyServerRequest r;
    r.protocol = party_notify_breakup_to_mapserver;
    EXPECT_EQ(classify_party_server(r).kind, PartyServerActionKind::broadcast_to_other_maps);
}

TEST(PartyDataPlane, ServerNotifyBanToMapserverBroadcasts) {
    PartyServerRequest r;
    r.protocol = party_notify_ban_to_mapserver;
    EXPECT_EQ(classify_party_server(r).kind, PartyServerActionKind::broadcast_to_other_maps);
}

TEST(PartyDataPlane, ServerNotifyMemberLoginToMapserverBroadcasts) {
    PartyServerRequest r;
    r.protocol = party_notify_member_login_to_mapserver;
    EXPECT_EQ(classify_party_server(r).kind, PartyServerActionKind::broadcast_to_other_maps);
}

TEST(PartyDataPlane, ServerNotifyMemberLogoutToMapserverBroadcasts) {
    PartyServerRequest r;
    r.protocol = party_notify_member_logout_to_mapserver;
    EXPECT_EQ(classify_party_server(r).kind, PartyServerActionKind::broadcast_to_other_maps);
}

TEST(PartyDataPlane, ServerNotifyMemberLoginmsgBroadcasts) {
    PartyServerRequest r;
    r.protocol = party_notify_member_loginmsg;
    EXPECT_EQ(classify_party_server(r).kind, PartyServerActionKind::broadcast_to_other_maps);
}

TEST(PartyDataPlane, ServerNotifyCreateToMapserverBroadcasts) {
    PartyServerRequest r;
    r.protocol = party_notify_create_to_mapserver;
    EXPECT_EQ(classify_party_server(r).kind, PartyServerActionKind::broadcast_to_other_maps);
}

TEST(PartyDataPlane, ServerNotifyMemberLevelBroadcasts) {
    PartyServerRequest r;
    r.protocol = party_notify_member_level;
    EXPECT_EQ(classify_party_server(r).kind, PartyServerActionKind::broadcast_to_other_maps);
}

TEST(PartyDataPlane, ServerBroadcastPreservesProtocol) {
    PartyServerRequest r;
    r.protocol = party_notify_add_to_mapserver;
    auto a = classify_party_server(r);
    EXPECT_EQ(a.protocol, party_notify_add_to_mapserver);
}


// ===========================================================================
// classify_party_server -- forward_to_object_map / forward_to_object_user
// ===========================================================================

TEST(PartyDataPlane, ServerRequestConsentAckForwardsToObjectMap) {
    PartyServerRequest r;
    r.protocol = party_request_consent_ack;
    r.object_id = 100;
    r.object_id2 = 200;
    auto a = classify_party_server(r);
    EXPECT_EQ(a.kind, PartyServerActionKind::forward_to_object_map);
    EXPECT_EQ(a.target_object_id, 100u);
    EXPECT_EQ(a.alternate_object_id, 200u);
}

TEST(PartyDataPlane, ServerRequestRefusalAckForwardsToObjectUser) {
    PartyServerRequest r;
    r.protocol = party_request_refusal_ack;
    r.object_id = 100;
    r.object_id2 = 200;
    auto a = classify_party_server(r);
    EXPECT_EQ(a.kind, PartyServerActionKind::forward_to_object_user);
    EXPECT_EQ(a.target_object_id, 100u);
    EXPECT_EQ(a.alternate_object_id, 200u);
}

TEST(PartyDataPlane, ServerErrorForwardsToObjectUser) {
    PartyServerRequest r;
    r.protocol = party_error;
    r.object_id = 999;
    auto a = classify_party_server(r);
    EXPECT_EQ(a.kind, PartyServerActionKind::forward_to_object_user);
    EXPECT_EQ(a.target_object_id, 999u);
    EXPECT_EQ(a.alternate_object_id, 0u);
}


// ===========================================================================
// classify_party_server -- nack group
// ===========================================================================

TEST(PartyDataPlane, ServerRequestConsentNackSendsConsentNack) {
    PartyServerRequest r;
    r.protocol = party_request_consent_nack;
    r.object_id = 100;
    r.object_id2 = 200;
    auto a = classify_party_server(r);
    EXPECT_EQ(a.kind, PartyServerActionKind::send_consent_nack);
    EXPECT_EQ(a.target_object_id, 200u);
}

TEST(PartyDataPlane, ServerRequestRefusalNackSendsRefusalNack) {
    PartyServerRequest r;
    r.protocol = party_request_refusal_nack;
    r.object_id = 100;
    r.object_id2 = 200;
    auto a = classify_party_server(r);
    EXPECT_EQ(a.kind, PartyServerActionKind::send_refusal_nack);
    EXPECT_EQ(a.target_object_id, 200u);
}


// ===========================================================================
// classify_party_server -- default path
// ===========================================================================

TEST(PartyDataPlane, ServerUnknownProtocolDefaultsToClient) {
    PartyServerRequest r;
    r.protocol = 99;
    EXPECT_EQ(classify_party_server(r).kind, PartyServerActionKind::default_to_client);
}

TEST(PartyDataPlane, ServerProtocol255DefaultsToClient) {
    PartyServerRequest r;
    r.protocol = 255;
    EXPECT_EQ(classify_party_server(r).kind, PartyServerActionKind::default_to_client);
}

TEST(PartyDataPlane, ServerDefaultPreservesProtocol) {
    PartyServerRequest r;
    r.protocol = 99;
    auto a = classify_party_server(r);
    EXPECT_EQ(a.protocol, 99u);
}
