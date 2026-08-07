// agent_wire_first_packet_test.cpp
//
// E2 T2 wire-bytes verification for AgentServer first packet.
// Spins up an AgentHandler in legacy framing mode, connects a raw
// TCP socket, and captures the first wire packet (AgentConnectSuccess).
// Verifies:
//   - Header category is UserConn (legacy cat=7).
//   - Header protocol is AgentConnectSuccess (legacy proto=8).
//   - object_id (auth_key) is in the legacy range [50000, 99999].
//   - First 4 wire bytes (checksum + code + cat + proto) are
//     byte-stable across connections.
//
// Per ROADMAP.md E2 T2: this is the AgentServer half of the wire
// SHA-256 capture. The LoginServer half is in login_wire_sha256_test.cpp.
// The auth_key portion is randomized (1:1 with legacy [Server]Agent/
// AgentNetworkMsgParser.cpp) and is captured separately so the
// regression anchor is purely on the deterministic portion of the
// wire bytes.

#include "mxh/net/net.hpp"
#include "mxh/server/server.hpp"
#include "mxh/db/db_adapter.hpp"
#include "mxh/db/sqlite_adapter.hpp"
#include "mxh/proto/protocol.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
    #include <winsock2.h>
#endif

namespace {

using mxh::net::ConnectionId;
using mxh::net::Message;
using mxh::net::NetError;
using mxh::net::ServerConfig;
using mxh::net::TcpServer;
using mxh::proto::Category;
using mxh::proto::UserConnProtocol;

constexpr std::uint16_t kAgentWirePort = 54323;

// Connect a raw TCP socket to localhost:port, return the socket.
SOCKET connect_localhost(std::uint16_t port) {
    SOCKET s = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) return INVALID_SOCKET;
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(port);
    if (::connect(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        ::closesocket(s);
        return INVALID_SOCKET;
    }
    return s;
}

// Read N bytes from a socket with a timeout (returns whatever was
// available within the timeout, possibly < N if the peer closed).
std::vector<std::uint8_t> recv_n(SOCKET sock, std::size_t n,
                                 std::chrono::milliseconds timeout) {
    std::vector<std::uint8_t> out;
    out.reserve(n);
    auto deadline = std::chrono::steady_clock::now() + timeout;
    while (out.size() < n) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(sock, &fds);
        timeval tv{};
        auto now = std::chrono::steady_clock::now();
        if (now >= deadline) break;
        auto remaining = std::chrono::duration_cast<std::chrono::microseconds>(
            deadline - now);
        tv.tv_sec = static_cast<long>(remaining.count() / 1000000);
        tv.tv_usec = static_cast<long>(remaining.count() % 1000000);
        int rc = ::select(0, &fds, nullptr, nullptr, &tv);
        if (rc <= 0) break;
        std::uint8_t buf[64];
        int got = ::recv(sock, reinterpret_cast<char*>(buf),
                         static_cast<int>(std::min<std::size_t>(n - out.size(), 64)),
                         0);
        if (got <= 0) break;
        out.insert(out.end(), buf, buf + got);
    }
    return out;
}

class AgentWireFixture : public ::testing::Test {
protected:
    void SetUp() override {
        auto tmp = std::filesystem::temp_directory_path();
        db_path_ = (tmp / (std::string("agent_wire_") +
            std::to_string(static_cast<long long>(::GetCurrentProcessId())) +
            "_" + std::to_string(test_id_++) + ".db"))
            .string();
        std::remove(db_path_.c_str());

        db_ = mxh::db::make_adapter("sqlite");
        ASSERT_NE(db_, nullptr);
        mxh::db::ConnectionConfig cfg;
        cfg.backend = "sqlite";
        cfg.path = db_path_;
        ASSERT_TRUE(db_->connect(cfg));

        std::string schema =
            "CREATE TABLE IF NOT EXISTS chr_log_info ("
            " id TEXT PRIMARY KEY, pw TEXT NOT NULL,"
            " userlevel INTEGER NOT NULL DEFAULT 0);"
            "CREATE TABLE IF NOT EXISTS character_info ("
            " char_idx INTEGER PRIMARY KEY, user_id INTEGER NOT NULL,"
            " char_name TEXT NOT NULL);";
        auto* sa = static_cast<mxh::db::SqliteAdapter*>(db_.get());
        ASSERT_TRUE(sa->exec_multi(schema));

        // The AgentHandler replies via its reply_ callback. We capture
        // those replies into a buffer so we can route them through the
        // TcpServer (which owns the actual connection).
        handler_ = std::make_unique<mxh::server::AgentHandler>(
            *db_,
            [this](ConnectionId id, const Message& m) {
                std::lock_guard<std::mutex> lk(replies_mu_);
                replies_[id.value].push_back(m);
            },
            /*use_legacy_framing=*/true);

        server_ = std::make_unique<TcpServer>(*handler_);
        ServerConfig scfg;
        scfg.bind_address = "127.0.0.1";
        scfg.port = kAgentWirePort;
        scfg.use_legacy_framing = true;
        scfg.idle_timeout = std::chrono::milliseconds{5000};
        auto sr = server_->start(scfg);
        ASSERT_EQ(sr, NetError::Ok) << mxh::net::to_string(sr);

        // Drain thread: route handler replies to the TcpServer.
        drain_running_.store(true);
        drain_thread_ = std::thread([this]{
            while (drain_running_.load()) {
                std::unordered_map<std::uint64_t, std::vector<Message>> batch;
                {
                    std::lock_guard<std::mutex> lk(replies_mu_);
                    batch.swap(replies_);
                }
                for (auto& kv : batch) {
                    ConnectionId id{static_cast<std::uint32_t>(kv.first)};
                    for (auto& m : kv.second) {
                        server_->send(id, m);
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds{10});
            }
        });

        std::this_thread::sleep_for(std::chrono::milliseconds{50});
    }

    void TearDown() override {
        drain_running_.store(false);
        if (drain_thread_.joinable()) drain_thread_.join();
        if (server_) server_->stop();
        handler_.reset();
        server_.reset();
        db_.reset();
        std::remove(db_path_.c_str());
    }

    static std::atomic<int> test_id_;
    std::string db_path_;
    std::unique_ptr<mxh::db::IDbAdapter> db_;
    std::unique_ptr<mxh::server::AgentHandler> handler_;
    std::unique_ptr<TcpServer> server_;
    std::thread drain_thread_;
    std::atomic<bool> drain_running_{false};
    std::mutex replies_mu_;
    std::unordered_map<std::uint64_t, std::vector<Message>> replies_;
};

std::atomic<int> AgentWireFixture::test_id_{0};

}  // namespace

TEST_F(AgentWireFixture, FirstPacketHeaderIsAgentConnectSuccess) {
    SOCKET s = connect_localhost(kAgentWirePort);
    ASSERT_NE(s, INVALID_SOCKET);
    // Legacy framing adds a 2-byte length prefix before the MsgHeader,
    // so the full first wire frame is 2 + 8 = 10 bytes.
    auto bytes = recv_n(s, /*n=*/10, std::chrono::milliseconds{2000});
    ::closesocket(s);

    ASSERT_EQ(bytes.size(), 10u);
    // bytes[0..1]: 2-byte length prefix (little-endian, == 8)
    EXPECT_EQ(bytes[0], 8u);
    EXPECT_EQ(bytes[1], 0u);
    // bytes[2..9]: MsgHeader [checksum:1][code:1][category:1][protocol:1][object_id:4]
    EXPECT_EQ(bytes[2], 0u);  // checksum (default-init)
    EXPECT_EQ(static_cast<std::int8_t>(bytes[3]), 0);  // code (default-init)
    EXPECT_EQ(bytes[4], static_cast<std::uint8_t>(Category::UserConn));
    EXPECT_EQ(bytes[5], static_cast<std::uint8_t>(UserConnProtocol::AgentConnectSuccess));

    // object_id is little-endian per the legacy MsgHeader packing.
    std::uint32_t auth_key =
        static_cast<std::uint32_t>(bytes[6]) |
        (static_cast<std::uint32_t>(bytes[7]) << 8) |
        (static_cast<std::uint32_t>(bytes[8]) << 16) |
        (static_cast<std::uint32_t>(bytes[9]) << 24);
    EXPECT_GE(auth_key, 50000u);
    EXPECT_LE(auth_key, 99999u);
}

TEST_F(AgentWireFixture, DeterministicHeaderPrefixIsStableAcrossConnections) {
    // Connect twice and capture the first 6 wire bytes each time. The
    // 2-byte length prefix + 4-byte MsgHeader prefix (checksum + code +
    // cat + proto) are deterministic, so the prefix must match across
    // connections. The 4-byte auth_key object_id is randomized.
    std::vector<std::uint8_t> first_prefix;
    std::vector<std::uint8_t> second_prefix;
    std::uint32_t first_auth_key = 0;
    std::uint32_t second_auth_key = 0;

    for (int i = 0; i < 2; ++i) {
        SOCKET s = connect_localhost(kAgentWirePort);
        ASSERT_NE(s, INVALID_SOCKET);
        auto bytes = recv_n(s, 10, std::chrono::milliseconds{2000});
        ::closesocket(s);
        ASSERT_EQ(bytes.size(), 10u);
        if (i == 0) {
            // bytes[0..1] length prefix; bytes[2..5] header prefix
            first_prefix.assign(bytes.begin(), bytes.begin() + 6);
            first_auth_key =
                static_cast<std::uint32_t>(bytes[6]) |
                (static_cast<std::uint32_t>(bytes[7]) << 8) |
                (static_cast<std::uint32_t>(bytes[8]) << 16) |
                (static_cast<std::uint32_t>(bytes[9]) << 24);
        } else {
            second_prefix.assign(bytes.begin(), bytes.begin() + 6);
            second_auth_key =
                static_cast<std::uint32_t>(bytes[6]) |
                (static_cast<std::uint32_t>(bytes[7]) << 8) |
                (static_cast<std::uint32_t>(bytes[8]) << 16) |
                (static_cast<std::uint32_t>(bytes[9]) << 24);
        }
    }

    // First 6 wire bytes (length prefix + checksum + code + cat + proto)
    // must match across connections.
    EXPECT_EQ(first_prefix, second_prefix);
    // The expected prefix is [0x08, 0x00, 0x00, 0x00, 0x07, 0x08].
    // (length=8 little-endian, checksum=0, code=0, UserConn, AgentConnectSuccess)
    const std::vector<std::uint8_t> kExpectedPrefix = {
        0x08, 0x00, 0x00, 0x00, 0x07, 0x08,
    };
    EXPECT_EQ(first_prefix, kExpectedPrefix);
    // auth_keys are randomized (legacy [Server]Agent/) so they should
    // differ across connections; we only assert they're in range.
    EXPECT_GE(first_auth_key, 50000u);
    EXPECT_LE(first_auth_key, 99999u);
    EXPECT_GE(second_auth_key, 50000u);
    EXPECT_LE(second_auth_key, 99999u);
}
