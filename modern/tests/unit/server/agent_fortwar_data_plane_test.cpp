// agent_fortwar_data_plane_test.cpp
//
// Comprehensive data plane tests for mxh::server::classify_fortwar (D4.150).
// Augments the legacy 9-test agent_fortwar_test.cpp with deeper coverage of:
//   - fortwar_category constant = 76 (MP_FORTWAR)
//   - 9 sub-protocol constants (fortwar_info=0 .. fortwar_end_to_map=8)
//   - FortWarActionKind enum (broadcast_to_all_users, broadcast_to_other_maps,
//     forward_to_user_if_found, drop_no_user)
//   - FortWarRequest struct defaults
//   - FortWarAction struct defaults
//   - classify_fortwar truth table:
//       start_before10min, start, end -> broadcast_to_all_users
//       start_before10min_to_map, start_to_map, ing_to_map, end_to_map -> broadcast_to_other_maps
//       info, ing, default -> forward_to_user_if_found (user found) | drop_no_user (not found)
//
// 1:1 invariants (locked):
//   - fortwar_category = 76
//   - 9 protocol constants (0..8, all distinct)
//   - 3 broadcast_to_all_users protocols
//   - 4 broadcast_to_other_maps protocols
//   - info, ing: forward_to_user_if_found only when user_object_found=true
//   - All other protocols (default case): same forward/drop gate

#pragma once

#include "mxh/server/agent_fortwar.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <set>
#include <type_traits>

namespace {

using mxh::server::classify_fortwar;
using mxh::server::fortwar_category;
using mxh::server::fortwar_end;
using mxh::server::fortwar_end_to_map;
using mxh::server::fortwar_info;
using mxh::server::fortwar_ing;
using mxh::server::fortwar_ing_to_map;
using mxh::server::fortwar_start;
using mxh::server::fortwar_start_before10min;
using mxh::server::fortwar_start_before10min_to_map;
using mxh::server::fortwar_start_to_map;
using mxh::server::FortWarAction;
using mxh::server::FortWarActionKind;
using mxh::server::FortWarRequest;

}  // namespace


// ===========================================================================
// Constants
// ===========================================================================

TEST(FortWarDataPlane, CategoryIsSeventySix) {
    EXPECT_EQ(fortwar_category, 76u);
}

TEST(FortWarDataPlane, ProtocolInfoIsZero) { EXPECT_EQ(fortwar_info, 0u); }
TEST(FortWarDataPlane, ProtocolStartBefore10MinIsOne) { EXPECT_EQ(fortwar_start_before10min, 1u); }
TEST(FortWarDataPlane, ProtocolStartIsTwo) { EXPECT_EQ(fortwar_start, 2u); }
TEST(FortWarDataPlane, ProtocolIngIsThree) { EXPECT_EQ(fortwar_ing, 3u); }
TEST(FortWarDataPlane, ProtocolEndIsFour) { EXPECT_EQ(fortwar_end, 4u); }
TEST(FortWarDataPlane, ProtocolStartBefore10MinToMapIsFive) { EXPECT_EQ(fortwar_start_before10min_to_map, 5u); }
TEST(FortWarDataPlane, ProtocolStartToMapIsSix) { EXPECT_EQ(fortwar_start_to_map, 6u); }
TEST(FortWarDataPlane, ProtocolIngToMapIsSeven) { EXPECT_EQ(fortwar_ing_to_map, 7u); }
TEST(FortWarDataPlane, ProtocolEndToMapIsEight) { EXPECT_EQ(fortwar_end_to_map, 8u); }

TEST(FortWarDataPlane, ProtocolConstantsAllDistinct) {
    std::set<std::uint8_t> seen = {
        fortwar_info, fortwar_start_before10min, fortwar_start, fortwar_ing,
        fortwar_end, fortwar_start_before10min_to_map, fortwar_start_to_map,
        fortwar_ing_to_map, fortwar_end_to_map,
    };
    EXPECT_EQ(seen.size(), 9u);
}


// ===========================================================================
// Enum types
// ===========================================================================

TEST(FortWarDataPlane, ActionKindHasFourValues) {
    auto all = {
        FortWarActionKind::broadcast_to_all_users,
        FortWarActionKind::broadcast_to_other_maps,
        FortWarActionKind::forward_to_user_if_found,
        FortWarActionKind::drop_no_user,
    };
    EXPECT_EQ(all.size(), 4u);
}


// ===========================================================================
// Struct defaults
// ===========================================================================

TEST(FortWarDataPlane, RequestDefaults) {
    FortWarRequest r{};
    EXPECT_EQ(r.protocol, 0u);
    EXPECT_EQ(r.object_id, 0u);
    EXPECT_FALSE(r.user_object_found);
}

TEST(FortWarDataPlane, ActionDefaults) {
    FortWarAction a{};
    EXPECT_EQ(a.kind, FortWarActionKind::drop_no_user);
    EXPECT_EQ(a.protocol, 0u);
}


// ===========================================================================
// broadcast_to_all_users group (3 protocols)
// ===========================================================================

TEST(FortWarDataPlane, ClassifyStartBefore10MinBroadcastsToAllUsers) {
    FortWarRequest r;
    r.protocol = fortwar_start_before10min;
    auto a = classify_fortwar(r);
    EXPECT_EQ(a.kind, FortWarActionKind::broadcast_to_all_users);
    EXPECT_EQ(a.protocol, fortwar_start_before10min);
}

TEST(FortWarDataPlane, ClassifyStartBroadcastsToAllUsers) {
    FortWarRequest r;
    r.protocol = fortwar_start;
    EXPECT_EQ(classify_fortwar(r).kind, FortWarActionKind::broadcast_to_all_users);
}

TEST(FortWarDataPlane, ClassifyEndBroadcastsToAllUsers) {
    FortWarRequest r;
    r.protocol = fortwar_end;
    EXPECT_EQ(classify_fortwar(r).kind, FortWarActionKind::broadcast_to_all_users);
}

TEST(FortWarDataPlane, ClassifyAllUsersBroadcastIgnoresUserObjectFound) {
    // user_object_found has no effect on broadcast_to_all_users protocols.
    FortWarRequest r;
    r.protocol = fortwar_start;
    r.user_object_found = false;
    EXPECT_EQ(classify_fortwar(r).kind, FortWarActionKind::broadcast_to_all_users);
}


// ===========================================================================
// broadcast_to_other_maps group (4 protocols)
// ===========================================================================

TEST(FortWarDataPlane, ClassifyStartBefore10MinToMapBroadcastsToOtherMaps) {
    FortWarRequest r;
    r.protocol = fortwar_start_before10min_to_map;
    EXPECT_EQ(classify_fortwar(r).kind, FortWarActionKind::broadcast_to_other_maps);
}

TEST(FortWarDataPlane, ClassifyStartToMapBroadcastsToOtherMaps) {
    FortWarRequest r;
    r.protocol = fortwar_start_to_map;
    EXPECT_EQ(classify_fortwar(r).kind, FortWarActionKind::broadcast_to_other_maps);
}

TEST(FortWarDataPlane, ClassifyIngToMapBroadcastsToOtherMaps) {
    FortWarRequest r;
    r.protocol = fortwar_ing_to_map;
    EXPECT_EQ(classify_fortwar(r).kind, FortWarActionKind::broadcast_to_other_maps);
}

TEST(FortWarDataPlane, ClassifyEndToMapBroadcastsToOtherMaps) {
    FortWarRequest r;
    r.protocol = fortwar_end_to_map;
    EXPECT_EQ(classify_fortwar(r).kind, FortWarActionKind::broadcast_to_other_maps);
}

TEST(FortWarDataPlane, ClassifyMapsBroadcastIgnoresUserObjectFound) {
    FortWarRequest r;
    r.protocol = fortwar_end_to_map;
    r.user_object_found = false;
    EXPECT_EQ(classify_fortwar(r).kind, FortWarActionKind::broadcast_to_other_maps);
}


// ===========================================================================
// info / ing path: forward vs drop based on user_object_found
// ===========================================================================

TEST(FortWarDataPlane, ClassifyInfoUserFoundForwardsToUser) {
    FortWarRequest r;
    r.protocol = fortwar_info;
    r.user_object_found = true;
    EXPECT_EQ(classify_fortwar(r).kind, FortWarActionKind::forward_to_user_if_found);
}

TEST(FortWarDataPlane, ClassifyInfoUserMissingDrops) {
    FortWarRequest r;
    r.protocol = fortwar_info;
    r.user_object_found = false;
    EXPECT_EQ(classify_fortwar(r).kind, FortWarActionKind::drop_no_user);
}

TEST(FortWarDataPlane, ClassifyIngUserFoundForwardsToUser) {
    FortWarRequest r;
    r.protocol = fortwar_ing;
    r.user_object_found = true;
    EXPECT_EQ(classify_fortwar(r).kind, FortWarActionKind::forward_to_user_if_found);
}

TEST(FortWarDataPlane, ClassifyIngUserMissingDrops) {
    FortWarRequest r;
    r.protocol = fortwar_ing;
    r.user_object_found = false;
    EXPECT_EQ(classify_fortwar(r).kind, FortWarActionKind::drop_no_user);
}


// ===========================================================================
// Default path: same forward/drop gate
// ===========================================================================

TEST(FortWarDataPlane, ClassifyUnknownProtocolUserFoundForwards) {
    FortWarRequest r;
    r.protocol = 99;
    r.user_object_found = true;
    EXPECT_EQ(classify_fortwar(r).kind, FortWarActionKind::forward_to_user_if_found);
}

TEST(FortWarDataPlane, ClassifyUnknownProtocolUserMissingDrops) {
    FortWarRequest r;
    r.protocol = 99;
    r.user_object_found = false;
    EXPECT_EQ(classify_fortwar(r).kind, FortWarActionKind::drop_no_user);
}

TEST(FortWarDataPlane, ClassifyProtocol255UserFoundForwards) {
    FortWarRequest r;
    r.protocol = 255;
    r.user_object_found = true;
    EXPECT_EQ(classify_fortwar(r).kind, FortWarActionKind::forward_to_user_if_found);
}


// ===========================================================================
// Protocol preservation
// ===========================================================================

TEST(FortWarDataPlane, ClassifyDefaultPreservesProtocol) {
    FortWarRequest r;
    r.protocol = 99;
    auto a = classify_fortwar(r);
    EXPECT_EQ(a.protocol, 99u);
}

TEST(FortWarDataPlane, ClassifyInfoPreservesProtocol) {
    FortWarRequest r;
    r.protocol = fortwar_info;
    r.user_object_found = true;
    auto a = classify_fortwar(r);
    EXPECT_EQ(a.protocol, fortwar_info);
}

TEST(FortWarDataPlane, ClassifyIngPreservesProtocol) {
    FortWarRequest r;
    r.protocol = fortwar_ing;
    r.user_object_found = true;
    auto a = classify_fortwar(r);
    EXPECT_EQ(a.protocol, fortwar_ing);
}
