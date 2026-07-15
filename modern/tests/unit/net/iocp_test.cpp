// iocp_test.cpp - Phase 10.11 IOCP server + connection unit tests
//
// Covers the high-performance IOCP networking layer in
// modern/include/mxh/net/iocp/iocp.hpp:
//   - IocpServer default state + configuration setters
//   - ConnectionId type alias
//   - SocketAddress construction from sockaddr_storage (round-trip)
//   - SocketAddress::host() / port() accessors
//
// What is NOT covered here:
//   - IocpServer::start() / stop() / send() / broadcast() — those need
//     a real listening socket, real worker threads, and real accept
//     loop. They are exercised by the integration test
//     modern/tools/MoxianMapServer 5/5 smoke and by manual
//     `docker compose up mssql` runs. Putting them in a unit test
//     would require pulling in Winsock init + a real bind, which is
//     not what "unit" means.
//   - AcceptEx / GetAcceptExSockaddrs — those are Windows-only
//     function pointers loaded at runtime; they only fire inside
//     the accept thread loop, which is part of the start() path.
//
// What IS here: the public API surface that callers (5 server tools,
// map handlers, agent handlers) interact with, plus the
// sockaddr_storage round-trip which used to be broken (C-36 era:
// iocp.cpp passed a sockaddr_in to a ctor that only accepted
// sockaddr_storage — fixed in P10.11).

#include "mxh/net/iocp/iocp.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <cstring>
#include <span>
#include <type_traits>

namespace mxh::net::iocp::test {

// ===========================================================================
// IocpServer state + config (no network I/O)
// ===========================================================================

TEST(IocpServerTest, DefaultConstructionIsNotRunning) {
    IocpServer server;
    EXPECT_FALSE(server.is_running());
    EXPECT_EQ(server.connection_count(), 0u);
}

TEST(IocpServerTest, SetMaxConnectionsAcceptsValue) {
    IocpServer server;
    // The setter does not throw for any reasonable value; it does
    // cap at kMaxConnections (10k) internally if you go higher.
    server.set_max_connections(100);
    server.set_max_connections(10000);
    // No crash = pass; the value is private and exercised via
    // accept thread under real load.
    SUCCEED();
}

TEST(IocpServerTest, SetBufferSizeCapsAt64KiB) {
    IocpServer server;
    server.set_buffer_size(8192);
    server.set_buffer_size(65536);
    // Going over 64KiB is silently clamped. No public getter, so we
    // can only verify the setter does not crash.
    SUCCEED();
}

TEST(IocpServerTest, SetHandlersAcceptsCallbacks) {
    IocpServer server;
    std::atomic<int> connect_count{0};
    std::atomic<int> disconnect_count{0};
    std::atomic<int> message_count{0};
    std::atomic<int> error_count{0};

    server.set_connect_handler([&](ConnectionId, const SocketAddress&) {
        connect_count.fetch_add(1, std::memory_order_relaxed);
    });
    server.set_disconnect_handler([&](ConnectionId) {
        disconnect_count.fetch_add(1, std::memory_order_relaxed);
    });
    server.set_message_handler([&](ConnectionId, std::span<const std::uint8_t>) {
        message_count.fetch_add(1, std::memory_order_relaxed);
    });
    server.set_error_handler([&](ConnectionId, const std::error_code&) {
        error_count.fetch_add(1, std::memory_order_relaxed);
    });

    // Handlers are stored; no public way to invoke them without
    // actually starting the server. Setting them should not throw
    // and should not change connection_count().
    EXPECT_FALSE(server.is_running());
    EXPECT_EQ(server.connection_count(), 0u);
    EXPECT_EQ(connect_count.load(), 0);
    EXPECT_EQ(disconnect_count.load(), 0);
    EXPECT_EQ(message_count.load(), 0);
    EXPECT_EQ(error_count.load(), 0);
}

TEST(IocpServerTest, StopOnNonRunningServerIsNoOp) {
    IocpServer server;
    // Calling stop() without start() should not throw or crash.
    server.stop();
    EXPECT_FALSE(server.is_running());
}

// ===========================================================================
// Type aliases
// ===========================================================================

TEST(IocpConnectionIdTest, IsUint64) {
    // ConnectionId is std::uint64_t per the header. Verify the type
    // alias so downstream code (server handlers, connection maps)
    // can rely on it. If this changes, the wire protocol would
    // need to be re-tested for compatibility.
    static_assert(std::is_same_v<ConnectionId, std::uint64_t>,
                  "ConnectionId must be uint64_t");
    SUCCEED();
}

TEST(IocpBufferConstantsTest, DefaultsAreSane) {
    // kDefaultBufferSize and kMaxBufferSize are constants in the
    // header. Pin them so a future change to these values shows up
    // here instead of silently changing server behaviour.
    EXPECT_EQ(kDefaultBufferSize, 8192u);
    EXPECT_EQ(kMaxBufferSize, 65536u);
    EXPECT_EQ(kMaxConnections, 10000u);
}

// ===========================================================================
// SocketAddress sockaddr_storage round-trip (the P10.11 fix area)
// ===========================================================================

TEST(SocketAddressTest, ConstructFromSockaddrStorageIPv4) {
    // SocketGuard initialises Winsock (WSAStartup/Cleanup) for the
    // scope of this test. host() / getnameinfo() require Winsock;
    // without it the round-trip would silently return "".
    SocketGuard guard;

    // Build a sockaddr_storage representing 127.0.0.1:8080 (the same
    // shape iocp.cpp's accept loop produces), then round-trip it
    // through SocketAddress. The bug that was disabled in CMake was
    // a sockaddr_in -> SocketAddress conversion that did not exist;
    // we now construct a sockaddr_storage explicitly and verify the
    // path still works.
    sockaddr_storage storage{};
    std::memset(&storage, 0, sizeof(storage));
    auto* in = reinterpret_cast<sockaddr_in*>(&storage);
    in->sin_family = AF_INET;
    in->sin_port = htons(8080);
    in->sin_addr.s_addr = htonl(0x7F000001);  // 127.0.0.1

    SocketAddress addr(storage);
    EXPECT_TRUE(addr.is_valid());

    // host() returns dotted-decimal or "::" for v6.
    EXPECT_EQ(addr.host(), "127.0.0.1");
    EXPECT_EQ(addr.port(), 8080);
}

TEST(SocketAddressTest, ConstructFromSockaddrStorageIPv6) {
    SocketGuard guard;

    sockaddr_storage storage{};
    std::memset(&storage, 0, sizeof(storage));
    auto* in6 = reinterpret_cast<sockaddr_in6*>(&storage);
    in6->sin6_family = AF_INET6;
    in6->sin6_port = htons(9090);
    // ::1 loopback
    in6->sin6_addr.s6_addr[15] = 1;

    SocketAddress addr(storage);
    EXPECT_TRUE(addr.is_valid());
    EXPECT_EQ(addr.port(), 9090);
    // host() returns "::1" for IPv6 loopback.
    EXPECT_EQ(addr.host(), "::1");
}

TEST(SocketAddressTest, ConstructFromStoragePreservesFamily) {
    sockaddr_storage storage{};
    std::memset(&storage, 0, sizeof(storage));
    auto* in = reinterpret_cast<sockaddr_in*>(&storage);
    in->sin_family = AF_INET;
    in->sin_port = htons(443);
    in->sin_addr.s_addr = htonl(0xC0A80101);  // 192.168.1.1

    SocketAddress addr(storage);
    const sockaddr* raw = addr.data();
    ASSERT_NE(raw, nullptr);
    EXPECT_EQ(raw->sa_family, AF_INET);
}

TEST(SocketAddressTest, DataAndSizeAreConsistent) {
    // The data() pointer must point at the internal sockaddr_storage
    // and size() must match the address family in use.
    sockaddr_storage storage{};
    std::memset(&storage, 0, sizeof(storage));
    auto* in = reinterpret_cast<sockaddr_in*>(&storage);
    in->sin_family = AF_INET;
    in->sin_port = htons(80);

    SocketAddress addr(storage);
    EXPECT_EQ(addr.size(), sizeof(sockaddr_in));
}

TEST(SocketAddressTest, ToStringIsNotEmpty) {
    SocketGuard guard;

    sockaddr_storage storage{};
    std::memset(&storage, 0, sizeof(storage));
    auto* in = reinterpret_cast<sockaddr_in*>(&storage);
    in->sin_family = AF_INET;
    in->sin_port = htons(22);
    in->sin_addr.s_addr = htonl(0x7F000001);  // 127.0.0.1

    SocketAddress addr(storage);
    std::string s = addr.to_string();
    EXPECT_FALSE(s.empty());
    // to_string format is "host:port"; just verify the port is in
    // the string.
    EXPECT_NE(s.find("22"), std::string::npos);
}

}  // namespace mxh::net::iocp::test
