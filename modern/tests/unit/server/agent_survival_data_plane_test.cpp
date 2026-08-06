// agent_survival_data_plane_test.cpp
//
// Comprehensive data plane tests for mxh::server::classify_survival_user +
// classify_survival_server (D4.151).
// Augments the legacy 10-test agent_survival_test.cpp with deeper coverage of:
//   - survival_category constant = 70 (MP_SURVIVAL)
//   - 8 sub-protocol constants (info=0/aliveuser_count=1/returntomap=2/leave_syn=3/
//     ready_syn=7/stop_syn=16/mapoff_syn=19/itemusingcount_set=24)
//   - 3-value SurvivalUserActionKind enum (send_leave_syn_to_map,
//     gm_protected_forward_to_map, default_forward_to_map)
//   - 2-value SurvivalServerActionKind enum (update_user_map_and_forward_to_client,
//     default_forward_to_client)
//   - struct defaults
//   - classify_survival_user truth table:
//       !user_found -> default_forward_to_map (any protocol)
//       leave_syn + user_found -> send_leave_syn_to_map (with extra fields)
//       ready_syn/stop_syn/mapoff_syn/itemusingcount_set + user_found -> gm_protected_forward_to_map
//       default -> default_forward_to_map
//   - classify_survival_server truth table:
//       returntomap + user_found + target_map_port_found -> update_user_map_and_forward_to_client
//       returntomap (no both flags) -> default_forward_to_client
//       default -> default_forward_to_client

#pragma once

#include "mxh/server/agent_survival.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <set>
#include <type_traits>

namespace {

using mxh::server::classify_survival_server;
using mxh::server::classify_survival_user;
using mxh::server::survival_aliveuser_count;
using mxh::server::survival_category;
using mxh::server::survival_info;
using mxh::server::survival_itemusingcount_set;
using mxh::server::survival_leave_syn;
using mxh::server::survival_mapoff_syn;
using mxh::server::survival_ready_syn;
using mxh::server::survival_returntomap;
using mxh::server::survival_stop_syn;
using mxh::server::SurvivalServerAction;
using mxh::server::SurvivalServerActionKind;
using mxh::server::SurvivalServerRequest;
using mxh::server::SurvivalUserAction;
using mxh::server::SurvivalUserActionKind;
using mxh::server::SurvivalUserRequest;

}  // namespace


// ===========================================================================
// Constants
// ===========================================================================

TEST(SurvivalDataPlane, CategoryIsSeventy) {
    EXPECT_EQ(survival_category, 70u);
}

TEST(SurvivalDataPlane, ProtocolInfoIsZero) { EXPECT_EQ(survival_info, 0u); }
TEST(SurvivalDataPlane, ProtocolAliveuserCountIsOne) { EXPECT_EQ(survival_aliveuser_count, 1u); }
TEST(SurvivalDataPlane, ProtocolReturntomapIsTwo) { EXPECT_EQ(survival_returntomap, 2u); }
TEST(SurvivalDataPlane, ProtocolLeaveSynIsThree) { EXPECT_EQ(survival_leave_syn, 3u); }
TEST(SurvivalDataPlane, ProtocolReadySynIsSeven) { EXPECT_EQ(survival_ready_syn, 7u); }
TEST(SurvivalDataPlane, ProtocolStopSynIsSixteen) { EXPECT_EQ(survival_stop_syn, 16u); }
TEST(SurvivalDataPlane, ProtocolMapoffSynIsNineteen) { EXPECT_EQ(survival_mapoff_syn, 19u); }
TEST(SurvivalDataPlane, ProtocolItemusingcountSetIsTwentyFour) { EXPECT_EQ(survival_itemusingcount_set, 24u); }

TEST(SurvivalDataPlane, ProtocolConstantsAllDistinct) {
    std::set<std::uint8_t> seen = {
        survival_info, survival_aliveuser_count, survival_returntomap, survival_leave_syn,
        survival_ready_syn, survival_stop_syn, survival_mapoff_syn, survival_itemusingcount_set,
    };
    EXPECT_EQ(seen.size(), 8u);
}


// ===========================================================================
// Enum types
// ===========================================================================

TEST(SurvivalDataPlane, UserActionKindHasThreeValues) {
    auto all = {
        SurvivalUserActionKind::send_leave_syn_to_map,
        SurvivalUserActionKind::gm_protected_forward_to_map,
        SurvivalUserActionKind::default_forward_to_map,
    };
    EXPECT_EQ(all.size(), 3u);
}

TEST(SurvivalDataPlane, ServerActionKindHasTwoValues) {
    auto all = {
        SurvivalServerActionKind::update_user_map_and_forward_to_client,
        SurvivalServerActionKind::default_forward_to_client,
    };
    EXPECT_EQ(all.size(), 2u);
}


// ===========================================================================
// Struct defaults
// ===========================================================================

TEST(SurvivalDataPlane, UserRequestDefaults) {
    SurvivalUserRequest r{};
    EXPECT_EQ(r.protocol, 0u);
    EXPECT_EQ(r.object_id, 0u);
    EXPECT_EQ(r.unique_connect_idx, 0u);
    EXPECT_EQ(r.user_level, 0u);
    EXPECT_EQ(r.channel, 0u);
    EXPECT_TRUE(r.user_found);
}

TEST(SurvivalDataPlane, UserActionDefaults) {
    SurvivalUserAction a{};
    EXPECT_EQ(a.kind, SurvivalUserActionKind::default_forward_to_map);
    EXPECT_EQ(a.protocol, 0u);
    EXPECT_EQ(a.object_id, 0u);
    EXPECT_EQ(a.unique_connect_idx, 0u);
    EXPECT_EQ(a.user_level, 0u);
    EXPECT_EQ(a.channel, 0u);
}

TEST(SurvivalDataPlane, ServerRequestDefaults) {
    SurvivalServerRequest r{};
    EXPECT_EQ(r.protocol, 0u);
    EXPECT_EQ(r.object_id, 0u);
    EXPECT_EQ(r.target_map, 0u);
    EXPECT_FALSE(r.target_map_port_found);
    EXPECT_TRUE(r.user_found);
}

TEST(SurvivalDataPlane, ServerActionDefaults) {
    SurvivalServerAction a{};
    EXPECT_EQ(a.kind, SurvivalServerActionKind::default_forward_to_client);
    EXPECT_EQ(a.protocol, 0u);
    EXPECT_EQ(a.object_id, 0u);
    EXPECT_EQ(a.target_map, 0u);
    EXPECT_FALSE(a.update_user_state);
}


// ===========================================================================
// classify_survival_user -- !user_found wins
// ===========================================================================

TEST(SurvivalDataPlane, ClassifyUserNotFoundDefaultsForward) {
    SurvivalUserRequest r;
    r.protocol = survival_leave_syn;
    r.user_found = false;
    EXPECT_EQ(classify_survival_user(r).kind, SurvivalUserActionKind::default_forward_to_map);
}

TEST(SurvivalDataPlane, ClassifyUserNotFoundOverridesLeaveSyn) {
    SurvivalUserRequest r;
    r.protocol = survival_leave_syn;
    r.user_found = false;
    r.unique_connect_idx = 11;
    auto a = classify_survival_user(r);
    EXPECT_EQ(a.kind, SurvivalUserActionKind::default_forward_to_map);
    // unique_connect_idx is reset to 0 by the !user_found branch.
    EXPECT_EQ(a.unique_connect_idx, 0u);
}

TEST(SurvivalDataPlane, ClassifyUserNotFoundPreservesProtocol) {
    SurvivalUserRequest r;
    r.protocol = survival_leave_syn;
    r.user_found = false;
    auto a = classify_survival_user(r);
    EXPECT_EQ(a.protocol, survival_leave_syn);
}


// ===========================================================================
// classify_survival_user -- leave_syn path
// ===========================================================================

TEST(SurvivalDataPlane, ClassifyLeaveSynSendsAnnotatedMessageToMap) {
    SurvivalUserRequest r;
    r.protocol = survival_leave_syn;
    r.user_found = true;
    r.unique_connect_idx = 11;
    r.user_level = 1;
    r.channel = 2;
    auto a = classify_survival_user(r);
    EXPECT_EQ(a.kind, SurvivalUserActionKind::send_leave_syn_to_map);
    EXPECT_EQ(a.protocol, survival_leave_syn);
    EXPECT_EQ(a.unique_connect_idx, 11u);
    EXPECT_EQ(a.user_level, 1);
    EXPECT_EQ(a.channel, 2);
}

TEST(SurvivalDataPlane, ClassifyLeaveSynPreservesObjectId) {
    SurvivalUserRequest r;
    r.protocol = survival_leave_syn;
    r.user_found = true;
    r.object_id = 0xDEADBEEFu;
    auto a = classify_survival_user(r);
    EXPECT_EQ(a.object_id, 0xDEADBEEFu);
}

TEST(SurvivalDataPlane, ClassifyLeaveSynMaxBoundaryFields) {
    SurvivalUserRequest r;
    r.protocol = survival_leave_syn;
    r.user_found = true;
    r.unique_connect_idx = 0xFFFFFFFFu;
    r.user_level = 255;
    r.channel = 255;
    auto a = classify_survival_user(r);
    EXPECT_EQ(a.unique_connect_idx, 0xFFFFFFFFu);
    EXPECT_EQ(a.user_level, 255);
    EXPECT_EQ(a.channel, 255);
}


// ===========================================================================
// classify_survival_user -- gm_protected group
// ===========================================================================

TEST(SurvivalDataPlane, ClassifyReadySynGmProtectedForward) {
    SurvivalUserRequest r;
    r.protocol = survival_ready_syn;
    r.user_found = true;
    EXPECT_EQ(classify_survival_user(r).kind, SurvivalUserActionKind::gm_protected_forward_to_map);
}

TEST(SurvivalDataPlane, ClassifyStopSynGmProtectedForward) {
    SurvivalUserRequest r;
    r.protocol = survival_stop_syn;
    EXPECT_EQ(classify_survival_user(r).kind, SurvivalUserActionKind::gm_protected_forward_to_map);
}

TEST(SurvivalDataPlane, ClassifyMapoffSynGmProtectedForward) {
    SurvivalUserRequest r;
    r.protocol = survival_mapoff_syn;
    EXPECT_EQ(classify_survival_user(r).kind, SurvivalUserActionKind::gm_protected_forward_to_map);
}

TEST(SurvivalDataPlane, ClassifyItemusingcountSetGmProtectedForward) {
    SurvivalUserRequest r;
    r.protocol = survival_itemusingcount_set;
    EXPECT_EQ(classify_survival_user(r).kind, SurvivalUserActionKind::gm_protected_forward_to_map);
}

TEST(SurvivalDataPlane, ClassifyGmProtectedResetsExtraFields) {
    SurvivalUserRequest r;
    r.protocol = survival_ready_syn;
    r.user_found = true;
    r.unique_connect_idx = 99;
    r.user_level = 7;
    r.channel = 5;
    auto a = classify_survival_user(r);
    EXPECT_EQ(a.unique_connect_idx, 0u);
    EXPECT_EQ(a.user_level, 0);
    EXPECT_EQ(a.channel, 0);
}


// ===========================================================================
// classify_survival_user -- default path
// ===========================================================================

TEST(SurvivalDataPlane, ClassifyInfoDefaultsForward) {
    SurvivalUserRequest r;
    r.protocol = survival_info;
    EXPECT_EQ(classify_survival_user(r).kind, SurvivalUserActionKind::default_forward_to_map);
}

TEST(SurvivalDataPlane, ClassifyAliveuserCountDefaultsForward) {
    SurvivalUserRequest r;
    r.protocol = survival_aliveuser_count;
    EXPECT_EQ(classify_survival_user(r).kind, SurvivalUserActionKind::default_forward_to_map);
}

TEST(SurvivalDataPlane, ClassifyUnknownProtocolDefaultsForward) {
    SurvivalUserRequest r;
    r.protocol = 99;
    EXPECT_EQ(classify_survival_user(r).kind, SurvivalUserActionKind::default_forward_to_map);
}


// ===========================================================================
// classify_survival_server -- returntomap path
// ===========================================================================

TEST(SurvivalDataPlane, ServerReturnToMapWithPortUpdatesUserState) {
    SurvivalServerRequest r;
    r.protocol = survival_returntomap;
    r.user_found = true;
    r.target_map = 12;
    r.target_map_port_found = true;
    auto a = classify_survival_server(r);
    EXPECT_EQ(a.kind, SurvivalServerActionKind::update_user_map_and_forward_to_client);
    EXPECT_EQ(a.target_map, 12u);
    EXPECT_TRUE(a.update_user_state);
}

TEST(SurvivalDataPlane, ServerReturnToMapWithoutUserFoundDefaultsForward) {
    SurvivalServerRequest r;
    r.protocol = survival_returntomap;
    r.user_found = false;
    r.target_map_port_found = true;
    auto a = classify_survival_server(r);
    EXPECT_EQ(a.kind, SurvivalServerActionKind::default_forward_to_client);
    EXPECT_FALSE(a.update_user_state);
}

TEST(SurvivalDataPlane, ServerReturnToMapWithoutPortFoundDefaultsForward) {
    SurvivalServerRequest r;
    r.protocol = survival_returntomap;
    r.user_found = true;
    r.target_map_port_found = false;
    auto a = classify_survival_server(r);
    EXPECT_EQ(a.kind, SurvivalServerActionKind::default_forward_to_client);
    EXPECT_FALSE(a.update_user_state);
}

TEST(SurvivalDataPlane, ServerReturnToMapWithoutBothDefaultsForward) {
    SurvivalServerRequest r;
    r.protocol = survival_returntomap;
    r.user_found = false;
    r.target_map_port_found = false;
    auto a = classify_survival_server(r);
    EXPECT_EQ(a.kind, SurvivalServerActionKind::default_forward_to_client);
    EXPECT_FALSE(a.update_user_state);
}

TEST(SurvivalDataPlane, ServerReturnToMapWithPortZeroTargetMap) {
    SurvivalServerRequest r;
    r.protocol = survival_returntomap;
    r.user_found = true;
    r.target_map = 0;
    r.target_map_port_found = true;
    auto a = classify_survival_server(r);
    EXPECT_EQ(a.kind, SurvivalServerActionKind::update_user_map_and_forward_to_client);
    EXPECT_EQ(a.target_map, 0u);
    EXPECT_TRUE(a.update_user_state);
}

TEST(SurvivalDataPlane, ServerReturnToMapWithPortMaxTargetMap) {
    SurvivalServerRequest r;
    r.protocol = survival_returntomap;
    r.user_found = true;
    r.target_map = 0xFFFFFFFFu;
    r.target_map_port_found = true;
    auto a = classify_survival_server(r);
    EXPECT_EQ(a.target_map, 0xFFFFFFFFu);
}


// ===========================================================================
// classify_survival_server -- default
// ===========================================================================

TEST(SurvivalDataPlane, ServerInfoDefaultsForwardToClient) {
    SurvivalServerRequest r;
    r.protocol = survival_info;
    EXPECT_EQ(classify_survival_server(r).kind, SurvivalServerActionKind::default_forward_to_client);
}

TEST(SurvivalDataPlane, ServerUnknownProtocolDefaultsForwardToClient) {
    SurvivalServerRequest r;
    r.protocol = 99;
    EXPECT_EQ(classify_survival_server(r).kind, SurvivalServerActionKind::default_forward_to_client);
}

TEST(SurvivalDataPlane, ServerDefaultPreservesProtocol) {
    SurvivalServerRequest r;
    r.protocol = 99;
    auto a = classify_survival_server(r);
    EXPECT_EQ(a.protocol, 99u);
}
