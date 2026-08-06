
// server_list_data_plane_test.cpp
//
// Comprehensive data plane tests for mxh::server::server_list (D4.129).
// Augments the legacy 4-test server_list_test.cpp with deeper coverage of:
//   - ServerListEntry wire-format invariants (size + field offsets).
//   - Kind enumeration (0 = agent, 1 = map) and the (channel, kind)
//     uniqueness key that drives ServerList::add() dup-reject gate.
//   - add() port-zero rejection (the one byte-level check the legacy
//     ServerList.cpp performs before insertion).
//   - find_for_map() lowest-channel selection across heterogeneous
//     kind populations.
//   - snapshot() / clear() copy semantics.
//   - Default port constant + DEFAULT_DISTRIBUTE_PORT = 6001.
//
// 1:1 invariants (locked):
//   - DEFAULT_DISTRIBUTE_PORT = 6001 (legacy DistributeServer default).
//   - ServerListEntry channel_num at offset 0 (uint16).
//   - ServerListEntry map_num at offset 2 (uint16).
//   - ServerListEntry kind at offset 4 (uint8); 0 = agent, 1 = map.
//   - ServerListEntry reserved0 at offset 5 (uint8).
//   - ServerListEntry reserved1 at offset 6 (uint16).
//   - ServerListEntry ip[16] at offset 8 (legacy ip_str[16]).
//   - ServerListEntry port at offset 24 (uint16).
//   - sizeof(ServerListEntry) = 26 (natural alignment, max field align = 2).
//   - add() rejects entries with port == 0 (legacy zero-port guard).
//   - add() rejects entries that duplicate an existing (channel, kind) pair.
//   - find_for_map() returns the entry with the matching (map_num, kind)
//     that has the lowest channel_num (legacy uses < to pick).

#pragma once

#include "mxh/server/server_list.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>

namespace {

using mxh::server::ServerList;
using mxh::server::ServerListEntry;

// DEFAULT_DISTRIBUTE_PORT is declared in distribute_server.hpp; we duplicate
// the literal here so server_list_data_plane_test.cpp stays self-contained.
static constexpr std::uint16_t kDefaultDistributePort = 6001;

ServerListEntry make_entry(std::uint16_t ch, std::uint16_t map_num,
                            std::uint8_t kind, const char* ip, std::uint16_t port) {
    ServerListEntry e{};
    e.channel_num = ch;
    e.map_num     = map_num;
    e.kind        = kind;
    std::strncpy(e.ip.data(), ip, e.ip.size() - 1);
    e.port = port;
    return e;
}

}  // namespace


// ===========================================================================
// Wire format: ServerListEntry (1:1 with legacy SERVERLIST_INFO)
// ===========================================================================

TEST(ServerListDataPlane, EntrySizeIsTwentySixBytes) {
    EXPECT_EQ(sizeof(ServerListEntry), 26u);
}

TEST(ServerListDataPlane, EntryChannelNumAtOffsetZero) {
    EXPECT_EQ(offsetof(ServerListEntry, channel_num), 0u);
}

TEST(ServerListDataPlane, EntryMapNumAtOffsetTwo) {
    EXPECT_EQ(offsetof(ServerListEntry, map_num), 2u);
}

TEST(ServerListDataPlane, EntryKindAtOffsetFour) {
    EXPECT_EQ(offsetof(ServerListEntry, kind), 4u);
}

TEST(ServerListDataPlane, EntryReserved0AtOffsetFive) {
    EXPECT_EQ(offsetof(ServerListEntry, reserved0), 5u);
}

TEST(ServerListDataPlane, EntryReserved1AtOffsetSix) {
    EXPECT_EQ(offsetof(ServerListEntry, reserved1), 6u);
}

TEST(ServerListDataPlane, EntryIpAtOffsetEight) {
    EXPECT_EQ(offsetof(ServerListEntry, ip), 8u);
}

TEST(ServerListDataPlane, EntryPortAtOffsetTwentyFour) {
    EXPECT_EQ(offsetof(ServerListEntry, port), 24u);
}

TEST(ServerListDataPlane, EntryDefaultConstructedIsAllZeros) {
    ServerListEntry e{};
    EXPECT_EQ(e.channel_num, 0u);
    EXPECT_EQ(e.map_num, 0u);
    EXPECT_EQ(e.kind, 0u);
    EXPECT_EQ(e.reserved0, 0u);
    EXPECT_EQ(e.reserved1, 0u);
    EXPECT_EQ(e.port, 0u);
    for (auto b : e.ip) {
        EXPECT_EQ(b, 0);
    }
}

TEST(ServerListDataPlane, EntryIpArrayIsSixteenBytes) {
    ServerListEntry e{};
    EXPECT_EQ(sizeof(e.ip), 16u);
}


// ===========================================================================
// Constants
// ===========================================================================

TEST(ServerListDataPlane, DefaultPortIsSixThousandOne) {
    EXPECT_EQ(kDefaultDistributePort, 6001u);
}

TEST(ServerListDataPlane, KindAgentIsZero) {
    ServerListEntry a{};
    a.kind = 0;
    EXPECT_EQ(a.kind, 0u);
}

TEST(ServerListDataPlane, KindMapIsOne) {
    ServerListEntry m{};
    m.kind = 1;
    EXPECT_EQ(m.kind, 1u);
}


// ===========================================================================
// Default state
// ===========================================================================

TEST(ServerListDataPlane, DefaultServerListIsEmpty) {
    ServerList s;
    EXPECT_EQ(s.size(), 0u);
    EXPECT_TRUE(s.snapshot().empty());
}

TEST(ServerListDataPlane, DefaultServerListFindReturnsNull) {
    ServerList s;
    EXPECT_EQ(s.find_for_map(0, 0), nullptr);
    EXPECT_EQ(s.find_for_map(7, 0), nullptr);
    EXPECT_EQ(s.find_for_map(7, 1), nullptr);
}


// ===========================================================================
// add() - port-zero rejection (legacy guard)
// ===========================================================================

TEST(ServerListDataPlane, AddRejectsZeroPort) {
    ServerList s;
    auto e = make_entry(1, 7, 0, "10.0.0.1", 0);  // port=0
    EXPECT_FALSE(s.add(e));
    EXPECT_EQ(s.size(), 0u);
}

TEST(ServerListDataPlane, AddAcceptsMinimumPortOne) {
    ServerList s;
    auto e = make_entry(1, 7, 0, "10.0.0.1", 1);
    EXPECT_TRUE(s.add(e));
    EXPECT_EQ(s.size(), 1u);
}

TEST(ServerListDataPlane, AddAcceptsMaximumPort65535) {
    ServerList s;
    auto e = make_entry(1, 7, 0, "10.0.0.1", 65535);
    EXPECT_TRUE(s.add(e));
    EXPECT_EQ(s.size(), 1u);
    EXPECT_EQ(s.snapshot().front().port, 65535u);
}


// ===========================================================================
// add() - (channel, kind) uniqueness key
// ===========================================================================

TEST(ServerListDataPlane, AddRejectsSameChannelSameKindDifferentMap) {
    ServerList s;
    EXPECT_TRUE(s.add(make_entry(1, 7, 0, "10.0.0.1", 7001)));
    EXPECT_FALSE(s.add(make_entry(1, 8, 0, "10.0.0.2", 7002)));  // channel=1 kind=0 dup
    EXPECT_EQ(s.size(), 1u);
}

TEST(ServerListDataPlane, AddAcceptsSameChannelDifferentKind) {
    ServerList s;
    EXPECT_TRUE(s.add(make_entry(1, 7, 0, "10.0.0.1", 7001)));  // channel=1 kind=0 (agent)
    EXPECT_TRUE(s.add(make_entry(1, 7, 1, "10.0.0.2", 7002)));  // channel=1 kind=1 (map)
    EXPECT_EQ(s.size(), 2u);
}

TEST(ServerListDataPlane, AddAcceptsSameKindDifferentChannel) {
    ServerList s;
    EXPECT_TRUE(s.add(make_entry(1, 7, 0, "10.0.0.1", 7001)));
    EXPECT_TRUE(s.add(make_entry(2, 7, 0, "10.0.0.2", 7002)));
    EXPECT_TRUE(s.add(make_entry(3, 7, 0, "10.0.0.3", 7003)));
    EXPECT_EQ(s.size(), 3u);
}

TEST(ServerListDataPlane, AddRejectsSameChannelKindEvenWithDifferentIp) {
    ServerList s;
    EXPECT_TRUE(s.add(make_entry(5, 7, 1, "10.0.0.1", 7001)));
    EXPECT_FALSE(s.add(make_entry(5, 7, 1, "10.0.0.99", 7999)));
    EXPECT_EQ(s.size(), 1u);
}

TEST(ServerListDataPlane, AddRejectsDuplicateAfterClearThenReAdd) {
    ServerList s;
    EXPECT_TRUE(s.add(make_entry(1, 7, 0, "10.0.0.1", 7001)));
    s.clear();
    EXPECT_TRUE(s.add(make_entry(1, 7, 0, "10.0.0.2", 7002)));  // cleared, can re-add
    EXPECT_EQ(s.size(), 1u);
    EXPECT_FALSE(s.add(make_entry(1, 7, 0, "10.0.0.3", 7003)));  // now dup
    EXPECT_EQ(s.size(), 1u);
}



// ===========================================================================
// find_for_map() - lowest channel selection
// ===========================================================================

TEST(ServerListDataPlane, FindReturnsLowestChannelWhenMultiple) {
    ServerList s;
    s.add(make_entry(9, 7, 0, "10.0.0.9", 7009));
    s.add(make_entry(3, 7, 0, "10.0.0.3", 7003));
    s.add(make_entry(1, 7, 0, "10.0.0.1", 7001));
    s.add(make_entry(2, 7, 0, "10.0.0.2", 7002));
    auto* e = s.find_for_map(7, 0);
    ASSERT_NE(e, nullptr);
    EXPECT_EQ(e->channel_num, 1u);
    EXPECT_EQ(e->port, 7001u);
}

TEST(ServerListDataPlane, FindSkipsWrongKind) {
    ServerList s;
    s.add(make_entry(1, 7, 1, "10.0.0.1", 7001));  // map endpoint
    auto* e = s.find_for_map(7, 0);  // asking for agent (kind=0)
    EXPECT_EQ(e, nullptr);
}

TEST(ServerListDataPlane, FindSkipsWrongMap) {
    ServerList s;
    s.add(make_entry(1, 7, 0, "10.0.0.1", 7001));
    EXPECT_EQ(s.find_for_map(8, 0), nullptr);
    EXPECT_EQ(s.find_for_map(0, 0), nullptr);
    EXPECT_EQ(s.find_for_map(65535, 0), nullptr);
}

TEST(ServerListDataPlane, FindMatchesKindOne) {
    ServerList s;
    s.add(make_entry(1, 7, 1, "10.0.0.1", 7001));  // map endpoint
    auto* e = s.find_for_map(7, 1);
    ASSERT_NE(e, nullptr);
    EXPECT_EQ(e->channel_num, 1u);
    EXPECT_EQ(e->kind, 1u);
}

TEST(ServerListDataPlane, FindAcrossMixedKinds) {
    ServerList s;
    s.add(make_entry(1, 7, 0, "10.0.0.1", 7001));  // agent for map 7
    s.add(make_entry(2, 7, 1, "10.0.0.2", 7002));  // map for map 7
    s.add(make_entry(3, 8, 0, "10.0.0.3", 7003));  // agent for map 8
    auto* agent = s.find_for_map(7, 0);
    auto* mapend = s.find_for_map(7, 1);
    auto* agent8 = s.find_for_map(8, 0);
    ASSERT_NE(agent, nullptr);
    ASSERT_NE(mapend, nullptr);
    ASSERT_NE(agent8, nullptr);
    EXPECT_EQ(agent->channel_num, 1u);
    EXPECT_EQ(mapend->channel_num, 2u);
    EXPECT_EQ(agent8->channel_num, 3u);
}


// ===========================================================================
// snapshot() / clear()
// ===========================================================================

TEST(ServerListDataPlane, SnapshotPreservesInsertionOrder) {
    ServerList s;
    s.add(make_entry(3, 7, 0, "10.0.0.3", 7003));
    s.add(make_entry(1, 7, 0, "10.0.0.1", 7001));
    s.add(make_entry(2, 7, 0, "10.0.0.2", 7002));
    auto snap = s.snapshot();
    ASSERT_EQ(snap.size(), 3u);
    EXPECT_EQ(snap[0].channel_num, 3u);
    EXPECT_EQ(snap[1].channel_num, 1u);
    EXPECT_EQ(snap[2].channel_num, 2u);
}

TEST(ServerListDataPlane, SnapshotIsCopyNotReference) {
    ServerList s;
    s.add(make_entry(1, 7, 0, "10.0.0.1", 7001));
    auto snap = s.snapshot();
    snap[0].port = 9999;  // mutate copy
    auto again = s.snapshot();
    EXPECT_EQ(again[0].port, 7001u);  // original unchanged
}

TEST(ServerListDataPlane, ClearEmptiesList) {
    ServerList s;
    s.add(make_entry(1, 7, 0, "10.0.0.1", 7001));
    s.add(make_entry(2, 7, 1, "10.0.0.2", 7002));
    s.clear();
    EXPECT_EQ(s.size(), 0u);
    EXPECT_TRUE(s.snapshot().empty());
}

TEST(ServerListDataPlane, ClearOnEmptyIsNoop) {
    ServerList s;
    s.clear();
    s.clear();
    EXPECT_EQ(s.size(), 0u);
}

TEST(ServerListDataPlane, ClearThenFindReturnsNull) {
    ServerList s;
    s.add(make_entry(1, 7, 0, "10.0.0.1", 7001));
    s.clear();
    EXPECT_EQ(s.find_for_map(7, 0), nullptr);
}



// ===========================================================================
// IP array edge cases
// ===========================================================================

TEST(ServerListDataPlane, IpDefaultsAllZeros) {
    ServerListEntry e{};
    for (auto b : e.ip) {
        EXPECT_EQ(b, 0);
    }
}

TEST(ServerListDataPlane, IpStoresFifteenCharsPlusNull) {
    ServerListEntry e{};
    const char* ip = "123.456.789.012";  // 15 chars
    ASSERT_EQ(std::strlen(ip), 15u);
    std::strncpy(e.ip.data(), ip, e.ip.size() - 1);
    EXPECT_STREQ(e.ip.data(), ip);
    EXPECT_EQ(e.ip[15], 0);  // null-terminated
}

TEST(ServerListDataPlane, IpTruncatesAtFifteenChars) {
    ServerListEntry e{};
    std::strncpy(e.ip.data(), "AAAAAAAAAAAAAAAA", e.ip.size() - 1);  // 16 chars
    EXPECT_EQ(std::strlen(e.ip.data()), 15u);
}


// ===========================================================================
// Boundary values
// ===========================================================================

TEST(ServerListDataPlane, ChannelNumZeroIsValid) {
    ServerList s;
    EXPECT_TRUE(s.add(make_entry(0, 7, 0, "10.0.0.0", 7000)));
    auto* e = s.find_for_map(7, 0);
    ASSERT_NE(e, nullptr);
    EXPECT_EQ(e->channel_num, 0u);
}

TEST(ServerListDataPlane, ChannelNumMaxIsValid) {
    ServerList s;
    EXPECT_TRUE(s.add(make_entry(65535, 7, 0, "10.0.0.0", 7000)));
    auto* e = s.find_for_map(7, 0);
    ASSERT_NE(e, nullptr);
    EXPECT_EQ(e->channel_num, 65535u);
}

TEST(ServerListDataPlane, MapNumMaxIsValid) {
    ServerList s;
    EXPECT_TRUE(s.add(make_entry(1, 65535, 0, "10.0.0.0", 7000)));
    auto* e = s.find_for_map(65535, 0);
    ASSERT_NE(e, nullptr);
    EXPECT_EQ(e->map_num, 65535u);
}
