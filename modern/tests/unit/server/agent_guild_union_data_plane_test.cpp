// agent_guild_union_data_plane_test.cpp
//
// Comprehensive data plane tests for mxh::server::classify_guild_union_user +
// classify_guild_union_server (D4.142).
// Augments the legacy 5-test agent_guild_union_test.cpp with deeper coverage of:
//   - guild_union_category = 61
//   - user-side protocol constants (create_syn=2, create_nack=4)
//   - server-side notify constants (20..26, 7 distinct)
//   - error_code constant (guild_union_err_not_valid_name=1)
//   - GuildUnionActionKind enum (forward_to_map, send_create_nack_to_user, drop_no_user)
//   - GuildUnionServerActionKind enum (broadcast_to_other_maps, default_forward_to_client, drop_unknown)
//   - classify_guild_union_user truth table:
//       user_found=false overrides everything -> drop_no_user
//       create_syn + bad_name -> send_create_nack_to_user
//       create_syn + good_name -> forward_to_map
//       non-create protocol -> forward_to_map with protocol preserved
//   - classify_guild_union_server protocol sweep: 7 notify protocols broadcast,
//     everything else drop_unknown
//
// 1:1 invariants (locked):
//   - guild_union_category = 61
//   - guild_union_create_syn=2, guild_union_create_nack=4
//   - 7 notify protocols: create=20, destroy=21, invite_accept=22, add=23,
//     remove=24, secede=25, mark_regist=26
//   - guild_union_err_not_valid_name = 1
//   - User dispatch: user_found=false wins regardless of other fields
//   - User dispatch: only create_syn checks name_usable/has_invalid_char
//   - Server dispatch: 7 notify protocols broadcast; default drop_unknown

#pragma once

#include "mxh/server/agent_guild_union.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <set>
#include <type_traits>
#include <vector>

namespace {

using mxh::server::classify_guild_union_server;
using mxh::server::classify_guild_union_user;
using mxh::server::guild_union_add_notify_to_map;
using mxh::server::guild_union_category;
using mxh::server::guild_union_create_nack;
using mxh::server::guild_union_create_notify_to_map;
using mxh::server::guild_union_create_syn;
using mxh::server::guild_union_destroy_notify_to_map;
using mxh::server::guild_union_err_not_valid_name;
using mxh::server::guild_union_invite_accept_notify_to_map;
using mxh::server::guild_union_mark_regist_notify_to_map;
using mxh::server::guild_union_remove_notify_to_map;
using mxh::server::guild_union_secede_notify_to_map;
using mxh::server::GuildUnionAction;
using mxh::server::GuildUnionActionKind;
using mxh::server::GuildUnionRequest;
using mxh::server::GuildUnionServerAction;
using mxh::server::GuildUnionServerActionKind;
using mxh::server::GuildUnionServerRequest;

}  // namespace


// ===========================================================================
// guild_union_category constant
// ===========================================================================

TEST(GuildUnionDataPlane, CategoryIsSixtyOne) {
    EXPECT_EQ(guild_union_category, 61u);
}


// ===========================================================================
// Protocol constants -- user-side
// ===========================================================================

TEST(GuildUnionDataPlane, CreateSynIsTwo) { EXPECT_EQ(guild_union_create_syn, 2u); }
TEST(GuildUnionDataPlane, CreateNackIsFour) { EXPECT_EQ(guild_union_create_nack, 4u); }


// ===========================================================================
// Protocol constants -- server-side notify group (20..26)
// ===========================================================================

TEST(GuildUnionDataPlane, CreateNotifyToMapIsTwenty) { EXPECT_EQ(guild_union_create_notify_to_map, 20u); }
TEST(GuildUnionDataPlane, DestroyNotifyToMapIsTwentyOne) { EXPECT_EQ(guild_union_destroy_notify_to_map, 21u); }
TEST(GuildUnionDataPlane, InviteAcceptNotifyToMapIsTwentyTwo) { EXPECT_EQ(guild_union_invite_accept_notify_to_map, 22u); }
TEST(GuildUnionDataPlane, AddNotifyToMapIsTwentyThree) { EXPECT_EQ(guild_union_add_notify_to_map, 23u); }
TEST(GuildUnionDataPlane, RemoveNotifyToMapIsTwentyFour) { EXPECT_EQ(guild_union_remove_notify_to_map, 24u); }
TEST(GuildUnionDataPlane, SecedeNotifyToMapIsTwentyFive) { EXPECT_EQ(guild_union_secede_notify_to_map, 25u); }
TEST(GuildUnionDataPlane, MarkRegistNotifyToMapIsTwentySix) { EXPECT_EQ(guild_union_mark_regist_notify_to_map, 26u); }

TEST(GuildUnionDataPlane, NotifyProtocolsAllDistinct) {
    std::set<std::uint8_t> seen = {
        guild_union_create_notify_to_map, guild_union_destroy_notify_to_map,
        guild_union_invite_accept_notify_to_map, guild_union_add_notify_to_map,
        guild_union_remove_notify_to_map, guild_union_secede_notify_to_map,
        guild_union_mark_regist_notify_to_map,
    };
    EXPECT_EQ(seen.size(), 7u);
}

TEST(GuildUnionDataPlane, ErrorCodeNotValidNameIsOne) {
    EXPECT_EQ(guild_union_err_not_valid_name, 1u);
}


// ===========================================================================
// Enum types
// ===========================================================================

TEST(GuildUnionDataPlane, ActionKindHasThreeValues) {
    auto all = {
        GuildUnionActionKind::forward_to_map,
        GuildUnionActionKind::send_create_nack_to_user,
        GuildUnionActionKind::drop_no_user,
    };
    EXPECT_EQ(all.size(), 3u);
}

TEST(GuildUnionDataPlane, ServerActionKindHasThreeValues) {
    auto all = {
        GuildUnionServerActionKind::broadcast_to_other_maps,
        GuildUnionServerActionKind::default_forward_to_client,
        GuildUnionServerActionKind::drop_unknown,
    };
    EXPECT_EQ(all.size(), 3u);
}


// ===========================================================================
// Struct defaults
// ===========================================================================

TEST(GuildUnionDataPlane, RequestDefaults) {
    GuildUnionRequest r{};
    EXPECT_EQ(r.protocol, 0u);
    EXPECT_EQ(r.object_id, 0u);
    EXPECT_TRUE(r.user_found);
    EXPECT_TRUE(r.name_usable);
    EXPECT_FALSE(r.has_invalid_char);
}

TEST(GuildUnionDataPlane, ActionDefaults) {
    GuildUnionAction a{};
    EXPECT_EQ(a.kind, GuildUnionActionKind::forward_to_map);
    EXPECT_EQ(a.protocol, 0u);
    EXPECT_EQ(a.object_id, 0u);
    EXPECT_EQ(a.error_code, 0u);
}

TEST(GuildUnionDataPlane, ServerRequestDefaults) {
    GuildUnionServerRequest r{};
    EXPECT_EQ(r.protocol, 0u);
    EXPECT_EQ(r.object_id, 0u);
}

TEST(GuildUnionDataPlane, ServerActionDefaults) {
    GuildUnionServerAction a{};
    EXPECT_EQ(a.kind, GuildUnionServerActionKind::default_forward_to_client);
    EXPECT_EQ(a.protocol, 0u);
}


// ===========================================================================
// classify_guild_union_user -- drop_no_user path
// ===========================================================================

TEST(GuildUnionDataPlane, ClassifyUserNotFoundDrops) {
    GuildUnionRequest r;
    r.protocol = guild_union_create_syn;
    r.user_found = false;
    EXPECT_EQ(classify_guild_union_user(r).kind, GuildUnionActionKind::drop_no_user);
}

TEST(GuildUnionDataPlane, ClassifyUserNotFoundPreservesProtocol) {
    GuildUnionRequest r;
    r.protocol = 99;
    r.user_found = false;
    auto a = classify_guild_union_user(r);
    EXPECT_EQ(a.kind, GuildUnionActionKind::drop_no_user);
    EXPECT_EQ(a.protocol, 99u);
}

TEST(GuildUnionDataPlane, ClassifyUserNotFoundOverridesCreateSynPath) {
    // user_found=false wins, even on create_syn.
    GuildUnionRequest r;
    r.protocol = guild_union_create_syn;
    r.user_found = false;
    r.name_usable = true;
    r.has_invalid_char = false;
    EXPECT_EQ(classify_guild_union_user(r).kind, GuildUnionActionKind::drop_no_user);
}


// ===========================================================================
// classify_guild_union_user -- create_syn path
// ===========================================================================

TEST(GuildUnionDataPlane, ClassifyCreateSynInvalidCharSendsNack) {
    GuildUnionRequest r;
    r.protocol = guild_union_create_syn;
    r.has_invalid_char = true;
    auto a = classify_guild_union_user(r);
    EXPECT_EQ(a.kind, GuildUnionActionKind::send_create_nack_to_user);
    EXPECT_EQ(a.protocol, guild_union_create_nack);
    EXPECT_EQ(a.error_code, guild_union_err_not_valid_name);
}

TEST(GuildUnionDataPlane, ClassifyCreateSynUnusableNameSendsNack) {
    GuildUnionRequest r;
    r.protocol = guild_union_create_syn;
    r.name_usable = false;
    auto a = classify_guild_union_user(r);
    EXPECT_EQ(a.kind, GuildUnionActionKind::send_create_nack_to_user);
}

TEST(GuildUnionDataPlane, ClassifyCreateSynBothBadNameSendsNack) {
    GuildUnionRequest r;
    r.protocol = guild_union_create_syn;
    r.name_usable = false;
    r.has_invalid_char = true;
    EXPECT_EQ(classify_guild_union_user(r).kind, GuildUnionActionKind::send_create_nack_to_user);
}

TEST(GuildUnionDataPlane, ClassifyCreateSynValidForwardsToMap) {
    GuildUnionRequest r;
    r.protocol = guild_union_create_syn;
    r.name_usable = true;
    r.has_invalid_char = false;
    auto a = classify_guild_union_user(r);
    EXPECT_EQ(a.kind, GuildUnionActionKind::forward_to_map);
    EXPECT_EQ(a.protocol, guild_union_create_syn);
    EXPECT_EQ(a.error_code, 0u);
}


// ===========================================================================
// classify_guild_union_user -- non-create path
// ===========================================================================

TEST(GuildUnionDataPlane, ClassifyNonCreateProtocolForwardsToMap) {
    GuildUnionRequest r;
    r.protocol = 99;
    auto a = classify_guild_union_user(r);
    EXPECT_EQ(a.kind, GuildUnionActionKind::forward_to_map);
    EXPECT_EQ(a.protocol, 99u);
}

TEST(GuildUnionDataPlane, ClassifyNonCreatePreservesProtocol) {
    for (std::uint8_t p : { 5u, 10u, 50u, 100u, 200u, 255u }) {
        GuildUnionRequest r;
        r.protocol = p;
        auto a = classify_guild_union_user(r);
        EXPECT_EQ(a.kind, GuildUnionActionKind::forward_to_map);
        EXPECT_EQ(a.protocol, p);
        EXPECT_EQ(a.error_code, 0u);
    }
}

TEST(GuildUnionDataPlane, ClassifyPreservesObjectId) {
    GuildUnionRequest r;
    r.protocol = guild_union_create_syn;
    r.object_id = 0xDEADBEEFu;
    auto a = classify_guild_union_user(r);
    EXPECT_EQ(a.object_id, 0xDEADBEEFu);
}


// ===========================================================================
// classify_guild_union_server -- broadcast group
// ===========================================================================

TEST(GuildUnionDataPlane, ServerClassifyCreateNotifyBroadcast) {
    GuildUnionServerRequest r;
    r.protocol = guild_union_create_notify_to_map;
    auto a = classify_guild_union_server(r);
    EXPECT_EQ(a.kind, GuildUnionServerActionKind::broadcast_to_other_maps);
    EXPECT_EQ(a.protocol, guild_union_create_notify_to_map);
}

TEST(GuildUnionDataPlane, ServerClassifyDestroyNotifyBroadcast) {
    GuildUnionServerRequest r;
    r.protocol = guild_union_destroy_notify_to_map;
    EXPECT_EQ(classify_guild_union_server(r).kind, GuildUnionServerActionKind::broadcast_to_other_maps);
}

TEST(GuildUnionDataPlane, ServerClassifyInviteAcceptNotifyBroadcast) {
    GuildUnionServerRequest r;
    r.protocol = guild_union_invite_accept_notify_to_map;
    EXPECT_EQ(classify_guild_union_server(r).kind, GuildUnionServerActionKind::broadcast_to_other_maps);
}

TEST(GuildUnionDataPlane, ServerClassifyAddNotifyBroadcast) {
    GuildUnionServerRequest r;
    r.protocol = guild_union_add_notify_to_map;
    EXPECT_EQ(classify_guild_union_server(r).kind, GuildUnionServerActionKind::broadcast_to_other_maps);
}

TEST(GuildUnionDataPlane, ServerClassifyRemoveNotifyBroadcast) {
    GuildUnionServerRequest r;
    r.protocol = guild_union_remove_notify_to_map;
    EXPECT_EQ(classify_guild_union_server(r).kind, GuildUnionServerActionKind::broadcast_to_other_maps);
}

TEST(GuildUnionDataPlane, ServerClassifySecedeNotifyBroadcast) {
    GuildUnionServerRequest r;
    r.protocol = guild_union_secede_notify_to_map;
    EXPECT_EQ(classify_guild_union_server(r).kind, GuildUnionServerActionKind::broadcast_to_other_maps);
}

TEST(GuildUnionDataPlane, ServerClassifyMarkRegistNotifyBroadcast) {
    GuildUnionServerRequest r;
    r.protocol = guild_union_mark_regist_notify_to_map;
    EXPECT_EQ(classify_guild_union_server(r).kind, GuildUnionServerActionKind::broadcast_to_other_maps);
}


// ===========================================================================
// classify_guild_union_server -- default
// ===========================================================================

TEST(GuildUnionDataPlane, ServerClassifyUnknownProtocolDrops) {
    GuildUnionServerRequest r;
    r.protocol = 99;
    auto a = classify_guild_union_server(r);
    EXPECT_EQ(a.kind, GuildUnionServerActionKind::drop_unknown);
    EXPECT_EQ(a.protocol, 99u);
}

TEST(GuildUnionDataPlane, ServerClassifyBelowNotifyRangeDrops) {
    // Below 20 (notify range) -> drop_unknown
    for (std::uint8_t p : { 0u, 1u, 5u, 10u, 19u }) {
        GuildUnionServerRequest r;
        r.protocol = p;
        EXPECT_EQ(classify_guild_union_server(r).kind, GuildUnionServerActionKind::drop_unknown);
    }
}

TEST(GuildUnionDataPlane, ServerClassifyAboveNotifyRangeDrops) {
    // Above 26 -> drop_unknown
    for (std::uint8_t p : { 27u, 30u, 100u, 200u, 255u }) {
        GuildUnionServerRequest r;
        r.protocol = p;
        EXPECT_EQ(classify_guild_union_server(r).kind, GuildUnionServerActionKind::drop_unknown);
    }
}

TEST(GuildUnionDataPlane, ServerClassifyPreservesProtocolOnBroadcast) {
    GuildUnionServerRequest r;
    r.protocol = guild_union_create_notify_to_map;
    r.object_id = 0xCAFEu;
    auto a = classify_guild_union_server(r);
    EXPECT_EQ(a.protocol, guild_union_create_notify_to_map);
}

TEST(GuildUnionDataPlane, ServerClassifyPreservesProtocolOnDrop) {
    GuildUnionServerRequest r;
    r.protocol = 200;
    auto a = classify_guild_union_server(r);
    EXPECT_EQ(a.protocol, 200u);
}
