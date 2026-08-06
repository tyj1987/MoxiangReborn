
// agent_itemlimit_data_plane_test.cpp
//
// Comprehensive data plane tests for mxh::server::agent_itemlimit (D4.136).
// Augments the legacy 3-test agent_itemlimit_test.cpp with deeper coverage of:
//   - MP_ITEMLIMIT category byte = 74.
//   - 2 sub-protocols: itemlimit_addcount_to_map=0 + itemlimit_full_to_client=1.
//   - classify_itemlimit() dispatch:
//       addcount_to_map -> broadcast_to_other_maps
//       full_to_client -> forward_to_client
//       any other protocol -> forward_to_client (default).
//   - ItemLimitAction struct fields (kind + protocol preserved).
//   - ItemLimitRequest struct defaults.
//   - ItemLimitActionKind enum distinctness.
//
// 1:1 invariants (locked):
//   - itemlimit_category = 74 (MP_ITEMLIMIT).
//   - itemlimit_addcount_to_map = 0, itemlimit_full_to_client = 1.
//   - classify_itemlimit: 0 -> broadcast, 1 -> forward, any other -> forward.
//   - ItemLimitActionKind has 2 values: broadcast_to_other_maps, forward_to_client.

#pragma once

#include "mxh/server/agent_itemlimit.hpp"

#include <gtest/gtest.h>

#include <cstdint>

namespace {

using mxh::server::classify_itemlimit;
using mxh::server::itemlimit_addcount_to_map;
using mxh::server::itemlimit_category;
using mxh::server::itemlimit_full_to_client;
using mxh::server::ItemLimitAction;
using mxh::server::ItemLimitActionKind;
using mxh::server::ItemLimitRequest;

}  // namespace


// ===========================================================================
// Wire constants
// ===========================================================================

TEST(AgentItemlimitDataPlane, CategoryByteIsSeventyFour) {
    EXPECT_EQ(itemlimit_category, 74u);
}

TEST(AgentItemlimitDataPlane, AddCountToMapSubProtocolIsZero) {
    EXPECT_EQ(itemlimit_addcount_to_map, 0u);
}

TEST(AgentItemlimitDataPlane, FullToClientSubProtocolIsOne) {
    EXPECT_EQ(itemlimit_full_to_client, 1u);
}

TEST(AgentItemlimitDataPlane, SubProtocolsAreDistinct) {
    EXPECT_NE(itemlimit_addcount_to_map,
              itemlimit_full_to_client);
}


// ===========================================================================
// ItemLimitActionKind enum
// ===========================================================================

TEST(AgentItemlimitDataPlane, KindBroadcastIsZero) {
    EXPECT_EQ(static_cast<std::uint8_t>(ItemLimitActionKind::broadcast_to_other_maps), 0u);
}

TEST(AgentItemlimitDataPlane, KindForwardIsOne) {
    EXPECT_EQ(static_cast<std::uint8_t>(ItemLimitActionKind::forward_to_client), 1u);
}

TEST(AgentItemlimitDataPlane, KindValuesAreDistinct) {
    EXPECT_NE(ItemLimitActionKind::broadcast_to_other_maps,
              ItemLimitActionKind::forward_to_client);
}


// ===========================================================================
// ItemLimitRequest defaults
// ===========================================================================

TEST(AgentItemlimitDataPlane, RequestDefaultProtocolIsZero) {
    ItemLimitRequest r{};
    EXPECT_EQ(r.protocol, 0u);
}

TEST(AgentItemlimitDataPlane, RequestProtocolAssignable) {
    ItemLimitRequest r{};
    r.protocol = itemlimit_full_to_client;
    EXPECT_EQ(r.protocol, 1u);
}


// ===========================================================================
// classify_itemlimit - dispatch
// ===========================================================================

TEST(AgentItemlimitDataPlane, AddCountToMapBroadcastsToOtherMaps) {
    ItemLimitRequest r{};
    r.protocol = itemlimit_addcount_to_map;
    auto a = classify_itemlimit(r);
    EXPECT_EQ(a.kind, ItemLimitActionKind::broadcast_to_other_maps);
    EXPECT_EQ(a.protocol, itemlimit_addcount_to_map);
}

TEST(AgentItemlimitDataPlane, FullToClientForwardsToClient) {
    ItemLimitRequest r{};
    r.protocol = itemlimit_full_to_client;
    auto a = classify_itemlimit(r);
    EXPECT_EQ(a.kind, ItemLimitActionKind::forward_to_client);
    EXPECT_EQ(a.protocol, itemlimit_full_to_client);
}

TEST(AgentItemlimitDataPlane, UnknownProtocolForwardsToClient) {
    ItemLimitRequest r{};
    r.protocol = 99u;
    auto a = classify_itemlimit(r);
    EXPECT_EQ(a.kind, ItemLimitActionKind::forward_to_client);
    EXPECT_EQ(a.protocol, 99u);
}


// ===========================================================================
// Boundary protocols
// ===========================================================================

TEST(AgentItemlimitDataPlane, ProtocolTwoForwardsToClient) {
    ItemLimitRequest r{};
    r.protocol = 2u;
    auto a = classify_itemlimit(r);
    EXPECT_EQ(a.kind, ItemLimitActionKind::forward_to_client);
    EXPECT_EQ(a.protocol, 2u);
}

TEST(AgentItemlimitDataPlane, Protocol255ForwardsToClient) {
    ItemLimitRequest r{};
    r.protocol = 0xFFu;
    auto a = classify_itemlimit(r);
    EXPECT_EQ(a.kind, ItemLimitActionKind::forward_to_client);
    EXPECT_EQ(a.protocol, 0xFFu);
}

TEST(AgentItemlimitDataPlane, ProtocolZeroBroadcasts) {
    // protocol=0 == itemlimit_addcount_to_map, so it broadcasts.
    ItemLimitRequest r{};
    r.protocol = 0u;
    auto a = classify_itemlimit(r);
    EXPECT_EQ(a.kind, ItemLimitActionKind::broadcast_to_other_maps);
}

TEST(AgentItemlimitDataPlane, ProtocolOneForwards) {
    // protocol=1 == itemlimit_full_to_client, so it forwards.
    ItemLimitRequest r{};
    r.protocol = 1u;
    auto a = classify_itemlimit(r);
    EXPECT_EQ(a.kind, ItemLimitActionKind::forward_to_client);
}


// ===========================================================================
// Action struct preserves input protocol
// ===========================================================================

TEST(AgentItemlimitDataPlane, ActionPreservesInputProtocolForBroadcast) {
    ItemLimitRequest r{};
    r.protocol = 0u;
    auto a = classify_itemlimit(r);
    EXPECT_EQ(a.protocol, 0u);
    EXPECT_EQ(a.kind, ItemLimitActionKind::broadcast_to_other_maps);
}

TEST(AgentItemlimitDataPlane, ActionPreservesInputProtocolForForward) {
    ItemLimitRequest r{};
    r.protocol = 1u;
    auto a = classify_itemlimit(r);
    EXPECT_EQ(a.protocol, 1u);
    EXPECT_EQ(a.kind, ItemLimitActionKind::forward_to_client);
}

TEST(AgentItemlimitDataPlane, ActionPreservesUnknownProtocolForForward) {
    ItemLimitRequest r{};
    r.protocol = 42u;
    auto a = classify_itemlimit(r);
    EXPECT_EQ(a.protocol, 42u);
    EXPECT_EQ(a.kind, ItemLimitActionKind::forward_to_client);
}


// ===========================================================================
// Sequential calls produce independent actions
// ===========================================================================

TEST(AgentItemlimitDataPlane, SequentialCallsAreIndependent) {
    ItemLimitRequest r1{};
    r1.protocol = 0u;
    auto a1 = classify_itemlimit(r1);
    EXPECT_EQ(a1.kind, ItemLimitActionKind::broadcast_to_other_maps);

    ItemLimitRequest r2{};
    r2.protocol = 1u;
    auto a2 = classify_itemlimit(r2);
    EXPECT_EQ(a2.kind, ItemLimitActionKind::forward_to_client);

    EXPECT_EQ(a1.kind, ItemLimitActionKind::broadcast_to_other_maps);  // unaffected
}

TEST(AgentItemlimitDataPlane, AllNonZeroProtocolsForward) {
    for (std::uint16_t p = 1; p <= 255; ++p) {
        ItemLimitRequest r{};
        r.protocol = static_cast<std::uint8_t>(p);
        auto a = classify_itemlimit(r);
        EXPECT_EQ(a.kind, ItemLimitActionKind::forward_to_client);
        EXPECT_EQ(a.protocol, static_cast<std::uint8_t>(p));
    }
}

TEST(AgentItemlimitDataPlane, OnlyZeroProtocolBroadcasts) {
    for (std::uint16_t p = 0; p <= 0; ++p) {
        ItemLimitRequest r{};
        r.protocol = static_cast<std::uint8_t>(p);
        auto a = classify_itemlimit(r);
        EXPECT_EQ(a.kind, ItemLimitActionKind::broadcast_to_other_maps);
    }
}

TEST(AgentItemlimitDataPlane, RequestProtocolFieldType) {
    static_assert(std::is_same<decltype(ItemLimitRequest{}.protocol),
                               std::uint8_t>::value,
                  "ItemLimitRequest.protocol must be uint8_t");
    EXPECT_TRUE(true);
}
