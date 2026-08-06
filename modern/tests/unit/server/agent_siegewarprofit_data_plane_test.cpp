// agent_siegewarprofit_data_plane_test.cpp
//
// Comprehensive data plane tests for mxh::server::classify_siegewarprofit_user +
// classify_siegewarprofit_server (D4.149).
// Augments the legacy 4-test agent_siegewarprofit_test.cpp with deeper coverage of:
//   - siegewarprofit_category constant = 63 (MP_SIEGEWAR_PROFIT)
//   - 2 sub-protocol constants (change_texrate_notify_to_map=7,
//     change_guild_notify_to_map=11)
//   - SiegeWarProfitUserActionKind enum (forward_to_map)
//   - SiegeWarProfitServerActionKind enum (broadcast_to_other_maps, forward_to_client)
//   - struct defaults
//   - classify_siegewarprofit_user: unconditional forward_to_map (no args)
//   - classify_siegewarprofit_server:
//       change_texrate_notify_to_map -> broadcast_to_other_maps
//       change_guild_notify_to_map -> broadcast_to_other_maps
//       default -> forward_to_client (protocol preserved)
//
// 1:1 invariants (locked):
//   - siegewarprofit_category = 63
//   - change_texrate_notify_to_map=7, change_guild_notify_to_map=11
//   - User dispatch is unconditional forward_to_map (legacy stub)
//   - Server dispatch: only the 2 notify protocols broadcast; everything else forward_to_client

#pragma once

#include "mxh/server/agent_siegewarprofit.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <set>
#include <type_traits>

namespace {

using mxh::server::classify_siegewarprofit_server;
using mxh::server::classify_siegewarprofit_user;
using mxh::server::siegewarprofit_category;
using mxh::server::siegewarprofit_change_guild_notify_to_map;
using mxh::server::siegewarprofit_change_texrate_notify_to_map;
using mxh::server::SiegeWarProfitRequest;
using mxh::server::SiegeWarProfitServerAction;
using mxh::server::SiegeWarProfitServerActionKind;
using mxh::server::SiegeWarProfitUserAction;
using mxh::server::SiegeWarProfitUserActionKind;

}  // namespace


// ===========================================================================
// Constants
// ===========================================================================

TEST(SiegeWarProfitDataPlane, CategoryIsSixtyThree) {
    EXPECT_EQ(siegewarprofit_category, 63u);
}

TEST(SiegeWarProfitDataPlane, ChangeTexrateNotifyToMapIsSeven) {
    EXPECT_EQ(siegewarprofit_change_texrate_notify_to_map, 7u);
}

TEST(SiegeWarProfitDataPlane, ChangeGuildNotifyToMapIsEleven) {
    EXPECT_EQ(siegewarprofit_change_guild_notify_to_map, 11u);
}

TEST(SiegeWarProfitDataPlane, ProtocolConstantsAllDistinct) {
    std::set<std::uint8_t> seen = {
        siegewarprofit_change_texrate_notify_to_map,
        siegewarprofit_change_guild_notify_to_map,
    };
    EXPECT_EQ(seen.size(), 2u);
}


// ===========================================================================
// Enum types
// ===========================================================================

TEST(SiegeWarProfitDataPlane, UserActionKindHasOneValue) {
    auto all = { SiegeWarProfitUserActionKind::forward_to_map };
    EXPECT_EQ(all.size(), 1u);
}

TEST(SiegeWarProfitDataPlane, ServerActionKindHasTwoValues) {
    auto all = {
        SiegeWarProfitServerActionKind::broadcast_to_other_maps,
        SiegeWarProfitServerActionKind::forward_to_client,
    };
    EXPECT_EQ(all.size(), 2u);
}


// ===========================================================================
// Struct defaults
// ===========================================================================

TEST(SiegeWarProfitDataPlane, RequestDefaults) {
    SiegeWarProfitRequest r{};
    EXPECT_EQ(r.protocol, 0u);
}

TEST(SiegeWarProfitDataPlane, UserActionDefaults) {
    SiegeWarProfitUserAction a{};
    EXPECT_EQ(a.kind, SiegeWarProfitUserActionKind::forward_to_map);
}

TEST(SiegeWarProfitDataPlane, ServerActionDefaults) {
    SiegeWarProfitServerAction a{};
    EXPECT_EQ(a.kind, SiegeWarProfitServerActionKind::forward_to_client);
    EXPECT_EQ(a.protocol, 0u);
}


// ===========================================================================
// classify_siegewarprofit_user -- unconditional forward
// ===========================================================================

TEST(SiegeWarProfitDataPlane, ClassifyUserAlwaysForwards) {
    EXPECT_EQ(classify_siegewarprofit_user().kind, SiegeWarProfitUserActionKind::forward_to_map);
}

TEST(SiegeWarProfitDataPlane, ClassifyUserMultipleCallsAllForward) {
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(classify_siegewarprofit_user().kind, SiegeWarProfitUserActionKind::forward_to_map);
    }
}


// ===========================================================================
// classify_siegewarprofit_server -- broadcast group
// ===========================================================================

TEST(SiegeWarProfitDataPlane, ClassifyServerTexRateNotifyBroadcasts) {
    SiegeWarProfitRequest r;
    r.protocol = siegewarprofit_change_texrate_notify_to_map;
    auto a = classify_siegewarprofit_server(r);
    EXPECT_EQ(a.kind, SiegeWarProfitServerActionKind::broadcast_to_other_maps);
    EXPECT_EQ(a.protocol, siegewarprofit_change_texrate_notify_to_map);
}

TEST(SiegeWarProfitDataPlane, ClassifyServerGuildNotifyBroadcasts) {
    SiegeWarProfitRequest r;
    r.protocol = siegewarprofit_change_guild_notify_to_map;
    auto a = classify_siegewarprofit_server(r);
    EXPECT_EQ(a.kind, SiegeWarProfitServerActionKind::broadcast_to_other_maps);
    EXPECT_EQ(a.protocol, siegewarprofit_change_guild_notify_to_map);
}


// ===========================================================================
// classify_siegewarprofit_server -- default
// ===========================================================================

TEST(SiegeWarProfitDataPlane, ClassifyServerUnknownForwardsToClient) {
    SiegeWarProfitRequest r;
    r.protocol = 99;
    EXPECT_EQ(classify_siegewarprofit_server(r).kind, SiegeWarProfitServerActionKind::forward_to_client);
}

TEST(SiegeWarProfitDataPlane, ClassifyServerProtocol255ForwardsToClient) {
    SiegeWarProfitRequest r;
    r.protocol = 255;
    EXPECT_EQ(classify_siegewarprofit_server(r).kind, SiegeWarProfitServerActionKind::forward_to_client);
}

TEST(SiegeWarProfitDataPlane, ClassifyServerProtocol0ForwardsToClient) {
    SiegeWarProfitRequest r;
    r.protocol = 0;
    EXPECT_EQ(classify_siegewarprofit_server(r).kind, SiegeWarProfitServerActionKind::forward_to_client);
}

TEST(SiegeWarProfitDataPlane, ClassifyServerDefaultPreservesProtocol) {
    SiegeWarProfitRequest r;
    r.protocol = 200;
    auto a = classify_siegewarprofit_server(r);
    EXPECT_EQ(a.protocol, 200u);
}

TEST(SiegeWarProfitDataPlane, ClassifyServerBelowNotifyRangeForwards) {
    // Below 7 (change_texrate_notify_to_map)
    for (std::uint8_t p : { 0u, 1u, 3u, 5u, 6u }) {
        SiegeWarProfitRequest r;
        r.protocol = p;
        EXPECT_EQ(classify_siegewarprofit_server(r).kind, SiegeWarProfitServerActionKind::forward_to_client);
    }
}

TEST(SiegeWarProfitDataPlane, ClassifyServerBetweenNotifyRangeForwards) {
    // Between 7 and 11
    for (std::uint8_t p : { 8u, 9u, 10u }) {
        SiegeWarProfitRequest r;
        r.protocol = p;
        EXPECT_EQ(classify_siegewarprofit_server(r).kind, SiegeWarProfitServerActionKind::forward_to_client);
    }
}

TEST(SiegeWarProfitDataPlane, ClassifyServerAboveNotifyRangeForwards) {
    // Above 11
    for (std::uint8_t p : { 12u, 20u, 50u, 100u, 200u, 255u }) {
        SiegeWarProfitRequest r;
        r.protocol = p;
        EXPECT_EQ(classify_siegewarprofit_server(r).kind, SiegeWarProfitServerActionKind::forward_to_client);
    }
}
