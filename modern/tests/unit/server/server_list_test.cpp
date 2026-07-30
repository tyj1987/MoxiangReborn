// server_list_test.cpp

#include "mxh/server/server_list.hpp"
#include <gtest/gtest.h>

namespace {
using mxh::server::ServerList;
using mxh::server::ServerListEntry;

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
}

TEST(ServerList, AddUniqueChannelKind) {
    ServerList s;
    EXPECT_TRUE(s.add(make_entry(1, 7, 0, "10.0.0.1", 7001)));
    EXPECT_FALSE(s.add(make_entry(1, 7, 0, "10.0.0.2", 7002)));   // dup channel+kind
    EXPECT_TRUE(s.add(make_entry(1, 7, 1, "10.0.0.1", 7003)));    // diff kind same ch
    EXPECT_EQ(s.size(), 2u);
}

TEST(ServerList, FindPicksLowestChannel) {
    ServerList s;
    s.add(make_entry(3, 7, 0, "10.0.0.3", 7003));
    s.add(make_entry(1, 7, 0, "10.0.0.1", 7001));
    s.add(make_entry(2, 7, 0, "10.0.0.2", 7002));
    s.add(make_entry(9, 8, 0, "10.0.0.9", 7009));                // wrong map
    auto* e = s.find_for_map(7, 0);
    ASSERT_NE(e, nullptr);
    EXPECT_EQ(e->channel_num, 1u);
}

TEST(ServerList, FindForUnknownReturnsNull) {
    ServerList s;
    s.add(make_entry(1, 7, 0, "10.0.0.1", 7001));
    EXPECT_EQ(s.find_for_map(99, 0), nullptr);
    EXPECT_EQ(s.find_for_map(7, 1), nullptr);    // wrong kind
}

TEST(ServerList, SnapshotAndClear) {
    ServerList s;
    s.add(make_entry(1, 7, 0, "10.0.0.1", 7001));
    s.add(make_entry(2, 7, 1, "10.0.0.2", 7002));
    auto snap = s.snapshot();
    EXPECT_EQ(snap.size(), 2u);
    s.clear();
    EXPECT_EQ(s.size(), 0u);
}
