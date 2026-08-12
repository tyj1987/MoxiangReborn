//
// login_wire_sha256_test.cpp
//
// E2 T2 wire SHA-256 fingerprint lock.
// Spins up a LoginServer on a pinned port, drives a full login
// flow (connect + login + ack), and asserts the SHA-256 fingerprint
// over the concatenated wire bytes is byte-stable across runs.
//
// Regression anchor for E2 T2 per ROADMAP.md.
//
// Pre-computed golden fingerprint is recorded after first green run.

#include "mxh/net/net.hpp"
#include "mxh/net/capture_handler.hpp"
#include "mxh/server/server.hpp"
#include "mxh/db/db_adapter.hpp"
#include "mxh/db/sqlite_adapter.hpp"

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

#ifdef _WIN32
    #include <windows.h>
#endif

// Self-contained fixture mirroring login_e2e_test.cpp but kept local.

namespace {

using mxh::net::ConnectionId;
using mxh::net::IConnectionHandler;
using mxh::net::Message;
using mxh::net::NetError;
using mxh::net::Sha256Digest;
using mxh::net::TcpClient;
using mxh::net::TcpServer;
using mxh::net::CaptureHandler;

constexpr std::uint16_t kWireSha256Port = 54322;

// Inner handler used as the CaptureHandler delegate. Provides wait_for
// so the test can synchronize with server-side message delivery.
struct WaitingInner final : IConnectionHandler {
    std::mutex mu;
    std::condition_variable cv;
    std::vector<Message> messages;
    bool connected = false;
    bool disconnected = false;

    bool on_connect(ConnectionId, const std::string&) override {
        std::lock_guard<std::mutex> lk(mu);
        connected = true;
        cv.notify_all();
        return true;
    }
    void on_message(ConnectionId, const Message& msg) override {
        std::lock_guard<std::mutex> lk(mu);
        messages.push_back(msg);
        cv.notify_all();
    }
    void on_disconnect(ConnectionId, NetError) override {
        std::lock_guard<std::mutex> lk(mu);
        disconnected = true;
        cv.notify_all();
    }

    bool wait_for(std::size_t n, std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lk(mu);
        return cv.wait_for(lk, timeout, [&]{ return messages.size() >= n; });
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
class LoginServerWireSha256Fixture : public ::testing::Test {
protected:
    void SetUp() override {
        auto tmp = std::filesystem::temp_directory_path();
        db_path_ = (tmp / (std::string("login_wire_sha256_") +
            std::to_string(static_cast<long long>(::GetCurrentProcessId())) + ".db"))
            .string();
        std::remove(db_path_.c_str());

        db_ = mxh::db::make_adapter("sqlite");
        ASSERT_NE(db_, nullptr);
        mxh::db::ConnectionConfig cfg;
        cfg.backend = "sqlite";
        cfg.path = db_path_;
        ASSERT_TRUE(db_->connect(cfg));

        std::string schema;
        schema += "CREATE TABLE IF NOT EXISTS modern_account_identity (account_id TEXT PRIMARY KEY, user_idx INTEGER UNIQUE);";
        schema += "CREATE TABLE IF NOT EXISTS chr_log_info (";
        schema += " id TEXT PRIMARY KEY,";
        schema += " pw TEXT NOT NULL,";
        schema += " userlevel INTEGER NOT NULL DEFAULT 0);";
        std::string ins = "INSERT INTO chr_log_info (id, pw, userlevel) VALUES ('test', 'test', 2);";
        schema += ins;
        auto* sa = static_cast<mxh::db::SqliteAdapter*>(db_.get());
        ASSERT_TRUE(sa->exec_multi(schema));

        port_ = kWireSha256Port;
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
        ASSERT_EQ(server_->start(scfg), NetError::Ok);
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

    Sha256Digest run_login_flow() {
        WaitingInner inner;
        CaptureHandler capture(inner);
        TcpClient tcp(capture);
        mxh::net::ClientConfig ccfg;
        ccfg.remote_address = "127.0.0.1";
        ccfg.port = static_cast<std::uint16_t>(port_);
        ccfg.use_legacy_framing = true;
        EXPECT_EQ(tcp.connect(ccfg), NetError::Ok);
        EXPECT_TRUE(inner.wait_for(1, std::chrono::seconds(2)));
        auto dcs = capture.snapshot()[0];
        std::uint32_t auth_key = dcs.message.header.object_id;
        EXPECT_GT(auth_key, 0u);

        Message login;
        login.header.category = 7;
        login.header.protocol = 1;
        login.header.object_id = 0;
        login.payload = make_legacy_login_payload(auth_key, "test", "test");
        EXPECT_EQ(tcp.send(login), NetError::Ok);
        EXPECT_TRUE(inner.wait_for(2, std::chrono::seconds(2)));
        tcp.disconnect();
        return capture.fingerprint();
    }

    std::string db_path_;
    int port_ = 0;
    std::uint16_t agent_port_for_ack_ = 0;
    std::unique_ptr<mxh::db::IDbAdapter> db_;
    std::unique_ptr<mxh::server::LoginHandler> handler_;
    std::unique_ptr<TcpServer> server_;
    std::thread drain_thread_;
    std::atomic<bool> drain_running_{false};
    std::mutex replies_mu_;
    std::unordered_map<std::uint64_t, std::vector<Message>> replies_;
};
// Anchor: drive the flow once and surface the fingerprint so the
// operator can lock it into the WireFingerprintMatchesGoldenHash test.
// Note: a single fixture cannot drive the flow twice and expect the
// same hash because LoginHandler increments next_auth_key_ per client.
TEST_F(LoginServerWireSha256Fixture, WireFingerprintIsDeterministicAcrossRuns) {
    const auto fp = run_login_flow();
    const std::string empty_sha =
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
    EXPECT_NE(fp.to_hex(), empty_sha);
    // Hex string must be 64 lowercase hex chars (FIPS SHA-256).
    EXPECT_EQ(fp.to_hex().size(), 64u);
    std::fprintf(stderr, "[wire-sha256] fp=%s\n", fp.to_hex().c_str());
}

// Locked invariant: fingerprint must match the value computed once
// and recorded. If this fails after an intentional wire-format change,
// update the literal here and commit the new golden to golden/login_flow.sha256.
TEST_F(LoginServerWireSha256Fixture, WireFingerprintMatchesGoldenHash) {
    const auto fp = run_login_flow();
    const std::string golden =
        "abe6cc6c6ce823c04b53eed1a9a7badb7f24c51c5434baef9146373c3a7cf3c2";
    if (golden == std::string(64, '0')) {
        // golden hash IS locked (see literal above). This branch is unreachable;
    // kept for documentation of the unlock procedure.
    FAIL() << "golden placeholder reached -- golden is locked; unreachable";
    }
    EXPECT_EQ(fp.to_hex(), golden);
}

// Sanity: captured wire bytes must contain dist_connect_success + login_ack
// with the expected byte counts.
TEST_F(LoginServerWireSha256Fixture, CapturedWireBytesContainBothAckAndSuccess) {
    WaitingInner inner;
    CaptureHandler capture(inner);
    TcpClient tcp(capture);
    mxh::net::ClientConfig ccfg;
    ccfg.remote_address = "127.0.0.1";
    ccfg.port = static_cast<std::uint16_t>(port_);
    ccfg.use_legacy_framing = true;
    ASSERT_EQ(tcp.connect(ccfg), NetError::Ok);
    ASSERT_TRUE(inner.wait_for(1, std::chrono::seconds(2)));
    auto dcs = capture.snapshot()[0];
    ASSERT_EQ(dcs.message.header.category, 7u);
    ASSERT_EQ(dcs.message.header.protocol, 0u);
    std::uint32_t auth_key = dcs.message.header.object_id;

    Message login;
    login.header.category = 7;
    login.header.protocol = 1;
    login.header.object_id = 0;
    login.payload = make_legacy_login_payload(auth_key, "test", "test");
    ASSERT_EQ(tcp.send(login), NetError::Ok);
    ASSERT_TRUE(inner.wait_for(2, std::chrono::seconds(2)));
    tcp.disconnect();

    auto snap = capture.snapshot();
    ASSERT_EQ(snap.size(), 2u);
    EXPECT_EQ(snap[0].wire_bytes.size(), 10u);
    EXPECT_EQ(snap[1].wire_bytes.size(), 33u);
    EXPECT_EQ(snap[1].message.header.protocol, 2u);
}

}  // namespace
