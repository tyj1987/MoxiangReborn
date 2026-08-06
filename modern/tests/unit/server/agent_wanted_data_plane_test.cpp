// agent_wanted_data_plane_test.cpp
//
// Comprehensive data plane tests for mxh::server::classify_wanted (D4.152).
// Augments the legacy 7-test agent_wanted_test.cpp with deeper coverage of:
//   - wanted_category constant = 51 (MP_WANTED)
//   - 7 sub-protocol constants
//   - WantedServerActionKind enum (broadcast_to_other_maps,
//     complete_notcomplete_send_to_map, default_forward_to_client, drop_no_user)
//   - struct defaults
//   - classify_wanted truth table:
//       notify_delete_to_map, notify_regist_to_map, notify_notcomplete_to_map,
//       destroyed_to_map -> broadcast_to_other_maps (4 protocols)
//       notcomplete_to_agent + !user_found -> drop_no_user
//       notcomplete_to_agent + user_found -> complete_notcomplete_send_to_map with
//         converted protocol (notcomplete_by_delchr) + target_map_connection_index
//       default -> default_forward_to_client (protocol preserved)
//
// 1:1 invariants (locked):
//   - wanted_category = 51
//   - 4 broadcast protocols: notify_delete=9, notify_regist=8,
//     notify_notcomplete=18, destroyed=28
//   - notcomplete_to_agent=23 + user_found -> notcomplete_by_delchr=24

#pragma once

#include "mxh/server/agent_wanted.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <set>
#include <type_traits>

namespace {

using mxh::server::classify_wanted;
using mxh::server::wanted_category;
using mxh::server::wanted_destroyed_to_map;
using mxh::server::wanted_notcomplete_by_delchr;
using mxh::server::wanted_notcomplete_to_agent;
using mxh::server::wanted_notify_delete_to_map;
using mxh::server::wanted_notify_notcomplete_to_map;
using mxh::server::wanted_notify_regist_to_map;
using mxh::server::WantedAction;
using mxh::server::WantedRequest;
using mxh::server::WantedServerActionKind;

}  // namespace


// ===========================================================================
// Constants
// ===========================================================================

TEST(WantedDataPlane, CategoryIsFiftyOne) {
    EXPECT_EQ(wanted_category, 51u);
}

TEST(WantedDataPlane, ProtocolNotifyRegistToMapIsEight) {
    EXPECT_EQ(wanted_notify_regist_to_map, 8u);
}

TEST(WantedDataPlane, ProtocolNotifyDeleteToMapIsNine) {
    EXPECT_EQ(wanted_notify_delete_to_map, 9u);
}

TEST(WantedDataPlane, ProtocolNotifyNotcompleteToMapIsEighteen) {
    EXPECT_EQ(wanted_notify_notcomplete_to_map, 18u);
}

TEST(WantedDataPlane, ProtocolNotcompleteToAgentIsTwentyThree) {
    EXPECT_EQ(wanted_notcomplete_to_agent, 23u);
}

TEST(WantedDataPlane, ProtocolNotcompleteByDelchrIsTwentyFour) {
    EXPECT_EQ(wanted_notcomplete_by_delchr, 24u);
}

TEST(WantedDataPlane, ProtocolDestroyedToMapIsTwentyEight) {
    EXPECT_EQ(wanted_destroyed_to_map, 28u);
}

TEST(WantedDataPlane, ProtocolConstantsAllDistinct) {
    std::set<std::uint8_t> seen = {
        wanted_notify_regist_to_map, wanted_notify_delete_to_map,
        wanted_notify_notcomplete_to_map, wanted_notcomplete_to_agent,
        wanted_notcomplete_by_delchr, wanted_destroyed_to_map,
    };
    EXPECT_EQ(seen.size(), 6u);
}


// ===========================================================================
// Enum types
// ===========================================================================

TEST(WantedDataPlane, ActionKindHasFourValues) {
    auto all = {
        WantedServerActionKind::broadcast_to_other_maps,
        WantedServerActionKind::complete_notcomplete_send_to_map,
        WantedServerActionKind::default_forward_to_client,
        WantedServerActionKind::drop_no_user,
    };
    EXPECT_EQ(all.size(), 4u);
}


// ===========================================================================
// Struct defaults
// ===========================================================================

TEST(WantedDataPlane, RequestDefaults) {
    WantedRequest r{};
    EXPECT_EQ(r.protocol, 0u);
    EXPECT_EQ(r.object_id, 0u);
    EXPECT_TRUE(r.user_found);
    EXPECT_EQ(r.target_map_connection_index, 0u);
}

TEST(WantedDataPlane, ActionDefaults) {
    WantedAction a{};
    EXPECT_EQ(a.kind, WantedServerActionKind::default_forward_to_client);
    EXPECT_EQ(a.protocol, 0u);
    EXPECT_EQ(a.object_id, 0u);
    EXPECT_EQ(a.target_map_connection_index, 0u);
}


// ===========================================================================
// broadcast_to_other_maps group (4 protocols)
// ===========================================================================

TEST(WantedDataPlane, ClassifyNotifyDeleteToMapBroadcasts) {
    WantedRequest r;
    r.protocol = wanted_notify_delete_to_map;
    EXPECT_EQ(classify_wanted(r).kind, WantedServerActionKind::broadcast_to_other_maps);
}

TEST(WantedDataPlane, ClassifyNotifyRegistToMapBroadcasts) {
    WantedRequest r;
    r.protocol = wanted_notify_regist_to_map;
    EXPECT_EQ(classify_wanted(r).kind, WantedServerActionKind::broadcast_to_other_maps);
}

TEST(WantedDataPlane, ClassifyNotifyNotcompleteToMapBroadcasts) {
    WantedRequest r;
    r.protocol = wanted_notify_notcomplete_to_map;
    EXPECT_EQ(classify_wanted(r).kind, WantedServerActionKind::broadcast_to_other_maps);
}

TEST(WantedDataPlane, ClassifyDestroyedToMapBroadcasts) {
    WantedRequest r;
    r.protocol = wanted_destroyed_to_map;
    EXPECT_EQ(classify_wanted(r).kind, WantedServerActionKind::broadcast_to_other_maps);
}

TEST(WantedDataPlane, ClassifyBroadcastPreservesProtocol) {
    WantedRequest r;
    r.protocol = wanted_notify_regist_to_map;
    auto a = classify_wanted(r);
    EXPECT_EQ(a.protocol, wanted_notify_regist_to_map);
}

TEST(WantedDataPlane, ClassifyBroadcastIgnoresUserFound) {
    WantedRequest r;
    r.protocol = wanted_notify_delete_to_map;
    r.user_found = false;
    EXPECT_EQ(classify_wanted(r).kind, WantedServerActionKind::broadcast_to_other_maps);
}


// ===========================================================================
// notcomplete_to_agent path
// ===========================================================================

TEST(WantedDataPlane, ClassifyNotcompleteToAgentUserMissingDrops) {
    WantedRequest r;
    r.protocol = wanted_notcomplete_to_agent;
    r.user_found = false;
    EXPECT_EQ(classify_wanted(r).kind, WantedServerActionKind::drop_no_user);
}

TEST(WantedDataPlane, ClassifyNotcompleteToAgentUserMissingPreservesProtocol) {
    WantedRequest r;
    r.protocol = wanted_notcomplete_to_agent;
    r.user_found = false;
    auto a = classify_wanted(r);
    EXPECT_EQ(a.protocol, wanted_notcomplete_to_agent);
}

TEST(WantedDataPlane, ClassifyNotcompleteToAgentUserMissingPreservesObjectId) {
    WantedRequest r;
    r.protocol = wanted_notcomplete_to_agent;
    r.user_found = false;
    r.object_id = 0xDEADBEEFu;
    auto a = classify_wanted(r);
    EXPECT_EQ(a.object_id, 0xDEADBEEFu);
}

TEST(WantedDataPlane, ClassifyNotcompleteToAgentUserFoundCompletes) {
    WantedRequest r;
    r.protocol = wanted_notcomplete_to_agent;
    r.user_found = true;
    auto a = classify_wanted(r);
    EXPECT_EQ(a.kind, WantedServerActionKind::complete_notcomplete_send_to_map);
    EXPECT_EQ(a.protocol, wanted_notcomplete_by_delchr);
}

TEST(WantedDataPlane, ClassifyNotcompleteToAgentUserFoundPreservesObjectId) {
    WantedRequest r;
    r.protocol = wanted_notcomplete_to_agent;
    r.user_found = true;
    r.object_id = 0xCAFEBABEu;
    auto a = classify_wanted(r);
    EXPECT_EQ(a.object_id, 0xCAFEBABEu);
}

TEST(WantedDataPlane, ClassifyNotcompleteToAgentUserFoundPreservesTargetMapConnIndex) {
    WantedRequest r;
    r.protocol = wanted_notcomplete_to_agent;
    r.user_found = true;
    r.target_map_connection_index = 7;
    auto a = classify_wanted(r);
    EXPECT_EQ(a.target_map_connection_index, 7);
}

TEST(WantedDataPlane, ClassifyNotcompleteToAgentMaxTargetMapConnIndex) {
    WantedRequest r;
    r.protocol = wanted_notcomplete_to_agent;
    r.user_found = true;
    r.target_map_connection_index = 255;
    auto a = classify_wanted(r);
    EXPECT_EQ(a.target_map_connection_index, 255);
}


// ===========================================================================
// Default path
// ===========================================================================

TEST(WantedDataPlane, ClassifyUnknownForwardsToClient) {
    WantedRequest r;
    r.protocol = 99;
    EXPECT_EQ(classify_wanted(r).kind, WantedServerActionKind::default_forward_to_client);
}

TEST(WantedDataPlane, ClassifyProtocol255ForwardsToClient) {
    WantedRequest r;
    r.protocol = 255;
    EXPECT_EQ(classify_wanted(r).kind, WantedServerActionKind::default_forward_to_client);
}

TEST(WantedDataPlane, ClassifyProtocol0ForwardsToClient) {
    WantedRequest r;
    r.protocol = 0;
    EXPECT_EQ(classify_wanted(r).kind, WantedServerActionKind::default_forward_to_client);
}

TEST(WantedDataPlane, ClassifyDefaultPreservesProtocol) {
    WantedRequest r;
    r.protocol = 200;
    auto a = classify_wanted(r);
    EXPECT_EQ(a.protocol, 200u);
}

TEST(WantedDataPlane, ClassifyBelowBroadcastRangeForwardsToClient) {
    // Below notify_regist=8 / notify_delete=9
    for (std::uint8_t p : { 0u, 1u, 3u, 5u, 7u }) {
        WantedRequest r;
        r.protocol = p;
        EXPECT_EQ(classify_wanted(r).kind, WantedServerActionKind::default_forward_to_client);
    }
}

TEST(WantedDataPlane, ClassifyBetweenBroadcastAndNotcompleteForwardsToClient) {
    // Between 9 and 18, 23, 28
    for (std::uint8_t p : { 10u, 12u, 15u, 17u, 19u, 22u, 25u, 27u }) {
        WantedRequest r;
        r.protocol = p;
        EXPECT_EQ(classify_wanted(r).kind, WantedServerActionKind::default_forward_to_client);
    }
}
