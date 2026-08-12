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

TEST(TcpServerTest, CompletedConnectionIsReapedWhileServerRuns) {
    CountingHandler h;
    TcpServer srv(h);
    const int port = find_free_port();
    ASSERT_GT(port, 0);
    ServerConfig cfg;
    cfg.port = static_cast<std::uint16_t>(port);
    cfg.worker_thread_count = 1;
    ASSERT_EQ(srv.start(cfg), NetError::Ok);

    SOCKET csock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ASSERT_NE(csock, INVALID_SOCKET);
    sockaddr_in caddr{};
    caddr.sin_family = AF_INET;
    caddr.sin_port = htons(static_cast<u_short>(port));
    caddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    ASSERT_EQ(connect(csock, reinterpret_cast<sockaddr*>(&caddr), sizeof(caddr)), 0);
    for (int i = 0; i < 50 && srv.connection_count() == 0; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    ASSERT_EQ(srv.connection_count(), 1u);

    closesocket(csock);
    for (int i = 0; i < 100 && srv.connection_count() != 0; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    EXPECT_EQ(srv.connection_count(), 0u);
    EXPECT_EQ(h.disconnects.load(), 1u);
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

TEST(TcpClientTest, RetriesTransientConnectFailureUntilTimeout) {
    CountingHandler h;
    TcpServer server(h);
    const int port = find_free_port();
    ASSERT_GT(port, 0);
    ServerConfig server_cfg;
    server_cfg.port = static_cast<std::uint16_t>(port);
    server_cfg.worker_thread_count = 1;

    std::thread delayed_start([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        EXPECT_EQ(server.start(server_cfg), NetError::Ok);
    });

    TcpClient client(h);
    ClientConfig client_cfg;
    client_cfg.remote_address = "127.0.0.1";
    client_cfg.port = static_cast<std::uint16_t>(port);
    client_cfg.connect_timeout = std::chrono::milliseconds(1000);
    EXPECT_EQ(client.connect(client_cfg), NetError::Ok);
    delayed_start.join();
    client.disconnect();
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

// ============================================================================
// Phase 4.4: Encryption integration tests.
// ============================================================================

// Simple XOR "encryptor" for testing. NOT secure — just verifies the
// encrypt/decrypt hooks are called correctly.
class XorEncryptor : public IEncryptor {
public:
    explicit XorEncryptor(std::uint8_t key = 0xA5) : key_(key) {}

    NetError encrypt(std::span<std::uint8_t> data) override {
        encrypt_count_.fetch_add(1);
        for (auto& b : data) b ^= key_;
        return NetError::Ok;
    }

    NetError decrypt(std::span<std::uint8_t> data) override {
        decrypt_count_.fetch_add(1);
        for (auto& b : data) b ^= key_;
        return NetError::Ok;
    }

    void seed() override { seed_count_.fetch_add(1); }

    std::atomic<int> encrypt_count_{0};
    std::atomic<int> decrypt_count_{0};
    std::atomic<int> seed_count_{0};

private:
    std::uint8_t key_;
};

// Handler that provides a XorEncryptor for each connection.
struct EncryptedHandler : IConnectionHandler {
    XorEncryptor encryptor;
    std::atomic<std::size_t> connects{0};
    std::atomic<std::size_t> messages{0};
    std::vector<Message> received;
    std::mutex mu;
    ConnectionId last_cid{0};

    bool on_connect(ConnectionId id, const std::string&) override {
        connects.fetch_add(1);
        last_cid = id;
        return true;
    }

    void on_message(ConnectionId id, const Message& msg) override {
        messages.fetch_add(1);
        std::lock_guard<std::mutex> lk(mu);
        received.push_back(msg);
        last_cid = id;
    }

    void on_disconnect(ConnectionId, NetError) override {}

    IEncryptor* encryptor_for(ConnectionId) override {
        return &encryptor;
    }

    ConnectionId last_id() const { return last_cid; }
};

TEST(EncryptionIntegration, ServerEncryptsOutgoingMessages) {
    EncryptedHandler sh;
    TcpServer server(sh);
    int port = find_free_port();
    ASSERT_GT(port, 0);
    ServerConfig cfg;
    cfg.port = static_cast<std::uint16_t>(port);
    cfg.worker_thread_count = 1;
    NetError err = server.start(cfg);
    ASSERT_EQ(err, NetError::Ok);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Connect a raw socket (no decryption on client side).
    SOCKET csock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in caddr{};
    caddr.sin_family = AF_INET;
    caddr.sin_port = htons(static_cast<u_short>(port));
    caddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    connect(csock, reinterpret_cast<sockaddr*>(&caddr), sizeof(caddr));

    for (int i = 0; i < 50 && sh.connects.load() == 0; ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    ASSERT_EQ(sh.connects.load(), 1u);
    ConnectionId client_id = sh.last_id();

    // Server sends a message — it should be XOR-encrypted on the wire.
    Message reply;
    reply.header.category = 0x02;
    reply.header.protocol = 0x03;
    reply.payload = {0x11, 0x22, 0x33};
    NetError send_err = server.send(client_id, reply);
    EXPECT_EQ(send_err, NetError::Ok);

    // Read raw bytes from socket — they should be XOR'd.
    std::uint8_t recv_buf[64] = {};
    int n = recv(csock, reinterpret_cast<char*>(recv_buf), sizeof(recv_buf), 0);
    EXPECT_GE(n, 10);

    // The payload bytes should NOT match the original (they're XOR'd).
    // After XOR with 0xA5: 0x11^0xA5=0xB4, 0x22^0xA5=0x87, 0x33^0xA5=0x96
    // Note: header is also encrypted, so we check payload offset.
    bool found_encrypted = false;
    for (int i = 0; i < n - 2; ++i) {
        if (recv_buf[i] == 0xB4 && recv_buf[i+1] == 0x87 && recv_buf[i+2] == 0x96) {
            found_encrypted = true;
            break;
        }
    }
    EXPECT_TRUE(found_encrypted) << "encrypted payload not found on wire";
    EXPECT_GE(sh.encryptor.encrypt_count_.load(), 1);

    closesocket(csock);
    server.stop();
}

TEST(EncryptionIntegration, ServerDecryptsIncomingMessages) {
    EncryptedHandler sh;
    TcpServer server(sh);
    int port = find_free_port();
    ASSERT_GT(port, 0);
    ServerConfig cfg;
    cfg.port = static_cast<std::uint16_t>(port);
    cfg.worker_thread_count = 1;
    cfg.recv_buffer_size = 4096;
    NetError err = server.start(cfg);
    ASSERT_EQ(err, NetError::Ok);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Connect raw socket and send XOR-encrypted data.
    SOCKET csock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in caddr{};
    caddr.sin_family = AF_INET;
    caddr.sin_port = htons(static_cast<u_short>(port));
    caddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    connect(csock, reinterpret_cast<sockaddr*>(&caddr), sizeof(caddr));

    for (int i = 0; i < 50 && sh.connects.load() == 0; ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    ASSERT_EQ(sh.connects.load(), 1u);

    // Build a message, then XOR-encrypt it before sending.
    MsgHeader hdr{};
    hdr.category = 0x01;
    hdr.protocol = 0x05;
    std::uint8_t pkt[10] = {};
    std::memcpy(pkt, &hdr, sizeof(hdr));
    pkt[8] = 0xAB;
    pkt[9] = 0xCD;

    // XOR-encrypt the entire packet.
    for (int i = 0; i < 10; ++i) pkt[i] ^= 0xA5;

    int sent = send(csock, reinterpret_cast<const char*>(pkt), sizeof(pkt), 0);
    EXPECT_EQ(sent, 10);

    // Wait for server to receive and decrypt.
    for (int i = 0; i < 50 && sh.messages.load() == 0; ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    EXPECT_EQ(sh.messages.load(), 1u);

    {
        std::lock_guard<std::mutex> lk(sh.mu);
        ASSERT_EQ(sh.received.size(), 1u);
        // After decryption, payload should match original.
        EXPECT_EQ(sh.received[0].payload.size(), 2u);
        EXPECT_EQ(sh.received[0].payload[0], 0xAB);
        EXPECT_EQ(sh.received[0].payload[1], 0xCD);
    }
    EXPECT_GE(sh.encryptor.decrypt_count_.load(), 1);

    closesocket(csock);
    server.stop();
}

TEST(EncryptionIntegration, TcpClientSendUsesEncryptor) {
    // Set up server.
    EncryptedHandler sh;
    TcpServer server(sh);
    int port = find_free_port();
    ASSERT_GT(port, 0);
    ServerConfig cfg;
    cfg.port = static_cast<std::uint16_t>(port);
    cfg.worker_thread_count = 1;
    NetError err = server.start(cfg);
    ASSERT_EQ(err, NetError::Ok);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Set up encrypted client.
    EncryptedHandler ch;
    TcpClient client(ch);
    ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port);
    NetError cerr = client.connect(ccfg);
    ASSERT_EQ(cerr, NetError::Ok);

    for (int i = 0; i < 50 && sh.connects.load() == 0; ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    ASSERT_EQ(sh.connects.load(), 1u);

    // Client sends a message — should be encrypted by client's encryptor.
    Message msg;
    msg.header.category = 0x07;
    msg.header.protocol = 0x01;
    msg.payload = {0xDE, 0xAD};
    NetError send_err = client.send(msg);
    EXPECT_EQ(send_err, NetError::Ok);
    EXPECT_GE(ch.encryptor.encrypt_count_.load(), 1);

    // Server should receive and decrypt the message.
    for (int i = 0; i < 50 && sh.messages.load() == 0; ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    EXPECT_EQ(sh.messages.load(), 1u);

    {
        std::lock_guard<std::mutex> lk(sh.mu);
        ASSERT_EQ(sh.received.size(), 1u);
        EXPECT_EQ(sh.received[0].payload.size(), 2u);
        EXPECT_EQ(sh.received[0].payload[0], 0xDE);
        EXPECT_EQ(sh.received[0].payload[1], 0xAD);
    }

    client.disconnect();
    server.stop();
}

}  // namespace mxh::net
