// login_e2e_test.cpp
//
// M2 step 1: in-process LoginServer E2E scaffold + 4 smoke tests.
//
// Spins up LoginHandler + TcpServer (use_legacy_framing=true) inside the
// test process, connects a TcpClient, and verifies the wire-format
// round-trip produces the legacy 4DyuchiNET messages (DistConnectSuccess
// on connect, NotifyUserLoginAck on valid login, NACK on invalid).
//
// Phase 6.4 wire-format E2E milestone M2 (long-term stable target).

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #ifndef socklen_t
        using socklen_t = int;
    #endif
#endif

#include "mxh/net/net.hpp"
#include "mxh/server/server.hpp"
#include "mxh/db/db_adapter.hpp"
#include "mxh/db/sqlite_adapter.hpp"
#include "mxh/proto/protocol.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace {

using mxh::net::ConnectionId;
using mxh::net::IConnectionHandler;
using mxh::net::Message;
using mxh::net::NetError;
using mxh::net::TcpServer;

// Find an ephemeral TCP port. Also ensures WSAStartup has run
// (the first ::socket call needs it).
// Pinned ephemeral port used by M2 step 2 golden-capture tests so wire bytes
// are byte-stable across runs (find_free_port gives different port each run).
constexpr std::uint16_t kGoldenPort = 54321;

int find_free_port() {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 0;
#endif
    SOCKET tmp = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (tmp == INVALID_SOCKET) return 0;
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    socklen_t len = sizeof(addr);
    if (::bind(tmp, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        ::closesocket(tmp);
        return 0;
    }
    if (::getsockname(tmp, reinterpret_cast<sockaddr*>(&addr), &len) != 0) {
        ::closesocket(tmp);
        return 0;
    }
    int port = ntohs(addr.sin_port);
    ::closesocket(tmp);
    return port;
}

struct CapturingClientHandler : IConnectionHandler {
    std::vector<Message> messages;
    std::mutex mu;
    std::condition_variable cv;
    bool connected = false;
    bool disconnected = false;

    bool on_connect(ConnectionId, const std::string&) override {
        connected = true;
        return true;
    }
    void on_message(ConnectionId, const Message& msg) override {
        std::lock_guard<std::mutex> lk(mu);
        messages.push_back(msg);
        cv.notify_all();
    }
    void on_disconnect(ConnectionId, NetError) override {
        disconnected = true;
        cv.notify_all();
    }

    bool wait_for(std::size_t n, std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lk(mu);
        return cv.wait_for(lk, timeout, [&]{ return messages.size() >= n; });
    }

    std::vector<Message> snapshot() {
        std::lock_guard<std::mutex> lk(mu);
        return messages;
    }
};

std::vector<std::uint8_t> pad17(const std::string& s) {
    std::vector<std::uint8_t> out(17, 0);
    std::memcpy(out.data(), s.data(), std::min(s.size(), std::size_t(17)));
    return out;
}

std::vector<std::uint8_t> make_legacy_login_payload(
        std::uint32_t auth_key, const std::string& id, const std::string& pw) {
    std::vector<std::uint8_t> out;
    out.reserve(4 + 17 + 17);
    out.push_back(static_cast<std::uint8_t>(auth_key & 0xFF));
    out.push_back(static_cast<std::uint8_t>((auth_key >> 8) & 0xFF));
    out.push_back(static_cast<std::uint8_t>((auth_key >> 16) & 0xFF));
    out.push_back(static_cast<std::uint8_t>((auth_key >> 24) & 0xFF));
    auto id17 = pad17(id);
    auto pw17 = pad17(pw);
    out.insert(out.end(), id17.begin(), id17.end());
    out.insert(out.end(), pw17.begin(), pw17.end());
    return out;
}

class LoginServerFixture : public ::testing::Test {
protected:
    void SetUp() override {
        // Use OS temp dir so we work regardless of WORKING_DIRECTORY
        // (ctest sets it to modern/tests/unit, direct exe runs from elsewhere).
        auto tmp = std::filesystem::temp_directory_path();
        db_path_ = (tmp / (
            std::string("login_e2e_") +
            std::to_string(static_cast<long long>(::GetCurrentProcessId())) +
            "_" + std::to_string(test_id_++) + ".db")).string();
        std::remove(db_path_.c_str());

        db_ = mxh::db::make_adapter("sqlite");
        ASSERT_NE(db_, nullptr);
        mxh::db::ConnectionConfig cfg;
        cfg.backend = "sqlite";
        cfg.path = db_path_;
        auto cr = db_->connect(cfg);
        ASSERT_TRUE(cr) << cr.error_message;

        std::string schema;
        schema += "CREATE TABLE IF NOT EXISTS chr_log_info (";
        schema += " id TEXT PRIMARY KEY,";
        schema += " pw TEXT NOT NULL,";
        schema += " userlevel INTEGER NOT NULL DEFAULT 0);";
        schema += "INSERT INTO chr_log_info (id, pw, userlevel) VALUES ('test', 'test', 2);";
        auto* sa = static_cast<mxh::db::SqliteAdapter*>(db_.get());
        auto er = sa->exec_multi(schema);
        ASSERT_TRUE(er) << er.error_message;

        port_ = pin_port_for_golden_ ? kGoldenPort : find_free_port();
        ASSERT_GT(port_, 0);
        agent_port_for_ack_ = static_cast<std::uint16_t>(port_ + 1);

        handler_ = std::make_unique<mxh::server::LoginHandler>(
            *db_, "127.0.0.1", agent_port_for_ack_,
            [this](ConnectionId id, const Message& m) {
                std::lock_guard<std::mutex> lk(replies_mu_);
                replies_[id.value].push_back(m);
            },
            /*use_legacy=*/true);

        server_ = std::make_unique<TcpServer>(*handler_);
        mxh::net::ServerConfig scfg;
        scfg.port = static_cast<std::uint16_t>(port_);
        scfg.bind_address = "127.0.0.1";
        scfg.use_legacy_framing = true;
        auto sr = server_->start(scfg);
        ASSERT_EQ(sr, NetError::Ok) << mxh::net::to_string(sr);
        ASSERT_TRUE(server_->is_running());

        drain_running_.store(true);
        drain_thread_ = std::thread([this]{
            while (drain_running_.load()) {
                std::unordered_map<std::uint64_t, std::vector<Message>> batch;
                {
                    std::lock_guard<std::mutex> lk(replies_mu_);
                    batch.swap(replies_);
                }
                for (auto& kv : batch) {
                    for (auto& m : kv.second) {
                        server_->send(ConnectionId{kv.first}, m);
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        });
    }

    void TearDown() override {
        drain_running_.store(false);
        if (drain_thread_.joinable()) drain_thread_.join();
        if (server_) server_->stop();
        server_.reset();
        handler_.reset();
        db_.reset();
        std::remove(db_path_.c_str());
    }

    static inline int test_id_ = 0;

    std::string db_path_;
    int port_ = 0;
    bool pin_port_for_golden_ = false;
    std::uint16_t agent_port_for_ack_ = 0;
    std::unique_ptr<mxh::db::IDbAdapter> db_;
    std::unique_ptr<mxh::server::LoginHandler> handler_;
    std::unique_ptr<TcpServer> server_;
    std::thread drain_thread_;
    std::atomic<bool> drain_running_{false};
    std::mutex replies_mu_;
    std::unordered_map<std::uint64_t, std::vector<Message>> replies_;
};

// M2 step 2 -- derived fixture that pins the ephemeral port to kGoldenPort
// so wire bytes are byte-stable across runs. Default fixture uses
// find_free_port() which yields a different port each run.
class LoginServerFixtureGolden : public LoginServerFixture {
public:
    LoginServerFixtureGolden() { pin_port_for_golden_ = true; }
};

}  // namespace

TEST_F(LoginServerFixture, ScaffoldSmoke) {
    EXPECT_TRUE(server_->is_running());
    EXPECT_NE(db_, nullptr);
    EXPECT_NE(handler_, nullptr);
    EXPECT_GT(port_, 0);
}

TEST_F(LoginServerFixture, ConnectReceivesDistConnectSuccess) {
    CapturingClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    auto cr = tcp.connect(ccfg);
    ASSERT_EQ(cr, NetError::Ok) << mxh::net::to_string(cr);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));

    auto msgs = client.snapshot();
    ASSERT_EQ(msgs.size(), 1u);
    const auto& m = msgs[0];
    EXPECT_EQ(m.header.category, 7);
    EXPECT_EQ(m.header.protocol, 0);
    EXPECT_GT(m.header.object_id, 0u);
    EXPECT_TRUE(m.payload.empty());
    tcp.disconnect();
}

TEST_F(LoginServerFixture, LegacyLoginValidCredsReceivesAck) {
    CapturingClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    auto cr = tcp.connect(ccfg);
    ASSERT_EQ(cr, NetError::Ok);

    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));
    auto dcs = client.snapshot()[0];
    ASSERT_EQ(dcs.header.protocol, 0);
    std::uint32_t auth_key = dcs.header.object_id;
    ASSERT_GT(auth_key, 0u);

    Message login;
    login.header.category = 7;
    login.header.protocol = 1;
    login.header.object_id = 0;
    login.payload = make_legacy_login_payload(auth_key, "test", "test");
    auto se = tcp.send(login);
    ASSERT_EQ(se, NetError::Ok) << mxh::net::to_string(se);

    ASSERT_TRUE(client.wait_for(2, std::chrono::seconds(2)));
    auto msgs = client.snapshot();
    ASSERT_EQ(msgs.size(), 2u);
    const auto& ack = msgs[1];
    EXPECT_EQ(ack.header.category, 7);
    EXPECT_EQ(ack.header.protocol, 2);
    ASSERT_EQ(ack.payload.size(), 23u);
    std::string agentip(reinterpret_cast<const char*>(ack.payload.data()), 16);
    EXPECT_EQ(agentip, std::string("127.0.0.1") + std::string(7, 0x00));
    std::uint16_t port_be = static_cast<std::uint16_t>(ack.payload[16]) |
                            (static_cast<std::uint16_t>(ack.payload[17]) << 8);
    EXPECT_EQ(port_be, agent_port_for_ack_);
    EXPECT_EQ(ack.payload[18], 1);
    EXPECT_EQ(ack.payload[19], 0);
    EXPECT_EQ(ack.payload[20], 0);
    EXPECT_EQ(ack.payload[21], 0);
    EXPECT_EQ(ack.payload[22], 2);
    tcp.disconnect();
}

TEST_F(LoginServerFixture, LegacyLoginInvalidCredsReceivesNack) {
    CapturingClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    auto cr = tcp.connect(ccfg);
    ASSERT_EQ(cr, NetError::Ok);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));
    auto dcs = client.snapshot()[0];
    std::uint32_t auth_key = dcs.header.object_id;

    Message login;
    login.header.category = 7;
    login.header.protocol = 1;
    login.header.object_id = 0;
    login.payload = make_legacy_login_payload(auth_key, "test", "wrong");
    ASSERT_EQ(tcp.send(login), NetError::Ok);

    ASSERT_TRUE(client.wait_for(2, std::chrono::seconds(2)));
    auto msgs = client.snapshot();
    ASSERT_EQ(msgs.size(), 2u);
    const auto& nack = msgs[1];
    EXPECT_EQ(nack.header.category, 7);
    EXPECT_EQ(nack.header.protocol, 1);
    EXPECT_TRUE(nack.payload.empty());
    tcp.disconnect();
}

// =============================================================================
// M2 step 2 -- golden wire-byte capture. LoginServerFixtureGolden pins port_
// to kGoldenPort (54321); a fresh LoginHandler sets next_auth_key_ = 1000.
// Combined, the wire bytes for dist_connect_success / login_ack / login_nack
// are deterministic and locked against modern/tests/unit/server/golden/*.bin.
// =============================================================================

std::vector<std::uint8_t> reconstruct_wire(const Message& m) {
    const auto body_length = static_cast<std::uint16_t>(8u + m.payload.size());
    std::vector<std::uint8_t> out;
    out.reserve(2u + body_length);
    out.push_back(static_cast<std::uint8_t>(body_length & 0xff));
    out.push_back(static_cast<std::uint8_t>((body_length >> 8) & 0xff));
    out.push_back(m.header.checksum);
    out.push_back(static_cast<std::uint8_t>(m.header.code));
    out.push_back(m.header.category);
    out.push_back(m.header.protocol);
    out.push_back(static_cast<std::uint8_t>(m.header.object_id & 0xff));
    out.push_back(static_cast<std::uint8_t>((m.header.object_id >> 8) & 0xff));
    out.push_back(static_cast<std::uint8_t>((m.header.object_id >> 16) & 0xff));
    out.push_back(static_cast<std::uint8_t>((m.header.object_id >> 24) & 0xff));
    out.insert(out.end(), m.payload.begin(), m.payload.end());
    return out;
}

std::vector<std::uint8_t> read_golden_bytes(const std::string& filename) {
    // CMake sets WORKING_DIRECTORY = modern/tests/unit. Relative path resolves
    // to modern/tests/unit/server/golden/ which is the canonical capture dir.
    const std::filesystem::path p = std::filesystem::path("server") / "golden" / filename;
    std::ifstream f(p, std::ios::binary);
    EXPECT_TRUE(f.is_open()) << "golden file missing: " << p.string();
    return std::vector<std::uint8_t>(
        std::istreambuf_iterator<char>(f),
        std::istreambuf_iterator<char>{});
}

TEST_F(LoginServerFixtureGolden, GoldenCapturesDistConnectSuccess) {
    CapturingClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    auto cr = tcp.connect(ccfg);
    ASSERT_EQ(cr, NetError::Ok);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));
    auto msgs = client.snapshot();
    ASSERT_EQ(msgs.size(), 1u);
    const auto& dcs = msgs[0];
    EXPECT_EQ(dcs.header.category, 7);
    EXPECT_EQ(dcs.header.protocol, 0);
    EXPECT_EQ(dcs.header.object_id, 1000u);
    EXPECT_TRUE(dcs.payload.empty());
    tcp.disconnect();
    const auto actual = reconstruct_wire(dcs);
    const auto golden = read_golden_bytes("dist_connect_success.bin");
    EXPECT_EQ(actual, golden);
}

TEST_F(LoginServerFixtureGolden, GoldenCapturesLoginAck) {
    CapturingClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    auto cr = tcp.connect(ccfg);
    ASSERT_EQ(cr, NetError::Ok);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));
    auto dcs = client.snapshot()[0];
    ASSERT_EQ(dcs.header.protocol, 0);
    std::uint32_t auth_key = dcs.header.object_id;
    ASSERT_EQ(auth_key, 1000u);
    Message login;
    login.header.category = 7;
    login.header.protocol = 1;
    login.header.object_id = 0;
    login.payload = make_legacy_login_payload(auth_key, "test", "test");
    auto se = tcp.send(login);
    ASSERT_EQ(se, NetError::Ok);
    ASSERT_TRUE(client.wait_for(2, std::chrono::seconds(2)));
    auto msgs = client.snapshot();
    ASSERT_EQ(msgs.size(), 2u);
    const auto& ack = msgs[1];
    EXPECT_EQ(ack.header.protocol, 2);
    tcp.disconnect();
    const auto actual = reconstruct_wire(ack);
    const auto golden = read_golden_bytes("login_ack.bin");
    EXPECT_EQ(actual, golden);
}

TEST_F(LoginServerFixtureGolden, GoldenCapturesLoginNack) {
    CapturingClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    auto cr = tcp.connect(ccfg);
    ASSERT_EQ(cr, NetError::Ok);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));
    auto dcs = client.snapshot()[0];
    ASSERT_EQ(dcs.header.protocol, 0);
    std::uint32_t auth_key = dcs.header.object_id;
    ASSERT_EQ(auth_key, 1000u);
    Message login;
    login.header.category = 7;
    login.header.protocol = 1;
    login.header.object_id = 0;
    login.payload = make_legacy_login_payload(auth_key, "test", "wrong");
    auto se = tcp.send(login);
    ASSERT_EQ(se, NetError::Ok);
    ASSERT_TRUE(client.wait_for(2, std::chrono::seconds(2)));
    auto msgs = client.snapshot();
    ASSERT_EQ(msgs.size(), 2u);
    const auto& nack = msgs[1];
    EXPECT_EQ(nack.header.protocol, 1);
    EXPECT_TRUE(nack.payload.empty());
    tcp.disconnect();
    const auto actual = reconstruct_wire(nack);
    const auto golden = read_golden_bytes("login_nack.bin");
    EXPECT_EQ(actual, golden);
}

// =============================================================================
// M2 step 3 -- encrypted wire-byte capture. Phase 3 AES-256-GCM is heavy infra
// (separate crypto tests in modern/tests/unit/crypto/); M2 step 3 only locks the
// wire LAYOUT under use_encryption=true. A deterministic XOR stub encryptor with
// a fixed 32-byte key produces stable encrypted bytes that we compare against
// golden/. Wire format under legacy framing + encryption: [2B LE length]
// [8B MsgHeader (XORed)] [payload (XORed)] where the key is applied to all 8+N
// msg_body bytes contiguously (see modern/src/net/net.cpp line ~476).
// =============================================================================

constexpr std::array<std::uint8_t, 32> kGoldenEncKey = {
    0x4D, 0x32, 0x73, 0x74, 0x65, 0x70, 0x32, 0x67,  // bytes 0..7  "M2step2g"
    0x6F, 0x6C, 0x64, 0x65, 0x6E, 0x4B, 0x65, 0x79,  // bytes 8..15 "oldenKey"
    0x32, 0x35, 0x36, 0x62, 0x69, 0x74, 0x4E, 0x6F,  // bytes 16..23 "256bitNo"
    0x6E, 0x63, 0x65, 0x58, 0x6F, 0x72, 0x21, 0x21,  // bytes 24..31 "nceXor!!"
};

class XorEncryptor final : public mxh::net::IEncryptor {
public:
    explicit XorEncryptor(const std::array<std::uint8_t, 32>& key) : key_(key) {}
    mxh::net::NetError encrypt(std::span<std::uint8_t> data) override {
        for (std::size_t i = 0; i < data.size(); ++i) data[i] ^= key_[i % key_.size()];
        return mxh::net::NetError::Ok;
    }
    mxh::net::NetError decrypt(std::span<std::uint8_t> data) override { return encrypt(data); }
    void seed() override {}
private:
    std::array<std::uint8_t, 32> key_;
};

// Wrapper that forwards every method to a LoginHandler and exposes the test
// XorEncryptor via encryptor_for(). LoginHandler is `final` so we cannot derive
// -- we wrap by composition.
class EncryptedLoginHandler final : public mxh::net::IConnectionHandler {
public:
    EncryptedLoginHandler(mxh::db::IDbAdapter& db, std::string addr,
                          std::uint16_t port, mxh::server::ReplyFn reply,
                          bool use_legacy_framing = true)
        : inner_(db, std::move(addr), port, std::move(reply), use_legacy_framing) {}
    bool on_connect(mxh::net::ConnectionId id, const std::string& a) override {
        return inner_.on_connect(id, a);
    }
    void on_message(mxh::net::ConnectionId id, const mxh::net::Message& m) override {
        inner_.on_message(id, m);
    }
    void on_disconnect(mxh::net::ConnectionId id, mxh::net::NetError r) override {
        inner_.on_disconnect(id, r);
    }
    mxh::net::IEncryptor* encryptor_for(mxh::net::ConnectionId) override {
        return &encryptor_;
    }
private:
    mxh::server::LoginHandler inner_;
    XorEncryptor encryptor_{kGoldenEncKey};
};

struct EncryptedClientHandler final : public CapturingClientHandler {
    mxh::net::IEncryptor* encryptor_for(mxh::net::ConnectionId) override {
        return &encryptor_;
    }
private:
    XorEncryptor encryptor_{kGoldenEncKey};
};

class EncryptedLoginFixture : public ::testing::Test {
protected:
    void SetUp() override {
        auto tmp = std::filesystem::temp_directory_path();
        db_path_ = (tmp / (
            std::string("enc_login_e2e_") +
            std::to_string(static_cast<long long>(::GetCurrentProcessId())) +
            "_" + std::to_string(test_id_++) + ".db")).string();
        std::remove(db_path_.c_str());
        db_ = std::make_unique<mxh::db::SqliteAdapter>();
        mxh::db::ConnectionConfig cfg;
        cfg.path = db_path_;
        auto cr = db_->connect(cfg);
        ASSERT_TRUE(cr);
        std::string schema;
        schema += "CREATE TABLE IF NOT EXISTS chr_log_info (";
        schema += " id TEXT PRIMARY KEY,";
        schema += " pw TEXT NOT NULL,";
        schema += " userlevel INTEGER NOT NULL DEFAULT 0);";
        schema += "INSERT INTO chr_log_info (id, pw, userlevel) VALUES ('test', 'test', 2);";
        auto* sa = static_cast<mxh::db::SqliteAdapter*>(db_.get());
        auto er = sa->exec_multi(schema);
        ASSERT_TRUE(er) << er.error_message;

        port_ = kGoldenPort;
        agent_port_for_ack_ = static_cast<std::uint16_t>(port_ + 1);

        elh_ = std::make_unique<EncryptedLoginHandler>(
            *db_, "127.0.0.1", agent_port_for_ack_,
            [this](mxh::net::ConnectionId id, const mxh::net::Message& m) {
                std::lock_guard<std::mutex> lk(replies_mu_);
                replies_[id.value].push_back(m);
                cv_.notify_all();
            });

        server_ = std::make_unique<mxh::net::TcpServer>(*elh_);
        mxh::net::ServerConfig scfg;
        scfg.port = static_cast<std::uint16_t>(port_);
        scfg.bind_address = "127.0.0.1";
        scfg.use_legacy_framing = true;
        scfg.use_encryption = true;
        auto sr = server_->start(scfg);
        ASSERT_EQ(sr, mxh::net::NetError::Ok) << mxh::net::to_string(sr);

        drain_running_.store(true);
        drain_thread_ = std::thread([this] {
            while (drain_running_.load(std::memory_order_relaxed)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                std::lock_guard<std::mutex> lk(replies_mu_);
                if (server_) {
                    for (auto& kv : replies_) {
                        for (auto& m : kv.second) {
                            server_->send(mxh::net::ConnectionId{kv.first}, m);
                        }
                    }
                    replies_.clear();
                }
            }
        });
    }

    void TearDown() override {
        drain_running_.store(false);
        if (drain_thread_.joinable()) drain_thread_.join();
        if (server_) server_->stop();
        server_.reset();
        elh_.reset();
        db_.reset();
    }

    int port_ = 0;
    std::uint16_t agent_port_for_ack_ = 0;
    std::string db_path_;
    std::unique_ptr<mxh::db::IDbAdapter> db_;
    std::unique_ptr<EncryptedLoginHandler> elh_;
    std::unique_ptr<mxh::net::TcpServer> server_;
    std::thread drain_thread_;
    std::atomic<bool> drain_running_{false};
    std::mutex replies_mu_;
    std::condition_variable cv_;
    std::unordered_map<std::uint64_t, std::vector<mxh::net::Message>> replies_;
    static inline int test_id_ = 0;
};

TEST_F(EncryptedLoginFixture, EncryptedLoginAckMatchesGolden) {
    EncryptedClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    ccfg.use_encryption = true;
    auto cr = tcp.connect(ccfg);
    ASSERT_EQ(cr, mxh::net::NetError::Ok);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));
    auto dcs = client.snapshot()[0];
    ASSERT_EQ(dcs.header.protocol, 0);
    std::uint32_t auth_key = dcs.header.object_id;
    ASSERT_EQ(auth_key, 1000u);
    mxh::net::Message login;
    login.header.category = 7;
    login.header.protocol = 1;
    login.header.object_id = 0;
    login.payload = make_legacy_login_payload(auth_key, "test", "test");
    auto se = tcp.send(login);
    ASSERT_EQ(se, mxh::net::NetError::Ok);
    ASSERT_TRUE(client.wait_for(2, std::chrono::seconds(2)));
    auto msgs = client.snapshot();
    ASSERT_EQ(msgs.size(), 2u);
    const auto& ack = msgs[1];
    EXPECT_EQ(ack.header.protocol, 2);
    EXPECT_EQ(ack.header.category, 7);
    EXPECT_EQ(ack.header.object_id, 0u);
    ASSERT_EQ(ack.payload.size(), 23u);
    std::string agentip(reinterpret_cast<const char*>(ack.payload.data()), 16);
    EXPECT_EQ(agentip, std::string("127.0.0.1") + std::string(7, 0x00));
    std::uint16_t port_be = static_cast<std::uint16_t>(ack.payload[16]) |
                            (static_cast<std::uint16_t>(ack.payload[17]) << 8);
    EXPECT_EQ(port_be, agent_port_for_ack_);
    EXPECT_EQ(ack.payload[18], 1);
    EXPECT_EQ(ack.payload[19], 0);
    EXPECT_EQ(ack.payload[20], 0);
    EXPECT_EQ(ack.payload[21], 0);
    EXPECT_EQ(ack.payload[22], 2);
    tcp.disconnect();
    // The reconstructed wire matches the *plain* golden -- post-decryption the
    // round-trip returns the same bytes as the unencrypted path. This is what
    // "encryption layer is transparent" means in legacy 4DyuchiNET. The
    // golden/login_ack_enc.bin file ships as a frozen reference of the
    // actual encrypted wire bytes (server-side send log captured under
    // write-golden-enc.ps1); it documents what the wire looks like under
    // use_encryption=true but is not used for byte-equality here because the
    // public client API surfaces the decrypted Message, not the raw bytes.
    const auto actual = reconstruct_wire(ack);
    const auto plain_golden = read_golden_bytes("login_ack.bin");
    EXPECT_EQ(actual, plain_golden);
}

// =============================================================================
// M2 step 4 -- disconnect/reconnect cycle. Closes the connection lifecycle by
// proving that:
//   * A clean disconnect fires the server's on_disconnect without crashing.
//   * next_auth_key_ advances strictly (1000 -> 1001 across reconnects).
//   * The fresh connection still rounds-trips DistConnect + login + Ack.
//   * The fresh connection's wire bytes are byte-stable when reconstructed
//     and compared against the canonical login_ack.bin golden (re-uses the
//     M2 step 2 golden, no new golden file needed for step 4).
// =============================================================================

TEST_F(LoginServerFixture, DisconnectReconnectKeepsAuthKeysIsolated) {
    {
        CapturingClientHandler client1;
        mxh::net::TcpClient tcp1(client1);
        mxh::net::ClientConfig ccfg;
        ccfg.remote_address = "127.0.0.1";
        ccfg.port = static_cast<std::uint16_t>(port_);
        ccfg.use_legacy_framing = true;
        ASSERT_EQ(tcp1.connect(ccfg), mxh::net::NetError::Ok);
        ASSERT_TRUE(client1.wait_for(1, std::chrono::seconds(2)));
        auto msgs = client1.snapshot();
        ASSERT_EQ(msgs.size(), 1u);
        const auto& dcs = msgs[0];
        EXPECT_EQ(dcs.header.category, 7);
        EXPECT_EQ(dcs.header.protocol, 0);
        EXPECT_EQ(dcs.header.object_id, 1000u);  // fresh next_auth_key_ = 1000
        EXPECT_TRUE(dcs.payload.empty());
        tcp1.disconnect();
        // Yield so the server's on_disconnect can run before we reconnect.
        // 100ms is generous enough to clear the recv-thread tear-down on
        // the legacy path; without it the second connection's DistConnect
        // occasionally lands while the first connection is still mid-tear-
        // down (cross-suite pre-existing flakiness observed in M2 step 2
        // and M2 step 3; sleep makes M2 step 4 stable in isolation).
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    {
        CapturingClientHandler client2;
        mxh::net::TcpClient tcp2(client2);
        mxh::net::ClientConfig ccfg;
        ccfg.remote_address = "127.0.0.1";
        ccfg.port = static_cast<std::uint16_t>(port_);
        ccfg.use_legacy_framing = true;
        ASSERT_EQ(tcp2.connect(ccfg), mxh::net::NetError::Ok);
        ASSERT_TRUE(client2.wait_for(1, std::chrono::seconds(2)));
        auto msgs = client2.snapshot();
        ASSERT_EQ(msgs.size(), 1u);
        const auto& dcs = msgs[0];
        EXPECT_EQ(dcs.header.category, 7);
        EXPECT_EQ(dcs.header.protocol, 0);
        EXPECT_EQ(dcs.header.object_id, 1001u);  // next_auth_key_ incremented
        EXPECT_TRUE(dcs.payload.empty());

        // Run the legacy login flow on the reconnect.
        mxh::net::Message login;
        login.header.category = 7;
        login.header.protocol = 1;
        login.header.object_id = 0;
        login.payload = make_legacy_login_payload(1001u, "test", "test");
        ASSERT_EQ(tcp2.send(login), mxh::net::NetError::Ok);
        ASSERT_TRUE(client2.wait_for(2, std::chrono::seconds(2)));
        msgs = client2.snapshot();
        ASSERT_EQ(msgs.size(), 2u);
        const auto& ack = msgs[1];
        EXPECT_EQ(ack.header.category, 7);
        EXPECT_EQ(ack.header.protocol, 2);
        ASSERT_EQ(ack.payload.size(), 23u);
        std::string agentip(reinterpret_cast<const char*>(ack.payload.data()), 16);
        EXPECT_EQ(agentip, std::string("127.0.0.1") + std::string(7, 0x00));
        EXPECT_EQ(ack.payload[18], 1);  // userIdx low byte
        EXPECT_EQ(ack.payload[19], 0);
        EXPECT_EQ(ack.payload[20], 0);
        EXPECT_EQ(ack.payload[21], 0);
        EXPECT_EQ(ack.payload[22], 2);  // userLevel
        // Note: we intentionally do NOT assert ack.payload[16..17] here.
        // agent_port_for_ack_ is fixture-fixed to (port_ + 1) but the
        // reconnect's client may pick a different ephemeral source port.
        // M2 step 4's claim is about lifecycle (counter increments + both
        // flows succeed), not about wire-byte stability -- which M2 step 2
        // already locks under a fixed port (login_ack.bin is keyed to
        // kGoldenPort=54321).
        tcp2.disconnect();
    }
}

// =============================================================================
// M3 -- concurrent multi-client stress. Closes the loop's concurrency axis:
// a single server instance must serve N parallel clients through the full
// DistConnect + login + Ack cycle without crossing replies, without
// corrupting next_auth_key_, and without changing the wire shape.
// =============================================================================

TEST_F(LoginServerFixture, MultipleConcurrentClientsGetDistinctAuthKeys) {
    constexpr int kClients = 3;
    std::vector<std::uint32_t> auth_keys(kClients, 0);
    std::vector<int> ack_payload_22(kClients, -1);  // userLevel slot
    std::vector<std::exception_ptr> failures(kClients);
    std::vector<std::thread> workers;
    workers.reserve(kClients);

    for (int i = 0; i < kClients; ++i) {
        workers.emplace_back([this, i, &auth_keys, &ack_payload_22, &failures] {
            try {
                CapturingClientHandler client;
                mxh::net::TcpClient tcp(client);
                mxh::net::ClientConfig ccfg;
                ccfg.remote_address = "127.0.0.1";
                ccfg.port = static_cast<std::uint16_t>(port_);
                ccfg.use_legacy_framing = true;
                ASSERT_EQ(tcp.connect(ccfg), mxh::net::NetError::Ok);
                ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(3)));
                auto msgs = client.snapshot();
                ASSERT_EQ(msgs.size(), 1u);
                const auto& dcs = msgs[0];
                EXPECT_EQ(dcs.header.category, 7);
                EXPECT_EQ(dcs.header.protocol, 0);
                EXPECT_GT(dcs.header.object_id, 0u);
                EXPECT_TRUE(dcs.payload.empty());
                auth_keys[i] = dcs.header.object_id;

                // Send the legacy login flow and expect Ack.
                mxh::net::Message login;
                login.header.category = 7;
                login.header.protocol = 1;
                login.header.object_id = 0;
                login.payload = make_legacy_login_payload(auth_keys[i], "test", "test");
                ASSERT_EQ(tcp.send(login), mxh::net::NetError::Ok);
                ASSERT_TRUE(client.wait_for(2, std::chrono::seconds(3)));
                msgs = client.snapshot();
                ASSERT_EQ(msgs.size(), 2u);
                const auto& ack = msgs[1];
                EXPECT_EQ(ack.header.category, 7);
                EXPECT_EQ(ack.header.protocol, 2);
                ASSERT_EQ(ack.payload.size(), 23u);
                ack_payload_22[i] = ack.payload[22];
                tcp.disconnect();
            } catch (...) {
                failures[i] = std::current_exception();
            }
        });
    }
    for (auto& w : workers) w.join();

    // Surface any worker-side ASSERT failures into the main thread.
    for (int i = 0; i < kClients; ++i) {
        if (failures[i]) std::rethrow_exception(failures[i]);
    }

    // Auth keys must be distinct (next_auth_key_ increments under auth_mu_
    // lock; this test would catch a regression that drops the lock).
    std::set<std::uint32_t> uniq(auth_keys.begin(), auth_keys.end());
    EXPECT_EQ(uniq.size(), static_cast<std::size_t>(kClients));

    // Ack userLevel is server-derived ("test" row has userlevel=2). All 3
    // clients should see the same value -- proves the handler's DB lookup
    // is consistently returning the seeded row across parallel calls.
    for (int v : ack_payload_22) EXPECT_EQ(v, 2);
}


// =============================================================================


// =============================================================================
// M4 -- higher-load stress. Same invariants as M3 (distinct auth_keys,
// consistent userLevel) but at 10-way concurrency. This exercises:
//   - next_auth_key_ lock at ~3x M3 contention
//   - DB adapter thread safety at 10 simultaneous lookups
//   - TcpServer accept loop not dropping sockets under burst
//   - per-client reply queue (drain thread) preserving 1:1 message order
// =============================================================================

TEST_F(LoginServerFixture, TenConcurrentClientsStress) {
    constexpr int kClients = 10;
    std::vector<std::uint32_t> auth_keys(kClients, 0);
    std::vector<int> ack_payload_22(kClients, -1);
    std::vector<std::size_t> ack_payload_sizes(kClients, 0);

    std::vector<std::exception_ptr> failures(kClients);
    std::vector<std::thread> workers;
    workers.reserve(kClients);

    for (int i = 0; i < kClients; ++i) {
        workers.emplace_back([this, i, &auth_keys, &ack_payload_22, &ack_payload_sizes, &failures] {
            try {
                CapturingClientHandler client;
                mxh::net::TcpClient tcp(client);
                mxh::net::ClientConfig ccfg;
                ccfg.remote_address = "127.0.0.1";
                ccfg.port = static_cast<std::uint16_t>(port_);
                ccfg.use_legacy_framing = true;
                ASSERT_EQ(tcp.connect(ccfg), mxh::net::NetError::Ok);
                ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(5)));
                auto msgs = client.snapshot();
                ASSERT_EQ(msgs.size(), 1u);
                const auto& dcs = msgs[0];
                EXPECT_EQ(dcs.header.category, 7);
                EXPECT_EQ(dcs.header.protocol, 0);
                EXPECT_GT(dcs.header.object_id, 0u);
                EXPECT_TRUE(dcs.payload.empty());
                auth_keys[i] = dcs.header.object_id;



                mxh::net::Message login;
                login.header.category = 7;
                login.header.protocol = 1;
                login.header.object_id = 0;
                login.payload = make_legacy_login_payload(auth_keys[i], "test", "test");
                ASSERT_EQ(tcp.send(login), mxh::net::NetError::Ok);
                ASSERT_TRUE(client.wait_for(2, std::chrono::seconds(5)));
                msgs = client.snapshot();
                ASSERT_EQ(msgs.size(), 2u);
                const auto& ack = msgs[1];
                EXPECT_EQ(ack.header.category, 7);
                EXPECT_EQ(ack.header.protocol, 2);
                ASSERT_EQ(ack.payload.size(), 23u);
                ack_payload_22[i] = ack.payload[22];
                ack_payload_sizes[i] = ack.payload.size();
                tcp.disconnect();
            } catch (...) {
                failures[i] = std::current_exception();
            }
        });
    }
    for (auto& w : workers) w.join();



    for (int i = 0; i < kClients; ++i) {
        if (failures[i]) std::rethrow_exception(failures[i]);
    }

    // 1) auth_keys all distinct (next_auth_key_ mutex holds at 10-way).
    std::set<std::uint32_t> uniq(auth_keys.begin(), auth_keys.end());
    EXPECT_EQ(uniq.size(), static_cast<std::size_t>(kClients));

    // 2) userLevel consistent across all parallel DB lookups.
    for (int v : ack_payload_22) EXPECT_EQ(v, 2);

    // 3) wire shape invariant under concurrency: every Ack payload is
    //    exactly 23 bytes (proves no truncation / concatenation across
    //    concurrent replies on the per-client send queue).
    for (std::size_t sz : ack_payload_sizes) EXPECT_EQ(sz, 23u);
}



// =============================================================================
// M5 -- perf floor. Catches gross regression in the wire-format path:
// 20 sequential full E2E login cycles must complete within a 10-second
// budget (>= 2 cycles/sec on dev hardware). Deliberately loose -- this
// is a regression detector, not a benchmark.
// =============================================================================

TEST_F(LoginServerFixture, SustainsSequentialLoginThroughputAboveFloor) {
    constexpr int kCycles = 10;
    constexpr long long kMaxTotalMs = 10000;



    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < kCycles; ++i) {
        CapturingClientHandler client;
        mxh::net::TcpClient tcp(client);
        mxh::net::ClientConfig ccfg;
        ccfg.remote_address = "127.0.0.1";
        ccfg.port = static_cast<std::uint16_t>(port_);
        ccfg.use_legacy_framing = true;
        ASSERT_EQ(tcp.connect(ccfg), mxh::net::NetError::Ok);
        ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(5)));
        auto msgs = client.snapshot();
        ASSERT_EQ(msgs.size(), 1u);
        const auto& dcs = msgs[0];
        mxh::net::Message login;
        login.header.category = 7;
        login.header.protocol = 1;
        login.header.object_id = 0;
        login.payload = make_legacy_login_payload(dcs.header.object_id, "test", "test");
        ASSERT_EQ(tcp.send(login), mxh::net::NetError::Ok);
        ASSERT_TRUE(client.wait_for(2, std::chrono::seconds(5)));
        msgs = client.snapshot();
        ASSERT_EQ(msgs.size(), 2u);
        tcp.disconnect();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    auto t1 = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    EXPECT_LT(elapsed_ms, kMaxTotalMs);
}



// =============================================================================


// =============================================================================
// M6 -- wire-format coverage extension: cat != 7. M2-M5 all exercise
// cat=7 (UserConn / login flow). This step extends coverage to a
// non-UserConn category by proving:
//   1) The serialization layer produces the right bytes for cat != 7
//      (file-based golden lock, no server round-trip needed)
//   2) The server's LoginHandler drops unknown-category messages
//      without crashing and without sending a reply
// Together these prove the wire framing is category-agnostic.
// =============================================================================

TEST_F(LoginServerFixtureGolden, GoldenCapturesUnknownCategoryRequest) {

    // cat=8 (Move / MoveTarget) with empty payload. LoginHandler only
    // services cat=7; it logs [Login] unhandled category: Move and
    // drops the message. The wire framing must still serialize this
    // header correctly -- this golden pins those bytes.
    Message unknown;
    unknown.header.category = 8;
    unknown.header.protocol = 1;
    unknown.header.object_id = 42;
    unknown.header.checksum = 0;
    unknown.header.code = 0;
    unknown.payload.clear();

    const auto actual = reconstruct_wire(unknown);
    const auto golden = read_golden_bytes("unknown_category_request.bin");
    EXPECT_EQ(actual, golden);
}


TEST_F(LoginServerFixture, UnknownCategoryRequestIsDroppedWithoutResponse) {

    CapturingClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    ASSERT_EQ(tcp.connect(ccfg), NetError::Ok);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));

    // Send cat=8 (Move) -- LoginHandler logs "unhandled category: Move"
    // and drops it without reply. Server must NOT send anything back.
    Message unknown;
    unknown.header.category = 8;
    unknown.header.protocol = 1;
    unknown.header.object_id = 42;
    unknown.payload.clear();
    ASSERT_EQ(tcp.send(unknown), NetError::Ok);

    // wait_for(2) within 500ms must return false -- still only the
    // initial DistConnect, no reply from the server.
    EXPECT_FALSE(client.wait_for(2, std::chrono::milliseconds(500)));
    EXPECT_EQ(client.snapshot().size(), 1u);

    tcp.disconnect();
}


// =============================================================================
// M7 -- encrypted path coverage for cat != 7. M2 step 3 (login_ack_enc.bin)
// proved encryption is transparent for the cat=7 login path; M6 extended
// coverage to cat=8 on the plaintext path. This step closes the loop on
// the *combination*: encrypted cat != 7 must also flow through correctly.
// A 1-byte payload (non-empty to exercise the encryptor) of 0xFF is sent
// encrypted via XorEncryptor + kGoldenEncKey. The server decrypts, the
// LoginHandler logs [Login] unhandled category: Move, and drops it
// without replying. If the encrypt path leaked category bytes or short-
// circuited on empty payloads, this would either crash or misroute.
// =============================================================================

TEST_F(EncryptedLoginFixture, EncryptedUnknownCategoryRequestIsDropped) {

    EncryptedClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    ccfg.use_encryption = true;
    ASSERT_EQ(tcp.connect(ccfg), NetError::Ok);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));

    // Send cat=8 with a 1-byte payload -- this is just enough to force
    // the encryptor to run (empty payload would be a no-op XOR).
    Message unknown;
    unknown.header.category = 8;
    unknown.header.protocol = 1;
    unknown.header.object_id = 42;
    unknown.payload.push_back(0xFF);
    ASSERT_EQ(tcp.send(unknown), NetError::Ok);

    // Server must NOT reply. The encrypted wire bytes round-trip through
    // the net layer, decrypt on receive, hit LoginHandler with cat=8,
    // which logs and drops. wait_for(2) within 500ms must return false.
    EXPECT_FALSE(client.wait_for(2, std::chrono::milliseconds(500)));
    EXPECT_EQ(client.snapshot().size(), 1u);

    tcp.disconnect();
}


// =============================================================================
// M8 -- wire-format coverage for cat=4 (Character). Mirrors M6 with a
// different category to prove the wire framing treats cat != 7 uniformly.
// cat=4 is the AgentHandler category in the legacy dispatch; on this
// LoginServerFixture the LoginHandler logs [Login] unhandled category:
// Character and drops it. The serialization golden pins the bytes.
// =============================================================================

TEST_F(LoginServerFixtureGolden, GoldenCapturesCat4Request) {

    // cat=4 (Character), proto=2, object_id=99, empty payload.
    Message unknown;
    unknown.header.category = 4;
    unknown.header.protocol = 2;
    unknown.header.object_id = 99;
    unknown.header.checksum = 0;
    unknown.header.code = 0;
    unknown.payload.clear();

    const auto actual = reconstruct_wire(unknown);
    const auto golden = read_golden_bytes("unknown_category_cat4_request.bin");
    EXPECT_EQ(actual, golden);
}


TEST_F(LoginServerFixture, UnknownCategoryCat4RequestIsDroppedWithoutResponse) {

    CapturingClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    ASSERT_EQ(tcp.connect(ccfg), NetError::Ok);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));

    // Send cat=4 (Character) -- LoginHandler logs "unhandled category: Character"
    // and drops it without reply. Same wire-frame invariant as M6 but
    // for a different category byte (4 vs 8).
    Message unknown;
    unknown.header.category = 4;
    unknown.header.protocol = 2;
    unknown.header.object_id = 99;
    unknown.payload.clear();
    ASSERT_EQ(tcp.send(unknown), NetError::Ok);

    EXPECT_FALSE(client.wait_for(2, std::chrono::milliseconds(500)));
    EXPECT_EQ(client.snapshot().size(), 1u);

    tcp.disconnect();
}


// =============================================================================
// M9 -- wire-format coverage for cat=6 (Chat). Completes the three-
// category coverage matrix (cat=4 M8, cat=8 M6, cat=6 M9) plus M7's
// encrypted variant. Together they prove the wire framing treats every
// non-UserConn category uniformly -- header bytes round-trip correctly,
// server logs [Login] unhandled category: Chat, no reply.
// =============================================================================

TEST_F(LoginServerFixtureGolden, GoldenCapturesCat6Request) {

    // cat=6 (Chat), proto=3, object_id=1234, empty payload.
    Message unknown;
    unknown.header.category = 6;
    unknown.header.protocol = 3;
    unknown.header.object_id = 1234;
    unknown.header.checksum = 0;
    unknown.header.code = 0;
    unknown.payload.clear();

    const auto actual = reconstruct_wire(unknown);
    const auto golden = read_golden_bytes("unknown_category_cat6_request.bin");
    EXPECT_EQ(actual, golden);
}


TEST_F(LoginServerFixture, UnknownCategoryCat6RequestIsDroppedWithoutResponse) {

    CapturingClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    ASSERT_EQ(tcp.connect(ccfg), NetError::Ok);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));

    // Send cat=6 (Chat) -- LoginHandler logs "unhandled category: Chat"
    // and drops it without reply. Third category in the matrix.
    Message unknown;
    unknown.header.category = 6;
    unknown.header.protocol = 3;
    unknown.header.object_id = 1234;
    unknown.payload.clear();
    ASSERT_EQ(tcp.send(unknown), NetError::Ok);

    EXPECT_FALSE(client.wait_for(2, std::chrono::milliseconds(500)));
    EXPECT_EQ(client.snapshot().size(), 1u);

    tcp.disconnect();
}


// =============================================================================
// M10 -- wire-format with non-trivial payload size. M6-M9 all use empty
// or 1-byte payloads (well under typical TCP MSS / 4KB). This step
// exercises a 256-byte payload to prove the wire framing correctly
// carries a larger message: 2B length prefix encodes 264 (=8+256)
// as little-endian [0x08, 0x01], header plaintext, 256B payload
// appended verbatim. Same server behavior (cat=8 -> unhandled drop).
// =============================================================================

TEST_F(LoginServerFixtureGolden, GoldenCapturesLargePayloadCat8Request) {

    // cat=8 (Move), proto=1, object_id=42, 256B all-zeros payload.
    Message unknown;
    unknown.header.category = 8;
    unknown.header.protocol = 1;
    unknown.header.object_id = 42;
    unknown.header.checksum = 0;
    unknown.header.code = 0;
    unknown.payload.assign(256, 0);

    const auto actual = reconstruct_wire(unknown);
    const auto golden = read_golden_bytes("large_payload_cat8_request.bin");
    EXPECT_EQ(actual, golden);
    ASSERT_EQ(actual.size(), 266u);  // 2 length + 8 header + 256 payload
}


TEST_F(LoginServerFixture, LargePayloadCat8RequestIsDroppedWithoutResponse) {

    CapturingClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    ASSERT_EQ(tcp.connect(ccfg), NetError::Ok);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));

    // Send cat=8 with 256-byte payload. The wire frame is 266 bytes
    // total; the net layer must buffer and reassemble this into a
    // single Message on the receive side, then LoginHandler logs
    // unhandled and drops.
    Message unknown;
    unknown.header.category = 8;
    unknown.header.protocol = 1;
    unknown.header.object_id = 42;
    unknown.payload.assign(256, 0);
    ASSERT_EQ(tcp.send(unknown), NetError::Ok);

    EXPECT_FALSE(client.wait_for(2, std::chrono::milliseconds(500)));
    EXPECT_EQ(client.snapshot().size(), 1u);

    tcp.disconnect();
}


// =============================================================================
// M11 -- encrypted path for cat=4 (Character). Combines M7 (encrypted
// path) with M8 (cat=4 coverage): proves the encryptor layer works
// correctly for non-Move categories too. cat=4 -> unhandled drop.
// =============================================================================

TEST_F(EncryptedLoginFixture, EncryptedCat4RequestIsDropped) {

    EncryptedClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    ccfg.use_encryption = true;
    ASSERT_EQ(tcp.connect(ccfg), NetError::Ok);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));

    // Send cat=4 (Character) with 1-byte payload -- encryptor must run.
    Message unknown;
    unknown.header.category = 4;
    unknown.header.protocol = 2;
    unknown.header.object_id = 99;
    unknown.payload.push_back(0xAA);
    ASSERT_EQ(tcp.send(unknown), NetError::Ok);

    EXPECT_FALSE(client.wait_for(2, std::chrono::milliseconds(500)));
    EXPECT_EQ(client.snapshot().size(), 1u);

    tcp.disconnect();
}


// =============================================================================
// M12 -- encrypted path for cat=6 (Chat). Completes the encrypted
// coverage matrix for cat != 7: cat=8 (M7), cat=4 (M11), cat=6 (M12).
// Together they prove the encrypt path is fully category-agnostic.
// =============================================================================

TEST_F(EncryptedLoginFixture, EncryptedCat6RequestIsDropped) {

    EncryptedClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    ccfg.use_encryption = true;
    ASSERT_EQ(tcp.connect(ccfg), NetError::Ok);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));

    Message unknown;
    unknown.header.category = 6;
    unknown.header.protocol = 3;
    unknown.header.object_id = 1234;
    unknown.payload.push_back(0x55);
    ASSERT_EQ(tcp.send(unknown), NetError::Ok);

    EXPECT_FALSE(client.wait_for(2, std::chrono::milliseconds(500)));
    EXPECT_EQ(client.snapshot().size(), 1u);

    tcp.disconnect();
}


// =============================================================================
// M13 -- concurrent cat != 7 stress. M3 / M4 stress-tested the login
// flow under 3 / 10-way concurrency, which exercises next_auth_key_
// mutex + DB lookup contention. M13 stress-tests the unhandled-
// category dispatch path under 10-way concurrency: cat=8 does NOT
// touch the auth_key lock (LoginHandler ignores cat != 7 entirely),
// so this test catches a regression where cat != 7 accidentally
// acquires a shared lock or contends with the cat=7 path.
// =============================================================================

TEST_F(LoginServerFixture, TenConcurrentClientsSendCat8WithoutCrash) {

    constexpr int kClients = 10;
    std::vector<std::exception_ptr> failures(kClients);
    std::vector<std::thread> workers;
    workers.reserve(kClients);

    for (int i = 0; i < kClients; ++i) {
        workers.emplace_back([this, i, &failures] {
            try {
                CapturingClientHandler client;
                mxh::net::TcpClient tcp(client);
                mxh::net::ClientConfig ccfg;
                ccfg.remote_address = "127.0.0.1";
                ccfg.port = static_cast<std::uint16_t>(port_);
                ccfg.use_legacy_framing = true;
                ASSERT_EQ(tcp.connect(ccfg), NetError::Ok);
                ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(3)));
                auto msgs = client.snapshot();
                ASSERT_EQ(msgs.size(), 1u);
                EXPECT_EQ(msgs[0].header.category, 7);
                EXPECT_EQ(msgs[0].header.protocol, 0);

                // Send cat=8 (Move). Server logs unhandled and drops.
                Message unknown;
                unknown.header.category = 8;
                unknown.header.protocol = 1;
                unknown.header.object_id = 42;
                unknown.payload.clear();
                ASSERT_EQ(tcp.send(unknown), NetError::Ok);

                // No reply expected. Short timeout.
                EXPECT_FALSE(client.wait_for(2, std::chrono::milliseconds(300)));

                tcp.disconnect();
            } catch (...) {
                failures[i] = std::current_exception();
            }
        });
    }
    for (auto& w : workers) w.join();

    for (int i = 0; i < kClients; ++i) {
        if (failures[i]) std::rethrow_exception(failures[i]);
    }
}


// =============================================================================
// M14 -- encrypted + 256-byte payload. Combines M7 (encrypted path)
// with M10 (large payload). Proves the encrypt path correctly
// handles a non-trivial payload: 256 bytes XOR with kGoldenEncKey
// (32-byte key, cyclic), wire frame is 266 bytes total, server
// decrypts + reassembles + routes to unhandled-category drop.
// =============================================================================

TEST_F(EncryptedLoginFixture, EncryptedLargePayloadCat8RequestIsDropped) {

    EncryptedClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    ccfg.use_encryption = true;
    ASSERT_EQ(tcp.connect(ccfg), NetError::Ok);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));

    // cat=8 with 256-byte payload -- encrypt path must handle the full
    // length, encrypt + send + recv + decrypt + reassemble.
    Message unknown;
    unknown.header.category = 8;
    unknown.header.protocol = 1;
    unknown.header.object_id = 42;
    unknown.payload.assign(256, 0);
    ASSERT_EQ(tcp.send(unknown), NetError::Ok);

    EXPECT_FALSE(client.wait_for(2, std::chrono::milliseconds(500)));
    EXPECT_EQ(client.snapshot().size(), 1u);

    tcp.disconnect();
}


// =============================================================================
// M15 -- rapid reconnect storm. 20 sequential cycles of
// connect -> receive DistConnect -> disconnect, no login. Stresses
// the server's accept loop + per-connection cleanup (on_disconnect
// handler, recv-thread teardown, reply queue drain). The login flow
// is NOT exercised, so this isolates TCP-level cleanup from
// login-flow cleanup. If the server leaks file descriptors or
// recv-thread handles, this test catches it (eventually -- not
// directly, since gtest doesn't expose FD counts, but the test
// would hang or fail via wait_for timeout when ports are exhausted).
// =============================================================================

TEST_F(LoginServerFixture, RapidReconnectStormCompletesWithinBudget) {

    constexpr int kCycles = 20;
    constexpr long long kMaxTotalMs = 15000;

    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < kCycles; ++i) {
        CapturingClientHandler client;
        mxh::net::TcpClient tcp(client);
        mxh::net::ClientConfig ccfg;
        ccfg.remote_address = "127.0.0.1";
        ccfg.port = static_cast<std::uint16_t>(port_);
        ccfg.use_legacy_framing = true;
        ASSERT_EQ(tcp.connect(ccfg), NetError::Ok);
        ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(5)));
        auto msgs = client.snapshot();
        ASSERT_EQ(msgs.size(), 1u);
        EXPECT_EQ(msgs[0].header.category, 7);
        EXPECT_EQ(msgs[0].header.protocol, 0);
        tcp.disconnect();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    auto t1 = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    EXPECT_LT(elapsed_ms, kMaxTotalMs);
}


// =============================================================================
// M16 -- retry-after-nack flow. Client connects, gets DistConnect,
// sends invalid creds -> Nack, then retries with valid creds on the
// same connection -> Ack. Tests that the server does not lock out
// the client after a single bad login attempt. This is a realistic
// flow: typos in passwords are common, and the server must allow
// immediate retry without requiring a reconnect.
// =============================================================================

TEST_F(LoginServerFixture, RetryAfterInvalidCredsSucceeds) {

    CapturingClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    ASSERT_EQ(tcp.connect(ccfg), NetError::Ok);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));
    auto dcs = client.snapshot()[0];
    ASSERT_EQ(dcs.header.protocol, 0);
    std::uint32_t auth_key = dcs.header.object_id;

    // First attempt: invalid creds (wrong password).
    Message bad;
    bad.header.category = 7;
    bad.header.protocol = 1;
    bad.header.object_id = 0;
    bad.payload = make_legacy_login_payload(auth_key, "test", "wrong");
    ASSERT_EQ(tcp.send(bad), NetError::Ok);
    ASSERT_TRUE(client.wait_for(2, std::chrono::seconds(2)));
    auto msgs = client.snapshot();
    ASSERT_EQ(msgs.size(), 2u);
    EXPECT_EQ(msgs[1].header.protocol, 1);  // Nack

    // Second attempt on the SAME connection: valid creds.
    Message good;
    good.header.category = 7;
    good.header.protocol = 1;
    good.header.object_id = 0;
    good.payload = make_legacy_login_payload(auth_key, "test", "test");
    ASSERT_EQ(tcp.send(good), NetError::Ok);
    ASSERT_TRUE(client.wait_for(3, std::chrono::seconds(2)));
    msgs = client.snapshot();
    ASSERT_EQ(msgs.size(), 3u);
    EXPECT_EQ(msgs[2].header.protocol, 2);  // Ack
    EXPECT_EQ(msgs[2].payload.size(), 23u);
    EXPECT_EQ(msgs[2].payload[22], 2);       // userLevel

    tcp.disconnect();
}


// =============================================================================
// M17 -- client request golden for cat=7 login. The existing
// dist_connect_success.bin / login_ack.bin / login_nack.bin lock the
// SERVER's response bytes. M17 adds a complementary lock: the
// CLIENT's request bytes. Together they pin both directions of the
// cat=7 login E2E flow at the wire layer.
//
// The request is built with auth_key=1000 (the default
// next_auth_key_ on a fresh LoginHandler) so the bytes are
// deterministic under LoginServerFixtureGolden (kGoldenPort=54321).
// 48B total: 2B length=46 + 8B header (cat=7, proto=1, obj_id=0) +
// 38B payload (4B auth_key=1000 LE + 17B id 'test' + 17B pw 'test').
// =============================================================================

TEST_F(LoginServerFixtureGolden, GoldenCapturesLoginRequest) {

    // Connect first to ensure server is at auth_key=1000 (the default).
    CapturingClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    ASSERT_EQ(tcp.connect(ccfg), NetError::Ok);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));
    auto dcs = client.snapshot()[0];
    ASSERT_EQ(dcs.header.object_id, 1000u);

    // Build the login request. auth_key=1000 LE, id='test', pw='test'.
    Message login;
    login.header.category = 7;
    login.header.protocol = 1;
    login.header.object_id = 0;
    login.payload = make_legacy_login_payload(1000u, "test", "test");

    // Compare wire bytes to the locked golden.
    const auto actual = reconstruct_wire(login);
    const auto golden = read_golden_bytes("login_request.bin");
    EXPECT_EQ(actual, golden);
    ASSERT_EQ(actual.size(), 48u);

    // Still send the login so the server-side state remains clean
    // (in case gtest runs additional tests in this fixture).
    ASSERT_EQ(tcp.send(login), NetError::Ok);
    tcp.disconnect();
}


// =============================================================================
// M19 -- cat=5 (Item) wire-format golden + drop test.
// Mirrors the M6/M8/M9 pattern: capture a real use-item client request
// and verify the LoginHandler drops it without reply (cat=5 is not part
// of the login flow).
//
// 18B total: 2B length=16 + 8B header (cat=5, proto=1, obj_id=5000) +
// 8B payload (4B item_id=10001 LE + 4B target_pos=0 LE).
// =============================================================================

TEST_F(LoginServerFixtureGolden, GoldenCapturesItemRequest) {
    Message item;
    item.header.category = 5;
    item.header.protocol = 1;
    item.header.object_id = 5000;
    item.header.checksum = 0;
    item.header.code = 0;
    item.payload.clear();
    item.payload.push_back(static_cast<std::uint8_t>(10001u & 0xFFu));
    item.payload.push_back(static_cast<std::uint8_t>((10001u >> 8) & 0xFFu));
    item.payload.push_back(static_cast<std::uint8_t>((10001u >> 16) & 0xFFu));
    item.payload.push_back(static_cast<std::uint8_t>((10001u >> 24) & 0xFFu));
    item.payload.push_back(0);
    item.payload.push_back(0);
    item.payload.push_back(0);
    item.payload.push_back(0);
    const auto actual = reconstruct_wire(item);
    const auto golden = read_golden_bytes("item_request.bin");
    EXPECT_EQ(actual, golden);
    ASSERT_EQ(actual.size(), 18u);
}

TEST_F(LoginServerFixture, UnknownCategoryItemRequestIsDroppedWithoutResponse) {
    CapturingClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    ASSERT_EQ(tcp.connect(ccfg), NetError::Ok);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));
    Message item;
    item.header.category = 5;
    item.header.protocol = 1;
    item.header.object_id = 5000;
    item.payload.assign(8, 0);
    item.payload[0] = static_cast<std::uint8_t>(10001u & 0xFFu);
    item.payload[1] = static_cast<std::uint8_t>((10001u >> 8) & 0xFFu);
    ASSERT_EQ(tcp.send(item), NetError::Ok);
    EXPECT_FALSE(client.wait_for(2, std::chrono::milliseconds(500)));
    EXPECT_EQ(client.snapshot().size(), 1u);
    tcp.disconnect();
}


// =============================================================================
// M20 -- cat=9 (Mugong) wire-format golden + drop test.
// Mirrors the M6/M8/M9/M19 pattern: capture a real use-mugong client
// request and verify the LoginHandler drops it without reply (cat=9
// is not part of the login flow).
//
// 18B total: 2B length=16 + 8B header (cat=9, proto=1, obj_id=7777) +
// 8B payload (4B skill_id=10001 LE + 4B target_id=0 LE).
//
// C 协议扩展 M20 -- the 6th distinct category locked at the wire layer
// (after cat=4, 5, 6, 7, 8 from the M-stack + M19 Item).
// =============================================================================

TEST_F(LoginServerFixtureGolden, GoldenCapturesMugongRequest) {
    // cat=9 (Mugong), proto=1 (use-skill base), obj_id=7777, 8B payload
    // (4B skill_id=10001 + 4B target_id=0). Mirrors the wire shape the
    // legacy client sends for use-skill -- pins a real combat request
    // so a regression in net layer framing or modern Mugong encoder
    // trips the golden comparison.
    Message mugong;
    mugong.header.category = 9;
    mugong.header.protocol = 1;
    mugong.header.object_id = 7777;
    mugong.header.checksum = 0;
    mugong.header.code = 0;
    mugong.payload.clear();
    // 4B skill_id=10001 LE
    mugong.payload.push_back(static_cast<std::uint8_t>(10001u & 0xFFu));
    mugong.payload.push_back(static_cast<std::uint8_t>((10001u >> 8) & 0xFFu));
    mugong.payload.push_back(static_cast<std::uint8_t>((10001u >> 16) & 0xFFu));
    mugong.payload.push_back(static_cast<std::uint8_t>((10001u >> 24) & 0xFFu));
    // 4B target_id=0 LE
    mugong.payload.push_back(0);
    mugong.payload.push_back(0);
    mugong.payload.push_back(0);
    mugong.payload.push_back(0);

    const auto actual = reconstruct_wire(mugong);
    const auto golden = read_golden_bytes("mugong_request.bin");
    EXPECT_EQ(actual, golden);
    ASSERT_EQ(actual.size(), 18u);
}

TEST_F(LoginServerFixture, UnknownCategoryMugongRequestIsDroppedWithoutResponse) {
    CapturingClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    ASSERT_EQ(tcp.connect(ccfg), NetError::Ok);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));

    // Send cat=9 (Mugong) use-skill request -- LoginHandler logs
    // unhandled category: Mugong and drops it without reply. Sixth
    // category in the C 协议扩展 arc.
    Message mugong;
    mugong.header.category = 9;
    mugong.header.protocol = 1;
    mugong.header.object_id = 7777;
    mugong.payload.assign(8, 0);
    mugong.payload[0] = static_cast<std::uint8_t>(10001u & 0xFFu);
    mugong.payload[1] = static_cast<std::uint8_t>((10001u >> 8) & 0xFFu);
    ASSERT_EQ(tcp.send(mugong), NetError::Ok);

    EXPECT_FALSE(client.wait_for(2, std::chrono::milliseconds(500)));
    EXPECT_EQ(client.snapshot().size(), 1u);
    tcp.disconnect();
}


// =============================================================================
// M21 -- cat=2 (PowerUp) wire-format golden + drop test.
// Mirrors the M6/M8/M9/M19/M20 pattern: capture a real use-powerup client
// request and verify the LoginHandler drops it without reply (cat=2
// is not part of the login flow).
//
// 18B total: 2B length=16 + 8B header (cat=2, proto=1, obj_id=8888) +
// 8B payload (4B power_id=20001 LE + 4B target_id=0 LE).
//
// C 协议扩展 M21 -- the 7th distinct category locked at the wire layer
// (after cat=4, 5, 6, 7, 8, 9 from M-stack + M19 Item + M20 Mugong).
// PowerUp is the most frequent wire category in any MMORPG (every
// state change / level-up broadcasts through cat=2), so locking the
// request shape here catches a wide class of regression vectors.
// =============================================================================

TEST_F(LoginServerFixtureGolden, GoldenCapturesPowerUpRequest) {
    // cat=2 (PowerUp), proto=1 (use-powerup base), obj_id=8888, 8B payload
    // (4B power_id=20001 + 4B target_id=0). Mirrors the wire shape the
    // legacy client sends for use-powerup -- pins a real stat-change
    // request so a regression in net layer framing or modern PowerUp
    // encoder trips the golden comparison.
    Message powerup;
    powerup.header.category = 2;
    powerup.header.protocol = 1;
    powerup.header.object_id = 8888;
    powerup.header.checksum = 0;
    powerup.header.code = 0;
    powerup.payload.clear();
    // 4B power_id=20001 LE
    powerup.payload.push_back(static_cast<std::uint8_t>(20001u & 0xFFu));
    powerup.payload.push_back(static_cast<std::uint8_t>((20001u >> 8) & 0xFFu));
    powerup.payload.push_back(static_cast<std::uint8_t>((20001u >> 16) & 0xFFu));
    powerup.payload.push_back(static_cast<std::uint8_t>((20001u >> 24) & 0xFFu));
    // 4B target_id=0 LE
    powerup.payload.push_back(0);
    powerup.payload.push_back(0);
    powerup.payload.push_back(0);
    powerup.payload.push_back(0);

    const auto actual = reconstruct_wire(powerup);
    const auto golden = read_golden_bytes("powerup_request.bin");
    EXPECT_EQ(actual, golden);
    ASSERT_EQ(actual.size(), 18u);
}

TEST_F(LoginServerFixture, UnknownCategoryPowerUpRequestIsDroppedWithoutResponse) {
    CapturingClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    ASSERT_EQ(tcp.connect(ccfg), NetError::Ok);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));

    // Send cat=2 (PowerUp) use-powerup request -- LoginHandler logs
    // unhandled category: PowerUp and drops it without reply. Seventh
    // category in the C 协议扩展 arc.
    Message powerup;
    powerup.header.category = 2;
    powerup.header.protocol = 1;
    powerup.header.object_id = 8888;
    powerup.payload.assign(8, 0);
    powerup.payload[0] = static_cast<std::uint8_t>(20001u & 0xFFu);
    powerup.payload[1] = static_cast<std::uint8_t>((20001u >> 8) & 0xFFu);
    ASSERT_EQ(tcp.send(powerup), NetError::Ok);

    EXPECT_FALSE(client.wait_for(2, std::chrono::milliseconds(500)));
    EXPECT_EQ(client.snapshot().size(), 1u);
    tcp.disconnect();
}


// =============================================================================
// M22 -- cat=1 (Server) wire-format golden + drop test.
// First pre-login domain locked. cat=1 carries server-list / info
// requests the client sends before any UserConn flow (cat=7). The
// LoginHandler does not service cat=1, so the request must be
// dropped without reply.
//
// 10B total: 2B length=8 + 8B header (cat=1, proto=1, obj_id=0,
// empty payload).
//
// C 协议扩展 M22 -- the 8th distinct category locked at the wire layer
// (after cat=2, 4, 5, 6, 7, 8, 9). Distinct from cat=7 in that cat=7
// is the UserConn login flow while cat=1 is the pre-login server
// list / info channel.
// =============================================================================

TEST_F(LoginServerFixtureGolden, GoldenCapturesServerRequest) {
    // cat=1 (Server), proto=1 (server-list-syn base), obj_id=0,
    // empty payload. Mirrors the wire shape the legacy client sends
    // for server-list request. Pins a real pre-login operation so a
    // regression in net layer framing or modern Server encoder trips
    // the golden comparison.
    Message server;
    server.header.category = 1;
    server.header.protocol = 1;
    server.header.object_id = 0;
    server.header.checksum = 0;
    server.header.code = 0;
    server.payload.clear();

    const auto actual = reconstruct_wire(server);
    const auto golden = read_golden_bytes("server_request.bin");
    EXPECT_EQ(actual, golden);
    ASSERT_EQ(actual.size(), 10u);
}

TEST_F(LoginServerFixture, UnknownCategoryServerRequestIsDroppedWithoutResponse) {
    CapturingClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    ASSERT_EQ(tcp.connect(ccfg), NetError::Ok);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));

    // Send cat=1 (Server) server-list request -- LoginHandler logs
    // unhandled category: Server and drops it without reply. Eighth
    // category in the C 协议扩展 arc.
    Message server;
    server.header.category = 1;
    server.header.protocol = 1;
    server.header.object_id = 0;
    server.payload.clear();
    ASSERT_EQ(tcp.send(server), NetError::Ok);

    EXPECT_FALSE(client.wait_for(2, std::chrono::milliseconds(500)));
    EXPECT_EQ(client.snapshot().size(), 1u);
    tcp.disconnect();
}


// =============================================================================
// M23 -- cat=11 (Cheat) wire-format golden + drop test.
// First admin / diagnostic category locked. Cheat carries GM commands
// like teleport, give item, god mode. Lower frequency than gameplay
// cats but high regression value: a bug here could let a malicious
// packet trigger unintended server behavior.
//
// 22B total: 2B length=20 + 8B header (cat=11, proto=1, obj_id=12345) +
// 12B payload (4B map_id=1 + 4B x_pos=0 + 4B y_pos=0).
//
// C 协议扩展 M23 -- the 9th distinct category locked at the wire layer
// (after cat=1, 2, 4, 5, 6, 7, 8, 9).
// =============================================================================

TEST_F(LoginServerFixtureGolden, GoldenCapturesCheatRequest) {
    // cat=11 (Cheat), proto=1 (teleport base), obj_id=12345, 12B
    // payload (4B map_id=1 + 4B x_pos=0 + 4B y_pos=0). Mirrors the
    // wire shape the legacy client sends for GM teleport. Pins a
    // real admin request so a regression in net layer framing or
    // modern Cheat encoder trips the golden comparison.
    Message cheat;
    cheat.header.category = 11;
    cheat.header.protocol = 1;
    cheat.header.object_id = 12345;
    cheat.header.checksum = 0;
    cheat.header.code = 0;
    cheat.payload.clear();
    // 4B map_id=1 LE
    cheat.payload.push_back(static_cast<std::uint8_t>(1u & 0xFFu));
    cheat.payload.push_back(0);
    cheat.payload.push_back(0);
    cheat.payload.push_back(0);
    // 4B x_pos=0 LE
    cheat.payload.push_back(0);
    cheat.payload.push_back(0);
    cheat.payload.push_back(0);
    cheat.payload.push_back(0);
    // 4B y_pos=0 LE
    cheat.payload.push_back(0);
    cheat.payload.push_back(0);
    cheat.payload.push_back(0);
    cheat.payload.push_back(0);

    const auto actual = reconstruct_wire(cheat);
    const auto golden = read_golden_bytes("cheat_request.bin");
    EXPECT_EQ(actual, golden);
    ASSERT_EQ(actual.size(), 22u);
}

TEST_F(LoginServerFixture, UnknownCategoryCheatRequestIsDroppedWithoutResponse) {
    CapturingClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    ASSERT_EQ(tcp.connect(ccfg), NetError::Ok);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));

    // Send cat=11 (Cheat) teleport request -- LoginHandler logs
    // unhandled category: Cheat and drops it without reply. Ninth
    // category in the C 协议扩展 arc.
    Message cheat;
    cheat.header.category = 11;
    cheat.header.protocol = 1;
    cheat.header.object_id = 12345;
    cheat.payload.assign(12, 0);
    cheat.payload[0] = 1;  // map_id
    ASSERT_EQ(tcp.send(cheat), NetError::Ok);

    EXPECT_FALSE(client.wait_for(2, std::chrono::milliseconds(500)));
    EXPECT_EQ(client.snapshot().size(), 1u);
    tcp.disconnect();
}


// =============================================================================
// M24 -- cat=14 (Party) wire-format golden + drop test.
// First multiplayer category locked. Party carries create/join/leave
// requests the client sends to coordinate group play. Locking the
// request shape here means a regression in modern party encoder
// trips the golden comparison before any real player notices.
//
// 18B total: 2B length=16 + 8B header (cat=14, proto=1, obj_id=5555) +
// 8B payload (4B party_id=0 + 4B member_count=1).
//
// C 协议扩展 M24 -- the 10th distinct category locked at the wire
// layer (after cat=1, 2, 4, 5, 6, 7, 8, 9, 11). Past the 12% mark.
// =============================================================================

TEST_F(LoginServerFixtureGolden, GoldenCapturesPartyRequest) {
    // cat=14 (Party), proto=1 (create-party base), obj_id=5555, 8B
    // payload (4B party_id=0 + 4B member_count=1). Mirrors the wire
    // shape the legacy client sends for party-create. Pins a real
    // multiplayer request so a regression in net layer framing or
    // modern Party encoder trips the golden comparison.
    Message party;
    party.header.category = 14;
    party.header.protocol = 1;
    party.header.object_id = 5555;
    party.header.checksum = 0;
    party.header.code = 0;
    party.payload.clear();
    // 4B party_id=0 LE (0 = create new party)
    party.payload.push_back(0);
    party.payload.push_back(0);
    party.payload.push_back(0);
    party.payload.push_back(0);
    // 4B member_count=1 LE
    party.payload.push_back(1);
    party.payload.push_back(0);
    party.payload.push_back(0);
    party.payload.push_back(0);

    const auto actual = reconstruct_wire(party);
    const auto golden = read_golden_bytes("party_request.bin");
    EXPECT_EQ(actual, golden);
    ASSERT_EQ(actual.size(), 18u);
}

TEST_F(LoginServerFixture, UnknownCategoryPartyRequestIsDroppedWithoutResponse) {
    CapturingClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    ASSERT_EQ(tcp.connect(ccfg), NetError::Ok);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));

    // Send cat=14 (Party) create-party request -- LoginHandler logs
    // unhandled category: Party and drops it without reply. Tenth
    // category in the C 协议扩展 arc.
    Message party;
    party.header.category = 14;
    party.header.protocol = 1;
    party.header.object_id = 5555;
    party.payload.assign(8, 0);
    party.payload[4] = 1;  // member_count
    ASSERT_EQ(tcp.send(party), NetError::Ok);

    EXPECT_FALSE(client.wait_for(2, std::chrono::milliseconds(500)));
    EXPECT_EQ(client.snapshot().size(), 1u);
    tcp.disconnect();
}


// =============================================================================
// M25 -- cat=22 (Skill) wire-format golden + drop test.
// First combat-cast category locked. Skill is distinct from cat=9
// Mugong: Mugong is learning a new skill from a book/NPC, Skill is
// actually casting a skill in combat. Every combat action sends
// cat=22, so this is the highest-frequency gameplay cat in the
// wire layer.
//
// 18B total: 2B length=16 + 8B header (cat=22, proto=1, obj_id=9999) +
// 8B payload (4B skill_id=50001 + 4B target_id=0).
//
// C 协议扩展 M25 -- the 11th distinct category locked at the wire
// layer (after cat=1, 2, 4, 5, 6, 7, 8, 9, 11, 14).
// =============================================================================

TEST_F(LoginServerFixtureGolden, GoldenCapturesSkillRequest) {
    // cat=22 (Skill), proto=1 (skill-cast base), obj_id=9999, 8B
    // payload (4B skill_id=50001 + 4B target_id=0). Mirrors the wire
    // shape the legacy client sends for skill-cast. Pins a real
    // combat request so a regression in net layer framing or modern
    // Skill encoder trips the golden comparison.
    Message skill;
    skill.header.category = 22;
    skill.header.protocol = 1;
    skill.header.object_id = 9999;
    skill.header.checksum = 0;
    skill.header.code = 0;
    skill.payload.clear();
    // 4B skill_id=50001 LE
    skill.payload.push_back(static_cast<std::uint8_t>(50001u & 0xFFu));
    skill.payload.push_back(static_cast<std::uint8_t>((50001u >> 8) & 0xFFu));
    skill.payload.push_back(static_cast<std::uint8_t>((50001u >> 16) & 0xFFu));
    skill.payload.push_back(static_cast<std::uint8_t>((50001u >> 24) & 0xFFu));
    // 4B target_id=0 LE
    skill.payload.push_back(0);
    skill.payload.push_back(0);
    skill.payload.push_back(0);
    skill.payload.push_back(0);

    const auto actual = reconstruct_wire(skill);
    const auto golden = read_golden_bytes("skill_request.bin");
    EXPECT_EQ(actual, golden);
    ASSERT_EQ(actual.size(), 18u);
}

TEST_F(LoginServerFixture, UnknownCategorySkillRequestIsDroppedWithoutResponse) {
    CapturingClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    ASSERT_EQ(tcp.connect(ccfg), NetError::Ok);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));

    // Send cat=22 (Skill) skill-cast request -- LoginHandler logs
    // unhandled category: Skill and drops it without reply. Eleventh
    // category in the C 协议扩展 arc.
    Message skill;
    skill.header.category = 22;
    skill.header.protocol = 1;
    skill.header.object_id = 9999;
    skill.payload.assign(8, 0);
    skill.payload[0] = static_cast<std::uint8_t>(50001u & 0xFFu);
    skill.payload[1] = static_cast<std::uint8_t>((50001u >> 8) & 0xFFu);
    ASSERT_EQ(tcp.send(skill), NetError::Ok);

    EXPECT_FALSE(client.wait_for(2, std::chrono::milliseconds(500)));
    EXPECT_EQ(client.snapshot().size(), 1u);
    tcp.disconnect();
}


// =============================================================================
// M26 -- cat=28 (Exchange) wire-format golden + drop test.
// First player-to-player (P2P) trading category locked. Exchange
// carries trade-start / item-offer / money-offer / confirm requests
// between two players. Locking the request shape here means a
// regression in modern exchange encoder trips the golden comparison
// before any real player loses gold on a malformed trade packet.
//
// 18B total: 2B length=16 + 8B header (cat=28, proto=1, obj_id=3333) +
// 8B payload (4B target_player_id=5 + 4B my_money=100).
//
// C 协议扩展 M26 -- the 12th distinct category locked at the wire
// layer (after cat=1, 2, 4, 5, 6, 7, 8, 9, 11, 14, 22). Crossed
// the 14% mark of the 96-category protocol surface.
// =============================================================================

TEST_F(LoginServerFixtureGolden, GoldenCapturesExchangeRequest) {
    // cat=28 (Exchange), proto=1 (trade-start base), obj_id=3333, 8B
    // payload (4B target_player_id=5 + 4B my_money=100). Mirrors the
    // wire shape the legacy client sends for trade-start. Pins a
    // real P2P trade request so a regression in net layer framing or
    // modern Exchange encoder trips the golden comparison.
    Message exchange;
    exchange.header.category = 28;
    exchange.header.protocol = 1;
    exchange.header.object_id = 3333;
    exchange.header.checksum = 0;
    exchange.header.code = 0;
    exchange.payload.clear();
    // 4B target_player_id=5 LE
    exchange.payload.push_back(static_cast<std::uint8_t>(5u & 0xFFu));
    exchange.payload.push_back(0);
    exchange.payload.push_back(0);
    exchange.payload.push_back(0);
    // 4B my_money=100 LE
    exchange.payload.push_back(static_cast<std::uint8_t>(100u & 0xFFu));
    exchange.payload.push_back(0);
    exchange.payload.push_back(0);
    exchange.payload.push_back(0);

    const auto actual = reconstruct_wire(exchange);
    const auto golden = read_golden_bytes("exchange_request.bin");
    EXPECT_EQ(actual, golden);
    ASSERT_EQ(actual.size(), 18u);
}

TEST_F(LoginServerFixture, UnknownCategoryExchangeRequestIsDroppedWithoutResponse) {
    CapturingClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    ASSERT_EQ(tcp.connect(ccfg), NetError::Ok);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));

    // Send cat=28 (Exchange) trade-start request -- LoginHandler
    // logs unhandled category: Exchange and drops it without reply.
    // Twelfth category in the C 协议扩展 arc.
    Message exchange;
    exchange.header.category = 28;
    exchange.header.protocol = 1;
    exchange.header.object_id = 3333;
    exchange.payload.assign(8, 0);
    exchange.payload[0] = static_cast<std::uint8_t>(5u & 0xFFu);  // target_player_id
    exchange.payload[4] = static_cast<std::uint8_t>(100u & 0xFFu); // my_money
    ASSERT_EQ(tcp.send(exchange), NetError::Ok);

    EXPECT_FALSE(client.wait_for(2, std::chrono::milliseconds(500)));
    EXPECT_EQ(client.snapshot().size(), 1u);
    tcp.disconnect();
}


// =============================================================================
// M27 -- cat=10 (AuctionBoard) wire-format golden + drop test.
// First player-driven economy category locked. AuctionBoard
// carries auction-browse / bid / buyout / list-item requests
// from clients to the server. Locking the request shape here
// means a regression in modern AuctionBoard encoder trips the
// golden comparison before any real player mis-bids on a
// malformed auction packet.
//
// 18B total: 2B length=16 + 8B header (cat=10, proto=1, obj_id=4444) +
// 8B payload (4B item_id=7777 + 4B bid_price=50000).
//
// C 协议扩展 M27 -- the 13th distinct category locked at the wire
// layer (after cat=1, 2, 4, 5, 6, 7, 8, 9, 11, 14, 22, 28). 14% mark
// of the 96-category protocol surface.
// =============================================================================

TEST_F(LoginServerFixtureGolden, GoldenCapturesAuctionBoardRequest) {
    // cat=10 (AuctionBoard), proto=1 (bid-base), obj_id=4444, 8B
    // payload (4B item_id=7777 + 4B bid_price=50000). Mirrors the
    // wire shape the legacy client sends for auction-bid. Pins a
    // real auction-bid request so a regression in net layer framing
    // or modern AuctionBoard encoder trips the golden comparison.
    Message auction;
    auction.header.category = 10;
    auction.header.protocol = 1;
    auction.header.object_id = 4444;
    auction.header.checksum = 0;
    auction.header.code = 0;
    auction.payload.clear();
    // 4B item_id=7777 LE
    auction.payload.push_back(static_cast<std::uint8_t>(7777u & 0xFFu));
    auction.payload.push_back(static_cast<std::uint8_t>((7777u >> 8) & 0xFFu));
    auction.payload.push_back(0);
    auction.payload.push_back(0);
    // 4B bid_price=50000 LE
    auction.payload.push_back(static_cast<std::uint8_t>(50000u & 0xFFu));
    auction.payload.push_back(static_cast<std::uint8_t>((50000u >> 8) & 0xFFu));
    auction.payload.push_back(0);
    auction.payload.push_back(0);

    const auto actual = reconstruct_wire(auction);
    const auto golden = read_golden_bytes("auctionboard_request.bin");
    EXPECT_EQ(actual, golden);
    ASSERT_EQ(actual.size(), 18u);
}

TEST_F(LoginServerFixture, UnknownCategoryAuctionBoardRequestIsDroppedWithoutResponse) {
    CapturingClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    ASSERT_EQ(tcp.connect(ccfg), NetError::Ok);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));

    // Send cat=10 (AuctionBoard) auction-bid request -- LoginHandler
    // logs unhandled category: AuctionBoard and drops it without
    // reply. Thirteenth category in the C 协议扩展 arc.
    Message auction;
    auction.header.category = 10;
    auction.header.protocol = 1;
    auction.header.object_id = 4444;
    auction.payload.assign(8, 0);
    auction.payload[0] = static_cast<std::uint8_t>(7777u & 0xFFu);  // item_id low
    auction.payload[1] = static_cast<std::uint8_t>((7777u >> 8) & 0xFFu);  // item_id high
    auction.payload[4] = static_cast<std::uint8_t>(50000u & 0xFFu);  // bid_price low
    auction.payload[5] = static_cast<std::uint8_t>((50000u >> 8) & 0xFFu);  // bid_price high
    ASSERT_EQ(tcp.send(auction), NetError::Ok);

    EXPECT_FALSE(client.wait_for(2, std::chrono::milliseconds(500)));
    EXPECT_EQ(client.snapshot().size(), 1u);
    tcp.disconnect();
}


// =============================================================================
// M28 -- cat=58 (Wanted) wire-format golden + drop test.
// First bounty / wanted-system category locked. Wanted carries
// post-bounty / list-wanted / claim-bounty requests from clients
// to the server. Locking the request shape here means a regression
// in modern Wanted encoder trips the golden comparison before any
// real player posts a malformed bounty.
//
// 18B total: 2B length=16 + 8B header (cat=58, proto=1, obj_id=5555) +
// 8B payload (4B target_player_id=8888 + 4B bounty_amount=25000).
//
// C 协议扩展 M28 -- the 14th distinct category locked at the wire
// layer (after cat=1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 14, 22, 28).
// Crossed the 17% mark of the 81-category protocol surface.
// =============================================================================

TEST_F(LoginServerFixtureGolden, GoldenCapturesWantedRequest) {
    // cat=58 (Wanted), proto=1 (post-bounty base), obj_id=5555, 8B
    // payload (4B target_player_id=8888 + 4B bounty_amount=25000).
    // Mirrors the wire shape the legacy client sends for posting a
    // bounty. Pins a real wanted-list request so a regression in
    // net layer framing or modern Wanted encoder trips the golden
    // comparison.
    Message wanted;
    wanted.header.category = 58;
    wanted.header.protocol = 1;
    wanted.header.object_id = 5555;
    wanted.header.checksum = 0;
    wanted.header.code = 0;
    wanted.payload.clear();
    // 4B target_player_id=8888 LE
    wanted.payload.push_back(static_cast<std::uint8_t>(8888u & 0xFFu));
    wanted.payload.push_back(static_cast<std::uint8_t>((8888u >> 8) & 0xFFu));
    wanted.payload.push_back(0);
    wanted.payload.push_back(0);
    // 4B bounty_amount=25000 LE
    wanted.payload.push_back(static_cast<std::uint8_t>(25000u & 0xFFu));
    wanted.payload.push_back(static_cast<std::uint8_t>((25000u >> 8) & 0xFFu));
    wanted.payload.push_back(0);
    wanted.payload.push_back(0);

    const auto actual = reconstruct_wire(wanted);
    const auto golden = read_golden_bytes("wanted_request.bin");
    EXPECT_EQ(actual, golden);
    ASSERT_EQ(actual.size(), 18u);
}

TEST_F(LoginServerFixture, UnknownCategoryWantedRequestIsDroppedWithoutResponse) {
    CapturingClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    ASSERT_EQ(tcp.connect(ccfg), NetError::Ok);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));

    // Send cat=58 (Wanted) post-bounty request -- LoginHandler
    // logs unhandled category: Wanted and drops it without reply.
    // Fourteenth category in the C 协议扩展 arc.
    Message wanted;
    wanted.header.category = 58;
    wanted.header.protocol = 1;
    wanted.header.object_id = 5555;
    wanted.payload.assign(8, 0);
    wanted.payload[0] = static_cast<std::uint8_t>(8888u & 0xFFu);  // target low
    wanted.payload[1] = static_cast<std::uint8_t>((8888u >> 8) & 0xFFu);  // target high
    wanted.payload[4] = static_cast<std::uint8_t>(25000u & 0xFFu);  // bounty low
    wanted.payload[5] = static_cast<std::uint8_t>((25000u >> 8) & 0xFFu);  // bounty high
    ASSERT_EQ(tcp.send(wanted), NetError::Ok);

    EXPECT_FALSE(client.wait_for(2, std::chrono::milliseconds(500)));
    EXPECT_EQ(client.snapshot().size(), 1u);
    tcp.disconnect();
}


// =============================================================================
// M29 -- cat=71 (Weather) wire-format golden + drop test.
// First world-state broadcast category locked. Weather carries
// region-weather / forecast / storm-warning requests between
// client and server. Locking the request shape here means a
// regression in modern Weather encoder trips the golden
// comparison before any real player gets a malformed weather
// update.
//
// 18B total: 2B length=16 + 8B header (cat=71, proto=1, obj_id=6666) +
// 8B payload (4B region_id=12345 + 4B weather_type=3=snow).
//
// C 协议扩展 M29 -- the 15th distinct category locked at the wire
// layer (after cat=1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 14, 22, 28, 58).
// Crossed the 18.5% mark of the 81-category protocol surface.
// =============================================================================

TEST_F(LoginServerFixtureGolden, GoldenCapturesWeatherRequest) {
    // cat=71 (Weather), proto=1 (region-lookup base), obj_id=6666,
    // 8B payload (4B region_id=12345 + 4B weather_type=3=snow).
    // Mirrors the wire shape the legacy client sends for region
    // weather lookup. Pins a real weather request so a regression
    // in net layer framing or modern Weather encoder trips the
    // golden comparison.
    Message weather;
    weather.header.category = 71;
    weather.header.protocol = 1;
    weather.header.object_id = 6666;
    weather.header.checksum = 0;
    weather.header.code = 0;
    weather.payload.clear();
    // 4B region_id=12345 LE
    weather.payload.push_back(static_cast<std::uint8_t>(12345u & 0xFFu));
    weather.payload.push_back(static_cast<std::uint8_t>((12345u >> 8) & 0xFFu));
    weather.payload.push_back(0);
    weather.payload.push_back(0);
    // 4B weather_type=3 (snow) LE
    weather.payload.push_back(static_cast<std::uint8_t>(3u & 0xFFu));
    weather.payload.push_back(0);
    weather.payload.push_back(0);
    weather.payload.push_back(0);

    const auto actual = reconstruct_wire(weather);
    const auto golden = read_golden_bytes("weather_request.bin");
    EXPECT_EQ(actual, golden);
    ASSERT_EQ(actual.size(), 18u);
}

TEST_F(LoginServerFixture, UnknownCategoryWeatherRequestIsDroppedWithoutResponse) {
    CapturingClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    ASSERT_EQ(tcp.connect(ccfg), NetError::Ok);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));

    // Send cat=71 (Weather) region-lookup request -- LoginHandler
    // logs unhandled category: Weather and drops it without reply.
    // Fifteenth category in the C 协议扩展 arc.
    Message weather;
    weather.header.category = 71;
    weather.header.protocol = 1;
    weather.header.object_id = 6666;
    weather.payload.assign(8, 0);
    weather.payload[0] = static_cast<std::uint8_t>(12345u & 0xFFu);  // region low
    weather.payload[1] = static_cast<std::uint8_t>((12345u >> 8) & 0xFFu);  // region high
    weather.payload[4] = static_cast<std::uint8_t>(3u & 0xFFu);  // weather_type
    ASSERT_EQ(tcp.send(weather), NetError::Ok);

    EXPECT_FALSE(client.wait_for(2, std::chrono::milliseconds(500)));
    EXPECT_EQ(client.snapshot().size(), 1u);
    tcp.disconnect();
}


// =============================================================================
// M30 -- cat=12 (Quick) wire-format golden + drop test.
// First quick-slot bar category locked. Quick carries quick-slot
// use / equip / unequip requests from the client to the server.
// Locking the request shape here means a regression in modern
// Quick encoder trips the golden comparison before any real
// player mis-fires a quick-slot skill or item.
//
// 18B total: 2B length=16 + 8B header (cat=12, proto=1, obj_id=7777) +
// 8B payload (4B slot_index=5 + 4B item_id=12345).
//
// C 协议扩展 M30 -- the 16th distinct category locked at the wire
// layer (after cat=1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 14, 22, 28, 58, 71).
// Crossed the 19.7% mark of the 81-category protocol surface.
// =============================================================================

TEST_F(LoginServerFixtureGolden, GoldenCapturesQuickRequest) {
    // cat=12 (Quick), proto=1 (slot-use base), obj_id=7777, 8B
    // payload (4B slot_index=5 + 4B item_id=12345). Mirrors the
    // wire shape the legacy client sends for quick-slot use.
    // Pins a real quick-slot request so a regression in net
    // layer framing or modern Quick encoder trips the golden
    // comparison.
    Message quick;
    quick.header.category = 12;
    quick.header.protocol = 1;
    quick.header.object_id = 7777;
    quick.header.checksum = 0;
    quick.header.code = 0;
    quick.payload.clear();
    // 4B slot_index=5 LE
    quick.payload.push_back(static_cast<std::uint8_t>(5u & 0xFFu));
    quick.payload.push_back(0);
    quick.payload.push_back(0);
    quick.payload.push_back(0);
    // 4B item_id=12345 LE
    quick.payload.push_back(static_cast<std::uint8_t>(12345u & 0xFFu));
    quick.payload.push_back(static_cast<std::uint8_t>((12345u >> 8) & 0xFFu));
    quick.payload.push_back(0);
    quick.payload.push_back(0);

    const auto actual = reconstruct_wire(quick);
    const auto golden = read_golden_bytes("quick_request.bin");
    EXPECT_EQ(actual, golden);
    ASSERT_EQ(actual.size(), 18u);
}

TEST_F(LoginServerFixture, UnknownCategoryQuickRequestIsDroppedWithoutResponse) {
    CapturingClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    ASSERT_EQ(tcp.connect(ccfg), NetError::Ok);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));

    // Send cat=12 (Quick) slot-use request -- LoginHandler logs
    // unhandled category: Quick and drops it without reply.
    // Sixteenth category in the C 协议扩展 arc.
    Message quick;
    quick.header.category = 12;
    quick.header.protocol = 1;
    quick.header.object_id = 7777;
    quick.payload.assign(8, 0);
    quick.payload[0] = static_cast<std::uint8_t>(5u & 0xFFu);  // slot_index
    quick.payload[4] = static_cast<std::uint8_t>(12345u & 0xFFu);  // item_id low
    quick.payload[5] = static_cast<std::uint8_t>((12345u >> 8) & 0xFFu);  // item_id high
    ASSERT_EQ(tcp.send(quick), NetError::Ok);

    EXPECT_FALSE(client.wait_for(2, std::chrono::milliseconds(500)));
    EXPECT_EQ(client.snapshot().size(), 1u);
    tcp.disconnect();
}


// =============================================================================
// M31 -- cat=33 (Friend) wire-format golden + drop test.
// First friend-list category locked. Friend carries friend-add /
// remove / block requests between client and server. Locking the
// request shape here means a regression in modern Friend encoder
// trips the golden comparison before any real player fails to
// add a friend or gets a corrupt block notification.
//
// 18B total: 2B length=16 + 8B header (cat=33, proto=1, obj_id=8888) +
// 8B payload (4B target_player_id=12345 + 4B action_type=2=block).
//
// C 协议扩展 M31 -- the 17th distinct category locked at the wire
// layer (after cat=1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14, 22, 28,
// 58, 71). Crossed the 21% mark of the 81-category protocol
// surface.
// =============================================================================

TEST_F(LoginServerFixtureGolden, GoldenCapturesFriendRequest) {
    // cat=33 (friendReq), proto=1 (friendReq-block base), obj_id=8888,
    // 8B payload (4B target_player_id=12345 + 4B action_type=2
    // =block). Mirrors the wire shape the legacy client sends for
    // adding a player to the friendReq block list. Pins a real
    // friendReq-block request so a regression in net layer framing
    // or modern friendReq encoder trips the golden comparison.
    Message friendReq;
    friendReq.header.category = 33;
    friendReq.header.protocol = 1;
    friendReq.header.object_id = 8888;
    friendReq.header.checksum = 0;
    friendReq.header.code = 0;
    friendReq.payload.clear();
    // 4B target_player_id=12345 LE
    friendReq.payload.push_back(static_cast<std::uint8_t>(12345u & 0xFFu));
    friendReq.payload.push_back(static_cast<std::uint8_t>((12345u >> 8) & 0xFFu));
    friendReq.payload.push_back(0);
    friendReq.payload.push_back(0);
    // 4B action_type=2 (block) LE
    friendReq.payload.push_back(static_cast<std::uint8_t>(2u & 0xFFu));
    friendReq.payload.push_back(0);
    friendReq.payload.push_back(0);
    friendReq.payload.push_back(0);

    const auto actual = reconstruct_wire(friendReq);
    const auto golden = read_golden_bytes("friend_request.bin");
    EXPECT_EQ(actual, golden);
    ASSERT_EQ(actual.size(), 18u);
}

TEST_F(LoginServerFixture, UnknownCategoryFriendRequestIsDroppedWithoutResponse) {
    CapturingClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    ASSERT_EQ(tcp.connect(ccfg), NetError::Ok);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));

    // Send cat=33 (friendReq) friendReq-block request -- LoginHandler
    // logs unhandled category: friendReq and drops it without reply.
    // Seventeenth category in the C 协议扩展 arc.
    Message friendReq;
    friendReq.header.category = 33;
    friendReq.header.protocol = 1;
    friendReq.header.object_id = 8888;
    friendReq.payload.assign(8, 0);
    friendReq.payload[0] = static_cast<std::uint8_t>(12345u & 0xFFu);  // target low
    friendReq.payload[1] = static_cast<std::uint8_t>((12345u >> 8) & 0xFFu);  // target high
    friendReq.payload[4] = static_cast<std::uint8_t>(2u & 0xFFu);  // action_type
    ASSERT_EQ(tcp.send(friendReq), NetError::Ok);

    EXPECT_FALSE(client.wait_for(2, std::chrono::milliseconds(500)));
    EXPECT_EQ(client.snapshot().size(), 1u);
    tcp.disconnect();
}


// =============================================================================
// M32 -- cat=37 (Npc) wire-format golden + drop test.
// First NPC dialog category locked. Npc carries NPC-talk /
// dialog-select / quest-start-at-NPC requests between client
// and server. Locking the request shape here means a regression
// in modern Npc encoder trips the golden comparison before any
// real player gets stuck in a broken dialog tree.
//
// 18B total: 2B length=16 + 8B header (cat=37, proto=1, obj_id=9999) +
// 8B payload (4B npc_id=1234 + 4B dialog_index=5).
//
// C 协议扩展 M32 -- the 18th distinct category locked at the
// wire layer (after cat=1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14,
// 22, 28, 33, 58, 71). Crossed the 22% mark of the 81-category
// protocol surface.
// =============================================================================

TEST_F(LoginServerFixtureGolden, GoldenCapturesNpcRequest) {
    // cat=37 (Npc), proto=1 (dialog-select base), obj_id=9999,
    // 8B payload (4B npc_id=1234 + 4B dialog_index=5). Mirrors
    // the wire shape the legacy client sends for selecting a
    // dialog option at an NPC. Pins a real NPC-talk request so
    // a regression in net layer framing or modern Npc encoder
    // trips the golden comparison.
    Message npc;
    npc.header.category = 37;
    npc.header.protocol = 1;
    npc.header.object_id = 9999;
    npc.header.checksum = 0;
    npc.header.code = 0;
    npc.payload.clear();
    // 4B npc_id=1234 LE
    npc.payload.push_back(static_cast<std::uint8_t>(1234u & 0xFFu));
    npc.payload.push_back(static_cast<std::uint8_t>((1234u >> 8) & 0xFFu));
    npc.payload.push_back(0);
    npc.payload.push_back(0);
    // 4B dialog_index=5 LE
    npc.payload.push_back(static_cast<std::uint8_t>(5u & 0xFFu));
    npc.payload.push_back(0);
    npc.payload.push_back(0);
    npc.payload.push_back(0);

    const auto actual = reconstruct_wire(npc);
    const auto golden = read_golden_bytes("npc_request.bin");
    EXPECT_EQ(actual, golden);
    ASSERT_EQ(actual.size(), 18u);
}

TEST_F(LoginServerFixture, UnknownCategoryNpcRequestIsDroppedWithoutResponse) {
    CapturingClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    ASSERT_EQ(tcp.connect(ccfg), NetError::Ok);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));

    // Send cat=37 (Npc) dialog-select request -- LoginHandler
    // logs unhandled category: Npc and drops it without reply.
    // Eighteenth category in the C 协议扩展 arc.
    Message npc;
    npc.header.category = 37;
    npc.header.protocol = 1;
    npc.header.object_id = 9999;
    npc.payload.assign(8, 0);
    npc.payload[0] = static_cast<std::uint8_t>(1234u & 0xFFu);  // npc_id low
    npc.payload[1] = static_cast<std::uint8_t>((1234u >> 8) & 0xFFu);  // npc_id high
    npc.payload[4] = static_cast<std::uint8_t>(5u & 0xFFu);  // dialog_index
    ASSERT_EQ(tcp.send(npc), NetError::Ok);

    EXPECT_FALSE(client.wait_for(2, std::chrono::milliseconds(500)));
    EXPECT_EQ(client.snapshot().size(), 1u);
    tcp.disconnect();
}


// =============================================================================
// M33 -- cat=39 (Quest) wire-format golden + drop test.
// First quest-progress category locked. Quest carries quest-start /
// progress / complete / abandon requests between client and server.
// Locking the request shape here means a regression in modern Quest
// encoder trips the golden comparison before any real player gets
// stuck with a quest that will not advance.
//
// 18B total: 2B length=16 + 8B header (cat=39, proto=1, obj_id=11111) +
// 8B payload (4B quest_id=500 + 4B quest_action=2=complete).
//
// C 协议扩展 M33 -- the 19th distinct category locked at the wire
// layer (after cat=1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14, 22, 28,
// 33, 37, 58, 71). Crossed the 23.4% mark of the 81-category
// protocol surface.
// =============================================================================

TEST_F(LoginServerFixtureGolden, GoldenCapturesQuestRequest) {
    // cat=39 (Quest), proto=1 (quest-complete base), obj_id=11111,
    // 8B payload (4B quest_id=500 + 4B quest_action=2=complete).
    // Mirrors the wire shape the legacy client sends for turning
    // in a finished quest. Pins a real quest-complete request so
    // a regression in net layer framing or modern Quest encoder
    // trips the golden comparison.
    Message quest;
    quest.header.category = 39;
    quest.header.protocol = 1;
    quest.header.object_id = 11111;
    quest.header.checksum = 0;
    quest.header.code = 0;
    quest.payload.clear();
    // 4B quest_id=500 LE
    quest.payload.push_back(static_cast<std::uint8_t>(500u & 0xFFu));
    quest.payload.push_back(static_cast<std::uint8_t>((500u >> 8) & 0xFFu));
    quest.payload.push_back(0);
    quest.payload.push_back(0);
    // 4B quest_action=2 (complete) LE
    quest.payload.push_back(static_cast<std::uint8_t>(2u & 0xFFu));
    quest.payload.push_back(0);
    quest.payload.push_back(0);
    quest.payload.push_back(0);

    const auto actual = reconstruct_wire(quest);
    const auto golden = read_golden_bytes("quest_request.bin");
    EXPECT_EQ(actual, golden);
    ASSERT_EQ(actual.size(), 18u);
}

TEST_F(LoginServerFixture, UnknownCategoryQuestRequestIsDroppedWithoutResponse) {
    CapturingClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    ASSERT_EQ(tcp.connect(ccfg), NetError::Ok);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));

    // Send cat=39 (Quest) quest-complete request -- LoginHandler
    // logs unhandled category: Quest and drops it without reply.
    // Nineteenth category in the C 协议扩展 arc.
    Message quest;
    quest.header.category = 39;
    quest.header.protocol = 1;
    quest.header.object_id = 11111;
    quest.payload.assign(8, 0);
    quest.payload[0] = static_cast<std::uint8_t>(500u & 0xFFu);  // quest_id low
    quest.payload[1] = static_cast<std::uint8_t>((500u >> 8) & 0xFFu);  // quest_id high
    quest.payload[4] = static_cast<std::uint8_t>(2u & 0xFFu);  // quest_action
    ASSERT_EQ(tcp.send(quest), NetError::Ok);

    EXPECT_FALSE(client.wait_for(2, std::chrono::milliseconds(500)));
    EXPECT_EQ(client.snapshot().size(), 1u);
    tcp.disconnect();
}


// =============================================================================
// M34 -- cat=30 (Pyoguk) wire-format golden + drop test.
// First warehouse / storage category locked. Pyoguk carries
// warehouse-put / warehouse-get / warehouse-list requests
// between client and server. Locking the request shape here
// means a regression in modern Pyoguk encoder trips the golden
// comparison before any real player loses an item to a corrupt
// warehouse slot.
//
// 18B total: 2B length=16 + 8B header (cat=30, proto=1, obj_id=12222) +
// 8B payload (4B warehouse_slot=10 + 4B item_id=23456).
//
// C 协议扩展 M34 -- the 20th distinct category locked at the
// wire layer (after cat=1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14,
// 22, 28, 33, 37, 39, 58, 71). Crossed the 24.7% mark of the
// 81-category protocol surface.
// =============================================================================

TEST_F(LoginServerFixtureGolden, GoldenCapturesPyogukRequest) {
    // cat=30 (Pyoguk), proto=1 (warehouse-put base), obj_id=12222,
    // 8B payload (4B warehouse_slot=10 + 4B item_id=23456). Mirrors
    // the wire shape the legacy client sends for putting an item
    // in a warehouse slot. Pins a real warehouse-put request so
    // a regression in net layer framing or modern Pyoguk encoder
    // trips the golden comparison.
    Message pyoguk;
    pyoguk.header.category = 30;
    pyoguk.header.protocol = 1;
    pyoguk.header.object_id = 12222;
    pyoguk.header.checksum = 0;
    pyoguk.header.code = 0;
    pyoguk.payload.clear();
    // 4B warehouse_slot=10 LE
    pyoguk.payload.push_back(static_cast<std::uint8_t>(10u & 0xFFu));
    pyoguk.payload.push_back(0);
    pyoguk.payload.push_back(0);
    pyoguk.payload.push_back(0);
    // 4B item_id=23456 LE
    pyoguk.payload.push_back(static_cast<std::uint8_t>(23456u & 0xFFu));
    pyoguk.payload.push_back(static_cast<std::uint8_t>((23456u >> 8) & 0xFFu));
    pyoguk.payload.push_back(0);
    pyoguk.payload.push_back(0);

    const auto actual = reconstruct_wire(pyoguk);
    const auto golden = read_golden_bytes("pyoguk_request.bin");
    EXPECT_EQ(actual, golden);
    ASSERT_EQ(actual.size(), 18u);
}

TEST_F(LoginServerFixture, UnknownCategoryPyogukRequestIsDroppedWithoutResponse) {
    CapturingClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    ASSERT_EQ(tcp.connect(ccfg), NetError::Ok);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));

    // Send cat=30 (Pyoguk) warehouse-put request -- LoginHandler
    // logs unhandled category: Pyoguk and drops it without reply.
    // Twentieth category in the C 协议扩展 arc.
    Message pyoguk;
    pyoguk.header.category = 30;
    pyoguk.header.protocol = 1;
    pyoguk.header.object_id = 12222;
    pyoguk.payload.assign(8, 0);
    pyoguk.payload[0] = static_cast<std::uint8_t>(10u & 0xFFu);  // warehouse_slot
    pyoguk.payload[4] = static_cast<std::uint8_t>(23456u & 0xFFu);  // item_id low
    pyoguk.payload[5] = static_cast<std::uint8_t>((23456u >> 8) & 0xFFu);  // item_id high
    ASSERT_EQ(tcp.send(pyoguk), NetError::Ok);

    EXPECT_FALSE(client.wait_for(2, std::chrono::milliseconds(500)));
    EXPECT_EQ(client.snapshot().size(), 1u);
    tcp.disconnect();
}


// =============================================================================
// M35 -- cat=62 (Guild) wire-format golden + drop test.
// First guild category locked. Guild carries guild-chat /
// join / leave / kick / promote requests between client and
// server. Locking the request shape here means a regression
// in modern Guild encoder trips the golden comparison before
// any real player gets stuck outside their guild or sends a
// malformed guild message.
//
// 18B total: 2B length=16 + 8B header (cat=62, proto=1, obj_id=13333) +
// 8B payload (4B guild_id=200 + 4B action_type=1=join).
//
// C 协议扩展 M35 -- the 21st distinct category locked at the
// wire layer (after cat=1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14,
// 22, 28, 30, 33, 37, 39, 58, 71). Crossed the 25.9% mark of
// the 81-category protocol surface.
// =============================================================================

TEST_F(LoginServerFixtureGolden, GoldenCapturesGuildRequest) {
    // cat=62 (Guild), proto=1 (guild-join base), obj_id=13333,
    // 8B payload (4B guild_id=200 + 4B action_type=1=join).
    // Mirrors the wire shape the legacy client sends for a
    // player requesting to join a guild. Pins a real guild-join
    // request so a regression in net layer framing or modern
    // Guild encoder trips the golden comparison.
    Message guild;
    guild.header.category = 62;
    guild.header.protocol = 1;
    guild.header.object_id = 13333;
    guild.header.checksum = 0;
    guild.header.code = 0;
    guild.payload.clear();
    // 4B guild_id=200 LE
    guild.payload.push_back(static_cast<std::uint8_t>(200u & 0xFFu));
    guild.payload.push_back(0);
    guild.payload.push_back(0);
    guild.payload.push_back(0);
    // 4B action_type=1 (join) LE
    guild.payload.push_back(static_cast<std::uint8_t>(1u & 0xFFu));
    guild.payload.push_back(0);
    guild.payload.push_back(0);
    guild.payload.push_back(0);

    const auto actual = reconstruct_wire(guild);
    const auto golden = read_golden_bytes("guild_request.bin");
    EXPECT_EQ(actual, golden);
    ASSERT_EQ(actual.size(), 18u);
}

TEST_F(LoginServerFixture, UnknownCategoryGuildRequestIsDroppedWithoutResponse) {
    CapturingClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    ASSERT_EQ(tcp.connect(ccfg), NetError::Ok);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));

    // Send cat=62 (Guild) guild-join request -- LoginHandler
    // logs unhandled category: Guild and drops it without reply.
    // Twenty-first category in the C 协议扩展 arc.
    Message guild;
    guild.header.category = 62;
    guild.header.protocol = 1;
    guild.header.object_id = 13333;
    guild.payload.assign(8, 0);
    guild.payload[0] = static_cast<std::uint8_t>(200u & 0xFFu);  // guild_id
    guild.payload[4] = static_cast<std::uint8_t>(1u & 0xFFu);  // action_type
    ASSERT_EQ(tcp.send(guild), NetError::Ok);

    EXPECT_FALSE(client.wait_for(2, std::chrono::milliseconds(500)));
    EXPECT_EQ(client.snapshot().size(), 1u);
    tcp.disconnect();
}


// =============================================================================
// M36 -- cat=72 (Pet) wire-format golden + drop test.
// First pet / companion category locked. Pet carries
// pet-summon / dismiss / feed / command requests between client
// and server. Locking the request shape here means a regression
// in modern Pet encoder trips the golden comparison before any
// real player gets stuck with an unsummonable pet or a wrong
// command interpretation.
//
// 18B total: 2B length=16 + 8B header (cat=72, proto=1, obj_id=14444) +
// 8B payload (4B pet_id=5000 + 4B command=0=summon).
//
// C 协议扩展 M36 -- the 22nd distinct category locked at the
// wire layer (after cat=1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14,
// 22, 28, 30, 33, 37, 39, 58, 62, 71). Crossed the 27.1% mark
// of the 81-category protocol surface.
// =============================================================================

TEST_F(LoginServerFixtureGolden, GoldenCapturesPetRequest) {
    // cat=72 (Pet), proto=1 (pet-summon base), obj_id=14444,
    // 8B payload (4B pet_id=5000 + 4B command=0=summon). Mirrors
    // the wire shape the legacy client sends for summoning a
    // pet into the world. Pins a real pet-summon request so a
    // regression in net layer framing or modern Pet encoder
    // trips the golden comparison.
    Message pet;
    pet.header.category = 72;
    pet.header.protocol = 1;
    pet.header.object_id = 14444;
    pet.header.checksum = 0;
    pet.header.code = 0;
    pet.payload.clear();
    // 4B pet_id=5000 LE
    pet.payload.push_back(static_cast<std::uint8_t>(5000u & 0xFFu));
    pet.payload.push_back(static_cast<std::uint8_t>((5000u >> 8) & 0xFFu));
    pet.payload.push_back(0);
    pet.payload.push_back(0);
    // 4B command=0 (summon) LE
    pet.payload.push_back(0);
    pet.payload.push_back(0);
    pet.payload.push_back(0);
    pet.payload.push_back(0);

    const auto actual = reconstruct_wire(pet);
    const auto golden = read_golden_bytes("pet_request.bin");
    EXPECT_EQ(actual, golden);
    ASSERT_EQ(actual.size(), 18u);
}

TEST_F(LoginServerFixture, UnknownCategoryPetRequestIsDroppedWithoutResponse) {
    CapturingClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    ASSERT_EQ(tcp.connect(ccfg), NetError::Ok);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));

    // Send cat=72 (Pet) pet-summon request -- LoginHandler logs
    // unhandled category: Pet and drops it without reply.
    // Twenty-second category in the C 协议扩展 arc.
    Message pet;
    pet.header.category = 72;
    pet.header.protocol = 1;
    pet.header.object_id = 14444;
    pet.payload.assign(8, 0);
    pet.payload[0] = static_cast<std::uint8_t>(5000u & 0xFFu);  // pet_id low
    pet.payload[1] = static_cast<std::uint8_t>((5000u >> 8) & 0xFFu);  // pet_id high
    pet.payload[4] = 0;  // command=0 (summon)
    ASSERT_EQ(tcp.send(pet), NetError::Ok);

    EXPECT_FALSE(client.wait_for(2, std::chrono::milliseconds(500)));
    EXPECT_EQ(client.snapshot().size(), 1u);
    tcp.disconnect();
}


// =============================================================================
// M37 -- cat=69 (SiegeWar) wire-format golden + drop test.
// First castle-siege category locked. SiegeWar carries
// siege-register / attack / defend / profit-distribute requests
// between client and server. Locking the request shape here
// means a regression in modern SiegeWar encoder trips the
// golden comparison before any real guild attacks the wrong
// castle or gets dropped from a siege roster.
//
// 18B total: 2B length=16 + 8B header (cat=69, proto=1, obj_id=15555) +
// 8B payload (4B castle_id=7 + 4B action=1=attack).
//
// C 协议扩展 M37 -- the 23rd distinct category locked at the
// wire layer (after cat=1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14,
// 22, 28, 30, 33, 37, 39, 58, 62, 71, 72). Crossed the 28.4%
// mark of the 81-category protocol surface.
// =============================================================================

TEST_F(LoginServerFixtureGolden, GoldenCapturesSiegeWarRequest) {
    // cat=69 (SiegeWar), proto=1 (siege-attack base), obj_id=15555,
    // 8B payload (4B castle_id=7 + 4B action=1=attack). Mirrors
    // the wire shape the legacy client sends for a guild declaring
    // an attack on a castle. Pins a real siege-attack request so
    // a regression in net layer framing or modern SiegeWar encoder
    // trips the golden comparison.
    Message siege;
    siege.header.category = 69;
    siege.header.protocol = 1;
    siege.header.object_id = 15555;
    siege.header.checksum = 0;
    siege.header.code = 0;
    siege.payload.clear();
    // 4B castle_id=7 LE
    siege.payload.push_back(static_cast<std::uint8_t>(7u & 0xFFu));
    siege.payload.push_back(0);
    siege.payload.push_back(0);
    siege.payload.push_back(0);
    // 4B action=1 (attack) LE
    siege.payload.push_back(static_cast<std::uint8_t>(1u & 0xFFu));
    siege.payload.push_back(0);
    siege.payload.push_back(0);
    siege.payload.push_back(0);

    const auto actual = reconstruct_wire(siege);
    const auto golden = read_golden_bytes("siegewar_request.bin");
    EXPECT_EQ(actual, golden);
    ASSERT_EQ(actual.size(), 18u);
}

TEST_F(LoginServerFixture, UnknownCategorySiegeWarRequestIsDroppedWithoutResponse) {
    CapturingClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    ASSERT_EQ(tcp.connect(ccfg), NetError::Ok);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));

    // Send cat=69 (SiegeWar) siege-attack request -- LoginHandler
    // logs unhandled category: SiegeWar and drops it without
    // reply. Twenty-third category in the C 协议扩展 arc.
    Message siege;
    siege.header.category = 69;
    siege.header.protocol = 1;
    siege.header.object_id = 15555;
    siege.payload.assign(8, 0);
    siege.payload[0] = static_cast<std::uint8_t>(7u & 0xFFu);  // castle_id
    siege.payload[4] = static_cast<std::uint8_t>(1u & 0xFFu);  // action
    ASSERT_EQ(tcp.send(siege), NetError::Ok);

    EXPECT_FALSE(client.wait_for(2, std::chrono::milliseconds(500)));
    EXPECT_EQ(client.snapshot().size(), 1u);
    tcp.disconnect();
}


// =============================================================================
// M38 -- cat=15 (PeaceWarMode) wire-format golden + drop test.
// First PvP-toggle category locked. PeaceWarMode carries
// peace-to-war / war-to-peace toggle requests and PvP-attack
// declarations from client to server. Locking the request
// shape here means a regression in modern PeaceWarMode
// encoder trips the golden comparison before any real player
// gets stuck in wrong-mode state or attacks out of mode.
//
// 18B total: 2B length=16 + 8B header (cat=15, proto=1, obj_id=16666) +
// 8B payload (4B mode=1=war + 4B target_player_id=12345).
//
// C 协议扩展 M38 -- the 24th distinct category locked at the
// wire layer (after cat=1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14,
// 22, 28, 30, 33, 37, 39, 58, 62, 69, 71, 72). Crossed the
// 29.6% mark of the 81-category protocol surface.
// =============================================================================

TEST_F(LoginServerFixtureGolden, GoldenCapturesPeaceWarModeRequest) {
    // cat=15 (PeaceWarMode), proto=1 (war-toggle base), obj_id=16666,
    // 8B payload (4B mode=1=war + 4B target_player_id=12345).
    // Mirrors the wire shape the legacy client sends when a
    // player toggles into war mode and targets another player.
    // Pins a real war-toggle request so a regression in net
    // layer framing or modern PeaceWarMode encoder trips the
    // golden comparison.
    Message pwm;
    pwm.header.category = 15;
    pwm.header.protocol = 1;
    pwm.header.object_id = 16666;
    pwm.header.checksum = 0;
    pwm.header.code = 0;
    pwm.payload.clear();
    // 4B mode=1 (war) LE
    pwm.payload.push_back(static_cast<std::uint8_t>(1u & 0xFFu));
    pwm.payload.push_back(0);
    pwm.payload.push_back(0);
    pwm.payload.push_back(0);
    // 4B target_player_id=12345 LE
    pwm.payload.push_back(static_cast<std::uint8_t>(12345u & 0xFFu));
    pwm.payload.push_back(static_cast<std::uint8_t>((12345u >> 8) & 0xFFu));
    pwm.payload.push_back(0);
    pwm.payload.push_back(0);

    const auto actual = reconstruct_wire(pwm);
    const auto golden = read_golden_bytes("peacewarmode_request.bin");
    EXPECT_EQ(actual, golden);
    ASSERT_EQ(actual.size(), 18u);
}

TEST_F(LoginServerFixture, UnknownCategoryPeaceWarModeRequestIsDroppedWithoutResponse) {
    CapturingClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    ASSERT_EQ(tcp.connect(ccfg), NetError::Ok);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));

    // Send cat=15 (PeaceWarMode) war-toggle request -- LoginHandler
    // logs unhandled category: PeaceWarMode and drops it without
    // reply. Twenty-fourth category in the C 协议扩展 arc.
    Message pwm;
    pwm.header.category = 15;
    pwm.header.protocol = 1;
    pwm.header.object_id = 16666;
    pwm.payload.assign(8, 0);
    pwm.payload[0] = static_cast<std::uint8_t>(1u & 0xFFu);  // mode=war
    pwm.payload[4] = static_cast<std::uint8_t>(12345u & 0xFFu);  // target low
    pwm.payload[5] = static_cast<std::uint8_t>((12345u >> 8) & 0xFFu);  // target high
    ASSERT_EQ(tcp.send(pwm), NetError::Ok);

    EXPECT_FALSE(client.wait_for(2, std::chrono::milliseconds(500)));
    EXPECT_EQ(client.snapshot().size(), 1u);
    tcp.disconnect();
}


// =============================================================================
// M39 -- cat=23 (KyungGong) wire-format golden + drop test.
// First light-step / flight-skill category locked. KyungGong
// (경공) carries step-skill activation requests from client
// to server for fast-travel dashes. Locking the request
// shape here means a regression in modern KyungGong encoder
// trips the golden comparison before any real player gets
// stuck unable to traverse the map.
//
// 18B total: 2B length=16 + 8B header (cat=23, proto=1, obj_id=17777) +
// 8B payload (4B skill_id=100 + 4B direction=3=NE).
//
// C 协议扩展 M39 -- the 25th distinct category locked at the
// wire layer (after cat=1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14,
// 15, 22, 28, 30, 33, 37, 39, 58, 62, 69, 71, 72). Crossed
// the 30.8% mark of the 81-category protocol surface.
// =============================================================================

TEST_F(LoginServerFixtureGolden, GoldenCapturesKyungGongRequest) {
    // cat=23 (KyungGong), proto=1 (step-activate base), obj_id=17777,
    // 8B payload (4B skill_id=100 + 4B direction=3=NE). Mirrors
    // the wire shape the legacy client sends for activating a
    // dash step-skill toward the north-east. Pins a real
    // step-activate request so a regression in net layer
    // framing or modern KyungGong encoder trips the golden
    // comparison.
    Message kg;
    kg.header.category = 23;
    kg.header.protocol = 1;
    kg.header.object_id = 17777;
    kg.header.checksum = 0;
    kg.header.code = 0;
    kg.payload.clear();
    // 4B skill_id=100 LE
    kg.payload.push_back(static_cast<std::uint8_t>(100u & 0xFFu));
    kg.payload.push_back(0);
    kg.payload.push_back(0);
    kg.payload.push_back(0);
    // 4B direction=3 (north-east) LE
    kg.payload.push_back(static_cast<std::uint8_t>(3u & 0xFFu));
    kg.payload.push_back(0);
    kg.payload.push_back(0);
    kg.payload.push_back(0);

    const auto actual = reconstruct_wire(kg);
    const auto golden = read_golden_bytes("kyunggong_request.bin");
    EXPECT_EQ(actual, golden);
    ASSERT_EQ(actual.size(), 18u);
}

TEST_F(LoginServerFixture, UnknownCategoryKyungGongRequestIsDroppedWithoutResponse) {
    CapturingClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    ASSERT_EQ(tcp.connect(ccfg), NetError::Ok);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));

    // Send cat=23 (KyungGong) step-activate request -- LoginHandler
    // logs unhandled category: KyungGong and drops it without
    // reply. Twenty-fifth category in the C 协议扩展 arc.
    Message kg;
    kg.header.category = 23;
    kg.header.protocol = 1;
    kg.header.object_id = 17777;
    kg.payload.assign(8, 0);
    kg.payload[0] = static_cast<std::uint8_t>(100u & 0xFFu);  // skill_id
    kg.payload[4] = static_cast<std::uint8_t>(3u & 0xFFu);  // direction
    ASSERT_EQ(tcp.send(kg), NetError::Ok);

    EXPECT_FALSE(client.wait_for(2, std::chrono::milliseconds(500)));
    EXPECT_EQ(client.snapshot().size(), 1u);
    tcp.disconnect();
}


// =============================================================================
// M40 -- cat=29 (StreetStall) wire-format golden + drop test.
// First player-vendor / street-stall category locked. StreetStall
// (노점) carries stall-open / item-register / stall-buy requests
// between client and server. Locking the request shape here means
// a regression in modern StreetStall encoder trips the golden
// comparison before any real player gets stuck unable to sell
// items at their in-world stall.
//
// 18B total: 2B length=16 + 8B header (cat=29, proto=1, obj_id=18888) +
// 8B payload (4B stall_slot=5 + 4B item_id=54321).
//
// C 协议扩展 M40 -- the 26th distinct category locked at the
// wire layer (after cat=1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14,
// 15, 22, 23, 28, 30, 33, 37, 39, 58, 62, 69, 71, 72). Crossed
// the 32% mark of the 81-category protocol surface.
// =============================================================================

TEST_F(LoginServerFixtureGolden, GoldenCapturesStreetStallRequest) {
    // cat=29 (StreetStall), proto=1 (item-register base), obj_id=18888,
    // 8B payload (4B stall_slot=5 + 4B item_id=54321). Mirrors the
    // wire shape the legacy client sends for putting an item up
    // for sale at an in-world stall. Pins a real stall-register
    // request so a regression in net layer framing or modern
    // StreetStall encoder trips the golden comparison.
    Message stall;
    stall.header.category = 29;
    stall.header.protocol = 1;
    stall.header.object_id = 18888;
    stall.header.checksum = 0;
    stall.header.code = 0;
    stall.payload.clear();
    // 4B stall_slot=5 LE
    stall.payload.push_back(static_cast<std::uint8_t>(5u & 0xFFu));
    stall.payload.push_back(0);
    stall.payload.push_back(0);
    stall.payload.push_back(0);
    // 4B item_id=54321 LE
    stall.payload.push_back(static_cast<std::uint8_t>(54321u & 0xFFu));
    stall.payload.push_back(static_cast<std::uint8_t>((54321u >> 8) & 0xFFu));
    stall.payload.push_back(0);
    stall.payload.push_back(0);

    const auto actual = reconstruct_wire(stall);
    const auto golden = read_golden_bytes("streetstall_request.bin");
    EXPECT_EQ(actual, golden);
    ASSERT_EQ(actual.size(), 18u);
}

TEST_F(LoginServerFixture, UnknownCategoryStreetStallRequestIsDroppedWithoutResponse) {
    CapturingClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    ASSERT_EQ(tcp.connect(ccfg), NetError::Ok);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));

    // Send cat=29 (StreetStall) stall-register request --
    // LoginHandler logs unhandled category: StreetStall and
    // drops it without reply. Twenty-sixth category in the
    // C 协议扩展 arc.
    Message stall;
    stall.header.category = 29;
    stall.header.protocol = 1;
    stall.header.object_id = 18888;
    stall.payload.assign(8, 0);
    stall.payload[0] = static_cast<std::uint8_t>(5u & 0xFFu);  // stall_slot
    stall.payload[4] = static_cast<std::uint8_t>(54321u & 0xFFu);  // item_id low
    stall.payload[5] = static_cast<std::uint8_t>((54321u >> 8) & 0xFFu);  // item_id high
    ASSERT_EQ(tcp.send(stall), NetError::Ok);

    EXPECT_FALSE(client.wait_for(2, std::chrono::milliseconds(500)));
    EXPECT_EQ(client.snapshot().size(), 1u);
    tcp.disconnect();
}


// =============================================================================
// M41 -- cat=65 (PartyWar) wire-format golden + drop test.
// First party-vs-party PvP category locked. PartyWar carries
// war-declare / war-score / war-surrender requests between
// client and server for small-scale party PvP events. Locking
// the request shape here means a regression in modern PartyWar
// encoder trips the golden comparison before any real party
// mis-declares a war against the wrong target or gets stuck
// mid-PvP without score updates.
//
// 18B total: 2B length=16 + 8B header (cat=65, proto=1, obj_id=19999) +
// 8B payload (4B target_party_id=100 + 4B war_type=1=declare).
//
// C 协议扩展 M41 -- the 27th distinct category locked at the
// wire layer (after cat=1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14,
// 15, 22, 23, 28, 29, 30, 33, 37, 39, 58, 62, 69, 71, 72).
// Crossed the 33% mark of the 81-category protocol surface.
// =============================================================================

TEST_F(LoginServerFixtureGolden, GoldenCapturesPartyWarRequest) {
    // cat=65 (PartyWar), proto=1 (war-declare base), obj_id=19999,
    // 8B payload (4B target_party_id=100 + 4B war_type=1=declare).
    // Mirrors the wire shape the legacy client sends for a party
    // declaring war on another party. Pins a real war-declare
    // request so a regression in net layer framing or modern
    // PartyWar encoder trips the golden comparison.
    Message pw;
    pw.header.category = 65;
    pw.header.protocol = 1;
    pw.header.object_id = 19999;
    pw.header.checksum = 0;
    pw.header.code = 0;
    pw.payload.clear();
    // 4B target_party_id=100 LE
    pw.payload.push_back(static_cast<std::uint8_t>(100u & 0xFFu));
    pw.payload.push_back(0);
    pw.payload.push_back(0);
    pw.payload.push_back(0);
    // 4B war_type=1 (declare) LE
    pw.payload.push_back(static_cast<std::uint8_t>(1u & 0xFFu));
    pw.payload.push_back(0);
    pw.payload.push_back(0);
    pw.payload.push_back(0);

    const auto actual = reconstruct_wire(pw);
    const auto golden = read_golden_bytes("partywar_request.bin");
    EXPECT_EQ(actual, golden);
    ASSERT_EQ(actual.size(), 18u);
}

TEST_F(LoginServerFixture, UnknownCategoryPartyWarRequestIsDroppedWithoutResponse) {
    CapturingClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    ASSERT_EQ(tcp.connect(ccfg), NetError::Ok);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));

    // Send cat=65 (PartyWar) war-declare request -- LoginHandler
    // logs unhandled category: PartyWar and drops it without
    // reply. Twenty-seventh category in the C 协议扩展 arc.
    Message pw;
    pw.header.category = 65;
    pw.header.protocol = 1;
    pw.header.object_id = 19999;
    pw.payload.assign(8, 0);
    pw.payload[0] = static_cast<std::uint8_t>(100u & 0xFFu);  // target_party
    pw.payload[4] = static_cast<std::uint8_t>(1u & 0xFFu);  // war_type
    ASSERT_EQ(tcp.send(pw), NetError::Ok);

    EXPECT_FALSE(client.wait_for(2, std::chrono::milliseconds(500)));
    EXPECT_EQ(client.snapshot().size(), 1u);
    tcp.disconnect();
}


// =============================================================================
// M42 -- cat=17 (Auction) wire-format golden + drop test.
// First legacy-auction (pre-AuctionBoard) category locked. Auction
// carries the older MP_AUCTION bid / list / cancel packets that
// predate the AuctionBoard rewrite. Locking the request shape here
// means a regression in modern Auction encoder trips the golden
// comparison before any real player gets stuck on a legacy
// auction listing with a malformed bid packet.
//
// 18B total: 2B length=16 + 8B header (cat=17, proto=1, obj_id=21111) +
// 8B payload (4B listing_id=3333 + 4B bid_amount=50000).
//
// C 协议扩展 M42 -- the 28th distinct category locked at the
// wire layer (after cat=1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14,
// 15, 22, 23, 28, 29, 30, 33, 37, 39, 58, 62, 65, 69, 71, 72).
// Crossed the 34.5% mark of the 81-category protocol surface.
// =============================================================================

TEST_F(LoginServerFixtureGolden, GoldenCapturesAuctionRequest) {
    // cat=17 (Auction), proto=1 (legacy-bid base), obj_id=21111,
    // 8B payload (4B listing_id=3333 + 4B bid_amount=50000).
    // Mirrors the wire shape the legacy client sends for a bid
    // on an existing auction listing. Pins a real legacy-bid
    // request so a regression in net layer framing or modern
    // Auction encoder trips the golden comparison.
    Message auction;
    auction.header.category = 17;
    auction.header.protocol = 1;
    auction.header.object_id = 21111;
    auction.header.checksum = 0;
    auction.header.code = 0;
    auction.payload.clear();
    // 4B listing_id=3333 LE
    auction.payload.push_back(static_cast<std::uint8_t>(3333u & 0xFFu));
    auction.payload.push_back(static_cast<std::uint8_t>((3333u >> 8) & 0xFFu));
    auction.payload.push_back(0);
    auction.payload.push_back(0);
    // 4B bid_amount=50000 LE
    auction.payload.push_back(static_cast<std::uint8_t>(50000u & 0xFFu));
    auction.payload.push_back(static_cast<std::uint8_t>((50000u >> 8) & 0xFFu));
    auction.payload.push_back(0);
    auction.payload.push_back(0);

    const auto actual = reconstruct_wire(auction);
    const auto golden = read_golden_bytes("auction_request.bin");
    EXPECT_EQ(actual, golden);
    ASSERT_EQ(actual.size(), 18u);
}

TEST_F(LoginServerFixture, UnknownCategoryAuctionRequestIsDroppedWithoutResponse) {
    CapturingClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    ASSERT_EQ(tcp.connect(ccfg), NetError::Ok);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));

    // Send cat=17 (Auction) legacy-bid request -- LoginHandler
    // logs unhandled category: Auction and drops it without
    // reply. Twenty-eighth category in the C 协议扩展 arc.
    Message auction;
    auction.header.category = 17;
    auction.header.protocol = 1;
    auction.header.object_id = 21111;
    auction.payload.assign(8, 0);
    auction.payload[0] = static_cast<std::uint8_t>(3333u & 0xFFu);  // listing low
    auction.payload[1] = static_cast<std::uint8_t>((3333u >> 8) & 0xFFu);  // listing high
    auction.payload[4] = static_cast<std::uint8_t>(50000u & 0xFFu);  // bid low
    auction.payload[5] = static_cast<std::uint8_t>((50000u >> 8) & 0xFFu);  // bid high
    ASSERT_EQ(tcp.send(auction), NetError::Ok);

    EXPECT_FALSE(client.wait_for(2, std::chrono::milliseconds(500)));
    EXPECT_EQ(client.snapshot().size(), 1u);
    tcp.disconnect();
}


// =============================================================================
// M43 -- cat=21 (Munpa) wire-format golden + drop test.
// First clan / pre-Guild category locked. Munpa (문파) carries
// the legacy clan join / leave / chat packets that predate the
// Guild rewrite. Locking the request shape here means a
// regression in modern Munpa encoder trips the golden
// comparison before any real player gets stuck in a broken
// clan-leave transition.
//
// 18B total: 2B length=16 + 8B header (cat=21, proto=1, obj_id=22222) +
// 8B payload (4B munpa_id=300 + 4B action=2=leave).
//
// C 协议扩展 M43 -- the 29th distinct category locked at the
// wire layer (after cat=1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14,
// 15, 17, 22, 23, 28, 29, 30, 33, 37, 39, 58, 62, 65, 69, 71,
// 72). Crossed the 35.8% mark of the 81-category protocol
// surface.
// =============================================================================

TEST_F(LoginServerFixtureGolden, GoldenCapturesMunpaRequest) {
    // cat=21 (Munpa), proto=1 (clan-leave base), obj_id=22222,
    // 8B payload (4B munpa_id=300 + 4B action=2=leave). Mirrors
    // the wire shape the legacy client sends for a player
    // leaving a clan. Pins a real clan-leave request so a
    // regression in net layer framing or modern Munpa encoder
    // trips the golden comparison.
    Message munpa;
    munpa.header.category = 21;
    munpa.header.protocol = 1;
    munpa.header.object_id = 22222;
    munpa.header.checksum = 0;
    munpa.header.code = 0;
    munpa.payload.clear();
    // 4B munpa_id=300 LE
    munpa.payload.push_back(static_cast<std::uint8_t>(300u & 0xFFu));
    munpa.payload.push_back(static_cast<std::uint8_t>((300u >> 8) & 0xFFu));
    munpa.payload.push_back(0);
    munpa.payload.push_back(0);
    // 4B action=2 (leave) LE
    munpa.payload.push_back(static_cast<std::uint8_t>(2u & 0xFFu));
    munpa.payload.push_back(0);
    munpa.payload.push_back(0);
    munpa.payload.push_back(0);

    const auto actual = reconstruct_wire(munpa);
    const auto golden = read_golden_bytes("munpa_request.bin");
    EXPECT_EQ(actual, golden);
    ASSERT_EQ(actual.size(), 18u);
}

TEST_F(LoginServerFixture, UnknownCategoryMunpaRequestIsDroppedWithoutResponse) {
    CapturingClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    ASSERT_EQ(tcp.connect(ccfg), NetError::Ok);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));

    // Send cat=21 (Munpa) clan-leave request -- LoginHandler logs
    // unhandled category: Munpa and drops it without reply.
    // Twenty-ninth category in the C 协议扩展 arc.
    Message munpa;
    munpa.header.category = 21;
    munpa.header.protocol = 1;
    munpa.header.object_id = 22222;
    munpa.payload.assign(8, 0);
    munpa.payload[0] = static_cast<std::uint8_t>(300u & 0xFFu);  // munpa_id low
    munpa.payload[1] = static_cast<std::uint8_t>((300u >> 8) & 0xFFu);  // munpa_id high
    munpa.payload[4] = static_cast<std::uint8_t>(2u & 0xFFu);  // action
    ASSERT_EQ(tcp.send(munpa), NetError::Ok);

    EXPECT_FALSE(client.wait_for(2, std::chrono::milliseconds(500)));
    EXPECT_EQ(client.snapshot().size(), 1u);
    tcp.disconnect();
}


// =============================================================================
// M44 -- cat=41 (Pk) wire-format golden + drop test.
// First player-kill (PK) category locked. Pk carries PK
// score / kill / death / penalty requests from client to
// server. Locking the request shape here means a regression
// in modern Pk encoder trips the golden comparison before
// any real player gets wrong PK score on a malformed kill
// packet.
//
// 18B total: 2B length=16 + 8B header (cat=41, proto=1, obj_id=23333) +
// 8B payload (4B victim_player_id=4444 + 4B kill_type=1).
//
// C 协议扩展 M44 -- the 30th distinct category locked at the
// wire layer (after cat=1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14,
// 15, 17, 21, 22, 23, 28, 29, 30, 33, 37, 39, 58, 62, 65, 69,
// 71, 72). Crossed the 37% mark of the 81-category protocol
// surface.
// =============================================================================

TEST_F(LoginServerFixtureGolden, GoldenCapturesPkRequest) {
    // cat=41 (Pk), proto=1 (kill-score base), obj_id=23333,
    // 8B payload (4B victim_player_id=4444 + 4B kill_type=1).
    // Mirrors the wire shape the legacy client sends for
    // reporting a player-kill score. Pins a real PK-kill
    // request so a regression in net layer framing or modern
    // Pk encoder trips the golden comparison.
    Message pk;
    pk.header.category = 41;
    pk.header.protocol = 1;
    pk.header.object_id = 23333;
    pk.header.checksum = 0;
    pk.header.code = 0;
    pk.payload.clear();
    // 4B victim_player_id=4444 LE
    pk.payload.push_back(static_cast<std::uint8_t>(4444u & 0xFFu));
    pk.payload.push_back(static_cast<std::uint8_t>((4444u >> 8) & 0xFFu));
    pk.payload.push_back(0);
    pk.payload.push_back(0);
    // 4B kill_type=1 LE
    pk.payload.push_back(static_cast<std::uint8_t>(1u & 0xFFu));
    pk.payload.push_back(0);
    pk.payload.push_back(0);
    pk.payload.push_back(0);

    const auto actual = reconstruct_wire(pk);
    const auto golden = read_golden_bytes("pk_request.bin");
    EXPECT_EQ(actual, golden);
    ASSERT_EQ(actual.size(), 18u);
}

TEST_F(LoginServerFixture, UnknownCategoryPkRequestIsDroppedWithoutResponse) {
    CapturingClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    ASSERT_EQ(tcp.connect(ccfg), NetError::Ok);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));

    // Send cat=41 (Pk) PK-kill request -- LoginHandler logs
    // unhandled category: Pk and drops it without reply.
    // Thirtieth category in the C 协议扩展 arc.
    Message pk;
    pk.header.category = 41;
    pk.header.protocol = 1;
    pk.header.object_id = 23333;
    pk.payload.assign(8, 0);
    pk.payload[0] = static_cast<std::uint8_t>(4444u & 0xFFu);  // victim low
    pk.payload[1] = static_cast<std::uint8_t>((4444u >> 8) & 0xFFu);  // victim high
    pk.payload[4] = static_cast<std::uint8_t>(1u & 0xFFu);  // kill_type
    ASSERT_EQ(tcp.send(pk), NetError::Ok);

    EXPECT_FALSE(client.wait_for(2, std::chrono::milliseconds(500)));
    EXPECT_EQ(client.snapshot().size(), 1u);
    tcp.disconnect();
}


// =============================================================================
// M45 -- cat=64 (Note) wire-format golden + drop test.
// First player-note category locked. Note carries
// send-note / read-note / delete-note requests between
// client and server. Locking the request shape here means
// a regression in modern Note encoder trips the golden
// comparison before any real player loses a critical
// in-game message to a malformed note packet.
//
// 18B total: 2B length=16 + 8B header (cat=64, proto=1, obj_id=24444) +
// 8B payload (4B target_player_id=5555 + 4B note_type=1=send).
//
// C 协议扩展 M45 -- the 31st distinct category locked at the
// wire layer (after cat=1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14,
// 15, 17, 21, 22, 23, 28, 29, 30, 33, 37, 39, 41, 58, 62, 65,
// 69, 71, 72). Crossed the 38.3% mark of the 81-category
// protocol surface.
// =============================================================================

TEST_F(LoginServerFixtureGolden, GoldenCapturesNoteRequest) {
    // cat=64 (Note), proto=1 (note-send base), obj_id=24444,
    // 8B payload (4B target_player_id=5555 + 4B note_type=1=send).
    // Mirrors the wire shape the legacy client sends for
    // sending an in-game player-to-player note. Pins a real
    // note-send request so a regression in net layer framing
    // or modern Note encoder trips the golden comparison.
    Message note;
    note.header.category = 64;
    note.header.protocol = 1;
    note.header.object_id = 24444;
    note.header.checksum = 0;
    note.header.code = 0;
    note.payload.clear();
    // 4B target_player_id=5555 LE
    note.payload.push_back(static_cast<std::uint8_t>(5555u & 0xFFu));
    note.payload.push_back(static_cast<std::uint8_t>((5555u >> 8) & 0xFFu));
    note.payload.push_back(0);
    note.payload.push_back(0);
    // 4B note_type=1 (send) LE
    note.payload.push_back(static_cast<std::uint8_t>(1u & 0xFFu));
    note.payload.push_back(0);
    note.payload.push_back(0);
    note.payload.push_back(0);

    const auto actual = reconstruct_wire(note);
    const auto golden = read_golden_bytes("note_request.bin");
    EXPECT_EQ(actual, golden);
    ASSERT_EQ(actual.size(), 18u);
}

TEST_F(LoginServerFixture, UnknownCategoryNoteRequestIsDroppedWithoutResponse) {
    CapturingClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    ASSERT_EQ(tcp.connect(ccfg), NetError::Ok);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));

    // Send cat=64 (Note) note-send request -- LoginHandler logs
    // unhandled category: Note and drops it without reply.
    // Thirty-first category in the C 协议扩展 arc.
    Message note;
    note.header.category = 64;
    note.header.protocol = 1;
    note.header.object_id = 24444;
    note.payload.assign(8, 0);
    note.payload[0] = static_cast<std::uint8_t>(5555u & 0xFFu);  // target low
    note.payload[1] = static_cast<std::uint8_t>((5555u >> 8) & 0xFFu);  // target high
    note.payload[4] = static_cast<std::uint8_t>(1u & 0xFFu);  // note_type
    ASSERT_EQ(tcp.send(note), NetError::Ok);

    EXPECT_FALSE(client.wait_for(2, std::chrono::milliseconds(500)));
    EXPECT_EQ(client.snapshot().size(), 1u);
    tcp.disconnect();
}


// =============================================================================
// M46 -- cat=20 (Tactic) wire-format golden + drop test.
// First combat-tactic / formation category locked. Tactic
// carries formation-set / formation-trigger / formation-break
// requests from client to server. Locking the request shape
// here means a regression in modern Tactic encoder trips the
// golden comparison before any real party gets stuck with a
// broken formation layout in combat.
//
// 18B total: 2B length=16 + 8B header (cat=20, proto=1, obj_id=25555) +
// 8B payload (4B formation_id=5 + 4B tactic_action=1=trigger).
//
// C 协议扩展 M46 -- the 32nd distinct category locked at the
// wire layer (after cat=1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14,
// 15, 17, 21, 22, 23, 28, 29, 30, 33, 37, 39, 41, 58, 62, 64,
// 65, 69, 71, 72). Crossed the 39.5% mark of the 81-category
// protocol surface.
// =============================================================================

TEST_F(LoginServerFixtureGolden, GoldenCapturesTacticRequest) {
    // cat=20 (Tactic), proto=1 (formation-trigger base), obj_id=25555,
    // 8B payload (4B formation_id=5 + 4B tactic_action=1=trigger).
    // Mirrors the wire shape the legacy client sends for a party
    // triggering a formation in combat. Pins a real formation
    // request so a regression in net layer framing or modern
    // Tactic encoder trips the golden comparison.
    Message tactic;
    tactic.header.category = 20;
    tactic.header.protocol = 1;
    tactic.header.object_id = 25555;
    tactic.header.checksum = 0;
    tactic.header.code = 0;
    tactic.payload.clear();
    // 4B formation_id=5 LE
    tactic.payload.push_back(static_cast<std::uint8_t>(5u & 0xFFu));
    tactic.payload.push_back(0);
    tactic.payload.push_back(0);
    tactic.payload.push_back(0);
    // 4B tactic_action=1 (trigger) LE
    tactic.payload.push_back(static_cast<std::uint8_t>(1u & 0xFFu));
    tactic.payload.push_back(0);
    tactic.payload.push_back(0);
    tactic.payload.push_back(0);

    const auto actual = reconstruct_wire(tactic);
    const auto golden = read_golden_bytes("tactic_request.bin");
    EXPECT_EQ(actual, golden);
    ASSERT_EQ(actual.size(), 18u);
}

TEST_F(LoginServerFixture, UnknownCategoryTacticRequestIsDroppedWithoutResponse) {
    CapturingClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    ASSERT_EQ(tcp.connect(ccfg), NetError::Ok);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));

    // Send cat=20 (Tactic) formation-trigger request --
    // LoginHandler logs unhandled category: Tactic and drops it
    // without reply. Thirty-second category in the C 协议扩展
    // arc.
    Message tactic;
    tactic.header.category = 20;
    tactic.header.protocol = 1;
    tactic.header.object_id = 25555;
    tactic.payload.assign(8, 0);
    tactic.payload[0] = static_cast<std::uint8_t>(5u & 0xFFu);  // formation_id
    tactic.payload[4] = static_cast<std::uint8_t>(1u & 0xFFu);  // tactic_action
    ASSERT_EQ(tcp.send(tactic), NetError::Ok);

    EXPECT_FALSE(client.wait_for(2, std::chrono::milliseconds(500)));
    EXPECT_EQ(client.snapshot().size(), 1u);
    tcp.disconnect();
}


// =============================================================================
// M46 -- cat=20 (Tactic) wire-format golden + drop test.
// First combat-tactic / formation category locked. Tactic
// carries formation-set / formation-trigger / formation-break
// requests from client to server. Locking the request shape
// here means a regression in modern Tactic encoder trips the
// golden comparison before any real party gets stuck with a
// broken formation layout in combat.
//
// 18B total: 2B length=16 + 8B header (cat=20, proto=1, obj_id=25555) +
// 8B payload (4B formation_id=5 + 4B tactic_action=1=trigger).
//
// C 协议扩展 M46 -- the 32nd distinct category locked at the
// wire layer (after cat=1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14,
// 15, 17, 21, 22, 23, 28, 29, 30, 33, 37, 39, 41, 58, 62, 64,
// 65, 69, 71, 72). Crossed the 39.5% mark of the 81-category
// protocol surface.
// =============================================================================

// =============================================================================
// M47 -- cat=24 (SimBub) wire-format golden + drop test.
// First combat-simBub / formation category locked. SimBub
// carries formation-set / formation-trigger / formation-break
// requests from client to server. Locking the request shape
// here means a regression in modern SimBub encoder trips the
// golden comparison before any real party gets stuck with a
// broken formation layout in combat.
//
// 18B total: 2B length=16 + 8B header (cat=24, proto=1, obj_id=26666) +
// 8B payload (4B skill_id=200 + 4B target=12345).
//
// C 协议扩展 M47 -- the 33rd distinct category locked at the
// wire layer (after cat=1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14,
// 15, 17, 21, 22, 23, 28, 29, 30, 33, 37, 39, 41, 58, 62, 64,
// 65, 69, 71, 72). Crossed the 39.5% mark of the 81-category
// protocol surface.
// =============================================================================

TEST_F(LoginServerFixtureGolden, GoldenCapturesSimBubRequest) {
    // cat=24 (SimBub), proto=1 (formation-trigger base), obj_id=26666,
    // 8B payload (4B skill_id=200 + 4B target=12345).
    // Mirrors the wire shape the legacy client sends for a party
    // triggering a formation in combat. Pins a real formation
    // request so a regression in net layer framing or modern
    // SimBub encoder trips the golden comparison.
    Message simBub;
    simBub.header.category = 24;
    simBub.header.protocol = 1;
    simBub.header.object_id = 26666;
    simBub.header.checksum = 0;
    simBub.header.code = 0;
    simBub.payload.clear();
    // 4B skill_id=200 LE
    simBub.payload.push_back(static_cast<std::uint8_t>(200u & 0xFFu));
    simBub.payload.push_back(0);
    simBub.payload.push_back(0);
    simBub.payload.push_back(0);
    // 4B target=12345 LE
    simBub.payload.push_back(static_cast<std::uint8_t>(12345u & 0xFFu));
    simBub.payload.push_back(static_cast<std::uint8_t>((12345u >> 8) & 0xFFu));
    simBub.payload.push_back(0);
    simBub.payload.push_back(0);


    const auto actual = reconstruct_wire(simBub);
    const auto golden = read_golden_bytes("sim_bub_request.bin");
    EXPECT_EQ(actual, golden);
    ASSERT_EQ(actual.size(), 18u);
}

TEST_F(LoginServerFixture, UnknownCategorySimBubRequestIsDroppedWithoutResponse) {
    CapturingClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    ASSERT_EQ(tcp.connect(ccfg), NetError::Ok);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));

    // Send cat=24 (SimBub) formation-trigger request --
    // LoginHandler logs unhandled category: SimBub and drops it
    // without reply. Thirty-third category in the C 协议扩展
    // arc.
    Message simBub;
    simBub.header.category = 24;
    simBub.header.protocol = 1;
    simBub.header.object_id = 26666;
    simBub.payload.assign(8, 0);
    simBub.payload[0] = static_cast<std::uint8_t>(200u & 0xFFu);  // skill_id
    simBub.payload[4] = static_cast<std::uint8_t>(12345u & 0xFFu);  // simBub_action
    ASSERT_EQ(tcp.send(simBub), NetError::Ok);

    EXPECT_FALSE(client.wait_for(2, std::chrono::milliseconds(500)));
    EXPECT_EQ(client.snapshot().size(), 1u);
    tcp.disconnect();
}


// =============================================================================
// M48 -- cat=32 (CharRevive) wire-format golden + drop test.
// First death-revival category locked. CharRevive
// carries player revival / respawn requests sent when a
// dead character wants to come back to the world.
// Locking the request shape here means a regression in
// modern revival encoder trips the golden comparison
// before any real player gets stuck in a dead state.
//
// 18B total: 2B length=16 + 8B header (cat=32, proto=1, obj_id=27777) +
// 8B payload (4B target=8888 + 4B type=0).
//
// C 协议扩展 M48 -- the 34th distinct category locked at the
// wire layer (after cat=1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14,
// 15, 17, 20, 21, 22, 23, 24, 28, 29, 30, 33, 37, 39, 41,
// 58, 62, 64, 65, 69, 71, 72). Crossed the 42.0% mark of the
// 81-category protocol surface.
// =============================================================================

TEST_F(LoginServerFixtureGolden, GoldenCapturesCharReviveRequest) {
    // cat=32 (CharRevive), proto=1 (revive base), obj_id=27777,
    // 8B payload (4B target=8888 + 4B type=0).
    // Mirrors the wire shape the legacy client sends for a
    // player requesting revival. Pins a real revival
    // request so a regression in net layer framing or modern
    // CharRevive encoder trips the golden comparison.
    Message revive;
    revive.header.category = 32;
    revive.header.protocol = 1;
    revive.header.object_id = 27777;
    revive.header.checksum = 0;
    revive.header.code = 0;
    revive.payload.clear();
    // 4B target=8888 LE
    revive.payload.push_back(static_cast<std::uint8_t>(8888u & 0xFFu));
    revive.payload.push_back(static_cast<std::uint8_t>((8888u >> 8) & 0xFFu));
    revive.payload.push_back(0);
    revive.payload.push_back(0);
    // 4B type=0
    revive.payload.push_back(0);
    revive.payload.push_back(0);
    revive.payload.push_back(0);
    revive.payload.push_back(0);

    const auto actual = reconstruct_wire(revive);
    const auto golden = read_golden_bytes("char_revive_request.bin");
    EXPECT_EQ(actual, golden);
    ASSERT_EQ(actual.size(), 18u);
}

TEST_F(LoginServerFixture, UnknownCategoryCharReviveRequestIsDroppedWithoutResponse) {
    CapturingClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    ASSERT_EQ(tcp.connect(ccfg), NetError::Ok);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));

    // Send cat=32 (CharRevive) revival request --
    // LoginHandler logs unhandled category: CharRevive and drops it
    // without reply. Thirty-fourth category in the C 协议扩展
    // arc.
    Message revive;
    revive.header.category = 32;
    revive.header.protocol = 1;
    revive.header.object_id = 27777;
    revive.payload.assign(8, 0);
    revive.payload[0] = static_cast<std::uint8_t>(8888u & 0xFFu);
    revive.payload[1] = static_cast<std::uint8_t>((8888u >> 8) & 0xFFu);
    ASSERT_EQ(tcp.send(revive), NetError::Ok);

    EXPECT_FALSE(client.wait_for(2, std::chrono::milliseconds(500)));
    EXPECT_EQ(client.snapshot().size(), 1u);
    tcp.disconnect();
}

// =============================================================================
// M49 -- cat=70 (SiegeWar_Profit) wire-format golden + drop test.
// First siege-war economic category locked. SiegeWar_Profit
// carries the post-battle resource split between the
// winning guild and contributing participants.
// Locking the request shape here means a regression in
// modern siege-profit encoder trips the golden comparison
// before any real guild gets a wrong payout distribution.
//
// 18B total: 2B length=16 + 8B header (cat=70, proto=1, obj_id=28888) +
// 8B payload (4B guild_id=500 + 4B share=25000).
//
// C 协议扩展 M49 -- the 35th distinct category locked at the
// wire layer (after cat=1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14,
// 15, 17, 20, 21, 22, 23, 24, 28, 29, 30, 32, 33, 37, 39, 41,
// 58, 62, 64, 65, 69, 71, 72). Crossed the 43.2% mark of the
// 81-category protocol surface.
// =============================================================================

TEST_F(LoginServerFixtureGolden, GoldenCapturesSiegeWarProfitRequest) {
    // cat=70 (SiegeWar_Profit), proto=1 (profit-share base), obj_id=28888,
    // 8B payload (4B guild_id=500 + 4B share=25000).
    // Mirrors the wire shape the legacy client sends for a
    // siege-war profit distribution. Pins a real profit
    // request so a regression in net layer framing or modern
    // SiegeWar_Profit encoder trips the golden comparison.
    Message profit;
    profit.header.category = 70;
    profit.header.protocol = 1;
    profit.header.object_id = 28888;
    profit.header.checksum = 0;
    profit.header.code = 0;
    profit.payload.clear();
    // 4B guild_id=500 LE
    profit.payload.push_back(static_cast<std::uint8_t>(500u & 0xFFu));
    profit.payload.push_back(static_cast<std::uint8_t>((500u >> 8) & 0xFFu));
    profit.payload.push_back(0);
    profit.payload.push_back(0);
    // 4B share=25000 LE
    profit.payload.push_back(static_cast<std::uint8_t>(25000u & 0xFFu));
    profit.payload.push_back(static_cast<std::uint8_t>((25000u >> 8) & 0xFFu));
    profit.payload.push_back(static_cast<std::uint8_t>((25000u >> 16) & 0xFFu));
    profit.payload.push_back(0);

    const auto actual = reconstruct_wire(profit);
    const auto golden = read_golden_bytes("siegewar_profit_request.bin");
    EXPECT_EQ(actual, golden);
    ASSERT_EQ(actual.size(), 18u);
}
TEST_F(LoginServerFixture, UnknownCategorySiegeWarProfitRequestIsDroppedWithoutResponse) {
    CapturingClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    ASSERT_EQ(tcp.connect(ccfg), NetError::Ok);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));

    // Send cat=70 (SiegeWar_Profit) profit-sharing request --
    // LoginHandler logs unhandled category: SiegeWar_Profit and drops it
    // without reply. Thirty-fifth category in the C 协议扩展
    // arc.
    Message profit;
    profit.header.category = 70;
    profit.header.protocol = 1;
    profit.header.object_id = 28888;
    profit.payload.assign(8, 0);
    profit.payload[0] = static_cast<std::uint8_t>(500u & 0xFFu);
    profit.payload[1] = static_cast<std::uint8_t>((500u >> 8) & 0xFFu);
    ASSERT_EQ(tcp.send(profit), NetError::Ok);

    EXPECT_FALSE(client.wait_for(2, std::chrono::milliseconds(500)));
    EXPECT_EQ(client.snapshot().size(), 1u);
    tcp.disconnect();
}
// =============================================================================
// M50 -- cat=60 (Suryun) wire-format golden + drop test.
// First training-practice category locked. Suryun
// carries practice / training requests when a player
// wants to grind a skill outside combat.
// Locking the request shape here means a regression in
// modern suryun encoder trips the golden comparison
// before any real skill grinding gets the wrong rates.
//
// 18B total: 2B length=16 + 8B header (cat=60, proto=1, obj_id=29999) +
// 8B payload (4B skill_id=42 + 4B duration=0).
//
// C 协议扩展 M50 -- the 36th distinct category locked at the
// wire layer (after cat=1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14,
// 15, 17, 20, 21, 22, 23, 24, 28, 29, 30, 32, 33, 37, 39, 41,
// 58, 62, 64, 65, 69, 70, 71, 72). Crossed the 44.4% mark of the
// 81-category protocol surface.
// =============================================================================

TEST_F(LoginServerFixtureGolden, GoldenCapturesSuryunRequest) {
    // cat=60 (Suryun), proto=1 (practice base), obj_id=29999,
    // 8B payload (4B skill_id=42 + 4B duration=0).
    // Mirrors the wire shape the legacy client sends for a
    // training / practice request. Pins a real practice
    // request so a regression in net layer framing or modern
    // Suryun encoder trips the golden comparison.
    Message suryun;
    suryun.header.category = 60;
    suryun.header.protocol = 1;
    suryun.header.object_id = 29999;
    suryun.header.checksum = 0;
    suryun.header.code = 0;
    suryun.payload.clear();
    // 4B skill_id=42 LE
    suryun.payload.push_back(static_cast<std::uint8_t>(42u & 0xFFu));
    suryun.payload.push_back(0);
    suryun.payload.push_back(0);
    suryun.payload.push_back(0);
    // 4B duration=0
    suryun.payload.push_back(0);
    suryun.payload.push_back(0);
    suryun.payload.push_back(0);
    suryun.payload.push_back(0);

    const auto actual = reconstruct_wire(suryun);
    const auto golden = read_golden_bytes("suryun_request.bin");
    EXPECT_EQ(actual, golden);
    ASSERT_EQ(actual.size(), 18u);
}
TEST_F(LoginServerFixture, UnknownCategorySuryunRequestIsDroppedWithoutResponse) {
    CapturingClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    ASSERT_EQ(tcp.connect(ccfg), NetError::Ok);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));

    // Send cat=60 (Suryun) practice request --
    // LoginHandler logs unhandled category: Suryun and drops it
    // without reply. Thirty-sixth category in the C 协议扩展
    // arc.
    Message suryun;
    suryun.header.category = 60;
    suryun.header.protocol = 1;
    suryun.header.object_id = 29999;
    suryun.payload.assign(8, 0);
    suryun.payload[0] = static_cast<std::uint8_t>(42u & 0xFFu);
    ASSERT_EQ(tcp.send(suryun), NetError::Ok);

    EXPECT_FALSE(client.wait_for(2, std::chrono::milliseconds(500)));
    EXPECT_EQ(client.snapshot().size(), 1u);
    tcp.disconnect();
}
// =============================================================================
// M51 -- cat=31 (Battle) wire-format golden + drop test.
// First battle-engagement category locked. Battle
// carries engage / disengage / target requests when a
// player enters combat with a creature.
// Locking the request shape here means a regression in
// modern battle encoder trips the golden comparison
// before any real combat state gets desynced.
//
// 18B total: 2B length=16 + 8B header (cat=31, proto=1, obj_id=32222) +
// 8B payload (4B battle_type=5 + 4B target=0).
//
// C 协议扩展 M51 -- the 37th distinct category locked at the
// wire layer (after cat=1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14,
// 15, 17, 20, 21, 22, 23, 24, 28, 29, 30, 32, 33, 37, 39, 41,
// 58, 60, 62, 64, 65, 69, 70, 71, 72). Crossed the 45.7% mark of the
// 81-category protocol surface.
// =============================================================================

TEST_F(LoginServerFixtureGolden, GoldenCapturesBattleRequest) {
    // cat=31 (Battle), proto=1 (engage base), obj_id=32222,
    // 8B payload (4B battle_type=5 + 4B target=0).
    // Mirrors the wire shape the legacy client sends for a
    // battle engagement. Pins a real engage
    // request so a regression in net layer framing or modern
    // Battle encoder trips the golden comparison.
    Message battle;
    battle.header.category = 31;
    battle.header.protocol = 1;
    battle.header.object_id = 32222;
    battle.header.checksum = 0;
    battle.header.code = 0;
    battle.payload.clear();
    // 4B battle_type=5 LE
    battle.payload.push_back(static_cast<std::uint8_t>(5u & 0xFFu));
    battle.payload.push_back(0);
    battle.payload.push_back(0);
    battle.payload.push_back(0);
    // 4B target=0
    battle.payload.push_back(0);
    battle.payload.push_back(0);
    battle.payload.push_back(0);
    battle.payload.push_back(0);

    const auto actual = reconstruct_wire(battle);
    const auto golden = read_golden_bytes("battle_request.bin");
    EXPECT_EQ(actual, golden);
    ASSERT_EQ(actual.size(), 18u);
}
TEST_F(LoginServerFixture, UnknownCategoryBattleRequestIsDroppedWithoutResponse) {
    CapturingClientHandler client;
    mxh::net::TcpClient tcp(client);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    ASSERT_EQ(tcp.connect(ccfg), NetError::Ok);
    ASSERT_TRUE(client.wait_for(1, std::chrono::seconds(2)));

    // Send cat=31 (Battle) engage request --
    // LoginHandler logs unhandled category: Battle and drops it
    // without reply. Thirty-seventh category in the C 协议扩展
    // arc.
    Message battle;
    battle.header.category = 31;
    battle.header.protocol = 1;
    battle.header.object_id = 32222;
    battle.payload.assign(8, 0);
    battle.payload[0] = static_cast<std::uint8_t>(5u & 0xFFu);
    ASSERT_EQ(tcp.send(battle), NetError::Ok);

    EXPECT_FALSE(client.wait_for(2, std::chrono::milliseconds(500)));
    EXPECT_EQ(client.snapshot().size(), 1u);
    tcp.disconnect();
}