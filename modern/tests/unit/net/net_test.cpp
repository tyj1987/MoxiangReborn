// net_test.cpp — TcpServer and TcpClient unit tests.
//
// Phase 4: tests the async TCP server/client without requiring a real
// connection to an external endpoint. Uses localhost ephemeral ports.

// Windows sockets MUST be first — before any standard library or gtest headers
// that might indirectly include <windows.h> (which conflicts with winsock2.h).
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #ifndef socklen_t
        using socklen_t = int;
    #endif
#endif

#include "mxh/net/net.hpp"

#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>

namespace {

int find_free_port() {
    SOCKET tmp = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (tmp == INVALID_SOCKET) return 0;
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    socklen_t len = sizeof(addr);
    if (bind(tmp, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        closesocket(tmp);
        return 0;
    }
    if (getsockname(tmp, reinterpret_cast<sockaddr*>(&addr), &len) != 0) {
        closesocket(tmp);
        return 0;
    }
    int port = ntohs(addr.sin_port);
    closesocket(tmp);
    return port;
}

}  // namespace

namespace mxh::net {

// Trivial handler that tracks connect/disconnect counts.
struct CountingHandler : IConnectionHandler {
    std::atomic<std::size_t> connects{0};
    std::atomic<std::size_t> disconnects{0};
    std::atomic<std::size_t> messages{0};
    std::vector<Message> received;
    std::mutex mu;

    bool on_connect(ConnectionId id, const std::string& addr) override {
        connects.fetch_add(1);
        last_id_ = id;
        last_addr_ = addr;
        return true;
    }

    void on_message(ConnectionId id, const Message& msg) override {
        messages.fetch_add(1);
        std::lock_guard<std::mutex> lk(mu);
        received.push_back(msg);
        last_id_ = id;
    }

    void on_disconnect(ConnectionId id, NetError reason) override {
        (void)id;
        (void)reason;
        disconnects.fetch_add(1);
    }

    ConnectionId last_id() const { return last_id_; }
    std::string last_addr() const { return last_addr_; }

private:
    ConnectionId last_id_{0};
    std::string last_addr_;
};

TEST(TcpServerTest, NotRunningBeforeStart) {
    CountingHandler h;
    TcpServer srv(h);
    EXPECT_FALSE(srv.is_running());
    EXPECT_EQ(srv.connection_count(), 0u);
}

TEST(TcpServerTest, StartSucceeds) {
    CountingHandler h;
    TcpServer srv(h);
    int port = find_free_port();
    ASSERT_GT(port, 0) << "could not find free port";

    ServerConfig cfg;
    cfg.port = static_cast<std::uint16_t>(port);
    cfg.worker_thread_count = 1;
    cfg.recv_buffer_size = 4096;
    NetError err = srv.start(cfg);
    EXPECT_EQ(err, NetError::Ok) << "server start failed: " << to_string(err);
    EXPECT_TRUE(srv.is_running());
    srv.stop();
    EXPECT_FALSE(srv.is_running());
}

TEST(TcpServerTest, StartTwiceReturnsAlreadyStarted) {
    CountingHandler h;
    TcpServer srv(h);
    int port = find_free_port();
    ServerConfig cfg;
    cfg.port = static_cast<std::uint16_t>(port);
    cfg.worker_thread_count = 1;
    NetError first = srv.start(cfg);
    EXPECT_EQ(first, NetError::Ok);
    NetError second = srv.start(cfg);
    EXPECT_EQ(second, NetError::AlreadyStarted);
    srv.stop();
}

TEST(TcpServerTest, ClientConnects) {
    CountingHandler h;
    TcpServer srv(h);
    int port = find_free_port();
    ASSERT_GT(port, 0);
    ServerConfig cfg;
    cfg.port = static_cast<std::uint16_t>(port);
    cfg.worker_thread_count = 1;
    NetError err = srv.start(cfg);
    ASSERT_EQ(err, NetError::Ok);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    SOCKET csock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ASSERT_NE(csock, INVALID_SOCKET);
    struct sockaddr_in caddr{};
    caddr.sin_family = AF_INET;
    caddr.sin_port = htons(static_cast<u_short>(port));
    caddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    int conn_ok = connect(csock, reinterpret_cast<sockaddr*>(&caddr), sizeof(caddr));
    ASSERT_EQ(conn_ok, 0) << "client connect failed";

    for (int i = 0; i < 50 && h.connects.load() == 0; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    EXPECT_EQ(h.connects.load(), 1u) << "server did not see connection";

    closesocket(csock);
    srv.stop();
}

TEST(TcpServerTest, ClientSendsMessage) {
    CountingHandler h;
    TcpServer srv(h);
    int port = find_free_port();
    ASSERT_GT(port, 0);
    ServerConfig cfg;
    cfg.port = static_cast<std::uint16_t>(port);
    cfg.worker_thread_count = 1;
    cfg.recv_buffer_size = 4096;
    NetError err = srv.start(cfg);
    ASSERT_EQ(err, NetError::Ok);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    SOCKET csock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ASSERT_NE(csock, INVALID_SOCKET);
    struct sockaddr_in caddr{};
    caddr.sin_family = AF_INET;
    caddr.sin_port = htons(static_cast<u_short>(port));
    caddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    connect(csock, reinterpret_cast<sockaddr*>(&caddr), sizeof(caddr));

    for (int i = 0; i < 50 && h.connects.load() == 0; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    ASSERT_EQ(h.connects.load(), 1u);

    MsgHeader hdr{};
    hdr.category = 0x01;
    hdr.protocol = 0x05;
    std::uint8_t pkt[10] = {};
    std::memcpy(pkt, &hdr, sizeof(hdr));
    pkt[8] = 0xAB;
    pkt[9] = 0xCD;

    int sent = send(csock, reinterpret_cast<const char*>(pkt), sizeof(pkt), 0);
    EXPECT_EQ(sent, static_cast<int>(sizeof(pkt)));

    for (int i = 0; i < 50 && h.messages.load() == 0; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    EXPECT_EQ(h.messages.load(), 1u);

    {
        std::lock_guard<std::mutex> lk(h.mu);
        ASSERT_EQ(h.received.size(), 1u);
        EXPECT_EQ(h.received[0].payload.size(), 2u);
        EXPECT_EQ(h.received[0].payload[0], 0xAB);
        EXPECT_EQ(h.received[0].payload[1], 0xCD);
    }

    closesocket(csock);
    srv.stop();
}

TEST(TcpServerTest, ServerSendsToClient) {
    CountingHandler h;
    TcpServer srv(h);
    int port = find_free_port();
    ASSERT_GT(port, 0);
    ServerConfig cfg;
    cfg.port = static_cast<std::uint16_t>(port);
    cfg.worker_thread_count = 1;
    cfg.recv_buffer_size = 4096;
    NetError err = srv.start(cfg);
    ASSERT_EQ(err, NetError::Ok);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    SOCKET csock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in caddr{};
    caddr.sin_family = AF_INET;
    caddr.sin_port = htons(static_cast<u_short>(port));
    caddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    connect(csock, reinterpret_cast<sockaddr*>(&caddr), sizeof(caddr));

    for (int i = 0; i < 50 && h.connects.load() == 0; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    ASSERT_EQ(h.connects.load(), 1u);
    ConnectionId client_id = h.last_id();

    MsgHeader hdr{};
    hdr.category = 0x02;
    hdr.protocol = 0x03;
    Message reply;
    reply.header = hdr;
    reply.payload = {0x11, 0x22, 0x33};
    NetError send_err = srv.send(client_id, reply);
    EXPECT_EQ(send_err, NetError::Ok);

    std::uint8_t recv_buf[64] = {};
    int n = recv(csock, reinterpret_cast<char*>(recv_buf), sizeof(recv_buf), 0);
    EXPECT_GE(n, 10);

    MsgHeader rhdr{};
    std::memcpy(&rhdr, recv_buf, sizeof(MsgHeader));
    EXPECT_EQ(rhdr.category, 0x02);
    EXPECT_EQ(rhdr.protocol, 0x03);

    closesocket(csock);
    srv.stop();
}

TEST(TcpServerTest, StopClosesAllConnections) {
    CountingHandler h;
    TcpServer srv(h);
    int port = find_free_port();
    ASSERT_GT(port, 0);
    ServerConfig cfg;
    cfg.port = static_cast<std::uint16_t>(port);
    cfg.worker_thread_count = 1;
    (void)srv.start(cfg);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    SOCKET csock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in caddr{};
    caddr.sin_family = AF_INET;
    caddr.sin_port = htons(static_cast<u_short>(port));
    caddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    connect(csock, reinterpret_cast<sockaddr*>(&caddr), sizeof(caddr));
    for (int i = 0; i < 50 && h.connects.load() == 0; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    ASSERT_EQ(h.connects.load(), 1u);

    srv.stop();
    EXPECT_FALSE(srv.is_running());

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_GE(h.disconnects.load(), 1u);

    closesocket(csock);
}

TEST(TcpClientTest, ConnectSucceeds) {
    CountingHandler sh;
    TcpServer server(sh);
    int port = find_free_port();
    ASSERT_GT(port, 0);
    ServerConfig cfg;
    cfg.port = static_cast<std::uint16_t>(port);
    cfg.worker_thread_count = 1;
    NetError err = server.start(cfg);
    ASSERT_EQ(err, NetError::Ok);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    TcpClient cli(sh);
    ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port);
    NetError cerr = cli.connect(ccfg);
    EXPECT_EQ(cerr, NetError::Ok);
    EXPECT_TRUE(cli.is_connected());

    cli.disconnect();
    EXPECT_FALSE(cli.is_connected());

    server.stop();
}

TEST(TcpClientTest, DisconnectWhenNotConnected) {
    CountingHandler h;
    TcpClient cli(h);
    cli.disconnect();
    EXPECT_FALSE(cli.is_connected());
}

TEST(TcpServerTest, SendToUnknownIdReturnsDisconnected) {
    CountingHandler h;
    TcpServer srv(h);
    MsgHeader hdr{};
    Message m{hdr, {0x01}};
    NetError err = srv.send(ConnectionId{9999}, m);
    EXPECT_EQ(err, NetError::Disconnected);
}

TEST(NetErrorTest, ToStringReturnsNonNull) {
    EXPECT_STREQ(to_string(NetError::Ok), "Ok");
    EXPECT_STREQ(to_string(NetError::BindFailed), "BindFailed");
    EXPECT_STREQ(to_string(NetError::ListenFailed), "ListenFailed");
    EXPECT_STREQ(to_string(NetError::ConnectFailed), "ConnectFailed");
    EXPECT_STREQ(to_string(NetError::SendFailed), "SendFailed");
    EXPECT_STREQ(to_string(NetError::RecvFailed), "RecvFailed");
    EXPECT_STREQ(to_string(NetError::EncryptionFailed), "EncryptionFailed");
    EXPECT_STREQ(to_string(NetError::DecryptionFailed), "DecryptionFailed");
    EXPECT_STREQ(to_string(NetError::Unknown), "Unknown");
}

TEST(TcpServerTest, BroadcastNoConnections) {
    CountingHandler h;
    TcpServer srv(h);
    MsgHeader hdr{};
    Message m{hdr, {0x01}};
    NetError err = srv.broadcast(m);
    EXPECT_EQ(err, NetError::Ok);
}

TEST(TcpServerTest, DisconnectUnknownId) {
    CountingHandler h;
    TcpServer srv(h);
    srv.disconnect(ConnectionId{12345});
    EXPECT_EQ(srv.connection_count(), 0u);
}

}  // namespace mxh::net
