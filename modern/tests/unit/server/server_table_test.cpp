// server_table_test.cpp - Phase 6.3 ServerTable 1:1 port tests.

#include "mxh/server/server_table.hpp"

#include <gtest/gtest.h>

#include <cstring>

namespace {

using mxh::server::FastServerPick;
using mxh::server::MAX_IPADDRESS_SIZE;
using mxh::server::ServerInfo;
using mxh::server::ServerKind;
using mxh::server::ServerTable;
using mxh::server::add_ms_server;
using mxh::server::add_self_server;
using mxh::server::add_server;
using mxh::server::collect_servers;
using mxh::server::find_map_server;
using mxh::server::find_server;
using mxh::server::find_server_for_connection_index;
using mxh::server::get_fast_server;
using mxh::server::get_max_server_connection_index;
using mxh::server::get_ms_server;
using mxh::server::get_self_server;
using mxh::server::get_server_num;
using mxh::server::get_server_port;
using mxh::server::make_server_info;
using mxh::server::make_server_table;
using mxh::server::remove_all_servers;
using mxh::server::remove_server_by_connection_index;
using mxh::server::remove_server_by_port;
using mxh::server::server_table_init;
using mxh::server::server_table_release;
using mxh::server::set_max_server_connection_index;

ServerInfo make_entry(std::uint16_t kind, std::uint16_t num,
                      const char* ip, std::uint16_t port_svr, std::uint16_t port_usr,
                      std::uint32_t conn_idx = 0, std::uint16_t user_cnt = 0) {
    ServerInfo s = make_server_info(kind, num, ip, port_svr, ip, port_usr);
    s.dwConnectionIndex = conn_idx;
    s.wAgentUserCnt     = user_cnt;
    return s;
}

} // namespace

// ---- Constants 1:1 ----

TEST(ServerTableConstants, MaxIPAddressSizeIs16) {
    EXPECT_EQ(MAX_IPADDRESS_SIZE, 16u);
}

TEST(ServerTableConstants, ServerKindEnumMatchesLegacy) {
    EXPECT_EQ(static_cast<std::uint16_t>(ServerKind::ERROR_SERVER),         0u);
    EXPECT_EQ(static_cast<std::uint16_t>(ServerKind::DISTRIBUTE_SERVER),    1u);
    EXPECT_EQ(static_cast<std::uint16_t>(ServerKind::AGENT_SERVER),         2u);
    EXPECT_EQ(static_cast<std::uint16_t>(ServerKind::MAP_SERVER),           3u);
    EXPECT_EQ(static_cast<std::uint16_t>(ServerKind::CHAT_SERVER),          4u);
    EXPECT_EQ(static_cast<std::uint16_t>(ServerKind::MURIM_SERVER),         5u);
    EXPECT_EQ(static_cast<std::uint16_t>(ServerKind::MONITOR_AGENT_SERVER), 6u);
    EXPECT_EQ(static_cast<std::uint16_t>(ServerKind::MONITOR_SERVER),       7u);
    EXPECT_EQ(static_cast<std::uint16_t>(ServerKind::BUDDYAUTH_SERVER),     8u);
    EXPECT_EQ(static_cast<std::uint16_t>(ServerKind::MAX_SERVER_KIND),      9u);
}

// ---- POD 1:1 ----

TEST(ServerTablePOD, ServerInfoDefaultsAreZero) {
    ServerInfo s;
    EXPECT_EQ(s.wServerKind,
              static_cast<std::uint16_t>(ServerKind::ERROR_SERVER));
    for (char c : s.szIPForServer) EXPECT_EQ(c, 0);
    for (char c : s.szIPForUser)   EXPECT_EQ(c, 0);
    EXPECT_EQ(s.wPortForServer, 0u);
    EXPECT_EQ(s.wPortForUser,   0u);
    EXPECT_EQ(s.wServerNum,     0u);
    EXPECT_EQ(s.dwConnectionIndex, 0u);
    EXPECT_EQ(s.wAgentUserCnt,  0u);
}

TEST(ServerTablePOD, MakeServerInfoMirrorsLegacyCtor) {
    ServerInfo s = make_server_info(static_cast<std::uint16_t>(ServerKind::AGENT_SERVER),
                                     /*num*/      7u,
                                     /*ip_svr*/   "10.0.0.1",
                                     /*port_svr*/ 4010u,
                                     /*ip_usr*/   "10.0.0.2",
                                     /*port_usr*/ 4011u);
    EXPECT_EQ(s.wServerKind,
              static_cast<std::uint16_t>(ServerKind::AGENT_SERVER));
    EXPECT_EQ(s.wServerNum,     7u);
    EXPECT_EQ(s.wPortForServer, 4010u);
    EXPECT_EQ(s.wPortForUser,   4011u);
    EXPECT_STREQ(s.szIPForServer, "10.0.0.1");
    EXPECT_STREQ(s.szIPForUser,   "10.0.0.2");
    EXPECT_EQ(s.dwConnectionIndex, 0u);
    EXPECT_EQ(s.wAgentUserCnt, 0u);
}

TEST(ServerTablePOD, MakeServerInfoTruncatesLongIP) {
    char long_ip[MAX_IPADDRESS_SIZE + 8];
    std::strncpy(long_ip, "123.123.123.123.123.123", sizeof(long_ip) - 1);
    long_ip[sizeof(long_ip) - 1] = 0;
    ServerInfo s = make_server_info(static_cast<std::uint16_t>(ServerKind::AGENT_SERVER),
                                     1u, long_ip, 4000u, long_ip, 4001u);
    // Truncation rule: legacy uses strcpy (unsafe), modern uses strncpy + NUL.
    EXPECT_EQ(std::strlen(s.szIPForServer), MAX_IPADDRESS_SIZE - 1u);
    EXPECT_EQ(s.szIPForServer[MAX_IPADDRESS_SIZE - 1u], 0);
}

TEST(ServerTablePOD, MakeServerInfoHandlesNullIPs) {
    ServerInfo s = make_server_info(static_cast<std::uint16_t>(ServerKind::AGENT_SERVER),
                                     1u, nullptr, 4000u, nullptr, 4001u);
    EXPECT_EQ(s.wPortForServer, 4000u);
    EXPECT_EQ(s.wPortForUser,   4001u);
    for (char c : s.szIPForServer) EXPECT_EQ(c, 0);
}

// ---- Lifecycle ----

TEST(ServerTableInit, MakeServerTableIsEmpty) {
    auto t = make_server_table();
    EXPECT_TRUE(t.m_Table.empty());
    EXPECT_EQ(t.m_pSelfServerInfo, nullptr);
    EXPECT_EQ(t.m_pMSServerInfo,   nullptr);
    EXPECT_EQ(t.m_MaxServerConnectionIndex, 0u);
}

TEST(ServerTableInit, ServerTableInitResetsAfterUse) {
    auto t = make_server_table();
    ServerInfo s = make_entry(static_cast<std::uint16_t>(ServerKind::AGENT_SERVER),
                              1u, "127.0.0.1", 4000u, 4001u, 0xABCDu);
    add_server(t, s, 4000u);
    ASSERT_TRUE(add_self_server(t, s));
    server_table_init(t);

    EXPECT_TRUE(t.m_Table.empty());
    EXPECT_EQ(t.m_pSelfServerInfo, nullptr);
    EXPECT_EQ(t.m_pMSServerInfo,   nullptr);
    EXPECT_EQ(t.m_MaxServerConnectionIndex, 0u);
}

TEST(ServerTableRelease, ReleaseClearsEverything) {
    auto t = make_server_table();
    ServerInfo a = make_entry(static_cast<std::uint16_t>(ServerKind::AGENT_SERVER), 1u, "1.1.1.1", 4000u, 4001u);
    ServerInfo m = make_entry(static_cast<std::uint16_t>(ServerKind::MAP_SERVER),   2u, "2.2.2.2", 5000u, 5001u);
    add_server(t, a, 4000u);
    add_server(t, m, 5000u);
    ASSERT_TRUE(add_self_server(t, a));
    ASSERT_TRUE(add_ms_server(t, m));

    server_table_release(t);

    EXPECT_TRUE(t.m_Table.empty());
    EXPECT_EQ(t.m_pSelfServerInfo, nullptr);
    EXPECT_EQ(t.m_pMSServerInfo,   nullptr);
}

// ---- Add / find ----

TEST(ServerTableAdd, AddAndFindByPort) {
    auto t = make_server_table();
    ServerInfo s = make_entry(static_cast<std::uint16_t>(ServerKind::AGENT_SERVER),
                              1u, "127.0.0.1", 4000u, 4001u);
    add_server(t, s, 4000u);

    const ServerInfo* found = find_server(t, 4000u);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->wServerNum, 1u);
    EXPECT_EQ(found->wPortForServer, 4000u);
    EXPECT_EQ(find_server(t, 9999u), nullptr);
}

TEST(ServerTableAdd, AddOverwritesOnSamePort) {
    auto t = make_server_table();
    ServerInfo s1 = make_entry(static_cast<std::uint16_t>(ServerKind::AGENT_SERVER), 1u, "1.1.1.1", 4000u, 4001u);
    ServerInfo s2 = make_entry(static_cast<std::uint16_t>(ServerKind::AGENT_SERVER), 2u, "2.2.2.2", 4000u, 4002u);
    add_server(t, s1, 4000u);
    add_server(t, s2, 4000u);
    EXPECT_EQ(t.m_Table.size(), 1u);
    EXPECT_EQ(find_server(t, 4000u)->wServerNum, 2u);
}

TEST(ServerTableAdd, FindServerForConnectionIndex) {
    auto t = make_server_table();
    ServerInfo s = make_entry(static_cast<std::uint16_t>(ServerKind::MAP_SERVER),
                              5u, "10.0.0.5", 5005u, 5006u,
                              /*conn_idx*/ 0xDEADBEEFu);
    add_server(t, s, 5005u);
    ServerInfo* found = find_server_for_connection_index(t, 0xDEADBEEFu);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->wPortForServer, 5005u);
    EXPECT_EQ(find_server_for_connection_index(t, 0xFFFFFFFFu), nullptr);
}

// ---- Self / MS pointers ----

TEST(ServerTableSelfMS, AddSelfServerOnce) {
    auto t = make_server_table();
    ServerInfo a = make_entry(static_cast<std::uint16_t>(ServerKind::AGENT_SERVER), 1u, "1.1.1.1", 4000u, 4001u);
    add_server(t, a, 4000u);
    EXPECT_TRUE(add_self_server(t, a));
    EXPECT_EQ(get_self_server(t), &a);
}

TEST(ServerTableSelfMS, AddSelfServerTwiceRejected) {
    auto t = make_server_table();
    ServerInfo a = make_entry(static_cast<std::uint16_t>(ServerKind::AGENT_SERVER), 1u, "1.1.1.1", 4000u, 4001u);
    ServerInfo b = make_entry(static_cast<std::uint16_t>(ServerKind::AGENT_SERVER), 2u, "2.2.2.2", 4002u, 4003u);
    add_server(t, a, 4000u);
    add_server(t, b, 4002u);
    EXPECT_TRUE(add_self_server(t, a));
    EXPECT_FALSE(add_self_server(t, b));   // legacy asserts; modern returns false
    EXPECT_EQ(get_self_server(t), &a);
}

TEST(ServerTableSelfMS, AddMSServerOnceAndTwice) {
    auto t = make_server_table();
    ServerInfo m = make_entry(static_cast<std::uint16_t>(ServerKind::MAP_SERVER), 1u, "9.9.9.9", 5000u, 5001u);
    add_server(t, m, 5000u);
    EXPECT_TRUE(add_ms_server(t, m));
    EXPECT_FALSE(add_ms_server(t, m));
    EXPECT_EQ(get_ms_server(t), &m);
}

// ---- Remove ----

TEST(ServerTableRemove, RemoveByPortReturnsValue) {
    auto t = make_server_table();
    ServerInfo s = make_entry(static_cast<std::uint16_t>(ServerKind::AGENT_SERVER), 1u, "1.1.1.1", 4000u, 4001u);
    add_server(t, s, 4000u);
    ServerInfo popped = remove_server_by_port(t, 4000u);
    EXPECT_EQ(popped.wPortForServer, 4000u);
    EXPECT_EQ(find_server(t, 4000u), nullptr);
}

TEST(ServerTableRemove, RemoveByPortMissingReturnsEmpty) {
    auto t = make_server_table();
    ServerInfo popped = remove_server_by_port(t, 9999u);
    EXPECT_EQ(popped.wPortForServer, 0u);
}

TEST(ServerTableRemove, RemoveByConnectionIndex) {
    auto t = make_server_table();
    ServerInfo s = make_entry(static_cast<std::uint16_t>(ServerKind::MAP_SERVER),
                              5u, "10.0.0.5", 5005u, 5006u,
                              /*conn_idx*/ 0xCAFEu);
    add_server(t, s, 5005u);
    auto popped = remove_server_by_connection_index(t, 0xCAFEu);
    ASSERT_TRUE(popped.has_value());
    EXPECT_EQ(popped->wPortForServer, 5005u);
    EXPECT_EQ(find_server(t, 5005u), nullptr);
}

TEST(ServerTableRemove, RemoveByConnectionIndexMissing) {
    auto t = make_server_table();
    auto popped = remove_server_by_connection_index(t, 0xBEEFu);
    EXPECT_FALSE(popped.has_value());
}

TEST(ServerTableRemove, RemoveAllClearsTable) {
    auto t = make_server_table();
    for (std::uint16_t p = 4000u; p < 4005u; ++p) {
        ServerInfo s = make_entry(static_cast<std::uint16_t>(ServerKind::AGENT_SERVER),
                                  static_cast<std::uint16_t>(p - 4000u),
                                  "127.0.0.1", p, static_cast<std::uint16_t>(p + 1));
        add_server(t, s, p);
    }
    EXPECT_EQ(t.m_Table.size(), 5u);
    remove_all_servers(t);
    EXPECT_TRUE(t.m_Table.empty());
}

// ---- Port / Num / MapServer lookup ----

TEST(ServerTableLookup, GetServerPortByKindAndNum) {
    auto t = make_server_table();
    ServerInfo a = make_entry(static_cast<std::uint16_t>(ServerKind::AGENT_SERVER), 1u, "1.1.1.1", 4000u, 4001u);
    ServerInfo m = make_entry(static_cast<std::uint16_t>(ServerKind::MAP_SERVER),   3u, "3.3.3.3", 5003u, 5004u);
    add_server(t, a, 4000u);
    add_server(t, m, 5003u);

    EXPECT_EQ(get_server_port(t, ServerKind::MAP_SERVER,   3u), 5003u);
    EXPECT_EQ(get_server_port(t, ServerKind::AGENT_SERVER, 1u), 4000u);
    EXPECT_EQ(get_server_port(t, ServerKind::MAP_SERVER,   999u), 0u);
}

TEST(ServerTableLookup, GetServerNumByPort) {
    auto t = make_server_table();
    ServerInfo m = make_entry(static_cast<std::uint16_t>(ServerKind::MAP_SERVER), 7u, "7.7.7.7", 5007u, 5008u);
    add_server(t, m, 5007u);
    EXPECT_EQ(get_server_num(t, 5007u), 7u);
    EXPECT_EQ(get_server_num(t, 9999u), 0u);
}

TEST(ServerTableLookup, FindMapServerByMapNum) {
    auto t = make_server_table();
    ServerInfo m = make_entry(static_cast<std::uint16_t>(ServerKind::MAP_SERVER), 12u, "12.12.12.12", 5012u, 5013u);
    add_server(t, m, 5012u);
    ServerInfo* found = find_map_server(t, 12u);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->wPortForServer, 5012u);
    EXPECT_EQ(find_map_server(t, 9999u), nullptr);
}

// ---- GetFastServer ----

TEST(ServerTableFast, GetFastServerPicksLowestUserCount) {
    auto t = make_server_table();
    ServerInfo a = make_entry(static_cast<std::uint16_t>(ServerKind::AGENT_SERVER), 1u, "1", 4001u, 4001u, 0x1u, 500u);
    ServerInfo b = make_entry(static_cast<std::uint16_t>(ServerKind::AGENT_SERVER), 2u, "2", 4002u, 4002u, 0x2u, 100u);
    ServerInfo c = make_entry(static_cast<std::uint16_t>(ServerKind::AGENT_SERVER), 3u, "3", 4003u, 4003u, 0x3u, 250u);
    add_server(t, a, 4001u);
    add_server(t, b, 4002u);
    add_server(t, c, 4003u);

    FastServerPick pick = get_fast_server(t, ServerKind::AGENT_SERVER);
    EXPECT_TRUE(pick.valid);
    ASSERT_NE(pick.info, nullptr);
    EXPECT_EQ(pick.info->wServerNum, 2u);
    EXPECT_EQ(pick.connection_index, 0x2u);
}

TEST(ServerTableFast, GetFastServerEmptyTableReturnsInvalid) {
    auto t = make_server_table();
    FastServerPick pick = get_fast_server(t, ServerKind::AGENT_SERVER);
    EXPECT_FALSE(pick.valid);
    EXPECT_EQ(pick.info, nullptr);
}

TEST(ServerTableFast, GetFastServerIncludesUnconnectedSameKind) {
    // Legacy quirk: a server with dwConnectionIndex == 0 BUT wServerKind ==
    // target kind is still considered (so a freshly-launched, not-yet-
    // connected MAP server can be picked).
    auto t = make_server_table();
    ServerInfo connected = make_entry(static_cast<std::uint16_t>(ServerKind::MAP_SERVER),
                                       1u, "1", 5001u, 5001u, 0xAu, 1000u);
    ServerInfo unconnected = make_entry(static_cast<std::uint16_t>(ServerKind::MAP_SERVER),
                                         2u, "2", 5002u, 5002u, 0u, 50u);
    add_server(t, connected,    5001u);
    add_server(t, unconnected,  5002u);

    FastServerPick pick = get_fast_server(t, ServerKind::MAP_SERVER);
    ASSERT_NE(pick.info, nullptr);
    EXPECT_EQ(pick.info->wServerNum, 2u);
}

TEST(ServerTableFast, GetFastServerWithIpAndPortOverload) {
    auto t = make_server_table();
    ServerInfo a = make_entry(static_cast<std::uint16_t>(ServerKind::AGENT_SERVER),
                              1u, "9.9.9.9", 4001u, 7777u, 0x1u, 1u);
    add_server(t, a, 4001u);

    char buf[MAX_IPADDRESS_SIZE] = {};
    std::uint16_t port = 0;
    EXPECT_TRUE(get_fast_server(t, ServerKind::AGENT_SERVER, buf, &port));
    EXPECT_STREQ(buf, "9.9.9.9");
    EXPECT_EQ(port, 7777u);
}

TEST(ServerTableFast, GetFastServerWithIpAndPortEmpty) {
    auto t = make_server_table();
    char buf[MAX_IPADDRESS_SIZE] = {};
    std::uint16_t port = 0;
    EXPECT_FALSE(get_fast_server(t, ServerKind::AGENT_SERVER, buf, &port));
}

// ---- MaxServerConnectionIndex ----

TEST(ServerTableMax, GetSetRoundTrip) {
    auto t = make_server_table();
    EXPECT_EQ(get_max_server_connection_index(t), 0u);
    set_max_server_connection_index(t, 42u);
    EXPECT_EQ(get_max_server_connection_index(t), 42u);
}

// ---- Snapshot helper ----

TEST(ServerTableSnapshot, CollectServersReturnsAllEntries) {
    auto t = make_server_table();
    ServerInfo a = make_entry(static_cast<std::uint16_t>(ServerKind::AGENT_SERVER), 1u, "1", 4001u, 4001u);
    ServerInfo b = make_entry(static_cast<std::uint16_t>(ServerKind::MAP_SERVER),   2u, "2", 5002u, 5003u);
    add_server(t, a, 4001u);
    add_server(t, b, 5002u);

    auto v = collect_servers(t);
    EXPECT_EQ(v.size(), 2u);
    std::uint16_t sum = 0;
    for (const auto& s : v) sum = static_cast<std::uint16_t>(sum + s.wServerNum);
    EXPECT_EQ(sum, 3u);  // 1 + 2
}

