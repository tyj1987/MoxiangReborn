// socket_test.cpp - Unit tests for cross-platform socket abstraction.

#include "mxh/net/socket.hpp"

#include <gtest/gtest.h>

#include <thread>
#include <chrono>
#include <atomic>

using namespace mxh::net;
using namespace std::chrono_literals;

// ============================================================================
// SocketAddress tests
// ============================================================================

TEST(SocketAddressTest, DefaultConstruction) {
    SocketAddress addr;
    EXPECT_FALSE(addr.is_valid());
}

TEST(SocketAddressTest, ConstructFromHostPort) {
    SocketGuard guard;  // Initialize socket subsystem
    SocketAddress addr("127.0.0.1", 8080);
    EXPECT_TRUE(addr.is_valid());
    EXPECT_EQ(addr.host(), "127.0.0.1");
    EXPECT_EQ(addr.port(), 8080);
}

TEST(SocketAddressTest, ConstructFromIPv6) {
    SocketGuard guard;
    SocketAddress addr("::1", 9090);
    EXPECT_TRUE(addr.is_valid());
    EXPECT_EQ(addr.host(), "::1");
    EXPECT_EQ(addr.port(), 9090);
}

TEST(SocketAddressTest, ToString) {
    SocketGuard guard;
    SocketAddress addr("192.168.1.1", 1234);
    EXPECT_EQ(addr.to_string(), "192.168.1.1:1234");
}

// ============================================================================
// Socket tests
// ============================================================================

TEST(SocketTest, DefaultConstruction) {
    Socket sock;
    EXPECT_FALSE(sock.is_valid());
    EXPECT_EQ(sock.state(), Socket::State::Invalid);
}

TEST(SocketTest, CreateTCPSocket) {
    SocketGuard guard;
    Socket sock;
    auto ec = sock.create(Socket::Type::TCP);
    EXPECT_EQ(ec, SocketErrc::Success);
    EXPECT_TRUE(sock.is_valid());
    EXPECT_EQ(sock.state(), Socket::State::Created);
}

TEST(SocketTest, CreateUDPSocket) {
    SocketGuard guard;
    Socket sock;
    auto ec = sock.create(Socket::Type::UDP);
    EXPECT_EQ(ec, SocketErrc::Success);
    EXPECT_TRUE(sock.is_valid());
}

TEST(SocketTest, MoveConstruction) {
    SocketGuard guard;
    Socket sock1;
    sock1.create(Socket::Type::TCP);
    EXPECT_TRUE(sock1.is_valid());

    Socket sock2(std::move(sock1));
    EXPECT_TRUE(sock2.is_valid());
    EXPECT_FALSE(sock1.is_valid());
}

TEST(SocketTest, MoveAssignment) {
    SocketGuard guard;
    Socket sock1;
    sock1.create(Socket::Type::TCP);

    Socket sock2;
    sock2 = std::move(sock1);
    EXPECT_TRUE(sock2.is_valid());
    EXPECT_FALSE(sock1.is_valid());
}

TEST(SocketTest, BindAndListen) {
    SocketGuard guard;
    Socket sock;
    sock.create(Socket::Type::TCP);
    sock.set_reuse_addr(true);

    SocketAddress addr("127.0.0.1", 0);  // Let OS choose port
    auto ec = sock.bind(addr);
    EXPECT_EQ(ec, SocketErrc::Success);
    EXPECT_EQ(sock.state(), Socket::State::Bound);

    ec = sock.listen();
    EXPECT_EQ(ec, SocketErrc::Success);
    EXPECT_EQ(sock.state(), Socket::State::Listening);
}

TEST(SocketTest, ConnectToInvalidAddress) {
    SocketGuard guard;
    Socket sock;
    sock.create(Socket::Type::TCP);

    // Pick an ephemeral port that's almost certainly unbound: bind a temporary
    // socket to (127.0.0.1, 0) to grab an OS-assigned port, then close it. The
    // port is now in TIME_WAIT, and any fresh connect to it should fail fast
    // with WSAECONNREFUSED. This avoids relying on TEST-NET-1 (which triggers
    // a 21s TCP SYN-SENT timeout) or hardcoded port numbers.
    int port = 0;
    {
        SOCKET tmp = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (tmp != INVALID_SOCKET) {
            sockaddr_in a{};
            a.sin_family = AF_INET;
            a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            a.sin_port = 0;
            socklen_t len = sizeof(a);
            if (::bind(tmp, reinterpret_cast<sockaddr*>(&a), sizeof(a)) == 0
                && ::getsockname(tmp, reinterpret_cast<sockaddr*>(&a), &len) == 0) {
                port = ntohs(a.sin_port);
            }
            ::closesocket(tmp);
        }
    }
    ASSERT_NE(port, 0) << "could not allocate ephemeral port";

    SocketAddress addr("127.0.0.1", port);
    sock.set_connect_timeout(std::chrono::milliseconds(500));
    auto start = std::chrono::steady_clock::now();
    auto ec = sock.connect(addr);
    auto elapsed_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();

    // Should fail fast (< 1s) with connection refused or similar.
    EXPECT_NE(ec, SocketErrc::Success) << "expected non-success error (got: "
                                        << ec.message() << ")";
    EXPECT_LT(elapsed_ms, 1500) << "connect took " << elapsed_ms
                                 << "ms — Socket::connect has no timeout";
}

TEST(SocketTest, SetNonBlocking) {
    SocketGuard guard;
    Socket sock;
    sock.create(Socket::Type::TCP);

    auto ec = sock.set_non_blocking(true);
    EXPECT_EQ(ec, SocketErrc::Success);
}

TEST(SocketTest, SetTcpNodelay) {
    SocketGuard guard;
    Socket sock;
    sock.create(Socket::Type::TCP);

    auto ec = sock.set_tcp_nodelay(true);
    EXPECT_EQ(ec, SocketErrc::Success);
}

TEST(SocketTest, SetReuseAddr) {
    SocketGuard guard;
    Socket sock;
    sock.create(Socket::Type::TCP);

    auto ec = sock.set_reuse_addr(true);
    EXPECT_EQ(ec, SocketErrc::Success);
}

TEST(SocketTest, SetBufferSize) {
    SocketGuard guard;
    Socket sock;
    sock.create(Socket::Type::TCP);

    auto ec = sock.set_send_buffer_size(65536);
    EXPECT_EQ(ec, SocketErrc::Success);

    ec = sock.set_recv_buffer_size(65536);
    EXPECT_EQ(ec, SocketErrc::Success);
}

TEST(SocketTest, LocalAddress) {
    SocketGuard guard;
    Socket sock;
    sock.create(Socket::Type::TCP);
    sock.set_reuse_addr(true);

    SocketAddress addr("127.0.0.1", 0);
    sock.bind(addr);

    auto local = sock.local_address();
    EXPECT_TRUE(local.is_valid());
    EXPECT_EQ(local.host(), "127.0.0.1");
    EXPECT_NE(local.port(), 0);  // OS assigned port
}

// ============================================================================
// Error code tests
// ============================================================================

TEST(SocketErrorTest, SuccessCode) {
    auto ec = make_error_code(SocketErrc::Success);
    EXPECT_FALSE(ec);
    EXPECT_EQ(ec.message(), "Success");
}

TEST(SocketErrorTest, ErrorCodes) {
    auto ec1 = make_error_code(SocketErrc::CreateFailed);
    EXPECT_TRUE(ec1);
    EXPECT_EQ(ec1.message(), "Failed to create socket");

    auto ec2 = make_error_code(SocketErrc::ConnectionReset);
    EXPECT_TRUE(ec2);
    EXPECT_EQ(ec2.message(), "Connection reset by peer");
}

TEST(SocketErrorTest, ErrorCategory) {
    auto& cat = socket_error_category();
    EXPECT_STREQ(cat.name(), "socket");
}

// ============================================================================
// Integration test: TCP echo server
// ============================================================================

TEST(SocketIntegrationTest, TCPEchoServer) {
    SocketGuard guard;

    // Create server socket
    Socket server;
    server.create(Socket::Type::TCP);
    server.set_reuse_addr(true);

    SocketAddress addr("127.0.0.1", 0);
    ASSERT_EQ(server.bind(addr), SocketErrc::Success);
    ASSERT_EQ(server.listen(), SocketErrc::Success);

    auto server_addr = server.local_address();
    ASSERT_TRUE(server_addr.is_valid());

    std::atomic<bool> server_ready{false};
    std::atomic<bool> test_done{false};

    // Server thread
    std::thread server_thread([&]() {
        server_ready.store(true);

        auto [client, ec] = server.accept();
        ASSERT_EQ(ec, SocketErrc::Success);
        ASSERT_TRUE(client.is_valid());

        // Echo received data
        std::vector<std::uint8_t> buffer(1024);
        auto [received, recv_ec] = client.receive(buffer);
        ASSERT_EQ(recv_ec, SocketErrc::Success);
        ASSERT_GT(received, 0);

        auto [sent, send_ec] = client.send({buffer.data(), received});
        ASSERT_EQ(send_ec, SocketErrc::Success);
        ASSERT_EQ(sent, received);

        while (!test_done.load()) {
            std::this_thread::sleep_for(1ms);
        }
    });

    // Wait for server to be ready
    while (!server_ready.load()) {
        std::this_thread::sleep_for(1ms);
    }

    // Client connects
    Socket client;
    client.create(Socket::Type::TCP);
    ASSERT_EQ(client.connect(server_addr), SocketErrc::Success);
    ASSERT_EQ(client.state(), Socket::State::Connected);

    // Send data
    std::string message = "Hello, cross-platform!";
    auto [sent, send_ec] = client.send(
        {reinterpret_cast<const std::uint8_t*>(message.data()), message.size()});
    ASSERT_EQ(send_ec, SocketErrc::Success);
    ASSERT_EQ(sent, message.size());

    // Receive echo
    std::vector<std::uint8_t> buffer(1024);
    auto [received, recv_ec] = client.receive(buffer);
    ASSERT_EQ(recv_ec, SocketErrc::Success);
    ASSERT_EQ(received, message.size());

    std::string echo(buffer.begin(), buffer.begin() + received);
    EXPECT_EQ(echo, message);

    test_done.store(true);
    server_thread.join();
}
