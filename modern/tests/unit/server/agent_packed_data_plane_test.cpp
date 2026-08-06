// agent_packed_data_plane_test.cpp
//
// Comprehensive data plane tests for mxh::server::agent_packed (D4.128).
// Augments the legacy 5-test agent_packed_test.cpp with deeper coverage of
// the 3 sub-protocols (packed_normal / packed_to_mapserver /
// packed_to_broad_mapserver), the 4 PackedActionKind transitions, edge
// cases for receiver_count / data_size propagation, and wire constants
// (packed_category=13, packed_normal=0, packed_to_mapserver=1,
// packed_to_broad_mapserver=2).
//
// 1:1 invariants (locked):
//   - packed_category = 13 (MP_PACKEDDATA).
//   - packed_normal = 0, packed_to_mapserver = 1, packed_to_broad_mapserver = 2.
//   - Normal: fans out ONLY to receivers_present (legacy filters by
//     user existence, not just receiver_count).
//   - ToMapServer: forwards only when target_map_port_found == true;
//     else returns unknown.
//   - ToBroadMapServer: always broadcasts (no port gate).

#pragma once

#include "mxh/server/agent_packed.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

namespace {

using mxh::server::PackedAction;
using mxh::server::PackedActionKind;
using mxh::server::PackedRequest;
using mxh::server::classify_packed_user;
using mxh::server::packed_category;
using mxh::server::packed_normal;
using mxh::server::packed_to_broad_mapserver;
using mxh::server::packed_to_mapserver;

}  // namespace


// ===========================================================================
// Wire constants (1:1 with legacy MP_PACKEDDATA)
// ===========================================================================

TEST(AgentPackedDataPlane, CategoryByteIsThirteen) {
    EXPECT_EQ(packed_category, 13u);
}

TEST(AgentPackedDataPlane, NormalSubProtocolIsZero) {
    EXPECT_EQ(packed_normal, 0u);
}

TEST(AgentPackedDataPlane, ToMapServerSubProtocolIsOne) {
    EXPECT_EQ(packed_to_mapserver, 1u);
}

TEST(AgentPackedDataPlane, ToBroadMapServerSubProtocolIsTwo) {
    EXPECT_EQ(packed_to_broad_mapserver, 2u);
}

// ===========================================================================
// Fanout (packed_normal): receivers_present filter
// ===========================================================================

TEST(AgentPackedDataPlane, NormalFanoutWithAllPresentKeepsAll) {
    PackedRequest r;
    r.protocol = packed_normal;
    r.receiver_count = 3;
    r.receivers_present = {1, 2, 3};
    auto a = classify_packed_user(r);
    EXPECT_EQ(a.kind, PackedActionKind::fanout_to_users);
    EXPECT_EQ(a.receiver_count, 3u);
}

TEST(AgentPackedDataPlane, NormalFanoutWithNonePresentIsEmpty) {
    PackedRequest r;
    r.protocol = packed_normal;
    r.receiver_count = 5;
    r.receivers_present = {};
    auto a = classify_packed_user(r);
    EXPECT_EQ(a.kind, PackedActionKind::fanout_to_users);
    EXPECT_EQ(a.receiver_count, 0u);
}

TEST(AgentPackedDataPlane, NormalFanoutPreservesDataSize) {
    PackedRequest r;
    r.protocol = packed_normal;
    r.receiver_count = 1;
    r.data_size = 2048;
    r.receivers_present = {99};
    auto a = classify_packed_user(r);
    EXPECT_EQ(a.kind, PackedActionKind::fanout_to_users);
    EXPECT_EQ(a.data_size, 2048u);
}

TEST(AgentPackedDataPlane, NormalFanoutPropagatesProtocolByte) {
    PackedRequest r;
    r.protocol = packed_normal;
    r.receivers_present = {1};
    auto a = classify_packed_user(r);
    EXPECT_EQ(a.protocol, packed_normal);
}

TEST(AgentPackedDataPlane, NormalFanoutPreservesZeroDataSize) {
    PackedRequest r;
    r.protocol = packed_normal;
    r.receivers_present = {1};
    r.data_size = 0;
    auto a = classify_packed_user(r);
    EXPECT_EQ(a.kind, PackedActionKind::fanout_to_users);
    EXPECT_EQ(a.data_size, 0u);
}

// ===========================================================================
// ToMapServer: port gate
// ===========================================================================

TEST(AgentPackedDataPlane, ToMapServerPreservesTargetMapNum) {
    PackedRequest r;
    r.protocol = packed_to_mapserver;
    r.target_map_num = 42;
    r.target_map_port_found = true;
    auto a = classify_packed_user(r);
    EXPECT_EQ(a.kind, PackedActionKind::send_to_map_server_by_port);
    EXPECT_EQ(a.protocol, packed_to_mapserver);
}

TEST(AgentPackedDataPlane, ToMapServerPreservesZeroMapNum) {
    PackedRequest r;
    r.protocol = packed_to_mapserver;
    r.target_map_num = 0;
    r.target_map_port_found = true;
    auto a = classify_packed_user(r);
    EXPECT_EQ(a.kind, PackedActionKind::send_to_map_server_by_port);
}

TEST(AgentPackedDataPlane, ToMapServerWithPortMissingIsUnknown) {
    PackedRequest r;
    r.protocol = packed_to_mapserver;
    r.target_map_num = 12;
    r.target_map_port_found = false;
    EXPECT_EQ(classify_packed_user(r).kind, PackedActionKind::unknown);
}

// ===========================================================================
// ToBroadMapServer: unconditional broadcast
// ===========================================================================

TEST(AgentPackedDataPlane, ToBroadMapServerAlwaysBroadcasts) {
    PackedRequest r;
    r.protocol = packed_to_broad_mapserver;
    r.target_map_num = 999;
    r.target_map_port_found = false;  // irrelevant for broadcast
    auto a = classify_packed_user(r);
    EXPECT_EQ(a.kind, PackedActionKind::broadcast_to_other_maps);
    EXPECT_EQ(a.protocol, packed_to_broad_mapserver);
}

TEST(AgentPackedDataPlane, ToBroadMapServerWithNoMapNum) {
    PackedRequest r;
    r.protocol = packed_to_broad_mapserver;
    r.target_map_num = 0;
    auto a = classify_packed_user(r);
    EXPECT_EQ(a.kind, PackedActionKind::broadcast_to_other_maps);
}

TEST(AgentPackedDataPlane, ToBroadMapServerPreservesProtocolByte) {
    PackedRequest r;
    r.protocol = packed_to_broad_mapserver;
    auto a = classify_packed_user(r);
    EXPECT_EQ(a.protocol, packed_to_broad_mapserver);
}


// ===========================================================================
// Unknown protocol semantics
// ===========================================================================

TEST(AgentPackedDataPlane, Protocol255IsUnknown) {
    PackedRequest r;
    r.protocol = 255;
    EXPECT_EQ(classify_packed_user(r).kind, PackedActionKind::unknown);
}

TEST(AgentPackedDataPlane, Protocol77IsUnknown) {
    PackedRequest r;
    r.protocol = 77;
    EXPECT_EQ(classify_packed_user(r).kind, PackedActionKind::unknown);
}

TEST(AgentPackedDataPlane, ProtocolAboveTwoIsUnknown) {
    // packed_normal=0, packed_to_mapserver=1, packed_to_broad_mapserver=2
    // so any value 3..255 is unknown (1:1 with legacy switch fallthrough).
    PackedRequest r;
    r.protocol = 3;
    EXPECT_EQ(classify_packed_user(r).kind, PackedActionKind::unknown);
}

// ===========================================================================
// PackedRequest default-field invariants (1:1 with legacy data plane)
// ===========================================================================

TEST(AgentPackedDataPlane, PackedRequestDefaultIsZero) {
    PackedRequest r;
    EXPECT_EQ(r.protocol, 0u);
    EXPECT_EQ(r.receiver_count, 0u);
    EXPECT_EQ(r.data_size, 0u);
    EXPECT_EQ(r.target_map_num, 0u);
    EXPECT_FALSE(r.target_map_port_found);
    EXPECT_TRUE(r.receivers_present.empty());
}

TEST(AgentPackedDataPlane, PackedActionDefaultIsUnknown) {
    PackedAction a;
    EXPECT_EQ(a.kind, PackedActionKind::unknown);
    EXPECT_EQ(a.protocol, 0u);
    EXPECT_EQ(a.target_object_id, 0u);
    EXPECT_EQ(a.receiver_count, 0u);
    EXPECT_EQ(a.data_size, 0u);
}

// ===========================================================================
// PackedActionKind enum values (uint8_t backed)
// ===========================================================================

TEST(AgentPackedDataPlane, ActionKindEnumBackingIsUint8) {
    static_assert(std::is_same<decltype(static_cast<std::uint8_t>(PackedActionKind::fanout_to_users)),
                               std::uint8_t>::value,
                  "PackedActionKind must be backed by uint8_t");
}

TEST(AgentPackedDataPlane, ActionKindValuesAreDistinct) {
    EXPECT_NE(PackedActionKind::fanout_to_users, PackedActionKind::send_to_map_server_by_port);
    EXPECT_NE(PackedActionKind::fanout_to_users, PackedActionKind::broadcast_to_other_maps);
    EXPECT_NE(PackedActionKind::send_to_map_server_by_port, PackedActionKind::broadcast_to_other_maps);
    EXPECT_NE(PackedActionKind::unknown, PackedActionKind::fanout_to_users);
}

// ===========================================================================
// Receiver_count edge cases (boundary values)
// ===========================================================================

TEST(AgentPackedDataPlane, NormalFanoutWithMaxReceiversPresent) {
    PackedRequest r;
    r.protocol = packed_normal;
    r.receiver_count = 100;
    r.receivers_present.reserve(100);
    for (std::uint32_t i = 0; i < 100; ++i) r.receivers_present.push_back(i + 1);
    auto a = classify_packed_user(r);
    EXPECT_EQ(a.kind, PackedActionKind::fanout_to_users);
    EXPECT_EQ(a.receiver_count, 100u);
}

TEST(AgentPackedDataPlane, NormalFanoutFiltersHalfPresent) {
    PackedRequest r;
    r.protocol = packed_normal;
    r.receiver_count = 10;
    r.receivers_present = {2, 4, 6, 8, 10};
    auto a = classify_packed_user(r);
    EXPECT_EQ(a.kind, PackedActionKind::fanout_to_users);
    EXPECT_EQ(a.receiver_count, 5u);
}

TEST(AgentPackedDataPlane, NormalFanoutWithSingleReceiver) {
    PackedRequest r;
    r.protocol = packed_normal;
    r.receiver_count = 1;
    r.receivers_present = {42};
    auto a = classify_packed_user(r);
    EXPECT_EQ(a.kind, PackedActionKind::fanout_to_users);
    EXPECT_EQ(a.receiver_count, 1u);
}


// ===========================================================================
// Cross-checks: protocol/byte lock and dispatch independence
// ===========================================================================

TEST(AgentPackedDataPlane, SameProtocolDifferentReceiversIsStillFanout) {
    PackedRequest r;
    r.protocol = packed_normal;
    r.receiver_count = 2;
    r.receivers_present = {1, 2};
    auto a = classify_packed_user(r);
    EXPECT_EQ(a.kind, PackedActionKind::fanout_to_users);
    EXPECT_EQ(a.receiver_count, 2u);
}

TEST(AgentPackedDataPlane, ToMapServerIgnoresReceiversPresent) {
    // to_mapserver is direct forward; receivers_present is irrelevant.
    PackedRequest r;
    r.protocol = packed_to_mapserver;
    r.receiver_count = 10;
    r.receivers_present = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    r.target_map_num = 5;
    r.target_map_port_found = true;
    auto a = classify_packed_user(r);
    EXPECT_EQ(a.kind, PackedActionKind::send_to_map_server_by_port);
}

TEST(AgentPackedDataPlane, ToBroadMapServerIgnoresReceiversPresent) {
    PackedRequest r;
    r.protocol = packed_to_broad_mapserver;
    r.receiver_count = 10;
    r.receivers_present = {1, 2, 3};
    auto a = classify_packed_user(r);
    EXPECT_EQ(a.kind, PackedActionKind::broadcast_to_other_maps);
}

TEST(AgentPackedDataPlane, PackedActionCopiesProtocolFromRequest) {
    PackedRequest r;
    r.protocol = packed_to_mapserver;
    r.target_map_port_found = true;
    auto a = classify_packed_user(r);
    EXPECT_EQ(a.protocol, packed_to_mapserver);
}

TEST(AgentPackedDataPlane, DefaultActionObjectIdIsZero) {
    PackedRequest r;
    r.protocol = packed_normal;
    r.receivers_present = {1};
    auto a = classify_packed_user(r);
    EXPECT_EQ(a.target_object_id, 0u);
}

// ===========================================================================
// Data-size boundary preservation (legacy wire bytes)
// ===========================================================================

TEST(AgentPackedDataPlane, DataSize65527Preserved) {
    PackedRequest r;
    r.protocol = packed_normal;
    r.data_size = 65527u;
    r.receivers_present = {1};
    auto a = classify_packed_user(r);
    EXPECT_EQ(a.data_size, 65527u);
}

TEST(AgentPackedDataPlane, DataSize1Preserved) {
    PackedRequest r;
    r.protocol = packed_normal;
    r.data_size = 1u;
    r.receivers_present = {1};
    auto a = classify_packed_user(r);
    EXPECT_EQ(a.data_size, 1u);
}

// ===========================================================================
// Wire byte invariants (sub-protocol range 0..2)
// ===========================================================================

TEST(AgentPackedDataPlane, SubProtocolsAreContiguousFromZero) {
    EXPECT_EQ(packed_normal, 0u);
    EXPECT_EQ(packed_to_mapserver, 1u);
    EXPECT_EQ(packed_to_broad_mapserver, 2u);
}

TEST(AgentPackedDataPlane, AllSubProtocolsAreUnique) {
    EXPECT_NE(packed_normal, packed_to_mapserver);
    EXPECT_NE(packed_normal, packed_to_broad_mapserver);
    EXPECT_NE(packed_to_mapserver, packed_to_broad_mapserver);
}

