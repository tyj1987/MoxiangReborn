// server_handler_test.cpp - Phase 10.17 server handler smoke tests
//
// Covers modern/include/mxh/server/server.hpp ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â the 3 high-level
// game-server handlers (LoginHandler, AgentHandler, MapHandler).
// These are the bridge between the net layer and the game logic.
//
// Scope: this is a smoke test. It verifies the handlers can be
// constructed with a mock IDbAdapter, that on_connect / on_disconnect
// don't crash, and that the few state-setter / state-getter
// methods exposed on the public API surface work as documented.
//
// What is NOT tested here (covered by other test files):
//   - The actual on_message protocol dispatch ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â covered by
//     integration tests that wire a real TcpServer to a handler
//     and feed it bytes (see modern/tools/MoxianLoginServer 5/5
//     smoke). A unit test for the byte-level protocol would need
//     to re-derive the message framing, which would duplicate the
//     integration test surface for no extra value.
//   - HSEL encryption integration with the handlers ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â covered
//     by hsel_stream_test.cpp + aes_gcm_test.cpp.

#include "mxh/game/experience_curve.hpp"
#include "synthetic_server_resources.hpp"
#include "mxh/game/exp_penalty.hpp"
#include "mxh/game/hero_total_layout.hpp"
#include "mxh/proto/character_revive.hpp"
#include "mxh/server/agent_userconn.hpp"
#include "mxh/server/server.hpp"
#include "mxh/server/commit_present_revive.hpp"
#include "mxh/server/revive_vitality_messages.hpp"
#include "mxh/server/ai_system.hpp"
#include "client/CInGameState.hpp"
#include "mxh/render/EntityScene.hpp"
#include "mxh/game/item_manager.hpp"
#include "mxh/game/item_list_parser.hpp"
#include "mxh/compat/mh_file_ex.hpp"
#include "mxh/server/dealitem_parser.hpp"
#include "mxh/server/quest_script_loader.hpp"
#include "cstdint"
#include "filesystem"
#include "fstream"
#include "sstream"
#include "vector"
#include "mxh/db/db_adapter.hpp"
#include "mxh/db/sqlite_adapter.hpp"
#include "mxh/db/schema_migration.hpp"
#include "mxh/db/modern_shop_state.hpp"
#include "mxh/net/net.hpp"

#include <gtest/gtest.h>

#include <atomic>
#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif
#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <set>

namespace mxh::server::test {

// ===========================================================================
// Mock IDbAdapter ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â minimum viable stub for handler construction.
//
// All methods return a default-constructed DbResult (success).
// The DB-write counters are public so tests can verify that a
// handler did NOT call the DB on a no-DB code path.
class MockDbAdapter final : public mxh::db::IDbAdapter {
public:
    mxh::db::DbResult connect(const mxh::db::ConnectionConfig&) override {
        return {};
    }
    void disconnect() override {}
    bool is_connected() const noexcept override { return true; }
    mxh::db::DbResult execute(std::string_view sql, std::span<const mxh::db::Bind>) override {
        ++exec_count;
        executed_sql.emplace_back(sql);
        if (throw_write) throw std::runtime_error("injected adapter exception");
        if (!fail_write_matching.empty() && sql.find(fail_write_matching) != std::string_view::npos)
            return {mxh::db::DbError::IoError, "injected write failure"};
        return {};
    }
    // R-2: if userlevel_to_return != UINT8_MAX, treat the next query
    // against "chr_log_info" as returning one row with that value in
    // column [0]. UINT8_MAX means "do not inject" (default) so other
    // tests that do not care about userlevel keep the original empty
    // result semantics.
    std::atomic<std::uint8_t> userlevel_to_return{0xFFu};
    mxh::db::DbResult query(std::string_view sql, std::span<const mxh::db::Bind>,
                           mxh::db::ResultSet& out) override {
        ++query_count;
        out = mxh::db::ResultSet{};
        std::uint8_t lvl = userlevel_to_return.load();
        if (lvl != 0xFFu && sql.find("chr_log_info") != std::string_view::npos) {
            mxh::db::ResultSet injected;
            mxh::db::Row row;
            row.push_back(static_cast<std::int64_t>(lvl));
            injected.rows.push_back(std::move(row));
            out = std::move(injected);
        }
        return {};
    }

    mxh::db::DbResult begin_transaction() override {
        ++begin_count;
        return fail_begin ? mxh::db::DbResult{mxh::db::DbError::IoError, "injected begin failure"} : mxh::db::DbResult{};
    }
    mxh::db::DbResult commit() override {
        return fail_commit ? mxh::db::DbResult{mxh::db::DbError::IoError, "injected commit failure"} : mxh::db::DbResult{};
    }
    mxh::db::DbResult rollback() override {
        ++rollback_count;
        if (throw_rollback) throw std::runtime_error("injected rollback exception");
        return fail_rollback ? mxh::db::DbResult{mxh::db::DbError::IoError, "injected rollback failure"} : mxh::db::DbResult{};
    }
    std::string backend_name() const noexcept override { return backend; }

    // Counters for tests that want to verify "handler did NOT call
    // the DB on a no-DB code path". Public so the gtest bodies can
    // read them directly.
    std::atomic<int> exec_count{0};
    std::atomic<int> query_count{0};
    std::atomic<int> begin_count{0};
    std::atomic<int> rollback_count{0};
    std::string backend = "mock";
    std::vector<std::string> executed_sql;
    std::string fail_write_matching;
    bool fail_begin = false;
    bool fail_commit = false;
    bool fail_rollback = false;
    bool throw_write = false;
    bool throw_rollback = false;
};

// Helper: a ReplyFn-compatible spy that counts calls.
struct ReplySpy {
    std::atomic<int> call_count{0};
    mxh::net::ConnectionId last_id{};
    mxh::net::Message last_message{};
    std::vector<mxh::net::Message> messages;
    std::vector<mxh::net::ConnectionId> connection_ids;
};

inline mxh::server::ReplyFn make_reply_spy(ReplySpy& spy) {
    return [&spy](mxh::net::ConnectionId id, const mxh::net::Message& message) {
        ++spy.call_count;
        spy.last_id = id;
        spy.last_message = message;
        spy.messages.push_back(message);
        spy.connection_ids.push_back(id);
    };
}

// ===========================================================================
// MockTcpSender ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â Phase 12.1 P2-13: capture outgoing messages from
// AgentHandler without spinning up a real socket. Tests verify that
// GameOutSyn / GameInSyn forwarding actually fires by reading sent_msgs_.
// ===========================================================================
class MockTcpSender final : public mxh::net::ITcpSender {
public:
    [[nodiscard]] mxh::net::NetError send(const mxh::net::Message& msg) override {
        ++send_count;
        last_message = msg;
        if (!connected || fail_send) return mxh::net::NetError::SendFailed;
        sent_msgs.push_back(msg);
        if (on_send) on_send(msg);
        return mxh::net::NetError::Ok;
    }
    [[nodiscard]] bool is_connected() const noexcept override { return connected; }

    // Tests can flip connected ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¾ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ false to simulate the map server going
    // down between set_map_server() and on_disconnect().
    void set_connected(bool c) noexcept { connected = c; }
    void set_fail_send(bool f) noexcept { fail_send = f; }

    std::atomic<int> send_count{0};
    std::vector<mxh::net::Message> sent_msgs;
    std::function<void(const mxh::net::Message&)> on_send;
    mxh::net::Message last_message{};
private:
    bool connected = true;
    bool fail_send = false;
};

class MapHandlerForwardingSender final : public mxh::net::ITcpSender {
public:
    MapHandlerForwardingSender(mxh::server::MapHandler& target,
                               mxh::net::ConnectionId connection)
        : target_(target), connection_(connection) {}

    [[nodiscard]] mxh::net::NetError send(
        const mxh::net::Message& msg) override {
        if (!connected) return mxh::net::NetError::SendFailed;
        ++send_count;
        target_.on_message(connection_, msg);
        return mxh::net::NetError::Ok;
    }
    [[nodiscard]] bool is_connected() const noexcept override {
        return connected;
    }

    bool connected = true;
    std::atomic<int> send_count{0};

private:
    mxh::server::MapHandler& target_;
    mxh::net::ConnectionId connection_;
};

// ===========================================================================
// LoginHandler
// ===========================================================================

TEST(LoginHandlerTest, DefaultConstructionDoesNotCrash) {
    MockDbAdapter db;
    mxh::server::LoginHandler handler(db, "127.0.0.1", 7000,
                                       mxh::server::ReplyFn{});
    SUCCEED();
}

TEST(LoginHandlerTest, LegacyFlagIsOptional) {
    // The 4-arg form (without use_legacy_framing) and the 5-arg form
    // both work. use_legacy_framing defaults to false.
    MockDbAdapter db;
    mxh::server::LoginHandler h1(db, "127.0.0.1", 7000, mxh::server::ReplyFn{});
    mxh::server::LoginHandler h2(db, "127.0.0.1", 7000, mxh::server::ReplyFn{}, true);
    mxh::server::LoginHandler h3(db, "127.0.0.1", 7000, mxh::server::ReplyFn{}, false);
    SUCCEED();
}

TEST(LoginHandlerTest, OnConnectReturnsTrue) {
    // The default on_connect returns true. The login handler does not
    // override it, so we get the base-class true. In non-legacy mode
    // on_connect is just a logging hook (no reply_, no DB query).
    MockDbAdapter db;
    mxh::server::LoginHandler handler(db, "127.0.0.1", 7000,
                                       mxh::server::ReplyFn{});
    EXPECT_TRUE(handler.on_connect({}, "192.168.1.1:12345"));
}

TEST(LoginHandlerTest, OnDisconnectDoesNotCrash) {
    // The default on_disconnect is a no-op. Verify it does not call
    // reply_ or the DB.
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::LoginHandler handler(db, "127.0.0.1", 7000,
                                       make_reply_spy(reply));
    handler.on_disconnect({}, mxh::net::NetError::Disconnected);
    EXPECT_EQ(reply.call_count.load(), 0);
    EXPECT_EQ(db.exec_count.load(), 0);
    EXPECT_EQ(db.query_count.load(), 0);
}

// ===========================================================================
// AgentHandler
// ===========================================================================

TEST(AgentHandlerTest, DefaultConstructionDoesNotCrash) {
    MockDbAdapter db;
    mxh::server::AgentHandler handler(db, mxh::server::ReplyFn{});
    SUCCEED();
}

TEST(AgentHandlerTest, LegacyFlagIsOptional) {
    MockDbAdapter db;
    mxh::server::AgentHandler h1(db, mxh::server::ReplyFn{});
    mxh::server::AgentHandler h2(db, mxh::server::ReplyFn{}, true);
    SUCCEED();
}

TEST(AgentHandlerTest, GetMapConnectionDefaultsToInvalid) {
    // Before set_map_server() is called, the map connection should
    // be the invalid (zero) ConnectionId.
    MockDbAdapter db;
    mxh::server::AgentHandler handler(db, mxh::server::ReplyFn{});
    auto map_id = handler.get_map_connection();
    EXPECT_FALSE(map_id.valid());
    EXPECT_EQ(map_id.value, 0u);
}

TEST(AgentHandlerTest, SetMapServerStoresConnection) {
    // After set_map_server(client, conn), get_map_connection() must
    // return that conn. The TcpClient pointer is stored as-is (no
    // copy), so we pass nullptr to avoid any real network setup.
    MockDbAdapter db;
    mxh::server::AgentHandler handler(db, mxh::server::ReplyFn{});

    auto map_conn = mxh::net::make_connection_id(42);
    handler.set_map_server(nullptr, map_conn);

    auto retrieved = handler.get_map_connection();
    EXPECT_TRUE(retrieved.valid());
    EXPECT_EQ(retrieved.value, 42u);
}

TEST(AgentHandlerTest, SetMapServerOverwritesPrevious) {
    // Calling set_map_server twice replaces the previous connection.
    MockDbAdapter db;
    mxh::server::AgentHandler handler(db, mxh::server::ReplyFn{});

    handler.set_map_server(nullptr, mxh::net::make_connection_id(100));
    EXPECT_EQ(handler.get_map_connection().value, 100u);

    handler.set_map_server(nullptr, mxh::net::make_connection_id(200));
    EXPECT_EQ(handler.get_map_connection().value, 200u);
}

TEST(AgentHandlerTest, OnConnectNonLegacyDoesNotCallReply) {
    // In non-legacy mode, on_connect just logs and returns true.
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    EXPECT_TRUE(handler.on_connect({}, "10.0.0.1:9999"));
    EXPECT_EQ(reply.call_count.load(), 0);
    EXPECT_EQ(db.query_count.load(), 0);
}

TEST(AgentHandlerTest, OnDisconnectDoesNotCrash) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    handler.on_disconnect({}, mxh::net::NetError::Disconnected);
    EXPECT_EQ(reply.call_count.load(), 0);
    EXPECT_EQ(db.exec_count.load(), 0);
    EXPECT_EQ(db.query_count.load(), 0);
}

TEST(AgentHandlerTest, OnDisconnectWithoutMapServerDoesNotCrash) {
    // Phase 12.1: GameOutSyn forwarding only fires when a TcpClient*
    // has been set via set_map_server(). With no map server attached,
    // on_disconnect must still clean up local state and not deref
    // any null TcpClient.
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    handler.on_disconnect(mxh::net::make_connection_id(99),
                          mxh::net::NetError::Disconnected);
    EXPECT_EQ(reply.call_count.load(), 0);
    // No DB calls either ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â on_disconnect doesn't persist anything.
    EXPECT_EQ(db.exec_count.load(), 0);
    EXPECT_EQ(db.query_count.load(), 0);
}

TEST(AgentHandlerTest, OnDisconnectWithMapServerNullptrDoesNotCrash) {
    // Phase 12.1: even if set_map_server() was called with a
    // non-null ConnectionId but a null TcpClient* (e.g. test setup
    // or a transient race where the TcpClient was destroyed but the
    // connection id is still set), on_disconnect must not deref the
    // null pointer. It should log a "no MapServer connection" debug
    // line and continue cleanly.
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    handler.set_map_server(/*client=*/nullptr,
                           mxh::net::make_connection_id(7));
    handler.on_disconnect(mxh::net::make_connection_id(100),
                          mxh::net::NetError::Disconnected);
    // No reply was sent (no client, no map_client_ connected).
    EXPECT_EQ(reply.call_count.load(), 0);
    // TcpClient::send must NOT have been called (we'd crash otherwise
    // since the pointer is null and the code checks for it before
    // calling send).
    SUCCEED();
}

// ---------------------------------------------------------------------------
// Phase 12.1 P2-13: ITcpSender injection ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â verify GameOutSyn forwarding
// actually fires when a connected MockTcpSender is wired in. Before this
// refactor, AgentHandler held a concrete TcpClient* and there was no way
// to test "did the agent actually call send()?" without a live map server.
//
// Note: full "GameOutSyn sent" verification requires populating the
// private conn_user_ids_ / conn_char_ids_ / conn_map_nums_ maps, which
// has no public setter (the maps are populated by the legacy character
// select path). The tests below cover the early-return / null /
// disconnected paths; the "full send" path is covered by
// `test_map_integration.py` in the integration test (Phase 9).
// ---------------------------------------------------------------------------

TEST(AgentHandlerTest, OnDisconnectWithMockSenderNoSessionDoesNotSend) {
    // Mock sender is wired in but no user/char/map_num session has been
    // registered on the handler. on_disconnect must clean up state
    // without sending anything (the early-return path: removed_char_id
    // == 0 means we have nothing to tell the map server).
    MockDbAdapter db;
    ReplySpy reply;
    MockTcpSender mock;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    handler.set_map_server(&mock, mxh::net::make_connection_id(42));
    handler.on_disconnect(mxh::net::make_connection_id(100),
                          mxh::net::NetError::Disconnected);
    // Mock sender.send was never reached because the early-return fires
    // when there is no recorded session to forward.
    EXPECT_EQ(mock.send_count.load(), 0);
    EXPECT_TRUE(mock.sent_msgs.empty());
    EXPECT_EQ(reply.call_count.load(), 0);
}

TEST(AgentHandlerTest, OnDisconnectWithMockSenderDisconnectedSenderNoSend) {
    // Mock sender is wired in but the sender reports !is_connected().
    // on_disconnect must skip the send entirely (the code checks
    // is_connected() before calling send). Even if a session were
    // registered, no message should be delivered.
    MockDbAdapter db;
    ReplySpy reply;
    MockTcpSender mock;
    mock.set_connected(false);
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    handler.set_map_server(&mock, mxh::net::make_connection_id(42));
    handler.on_disconnect(mxh::net::make_connection_id(100),
                          mxh::net::NetError::Disconnected);
    EXPECT_EQ(mock.send_count.load(), 0);
    EXPECT_TRUE(mock.sent_msgs.empty());
}

TEST(AgentHandlerTest, ForwardFromMapWithMockSenderNoRoute) {
    // forward_from_map with a char_id that has no registered client
    // must not crash and not call reply (just a no-op debug log).
    MockDbAdapter db;
    ReplySpy reply;
    MockTcpSender mock;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    handler.set_map_server(&mock, mxh::net::make_connection_id(42));
    mxh::net::Message msg;
    msg.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Move);
    msg.header.object_id = 9999;  // char_id with no registered client
    handler.forward_from_map(mxh::net::make_connection_id(42), msg);
    EXPECT_EQ(reply.call_count.load(), 0);
}

TEST(AgentHandlerTest, ForwardFromMapRoutesPartyAndGuildToOffMapSession) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    handler.register_session(mxh::net::make_connection_id(100), 1u, 123u, 10u);
    handler.register_session(mxh::net::make_connection_id(101), 2u, 456u, 12u);

    for (const auto category : {mxh::proto::Category::Party,
                                mxh::proto::Category::Guild}) {
        mxh::net::Message msg;
        msg.header.category = static_cast<std::uint8_t>(category);
        msg.header.protocol = 0; // Info
        msg.header.object_id = 456u;
        msg.payload = {0x2a, 0x00, 0x00, 0x00, 0x01};
        handler.forward_from_map(mxh::net::make_connection_id(77), msg);
        ASSERT_EQ(reply.last_id.value, 101u);
        EXPECT_EQ(reply.last_message.header.category,
                  static_cast<std::uint8_t>(category));
        EXPECT_EQ(reply.last_message.header.object_id, 456u);
        EXPECT_EQ(reply.last_message.payload.size(), 5u);
    }
    EXPECT_EQ(reply.call_count.load(), 2);
}

TEST(AgentHandlerTest, MovementCorrectionRoutesOnlyToOwnerWhileReportsExcludeOwner) {
    MockDbAdapter db;
    std::vector<std::pair<std::uint64_t, mxh::net::Message>> delivered;
    AgentHandler handler(db, [&](mxh::net::ConnectionId id, const mxh::net::Message& msg) {
        delivered.emplace_back(id.value, msg);
    });
    handler.register_session(mxh::net::make_connection_id(100), 1u, 123u, 10u);
    handler.register_session(mxh::net::make_connection_id(101), 2u, 456u, 10u);
    auto move = mxh::client::make_move_message(123, mxh::proto::MoveProtocol::OneTarget, 120, 240);
    handler.forward_from_map(mxh::net::make_connection_id(77), move);
    ASSERT_EQ(delivered.size(), 1u);
    EXPECT_EQ(delivered[0].first, 101u);
    delivered.clear();
    move.header.protocol = static_cast<std::uint8_t>(mxh::proto::MoveProtocol::Correction);
    handler.forward_from_map(mxh::net::make_connection_id(77), move);
    ASSERT_EQ(delivered.size(), 1u);
    EXPECT_EQ(delivered[0].first, 100u);
    EXPECT_EQ(delivered[0].second.payload, move.payload);
    delivered.clear();
    move.header.object_id = 999u;
    handler.forward_from_map(mxh::net::make_connection_id(77), move);
    EXPECT_TRUE(delivered.empty());
}

TEST(AgentHandlerTest, SkillStartResponseRoutesToCasterWhileWorldSkillEventsExcludeCaster) {
    MockDbAdapter db;
    std::vector<std::pair<std::uint64_t, mxh::net::Message>> delivered;
    AgentHandler handler(db, [&](mxh::net::ConnectionId id, const mxh::net::Message& msg) {
        delivered.emplace_back(id.value, msg);
    });
    handler.register_session(mxh::net::make_connection_id(100), 1u, 123u, 10u);
    handler.register_session(mxh::net::make_connection_id(101), 2u, 456u, 10u);
    mxh::net::Message skill;
    skill.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Skill);
    skill.header.protocol = static_cast<std::uint8_t>(mxh::proto::SkillProtocol::StartAck);
    skill.header.object_id = 123u;
    skill.payload.resize(8);
    handler.forward_from_map(mxh::net::make_connection_id(77), skill, 10u);
    ASSERT_EQ(delivered.size(), 1u);
    EXPECT_EQ(delivered[0].first, 100u);
    delivered.clear();
    skill.header.protocol = static_cast<std::uint8_t>(mxh::proto::SkillProtocol::SkillObjectAdd);
    handler.forward_from_map(mxh::net::make_connection_id(77), skill, 10u);
    ASSERT_EQ(delivered.size(), 1u);
    EXPECT_EQ(delivered[0].first, 101u);
}

TEST(AgentHandlerTest, ExplicitMapSourceIsolatesWorldEventsAndRoutesNpcSpeechToOwner) {
    MockDbAdapter db;
    std::vector<std::uint64_t> delivered;
    AgentHandler handler(db, [&](mxh::net::ConnectionId id, const mxh::net::Message&) {
        delivered.push_back(id.value);
    });
    handler.register_session(mxh::net::make_connection_id(100), 1, 123, 10);
    handler.register_session(mxh::net::make_connection_id(101), 2, 456, 12);
    handler.register_session(mxh::net::make_connection_id(102), 3, 789, 10);
    mxh::net::Message message;
    message.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    message.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::NpcAdd);
    message.header.object_id = 92;
    handler.forward_from_map(mxh::net::make_connection_id(1), message, 12);
    EXPECT_EQ(delivered, std::vector<std::uint64_t>{101});
    delivered.clear();
    // The other independent TcpClient can also have connection ID 1.
    handler.forward_from_map(mxh::net::make_connection_id(1), message, 10);
    std::sort(delivered.begin(), delivered.end());
    EXPECT_EQ(delivered, (std::vector<std::uint64_t>{100,102}));
    delivered.clear();
    message.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Npc);
    message.header.protocol = static_cast<std::uint8_t>(mxh::proto::NpcProtocol::SpeechAck);
    message.header.object_id = 123;
    handler.forward_from_map(mxh::net::make_connection_id(1), message, 10);
    EXPECT_EQ(delivered, std::vector<std::uint64_t>{100});
    delivered.clear();
    handler.register_session(mxh::net::make_connection_id(100), 1, 123, 12);
    message.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    message.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::Money);
    handler.forward_from_map(mxh::net::make_connection_id(1), message, 10);
    EXPECT_TRUE(delivered.empty()); // Late source-map balance cannot replace destination state.
    message.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Party);
    handler.forward_from_map(mxh::net::make_connection_id(1), message, 10);
    EXPECT_EQ(delivered, std::vector<std::uint64_t>{100}); // Cross-map social state remains routable.
}

TEST(AgentHandlerTest, MonsterDeathRoutesToVictimAndRejectsOldMapVitality) {
    MockDbAdapter db;
    std::vector<std::uint64_t> delivered;
    AgentHandler handler(db, [&](mxh::net::ConnectionId id, const mxh::net::Message&) {
        delivered.push_back(id.value);
    });
    handler.register_session({100}, 1, 123, 10);
    handler.register_session({101}, 2, 456, 10);
    mxh::net::Message death;
    death.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    death.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::CharacterDie);
    death.header.object_id = 50023;
    death.payload.resize(8);
    const std::uint32_t attacker = 50023, victim = 123;
    std::memcpy(death.payload.data(), &attacker, 4);
    std::memcpy(death.payload.data() + 4, &victim, 4);
    handler.forward_from_map({77}, death, 12);
    EXPECT_TRUE(delivered.empty());
    handler.forward_from_map({77}, death, 10);
    ASSERT_EQ(delivered.size(), 1u);
    EXPECT_EQ(delivered[0], 100u);
    delivered.clear();
    death.payload.push_back(0);
    handler.forward_from_map({77}, death, 10);
    EXPECT_TRUE(delivered.empty());
    mxh::net::Message life;
    life.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Character);
    life.header.protocol = static_cast<std::uint8_t>(mxh::proto::CharacterProtocol::LifeAck);
    life.header.object_id = victim;
    life.payload.resize(4);
    handler.forward_from_map({77}, life, 12);
    EXPECT_TRUE(delivered.empty());
    handler.forward_from_map({77}, life, 10);
    ASSERT_EQ(delivered.size(), 1u);
    EXPECT_EQ(delivered[0], 100u);
}

TEST(AgentHandlerTest, SkillHitIncludesVictimAndSameMapObservers) {
    MockDbAdapter db;
    std::vector<std::uint64_t> delivered;
    AgentHandler handler(db, [&](mxh::net::ConnectionId id, const mxh::net::Message&) {
        delivered.push_back(id.value);
    });
    handler.register_session({100}, 1, 123, 10);
    handler.register_session({101}, 2, 456, 10);
    handler.register_session({102}, 3, 789, 12);
    mxh::net::Message hit;
    hit.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Skill);
    hit.header.protocol = static_cast<std::uint8_t>(mxh::proto::SkillProtocol::SingleResult);
    hit.header.object_id = 123;
    hit.payload.resize(9);
    const std::uint32_t victim = 123;
    const std::int32_t damage = 7;
    std::memcpy(hit.payload.data(), &victim, 4);
    std::memcpy(hit.payload.data() + 4, &damage, 4);
    hit.payload[8] = 1;
    handler.forward_from_map({77}, hit, 10);
    std::sort(delivered.begin(), delivered.end());
    EXPECT_EQ(delivered, (std::vector<std::uint64_t>{100, 101}));
}

TEST(AgentHandlerTest, RejectsFriendRequestForOfflineTarget) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    handler.register_session(mxh::net::make_connection_id(100), 1u, 123u, 10u);

    mxh::net::Message request;
    request.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Friend);
    request.header.protocol = 0; // FriendAddSyn
    request.header.object_id = 456u;
    handler.on_message(mxh::net::make_connection_id(100), request);

    ASSERT_EQ(reply.call_count.load(), 1);
    EXPECT_EQ(reply.last_message.header.category,
              static_cast<std::uint8_t>(mxh::proto::Category::Friend));
    EXPECT_EQ(reply.last_message.header.protocol, 2u);
    EXPECT_EQ(reply.last_message.header.object_id, 456u);
}

TEST(AgentHandlerTest, CompletesOnlineFriendInviteHandshake) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    handler.register_session(mxh::net::make_connection_id(100), 1u, 123u, 10u);
    handler.register_session(mxh::net::make_connection_id(101), 2u, 456u, 10u);

    mxh::net::Message add;
    add.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Friend);
    add.header.protocol = 0;
    add.header.object_id = 456u;
    handler.on_message(mxh::net::make_connection_id(100), add);
    ASSERT_EQ(reply.messages.size(), 2u);
    EXPECT_EQ(reply.messages[0].header.protocol, 3u);
    EXPECT_EQ(reply.messages[0].header.object_id, 123u);
    EXPECT_EQ(reply.messages[1].header.protocol, 1u);

    mxh::net::Message accept;
    accept.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Friend);
    accept.header.protocol = 4;
    accept.header.object_id = 123u;
    handler.on_message(mxh::net::make_connection_id(101), accept);
    ASSERT_EQ(reply.messages.size(), 4u);
    EXPECT_EQ(reply.messages[2].header.protocol, 5u);
    EXPECT_EQ(reply.messages[2].header.object_id, 456u);
    EXPECT_EQ(reply.messages[3].header.protocol, 5u);
    EXPECT_EQ(reply.messages[3].header.object_id, 123u);
}

TEST(AgentHandlerTest, DisconnectRemovesOffMapSocialRoute) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    handler.register_session(mxh::net::make_connection_id(101), 2u, 456u, 12u);
    MockTcpSender map_sender;
    handler.set_map_server(&map_sender, mxh::net::make_connection_id(77));
    handler.on_disconnect(mxh::net::make_connection_id(101), mxh::net::NetError::Disconnected);

    mxh::net::Message msg;
    msg.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Party);
    msg.header.protocol = static_cast<std::uint8_t>(mxh::proto::PartyProtocol::Info);
    msg.header.object_id = 456u;
    msg.payload = {0x2a, 0x00, 0x00, 0x00, 0x01};
    handler.forward_from_map(mxh::net::make_connection_id(77), msg);
    EXPECT_EQ(reply.call_count.load(), 0);
}

TEST(AgentHandlerTest, SetMapServerAcceptsITcpSender) {
    // Phase 12.1 P2-13: set_map_server now takes ITcpSender* (was
    // TcpClient*). A MockTcpSender must be accepted without conversion
    // and the handler must remember it. The test verifies the
    // signature change is real by passing a non-TcpClient through.
    MockDbAdapter db;
    ReplySpy reply;
    MockTcpSender mock;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    // Compile-time check: this line is only valid if set_map_server
    // takes ITcpSender* (or something more permissive). If the type
    // regresses to TcpClient*, the implicit upcast fails and the
    // test won't compile.
    handler.set_map_server(&mock, mxh::net::make_connection_id(7));
    EXPECT_EQ(handler.get_map_connection().value, 7u);
}

// ---------------------------------------------------------------------------
// Phase 12.1 P2-13 follow-up: register_session() + complete GameOutSyn
// forwarding test. The previous "no session" tests covered the early-return
// path; register_session lets us populate conn_user_ids_/conn_char_ids_/
// conn_map_nums_/char_to_client_ directly so the on_disconnect handler
// fires the real forward-GameOutSyn path and the mock sender captures it.
// ---------------------------------------------------------------------------

TEST(AgentHandlerTest, RegisterSessionStoresUserCharMap) {
    // register_session is the production-meaningful entry point that
    // populates the four session maps without going through the binary
    // protocol. It is also what tests use to drive the on_disconnect
    // forwarding path. Verify all three state slots are reachable.
    MockDbAdapter db;
    mxh::server::AgentHandler handler(db, mxh::server::ReplyFn{});
    handler.register_session(
        mxh::net::make_connection_id(100),
        /*user_id=*/3001, /*char_id=*/450035712, /*map_num=*/12);
    // on_disconnect should now be able to look up the session for conn=100.
    // We verify by triggering a real forward: a connected MockTcpSender
    // must see a GameOutSyn message after on_disconnect.
    MockTcpSender mock;
    handler.set_map_server(&mock, mxh::net::make_connection_id(42));
    handler.on_disconnect(mxh::net::make_connection_id(100),
                          mxh::net::NetError::Disconnected);
    EXPECT_EQ(mock.send_count.load(), 1);
    ASSERT_EQ(mock.sent_msgs.size(), 1u);
    const auto& fwd = mock.sent_msgs[0];
    EXPECT_EQ(static_cast<int>(fwd.header.category),
              static_cast<int>(mxh::proto::Category::UserConn));
    // GameOutSyn protocol id is 33 (per AgentHandler.cpp:244).
    EXPECT_EQ(fwd.header.protocol, 31);  // GameOutSyn = 31 per protocol.hpp
    EXPECT_EQ(fwd.header.object_id, 450035712u);
    // payload = wMapNum(2B) + bIsExiting=1(1B) + padding(5B) = 8B
    ASSERT_EQ(fwd.payload.size(), 8u);
    const std::uint16_t map_in_payload =
        static_cast<std::uint16_t>(fwd.payload[0]) |
        (static_cast<std::uint16_t>(fwd.payload[1]) << 8);
    EXPECT_EQ(map_in_payload, 12u);
    EXPECT_EQ(fwd.payload[2], 1);  // bIsExiting
}

TEST(AgentHandlerTest, RegisterSessionOverridesPriorSession) {
    // Calling register_session twice for the same conn_id replaces the
    // previous entry. The new char_id wins (so the forwarded message
    // uses the new char_id as the object_id).
    MockDbAdapter db;
    mxh::server::AgentHandler handler(db, mxh::server::ReplyFn{});
    handler.register_session(mxh::net::make_connection_id(100),
                              3001, 450035712, 12);
    handler.register_session(mxh::net::make_connection_id(100),
                              3001, 99999, 7);
    MockTcpSender mock;
    handler.set_map_server(&mock, mxh::net::make_connection_id(42));
    handler.on_disconnect(mxh::net::make_connection_id(100),
                          mxh::net::NetError::Disconnected);
    EXPECT_EQ(mock.send_count.load(), 1);
    EXPECT_EQ(mock.sent_msgs[0].header.object_id, 99999u);
    const std::uint16_t map_in_payload =
        static_cast<std::uint16_t>(mock.sent_msgs[0].payload[0]) |
        (static_cast<std::uint16_t>(mock.sent_msgs[0].payload[1]) << 8);
    EXPECT_EQ(map_in_payload, 7u);
}

TEST(AgentHandlerTest, RegisterSessionIsNoOpForUnknownConn) {
    // If we disconnect a conn_id that was never registered, no
    // forwarding happens. Same as the pre-P2-13 OnDisconnectWith-
    // MockSenderNoSessionDoesNotSend test, but exercises the path
    // AFTER a different conn was registered (proving the maps are
    // keyed by conn_id correctly).
    MockDbAdapter db;
    mxh::server::AgentHandler handler(db, mxh::server::ReplyFn{});
    handler.register_session(mxh::net::make_connection_id(100),
                              3001, 450035712, 12);
    MockTcpSender mock;
    handler.set_map_server(&mock, mxh::net::make_connection_id(42));
    handler.on_disconnect(mxh::net::make_connection_id(999),  // never registered
                          mxh::net::NetError::Disconnected);
    EXPECT_EQ(mock.send_count.load(), 0);
    EXPECT_TRUE(mock.sent_msgs.empty());
}

TEST(AgentHandlerTest, GameInUsesMapSpecificServerRoute) {
    MockDbAdapter db;
    mxh::server::AgentHandler handler(db, mxh::server::ReplyFn{}, true,
                                      false, {}, /*default_map_num=*/12);
    const auto connection = mxh::net::make_connection_id(1100);
    handler.register_session(connection, 3001u, 450035712u, 10u);

    MockTcpSender default_map;
    MockTcpSender target_map;
    handler.set_map_server(&default_map, mxh::net::make_connection_id(12));
    handler.set_map_server_for_map(10u, &target_map,
                                   mxh::net::make_connection_id(10));

    mxh::net::Message game_in;
    game_in.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::GameInSyn);
    game_in.header.object_id = 450035712u;
    game_in.payload.resize(8, 0);
    handler.on_message(connection, game_in);

    ASSERT_EQ(target_map.sent_msgs.size(), 1u);
    EXPECT_TRUE(default_map.sent_msgs.empty());
    EXPECT_EQ(target_map.sent_msgs.front().header.protocol,
              static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn));
    EXPECT_EQ(target_map.sent_msgs.front().header.object_id, 450035712u);
}

TEST(AgentHandlerTest, ProductionGameInRejectsWhenMapServerIsUnavailable) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply), true,
                                      false, {}, /*default_map_num=*/12);
    handler.set_allow_dev_gamein_fallback(false);
    const auto connection = mxh::net::make_connection_id(1104);
    handler.register_session(connection, 3001u, 450035716u, 12u);

    mxh::net::Message game_in;
    game_in.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::GameInSyn);
    game_in.header.object_id = 450035716u;
    handler.on_message(connection, game_in);

    ASSERT_EQ(reply.messages.size(), 1u);
    EXPECT_EQ(reply.messages.front().header.protocol,
              static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInNack));
    EXPECT_TRUE(reply.messages.front().payload.empty());
}

TEST(AgentHandlerTest, GameInSendFailureReturnsNackInsteadOfHanging) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply), true,
                                      false, {}, /*default_map_num=*/12);
    const auto connection = mxh::net::make_connection_id(1105);
    handler.register_session(connection, 3001u, 450035717u, 12u);
    MockTcpSender map;
    map.set_fail_send(true);
    handler.set_map_server(&map, mxh::net::make_connection_id(12));

    mxh::net::Message game_in;
    game_in.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::GameInSyn);
    game_in.header.object_id = 450035717u;
    handler.on_message(connection, game_in);

    ASSERT_EQ(reply.messages.size(), 1u);
    EXPECT_EQ(reply.messages.front().header.protocol,
              static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInNack));
}

TEST(AgentHandlerTest, ChangeMapUsesTargetRouteAndClosesCurrentRoute) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply), true,
                                      false, {}, /*default_map_num=*/12);
    const auto connection = mxh::net::make_connection_id(1101);
    handler.register_session(connection, 3001u, 450035713u, 7u);

    MockTcpSender current_map;
    MockTcpSender target_map;
    handler.set_map_server(&current_map, mxh::net::make_connection_id(7));
    handler.set_map_server_for_map(10u, &target_map,
                                   mxh::net::make_connection_id(10));

    mxh::net::Message change;
    change.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    change.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::ChangeMapSyn);
    change.header.object_id = 450035713u;
    change.payload.resize(4, 0);
    const std::uint16_t target_num = 10u;
    std::memcpy(change.payload.data(), &target_num, sizeof(target_num));
    handler.on_message(connection, change);

    ASSERT_EQ(current_map.sent_msgs.size(), 1u);
    EXPECT_EQ(current_map.sent_msgs.front().header.protocol,
              static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameOutSyn));
    EXPECT_TRUE(target_map.sent_msgs.empty());
    EXPECT_TRUE(reply.messages.empty());
    mxh::net::Message source_ack;
    source_ack.header = change.header;
    source_ack.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameOutAck);
    handler.forward_from_map({1}, source_ack, 10); // Wrong source cannot advance the transfer.
    EXPECT_TRUE(target_map.sent_msgs.empty());
    handler.forward_from_map({1}, source_ack, 7);
    ASSERT_EQ(target_map.sent_msgs.size(), 1u);
    EXPECT_EQ(target_map.sent_msgs.front().header.protocol,
              static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn));
    EXPECT_EQ(target_map.sent_msgs.front().header.object_id, 450035713u);
    EXPECT_TRUE(reply.messages.empty());
    mxh::net::Message entry_ack;
    entry_ack.header = change.header;
    entry_ack.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInAck);
    entry_ack.payload.resize(3775, 0);
    const std::uint32_t character = 450035713u, user = 3001u;
    std::memcpy(entry_ack.payload.data(), &character, 4);
    std::memcpy(entry_ack.payload.data() + 4, &user, 4);
    std::memcpy(entry_ack.payload.data() + 77, &target_num, 2);
    handler.forward_from_map({1}, entry_ack, 10);
    ASSERT_EQ(reply.messages.size(), 2u);
    EXPECT_EQ(reply.messages.front().header.protocol,
              static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::ChangeMapAck));
    EXPECT_EQ(reply.messages.back().header.protocol,
              static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInAck));
    handler.forward_from_map({1}, entry_ack, 10);
    handler.forward_from_map({1}, source_ack, 7);
    EXPECT_EQ(reply.messages.size(), 2u); // Terminal handshake duplicates must not replay entry.
}

TEST(AgentHandlerTest, ChangeMapWithoutTargetRouteReturnsNackAndKeepsCurrentMap) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply), true,
                                      false, {}, /*default_map_num=*/12);
    const auto connection = mxh::net::make_connection_id(1102);
    handler.register_session(connection, 3001u, 450035714u, 7u);

    MockTcpSender current_map;
    handler.set_map_server(&current_map, mxh::net::make_connection_id(7));

    mxh::net::Message change;
    change.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    change.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::ChangeMapSyn);
    change.header.object_id = 450035714u;
    change.payload.resize(4, 0);
    const std::uint16_t unavailable_map = 99u;
    std::memcpy(change.payload.data(), &unavailable_map,
                sizeof(unavailable_map));
    handler.on_message(connection, change);

    EXPECT_TRUE(current_map.sent_msgs.empty());
    ASSERT_EQ(reply.messages.size(), 1u);
    EXPECT_EQ(reply.messages.front().header.protocol,
              static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::ChangeMapNack));
}

TEST(AgentHandlerTest, ChangeMapBootstrapsTargetMapHandlerAndRelaysGameInAck) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply), true,
                                      false, {}, /*default_map_num=*/7);
    const auto client_connection = mxh::net::make_connection_id(1103);
    const auto target_connection = mxh::net::make_connection_id(1010);
    constexpr std::uint32_t char_id = 450035715u;
    constexpr std::uint16_t target_map = 10u;
    handler.register_session(client_connection, 3001u, char_id, 7u);

    MockTcpSender current_map;
    handler.set_map_server(&current_map, mxh::net::make_connection_id(7));
    mxh::server::MapHandler target_handler(
        db, target_map,
        [&](mxh::net::ConnectionId id, const mxh::net::Message& msg) {
            handler.forward_from_map(id, msg, target_map);
        });
    MapHandlerForwardingSender target_map_sender(target_handler,
                                                 target_connection);
    handler.set_map_server_for_map(target_map, &target_map_sender,
                                   target_connection);

    mxh::net::Message change;
    change.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    change.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::ChangeMapSyn);
    change.header.object_id = char_id;
    change.payload.resize(4, 0);
    std::memcpy(change.payload.data(), &target_map, sizeof(target_map));
    handler.on_message(client_connection, change);

    EXPECT_FALSE(target_handler.player_runtime_snapshot(char_id).has_value());
    mxh::net::Message source_ack;
    source_ack.header = change.header;
    source_ack.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameOutAck);
    handler.forward_from_map({7}, source_ack, 7);

    const auto snapshot = target_handler.player_runtime_snapshot(char_id);
    ASSERT_TRUE(snapshot.has_value());
    EXPECT_EQ(snapshot->lifecycle, mxh::server::PlayerLifecycle::Active);
    EXPECT_EQ(snapshot->map_num, target_map);
    EXPECT_EQ(target_map_sender.send_count.load(), 1);
    ASSERT_FALSE(current_map.sent_msgs.empty());
    EXPECT_EQ(current_map.sent_msgs.front().header.protocol,
              static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameOutSyn));

    bool saw_game_in_ack = false;
    bool saw_change_map_ack = false;
    for (const auto& message : reply.messages) {
        saw_game_in_ack |= message.header.protocol == static_cast<std::uint8_t>(
            mxh::proto::UserConnProtocol::GameInAck);
        saw_change_map_ack |= message.header.protocol == static_cast<std::uint8_t>(
            mxh::proto::UserConnProtocol::ChangeMapAck);
    }
    EXPECT_TRUE(saw_game_in_ack);
    EXPECT_TRUE(saw_change_map_ack);
    ASSERT_GE(reply.messages.size(), 2u);
    EXPECT_EQ(reply.messages[0].header.protocol, static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::ChangeMapAck));
    EXPECT_EQ(reply.messages[1].header.protocol, static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInAck));
}

TEST(AgentHandlerTest, TransferRejectionRestoresSourceBeforeResumingCommands) {
    using P = mxh::proto::UserConnProtocol;
    for (const bool reject_source : {true, false}) {
        SCOPED_TRACE(reject_source);
        MockDbAdapter db;
        ReplySpy reply;
        mxh::server::AgentHandler handler(db, make_reply_spy(reply), true, false, {}, 7);
        MockTcpSender source, target;
        handler.set_map_server(&source, {1});
        handler.set_map_server_for_map(10, &target, {1});
        handler.register_session({1101}, 3001, 777, 7);
        auto message = [](P protocol) {
            mxh::net::Message m;
            m.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
            m.header.protocol = static_cast<std::uint8_t>(protocol);
            m.header.object_id = 777;
            return m;
        };
        auto change = message(P::ChangeMapSyn);
        change.payload = {10, 0};
        handler.on_message({1101}, change);
        auto move = message(P::GameInSyn);
        move.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Move);
        handler.on_message({1101}, move);
        handler.on_message({1101}, change);
        ASSERT_EQ(source.sent_msgs.size(), 1u); // Neither movement nor duplicate request reaches the source.
        if (reject_source) {
            handler.forward_from_map({1}, message(P::GameOutNack), 7);
            handler.forward_from_map({1}, message(P::GameOutNack), 7);
            EXPECT_EQ(reply.messages.size(), 1u);
            EXPECT_TRUE(target.sent_msgs.empty());
        } else {
            handler.forward_from_map({1}, message(P::GameOutAck), 7);
            ASSERT_EQ(target.sent_msgs.size(), 1u);
            handler.forward_from_map({1}, message(P::GameInNack), 10);
            ASSERT_EQ(source.sent_msgs.size(), 2u);
            EXPECT_EQ(source.sent_msgs.back().header.protocol, static_cast<std::uint8_t>(P::GameInSyn));
            EXPECT_TRUE(reply.messages.empty()); // Rejection is not recovery until source admission succeeds.
            auto restored = message(P::GameInAck);
            restored.payload.resize(3775, 0);
            const std::uint32_t character = 777, user = 3001;
            const std::uint16_t map = 7;
            std::memcpy(restored.payload.data(), &character, 4);
            std::memcpy(restored.payload.data() + 4, &user, 4);
            std::memcpy(restored.payload.data() + 77, &map, 2);
            handler.forward_from_map({1}, restored, 7);
            ASSERT_EQ(reply.messages.size(), 2u);
            EXPECT_EQ(reply.messages.back().header.protocol, static_cast<std::uint8_t>(P::GameInAck));
        }
        ASSERT_FALSE(reply.messages.empty());
        EXPECT_EQ(reply.messages.front().header.protocol, static_cast<std::uint8_t>(reject_source ? P::ChangeMapNack : P::ChangeMapAck));
        const auto before = source.sent_msgs.size();
        handler.on_message({1101}, move);
        EXPECT_EQ(source.sent_msgs.size(), before + 1);
    }
}

TEST(AgentHandlerTest, TransferUncertainOutcomeFreezesLateRepliesAndCleansPossibleOwner) {
    using P = mxh::proto::UserConnProtocol;
    // Timeout before exit, timeout after exit, malformed admission, send failure.
    for (int failure = 0; failure != 4; ++failure) {
        SCOPED_TRACE(failure);
        MockDbAdapter db;
        ReplySpy reply;
        mxh::server::AgentHandler handler(db, make_reply_spy(reply), true, false, {}, 7);
        MockTcpSender source, target;
        handler.set_map_server(&source, {1});
        handler.set_map_server_for_map(10, &target, {1});
        handler.register_session({1101}, 3001, 777, 7);
        mxh::net::Message m;
        m.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
        m.header.object_id = 777;
        m.header.protocol = static_cast<std::uint8_t>(P::ChangeMapSyn);
        m.payload = {10, 0};
        handler.on_message({1101}, m);
        if (failure == 3) target.set_fail_send(true);
        if (failure != 0) {
            m.header.protocol = static_cast<std::uint8_t>(P::GameOutAck);
            m.payload.clear();
            handler.forward_from_map({1}, m, 7);
        }
        if (failure == 2) {
            m.header.protocol = static_cast<std::uint8_t>(P::GameInAck);
            handler.forward_from_map({1}, m, 10); // Empty / malformed payload must never commit the route.
        }
        const auto closes = handler.poll_map_transfers(std::chrono::steady_clock::now() + std::chrono::seconds(30));
        ASSERT_EQ(closes.size(), 1u);
        EXPECT_EQ(closes[0].value, 1101u);
        ASSERT_EQ(reply.messages.size(), 1u);
        EXPECT_EQ(reply.messages[0].header.protocol, static_cast<std::uint8_t>(P::GameInNack));
        m.header.protocol = static_cast<std::uint8_t>(P::GameOutAck);
        handler.forward_from_map({1}, m, 7);
        m.header.protocol = static_cast<std::uint8_t>(P::GameInAck);
        handler.forward_from_map({1}, m, 10);
        EXPECT_TRUE(handler.poll_map_transfers(std::chrono::steady_clock::now() + std::chrono::seconds(60)).empty());
        EXPECT_EQ(reply.messages.size(), 1u); // Late replies cannot reopen a failed transfer.
        const auto source_count = source.sent_msgs.size(), target_count = target.sent_msgs.size();
        m.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Move);
        handler.on_message({1101}, m);
        EXPECT_EQ(source.sent_msgs.size(), source_count);
        EXPECT_EQ(target.sent_msgs.size(), target_count);
        target.set_fail_send(false);
        handler.on_disconnect({1101}, mxh::net::NetError::Disconnected);
        if (failure == 0) EXPECT_EQ(source.sent_msgs.size(), source_count + 1);
        else {
            EXPECT_EQ(source.sent_msgs.size(), source_count);
            ASSERT_EQ(target.sent_msgs.size(), target_count + 1);
            EXPECT_EQ(target.sent_msgs.back().header.protocol, static_cast<std::uint8_t>(P::GameOutSyn));
        }
    }
}

TEST(AgentHandlerTest, DisconnectDetachesRouteBeforeSynchronousCleanupAndPreservesNewOwner) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply), true, false, {}, 7);
    MockTcpSender source;
    handler.set_map_server(&source, {1});
    handler.register_session({1101}, 3001, 777, 7);
    source.on_send = [&](const mxh::net::Message& request) {
        auto response = request;
        response.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameOutAck);
        response.payload.clear();
        handler.forward_from_map({1}, response, 7);
    };
    handler.on_disconnect({1101}, mxh::net::NetError::Disconnected);
    ASSERT_EQ(source.sent_msgs.size(), 1u);
    EXPECT_TRUE(reply.messages.empty());
    handler.register_session({1102}, 3001, 777, 7);
    handler.register_session({1103}, 3001, 777, 7);
    handler.on_disconnect({1102}, mxh::net::NetError::Disconnected);
    EXPECT_EQ(source.sent_msgs.size(), 1u); // Old connection must not log out the rebound character.
    mxh::net::Message state;
    state.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    state.header.object_id = 777;
    handler.forward_from_map({1}, state, 7);
    ASSERT_EQ(reply.messages.size(), 1u);
    EXPECT_EQ(reply.last_id.value, 1103u);
}

TEST(AgentHandlerTest, DisconnectSynAcknowledgesAndClearsMapRoute) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply), true);
    const auto connection = mxh::net::make_connection_id(1001);
    handler.register_session(connection, 3001, 450035712, 12, 5);
    MockTcpSender map;
    handler.set_map_server(&map, mxh::net::make_connection_id(42));

    mxh::net::Message disconnect;
    disconnect.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    disconnect.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::DisconnectSyn);
    handler.on_message(connection, disconnect);

    ASSERT_EQ(reply.call_count.load(), 1);
    EXPECT_EQ(reply.last_message.header.protocol, static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::DisconnectAck));
    EXPECT_TRUE(reply.last_message.payload.empty());
    ASSERT_EQ(map.sent_msgs.size(), 1u);
    EXPECT_EQ(map.sent_msgs[0].header.protocol, static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::GameOutSyn));
    EXPECT_EQ(map.sent_msgs[0].header.object_id, 450035712u);
    EXPECT_EQ(handler.user_level(connection), 0u);

    handler.on_disconnect(connection, mxh::net::NetError::Disconnected);
    EXPECT_EQ(map.sent_msgs.size(), 1u);
}

TEST(AgentHandlerTest, DisconnectSynWithoutCharacterDoesNotTouchMap) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply), true);
    const auto connection = mxh::net::make_connection_id(1002);
    handler.register_session(connection, 3002, 0, 0);
    MockTcpSender map;
    handler.set_map_server(&map, mxh::net::make_connection_id(42));

    mxh::net::Message disconnect;
    disconnect.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    disconnect.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::DisconnectSyn);
    handler.on_message(connection, disconnect);

    ASSERT_EQ(reply.call_count.load(), 1);
    EXPECT_EQ(reply.last_message.header.protocol, static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::DisconnectAck));
    EXPECT_TRUE(map.sent_msgs.empty());
}

TEST(AgentHandlerTest, ReviveRequestsUseOwnedMapRouteAndRejectResponsePayloads) {
    MockDbAdapter db; ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply), true);
    const auto connection=mxh::net::make_connection_id(1002);
    handler.register_session(connection,3002,777,10);
    MockTcpSender fallback, map;
    handler.set_map_server(&fallback,{42});
    handler.set_map_server_for_map(10,&map,{43});
    mxh::net::Message request;
    request.header.category=static_cast<std::uint8_t>(mxh::proto::Category::CharRevive);
    request.header.object_id=999; // Spoofed identity must be replaced.
    for(const auto protocol:{0,3,6}) {
        request.header.protocol=static_cast<std::uint8_t>(protocol);
        handler.on_message(connection,request);
    }
    ASSERT_EQ(map.sent_msgs.size(),3u);
    for(const auto& message:map.sent_msgs) {
        EXPECT_EQ(message.header.object_id,777u);
        EXPECT_TRUE(message.payload.empty());
    }
    EXPECT_TRUE(fallback.sent_msgs.empty());
    for(const auto protocol:{1,2,4,5,7,8,255}) {
        request.header.protocol=static_cast<std::uint8_t>(protocol);
        handler.on_message(connection,request);
    }
    request.header.protocol=0; request.payload={0};
    handler.on_message(connection,request);
    request.payload.clear();
    handler.on_message({9999},request);
    EXPECT_EQ(map.sent_msgs.size(),3u);
}

// ===========================================================================

// ===========================================================================
// R-2: HackShield routing tests
//
// Cover the data-plane: cat==HackShield messages route through the
// HackShieldManager state machine. Tests verify:
//   - non-superuser (UserLevel < 5) -> no reply, no state change
//   - superuser + GuidAck -> sends Req (160B)
//   - superuser + Ack -> no reply, state cleared
//   - superuser + Disconnect -> disconnect pending flag set
//   - on_disconnect cleans up hackshield_disconnect_pending_ entries
//   - register_session stores user_level + initializes HackShieldUserState
//   - inspection helpers return expected values
// ===========================================================================

TEST(AgentHandlerHackShieldTest, RegisterSessionStoresUserLevel) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    auto cid = mxh::net::make_connection_id(101);
    handler.register_session(cid, /*user_id=*/777, /*char_id=*/9001,
                              /*map_num=*/12, /*user_level=*/5);
    EXPECT_EQ(handler.user_level(cid), 5u);
}

TEST(AgentHandlerHackShieldTest, DefaultUserLevelIsZero) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    auto cid = mxh::net::make_connection_id(102);
    handler.register_session(cid, 1, 2, 3);  // no user_level -> default 0
    EXPECT_EQ(handler.user_level(cid), 0u);
    EXPECT_FALSE(handler.has_hackshield_state(cid));
    EXPECT_FALSE(handler.is_hackshield_disconnect_pending(cid));
}

TEST(AgentHandlerHackShieldTest, GuidReqNonSuperuserIsNoOp) {
    // UserLevel < HACKSHIELD_SUPERUSER_LEVEL (5) -> send_guid_req
    // returns hackshield_none(), so no reply should fire.
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    auto cid = mxh::net::make_connection_id(200);
    handler.register_session(cid, 1, 2, 3, /*user_level=*/2);

    mxh::net::Message msg;
    msg.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::HackShield);
    msg.header.protocol = static_cast<std::uint8_t>(
        mxh::server::HackShieldProtocol::GuidReq);
    handler.on_message(cid, msg);

    EXPECT_EQ(reply.call_count.load(), 0);
    EXPECT_FALSE(handler.has_hackshield_state(cid));
}

TEST(AgentHandlerHackShieldTest, GuidAckSuperuserSendsReq) {
    // superuser sends GuidAck (proto=1) -> state machine clears
    // m_bHSCheck to 0, then re-issues Req (proto=2, 160B).
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    auto cid = mxh::net::make_connection_id(201);
    handler.register_session(cid, 1, 2, 3, /*user_level=*/5);

    mxh::net::Message msg;
    msg.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::HackShield);
    msg.header.protocol = static_cast<std::uint8_t>(
        mxh::server::HackShieldProtocol::GuidAck);
    msg.payload.assign(mxh::server::HACKSHIELD_GUID_ACK_SIZE, 0xAB);
    handler.on_message(cid, msg);

    ASSERT_EQ(reply.call_count.load(), 1);
    EXPECT_EQ(reply.last_message.header.category,
              static_cast<std::uint8_t>(mxh::proto::Category::HackShield));
    EXPECT_EQ(reply.last_message.header.protocol,
              static_cast<std::uint8_t>(mxh::server::HackShieldProtocol::Req));
    EXPECT_EQ(reply.last_message.payload.size(),
              mxh::server::HACKSHIELD_REQ_SIZE);
    EXPECT_TRUE(handler.has_hackshield_state(cid));
    EXPECT_FALSE(handler.is_hackshield_disconnect_pending(cid));
}

TEST(AgentHandlerHackShieldTest, AckSuperuserClearsState) {
    // superuser sends Ack (proto=3) after a successful analysis ->
    // state machine clears m_bHSCheck to 0, no reply.
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    auto cid = mxh::net::make_connection_id(202);
    handler.register_session(cid, 1, 2, 3, /*user_level=*/5);

    mxh::net::Message msg;
    msg.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::HackShield);
    msg.header.protocol = static_cast<std::uint8_t>(
        mxh::server::HackShieldProtocol::Ack);
    msg.payload.assign(mxh::server::HACKSHIELD_ACK_SIZE, 0xCD);
    handler.on_message(cid, msg);

    EXPECT_EQ(reply.call_count.load(), 0);
    EXPECT_TRUE(handler.has_hackshield_state(cid));
    EXPECT_FALSE(handler.is_hackshield_disconnect_pending(cid));
}

TEST(AgentHandlerHackShieldTest, ClientDisconnectProtocolSetsPendingFlag) {
    // proto=4 (Disconnect) from client -> queue drop session.
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    auto cid = mxh::net::make_connection_id(203);
    handler.register_session(cid, 1, 2, 3, /*user_level=*/5);

    mxh::net::Message msg;
    msg.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::HackShield);
    msg.header.protocol = static_cast<std::uint8_t>(
        mxh::server::HackShieldProtocol::Disconnect);
    handler.on_message(cid, msg);

    EXPECT_EQ(reply.call_count.load(), 0);
    EXPECT_TRUE(handler.is_hackshield_disconnect_pending(cid));
}

TEST(AgentHandlerHackShieldTest, ReqSecondCallQueuesDisconnect) {
    // First Req from server: state machine sets m_bHSCheck=1 and
    // returns Send. Second Req with m_bHSCheck==1 returns Disconnect.
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    auto cid = mxh::net::make_connection_id(204);
    handler.register_session(cid, 1, 2, 3, /*user_level=*/5);

    // 1st Req: state was 0, sets to 1, returns Send.
    mxh::net::Message req1;
    req1.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::HackShield);
    req1.header.protocol = static_cast<std::uint8_t>(
        mxh::server::HackShieldProtocol::Req);
    handler.on_message(cid, req1);
    ASSERT_EQ(reply.call_count.load(), 1);
    EXPECT_EQ(reply.last_message.header.protocol,
              static_cast<std::uint8_t>(mxh::server::HackShieldProtocol::Req));
    EXPECT_EQ(reply.last_message.payload.size(),
              mxh::server::HACKSHIELD_REQ_SIZE);
    EXPECT_FALSE(handler.is_hackshield_disconnect_pending(cid));

    // 2nd Req: state was 1, returns Disconnect (no reply).
    mxh::net::Message req2;
    req2.header.category = req1.header.category;
    req2.header.protocol = req1.header.protocol;
    handler.on_message(cid, req2);
    EXPECT_EQ(reply.call_count.load(), 1);  // no second reply
    EXPECT_TRUE(handler.is_hackshield_disconnect_pending(cid));
}

TEST(AgentHandlerHackShieldTest, OnDisconnectClearsPendingFlag) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    auto cid = mxh::net::make_connection_id(205);
    handler.register_session(cid, 1, 2, 3, /*user_level=*/5);

    // Trigger disconnect pending via client Disconnect protocol.
    mxh::net::Message msg;
    msg.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::HackShield);
    msg.header.protocol = static_cast<std::uint8_t>(
        mxh::server::HackShieldProtocol::Disconnect);
    handler.on_message(cid, msg);
    ASSERT_TRUE(handler.is_hackshield_disconnect_pending(cid));

    // on_disconnect clears the pending flag and the HackShieldUserState.
    handler.on_disconnect(cid, mxh::net::NetError::Disconnected);
    EXPECT_FALSE(handler.is_hackshield_disconnect_pending(cid));
    EXPECT_FALSE(handler.has_hackshield_state(cid));
    EXPECT_EQ(handler.user_level(cid), 0u);  // user_level erased too
}

TEST(AgentHandlerHackShieldTest, NonHackShieldCatIsNotRouted) {
    // cat==Chat (6) -- make sure handle_hackshield is NOT called
    // for non-HackShield categories; existing on_message should
    // log "unhandled category" and not touch the HackShield map.
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    auto cid = mxh::net::make_connection_id(206);
    handler.register_session(cid, 1, 2, 3, /*user_level=*/5);

    mxh::net::Message msg;
    msg.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::Chat);
    msg.header.protocol = 0;
    handler.on_message(cid, msg);

    EXPECT_FALSE(handler.has_hackshield_state(cid));
    EXPECT_FALSE(handler.is_hackshield_disconnect_pending(cid));
}

TEST(AgentHandlerHackShieldTest, ConnIdMismatchDoesNotLeakState) {
    // Verify the per-connection map is keyed by id.value, not by
    // some other identifier; two different conn ids with the same
    // user_level should not interfere.
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));

    auto cid_a = mxh::net::make_connection_id(300);
    auto cid_b = mxh::net::make_connection_id(301);

    handler.register_session(cid_a, 1, 2, 3, /*user_level=*/5);
    handler.register_session(cid_b, 4, 5, 6, /*user_level=*/2);

    // Disconnect cid_a only.
    handler.on_disconnect(cid_a, mxh::net::NetError::Disconnected);
    EXPECT_EQ(handler.user_level(cid_a), 0u);
    EXPECT_EQ(handler.user_level(cid_b), 2u);
    EXPECT_FALSE(handler.has_hackshield_state(cid_a));
    EXPECT_FALSE(handler.has_hackshield_state(cid_b));
}



// ===========================================================================
// R-2.1: handle_legacy_character_list auto-populates user_level from DB
//
// Before R-2.1 the AgentHandler required a manual register_session call
// to set user_level before the HackShield gate would let any cat=67
// message through. After R-2.1, handle_legacy_character_list queries
// chr_log_info.userlevel automatically and stores it. This makes
// end-to-end HackShield routing work without any test scaffolding:
// 1) Client sends CharacterListSyn with user_id
// 2) Agent queries chr_log_info.userlevel
// 3) Agent stores the level in conn_user_levels_
// 4) Subsequent HackShield messages see the right threshold
// ===========================================================================

TEST(AgentHandlerHackShieldTest, CharacterListPopulatesUserLevelFromDb) {
    MockDbAdapter db;
    db.userlevel_to_return.store(5u);  // superuser
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply), true);
    auto cid = mxh::net::make_connection_id(700);

    // Drive CharacterListSyn with user_id=42 in the payload.
    mxh::net::Message msg;
    msg.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    msg.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::CharacterListSyn);
    msg.payload.resize(8, 0);
    std::uint32_t user_id = 42u;
    std::memcpy(msg.payload.data(), &user_id, 4);
    handler.on_message(cid, msg);

    EXPECT_EQ(handler.user_level(cid), 5u);

    // Now send a HackShield GuidAck -- the superuser gate should
    // fire and reply with a Req packet (160B).
    mxh::net::Message hs;
    hs.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::HackShield);
    hs.header.protocol = static_cast<std::uint8_t>(
        mxh::server::HackShieldProtocol::GuidAck);
    hs.payload.assign(mxh::server::HACKSHIELD_GUID_ACK_SIZE, 0xEF);
    handler.on_message(cid, hs);

    ASSERT_GE(reply.call_count.load(), 1);
    bool found_req = false;
    for (const auto& m : reply.messages) {
        if (m.header.category ==
                static_cast<std::uint8_t>(mxh::proto::Category::HackShield)
            && m.header.protocol ==
                static_cast<std::uint8_t>(mxh::server::HackShieldProtocol::Req)) {
            EXPECT_EQ(m.payload.size(), mxh::server::HACKSHIELD_REQ_SIZE);
            found_req = true;
            break;
        }
    }
    EXPECT_TRUE(found_req) << "superuser GuidAck should trigger Req reply";
}

TEST(AgentHandlerHackShieldTest, CharacterListNonSuperuserGatesHackShield) {
    MockDbAdapter db;
    db.userlevel_to_return.store(2u);  // NOT a superuser
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply), true);
    auto cid = mxh::net::make_connection_id(701);

    mxh::net::Message msg;
    msg.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    msg.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::CharacterListSyn);
    msg.payload.resize(8, 0);
    std::uint32_t user_id = 99u;
    std::memcpy(msg.payload.data(), &user_id, 4);
    handler.on_message(cid, msg);

    EXPECT_EQ(handler.user_level(cid), 2u);

    // GuidAck from non-superuser -- no reply, no state.
    mxh::net::Message hs;
    hs.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::HackShield);
    hs.header.protocol = static_cast<std::uint8_t>(
        mxh::server::HackShieldProtocol::GuidAck);
    handler.on_message(cid, hs);

    EXPECT_FALSE(handler.has_hackshield_state(cid));
}

TEST(AgentHandlerHackShieldTest, CharacterListMissingUserLevelDefaultsZero) {
    // userlevel_to_return left at default 0xFFu -- chr_log_info query
    // returns empty ResultSet (MockDbAdapter default) -- so
    // conn_user_levels_ is 0 (same as no register_session call).
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply), true);
    auto cid = mxh::net::make_connection_id(702);

    mxh::net::Message msg;
    msg.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    msg.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::CharacterListSyn);
    msg.payload.resize(8, 0);
    std::uint32_t user_id = 7u;
    std::memcpy(msg.payload.data(), &user_id, 4);
    handler.on_message(cid, msg);

    EXPECT_EQ(handler.user_level(cid), 0u);
    EXPECT_FALSE(handler.has_hackshield_state(cid));
}


// MapHandler
// ===========================================================================
// R-2.2: AgentHandler::tick_hackshield() server-side periodic recheck
//
// Walks every connection with active HackShield state and runs
// send_hackshield_req() through the same state machine the
// client-driven Req handler does. Legacy CHackShieldManager
// called this every ~30s. State machine decisions:
//   m_bHSCheck == 0 (idle)       -> send Req, -> 1 (waiting)
//   m_bHSCheck == 1 (waiting)    -> Disconnect (timeout)
//   m_bHSCheck == 2 (just-grace) -> 1 (waiting) without send
// ===========================================================================

TEST(AgentHandlerHackShieldTest, TickHackshieldOnEmptyHandlerIsZero) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    EXPECT_EQ(handler.tick_hackshield(), 0u);
    EXPECT_EQ(reply.call_count.load(), 0);
}

TEST(AgentHandlerHackShieldTest, TickHackshieldGraceSuperuserNoOpThenDisconnect) {
    // Send a GuidReq to push state machine into m_bHSCheck=2 (grace).
    // First tick: grace -> waiting (no send). Second tick: waiting ->
    // Disconnect (timeout exceeded).
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    auto cid = mxh::net::make_connection_id(800);
    handler.register_session(cid, 1, 2, 3, /*user_level=*/5);
    mxh::net::Message req;
    req.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::HackShield);
    req.header.protocol = static_cast<std::uint8_t>(
        mxh::server::HackShieldProtocol::GuidReq);
    handler.on_message(cid, req);
    EXPECT_TRUE(handler.has_hackshield_state(cid));
    reply.call_count.store(0);
    EXPECT_EQ(handler.tick_hackshield(), 0u);
    EXPECT_EQ(reply.call_count.load(), 0);
    EXPECT_GE(handler.tick_hackshield(), 1u);
    EXPECT_TRUE(handler.is_hackshield_disconnect_pending(cid));
}

TEST(AgentHandlerHackShieldTest, TickHackshieldNonSuperuserIsNoOp) {
    // Non-superuser: state should never have been created by the
    // client-driven handler (UserLevel < 5 guard), so tick
    // finds no entries to walk.
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    auto cid = mxh::net::make_connection_id(801);
    handler.register_session(cid, 1, 2, 3, /*user_level=*/2);
    mxh::net::Message req;
    req.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::HackShield);
    req.header.protocol = static_cast<std::uint8_t>(
        mxh::server::HackShieldProtocol::GuidReq);
    handler.on_message(cid, req);
    EXPECT_FALSE(handler.has_hackshield_state(cid));
    EXPECT_EQ(handler.tick_hackshield(), 0u);
    EXPECT_EQ(reply.call_count.load(), 0);
}

TEST(AgentHandlerHackShieldTest, TickHackshieldMixedSuperusersHandlesEach) {
    // Two superusers + one non-superuser. Two ticks produce 2
    // disconnect actions on the superusers only.
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    auto cid_super_a = mxh::net::make_connection_id(810);
    auto cid_super_b = mxh::net::make_connection_id(811);
    auto cid_normal = mxh::net::make_connection_id(812);
    handler.register_session(cid_super_a, 1, 2, 3, /*user_level=*/5);
    handler.register_session(cid_super_b, 4, 5, 6, /*user_level=*/5);
    handler.register_session(cid_normal, 7, 8, 9, /*user_level=*/3);
    auto send_guid = [&](mxh::net::ConnectionId id) {
        mxh::net::Message req;
        req.header.category = static_cast<std::uint8_t>(
            mxh::proto::Category::HackShield);
        req.header.protocol = static_cast<std::uint8_t>(
            mxh::server::HackShieldProtocol::GuidReq);
        handler.on_message(id, req);
    };
    send_guid(cid_super_a);
    send_guid(cid_super_b);
    send_guid(cid_normal);
    EXPECT_EQ(handler.tick_hackshield(), 0u);
    EXPECT_EQ(handler.tick_hackshield(), 2u);
    EXPECT_TRUE(handler.is_hackshield_disconnect_pending(cid_super_a));
    EXPECT_TRUE(handler.is_hackshield_disconnect_pending(cid_super_b));
    EXPECT_FALSE(handler.is_hackshield_disconnect_pending(cid_normal));
}

TEST(AgentHandlerCharacterRemoveTest, DeletesOnlyOwnedCharacterAndDependentState) {
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg{};
    cfg.backend = "sqlite";
    cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());
    ASSERT_TRUE(db.exec_multi(
        "CREATE TABLE character_info (chrid INTEGER PRIMARY KEY,userid TEXT,charname TEXT);"
        "CREATE TABLE modern_player_state (player_id INTEGER);"
        "CREATE TABLE modern_player_item (player_id INTEGER);"
        "CREATE TABLE modern_player_quest_log (player_id INTEGER);"
        "CREATE TABLE modern_player_quest_sub (player_id INTEGER);"
        "CREATE TABLE modern_item_grant (character_id INTEGER);"
        "INSERT INTO character_info VALUES(1001,'42','OwnedHero');"
        "INSERT INTO character_info VALUES(1002,'99','OtherHero');"
        "INSERT INTO modern_player_state VALUES(1001);"
        "INSERT INTO modern_player_item VALUES(1001);"
        "INSERT INTO modern_player_quest_log VALUES(1001);"
        "INSERT INTO modern_player_quest_sub VALUES(1001);"
        "INSERT INTO modern_item_grant VALUES(1001);").ok());

    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply), true);
    const auto connection = mxh::net::make_connection_id(501);
    handler.register_session(connection, 42, 0, 0);

    mxh::net::Message remove;
    remove.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    remove.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::CharacterRemoveSyn);
    remove.payload.resize(sizeof(std::uint32_t));
    const std::uint32_t character_id = 1001;
    std::memcpy(remove.payload.data(), &character_id, sizeof(character_id));
    handler.on_message(connection, remove);

    ASSERT_EQ(reply.call_count.load(), 1);
    EXPECT_EQ(reply.last_message.header.protocol, static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::CharacterRemoveAck));
    const std::vector<mxh::db::Bind> no_args;
    mxh::db::ResultSet rows;
    ASSERT_TRUE(db.query("SELECT chrid FROM character_info ORDER BY chrid", no_args, rows).ok());
    ASSERT_EQ(rows.rows.size(), 1u);
    EXPECT_EQ(std::get<std::int64_t>(rows.rows[0][0]), 1002);
    for (const auto table : {"modern_player_state", "modern_player_item",
                             "modern_player_quest_log", "modern_player_quest_sub",
                             "modern_item_grant"}) {
        rows = {};
        ASSERT_TRUE(db.query("SELECT * FROM " + std::string(table), no_args, rows).ok());
        EXPECT_TRUE(rows.empty()) << table;
    }
}

TEST(AgentHandlerCharacterRemoveTest, RejectsCharacterOwnedByAnotherUser) {
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg{};
    cfg.backend = "sqlite";
    cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());
    const std::vector<mxh::db::Bind> no_args;
    ASSERT_TRUE(db.execute(
        "CREATE TABLE character_info (chrid INTEGER PRIMARY KEY,userid TEXT,charname TEXT)", no_args).ok());
    ASSERT_TRUE(db.execute(
        "INSERT INTO character_info VALUES(1002,'99','OtherHero')", no_args).ok());

    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply), true);
    const auto connection = mxh::net::make_connection_id(502);
    handler.register_session(connection, 42, 0, 0);
    mxh::net::Message remove;
    remove.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    remove.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::CharacterRemoveSyn);
    remove.payload.resize(sizeof(std::uint32_t));
    const std::uint32_t character_id = 1002;
    std::memcpy(remove.payload.data(), &character_id, sizeof(character_id));
    handler.on_message(connection, remove);

    ASSERT_EQ(reply.call_count.load(), 1);
    EXPECT_EQ(reply.last_message.header.protocol, static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::CharacterRemoveNack));
    mxh::db::ResultSet rows;
    ASSERT_TRUE(db.query("SELECT chrid FROM character_info", no_args, rows).ok());
    EXPECT_EQ(rows.rows.size(), 1u);
}


// ===========================================================================

TEST(MapHandlerTest, ConstructionWithMapNumDoesNotCrash) {
    // MapHandler takes (db, map_num, reply, use_legacy_framing=true).
    // The header notes "legacy framing is mandatory for MapServer" so
    // the default is true; we pass false anyway to exercise the
    // non-default path.
    MockDbAdapter db;
    mxh::server::MapHandler h1(db, /*map_num=*/7, mxh::server::ReplyFn{});
    mxh::server::MapHandler h2(db, /*map_num=*/7, mxh::server::ReplyFn{}, true);
    SUCCEED();
}

TEST(MapHandlerTest, PartyCreateAndBreakupMutateAuthoritativeMapState) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 10, make_reply_spy(reply));
    const auto connection = mxh::net::make_connection_id(77);

    mxh::net::Message game_in;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    game_in.header.object_id = 123u;
    handler.on_message(connection, game_in);

    mxh::net::Message create;
    create.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Party);
    create.header.protocol = static_cast<std::uint8_t>(mxh::proto::PartyProtocol::CreateSyn);
    create.header.object_id = 123u;
    create.payload = {0};
    handler.on_message(connection, create);
    ASSERT_GE(reply.messages.size(), 2u); // GameInAck + PartyCreateAck
    const auto& created = reply.messages.back();
    EXPECT_EQ(created.header.protocol,
              static_cast<std::uint8_t>(mxh::proto::PartyProtocol::CreateAck));
    ASSERT_EQ(created.payload.size(), 28u);
    std::uint32_t party_id = 0;
    std::memcpy(&party_id, created.payload.data(), sizeof(party_id));
    EXPECT_EQ(party_id, 1u);
    EXPECT_EQ(created.payload[4], 1u);

    mxh::net::Message second_game_in = game_in;
    second_game_in.header.object_id = 456u;
    const auto second_connection = mxh::net::make_connection_id(78);
    handler.on_message(second_connection, second_game_in);

    mxh::net::Message invite = create;
    invite.header.protocol = static_cast<std::uint8_t>(mxh::proto::PartyProtocol::AddSyn);
    invite.payload.resize(8);
    std::memcpy(invite.payload.data(), &party_id, sizeof(party_id));
    const std::uint32_t target_id = 456u;
    std::memcpy(invite.payload.data() + 4, &target_id, sizeof(target_id));
    handler.on_message(connection, invite);
    bool saw_invite = false;
    for (const auto& message : reply.messages) {
        saw_invite |= message.header.protocol ==
                      static_cast<std::uint8_t>(mxh::proto::PartyProtocol::AddInvite) &&
                      message.header.object_id == target_id;
    }
    EXPECT_TRUE(saw_invite);

    mxh::net::Message accept;
    accept.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Party);
    accept.header.protocol = static_cast<std::uint8_t>(mxh::proto::PartyProtocol::InviteAcceptSyn);
    accept.header.object_id = target_id;
    accept.payload.resize(sizeof(party_id));
    std::memcpy(accept.payload.data(), &party_id, sizeof(party_id));
    handler.on_message(second_connection, accept);
    bool saw_join_ack = false;
    for (const auto& message : reply.messages) {
        if (message.header.protocol == static_cast<std::uint8_t>(
                mxh::proto::PartyProtocol::InviteAcceptAck) &&
            message.header.object_id == target_id) {
            saw_join_ack = true;
            ASSERT_GE(message.payload.size(), 5u);
            EXPECT_EQ(message.payload[4], 2u);
        }
    }
    EXPECT_TRUE(saw_join_ack);

    handler.on_message(connection, create);
    EXPECT_EQ(reply.messages.back().header.protocol,
              static_cast<std::uint8_t>(mxh::proto::PartyProtocol::CreateNack));

    mxh::net::Message breakup;
    breakup.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Party);
    breakup.header.protocol = static_cast<std::uint8_t>(mxh::proto::PartyProtocol::BreakupSyn);
    breakup.header.object_id = 123u;
    breakup.payload.resize(sizeof(party_id));
    std::memcpy(breakup.payload.data(), &party_id, sizeof(party_id));
    handler.on_message(connection, breakup);
    EXPECT_EQ(reply.messages.back().header.protocol,
              static_cast<std::uint8_t>(mxh::proto::PartyProtocol::BreakupAck));

    mxh::net::Message guild_create;
    guild_create.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Guild);
    guild_create.header.protocol = static_cast<std::uint8_t>(mxh::proto::GuildProtocol::CreateSyn);
    guild_create.header.object_id = 123u;
    guild_create.payload = {'K', 'n', 'i', 'g', 'h', 't', 's'};
    handler.on_message(connection, guild_create);
    ASSERT_EQ(reply.messages.back().header.protocol,
              static_cast<std::uint8_t>(mxh::proto::GuildProtocol::CreateAck));
    ASSERT_GE(reply.messages.back().payload.size(), 5u);
    std::uint32_t guild_id = 0;
    std::memcpy(&guild_id, reply.messages.back().payload.data(), sizeof(guild_id));
    EXPECT_EQ(reply.messages.back().payload[4], 1u);

    mxh::net::Message guild_invite = guild_create;
    guild_invite.header.protocol = static_cast<std::uint8_t>(mxh::proto::GuildProtocol::AddMemberSyn);
    guild_invite.payload.resize(8);
    std::memcpy(guild_invite.payload.data(), &guild_id, sizeof(guild_id));
    std::memcpy(guild_invite.payload.data() + 4, &target_id, sizeof(target_id));
    handler.on_message(connection, guild_invite);
    bool saw_guild_invite = false;
    for (const auto& message : reply.messages) {
        saw_guild_invite |= message.header.protocol == static_cast<std::uint8_t>(
                                mxh::proto::GuildProtocol::AddMemberInvite) &&
                            message.header.object_id == target_id;
    }
    EXPECT_TRUE(saw_guild_invite);

    mxh::net::Message guild_accept;
    guild_accept.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Guild);
    guild_accept.header.protocol = static_cast<std::uint8_t>(mxh::proto::GuildProtocol::InviteAccept);
    guild_accept.header.object_id = target_id;
    guild_accept.payload.resize(sizeof(guild_id));
    std::memcpy(guild_accept.payload.data(), &guild_id, sizeof(guild_id));
    handler.on_message(second_connection, guild_accept);
    bool saw_guild_join = false;
    for (const auto& message : reply.messages) {
        if (message.header.protocol == static_cast<std::uint8_t>(
                mxh::proto::GuildProtocol::InviteAccept) &&
            message.header.object_id == target_id) {
            saw_guild_join = true;
            ASSERT_GE(message.payload.size(), 5u);
            EXPECT_EQ(message.payload[4], 2u);
        }
    }
    EXPECT_TRUE(saw_guild_join);

    handler.on_message(connection, guild_create);
    EXPECT_EQ(reply.messages.back().header.protocol,
              static_cast<std::uint8_t>(mxh::proto::GuildProtocol::CreateNack));

    mxh::net::Message guild_breakup = guild_create;
    guild_breakup.header.protocol = static_cast<std::uint8_t>(mxh::proto::GuildProtocol::BreakupSyn);
    guild_breakup.payload.resize(sizeof(guild_id));
    std::memcpy(guild_breakup.payload.data(), &guild_id, sizeof(guild_id));
    handler.on_message(connection, guild_breakup);
    EXPECT_EQ(reply.messages.back().header.protocol,
              static_cast<std::uint8_t>(mxh::proto::GuildProtocol::BreakupAck));
}

TEST(MapHandlerTest, AllChatPersistsSanitizedAuditRecord) {
    auto db = mxh::db::make_adapter("sqlite");
    mxh::db::ConnectionConfig cfg;
    cfg.path = ":memory:";
    ASSERT_TRUE(db->connect(cfg).ok());
    ASSERT_TRUE(db->execute("CREATE TABLE log_chat (logid INTEGER PRIMARY KEY AUTOINCREMENT, chrname TEXT, channel TEXT, message TEXT, logtime TEXT)").ok());
    ReplySpy reply;
    mxh::server::MapHandler handler(*db, 7, make_reply_spy(reply));
    mxh::net::Message msg;
    msg.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Chat);
    msg.header.protocol = static_cast<std::uint8_t>(mxh::proto::ChatProtocol::All);
    msg.header.object_id = 42;
    msg.payload = {'h', 'i', 1, '!', 0, 'x'};
    handler.on_message(mxh::net::make_connection_id(9), msg);
    mxh::db::ResultSet rows;
    ASSERT_TRUE(db->query("SELECT chrname, channel, message FROM log_chat", rows).ok());
    ASSERT_EQ(rows.rows.size(), 1u);
    EXPECT_EQ(std::get<std::string>(rows.rows[0][0]), "#42");
    EXPECT_EQ(std::get<std::string>(rows.rows[0][1]), "all");
    EXPECT_EQ(std::get<std::string>(rows.rows[0][2]), "hi!");
}

TEST(MapHandlerTest, OnConnectNonLegacyReturnsTrue) {
    // MapHandler.on_connect accepts all clients (no version
    // negotiation, no auth key ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â those happen on Distribute/Agent).
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, /*map_num=*/7, make_reply_spy(reply));
    EXPECT_TRUE(handler.on_connect({}, "10.0.0.1:1234"));
}

TEST(MapHandlerTest, RuntimeSnapshotIsCreatedOnGameIn) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(mxh::net::make_connection_id(55), game_in);
    ASSERT_EQ(handler.player_runtime_count(), 1u);
    const auto snapshot = handler.player_runtime_snapshot(123u);
    ASSERT_TRUE(snapshot.has_value());
    EXPECT_EQ(snapshot->player_id, 123u);
    EXPECT_EQ(snapshot->map_num, 7u);
    EXPECT_FLOAT_EQ(snapshot->pos_x, 25000.0f);
    EXPECT_FLOAT_EQ(snapshot->pos_z, 25000.0f);
    EXPECT_EQ(snapshot->lifecycle, mxh::server::PlayerLifecycle::Active);
}

TEST(MapHandlerTest, AuthenticatedGameInRetainsAccountAndAppearanceInActor) {
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig config; config.path=":memory:";
    ASSERT_TRUE(db.connect(config).ok());
    ASSERT_TRUE(mxh::db::migrate_modern_schema(db).ok());
    ASSERT_TRUE(db.exec_multi("INSERT INTO character_info(chrid,userid,charname,map_num,sex_type,face_type,hair_type) "
                             "VALUES(777,'123','AppearanceHero',10,1,4,6);").ok());
    ReplySpy reply; mxh::server::MapHandler handler(db,10,make_reply_spy(reply));
    handler.set_allow_dev_gamein_fallback(false);
    mxh::net::Message request;
    request.header.category=static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    request.header.protocol=static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    request.header.object_id=777; request.payload.assign(16,0);
    request.payload[0]=124; // wrong authenticated owner must not create a runtime
    handler.on_message({99},request);
    EXPECT_FALSE(handler.player_runtime_snapshot(777));
    request.payload[0]=123;
    handler.on_message({99},request);
    const auto snapshot=handler.player_runtime_snapshot(777); ASSERT_TRUE(snapshot);
    EXPECT_EQ(snapshot->user_id,123u); EXPECT_EQ(snapshot->player_id,777u);
    EXPECT_EQ(snapshot->gender,1); EXPECT_EQ(snapshot->face_type,4); EXPECT_EQ(snapshot->hair_type,6);
}

TEST(MapHandlerTest, TargetMapGameInUsesTargetMapAndEmitsAck) {
    MockDbAdapter db;
    ReplySpy reply;
    constexpr std::uint32_t char_id = 450035713u;
    constexpr std::uint16_t target_map = 10u;
    const auto map_connection = mxh::net::make_connection_id(2010);
    mxh::server::MapHandler target(db, target_map, make_reply_spy(reply));

    mxh::net::Message game_in;
    game_in.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::GameInSyn);
    game_in.header.object_id = char_id;
    game_in.payload.resize(16, 0);
    std::uint32_t user_id = 3001u;
    std::memcpy(game_in.payload.data(), &user_id, sizeof(user_id));

    target.on_message(map_connection, game_in);

    const auto snapshot = target.player_runtime_snapshot(char_id);
    ASSERT_TRUE(snapshot.has_value());
    EXPECT_EQ(snapshot->lifecycle, mxh::server::PlayerLifecycle::Active);
    EXPECT_EQ(snapshot->map_num, target_map);
    EXPECT_EQ(snapshot->player_id, char_id);
    EXPECT_EQ(snapshot->user_id, user_id);

    const auto ack = std::find_if(reply.messages.begin(), reply.messages.end(),
        [](const mxh::net::Message& message) {
            return message.header.protocol == static_cast<std::uint8_t>(
                mxh::proto::UserConnProtocol::GameInAck);
        });
    ASSERT_NE(ack, reply.messages.end());
    EXPECT_EQ(ack->header.object_id, char_id);
}

TEST(MapHandlerTest, CharacterAddCarriesVisiblePlayerVitals) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));

    const auto enter = [&](std::uint32_t playerId, std::uint64_t connectionId) {
        mxh::net::Message gameIn;
        gameIn.header.object_id = playerId;
        gameIn.header.category = static_cast<std::uint8_t>(
            mxh::proto::Category::UserConn);
        gameIn.header.protocol = static_cast<std::uint8_t>(
            mxh::proto::UserConnProtocol::GameInSyn);
        handler.on_message(mxh::net::make_connection_id(connectionId), gameIn);
    };
    enter(101u, 51u);
    enter(202u, 52u);

    const auto message = std::find_if(
        reply.messages.begin(), reply.messages.end(),
        [](const mxh::net::Message& candidate) {
            return candidate.header.protocol == static_cast<std::uint8_t>(
                       mxh::proto::UserConnProtocol::CharacterAdd) &&
                   candidate.header.object_id == 202u;
        });
    ASSERT_NE(message, reply.messages.end());
    ASSERT_EQ(message->payload.size(), 288u);
    std::uint32_t currentLife = 0;
    std::uint32_t maxLife = 0;
    std::memcpy(&currentLife, message->payload.data() + 35,
                sizeof(currentLife));
    std::memcpy(&maxLife, message->payload.data() + 39,
                sizeof(maxLife));
    const auto actor=handler.player_runtime_snapshot(202u); ASSERT_TRUE(actor);
    EXPECT_EQ(currentLife, actor->current_hp);
    EXPECT_EQ(maxLife, actor->max_hp);
    const auto self=std::find_if(reply.messages.begin(),reply.messages.end(),[](const auto& packet) {
        return packet.header.object_id==202u && packet.header.protocol==static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInAck);
    });
    ASSERT_NE(self,reply.messages.end());
    std::uint32_t shield=0,maxShield=0,selfShield=0,selfMaxShield=0;
    std::memcpy(&shield,message->payload.data()+43,4); std::memcpy(&maxShield,message->payload.data()+47,4);
    std::memcpy(&selfShield,self->payload.data()+43,4); std::memcpy(&selfMaxShield,self->payload.data()+47,4);
    EXPECT_EQ(shield,selfShield); EXPECT_EQ(maxShield,selfMaxShield);
    EXPECT_GT(maxShield,0u); // fixture's level contribution must not be zero-filled

    // The admitted source defaults must reach both self and observer packets.
    // Zero-filling this block hides ordinary worn equipment in the Unity policy.
    std::size_t checked = 0;
    for (const auto& packet : reply.messages) {
        if (packet.header.category != static_cast<std::uint8_t>(mxh::proto::Category::UserConn)) continue;
        const bool self = packet.header.protocol == static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInAck);
        const bool other = packet.header.protocol == static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::CharacterAdd);
        if (!self && !other) continue;
        const std::size_t offset = self ? mxh::game::HERO_TOTAL_SHOP_OPTION_OFFSET : 161u;
        ASSERT_GE(packet.payload.size(), offset + 120u);
        for (std::size_t byte = 0; byte < 120; ++byte) {
            const auto expected = byte >= 24 && byte < 46 && byte % 2 == 0 ? 1 : 0;
            EXPECT_EQ(packet.payload[offset + byte], expected) << "shop byte=" << byte;
        }
        ++checked;
    }
    // Current Map routing copies each observer direction to both Agent links.
    EXPECT_EQ(checked, 6u); // two self ACKs and four observer copies
}

TEST(MapHandlerTest, GameInClaimsValidPendingGmGrantExactlyOnce) {
    auto db = mxh::db::make_adapter("sqlite"); mxh::db::ConnectionConfig cfg; cfg.path = ":memory:";
    ASSERT_TRUE(db->connect(cfg).ok());
    ASSERT_TRUE(db->execute("CREATE TABLE character_info (chrid INTEGER PRIMARY KEY,charname TEXT,level INTEGER,gender INTEGER,face INTEGER,hair INTEGER)").ok());
    ASSERT_TRUE(db->execute("INSERT INTO character_info VALUES (123,'GrantHero',1,0,0,0)").ok());
    ASSERT_TRUE(db->execute("CREATE TABLE modern_item_grant (grant_id INTEGER PRIMARY KEY,idempotency_key TEXT UNIQUE,character_id INTEGER,item_id INTEGER,item_count INTEGER,status TEXT,inventory_slot INTEGER,created_by TEXT,reason TEXT,created_at TEXT,claimed_at TEXT)").ok());
    ASSERT_TRUE(db->execute("INSERT INTO modern_item_grant VALUES (1,'once',123,8000,3,'pending',NULL,'gm','test',CURRENT_TIMESTAMP,NULL)").ok());
    ReplySpy reply; mxh::server::MapHandler handler(*db, 7, make_reply_spy(reply));
    const std::string item_path = std::string(MXH_SOURCE_DIR) + "/data/PlayDH/Resource/ItemList.bin";
    handler.load_item_list(item_path);
    mxh::net::Message game_in; game_in.header.object_id = 123;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(mxh::net::make_connection_id(55), game_in);
    const auto snapshot = handler.player_runtime_snapshot(123); ASSERT_TRUE(snapshot); EXPECT_EQ(snapshot->inventory_count, 1u);
    EXPECT_EQ(handler.claim_pending_item_grants_for_test(123), 0u);
    mxh::db::ResultSet rows; ASSERT_TRUE(db->query("SELECT status,inventory_slot FROM modern_item_grant WHERE grant_id=1", rows).ok());
    EXPECT_EQ(std::get<std::string>(rows.rows[0][0]), "claimed");
    EXPECT_EQ(std::get<std::int64_t>(rows.rows[0][1]), 0);
}

TEST(MapHandlerTest, GameInAckEmbedsCurrentItemLayoutWithoutLocalItemPacket) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    game_in.payload.resize(16, 0);
    const std::uint32_t authenticated_user = 42;
    std::memcpy(game_in.payload.data(), &authenticated_user, sizeof(authenticated_user));

    handler.on_message(mxh::net::make_connection_id(55), game_in);

    ASSERT_FALSE(reply.messages.empty());
    EXPECT_EQ(reply.call_count.load(), static_cast<int>(reply.messages.size()));
    const auto& ack = reply.messages.front();
    EXPECT_EQ(ack.header.category,
              static_cast<std::uint8_t>(mxh::proto::Category::UserConn));
    EXPECT_EQ(ack.header.protocol,
              static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInAck));
    ASSERT_EQ(ack.payload.size(), mxh::game::HERO_TOTAL_EMPTY_PAYLOAD_SIZE);
    std::uint32_t received_player = 0, received_user = 0;
    std::memcpy(&received_player, ack.payload.data(), sizeof(received_player));
    std::memcpy(&received_user, ack.payload.data() + 4, sizeof(received_user));
    EXPECT_EQ(received_player, 123u);
    EXPECT_EQ(received_user, authenticated_user);
    std::uint16_t spawn_x = 0, spawn_z = 0;
    std::memcpy(&spawn_x, ack.payload.data() + mxh::game::HERO_TOTAL_MOVE_OFFSET, sizeof(spawn_x));
    std::memcpy(&spawn_z, ack.payload.data() + mxh::game::HERO_TOTAL_MOVE_OFFSET + 2, sizeof(spawn_z));
    EXPECT_EQ(spawn_x, 25000u);
    EXPECT_EQ(spawn_z, 25000u);
    for (std::size_t offset = mxh::game::HERO_TOTAL_ITEM_OFFSET;
         offset < mxh::game::HERO_TOTAL_OPTION_COUNTS_OFFSET; ++offset) {
        EXPECT_EQ(ack.payload[offset], 0u);
    }
    EXPECT_EQ(ack.payload[mxh::game::HERO_TOTAL_ADDABLE_INFO_OFFSET], 0u);
    EXPECT_EQ(ack.payload[mxh::game::HERO_TOTAL_ADDABLE_INFO_OFFSET + 1], 0u);
    for (const auto& message : reply.messages) {
        EXPECT_NE(message.header.category,
                  static_cast<std::uint8_t>(mxh::proto::Category::Item));
    }
}

TEST(MapHandlerTest, GameInRestoresProgressAndSerializesLegacyHeroOffsets) {
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg{};
    cfg.backend = "sqlite";
    cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());
    ASSERT_TRUE(db.exec_multi(
        "CREATE TABLE character_info (chrid INTEGER PRIMARY KEY,charname TEXT,sex_type INTEGER,"
        "face_type INTEGER,hair_type INTEGER,height REAL,width REAL,level INTEGER,map_num INTEGER);"
        "CREATE TABLE modern_player_state (player_id INTEGER PRIMARY KEY,money INTEGER NOT NULL,"
        "level INTEGER NOT NULL,exp INTEGER NOT NULL,updated_at TEXT NOT NULL);"
        "INSERT INTO character_info VALUES(123,'PersistHero',0,1,1,1.0,1.0,1,7);"
        "INSERT INTO modern_player_state VALUES(123,7654321,12,34567,'now');").ok());

    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(mxh::net::make_connection_id(55), game_in);

    const auto snapshot = handler.player_runtime_snapshot(123u);
    ASSERT_TRUE(snapshot.has_value());
    EXPECT_EQ(snapshot->level, 12u);
    EXPECT_EQ(snapshot->level_exp, 34567u);
    EXPECT_EQ(handler.player_money_for_test(123u), 7654321u);
    ASSERT_FALSE(reply.messages.empty());
    const auto& ack = reply.messages.front();
    std::uint16_t wire_level = 0;
    std::int64_t wire_exp = 0;
    std::uint32_t wire_money = 0;
    std::memcpy(&wire_level, ack.payload.data() + mxh::game::HERO_TOTAL_CHARACTER_OFFSET + 40, sizeof(wire_level));
    std::memcpy(&wire_exp, ack.payload.data() + mxh::game::HERO_TOTAL_HERO_OFFSET + 22, sizeof(wire_exp));
    std::memcpy(&wire_money, ack.payload.data() + mxh::game::HERO_TOTAL_HERO_OFFSET + 36, sizeof(wire_money));
    EXPECT_EQ(wire_level, 12u);
    EXPECT_EQ(wire_exp, 34567);
    EXPECT_EQ(wire_money, 7654321u);
}

TEST(MapHandlerTest, InventoryAndEquipmentSurviveDisconnectAndRelogin) {
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg{};
    cfg.backend = "sqlite";
    cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());
    ASSERT_TRUE(mxh::db::migrate_modern_schema(db).ok());
    ASSERT_TRUE(db.exec_multi(
        "INSERT INTO character_info(chrid,userid,charname,sex_type,face_type,hair_type,height,width,level,map_num) "
        "VALUES(123,'123','ItemHero',0,1,1,1.0,1.0,1,7);"
        "INSERT INTO modern_player_item VALUES(123,0,4,9001,555,87,3,65535,8);"
        "INSERT INTO modern_player_item VALUES(123,1,2,9002,777,65,4,65535,1);").ok());

    const auto connection = mxh::net::make_connection_id(55);
    {
        ReplySpy reply;
        mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
        mxh::net::Message game_in;
        game_in.header.object_id = 123u;
        game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
        game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
        handler.on_message(connection, game_in);
        const auto snapshot = handler.player_runtime_snapshot(123u);
        ASSERT_TRUE(snapshot.has_value());
        EXPECT_EQ(snapshot->inventory_count, 1u);
        ASSERT_FALSE(reply.messages.empty());
        mxh::game::ItemTotalInfo items{};
        std::memcpy(&items, reply.messages.front().payload.data() + mxh::game::HERO_TOTAL_ITEM_OFFSET, sizeof(items));
        EXPECT_EQ(items.Inventory[4].dwDBIdx, 9001u);
        EXPECT_EQ(items.Inventory[4].wIconIdx, 555u);
        EXPECT_EQ(items.WearedItem[2].dwDBIdx, 9002u);
        EXPECT_EQ(items.WearedItem[2].wIconIdx, 777u);
        handler.on_disconnect(connection, mxh::net::NetError::Disconnected);
        EXPECT_FALSE(handler.is_draining());
    }
    mxh::db::ResultSet rows;
    const std::vector<mxh::db::Bind> no_args;
    ASSERT_TRUE(db.query("SELECT container,slot,item_idx,item_param FROM modern_player_item "
                         "WHERE player_id=123 ORDER BY container,slot", no_args, rows).ok());
    ASSERT_EQ(rows.rows.size(), 2u);
    EXPECT_EQ(std::get<std::int64_t>(rows.rows[0][1]), 4);
    EXPECT_EQ(std::get<std::int64_t>(rows.rows[0][2]), 555);
    EXPECT_EQ(std::get<std::int64_t>(rows.rows[1][1]), 2);
    EXPECT_EQ(std::get<std::int64_t>(rows.rows[1][2]), 777);
}

TEST(MapHandlerTest, ExtendedShopContainersSurviveExitAndFreshHandlerRelogin) {
    mxh::db::SqliteAdapter db; mxh::db::ConnectionConfig cfg; cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());
    ASSERT_TRUE(mxh::db::migrate_modern_schema(db).ok());
    ASSERT_TRUE(db.exec_multi(
        "INSERT INTO character_info(chrid,userid,charname,map_num) VALUES(123,'123','ShopHero',7);"
        "INSERT INTO modern_player_item VALUES(123,0,4,9001,555,87,3,65535,8);"
        "INSERT INTO modern_player_item VALUES(123,1,2,9002,777,65,4,65535,1);"
        "INSERT INTO modern_player_item VALUES(123,2,0,9003,55001,4294967295,3,65535,2147483648);"
        "INSERT INTO modern_player_item VALUES(123,2,19,9004,55002,71,4,65535,9);"
        "INSERT INTO modern_player_item VALUES(123,3,2,9005,55003,72,5,65535,10);"
        "INSERT INTO modern_player_item VALUES(123,4,6,9006,55004,73,6,65535,11);"
        "INSERT INTO modern_player_item VALUES(123,5,3,9007,55005,74,7,65535,12);").ok());
    for (int session = 0; session < 2; ++session) {
        ReplySpy reply; mxh::server::MapHandler handler(db,7,make_reply_spy(reply));
        handler.set_allow_dev_gamein_fallback(false);
        mxh::net::Message enter;
        enter.header.object_id=123;
        enter.header.category=static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
        enter.header.protocol=static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
        enter.payload.assign(16,0); enter.payload[0]=123;
        handler.on_message({55},enter);
        ASSERT_TRUE(handler.player_runtime_snapshot(123));
        const auto ack = std::find_if(reply.messages.begin(), reply.messages.end(), [](const auto& message) {
            return message.header.protocol == static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInAck);
        });
        ASSERT_NE(ack,reply.messages.end());
        ASSERT_GE(ack->payload.size(),mxh::game::HERO_TOTAL_ITEM_OFFSET+sizeof(mxh::game::ItemTotalInfo));
        mxh::game::ItemTotalInfo items{};
        std::memcpy(&items,ack->payload.data()+mxh::game::HERO_TOTAL_ITEM_OFFSET,sizeof(items));
        EXPECT_EQ(items.Inventory[4].dwDBIdx,9001u); EXPECT_EQ(items.WearedItem[2].dwDBIdx,9002u);
        EXPECT_EQ(items.ShopInventory[0].dwDBIdx,9003u); EXPECT_EQ(items.ShopInventory[0].wIconIdx,55001u);
        EXPECT_EQ(items.ShopInventory[0].Position,390u); EXPECT_EQ(items.ShopInventory[0].Durability,UINT32_MAX);
        EXPECT_EQ(items.ShopInventory[0].ItemParam,0x80000000u); EXPECT_EQ(items.ShopInventory[0].QuickPosition,65535u);
        EXPECT_EQ(items.ShopInventory[19].dwDBIdx,9004u); EXPECT_EQ(items.ShopInventory[19].Position,409u);
        EXPECT_EQ(items.PetWearedItem[2].dwDBIdx,9005u); EXPECT_EQ(items.PetWearedItem[2].Position,492u);
        EXPECT_EQ(items.TitanWearedItem[6].dwDBIdx,9006u); EXPECT_EQ(items.TitanWearedItem[6].Position,499u);
        EXPECT_EQ(items.TitanShopItem[3].dwDBIdx,9007u); EXPECT_EQ(items.TitanShopItem[3].Position,503u);
        if (session == 1) {
            ASSERT_TRUE(db.exec_multi("CREATE TRIGGER fail_shop_save BEFORE INSERT ON modern_player_item "
                "WHEN NEW.container=2 BEGIN SELECT RAISE(ABORT,'injected shop save failure'); END;").ok());
            auto leave = enter;
            leave.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameOutSyn);
            handler.on_message({55},leave);
            EXPECT_EQ(reply.last_message.header.protocol,static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameOutNack));
            EXPECT_TRUE(handler.player_runtime_snapshot(123));
            mxh::db::ResultSet retained;
            ASSERT_TRUE(db.query("SELECT container FROM modern_player_item WHERE player_id=123",{},retained).ok());
            EXPECT_EQ(retained.rows.size(),7u); // DELETE and preceding ordinary writes were rolled back.
            ASSERT_TRUE(db.exec_multi("DROP TRIGGER fail_shop_save;").ok());
        }
        handler.on_disconnect({55},mxh::net::NetError::Disconnected);
        EXPECT_FALSE(handler.is_draining());
        mxh::db::ResultSet rows;
        ASSERT_TRUE(db.query("SELECT container,slot,db_idx,item_idx,durability,rare_idx,quick_position,item_param "
                             "FROM modern_player_item WHERE player_id=123 ORDER BY container,slot",{},rows).ok());
        ASSERT_EQ(rows.rows.size(),7u);
        EXPECT_EQ(std::get<std::int64_t>(rows.rows[2][0]),2);
        EXPECT_EQ(std::get<std::int64_t>(rows.rows[2][4]),4294967295LL);
        EXPECT_EQ(std::get<std::int64_t>(rows.rows[2][7]),2147483648LL);
        EXPECT_EQ(std::get<std::int64_t>(rows.rows[3][1]),19);
        EXPECT_EQ(std::get<std::int64_t>(rows.rows[4][0]),3);
        EXPECT_EQ(std::get<std::int64_t>(rows.rows[5][0]),4);
        EXPECT_EQ(std::get<std::int64_t>(rows.rows[6][0]),5);
    }
}

TEST(MapHandlerTest, ShopAdmissionRestoresWireAndCommitsExpiryAtomically) {
    for (int failure : {1,2,0}) {
        SCOPED_TRACE(failure);
        mxh::db::SqliteAdapter db; mxh::db::ConnectionConfig cfg; cfg.path=":memory:";
        ASSERT_TRUE(db.connect(cfg).ok()); ASSERT_TRUE(mxh::db::migrate_modern_schema(db).ok());
        ASSERT_TRUE(db.exec_multi("INSERT INTO character_info(chrid,userid,charname,map_num) VALUES(777,'123','RestoreHero',10);"
            "INSERT INTO modern_player_item VALUES(777,2,0,9002,55002,1,0,65535,0);"
            "INSERT INTO modern_player_item VALUES(777,2,1,9003,63130,1,0,65535,0);"
            "INSERT INTO modern_player_item VALUES(777,2,2,9004,57680,1,0,65535,0);").ok());
        mxh::db::LegacyShopAppearanceRows saved;
        saved.skin={101,102,103,104,105};
        saved.used_items.push_back({55001,0,9001,2,0,60000});
        saved.used_items.push_back({55002,390,9002,1,0,0});
        saved.used_items.push_back({57680,392,9004,10,0,0xffffffffu});
        saved.used_items.push_back({63130,391,9003,10,0,0xffffffffu});
        const auto initial=mxh::db::load_modern_shop_state(db,777,123); ASSERT_TRUE(initial);
        ASSERT_TRUE(mxh::db::save_modern_shop_state(db,777,123,*initial,saved));
        ReplySpy reply; mxh::server::MapHandler handler(db,10,make_reply_spy(reply));
        handler.set_allow_dev_gamein_fallback(false);
        const auto root=std::filesystem::path(MXH_SOURCE_DIR)/"data/PlayDH/Resource";
        std::uint64_t shop_now=1000;
        ASSERT_TRUE(handler.set_movement_clock_for_test([&]{return shop_now;}));
        std::string error;
        ASSERT_TRUE(handler.load_shop_event_rates(root/"Server/DropRate.bin",mxh::server::ShopLocale::China,error)) << error;
        ASSERT_TRUE(handler.load_shop_dup_catalog(root/"ItemdupOption.bin",error)) << error;
        ASSERT_TRUE(handler.load_avatar_equip_catalog(root/"AvatarEquip.bin",error)) << error;
        mxh::game::ItemInfo buff{}; buff.ItemIdx=55001; buff.ItemKind=258; buff.Plus_MugongIdx=100; buff.SellPrice=2;
        handler.add_item_info_for_test(buff);
        mxh::game::ItemInfo expired{}; expired.ItemIdx=55002; expired.ItemType=11; expired.SellPrice=1;
        handler.add_item_info_for_test(expired);
        mxh::game::ItemInfo dress{}; dress.ItemIdx=63130; dress.ItemType=11; dress.SellPrice=1; dress.Life=55;
        handler.add_item_info_for_test(dress);
        auto previous_dress=dress; previous_dress.ItemIdx=57680; previous_dress.Life=77;
        handler.add_item_info_for_test(previous_dress);
        if (failure == 1) ASSERT_TRUE(db.exec_multi("CREATE TRIGGER fail_shop_record BEFORE UPDATE OF character_data ON character_info "
             "BEGIN SELECT RAISE(ABORT,'injected shop state failure'); END;").ok());
        if (failure == 2) ASSERT_TRUE(db.exec_multi("DROP TABLE modern_character_equipment;").ok());
        mxh::net::Message enter; enter.header.object_id=777;
        enter.header.category=static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
        enter.header.protocol=static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
        enter.payload.assign(16,0); enter.payload[0]=123;
        testing::internal::CaptureStdout();
        handler.on_message({55},enter);
        const auto logged=testing::internal::GetCapturedStdout();
        const auto restored=mxh::db::load_modern_shop_state(db,777,123); ASSERT_TRUE(restored);
        mxh::db::ResultSet inventory; ASSERT_TRUE(db.query("SELECT db_idx FROM modern_player_item WHERE player_id=777",{},inventory).ok());
        if (failure != 0) {
            EXPECT_FALSE(handler.player_runtime_snapshot(777));
            ASSERT_FALSE(reply.messages.empty());
            EXPECT_EQ(reply.last_message.header.protocol,static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInNack));
            EXPECT_EQ(restored->rows.used_items.size(),4u); EXPECT_EQ(inventory.rows.size(),3u);
            EXPECT_EQ(restored->rows.used_items[2].parameter,10u); // replacement update rolled back too
            EXPECT_EQ(logged.find("ShopItemUseEnd"),std::string::npos);
        } else {
            EXPECT_TRUE(handler.player_runtime_snapshot(777));
            ASSERT_EQ(restored->rows.used_items.size(),3u); EXPECT_EQ(restored->rows.used_items[0].item_id,55001u);
            EXPECT_EQ(restored->rows.used_items[1].item_id,57680u);
            EXPECT_EQ(restored->rows.used_items[1].parameter,1u); // source SellPrice after replacement
            EXPECT_EQ(restored->rows.used_items[2].parameter,10u);
            EXPECT_EQ(inventory.rows.size(),2u); EXPECT_NE(logged.find("ShopItemUseEnd player=777 db_idx=9002"),std::string::npos);
            const auto ack=std::find_if(reply.messages.begin(),reply.messages.end(),[](const auto& message) {
                return message.header.protocol==static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInAck);
            });
            ASSERT_NE(ack,reply.messages.end());
            mxh::game::ShopItemOption options{};
            std::memcpy(&options,ack->payload.data()+mxh::game::HERO_TOTAL_SHOP_OPTION_OFFSET,sizeof(options));
            EXPECT_EQ(options.Life,100); EXPECT_EQ(options.wSkinItem,saved.skin);
            EXPECT_EQ(options.Avatar[6],63130);
            EXPECT_EQ(options.Avatar[15],0); EXPECT_EQ(options.Avatar[16],0);
            const auto actor=handler.player_runtime_snapshot(777); ASSERT_TRUE(actor);
            const auto wire_u32=[&](std::size_t at) { std::uint32_t value; std::memcpy(&value,ack->payload.data()+at,4); return value; };
            EXPECT_EQ(wire_u32(35),actor->current_hp); EXPECT_EQ(wire_u32(39),actor->max_hp);
            EXPECT_EQ(wire_u32(155),actor->current_mp); EXPECT_EQ(wire_u32(159),actor->max_mp);
            EXPECT_EQ(actor->max_hp,actor->current_hp+155u); // charm 100 + worn dress 55; no healing
            mxh::game::ItemTotalInfo items{};
            std::memcpy(&items,ack->payload.data()+mxh::game::HERO_TOTAL_ITEM_OFFSET,sizeof(items));
            EXPECT_EQ(items.ShopInventory[0].dwDBIdx,0u);
            EXPECT_EQ(items.ShopInventory[1].dwDBIdx,9003u);
            EXPECT_EQ(items.ShopInventory[2].dwDBIdx,9004u); // replacing clothing does not discard it
            auto leave=enter;
            leave.header.protocol=static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameOutSyn);
            shop_now+=45000; // source logout bounds this final slice to 30 seconds
            ASSERT_TRUE(db.exec_multi("CREATE TRIGGER fail_logout_shop BEFORE UPDATE OF character_data ON character_info BEGIN SELECT RAISE(ABORT,'logout shop failure'); END;").ok());
            handler.on_message({55},leave);
            ASSERT_TRUE(handler.player_runtime_snapshot(777));
            EXPECT_EQ(reply.last_message.header.protocol,static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameOutNack));
            const auto failed_exit=mxh::db::load_modern_shop_state(db,777,123); ASSERT_TRUE(failed_exit);
            EXPECT_EQ(failed_exit->rows.used_items[0].remaining_time,60000u);
            ASSERT_TRUE(db.exec_multi("DROP TRIGGER fail_logout_shop;").ok());
            handler.on_message({55},leave);
            ASSERT_FALSE(handler.player_runtime_snapshot(777));
            reply.messages.clear();
            handler.on_message({55},enter);
            const auto rejoined=handler.player_runtime_snapshot(777); ASSERT_TRUE(rejoined);
            EXPECT_EQ(rejoined->max_hp,actor->max_hp);
            const auto next_ack=std::find_if(reply.messages.begin(),reply.messages.end(),[](const auto& packet) {
                return packet.header.protocol==static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInAck);
            });
            ASSERT_NE(next_ack,reply.messages.end());
            mxh::game::ShopItemOption next_options{};
            std::memcpy(&next_options,next_ack->payload.data()+mxh::game::HERO_TOTAL_SHOP_OPTION_OFFSET,sizeof(next_options));
            EXPECT_EQ(next_options.Avatar,options.Avatar);
            const auto next_saved=mxh::db::load_modern_shop_state(db,777,123); ASSERT_TRUE(next_saved);
            ASSERT_EQ(next_saved->rows.used_items.size(),3u);
            EXPECT_EQ(next_saved->rows.used_items[0].remaining_time,30000u);
            EXPECT_EQ(next_saved->rows.used_items[1].parameter,1u);
            EXPECT_EQ(next_saved->rows.used_items[2].parameter,10u);
        }
    }
}

TEST(MapHandlerTest, AdmissionRestoresPersistedZeroLifeAsDeadWithoutShopItems) {
    mxh::db::SqliteAdapter db; mxh::db::ConnectionConfig cfg; cfg.path=":memory:";
    ASSERT_TRUE(db.connect(cfg).ok()); ASSERT_TRUE(mxh::db::migrate_modern_schema(db).ok());
    ASSERT_TRUE(db.exec_multi("INSERT INTO character_info(chrid,userid,charname,map_num) VALUES(777,'123','DeadHero',10);").ok());
    const auto initial=mxh::db::load_modern_shop_state(db,777,123); ASSERT_TRUE(initial);
    const std::vector<mxh::db::PersistedPet> pets{{901,1,2,1234,57,1,0,0}};
    ASSERT_TRUE(mxh::db::save_modern_shop_state(db,777,123,*initial,{},mxh::db::PersistedVitals{0,0,7},pets));
    ReplySpy reply; mxh::server::MapHandler handler(db,10,make_reply_spy(reply));
    handler.set_allow_dev_gamein_fallback(false);
    mxh::net::Message enter; enter.header.object_id=777;
    enter.header.category=static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    enter.header.protocol=static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    enter.payload.assign(16,0); enter.payload[0]=123;
    handler.on_message({55},enter);
    const auto player=handler.player_runtime_snapshot(777); ASSERT_TRUE(player);
    EXPECT_EQ(player->current_hp,0u);
    EXPECT_EQ(player->current_mp,7u);
    EXPECT_EQ(player->pet_count,1u);
    EXPECT_EQ(player->lifecycle,mxh::server::PlayerLifecycle::Dead);
    ASSERT_TRUE(handler.set_player_vitals_for_test(777,0,5));
    handler.on_disconnect({55},mxh::net::NetError::Disconnected);
    EXPECT_FALSE(handler.player_runtime_snapshot(777));
    const auto saved=mxh::db::load_modern_shop_state(db,777,123); ASSERT_TRUE(saved);
    ASSERT_TRUE(saved->vitals);
    EXPECT_EQ(saved->vitals->life,0u);
    EXPECT_EQ(saved->vitals->naeryuk,5u);
    ASSERT_TRUE(saved->pets); ASSERT_EQ(saved->pets->size(),1u);
    EXPECT_EQ((*saved->pets)[0].summon_item,901u);
    EXPECT_EQ((*saved->pets)[0].friendship,57u);
    EXPECT_EQ((*saved->pets)[0].stamina,1234u);
    EXPECT_EQ((*saved->pets)[0].grade,2u);
    handler.on_message({56},enter);
    const auto reentered=handler.player_runtime_snapshot(777); ASSERT_TRUE(reentered);
    EXPECT_EQ(reentered->current_hp,0u);
    EXPECT_EQ(reentered->current_mp,5u);
    EXPECT_EQ(reentered->pet_count,1u);
    EXPECT_EQ(reentered->lifecycle,mxh::server::PlayerLifecycle::Dead);
}

TEST(MapHandlerTest, MapKindsAreProfileBoundAndFailedReloadClearsOldRules) {
    mxh::db::SqliteAdapter db;
    ReplySpy reply; mxh::server::MapHandler handler(db,10,make_reply_spy(reply));
    const auto source=std::filesystem::path(MXH_SOURCE_DIR)/"data/PlayDH/Resource/MapKindInfo.bin";
    EXPECT_FALSE(handler.resolved_map_kind(10));
    ASSERT_TRUE(handler.load_map_kinds(source,"playdh-current"));
    EXPECT_EQ(handler.resolved_map_kind(10),64u);
    EXPECT_FALSE(handler.resolved_map_kind(118));
    EXPECT_FALSE(handler.resolved_map_kind(99999));
    EXPECT_FALSE(handler.load_map_kinds(source,"sworking-2008-reference"));
    EXPECT_FALSE(handler.resolved_map_kind(10));
    ASSERT_TRUE(handler.load_map_kinds(source,"playdh-current"));
    EXPECT_FALSE(handler.load_map_kinds(source,"unknown"));
    EXPECT_FALSE(handler.resolved_map_kind(10));
}

TEST(MapHandlerTest, CompletePresentReviveCandidateCommitsActorAndPetTogether) {
    using namespace mxh::server;
    mxh::db::SqliteAdapter db; mxh::db::ConnectionConfig config; config.path=":memory:";
    ASSERT_TRUE(db.connect(config).ok());
    ASSERT_TRUE(db.exec_multi("CREATE TABLE character_info(chrid INT PRIMARY KEY,userid TEXT,level INT,character_data BLOB);"
        "CREATE TABLE modern_player_state(player_id INT PRIMARY KEY,level INT,exp INT,money INT);"
        "INSERT INTO character_info VALUES(777,'123',5,NULL);"
        "INSERT INTO modern_player_state VALUES(777,5,100,100);").ok());
    Player actor; PlayerSpawnInfo spawn;
    spawn.player_id=777; spawn.user_id=123; spawn.level=5; spawn.map_num=10;
    spawn.base.level=5; spawn.base.cheryuk=20; spawn.base.simmek=10;
    ASSERT_TRUE(actor.initialize(spawn)); ASSERT_TRUE(actor.activate());
    actor.state().progress.money=100; actor.state().progress.level_exp=100;
    actor.state().vitals.current_hp=0; ASSERT_TRUE(actor.mark_dead_if_zero_life());
    std::string text;
    for(unsigned level=1;level<=121;++level) text+=std::to_string(level)+" 1000\n";
    const auto curve=mxh::game::ExperienceCurve::load_from_text(text);
    PetManagerState pets; PetTotalInfo pet{};
    pet.PetSummonItemDBIdx=901; pet.PetKind=1; pet.PetGrade=1;
    pet.PetFriendly=10; pet.bAlive=1; pets.m_PetInfoList.push_back(pet);
    pets.m_curSummonItemDBIdx=901; pets.m_iFriendshipReduceAmount=-20;
    mxh::game::MapKindTable maps;
    maps[10]=64;
    LootingManagerState looting;
    create_looting_room(looting,make_looting_room(777,999,0,0));
    EXPECT_FALSE(prepare_resolved_present_revive(actor,curve,{}, {},{},pets,looting,maps,
        false,false,false));
    ASSERT_TRUE(close_looting_room(looting,777));
    const auto candidate=prepare_resolved_present_revive(actor,curve,{}, {},{},pets,looting,maps,
        false,false,false);
    ASSERT_TRUE(candidate);
    const auto snapshot=mxh::db::load_modern_shop_state(db,777,123); ASSERT_TRUE(snapshot);
    for(unsigned invalid=0;invalid<3;++invalid) {
        auto malformed=*candidate;
        if(invalid==0) malformed.actor.state().pos_x=-1;
        if(invalid==1) malformed.actor.state().map_num=2;
        if(invalid==2) malformed.actor.state().vitals.current_hp=0xffffffffu;
        EXPECT_EQ(commit_present_revive(db,*snapshot,actor,malformed),mxh::db::ReviveCommit::Rejected);
        mxh::db::ResultSet unchanged;
        ASSERT_TRUE(db.query("SELECT money,exp FROM modern_player_state WHERE player_id=777",{},unchanged).ok());
        ASSERT_EQ(unchanged.rows.size(),1u);
        EXPECT_EQ(std::get<std::int64_t>(unchanged.rows[0][0]),100);
        EXPECT_EQ(std::get<std::int64_t>(unchanged.rows[0][1]),100);
        const auto untouched=mxh::db::load_modern_shop_state(db,777,123);
        ASSERT_TRUE(untouched); EXPECT_FALSE(untouched->vitals); EXPECT_FALSE(untouched->pets);
    }
    EXPECT_EQ(commit_present_revive(db,*snapshot,actor,*candidate),mxh::db::ReviveCommit::Committed);
    const auto saved=mxh::db::load_modern_shop_state(db,777,123); ASSERT_TRUE(saved);
    ASSERT_TRUE(saved->vitals); EXPECT_EQ(saved->vitals->life,candidate->actor.state().vitals.current_hp);
    ASSERT_TRUE(saved->pets); ASSERT_EQ(saved->pets->size(),1u);
    EXPECT_EQ((*saved->pets)[0].alive,0u); EXPECT_EQ((*saved->pets)[0].friendship,0u);
    mxh::db::ResultSet result;
    ASSERT_TRUE(db.query("SELECT money,exp FROM modern_player_state WHERE player_id=777",{},result).ok());
    ASSERT_EQ(result.rows.size(),1u);
    EXPECT_EQ(std::get<std::int64_t>(result.rows[0][0]),94);
    EXPECT_EQ(std::get<std::int64_t>(result.rows[0][1]),70);
    EXPECT_EQ(actor.lifecycle(),PlayerLifecycle::Dead);
    EXPECT_EQ(pets.m_PetInfoList[0].PetFriendly,10u);
    EXPECT_EQ(commit_present_revive(db,*snapshot,actor,*candidate),mxh::db::ReviveCommit::Rejected);
}

TEST(MapHandlerTest, PresentSpotRequestAppliesPenaltyAndRestoresOnRelogin) {
    const auto root = std::filesystem::path(MXH_SOURCE_DIR) / "data/PlayDH/Resource";
    const auto curve = mxh::game::ExperienceCurve::load_from_bin(root / "CharacterExpPoint.bin");
    std::ifstream penalty_file(root / "Server/ExpPenalty.bin", std::ios::binary);
    const std::vector<std::uint8_t> penalty_bytes{
        std::istreambuf_iterator<char>(penalty_file), std::istreambuf_iterator<char>()};
    const auto penalties = mxh::game::decode_exp_penalty(
        penalty_bytes, mxh::game::ExpPenaltyProfile::PlayDhCurrent);
    ASSERT_TRUE(penalties);
    const auto threshold = curve.max_exp_point(5);
    ASSERT_GT(threshold, 1u);
    ASSERT_LE(threshold, 0xffffffffu);
    const auto seeded_exp = static_cast<std::uint32_t>(threshold - 1);
    const auto loss = mxh::game::unprotected_revive_loss(
        *penalties, mxh::game::ReviveLocation::Present, 5, 100000u, threshold);
    ASSERT_TRUE(loss);
    ASSERT_GT(loss->money, 0u);
    ASSERT_LT(loss->experience, seeded_exp);
    const auto reduced = curve.reduce_exp({5, seeded_exp}, loss->experience);
    EXPECT_EQ(reduced.level, 5u);

    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig config;
    config.path = ":memory:";
    ASSERT_TRUE(db.connect(config).ok());
    ASSERT_TRUE(mxh::db::migrate_modern_schema(db).ok());
    ASSERT_TRUE(db.exec_multi(
        "INSERT INTO character_info(charname,chrid,userid,level,map_num) "
        "VALUES('DeadHero',777,'123',5,10);"
        "INSERT INTO modern_player_state(player_id,money,level,exp,updated_at) "
        "VALUES(777,100000,5," + std::to_string(seeded_exp) + ",'now');").ok());
    const auto initial = mxh::db::load_modern_shop_state(db, 777, 123);
    ASSERT_TRUE(initial);
    ASSERT_TRUE(mxh::db::save_modern_shop_state(
        db, 777, 123, *initial, {}, mxh::db::PersistedVitals{0, 0, 40}));

    auto enter = [] {
        mxh::net::Message message;
        message.header.object_id = 777;
        message.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
        message.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
        message.payload.assign(16, 0);
        message.payload[0] = 123;
        return message;
    };
    auto revive = [](std::uint8_t protocol, std::uint32_t player) {
        mxh::net::Message message;
        message.header.category = static_cast<std::uint8_t>(mxh::proto::Category::CharRevive);
        message.header.protocol = protocol;
        message.header.object_id = player;
        return message;
    };
    {
        ReplySpy reply;
        mxh::server::MapHandler event_map(db, 58, make_reply_spy(reply));
        event_map.set_allow_dev_gamein_fallback(false);
        event_map.on_message({55}, enter());
        const auto dead = event_map.player_runtime_snapshot(777);
        ASSERT_TRUE(dead);
        EXPECT_EQ(dead->lifecycle, mxh::server::PlayerLifecycle::Dead);
        EXPECT_EQ(dead->map_num, 58u);
        reply.messages.clear();
        event_map.on_message({55}, revive(0, 777));
        EXPECT_TRUE(reply.messages.empty());
        EXPECT_EQ(event_map.player_runtime_snapshot(777)->lifecycle, mxh::server::PlayerLifecycle::Dead);
        event_map.on_disconnect({55}, mxh::net::NetError::Disconnected);
        EXPECT_FALSE(event_map.is_draining());
        EXPECT_FALSE(event_map.player_runtime_snapshot(777));
    }
    // The event-map exit records that map. Map 10 has no transfer route from it,
    // so drop the saved point before the ordinary-map admission.
    ASSERT_TRUE(db.exec_multi("DELETE FROM modern_player_position WHERE player_id=777;").ok());

    ReplySpy reply;
    mxh::server::MapHandler handler(db, 10, make_reply_spy(reply));
    handler.set_allow_dev_gamein_fallback(false);
    handler.load_experience_curve((root / "CharacterExpPoint.bin").string());
    ASSERT_TRUE(handler.has_loaded_experience_curve());
    ASSERT_TRUE(handler.load_exp_penalty(root / "Server/ExpPenalty.bin", "playdh-current"));
    ASSERT_TRUE(handler.load_map_kinds(root / "MapKindInfo.bin", "playdh-current"));
    handler.on_message({55}, enter());
    const auto before = handler.player_runtime_snapshot(777);
    ASSERT_TRUE(before);
    EXPECT_EQ(before->lifecycle, mxh::server::PlayerLifecycle::Dead);
    EXPECT_EQ(before->level, 5u);
    EXPECT_EQ(before->level_exp, seeded_exp);
    EXPECT_EQ(handler.player_money_for_test(777), 100000u);
    EXPECT_EQ(before->current_hp, 0u);
    EXPECT_GT(before->current_mp, 0u);
    EXPECT_FLOAT_EQ(before->pos_x, 25000.0f);
    EXPECT_FLOAT_EQ(before->pos_z, 25000.0f);

    reply.messages.clear();
    auto rejected = revive(0, 777);
    rejected.payload = {0};
    handler.on_message({55}, rejected);
    handler.on_message({55}, revive(3, 777));
    handler.on_message({55}, revive(6, 777));
    handler.on_message({99}, revive(0, 777));
    handler.on_message({55}, revive(0, 0));
    EXPECT_TRUE(reply.messages.empty());
    EXPECT_EQ(handler.player_runtime_snapshot(777)->lifecycle, mxh::server::PlayerLifecycle::Dead);
    EXPECT_EQ(handler.player_money_for_test(777), 100000u);

    reply.messages.clear();
    handler.on_message({55}, revive(0, 777));
    const auto after = handler.player_runtime_snapshot(777);
    ASSERT_TRUE(after);
    EXPECT_EQ(after->lifecycle, mxh::server::PlayerLifecycle::Active);
    EXPECT_EQ(after->level, reduced.level);
    EXPECT_EQ(after->level_exp, reduced.exp_point);
    EXPECT_EQ(handler.player_money_for_test(777), 100000u - loss->money);
    EXPECT_EQ(after->current_hp, static_cast<std::uint32_t>(after->max_hp * 0.3));
    EXPECT_GT(after->current_hp, 0u);
    EXPECT_EQ(after->current_mp, 0u);
    EXPECT_FALSE(handler.is_draining());

    const auto find_message = [&](std::uint8_t category, std::uint8_t protocol) {
        return std::find_if(reply.messages.begin(), reply.messages.end(), [&](const auto& message) {
            return message.header.category == category && message.header.protocol == protocol;
        });
    };
    const auto position_message = find_message(
        static_cast<std::uint8_t>(mxh::proto::Category::UserConn), 44);
    const auto money_message = find_message(
        static_cast<std::uint8_t>(mxh::proto::Category::Item),
        static_cast<std::uint8_t>(mxh::proto::ItemProtocol::Money));
    const auto experience_message = find_message(3, 13);
    const auto life_message = find_message(
        static_cast<std::uint8_t>(mxh::proto::Category::Character), 1);
    const auto mp_message = find_message(
        static_cast<std::uint8_t>(mxh::proto::Category::Character), 9);
    ASSERT_NE(position_message, reply.messages.end());
    ASSERT_NE(money_message, reply.messages.end());
    ASSERT_NE(life_message, reply.messages.end());
    ASSERT_NE(mp_message, reply.messages.end());
    EXPECT_LT(position_message, money_message);
    EXPECT_LT(money_message, life_message);
    EXPECT_LT(life_message, mp_message);
    EXPECT_EQ(position_message->header.object_id, 777u);
    const auto position = mxh::proto::decode_character_revive(777, position_message->payload);
    ASSERT_TRUE(position);
    EXPECT_EQ(position->x, 25000u);
    EXPECT_EQ(position->z, 25000u);
    ASSERT_EQ(money_message->payload.size(), 4u);
    std::uint32_t money = 0;
    std::memcpy(&money, money_message->payload.data(), 4);
    EXPECT_EQ(money, 100000u - loss->money);
    if (loss->experience == 0) {
        EXPECT_EQ(experience_message, reply.messages.end());
    } else {
        ASSERT_NE(experience_message, reply.messages.end());
        EXPECT_LT(money_message, experience_message);
        EXPECT_LT(experience_message, life_message);
        ASSERT_EQ(experience_message->payload.size(), 9u);
        std::int64_t experience = 0;
        std::memcpy(&experience, experience_message->payload.data(), 8);
        EXPECT_EQ(experience, static_cast<std::int64_t>(reduced.exp_point));
        EXPECT_EQ(experience_message->payload[8], 1u);
    }
    std::int32_t life_delta = 0;
    ASSERT_EQ(life_message->payload.size(), 4u);
    std::memcpy(&life_delta, life_message->payload.data(), 4);
    EXPECT_EQ(life_delta, static_cast<std::int32_t>(after->current_hp));
    std::int32_t mp_delta = 0;
    ASSERT_EQ(mp_message->payload.size(), 4u);
    std::memcpy(&mp_delta, mp_message->payload.data(), 4);
    EXPECT_EQ(mp_delta, -static_cast<std::int32_t>(before->current_mp));

    mxh::db::ResultSet saved;
    ASSERT_TRUE(db.query("SELECT level,exp,money FROM modern_player_state WHERE player_id=777", {}, saved).ok());
    ASSERT_EQ(saved.rows.size(), 1u);
    EXPECT_EQ(std::get<std::int64_t>(saved.rows[0][0]), static_cast<std::int64_t>(reduced.level));
    EXPECT_EQ(std::get<std::int64_t>(saved.rows[0][1]), static_cast<std::int64_t>(reduced.exp_point));
    EXPECT_EQ(std::get<std::int64_t>(saved.rows[0][2]), static_cast<std::int64_t>(100000u - loss->money));
    const auto persisted = mxh::db::load_modern_shop_state(db, 777, 123);
    ASSERT_TRUE(persisted);
    ASSERT_TRUE(persisted->vitals);
    EXPECT_EQ(persisted->vitals->life, after->current_hp);
    EXPECT_EQ(persisted->vitals->naeryuk, 0u);

    const auto count = reply.messages.size();
    handler.on_message({55}, revive(0, 777));
    ASSERT_EQ(reply.messages.size(), count + 1);
    EXPECT_EQ(reply.messages.back().header.protocol, mxh::server::userconn_character_revive_nack);
    EXPECT_EQ(reply.messages.back().payload, (std::vector<std::uint8_t>{1}));
    EXPECT_EQ(handler.player_runtime_snapshot(777)->current_hp, after->current_hp);

    handler.on_disconnect({55}, mxh::net::NetError::Disconnected);
    EXPECT_FALSE(handler.is_draining());
    mxh::server::MapHandler restored(db, 10, make_reply_spy(reply));
    restored.set_allow_dev_gamein_fallback(false);
    restored.on_message({56}, enter());
    const auto relogin = restored.player_runtime_snapshot(777);
    ASSERT_TRUE(relogin);
    EXPECT_EQ(relogin->lifecycle, mxh::server::PlayerLifecycle::Active);
    EXPECT_EQ(relogin->current_hp, after->current_hp);
    EXPECT_EQ(relogin->current_mp, 0u);
    EXPECT_EQ(relogin->level_exp, after->level_exp);
    EXPECT_EQ(restored.player_money_for_test(777), 100000u - loss->money);
}

void check_login_point_request(const std::filesystem::path& current_path,
                               const std::filesystem::path& legacy_path) {
    const auto root = std::filesystem::path(MXH_SOURCE_DIR) / "data/PlayDH/Resource";
    auto read_bytes = [](const std::filesystem::path& path) {
        std::ifstream input(path, std::ios::binary);
        return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(input),
                                         std::istreambuf_iterator<char>());
    };
    const auto current_points = mxh::game::decode_login_points(
        read_bytes(current_path), mxh::game::LoginPointProfile::PlayDhCurrent);
    const auto legacy_points = mxh::game::decode_login_points(
        read_bytes(legacy_path), mxh::game::LoginPointProfile::LegacyMhFile);
    ASSERT_TRUE(current_points);
    ASSERT_TRUE(legacy_points);
    const auto current_ten = mxh::game::login_revive_point(*current_points, 10);
    const auto legacy_ten = mxh::game::login_revive_point(*legacy_points, 10);
    const auto current_twelve = mxh::game::login_revive_point(*current_points, 12);
    const auto legacy_twelve = mxh::game::login_revive_point(*legacy_points, 12);
    ASSERT_TRUE(current_ten);
    ASSERT_TRUE(legacy_ten);
    ASSERT_TRUE(current_twelve);
    ASSERT_TRUE(legacy_twelve);
    EXPECT_FLOAT_EQ(current_ten->x, legacy_ten->x);
    EXPECT_FLOAT_EQ(current_ten->z, legacy_ten->z);
    EXPECT_FLOAT_EQ(current_twelve->x, legacy_twelve->x);
    EXPECT_FLOAT_EQ(current_twelve->z, legacy_twelve->z);
    EXPECT_NE(current_ten->x, 25000.0f);
    EXPECT_LT(current_ten->x, 65536.0f);
    EXPECT_LT(current_ten->z, 65536.0f);

    const auto curve = mxh::game::ExperienceCurve::load_from_bin(root / "CharacterExpPoint.bin");
    const auto penalties = mxh::game::decode_exp_penalty(
        read_bytes(root / "Server/ExpPenalty.bin"), mxh::game::ExpPenaltyProfile::PlayDhCurrent);
    ASSERT_TRUE(penalties);
    const auto threshold = curve.max_exp_point(5);
    ASSERT_GT(threshold, 1u);
    ASSERT_LE(threshold, 0xffffffffu);
    const auto seeded_exp = static_cast<std::uint32_t>(threshold - 1);
    const auto loss = mxh::game::unprotected_revive_loss(
        *penalties, mxh::game::ReviveLocation::Login, 5, 100000u, threshold);
    ASSERT_TRUE(loss);
    EXPECT_EQ(loss->money, static_cast<std::uint32_t>(100000.0 * 0.04));
    ASSERT_LT(loss->experience, seeded_exp);
    const auto reduced = curve.reduce_exp({5, seeded_exp}, loss->experience);

    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig config;
    config.path = ":memory:";
    ASSERT_TRUE(db.connect(config).ok());
    ASSERT_TRUE(mxh::db::migrate_modern_schema(db).ok());
    ASSERT_TRUE(db.exec_multi(
        "INSERT INTO character_info(charname,chrid,userid,level,map_num) "
        "VALUES('DeadHero',777,'123',5,10);"
        "INSERT INTO modern_player_state(player_id,money,level,exp,updated_at) "
        "VALUES(777,100000,5," + std::to_string(seeded_exp) + ",'now');").ok());
    const auto initial = mxh::db::load_modern_shop_state(db, 777, 123);
    ASSERT_TRUE(initial);
    ASSERT_TRUE(mxh::db::save_modern_shop_state(
        db, 777, 123, *initial, {}, mxh::db::PersistedVitals{0, 0, 40}));
    auto enter = [] {
        mxh::net::Message message;
        message.header.object_id = 777;
        message.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
        message.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
        message.payload.assign(16, 0);
        message.payload[0] = 123;
        return message;
    };
    auto revive = [](std::uint8_t protocol) {
        mxh::net::Message message;
        message.header.category = static_cast<std::uint8_t>(mxh::proto::Category::CharRevive);
        message.header.protocol = protocol;
        message.header.object_id = 777;
        return message;
    };
    {
        ReplySpy reply;
        mxh::server::MapHandler event_map(db, 58, make_reply_spy(reply));
        event_map.set_allow_dev_gamein_fallback(false);
        ASSERT_TRUE(event_map.load_login_points(current_path, "playdh-current"));
        event_map.on_message({55}, enter());
        ASSERT_EQ(event_map.player_runtime_snapshot(777)->lifecycle, mxh::server::PlayerLifecycle::Dead);
        reply.messages.clear();
        event_map.on_message({55}, revive(3));
        EXPECT_TRUE(reply.messages.empty());
        EXPECT_EQ(event_map.player_money_for_test(777), 100000u);
        EXPECT_EQ(event_map.player_runtime_snapshot(777)->level_exp, seeded_exp);
        EXPECT_EQ(event_map.player_runtime_snapshot(777)->lifecycle, mxh::server::PlayerLifecycle::Dead);
        event_map.on_disconnect({55}, mxh::net::NetError::Disconnected);
        EXPECT_FALSE(event_map.is_draining());
    }
    ASSERT_TRUE(db.exec_multi("DELETE FROM modern_player_position WHERE player_id=777;").ok());

    ReplySpy reply;
    mxh::server::MapHandler handler(db, 10, make_reply_spy(reply));
    handler.set_allow_dev_gamein_fallback(false);
    handler.load_experience_curve((root / "CharacterExpPoint.bin").string());
    ASSERT_TRUE(handler.load_exp_penalty(root / "Server/ExpPenalty.bin", "playdh-current"));
    ASSERT_TRUE(handler.load_map_kinds(root / "MapKindInfo.bin", "playdh-current"));
    ASSERT_TRUE(handler.load_login_points(current_path, "playdh-current"));
    handler.on_message({55}, enter());
    const auto before = handler.player_runtime_snapshot(777);
    ASSERT_TRUE(before);
    EXPECT_EQ(before->lifecycle, mxh::server::PlayerLifecycle::Dead);
    EXPECT_FLOAT_EQ(before->pos_x, 25000.0f);
    reply.messages.clear();
    auto malformed = revive(3);
    malformed.payload = {0};
    handler.on_message({55}, malformed);
    handler.on_message({55}, revive(6));
    handler.on_message({99}, revive(3));
    EXPECT_TRUE(reply.messages.empty());
    EXPECT_EQ(handler.player_money_for_test(777), 100000u);
    EXPECT_EQ(handler.player_runtime_snapshot(777)->level_exp, seeded_exp);
    EXPECT_EQ(handler.player_runtime_snapshot(777)->lifecycle, mxh::server::PlayerLifecycle::Dead);

    reply.messages.clear();
    handler.on_message({55}, revive(3));
    const auto after = handler.player_runtime_snapshot(777);
    ASSERT_TRUE(after);
    EXPECT_EQ(after->lifecycle, mxh::server::PlayerLifecycle::Active);
    EXPECT_EQ(handler.player_money_for_test(777), 100000u - loss->money);
    EXPECT_EQ(after->level_exp, reduced.exp_point);
    EXPECT_EQ(after->current_hp, static_cast<std::uint32_t>(after->max_hp * 0.3));
    EXPECT_EQ(after->current_shield, static_cast<std::uint32_t>(after->max_shield * 0.3));
    EXPECT_EQ(after->current_mp, 0u);
    EXPECT_FLOAT_EQ(after->pos_x, current_ten->x);
    EXPECT_FLOAT_EQ(after->pos_z, current_ten->z);
    const auto position = std::find_if(reply.messages.begin(), reply.messages.end(), [](const auto& message) {
        return message.header.protocol == 44;
    });
    ASSERT_NE(position, reply.messages.end());
    const auto decoded = mxh::proto::decode_character_revive(777, position->payload);
    ASSERT_TRUE(decoded);
    EXPECT_EQ(decoded->x, static_cast<std::uint16_t>(current_ten->x));
    EXPECT_EQ(decoded->z, static_cast<std::uint16_t>(current_ten->z));
    const auto repeat = reply.messages.size();
    handler.on_message({55}, revive(3));
    ASSERT_EQ(reply.messages.size(), repeat + 1);
    EXPECT_EQ(reply.messages.back().header.protocol, mxh::server::userconn_character_revive_nack);
    EXPECT_EQ(handler.player_money_for_test(777), 100000u - loss->money);

    handler.on_disconnect({55}, mxh::net::NetError::Disconnected);
    EXPECT_FALSE(handler.is_draining());
    mxh::server::MapHandler restored(db, 10, make_reply_spy(reply));
    restored.set_allow_dev_gamein_fallback(false);
    restored.on_message({56}, enter());
    const auto relogin = restored.player_runtime_snapshot(777);
    ASSERT_TRUE(relogin);
    EXPECT_EQ(relogin->lifecycle, mxh::server::PlayerLifecycle::Active);
    EXPECT_EQ(relogin->current_hp, after->current_hp);
    EXPECT_EQ(relogin->current_mp, 0u);
    EXPECT_EQ(relogin->level_exp, after->level_exp);
    EXPECT_EQ(restored.player_money_for_test(777), 100000u - loss->money);
    EXPECT_FLOAT_EQ(relogin->pos_x, current_ten->x);
    EXPECT_FLOAT_EQ(relogin->pos_z, current_ten->z);
}

TEST(MapHandlerTest, LoginPointRequestAppliesPenaltyAndRestoresOnRelogin) {
    namespace fixture = mxh::test::resources;
    const fixture::TemporaryBin current(fixture::current(fixture::login_text, fixture::login_key));
    const fixture::TemporaryBin legacy(fixture::legacy(fixture::login_text));
    check_login_point_request(current.path, legacy.path);
}

TEST(MapHandlerReferenceTest, DISABLED_LoginPointRequestAppliesPenaltyAndRestoresOnRelogin) {
    check_login_point_request(std::filesystem::path(MXH_SOURCE_DIR) / "data/PlayDH/Resource/Server/LoginPoint.bin",
        std::filesystem::path(MXH_SOURCE_DIR).parent_path() /
        "reference/legacy-source/4dddd9a6/SWorking/Resource/Server/LoginPoint.bin");
}

TEST(MapHandlerTest, DeadDisconnectAppliesLoginPenaltyWithoutMoving) {
    const auto root = std::filesystem::path(MXH_SOURCE_DIR) / "data/PlayDH/Resource";
    const auto curve = mxh::game::ExperienceCurve::load_from_bin(root / "CharacterExpPoint.bin");
    std::ifstream penalty_file(root / "Server/ExpPenalty.bin", std::ios::binary);
    const auto penalties = mxh::game::decode_exp_penalty(
        std::vector<std::uint8_t>(std::istreambuf_iterator<char>(penalty_file),
                                  std::istreambuf_iterator<char>()),
        mxh::game::ExpPenaltyProfile::PlayDhCurrent);
    ASSERT_TRUE(penalties);
    const auto threshold = curve.max_exp_point(5);
    ASSERT_GT(threshold, 1u);
    ASSERT_LE(threshold, 0xffffffffu);
    const auto seeded_exp = static_cast<std::uint32_t>(threshold - 1);
    const auto loss = mxh::game::unprotected_revive_loss(
        *penalties, mxh::game::ReviveLocation::Login, 5, 100000u, threshold);
    ASSERT_TRUE(loss);
    EXPECT_EQ(loss->money, static_cast<std::uint32_t>(100000.0 * 0.04));
    const auto reduced = curve.reduce_exp({5, seeded_exp}, loss->experience);

    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig config;
    config.path = ":memory:";
    ASSERT_TRUE(db.connect(config).ok());
    ASSERT_TRUE(mxh::db::migrate_modern_schema(db).ok());
    ASSERT_TRUE(db.exec_multi(
        "INSERT INTO character_info(charname,chrid,userid,level,map_num) "
        "VALUES('DeadHero',777,'123',5,10);"
        "INSERT INTO modern_player_state(player_id,money,level,exp,updated_at) "
        "VALUES(777,100000,5," + std::to_string(seeded_exp) + ",'now');").ok());
    const auto initial = mxh::db::load_modern_shop_state(db, 777, 123);
    ASSERT_TRUE(initial);
    ASSERT_TRUE(mxh::db::save_modern_shop_state(
        db, 777, 123, *initial, {}, mxh::db::PersistedVitals{0, 0, 40}));
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 10, make_reply_spy(reply));
    handler.set_allow_dev_gamein_fallback(false);
    handler.load_experience_curve((root / "CharacterExpPoint.bin").string());
    ASSERT_TRUE(handler.load_exp_penalty(root / "Server/ExpPenalty.bin", "playdh-current"));
    ASSERT_TRUE(handler.load_map_kinds(root / "MapKindInfo.bin", "playdh-current"));
    mxh::net::Message enter;
    enter.header.object_id = 777;
    enter.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    enter.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    enter.payload.assign(16, 0);
    enter.payload[0] = 123;
    handler.on_message({55}, enter);
    const auto before = handler.player_runtime_snapshot(777);
    ASSERT_TRUE(before);
    EXPECT_EQ(before->lifecycle, mxh::server::PlayerLifecycle::Dead);
    EXPECT_FLOAT_EQ(before->pos_x, 25000.0f);
    EXPECT_FLOAT_EQ(before->pos_z, 25000.0f);
    handler.on_disconnect({55}, mxh::net::NetError::Disconnected);
    EXPECT_FALSE(handler.is_draining());
    EXPECT_FALSE(handler.player_runtime_snapshot(777));
    mxh::db::ResultSet saved;
    ASSERT_TRUE(db.query("SELECT level,exp,money FROM modern_player_state WHERE player_id=777", {}, saved).ok());
    ASSERT_EQ(saved.rows.size(), 1u);
    EXPECT_EQ(std::get<std::int64_t>(saved.rows[0][0]), static_cast<std::int64_t>(reduced.level));
    EXPECT_EQ(std::get<std::int64_t>(saved.rows[0][1]), static_cast<std::int64_t>(reduced.exp_point));
    EXPECT_EQ(std::get<std::int64_t>(saved.rows[0][2]), static_cast<std::int64_t>(100000u - loss->money));
    const auto vitals = mxh::db::load_modern_shop_state(db, 777, 123);
    ASSERT_TRUE(vitals);
    ASSERT_TRUE(vitals->vitals);
    EXPECT_GT(vitals->vitals->life, 0u);
    EXPECT_EQ(vitals->vitals->naeryuk, 0u);
    mxh::db::ResultSet position;
    ASSERT_TRUE(db.query("SELECT pos_x,pos_z FROM modern_player_position WHERE player_id=777", {}, position).ok());
    ASSERT_EQ(position.rows.size(), 1u);
    EXPECT_EQ(std::get<std::int64_t>(position.rows[0][0]), 25000);
    EXPECT_EQ(std::get<std::int64_t>(position.rows[0][1]), 25000);

    mxh::server::MapHandler restored(db, 10, make_reply_spy(reply));
    restored.set_allow_dev_gamein_fallback(false);
    restored.on_message({56}, enter);
    const auto relogin = restored.player_runtime_snapshot(777);
    ASSERT_TRUE(relogin);
    EXPECT_EQ(relogin->lifecycle, mxh::server::PlayerLifecycle::Active);
    EXPECT_EQ(relogin->current_hp, vitals->vitals->life);
    EXPECT_EQ(relogin->current_mp, 0u);
    EXPECT_EQ(relogin->level_exp, static_cast<std::uint32_t>(reduced.exp_point));
    EXPECT_EQ(restored.player_money_for_test(777), 100000u - loss->money);
    EXPECT_FLOAT_EQ(relogin->pos_x, 25000.0f);
    EXPECT_FLOAT_EQ(relogin->pos_z, 25000.0f);
    EXPECT_EQ(relogin->current_hp, static_cast<std::uint32_t>(relogin->max_hp * 0.3));
}

TEST(MapHandlerTest, ReviveVitalityMessagesPreserveSignedDeltasAndRejectOverflow) {
    using namespace mxh::server;
    Player actor; PlayerSpawnInfo spawn;
    spawn.player_id=777; spawn.user_id=123; spawn.level=5; spawn.map_num=10;
    spawn.base.level=5; spawn.base.cheryuk=20; spawn.base.simmek=10;
    ASSERT_TRUE(actor.initialize(spawn)); ASSERT_TRUE(actor.activate());
    actor.state().vitals.current_hp=0;
    actor.state().vitals.current_mp=7;
    actor.state().vitals.current_shield=0;
    ASSERT_TRUE(actor.mark_dead_if_zero_life());
    Player revived=actor;
    ASSERT_TRUE(revived.revive());
    revived.state().vitals.current_hp=300;
    revived.state().vitals.current_mp=0;
    revived.state().vitals.current_shield=100;
    revived.state().pos_x=258.9f; revived.state().pos_z=404;
    const auto messages=prepare_revive_vitality_messages(actor,revived);
    ASSERT_TRUE(messages);
    EXPECT_EQ(messages->position.header.protocol,44);
    const auto position=mxh::proto::decode_character_revive(777,messages->position.payload);
    ASSERT_TRUE(position); EXPECT_EQ(position->x,258); EXPECT_EQ(position->z,404);
    ASSERT_EQ(messages->vitality.size(),3u);
    EXPECT_EQ(messages->vitality[0].header.protocol,1);
    EXPECT_EQ(messages->vitality[1].header.protocol,9);
    EXPECT_EQ(messages->vitality[2].header.protocol,5);
    EXPECT_EQ(messages->vitality[1].payload,(std::vector<std::uint8_t>{249,255,255,255}));
    revived.state().vitals.current_mp=7;
    EXPECT_EQ(prepare_revive_vitality_messages(actor,revived)->vitality.size(),2u);
    revived.state().vitals.current_hp=0xffffffffu;
    EXPECT_FALSE(prepare_revive_vitality_messages(actor,revived));
    EXPECT_EQ(actor.lifecycle(),PlayerLifecycle::Dead);
    EXPECT_EQ(actor.state().vitals.current_hp,0u);
}

TEST(MapHandlerTest, SkinCatalogLoadsRealNormalAndCostumeTablesAndRejectsCorruption) {
    const auto root = std::filesystem::path(MXH_SOURCE_DIR) / "data/PlayDH/Resource";
    std::string error;
    const auto catalog = mxh::server::load_skin_catalog(
        root / "SkinSelectItemList.bin", root / "CostumeSkinItemList.bin", error);
    ASSERT_TRUE(catalog) << error;
    EXPECT_FALSE(catalog->normal.empty());
    EXPECT_FALSE(catalog->costume.empty());
    EXPECT_TRUE(std::any_of(catalog->normal.begin(), catalog->normal.end(), [](const auto& row) {
        return std::any_of(row.equip_item.begin(), row.equip_item.end(), [](auto item) { return item != 0; });
    }));
    EXPECT_TRUE(std::any_of(catalog->costume.begin(), catalog->costume.end(), [](const auto& row) {
        return row.equip_item[0] != 0;
    }));

    std::ifstream input(root / "SkinSelectItemList.bin", std::ios::binary);
    ASSERT_TRUE(input);
    std::vector<std::uint8_t> corrupt{
        std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    ASSERT_GT(corrupt.size(), 14u);
    corrupt.back() ^= 0xffu;
    EXPECT_FALSE(mxh::server::parse_skin_catalog_file(corrupt, false, error));
    EXPECT_EQ(error, "skin catalog checksum mismatch");
}

TEST(MapHandlerTest, ShopAdmissionExpiresPhysicalItemsFromOriginalWornContainers) {
    struct Case { int container; int slot; std::uint16_t position; std::uint16_t kind; };
    for (const auto test : std::array{
             Case{1,2,82,mxh::server::LEGACY_SHOP_ITEM_EQUIP},
             Case{3,1,491,mxh::server::LEGACY_SHOP_ITEM_PET_EQUIP},
             Case{5,2,502,mxh::server::LEGACY_SHOP_ITEM_TITAN_EQUIP}}) {
        SCOPED_TRACE(test.container);
        mxh::db::SqliteAdapter db; mxh::db::ConnectionConfig cfg; cfg.path=":memory:";
        ASSERT_TRUE(db.connect(cfg).ok()); ASSERT_TRUE(mxh::db::migrate_modern_schema(db).ok());
        ASSERT_TRUE(db.exec_multi("INSERT INTO character_info(chrid,userid,charname,map_num) VALUES(777,'123','WornRestore',10);"
            "INSERT INTO modern_player_item VALUES(777," + std::to_string(test.container) + "," +
            std::to_string(test.slot) + ",9001,55001,1,0,65535,0);").ok());
        mxh::db::LegacyShopAppearanceRows saved;
        saved.used_items.push_back({55001,390,9001,1,0,0});
        const auto initial=mxh::db::load_modern_shop_state(db,777,123); ASSERT_TRUE(initial);
        ASSERT_TRUE(mxh::db::save_modern_shop_state(db,777,123,*initial,saved));
        ReplySpy reply; mxh::server::MapHandler handler(db,10,make_reply_spy(reply));
        handler.set_allow_dev_gamein_fallback(false);
        const auto root=std::filesystem::path(MXH_SOURCE_DIR)/"data/PlayDH/Resource";
        std::string error;
        ASSERT_TRUE(handler.load_shop_event_rates(root/"Server/DropRate.bin",mxh::server::ShopLocale::China,error)) << error;
        ASSERT_TRUE(handler.load_shop_dup_catalog(root/"ItemdupOption.bin",error)) << error;
        mxh::game::ItemInfo item{}; item.ItemIdx=55001; item.ItemType=11;
        item.ItemKind=test.kind; item.SellPrice=1;
        handler.add_item_info_for_test(item);
        mxh::net::Message enter; enter.header.object_id=777;
        enter.header.category=static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
        enter.header.protocol=static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
        enter.payload.assign(16,0); enter.payload[0]=123;
        handler.on_message({55},enter);
        ASSERT_TRUE(handler.player_runtime_snapshot(777));
        const auto persisted=mxh::db::load_modern_shop_state(db,777,123); ASSERT_TRUE(persisted);
        EXPECT_TRUE(persisted->rows.used_items.empty());
        mxh::db::ResultSet rows;
        ASSERT_TRUE(db.query("SELECT db_idx FROM modern_player_item WHERE player_id=777",{},rows).ok());
        EXPECT_TRUE(rows.rows.empty());
        const auto ack=std::find_if(reply.messages.begin(),reply.messages.end(),[](const auto& message) {
            return message.header.protocol==static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInAck);
        });
        ASSERT_NE(ack,reply.messages.end());
        mxh::game::ItemTotalInfo items{};
        std::memcpy(&items,ack->payload.data()+mxh::game::HERO_TOTAL_ITEM_OFFSET,sizeof(items));
        const auto& cleared = test.container==1 ? items.WearedItem[test.slot] :
            (test.container==3 ? items.PetWearedItem[test.slot] : items.TitanShopItem[test.slot]);
        EXPECT_EQ(cleared.dwDBIdx,0u);
        EXPECT_EQ(cleared.Position,test.position);
    }
}

TEST(MapHandlerTest, OnlineShopPlaytimePublishesMinuteAndCommitsExpiryBeforeUseEnd) {
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg;
    cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());
    ASSERT_TRUE(mxh::db::migrate_modern_schema(db).ok());
    ASSERT_TRUE(db.exec_multi(
        "INSERT INTO character_info(chrid,userid,charname,map_num) VALUES(777,'123','TimerHero',10);").ok());

    mxh::db::LegacyShopAppearanceRows saved;
    saved.used_items.push_back({55001, 0, 9001, 2, 0, 90001});
    saved.used_items.push_back({55002, 0, 9002, 2, 0, 900001});
    const auto initial = mxh::db::load_modern_shop_state(db, 777, 123);
    ASSERT_TRUE(initial);
    ASSERT_TRUE(mxh::db::save_modern_shop_state(db, 777, 123, *initial, saved));

    ReplySpy reply;
    mxh::server::MapHandler handler(db, 10, make_reply_spy(reply));
    handler.set_allow_dev_gamein_fallback(false);
    const auto root = std::filesystem::path(MXH_SOURCE_DIR) / "data/PlayDH/Resource";
    std::string error;
    ASSERT_TRUE(handler.load_shop_event_rates(
        root / "Server/DropRate.bin", mxh::server::ShopLocale::China, error)) << error;
    ASSERT_TRUE(handler.load_shop_dup_catalog(root / "ItemdupOption.bin", error)) << error;
    mxh::game::ItemInfo buff{};
    buff.ItemIdx = 55001;
    buff.ItemKind = 258;
    buff.SellPrice = mxh::game::SHOP_ITEM_PARAM_PLAY_TIME;
    handler.add_item_info_for_test(buff);
    auto long_buff = buff;
    long_buff.ItemIdx = 55002;
    handler.add_item_info_for_test(long_buff);

    std::uint64_t now = 1000;
    ASSERT_TRUE(handler.set_movement_clock_for_test([&] { return now; }));
    mxh::net::Message enter;
    enter.header.object_id = 777;
    enter.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    enter.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    enter.payload.assign(16, 0);
    enter.payload[0] = 123;
    handler.on_message({55}, enter);
    ASSERT_TRUE(handler.player_runtime_snapshot(777));
    reply.messages.clear();

    now = 31001;
    handler.tick_shop_items(30001);
    const auto minute_item = handler.shop_using_item_for_test(777, 55001);
    ASSERT_TRUE(minute_item);
    EXPECT_EQ(minute_item->Data.ShopItem.Remaintime, 60000u);
    ASSERT_EQ(reply.messages.size(), 1u);
    EXPECT_EQ(reply.messages[0].header.category,
        static_cast<std::uint8_t>(mxh::proto::Category::Item));
    EXPECT_EQ(reply.messages[0].header.protocol,
        static_cast<std::uint8_t>(mxh::proto::ItemProtocol::ShopItemOneMinute));
    EXPECT_EQ(reply.messages[0].header.object_id, 777u);
    ASSERT_EQ(reply.messages[0].payload.size(), 4u);
    std::uint32_t wire_icon = 0;
    std::memcpy(&wire_icon, reply.messages[0].payload.data(), sizeof(wire_icon));
    EXPECT_EQ(wire_icon, 55001u);
    const auto before_expiry = mxh::db::load_modern_shop_state(db, 777, 123);
    ASSERT_TRUE(before_expiry);
    ASSERT_EQ(before_expiry->rows.used_items.size(), 2u);
    EXPECT_EQ(before_expiry->rows.used_items[0].remaining_time, 90001u);

    reply.messages.clear();
    now = 61001;
    handler.tick_shop_items(30000);
    ASSERT_TRUE(handler.shop_using_item_for_test(777, 55001));
    EXPECT_TRUE(reply.messages.empty());

    now = 91001;
    handler.tick_shop_items(30000);
    EXPECT_FALSE(handler.shop_using_item_for_test(777, 55001));
    ASSERT_EQ(reply.messages.size(), 3u);
    EXPECT_EQ(reply.messages[0].header.protocol,
        static_cast<std::uint8_t>(mxh::proto::ItemProtocol::TotalInfoLocal));
    EXPECT_EQ(reply.messages[1].header.protocol, mxh::proto::kModernShopAppearance);
    EXPECT_EQ(reply.messages[1].header.category,
        static_cast<std::uint8_t>(mxh::proto::Category::Server));
    EXPECT_EQ(reply.messages[1].payload.size(), sizeof(mxh::game::ShopItemOption));
    EXPECT_EQ(reply.messages[2].header.protocol,
        static_cast<std::uint8_t>(mxh::proto::ItemProtocol::ShopItemUseEnd));
    const auto after_expiry = mxh::db::load_modern_shop_state(db, 777, 123);
    ASSERT_TRUE(after_expiry);
    ASSERT_EQ(after_expiry->rows.used_items.size(), 1u);
    EXPECT_EQ(after_expiry->rows.used_items[0].item_id, 55002u);

    reply.messages.clear();
    now = 601002;
    handler.tick_shop_items(510001);
    EXPECT_TRUE(reply.messages.empty());
    const auto after_flush = mxh::db::load_modern_shop_state(db, 777, 123);
    ASSERT_TRUE(after_flush);
    ASSERT_EQ(after_flush->rows.used_items.size(), 1u);
    EXPECT_EQ(after_flush->rows.used_items[0].remaining_time, 299999u);
    EXPECT_FALSE(handler.is_draining());
}

TEST(MapHandlerTest, OnlineShopExpiryRollbackPublishesNeitherStateNorUseEnd) {
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg;
    cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());
    ASSERT_TRUE(mxh::db::migrate_modern_schema(db).ok());
    ASSERT_TRUE(db.exec_multi(
        "INSERT INTO character_info(chrid,userid,charname,map_num) VALUES(777,'123','RollbackHero',10);"
        "INSERT INTO modern_player_item VALUES(777,2,0,9001,55001,1,0,65535,0);").ok());
    const auto root = std::filesystem::path(MXH_SOURCE_DIR) / "data/PlayDH/Resource";
    std::string error;
    const auto catalog = mxh::server::load_skin_catalog(
        root / "SkinSelectItemList.bin", root / "CostumeSkinItemList.bin", error);
    ASSERT_TRUE(catalog) << error;
    const auto normal = std::find_if(catalog->normal.begin(), catalog->normal.end(), [](const auto& row) {
        return row.equip_item[0] != 0;
    });
    ASSERT_NE(normal, catalog->normal.end());
    mxh::db::LegacyShopAppearanceRows saved;
    saved.skin[0] = normal->equip_item[0];
    saved.used_items.push_back({55001, 0, 9001, 2, 0, 30000});
    const auto initial = mxh::db::load_modern_shop_state(db, 777, 123);
    ASSERT_TRUE(initial);
    ASSERT_TRUE(mxh::db::save_modern_shop_state(db, 777, 123, *initial, saved));

    ReplySpy reply;
    mxh::server::MapHandler handler(db, 10, make_reply_spy(reply));
    handler.set_allow_dev_gamein_fallback(false);
    ASSERT_TRUE(handler.load_shop_event_rates(
        root / "Server/DropRate.bin", mxh::server::ShopLocale::China, error)) << error;
    ASSERT_TRUE(handler.load_shop_dup_catalog(root / "ItemdupOption.bin", error)) << error;
    ASSERT_TRUE(handler.load_skin_catalogs(
        root / "SkinSelectItemList.bin", root / "CostumeSkinItemList.bin", error)) << error;
    mxh::game::ItemInfo buff{};
    buff.ItemIdx = 55001;
    buff.ItemType = 11;
    buff.ItemKind = mxh::server::LEGACY_SHOP_ITEM_NOMALCLOTHES_SKIN;
    buff.SellPrice = mxh::game::SHOP_ITEM_PARAM_PLAY_TIME;
    handler.add_item_info_for_test(buff);
    std::uint64_t now = 1000;
    ASSERT_TRUE(handler.set_movement_clock_for_test([&] { return now; }));

    mxh::net::Message enter;
    enter.header.object_id = 777;
    enter.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    enter.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    enter.payload.assign(16, 0);
    enter.payload[0] = 123;
    handler.on_message({55}, enter);
    ASSERT_TRUE(handler.shop_using_item_for_test(777, 55001));
    reply.messages.clear();
    ASSERT_TRUE(db.exec_multi(
        "CREATE TRIGGER fail_online_shop BEFORE UPDATE OF character_data ON character_info "
        "BEGIN SELECT RAISE(ABORT,'injected online shop failure'); END;").ok());

    now = 31000;
    handler.tick_shop_items(30000);

    EXPECT_TRUE(handler.is_draining());
    EXPECT_TRUE(handler.shop_using_item_for_test(777, 55001));
    EXPECT_TRUE(reply.messages.empty());
    const auto retained = mxh::db::load_modern_shop_state(db, 777, 123);
    ASSERT_TRUE(retained);
    ASSERT_EQ(retained->rows.used_items.size(), 1u);
    EXPECT_EQ(retained->rows.used_items[0].remaining_time, 30000u);
    EXPECT_EQ(retained->rows.skin[0], normal->equip_item[0]);
    mxh::db::ResultSet physical;
    ASSERT_TRUE(db.query("SELECT db_idx FROM modern_player_item WHERE player_id=777", {}, physical).ok());
    ASSERT_EQ(physical.rows.size(), 1u);
    EXPECT_EQ(std::get<std::int64_t>(physical.rows[0][0]), 9001);
}

TEST(MapHandlerTest, OnlineShopTimerRejectsDatabaseStateChangedAfterAdmission) {
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg;
    cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());
    ASSERT_TRUE(mxh::db::migrate_modern_schema(db).ok());
    ASSERT_TRUE(db.exec_multi(
        "INSERT INTO character_info(chrid,userid,charname,map_num) VALUES(777,'123','DriftHero',10);").ok());
    mxh::db::LegacyShopAppearanceRows saved;
    saved.used_items.push_back({55001, 0, 9001, 2, 0, 30000});
    const auto initial = mxh::db::load_modern_shop_state(db, 777, 123);
    ASSERT_TRUE(initial);
    ASSERT_TRUE(mxh::db::save_modern_shop_state(db, 777, 123, *initial, saved));

    ReplySpy reply;
    mxh::server::MapHandler handler(db, 10, make_reply_spy(reply));
    handler.set_allow_dev_gamein_fallback(false);
    const auto root = std::filesystem::path(MXH_SOURCE_DIR) / "data/PlayDH/Resource";
    std::string error;
    ASSERT_TRUE(handler.load_shop_event_rates(
        root / "Server/DropRate.bin", mxh::server::ShopLocale::China, error)) << error;
    ASSERT_TRUE(handler.load_shop_dup_catalog(root / "ItemdupOption.bin", error)) << error;
    mxh::game::ItemInfo buff{};
    buff.ItemIdx = 55001;
    buff.ItemKind = 258;
    buff.SellPrice = mxh::game::SHOP_ITEM_PARAM_PLAY_TIME;
    handler.add_item_info_for_test(buff);
    std::uint64_t now = 1000;
    ASSERT_TRUE(handler.set_movement_clock_for_test([&] { return now; }));

    mxh::net::Message enter;
    enter.header.object_id = 777;
    enter.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    enter.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    enter.payload.assign(16, 0);
    enter.payload[0] = 123;
    handler.on_message({55}, enter);
    ASSERT_TRUE(handler.shop_using_item_for_test(777, 55001));
    reply.messages.clear();

    auto external = mxh::db::load_modern_shop_state(db, 777, 123);
    ASSERT_TRUE(external);
    auto externally_changed = external->rows;
    ASSERT_EQ(externally_changed.used_items.size(), 1u);
    externally_changed.used_items[0].remaining_time = 60000;
    ASSERT_TRUE(mxh::db::save_modern_shop_state(
        db, 777, 123, *external, externally_changed));

    now = 31000;
    handler.tick_shop_items(30000);

    EXPECT_TRUE(handler.is_draining());
    EXPECT_TRUE(handler.shop_using_item_for_test(777, 55001));
    EXPECT_TRUE(reply.messages.empty());
    const auto retained = mxh::db::load_modern_shop_state(db, 777, 123);
    ASSERT_TRUE(retained);
    ASSERT_EQ(retained->rows.used_items.size(), 1u);
    EXPECT_EQ(retained->rows.used_items[0].remaining_time, 60000u);
}

TEST(MapHandlerTest, OnlineRealtimeShopItemUsesPackedClockForMinuteAndExpiry) {
    const auto pack = [](std::uint8_t hour, std::uint8_t minute, std::uint8_t second) {
        return mxh::game::PackedTime{10u << 28 | 9u << 24 | 13u << 18 |
            static_cast<std::uint32_t>(hour) << 12 |
            static_cast<std::uint32_t>(minute) << 6 | second};
    };
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg;
    cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());
    ASSERT_TRUE(mxh::db::migrate_modern_schema(db).ok());
    ASSERT_TRUE(db.exec_multi(
        "INSERT INTO character_info(chrid,userid,charname,map_num) VALUES(777,'123','RealtimeHero',10);"
        "INSERT INTO modern_player_item VALUES(777,2,0,9001,55001,1,0,65535,0);").ok());
    mxh::db::LegacyShopAppearanceRows saved;
    saved.used_items.push_back({55001, 999, 9001, 1, pack(12, 0, 0).value, pack(12, 0, 30).value});
    const auto initial = mxh::db::load_modern_shop_state(db, 777, 123);
    ASSERT_TRUE(initial);
    ASSERT_TRUE(mxh::db::save_modern_shop_state(db, 777, 123, *initial, saved));

    ReplySpy reply;
    mxh::server::MapHandler handler(db, 10, make_reply_spy(reply));
    handler.set_allow_dev_gamein_fallback(false);
    const auto root = std::filesystem::path(MXH_SOURCE_DIR) / "data/PlayDH/Resource";
    std::string error;
    ASSERT_TRUE(handler.load_shop_event_rates(
        root / "Server/DropRate.bin", mxh::server::ShopLocale::China, error)) << error;
    ASSERT_TRUE(handler.load_shop_dup_catalog(root / "ItemdupOption.bin", error)) << error;
    mxh::game::ItemInfo buff{};
    buff.ItemIdx = 55001;
    buff.ItemKind = mxh::server::LEGACY_SHOP_ITEM_MAKEUP;
    buff.ItemType = 11;
    buff.SellPrice = mxh::game::SHOP_ITEM_PARAM_STORED_TIME;
    handler.add_item_info_for_test(buff);
    auto packed_now = pack(12, 0, 0);
    ASSERT_TRUE(handler.set_shop_time_clock_for_test([&] { return packed_now; }));

    mxh::net::Message enter;
    enter.header.object_id = 777;
    enter.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    enter.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    enter.payload.assign(16, 0);
    enter.payload[0] = 123;
    handler.on_message({55}, enter);
    ASSERT_TRUE(handler.shop_using_item_for_test(777, 55001));
    reply.messages.clear();

    packed_now = pack(12, 0, 1);
    handler.tick_shop_items(30000);
    ASSERT_EQ(reply.messages.size(), 1u);
    EXPECT_EQ(reply.messages[0].header.protocol,
        static_cast<std::uint8_t>(mxh::proto::ItemProtocol::ShopItemOneMinute));
    EXPECT_TRUE(handler.shop_using_item_for_test(777, 55001));

    reply.messages.clear();
    packed_now = pack(12, 0, 31);
    handler.tick_shop_items(30000);
    ASSERT_EQ(reply.messages.size(), 4u);
    EXPECT_EQ(reply.messages[0].header.protocol,
        static_cast<std::uint8_t>(mxh::proto::ItemProtocol::DiscardAck));
    ASSERT_EQ(reply.messages[0].payload.size(), 6u);
    std::uint16_t discard_position = 0;
    std::memcpy(&discard_position, reply.messages[0].payload.data(), 2u);
    EXPECT_EQ(discard_position, mxh::game::TP_SHOPINVEN_START);
    EXPECT_EQ(reply.messages[1].header.protocol,
        static_cast<std::uint8_t>(mxh::proto::ItemProtocol::TotalInfoLocal));
    EXPECT_EQ(reply.messages[2].header.protocol, mxh::proto::kModernShopAppearance);
    EXPECT_EQ(reply.messages[2].header.category,
        static_cast<std::uint8_t>(mxh::proto::Category::Server));
    EXPECT_EQ(reply.messages[2].payload.size(), sizeof(mxh::game::ShopItemOption));
    EXPECT_EQ(reply.messages[3].header.protocol,
        static_cast<std::uint8_t>(mxh::proto::ItemProtocol::ShopItemUseEnd));
    EXPECT_FALSE(handler.shop_using_item_for_test(777, 55001));
    const auto persisted = mxh::db::load_modern_shop_state(db, 777, 123);
    ASSERT_TRUE(persisted);
    EXPECT_TRUE(persisted->rows.used_items.empty());
    mxh::db::ResultSet inventory;
    ASSERT_TRUE(db.query("SELECT db_idx FROM modern_player_item WHERE player_id=777", {}, inventory).ok());
    EXPECT_TRUE(inventory.rows.empty());
}

TEST(MapHandlerTest, OnlineRealtimeExpirySearchesOriginalWornContainers) {
    struct Case {
        std::int64_t container;
        std::int64_t slot;
        std::uint16_t kind;
        std::uint16_t position;
        bool discard_ack;
    };
    const auto pack = [](std::uint8_t hour, std::uint8_t minute, std::uint8_t second) {
        return mxh::game::PackedTime{10u << 28 | 9u << 24 | 13u << 18 |
            static_cast<std::uint32_t>(hour) << 12 |
            static_cast<std::uint32_t>(minute) << 6 | second};
    };
    for (const auto test : std::array{
             Case{1, 2, mxh::server::LEGACY_SHOP_ITEM_MAKEUP,
                  static_cast<std::uint16_t>(mxh::game::TP_WEAREDITEM_START + 2), true},
             Case{1, 2, mxh::server::LEGACY_SHOP_ITEM_EQUIP,
                  static_cast<std::uint16_t>(mxh::game::TP_WEAREDITEM_START + 2), false},
             Case{3, 1, mxh::server::LEGACY_SHOP_ITEM_PET_EQUIP,
                  static_cast<std::uint16_t>(mxh::game::TP_PETWEAR_START + 1), true},
             Case{5, 2, mxh::server::LEGACY_SHOP_ITEM_TITAN_EQUIP,
                  static_cast<std::uint16_t>(mxh::game::TP_TITANSHOPITEM_START + 2), true}}) {
        SCOPED_TRACE(test.container);
        mxh::db::SqliteAdapter db;
        mxh::db::ConnectionConfig cfg;
        cfg.path = ":memory:";
        ASSERT_TRUE(db.connect(cfg).ok());
        ASSERT_TRUE(mxh::db::migrate_modern_schema(db).ok());
        const std::vector<mxh::db::Bind> no_args;
        ASSERT_TRUE(db.execute(
            "INSERT INTO character_info(chrid,userid,charname,map_num) VALUES(777,'123','WornTimerHero',10)",
            no_args).ok());
        const std::array item_args{
            mxh::db::bind(std::int64_t{777}), mxh::db::bind(test.container),
            mxh::db::bind(test.slot), mxh::db::bind(std::int64_t{9001}),
            mxh::db::bind(std::int64_t{55001}), mxh::db::bind(std::int64_t{1}),
            mxh::db::bind(std::int64_t{0}), mxh::db::bind(std::int64_t{65535}),
            mxh::db::bind(std::int64_t{0})};
        ASSERT_TRUE(db.execute(
            "INSERT INTO modern_player_item VALUES(?,?,?,?,?,?,?,?,?)",
            item_args).ok());
        mxh::db::LegacyShopAppearanceRows saved;
        saved.used_items.push_back(
            {55001, 999, 9001, 1, pack(12, 0, 0).value, pack(12, 0, 30).value});
        const auto initial = mxh::db::load_modern_shop_state(db, 777, 123);
        ASSERT_TRUE(initial);
        ASSERT_TRUE(mxh::db::save_modern_shop_state(db, 777, 123, *initial, saved));

        ReplySpy reply;
        mxh::server::MapHandler handler(db, 10, make_reply_spy(reply));
        handler.set_allow_dev_gamein_fallback(false);
        const auto root = std::filesystem::path(MXH_SOURCE_DIR) / "data/PlayDH/Resource";
        std::string error;
        ASSERT_TRUE(handler.load_shop_event_rates(
            root / "Server/DropRate.bin", mxh::server::ShopLocale::China, error)) << error;
        ASSERT_TRUE(handler.load_shop_dup_catalog(root / "ItemdupOption.bin", error)) << error;
        mxh::game::ItemInfo item{};
        item.ItemIdx = 55001;
        item.ItemKind = test.kind;
        item.ItemType = 11;
        item.SellPrice = mxh::game::SHOP_ITEM_PARAM_STORED_TIME;
        handler.add_item_info_for_test(item);
        auto packed_now = pack(12, 0, 0);
        ASSERT_TRUE(handler.set_shop_time_clock_for_test([&] { return packed_now; }));

        mxh::net::Message enter;
        enter.header.object_id = 777;
        enter.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
        enter.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
        enter.payload.assign(16, 0);
        enter.payload[0] = 123;
        handler.on_message({55}, enter);
        ASSERT_TRUE(handler.shop_using_item_for_test(777, 55001));
        reply.messages.clear();

        packed_now = pack(12, 0, 31);
        handler.tick_shop_items(30000);
        ASSERT_FALSE(handler.is_draining());
        EXPECT_FALSE(handler.shop_using_item_for_test(777, 55001));
        ASSERT_EQ(reply.messages.size(), test.discard_ack ? 4u : 3u);
        std::size_t total_index = 0;
        if (test.discard_ack) {
            EXPECT_EQ(reply.messages[0].header.protocol,
                static_cast<std::uint8_t>(mxh::proto::ItemProtocol::DiscardAck));
            std::uint16_t discarded_position = 0;
            std::memcpy(&discarded_position, reply.messages[0].payload.data(), 2u);
            EXPECT_EQ(discarded_position, test.position);
            total_index = 1;
        }
        EXPECT_EQ(reply.messages[total_index].header.protocol,
                        static_cast<std::uint8_t>(mxh::proto::ItemProtocol::TotalInfoLocal));
        ASSERT_EQ(reply.messages[total_index].payload.size(),sizeof(mxh::game::ItemTotalInfo));
        mxh::game::ItemTotalInfo total{};
        std::memcpy(&total,reply.messages[total_index].payload.data(),sizeof(total));
        const auto& cleared = test.container==1 ? total.WearedItem[test.slot] :
            (test.container==3 ? total.PetWearedItem[test.slot] : total.TitanShopItem[test.slot]);
        EXPECT_EQ(cleared.dwDBIdx,0u);
        EXPECT_EQ(cleared.Position,test.position);
        EXPECT_EQ(reply.messages[total_index+1].header.protocol, mxh::proto::kModernShopAppearance);
        EXPECT_EQ(reply.messages[total_index+2].header.protocol,
            static_cast<std::uint8_t>(mxh::proto::ItemProtocol::ShopItemUseEnd));
        mxh::db::ResultSet inventory;
        ASSERT_TRUE(db.query("SELECT db_idx FROM modern_player_item WHERE player_id=777", {}, inventory).ok());
        EXPECT_TRUE(inventory.rows.empty());
    }
}

TEST(MapHandlerTest, SkinExpiryClearsGameInSnapshotAndOnlinePublishesLegacyAck) {
    const auto pack = [](std::uint8_t hour, std::uint8_t minute, std::uint8_t second) {
        return mxh::game::PackedTime{10u << 28 | 9u << 24 | 13u << 18 |
            static_cast<std::uint32_t>(hour) << 12 |
            static_cast<std::uint32_t>(minute) << 6 | second};
    };
    const auto root = std::filesystem::path(MXH_SOURCE_DIR) / "data/PlayDH/Resource";
    std::string error;
    const auto catalog = mxh::server::load_skin_catalog(
        root / "SkinSelectItemList.bin", root / "CostumeSkinItemList.bin", error);
    ASSERT_TRUE(catalog) << error;
    const auto normal = std::find_if(catalog->normal.begin(), catalog->normal.end(), [](const auto& row) {
        return row.equip_item[0] != 0;
    });
    ASSERT_NE(normal, catalog->normal.end());
    const auto costume = std::find_if(catalog->costume.begin(), catalog->costume.end(), [](const auto& row) {
        return row.equip_item[0] != 0;
    });
    ASSERT_NE(costume, catalog->costume.end());
    struct SkinCase { std::uint16_t kind; std::uint16_t equipment; };

    for (const auto skin_case : std::array{
             SkinCase{mxh::server::LEGACY_SHOP_ITEM_NOMALCLOTHES_SKIN, normal->equip_item[0]},
             SkinCase{mxh::server::LEGACY_SHOP_ITEM_COSTUME_SKIN, costume->equip_item[0]}}) {
      for (const bool expire_online : {false, true}) {
        SCOPED_TRACE(testing::Message() << "kind=" << skin_case.kind
                                      << " online=" << expire_online);
        mxh::db::SqliteAdapter db;
        mxh::db::ConnectionConfig cfg;
        cfg.path = ":memory:";
        ASSERT_TRUE(db.connect(cfg).ok());
        ASSERT_TRUE(mxh::db::migrate_modern_schema(db).ok());
        ASSERT_TRUE(db.exec_multi(
            "INSERT INTO character_info(chrid,userid,charname,map_num) VALUES(777,'123','SkinHero',10);"
            "INSERT INTO character_info(chrid,userid,charname,map_num) VALUES(778,'124','ObserverHero',10);"
            "INSERT INTO modern_player_item VALUES(777,2,0,9001,55001,1,0,65535,0);").ok());
        mxh::db::LegacyShopAppearanceRows saved;
        saved.skin[0] = skin_case.equipment;
        saved.used_items.push_back({55001, 390, 9001, 1, pack(12, 0, 0).value,
            pack(12, 0, expire_online ? 30 : 0).value});
        const auto initial = mxh::db::load_modern_shop_state(db, 777, 123);
        ASSERT_TRUE(initial);
        ASSERT_TRUE(mxh::db::save_modern_shop_state(db, 777, 123, *initial, saved));

        ReplySpy reply;
        mxh::server::MapHandler handler(db, 10, make_reply_spy(reply));
        handler.set_allow_dev_gamein_fallback(false);
        ASSERT_TRUE(handler.load_shop_event_rates(
            root / "Server/DropRate.bin", mxh::server::ShopLocale::China, error)) << error;
        ASSERT_TRUE(handler.load_shop_dup_catalog(root / "ItemdupOption.bin", error)) << error;
        ASSERT_TRUE(handler.load_skin_catalogs(
            root / "SkinSelectItemList.bin", root / "CostumeSkinItemList.bin", error)) << error;
        mxh::game::ItemInfo skin_item{};
        skin_item.ItemIdx = 55001;
        skin_item.ItemType = 11;
        skin_item.ItemKind = skin_case.kind;
        skin_item.SellPrice = mxh::game::SHOP_ITEM_PARAM_STORED_TIME;
        handler.add_item_info_for_test(skin_item);
        auto now = pack(12, 0, expire_online ? 0 : 1);
        ASSERT_TRUE(handler.set_shop_time_clock_for_test([&] { return now; }));

        mxh::net::Message enter;
        enter.header.object_id = 777;
        enter.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
        enter.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
        enter.payload.assign(16, 0);
        enter.payload[0] = 123;
        handler.on_message({55}, enter);
        const auto ack = std::find_if(reply.messages.begin(), reply.messages.end(), [](const auto& packet) {
            return packet.header.category == static_cast<std::uint8_t>(mxh::proto::Category::UserConn) &&
                packet.header.protocol == static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInAck);
        });
        ASSERT_NE(ack, reply.messages.end());
        mxh::game::ShopItemOption options{};
        std::memcpy(&options, ack->payload.data() + mxh::game::HERO_TOTAL_SHOP_OPTION_OFFSET,
            sizeof(options));
        if (!expire_online) {
            EXPECT_EQ(options.wSkinItem[0], 0u);
            EXPECT_FALSE(handler.shop_using_item_for_test(777, 55001));
            EXPECT_TRUE(std::none_of(reply.messages.begin(), reply.messages.end(), [](const auto& packet) {
                return packet.header.protocol == static_cast<std::uint8_t>(
                    mxh::proto::ItemProtocol::ShopItemUseEnd);
            }));
        } else {
            EXPECT_EQ(options.wSkinItem[0], skin_case.equipment);
            ASSERT_TRUE(handler.shop_using_item_for_test(777, 55001));
            auto observer_enter = enter;
            observer_enter.header.object_id = 778;
            observer_enter.payload[0] = 124;
            handler.on_message({56}, observer_enter);
            ASSERT_TRUE(handler.player_runtime_snapshot(778));
            reply.messages.clear();
            reply.connection_ids.clear();
            now = pack(12, 0, 31);
            handler.tick_shop_items(30000);
            ASSERT_EQ(reply.messages.size(), 5u);
            EXPECT_EQ(reply.messages[0].header.category,
                static_cast<std::uint8_t>(mxh::proto::Category::ItemExt));
            EXPECT_EQ(reply.messages[0].header.protocol, static_cast<std::uint8_t>(
                mxh::proto::ItemExtProtocol::SkinItemDiscardAck));
            EXPECT_EQ(reply.messages[0].header.object_id, 777u);
            ASSERT_EQ(reply.messages[0].payload.size(), 10u);
            mxh::server::SkinItemSlots wire_skin{};
            std::memcpy(wire_skin.data(), reply.messages[0].payload.data(), sizeof(wire_skin));
            EXPECT_EQ(wire_skin, mxh::server::SkinItemSlots{});
            EXPECT_EQ(reply.messages[1].header.protocol, static_cast<std::uint8_t>(
                mxh::proto::ItemExtProtocol::SkinItemDiscardAck));
            ASSERT_GE(reply.connection_ids.size(), 2u);
            const std::array observer_ids{
                reply.connection_ids[0].value, reply.connection_ids[1].value};
            EXPECT_NE(observer_ids[0], observer_ids[1]);
            EXPECT_TRUE(std::find(observer_ids.begin(), observer_ids.end(), 55u) != observer_ids.end());
            EXPECT_TRUE(std::find(observer_ids.begin(), observer_ids.end(), 56u) != observer_ids.end());
            EXPECT_EQ(reply.messages[2].header.protocol,
                static_cast<std::uint8_t>(mxh::proto::ItemProtocol::TotalInfoLocal));
            EXPECT_EQ(reply.messages[3].header.protocol, mxh::proto::kModernShopAppearance);
            EXPECT_EQ(reply.messages[4].header.protocol,
                static_cast<std::uint8_t>(mxh::proto::ItemProtocol::ShopItemUseEnd));
        }
        const auto persisted = mxh::db::load_modern_shop_state(db, 777, 123);
        ASSERT_TRUE(persisted);
        EXPECT_EQ(persisted->rows.skin, mxh::server::SkinItemSlots{});
        EXPECT_TRUE(persisted->rows.used_items.empty());
        mxh::db::ResultSet physical;
        ASSERT_TRUE(db.query("SELECT db_idx FROM modern_player_item WHERE player_id=777", {}, physical).ok());
        EXPECT_TRUE(physical.rows.empty());
      }
    }
}

TEST(MapHandlerTest, ShopAdmissionBeginFailureDrainsConnection) {
    mxh::db::SqliteAdapter db; mxh::db::ConnectionConfig cfg; cfg.path=":memory:";
    ASSERT_TRUE(db.connect(cfg).ok()); ASSERT_TRUE(mxh::db::migrate_modern_schema(db).ok());
    ASSERT_TRUE(db.exec_multi("INSERT INTO character_info(chrid,userid,charname,map_num) VALUES(777,'123','BeginHero',10);").ok());
    ReplySpy reply; mxh::server::MapHandler handler(db,10,make_reply_spy(reply));
    handler.set_allow_dev_gamein_fallback(false);
    // An existing transaction makes BEGIN fail while connection ownership is uncertain.
    ASSERT_TRUE(db.begin_transaction().ok());
    mxh::net::Message enter; enter.header.object_id=777;
    enter.header.category=static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    enter.header.protocol=static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    enter.payload.assign(16,0); enter.payload[0]=123;
    handler.on_message({55},enter);
    EXPECT_FALSE(handler.player_runtime_snapshot(777));
    EXPECT_TRUE(handler.is_draining());
    EXPECT_FALSE(db.is_connected());
    ASSERT_FALSE(reply.messages.empty());
    EXPECT_EQ(reply.last_message.header.protocol,static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInNack));
}

TEST(MapHandlerTest, InvalidShopInventoryRejectsEntryWithoutErasingDatabaseRows) {
    for (const auto* slot : {"-1", "20", "40", "'bad-slot'"}) {
        SCOPED_TRACE(slot);
        mxh::db::SqliteAdapter db; mxh::db::ConnectionConfig cfg; cfg.path=":memory:";
        ASSERT_TRUE(db.connect(cfg).ok()); ASSERT_TRUE(mxh::db::migrate_modern_schema(db).ok());
        ASSERT_TRUE(db.exec_multi("INSERT INTO character_info(chrid,userid,charname,map_num) VALUES(123,'123','BadShop',7);").ok());
        ASSERT_TRUE(db.exec_multi(std::string("INSERT INTO modern_player_item VALUES(123,2,")+slot+",9003,55001,1,0,65535,0);").ok());
        ReplySpy reply; mxh::server::MapHandler handler(db,7,make_reply_spy(reply));
        handler.set_allow_dev_gamein_fallback(false);
        mxh::net::Message enter; enter.header.object_id=123;
        enter.header.category=static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
        enter.header.protocol=static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
        enter.payload.assign(16,0); enter.payload[0]=123;
        handler.on_message({55},enter);
        EXPECT_FALSE(handler.player_runtime_snapshot(123));
        ASSERT_FALSE(reply.messages.empty());
        EXPECT_EQ(reply.messages.back().header.protocol,static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInNack));
        handler.on_disconnect({55},mxh::net::NetError::Disconnected);
        mxh::db::ResultSet rows; ASSERT_TRUE(db.query("SELECT db_idx FROM modern_player_item WHERE player_id=123",{},rows).ok());
        ASSERT_EQ(rows.rows.size(),1u); EXPECT_EQ(std::get<std::int64_t>(rows.rows[0][0]),9003);
    }
}

TEST(MapHandlerTest, InvalidExtendedShopContainerSlotsRejectEntryWithoutErasingRows) {
    struct Case { const char* container; const char* slot; };
    for (const auto test : std::array{
             Case{"3","3"}, Case{"4","7"}, Case{"5","4"}, Case{"6","0"},
             Case{"-1","0"}, Case{"'bad-container'","0"}, Case{"3","'bad-slot'"},
             Case{"3","9223372036854775807"}}) {
        SCOPED_TRACE(std::string(test.container) + ":" + test.slot);
        mxh::db::SqliteAdapter db; mxh::db::ConnectionConfig cfg; cfg.path=":memory:";
        ASSERT_TRUE(db.connect(cfg).ok()); ASSERT_TRUE(mxh::db::migrate_modern_schema(db).ok());
        ASSERT_TRUE(db.exec_multi("INSERT INTO character_info(chrid,userid,charname,map_num) VALUES(123,'123','BadExtended',7);"
            "INSERT INTO modern_player_item VALUES(123," + std::string(test.container) + "," +
            test.slot + ",9003,55001,1,0,65535,0);").ok());
        ReplySpy reply; mxh::server::MapHandler handler(db,7,make_reply_spy(reply));
        handler.set_allow_dev_gamein_fallback(false);
        mxh::net::Message enter; enter.header.object_id=123;
        enter.header.category=static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
        enter.header.protocol=static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
        enter.payload.assign(16,0); enter.payload[0]=123;
        handler.on_message({55},enter);
        EXPECT_FALSE(handler.player_runtime_snapshot(123));
        ASSERT_FALSE(reply.messages.empty());
        EXPECT_EQ(reply.messages.back().header.protocol,
            static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInNack));
        handler.on_disconnect({55},mxh::net::NetError::Disconnected);
        mxh::db::ResultSet rows;
        ASSERT_TRUE(db.query("SELECT container,slot,db_idx FROM modern_player_item WHERE player_id=123",{},rows).ok());
        ASSERT_EQ(rows.rows.size(),1u);
        EXPECT_EQ(std::get<std::int64_t>(rows.rows[0][2]),9003);
    }
}

TEST(MapHandlerTest, MoveSynSwapsAuthoritativeSlotsAndPersistsDestination) {
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg{}; cfg.backend = "sqlite"; cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());
    ASSERT_TRUE(db.exec_multi(
        "CREATE TABLE modern_player_item (player_id INTEGER NOT NULL,container INTEGER NOT NULL,slot INTEGER NOT NULL,"
        "db_idx INTEGER NOT NULL,item_idx INTEGER NOT NULL,durability INTEGER NOT NULL,rare_idx INTEGER NOT NULL,"
        "quick_position INTEGER NOT NULL,item_param INTEGER NOT NULL,PRIMARY KEY(player_id,container,slot),"
        "UNIQUE(player_id,db_idx));").ok());
    ReplySpy reply; mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    mxh::net::Message game_in; game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    const auto connection = mxh::net::make_connection_id(55);
    handler.on_message(connection, game_in);
    ASSERT_TRUE(handler.add_player_item_for_test(123u, mxh::game::make_item(9001u, 555u, 4u)));
    mxh::net::Message move; move.header.object_id = 123u;
    move.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    move.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::MoveSyn);
    move.payload.resize(24u, 0u);
    const std::uint32_t db_idx = 9001u; const std::uint16_t destination = 9u;
    std::memcpy(move.payload.data(), &db_idx, sizeof(db_idx));
    std::memcpy(move.payload.data() + 22u, &destination, sizeof(destination));
    handler.on_message(connection, move);
    mxh::db::ResultSet rows; const std::vector<mxh::db::Bind> no_args;
    ASSERT_TRUE(db.query("SELECT slot,item_idx FROM modern_player_item WHERE player_id=123", no_args, rows).ok());
    ASSERT_EQ(rows.rows.size(), 1u);
    EXPECT_EQ(std::get<std::int64_t>(rows.rows[0][0]), 9);
    EXPECT_EQ(std::get<std::int64_t>(rows.rows[0][1]), 555);
}

TEST(MapHandlerTest, MoveSynMovesInventoryItemIntoEquipmentAndPersistsContainer) {
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg{}; cfg.backend = "sqlite"; cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());
    ASSERT_TRUE(db.exec_multi(
        "CREATE TABLE modern_player_item (player_id INTEGER NOT NULL,container INTEGER NOT NULL,slot INTEGER NOT NULL,"
        "db_idx INTEGER NOT NULL,item_idx INTEGER NOT NULL,durability INTEGER NOT NULL,rare_idx INTEGER NOT NULL,"
        "quick_position INTEGER NOT NULL,item_param INTEGER NOT NULL,PRIMARY KEY(player_id,container,slot),"
        "UNIQUE(player_id,db_idx));").ok());
    ReplySpy reply; mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    mxh::net::Message game_in; game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    const auto connection = mxh::net::make_connection_id(55);
    handler.on_message(connection, game_in);
    mxh::game::ItemInfo equip_info{};
    equip_info.ItemIdx = 601u;
    equip_info.ItemKind = 2048u;  // eEQUIP_ITEM
    equip_info.EquipKind = 0u;    // eWearedItem_Hat
    handler.add_item_info_for_test(equip_info);
    ASSERT_TRUE(handler.add_player_item_for_test(123u, mxh::game::make_item(9101u, 601u, 0u)));

    mxh::net::Message move; move.header.object_id = 123u;
    move.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    move.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::MoveSyn);
    move.payload.resize(24u, 0u);
    const std::uint32_t db_idx = 9101u;
    const std::uint16_t destination = mxh::game::TP_WEAREDITEM_START;
    std::memcpy(move.payload.data(), &db_idx, sizeof(db_idx));
    std::memcpy(move.payload.data() + 22u, &destination, sizeof(destination));
    handler.on_message(connection, move);

    mxh::db::ResultSet rows; const std::vector<mxh::db::Bind> no_args;
    ASSERT_TRUE(db.query("SELECT container,slot,item_idx FROM modern_player_item WHERE player_id=123",
                         no_args, rows).ok());
    ASSERT_EQ(rows.rows.size(), 1u);
    EXPECT_EQ(std::get<std::int64_t>(rows.rows[0][0]), 1);
    EXPECT_EQ(std::get<std::int64_t>(rows.rows[0][1]), 0);
    EXPECT_EQ(std::get<std::int64_t>(rows.rows[0][2]), 601);
    ASSERT_FALSE(reply.messages.empty());
    EXPECT_EQ(reply.messages.back().header.protocol,
              static_cast<std::uint8_t>(mxh::proto::ItemProtocol::TotalInfoLocal));
    EXPECT_EQ(reply.messages.back().payload.size(), sizeof(mxh::game::ItemTotalInfo));
}

TEST(MapHandlerTest, MoveSynRejectsKnownNonEquipmentItemIntoEquipment) {
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg{}; cfg.backend = "sqlite"; cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());
    ASSERT_TRUE(db.exec_multi(
        "CREATE TABLE modern_player_item (player_id INTEGER NOT NULL,container INTEGER NOT NULL,slot INTEGER NOT NULL,"
        "db_idx INTEGER NOT NULL,item_idx INTEGER NOT NULL,durability INTEGER NOT NULL,rare_idx INTEGER NOT NULL,"
        "quick_position INTEGER NOT NULL,item_param INTEGER NOT NULL,PRIMARY KEY(player_id,container,slot),"
        "UNIQUE(player_id,db_idx));").ok());
    ReplySpy reply; mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    mxh::game::ItemInfo item_info{};
    item_info.ItemIdx = 601u;
    item_info.ItemKind = 0u;
    handler.add_item_info_for_test(item_info);
    mxh::net::Message game_in; game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    const auto connection = mxh::net::make_connection_id(55);
    handler.on_message(connection, game_in);
    ASSERT_TRUE(handler.add_player_item_for_test(123u, mxh::game::make_item(9101u, 601u, 0u)));
    mxh::net::Message move; move.header.object_id = 123u;
    move.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    move.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::MoveSyn);
    move.payload.resize(24u, 0u);
    const std::uint32_t db_idx = 9101u;
    const std::uint16_t destination = mxh::game::TP_WEAREDITEM_START;
    std::memcpy(move.payload.data(), &db_idx, sizeof(db_idx));
    std::memcpy(move.payload.data() + 22u, &destination, sizeof(destination));
    handler.on_message(connection, move);
    ASSERT_FALSE(reply.messages.empty());
    EXPECT_EQ(reply.messages.back().header.protocol,
              static_cast<std::uint8_t>(mxh::proto::ItemProtocol::MoveNack));
    const auto snapshot = handler.player_runtime_snapshot(123u);
    ASSERT_TRUE(snapshot.has_value());
    EXPECT_EQ(snapshot->inventory_count, 1u);
}

TEST(MapHandlerTest, MoveSynRejectsEquipmentInWrongWearSlot) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    mxh::game::ItemInfo item_info{};
    item_info.ItemIdx = 602u;
    item_info.ItemKind = 2048u;  // eEQUIP_ITEM
    item_info.EquipKind = 1u;    // weapon slot, not hat slot 0
    handler.add_item_info_for_test(item_info);
    mxh::net::Message game_in; game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    const auto connection = mxh::net::make_connection_id(55);
    handler.on_message(connection, game_in);
    ASSERT_TRUE(handler.add_player_item_for_test(123u, mxh::game::make_item(9102u, 602u, 0u)));

    mxh::net::Message move; move.header.object_id = 123u;
    move.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    move.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::MoveSyn);
    move.payload.resize(24u, 0u);
    const std::uint32_t db_idx = 9102u;
    const std::uint16_t destination = mxh::game::TP_WEAREDITEM_START; // hat
    std::memcpy(move.payload.data(), &db_idx, sizeof(db_idx));
    std::memcpy(move.payload.data() + 22u, &destination, sizeof(destination));
    handler.on_message(connection, move);
    ASSERT_FALSE(reply.messages.empty());
    EXPECT_EQ(reply.messages.back().header.protocol,
              static_cast<std::uint8_t>(mxh::proto::ItemProtocol::MoveNack));
}

TEST(MapHandlerTest, MoveSynRejectsEquipmentAbovePlayerLevelLimit) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    mxh::game::ItemInfo item_info{};
    item_info.ItemIdx = 603u;
    item_info.ItemKind = 2048u;
    item_info.EquipKind = 0u;
    item_info.LimitLevel = 5u;
    handler.add_item_info_for_test(item_info);
    mxh::net::Message game_in; game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    const auto connection = mxh::net::make_connection_id(55);
    handler.on_message(connection, game_in);
    ASSERT_TRUE(handler.add_player_item_for_test(123u, mxh::game::make_item(9103u, 603u, 0u)));

    mxh::net::Message move; move.header.object_id = 123u;
    move.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    move.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::MoveSyn);
    move.payload.resize(24u, 0u);
    const std::uint32_t db_idx = 9103u;
    const std::uint16_t destination = mxh::game::TP_WEAREDITEM_START;
    std::memcpy(move.payload.data(), &db_idx, sizeof(db_idx));
    std::memcpy(move.payload.data() + 22u, &destination, sizeof(destination));
    handler.on_message(connection, move);
    ASSERT_FALSE(reply.messages.empty());
    EXPECT_EQ(reply.messages.back().header.protocol,
              static_cast<std::uint8_t>(mxh::proto::ItemProtocol::MoveNack));
}

TEST(MapHandlerTest, MoveSynRejectsCrossEquipmentSwapThatWouldMisplaceTarget) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    mxh::game::ItemInfo hat{};
    hat.ItemIdx = 601u; hat.ItemKind = 2048u; hat.EquipKind = 0u;
    mxh::game::ItemInfo weapon{};
    weapon.ItemIdx = 602u; weapon.ItemKind = 2048u; weapon.EquipKind = 1u;
    handler.add_item_info_for_test(hat);
    handler.add_item_info_for_test(weapon);
    mxh::net::Message game_in; game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    const auto connection = mxh::net::make_connection_id(55);
    handler.on_message(connection, game_in);
    ASSERT_TRUE(handler.add_player_item_for_test(123u, mxh::game::make_item(9101u, 601u, 0u)));
    ASSERT_TRUE(handler.add_player_item_for_test(123u, mxh::game::make_item(9102u, 602u, 1u)));

    const auto move_item = [&](std::uint32_t db_idx, std::uint16_t target) {
        mxh::net::Message move; move.header.object_id = 123u;
        move.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
        move.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::MoveSyn);
        move.payload.resize(24u, 0u);
        std::memcpy(move.payload.data(), &db_idx, sizeof(db_idx));
        std::memcpy(move.payload.data() + 22u, &target, sizeof(target));
        handler.on_message(connection, move);
    };
    move_item(9101u, mxh::game::TP_WEAREDITEM_START);      // hat -> slot 0
    move_item(9102u, mxh::game::TP_WEAREDITEM_START + 1u); // weapon -> slot 1
    move_item(9101u, mxh::game::TP_WEAREDITEM_START + 1u); // hat cannot -> slot 1

    ASSERT_FALSE(reply.messages.empty());
    EXPECT_EQ(reply.messages.back().header.protocol,
              static_cast<std::uint8_t>(mxh::proto::ItemProtocol::MoveNack));
}

TEST(MapHandlerTest, MonsterDeathNotifyReachesClientThenPickupSynClaims) {
    MockDbAdapter db;
    std::vector<mxh::net::Message> replies;
    mxh::server::MapHandler handler(db, 7,
        [&](mxh::net::ConnectionId, const mxh::net::Message& message) {
            replies.push_back(message);
        });
    const auto connection = mxh::net::make_connection_id(55);
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);

    mxh::server::DropTable table;
    table.drop_id = 9u;
    table.monster_kind = 77u;
    table.entries.push_back(mxh::server::DropItemEntry{501u, 10000u, 1u, 1u});
    handler.register_drop_table(table);

    mxh::game::MonsterInstance monster;
    monster.object_id = 88002u;
    monster.monster_kind = 77u;
    monster.map_num = 7u;
    monster.max_life = 1u;
    monster.current_life = 1u;
    monster.drop_item_id = 9u;
    monster.drop_item_ratio = 100u;
    monster.pos_x = 25000.0f;
    monster.pos_z = 25000.0f;
    ASSERT_TRUE(handler.add_monster_instance(monster));
    const auto drop = handler.apply_monster_damage(123u, monster.object_id, 1u, 0u);
    ASSERT_TRUE(drop.has_value());
    const auto notify = std::find_if(replies.begin(), replies.end(), [](const auto& message) {
        return message.header.protocol ==
            static_cast<std::uint8_t>(mxh::proto::ItemProtocol::MonsterObtainNotify);
    });
    ASSERT_NE(notify, replies.end());

    mxh::client::CInGameState state;
    state.on_message(mxh::net::make_connection_id(1), *notify);
    ASSERT_EQ(state.ground_drops().size(), 1u);
    EXPECT_EQ(state.ground_drops()[0].item_id, 501u);

    replies.clear();
    handler.on_message(connection, mxh::client::make_pickup_message(123u, drop->object_id));
    const auto ack = std::find_if(replies.begin(), replies.end(), [](const auto& message) {
        return message.header.protocol ==
            static_cast<std::uint8_t>(mxh::proto::ItemProtocol::PickupAck);
    });
    ASSERT_NE(ack, replies.end());
    state.on_message(mxh::net::make_connection_id(1), *ack);
    EXPECT_TRUE(state.ground_drops().empty());
    EXPECT_EQ(handler.player_runtime_snapshot(123u)->inventory_count, 1u);
}

namespace {
// Hoisted bin-file helpers used by SpeechSynShopListFeedsClientThenBuySynDebitsMoney
// (and the late R-8 / dealitem / quest tests further below). Kept here so the
// SpeechSynShopList test can resolve them in source order. The original
// definitions lived further down (line ~1908/2100/2310); they were moved here
// to avoid a forward-declaration-to-anonymous-namespace mismatch (each anonymous
// namespace in a TU has a unique generated name).
std::filesystem::path write_temp_bin(const std::vector<std::uint8_t>& bytes) {
    static std::atomic<std::uint64_t> next_file{0};
#ifdef _WIN32
    const auto process_id = _getpid();
#else
    const auto process_id = getpid();
#endif
    const auto path = std::filesystem::temp_directory_path() /
        ("mxh_map_handler_load_test_" + std::to_string(process_id) + "_" +
         std::to_string(next_file.fetch_add(1)) + ".bin");
    std::ofstream ofs(path, std::ios::binary);
    ofs.write(reinterpret_cast<const char*>(bytes.data()),
               static_cast<std::streamsize>(bytes.size()));
    return path;
}
std::vector<std::uint8_t> synthesize_dealitem_bin(const std::string& text) {
    std::vector<std::uint8_t> payload(text.begin(), text.end());
    const auto encrypted = mxh::compat::encrypt_bin_payload(payload, 42);
    mxh::compat::MhFileHeader header{1, 42, static_cast<std::uint32_t>(payload.size())};
    std::vector<std::uint8_t> raw(sizeof(header) + 1 + encrypted.size() + 1);
    std::memcpy(raw.data(), &header, sizeof(header));
    std::copy(encrypted.begin(), encrypted.end(), raw.begin() + sizeof(header) + 1);
    return raw;
}
std::vector<std::uint8_t> synthesize_item_list_bin(const std::string& text) {
    std::vector<std::uint8_t> out;
    const std::uint32_t version = 1;
    const std::uint32_t type = 3;
    const std::uint32_t file_size = static_cast<std::uint32_t>(text.size());
    auto put_u32le = [&out](std::uint32_t v) {
        out.push_back(static_cast<std::uint8_t>(v & 0xFFu));
        out.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFFu));
        out.push_back(static_cast<std::uint8_t>((v >> 16) & 0xFFu));
        out.push_back(static_cast<std::uint8_t>((v >> 24) & 0xFFu));
    };
    put_u32le(version);
    put_u32le(type);
    put_u32le(file_size);
    out.push_back(0);  // crc1 (not validated)

    std::vector<std::uint8_t> payload(text.begin(), text.end());
    std::uint8_t crc = static_cast<std::uint8_t>(type);
    for (std::uint32_t i = 0; i < payload.size(); ++i) {
        crc = static_cast<std::uint8_t>(crc + payload[i]);
        std::int32_t b = static_cast<std::int32_t>(payload[i])
                       + static_cast<std::int32_t>(i & 0xFFu);
        if (type != 0u && (i % type) == 0u) b += static_cast<std::int32_t>(type);
        payload[i] = static_cast<std::uint8_t>(b & 0xFF);
    }
    out.insert(out.end(), payload.begin(), payload.end());
    out.push_back(crc);  // crc2 (not validated by the parser)
    return out;
}
std::string build_test_row_56(std::uint16_t item_idx, std::uint16_t life_recover,
                              std::uint16_t item_kind = 0u) {
    // 56 columns matching ItemList.bin common-row layout (D6.x field order).
    std::vector<std::string> toks(56u);
    toks[0] = std::to_string(item_idx);
    toks[1] = "HpPotion";
    toks[2] = "0";
    toks[3] = "0";
    toks[4] = std::to_string(item_kind);  // ItemKind
    for (std::size_t i = 5; i < 50; ++i) toks[i] = "0";
    for (std::size_t i = 16; i <= 20; ++i) toks[i] = "0.0";
    for (std::size_t i = 38; i <= 42; ++i) toks[i] = "0.0";
    toks[50] = std::to_string(life_recover);  // LifeRecover
    toks[51] = "0.0";
    toks[52] = "0";
    toks[53] = "0.0";
    toks[54] = "0";
    toks[55] = "1";
    std::ostringstream row;
    for (std::size_t i = 0; i < toks.size(); ++i) {
        if (i != 0) row << "\t";
        row << toks[i];
    }
    return row.str();
}
}  // namespace

TEST(MapHandlerTest, SpeechSynShopListFeedsClientThenBuySynDebitsMoney) {
    MockDbAdapter db;
    std::vector<mxh::net::Message> replies;
    mxh::server::MapHandler handler(db, 7,
        [&](mxh::net::ConnectionId, const mxh::net::Message& message) {
            replies.push_back(message);
        });
    const auto connection = mxh::net::make_connection_id(55);
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);
    ASSERT_TRUE(handler.set_player_money_for_test(123u, 20000u));

    const std::string deal_text = "1 map 2 npc 7 10 20 0 1 tab 555 10\n";
    const auto deal_path = write_temp_bin(synthesize_dealitem_bin(deal_text));
    handler.load_dealitem(deal_path.string());
    std::error_code ec_deal; std::filesystem::remove(deal_path, ec_deal);
    std::vector<std::string> tokens(56u, "0");
    tokens[0] = "555"; tokens[1] = "Potion"; tokens[5] = "12345";
    std::string row;
    for (const auto& t : tokens) { row += t; row.push_back('\t'); }
    row.push_back('\r'); row.push_back('\n');
    const auto item_path = write_temp_bin(synthesize_item_list_bin(row));
    handler.load_item_prices(item_path.string());
    std::error_code ec_item; std::filesystem::remove(item_path, ec_item);

    mxh::net::Message talk;
    talk.header.object_id = 123u;
    talk.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Npc);
    talk.header.protocol = static_cast<std::uint8_t>(mxh::proto::NpcProtocol::SpeechSyn);
    talk.payload.resize(4);
    const std::uint32_t npc_id = 7u;
    std::memcpy(talk.payload.data(), &npc_id, sizeof(npc_id));
    replies.clear();
    handler.on_message(connection, talk);
    const auto shop = std::find_if(replies.begin(), replies.end(), [](const auto& message) {
        return message.header.protocol == mxh::proto::kModernShopList;
    });
    ASSERT_NE(shop, replies.end());

    mxh::client::CInGameState state;
    state.Init(nullptr);
    mxh::net::Message ack;
    ack.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    ack.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInAck);
    ack.payload.assign(mxh::game::HERO_TOTAL_EMPTY_PAYLOAD_SIZE, 0);
    const std::uint32_t pid = 123;
    std::memcpy(ack.payload.data(), &pid, 4);
    state.on_message(mxh::net::make_connection_id(1), ack);
    state.on_message(mxh::net::make_connection_id(1), *shop);
    ASSERT_TRUE(state.shop_open());
    ASSERT_FALSE(state.shop_items().empty());
    EXPECT_EQ(state.shop_npc_id(), 7u);

    replies.clear();
    handler.on_message(connection,
                       mxh::client::make_buy_message(123u, state.shop_items()[0].item_id, 1u));
    EXPECT_EQ(handler.player_money_for_test(123u), 7655u);

    // Dealer purchases must obey the same authoritative interaction radius;
    // a client cannot keep buying after moving away from the live NPC.
    ASSERT_TRUE(handler.set_player_position_for_test(123u, 10000.0f, 10000.0f));
    replies.clear();
    handler.on_message(connection,
                       mxh::client::make_buy_message(123u, state.shop_items()[0].item_id, 1u));
    const auto buy_nack = std::find_if(replies.begin(), replies.end(), [](const auto& message) {
        return message.header.protocol ==
            static_cast<std::uint8_t>(mxh::proto::ItemProtocol::BuyNack);
    });
    ASSERT_NE(buy_nack, replies.end());
    EXPECT_EQ(handler.player_money_for_test(123u), 7655u);
}

TEST(MapHandlerTest, PickupSynClaimsNearbyGroundDropOnce) {
    MockDbAdapter db;
    std::vector<mxh::net::Message> replies;
    mxh::server::MapHandler handler(db, 7,
        [&](mxh::net::ConnectionId, const mxh::net::Message& message) {
            replies.push_back(message);
        });
    const auto connection = mxh::net::make_connection_id(55);
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);
    const auto drop = handler.create_ground_drop_for_test(50000u, 77u, 1u, 25000.0f, 25000.0f);
    ASSERT_TRUE(drop.has_value());

    const auto pickup = mxh::client::make_pickup_message(123u, drop->object_id);
    handler.on_message(connection, pickup);
    const auto ack = std::find_if(replies.begin(), replies.end(), [](const auto& message) {
        return message.header.category == static_cast<std::uint8_t>(mxh::proto::Category::Item) &&
            message.header.protocol == static_cast<std::uint8_t>(mxh::proto::ItemProtocol::PickupAck);
    });
    ASSERT_NE(ack, replies.end());
    EXPECT_EQ(handler.player_runtime_snapshot(123u)->inventory_count, 1u);
    const auto total = std::find_if(ack + 1, replies.end(), [](const auto& message) {
        return message.header.category == static_cast<std::uint8_t>(mxh::proto::Category::Item) &&
            message.header.protocol == static_cast<std::uint8_t>(mxh::proto::ItemProtocol::TotalInfoLocal);
    });
    ASSERT_NE(total, replies.end());
    EXPECT_EQ(total->header.object_id, 123u);
    ASSERT_EQ(total->payload.size(), sizeof(mxh::game::ItemTotalInfo));
    mxh::game::ItemTotalInfo items{};
    std::memcpy(&items, total->payload.data(), sizeof(items));
    EXPECT_NE(items.Inventory[0].dwDBIdx, drop->object_id);
    EXPECT_GE(items.Inventory[0].dwDBIdx, 700000u);
    EXPECT_EQ(items.Inventory[0].wIconIdx, 77u);
    EXPECT_EQ(items.Inventory[0].ItemParam, 1u);

    replies.clear();
    handler.on_message(connection, pickup);
    const auto nack = std::find_if(replies.begin(), replies.end(), [](const auto& message) {
        return message.header.protocol ==
            static_cast<std::uint8_t>(mxh::proto::ItemProtocol::PickupNack);
    });
    ASSERT_NE(nack, replies.end());
    EXPECT_EQ(replies.size(), 1u);
    EXPECT_EQ(handler.player_runtime_snapshot(123u)->inventory_count, 1u);
}

TEST(MapHandlerTest, PickupDistanceBoundaryAndFullInventoryDoNotPublishFalseUpdates) {
    for (const auto distance : {500.0f, 501.0f}) {
        MockDbAdapter db;
        ReplySpy reply;
        MapHandler handler(db, 7, make_reply_spy(reply));
        const auto connection = mxh::net::make_connection_id(55);
        mxh::net::Message enter;
        enter.header.object_id = 123;
        enter.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
        enter.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
        handler.on_message(connection, enter);
        auto drop = handler.create_ground_drop_for_test(50000, 77, 1, 25000 + distance, 25000);
        ASSERT_TRUE(drop);
        reply.messages.clear();
        handler.on_message(connection, mxh::client::make_pickup_message(123, drop->object_id));
        ASSERT_FALSE(reply.messages.empty());
        EXPECT_EQ(reply.messages.front().header.protocol, static_cast<std::uint8_t>(distance == 500
            ? mxh::proto::ItemProtocol::PickupAck : mxh::proto::ItemProtocol::PickupNack));
        EXPECT_EQ(reply.messages.size(), distance == 500 ? 2u : 1u);
        // Fill every remaining carried slot, then request one more real pickup.
        while (handler.player_runtime_snapshot(123)->inventory_count < 80) {
            auto fill = handler.create_ground_drop_for_test(50000, 77, 1, 25000, 25000);
            ASSERT_TRUE(fill);
            ASSERT_TRUE(handler.claim_ground_drop_for_test(123, fill->object_id));
        }
        auto excess = handler.create_ground_drop_for_test(50000, 77, 1, 25000, 25000);
        ASSERT_TRUE(excess);
        reply.messages.clear();
        handler.on_message(connection, mxh::client::make_pickup_message(123, excess->object_id));
        ASSERT_EQ(reply.messages.size(), 1u);
        EXPECT_EQ(reply.messages[0].header.protocol, static_cast<std::uint8_t>(mxh::proto::ItemProtocol::PickupNack));
        EXPECT_EQ(handler.player_runtime_snapshot(123)->inventory_count, 80u);
    }
}

TEST(MapHandlerTest, PickupSnapshotSurvivesDisconnectAndFreshHandlerRelogin) {
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg{};
    cfg.backend = "sqlite"; cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());
    ASSERT_TRUE(mxh::db::migrate_modern_schema(db).ok());
    ASSERT_TRUE(db.exec_multi("INSERT INTO character_info(chrid,userid,charname,sex_type,face_type,hair_type,height,width,level,map_num) "
        "VALUES(123,'123','PickupHero',0,1,1,1.0,1.0,1,7);"
        "INSERT INTO modern_player_item VALUES(123,0,4,90000,88,100,0,65535,2);").ok());
    const auto connection = mxh::net::make_connection_id(55);
    mxh::net::Message enter;
    enter.header.object_id = 123;
    enter.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    enter.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    mxh::game::ItemTotalInfo picked{};
    {
        ReplySpy reply;
        MapHandler handler(db, 7, make_reply_spy(reply));
        handler.on_message(connection, enter);
        auto drop = handler.create_ground_drop_for_test(50000, 77, 3, 25000, 25000);
        ASSERT_TRUE(drop);
        reply.messages.clear();
        handler.on_message(connection, mxh::client::make_pickup_message(123, drop->object_id));
        const auto total = std::find_if(reply.messages.begin(), reply.messages.end(), [](const auto& m) {
            return m.header.category == static_cast<std::uint8_t>(mxh::proto::Category::Item) &&
                m.header.protocol == static_cast<std::uint8_t>(mxh::proto::ItemProtocol::TotalInfoLocal);
        });
        ASSERT_NE(total, reply.messages.end());
        ASSERT_EQ(total->payload.size(), sizeof(picked));
        std::memcpy(&picked, total->payload.data(), sizeof(picked));
        EXPECT_NE(picked.Inventory[0].dwDBIdx, drop->object_id);
        EXPECT_GT(picked.Inventory[0].dwDBIdx, 90000u);
        EXPECT_EQ(picked.Inventory[0].ItemParam, 3u);
        handler.on_disconnect(connection, mxh::net::NetError::Disconnected);
        ASSERT_FALSE(handler.is_draining());
    }
    ReplySpy reply;
    MapHandler reconnected(db, 7, make_reply_spy(reply));
    reconnected.on_message(connection, enter);
    const auto game = std::find_if(reply.messages.begin(), reply.messages.end(), [](const auto& m) {
        return m.header.category == static_cast<std::uint8_t>(mxh::proto::Category::UserConn) &&
            m.header.protocol == static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInAck);
    });
    ASSERT_NE(game, reply.messages.end());
    ASSERT_GE(game->payload.size(), mxh::game::HERO_TOTAL_ITEM_OFFSET + sizeof(picked));
    mxh::game::ItemTotalInfo restored{};
    std::memcpy(&restored, game->payload.data() + mxh::game::HERO_TOTAL_ITEM_OFFSET, sizeof(restored));
    EXPECT_EQ(restored.Inventory[0].dwDBIdx, picked.Inventory[0].dwDBIdx);
    EXPECT_EQ(restored.Inventory[0].wIconIdx, picked.Inventory[0].wIconIdx);
    EXPECT_EQ(restored.Inventory[0].ItemParam, picked.Inventory[0].ItemParam);
    EXPECT_EQ(restored.Inventory[4].dwDBIdx, 90000u);
    EXPECT_EQ(reconnected.player_runtime_snapshot(123)->inventory_count, 2u);
    // A fresh Handler reuses the transient ground ID. It must still award a
    // new persistent item identity, without replacing either saved item.
    const auto second = reconnected.create_ground_drop_for_test(50000, 99, 4, 25000, 25000);
    ASSERT_TRUE(second);
    EXPECT_EQ(second->object_id, 90000u);
    reply.messages.clear();
    reconnected.on_message(connection, mxh::client::make_pickup_message(123, second->object_id));
    ASSERT_EQ(reply.messages.size(), 2u);
    EXPECT_EQ(reply.messages.front().header.protocol, static_cast<std::uint8_t>(mxh::proto::ItemProtocol::PickupAck));
    ASSERT_EQ(reply.messages.back().payload.size(), sizeof(restored));
    std::memcpy(&restored, reply.messages.back().payload.data(), sizeof(restored));
    EXPECT_GT(restored.Inventory[1].dwDBIdx, picked.Inventory[0].dwDBIdx);
    EXPECT_EQ(restored.Inventory[1].wIconIdx, 99u);
    EXPECT_EQ(restored.Inventory[1].ItemParam, 4u);
    EXPECT_EQ(restored.Inventory[0].dwDBIdx, picked.Inventory[0].dwDBIdx);
    reconnected.on_disconnect(connection, mxh::net::NetError::Disconnected);
    ASSERT_FALSE(reconnected.is_draining());
    ReplySpy final_reply;
    MapHandler final_handler(db, 7, make_reply_spy(final_reply));
    final_handler.on_message(connection, enter);
    EXPECT_EQ(final_handler.player_runtime_snapshot(123)->inventory_count, 3u);
    const auto final_game = std::find_if(final_reply.messages.begin(), final_reply.messages.end(), [](const auto& m) {
        return m.header.category == static_cast<std::uint8_t>(mxh::proto::Category::UserConn) &&
            m.header.protocol == static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInAck);
    });
    ASSERT_NE(final_game, final_reply.messages.end());
    ASSERT_GE(final_game->payload.size(), mxh::game::HERO_TOTAL_ITEM_OFFSET + sizeof(restored));
    mxh::game::ItemTotalInfo final_items{};
    std::memcpy(&final_items, final_game->payload.data() + mxh::game::HERO_TOTAL_ITEM_OFFSET, sizeof(final_items));
    EXPECT_EQ(final_items.Inventory[1].dwDBIdx, restored.Inventory[1].dwDBIdx);
    EXPECT_EQ(final_items.Inventory[1].ItemParam, 4u);
    EXPECT_EQ(final_items.Inventory[4].dwDBIdx, 90000u);
}

TEST(MapHandlerTest, ConcurrentPickupRequestsKeepDistinctItemIdsAndClaimOnce) {
    MockDbAdapter db;
    ReplySpy reply;
    MapHandler handler(db, 7, make_reply_spy(reply));
    const auto connection = mxh::net::make_connection_id(55);
    mxh::net::Message enter;
    enter.header.object_id = 123;
    enter.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    enter.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, enter);
    std::vector<std::uint32_t> drop_ids;
    for (int n = 0; n < 12; ++n) {
        const auto drop = handler.create_ground_drop_for_test(50000, 77, 1, 25000, 25000);
        ASSERT_TRUE(drop); drop_ids.push_back(drop->object_id);
    }
    reply.messages.clear();
    std::vector<std::thread> callers;
    for (const auto id : drop_ids)
        for (int repeat = 0; repeat < 2; ++repeat)
            callers.emplace_back([&, id] { handler.on_message(connection, mxh::client::make_pickup_message(123, id)); });
    for (auto& caller : callers) caller.join();
    int successes = 0, rejections = 0, snapshots = 0;
    mxh::game::ItemTotalInfo final_items{};
    for (const auto& m : reply.messages) {
        if (m.header.protocol == static_cast<std::uint8_t>(mxh::proto::ItemProtocol::PickupAck)) ++successes;
        if (m.header.protocol == static_cast<std::uint8_t>(mxh::proto::ItemProtocol::PickupNack)) ++rejections;
        if (m.header.protocol == static_cast<std::uint8_t>(mxh::proto::ItemProtocol::TotalInfoLocal)) {
            ++snapshots; ASSERT_EQ(m.payload.size(), sizeof(final_items));
            std::memcpy(&final_items, m.payload.data(), sizeof(final_items));
        }
    }
    EXPECT_EQ(successes, 12); EXPECT_EQ(rejections, 12); EXPECT_EQ(snapshots, 12);
    std::set<std::uint32_t> ids;
    for (const auto& item : final_items.Inventory) if (item.dwDBIdx) ids.insert(item.dwDBIdx);
    EXPECT_EQ(ids.size(), 12u);
    EXPECT_EQ(handler.player_runtime_snapshot(123)->inventory_count, 12u);
}

TEST(MapHandlerTest, SpeechSynRejectsLiveNpcOutsideInteractionRange) {
    MockDbAdapter db;
    std::vector<mxh::net::Message> replies;
    MapHandler handler(db, 7,
        [&](mxh::net::ConnectionId, const mxh::net::Message& message) {
            replies.push_back(message);
        });
    const auto deal_path = write_temp_bin(
        synthesize_dealitem_bin("7 map 2 npc 7 10 20 0 1 tab 555 10\n"));
    handler.load_dealitem(deal_path.string());
    std::error_code ignored;
    std::filesystem::remove(deal_path, ignored);

    const auto connection = mxh::net::make_connection_id(55);
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);
    ASSERT_TRUE(handler.set_player_position_for_test(123u, 10000.0f, 10000.0f));

    mxh::net::Message talk;
    talk.header.object_id = 123u;
    talk.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Npc);
    talk.header.protocol = static_cast<std::uint8_t>(mxh::proto::NpcProtocol::SpeechSyn);
    talk.payload.resize(4, 0);
    const std::uint32_t npc_id = 7u;
    std::memcpy(talk.payload.data(), &npc_id, sizeof(npc_id));
    replies.clear();
    handler.on_message(connection, talk);
    ASSERT_FALSE(replies.empty());
    EXPECT_EQ(replies.front().header.protocol,
              static_cast<std::uint8_t>(mxh::proto::NpcProtocol::SpeechNack));
    EXPECT_EQ(replies.front().header.category,
              static_cast<std::uint8_t>(mxh::proto::Category::Npc));

    // A live map must also reject ids that are not among its spawned NPCs;
    // otherwise a forged speech packet could advance a quest or resolve a
    // dealer catalog without a real world target.
    replies.clear();
    const std::uint32_t unknown_npc_id = 999u;
    std::memcpy(talk.payload.data(), &unknown_npc_id, sizeof(unknown_npc_id));
    handler.on_message(connection, talk);
    ASSERT_FALSE(replies.empty());
    EXPECT_EQ(replies.front().header.protocol,
              static_cast<std::uint8_t>(mxh::proto::NpcProtocol::SpeechNack));
}

static mxh::server::FixedTileMap clear_movement_fixture() {
    std::vector<std::uint8_t> bytes(8 + 32 * 32 * 2);
    bytes[0] = 32; bytes[4] = 32;
    std::string error;
    return *mxh::server::FixedTileMap::decode(bytes, error);
}

TEST(MapHandlerTest, LightnessCatalogLoadsOriginalResourceAndCannotChangeDuringSession) {
    MockDbAdapter db;
    ReplySpy reply;
    MapHandler handler(db,10,make_reply_spy(reply));
    const auto path=std::filesystem::path(MXH_SOURCE_DIR)/"data/PlayDH/Resource/KyungGongInfo.bin";
    std::string error;
    ASSERT_TRUE(handler.load_kyunggong_catalog(path,error))<<error;
    ASSERT_TRUE(handler.kyunggong_info(2602));
    EXPECT_FLOAT_EQ(handler.kyunggong_info(2602)->speed,900);
    EXPECT_FALSE(handler.load_kyunggong_catalog(path.parent_path()/"missing-lightness-fixture.bin",error));
    EXPECT_FLOAT_EQ(handler.kyunggong_info(2602)->speed,900);
    mxh::net::Message enter;
    enter.header.object_id=123;
    enter.header.category=static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    enter.header.protocol=static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(mxh::net::make_connection_id(55),enter);
    EXPECT_FALSE(handler.load_kyunggong_catalog(path,error));
    EXPECT_EQ(error,"cannot replace lightness data with active players");
    EXPECT_FLOAT_EQ(handler.kyunggong_info(2602)->speed,900);
}

TEST(MapHandlerTest, ExcessiveClientMoveJumpIsCorrected) {
    MockDbAdapter db;
    std::vector<mxh::net::Message> replies;
    MapHandler handler(db, 7,
        [&](mxh::net::ConnectionId, const mxh::net::Message& message) {
            replies.push_back(message);
        });
    ASSERT_TRUE(handler.install_fixed_tiles(clear_movement_fixture()));
    const auto connection = mxh::net::make_connection_id(55);
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);
    ASSERT_TRUE(handler.set_player_position_for_test(123u, 1000.0f, 1000.0f));

    mxh::net::Message move;
    move.header.object_id = 123u;
    move.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Move);
    move.header.protocol = static_cast<std::uint8_t>(mxh::proto::MoveProtocol::OneTarget);
    move.payload.resize(4);
    const std::uint16_t x = 20000u;
    const std::uint16_t z = 20000u;
    std::memcpy(move.payload.data(), &x, sizeof(x));
    std::memcpy(move.payload.data() + 2, &z, sizeof(z));
    handler.on_message(connection, move);

    const auto correction = std::find_if(replies.begin(), replies.end(), [](const auto& message) {
        return message.header.protocol ==
            static_cast<std::uint8_t>(mxh::proto::MoveProtocol::Correction);
    });
    ASSERT_NE(correction, replies.end());
    ASSERT_EQ(correction->payload.size(), 4u);
    std::uint16_t corrected_x = 0;
    std::uint16_t corrected_z = 0;
    std::memcpy(&corrected_x, correction->payload.data(), sizeof(corrected_x));
    std::memcpy(&corrected_z, correction->payload.data() + 2, sizeof(corrected_z));
    EXPECT_EQ(corrected_x, 1000u);
    EXPECT_EQ(corrected_z, 1000u);
    const auto snapshot = handler.player_runtime_snapshot(123u);
    ASSERT_TRUE(snapshot.has_value());
    EXPECT_FLOAT_EQ(snapshot->pos_x, 1000.0f);
    EXPECT_FLOAT_EQ(snapshot->pos_z, 1000.0f);
}

TEST(MapHandlerTest, MovementFailsClosedWithoutTilesAndRejectsBlockedEndpoints) {
    for (bool install : {false, true}) {
        MockDbAdapter db;
        std::vector<mxh::net::Message> replies;
        MapHandler handler(db, 10, [&](mxh::net::ConnectionId, const mxh::net::Message& m) { replies.push_back(m); });
        if (install) {
            std::vector<std::uint8_t> bytes(8 + 4 * 4 * 2);
            bytes[0] = 4; bytes[4] = 4; bytes[8 + (1 * 4 + 1) * 2] = 1;
            std::string error;
            auto tiles = mxh::server::FixedTileMap::decode(bytes, error);
            ASSERT_TRUE(tiles);
            ASSERT_TRUE(handler.install_fixed_tiles(std::move(*tiles)));
        }
        const auto connection = mxh::net::make_connection_id(55);
        mxh::net::Message enter;
        enter.header.object_id = 123;
        enter.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
        enter.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
        handler.on_message(connection, enter);
        ASSERT_TRUE(handler.set_player_position_for_test(123, 1, 1));
        EXPECT_FALSE(handler.install_fixed_tiles(clear_movement_fixture()));
        for (auto protocol : {mxh::proto::MoveProtocol::OneTarget}) {
            replies.clear();
            handler.on_message(connection, mxh::client::make_move_message(123, protocol, 51, 51));
            ASSERT_EQ(replies.size(), 1);
            EXPECT_EQ(replies[0].header.protocol, static_cast<std::uint8_t>(mxh::proto::MoveProtocol::Correction));
            auto state = handler.player_runtime_snapshot(123); ASSERT_TRUE(state);
            EXPECT_FLOAT_EQ(state->pos_x, 1); EXPECT_FLOAT_EQ(state->pos_z, 1);
        }
        replies.clear();
        handler.on_message(mxh::net::make_connection_id(99), mxh::client::make_move_message(123, mxh::proto::MoveProtocol::Target, 51, 51));
        EXPECT_TRUE(replies.empty());
        auto malformed = mxh::client::make_move_message(123, mxh::proto::MoveProtocol::Target, 51, 51);
        malformed.payload.push_back(0);
        handler.on_message(connection, malformed);
        EXPECT_TRUE(replies.empty());
        for (auto protocol : {mxh::proto::MoveProtocol::Warp, mxh::proto::MoveProtocol::Correction, mxh::proto::MoveProtocol::Init})
            handler.on_message(connection, mxh::client::make_move_message(123, protocol, 101, 101));
        EXPECT_TRUE(replies.empty());
        EXPECT_FLOAT_EQ(handler.player_runtime_snapshot(123)->pos_x, 1);
    }
}

TEST(MapHandlerTest, MovementIsSentOncePerMultiplexedAgentConnection) {
    MockDbAdapter db;
    std::vector<std::pair<std::uint64_t, mxh::net::Message>> delivered;
    MapHandler handler(db, 10, [&](mxh::net::ConnectionId id, const mxh::net::Message& msg) {
        delivered.emplace_back(id.value, msg);
    });
    ASSERT_TRUE(handler.install_fixed_tiles(clear_movement_fixture()));
    mxh::net::Message enter;
    enter.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    enter.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    const auto shared = mxh::net::make_connection_id(55);
    for (auto player : {123u, 456u}) {
        enter.header.object_id = player;
        handler.on_message(shared, enter);
        ASSERT_TRUE(handler.set_player_position_for_test(player, 1000, 1000));
    }
    enter.header.object_id = 789u;
    handler.on_message(mxh::net::make_connection_id(56), enter);
    for (auto protocol : {mxh::proto::MoveProtocol::OneTarget, mxh::proto::MoveProtocol::Stop}) {
        delivered.clear();
        const auto move = mxh::client::make_move_message(123, protocol, 1100, 1100);
        handler.on_message(shared, move);
        ASSERT_EQ(delivered.size(), 2u);
        std::vector<std::uint64_t> destinations;
        for (const auto& item : delivered) {
            destinations.push_back(item.first);
            EXPECT_EQ(item.second.payload, move.payload);
            EXPECT_EQ(item.second.header.protocol, move.header.protocol);
        }
        std::sort(destinations.begin(), destinations.end());
        EXPECT_EQ(destinations, (std::vector<std::uint64_t>{55, 56}));
    }
}

TEST(MapHandlerTest, ZeroLifeMovementCannotRewritePositionAndRecoveryIsNotSticky) {
    MockDbAdapter db;
    std::vector<std::pair<std::uint64_t,mxh::net::Message>> delivered;
    MapHandler handler(db,10,[&](mxh::net::ConnectionId id,const mxh::net::Message& m) { delivered.emplace_back(id.value,m); });
    ASSERT_TRUE(handler.install_fixed_tiles(clear_movement_fixture()));
    const auto owner=mxh::net::make_connection_id(55);
    mxh::net::Message enter;
    enter.header.category=static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    enter.header.protocol=static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    enter.header.object_id=123; handler.on_message(owner,enter);
    enter.header.object_id=456; handler.on_message(mxh::net::make_connection_id(56),enter);
    ASSERT_TRUE(handler.set_player_position_for_test(123,1000,1000));
    ASSERT_TRUE(handler.set_player_vitals_for_test(123,0,0));
    for(auto protocol : {mxh::proto::MoveProtocol::OneTarget,mxh::proto::MoveProtocol::Target,mxh::proto::MoveProtocol::Stop}) {
        delivered.clear();
        handler.on_message(owner,mxh::client::make_move_message(123,protocol,1100,1100));
        ASSERT_EQ(delivered.size(),1);
        EXPECT_EQ(delivered[0].first,55);
        EXPECT_EQ(delivered[0].second.header.protocol,static_cast<std::uint8_t>(mxh::proto::MoveProtocol::Correction));
        auto state=handler.player_runtime_snapshot(123); ASSERT_TRUE(state);
        EXPECT_FLOAT_EQ(state->pos_x,1000); EXPECT_FLOAT_EQ(state->pos_z,1000);
    }
    ASSERT_TRUE(handler.set_player_vitals_for_test(123,100,0));
    delivered.clear();
    handler.on_message(owner,mxh::client::make_move_message(123,mxh::proto::MoveProtocol::OneTarget,1100,1100));
    EXPECT_EQ(delivered.size(),2);
    EXPECT_FLOAT_EQ(handler.player_runtime_snapshot(123)->pos_x,1100);
}

TEST(MapHandlerTest, TimedPositionCacheAdvancesAndLegacyResetCancelsTrajectory) {
    std::uint64_t now=100;
    MockDbAdapter db;
    ReplySpy reply;
    MapHandler handler(db,10,make_reply_spy(reply));
    ASSERT_TRUE(handler.set_movement_clock_for_test([&]{return now;}));
    ASSERT_TRUE(handler.install_fixed_tiles(clear_movement_fixture()));
    const auto owner=mxh::net::make_connection_id(55);
    mxh::net::Message enter;
    enter.header.object_id=123;
    enter.header.category=static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    enter.header.protocol=static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(owner,enter);
    EXPECT_FALSE(handler.set_movement_clock_for_test([&]{return now;}));
    ASSERT_TRUE(handler.set_player_position_for_test(123,1000,1000));
    ASSERT_TRUE(handler.start_player_trajectory_for_test(123,1400,1000,400));
    now=600;
    EXPECT_FLOAT_EQ(handler.player_runtime_snapshot(123)->pos_x,1200);
    handler.on_message(owner,mxh::client::make_move_message(123,mxh::proto::MoveProtocol::Stop,1250,1000));
    now=900;
    EXPECT_FLOAT_EQ(handler.player_runtime_snapshot(123)->pos_x,1250);
    ASSERT_TRUE(handler.start_player_trajectory_for_test(123,1450,1000,400));
    now=1150;
    EXPECT_FLOAT_EQ(handler.player_runtime_snapshot(123)->pos_x,1350);
    ASSERT_TRUE(handler.set_player_vitals_for_test(123,0,0));
    now=1200;
    EXPECT_FLOAT_EQ(handler.player_runtime_snapshot(123)->pos_x,1350);
    now=1250;
    EXPECT_FLOAT_EQ(handler.player_runtime_snapshot(123)->pos_x,1350);
    EXPECT_FALSE(handler.set_player_position_for_test(123,-1,1000));
    EXPECT_FLOAT_EQ(handler.player_runtime_snapshot(123)->pos_x,1350);
}

TEST(MapHandlerTest, PickupUsesMaterializedPositionWithoutSnapshotSideEffects) {
    std::uint64_t now=0;
    MockDbAdapter db;
    std::vector<mxh::net::Message> replies;
    MapHandler handler(db,10,[&](mxh::net::ConnectionId,const mxh::net::Message& m){replies.push_back(m);});
    ASSERT_TRUE(handler.set_movement_clock_for_test([&]{return now;}));
    const auto owner=mxh::net::make_connection_id(55);
    mxh::net::Message enter;
    enter.header.object_id=123;
    enter.header.category=static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    enter.header.protocol=static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(owner,enter);
    ASSERT_TRUE(handler.set_player_position_for_test(123,1000,1000));
    const auto drop=handler.create_ground_drop_for_test(50000,77,1,2000,1000); ASSERT_TRUE(drop);
    ASSERT_TRUE(handler.start_player_trajectory_for_test(123,2000,1000,400));
    for(const auto time : {1000u,1250u}) {
        now=time; replies.clear();
        handler.on_message(owner,mxh::client::make_pickup_message(123,drop->object_id));
        const auto expected=time==1000?mxh::proto::ItemProtocol::PickupNack:mxh::proto::ItemProtocol::PickupAck;
        EXPECT_TRUE(std::any_of(replies.begin(),replies.end(),[&](const auto& reply){
            return reply.header.category==static_cast<std::uint8_t>(mxh::proto::Category::Item) &&
                   reply.header.protocol==static_cast<std::uint8_t>(expected);
        }));
    }
    EXPECT_FLOAT_EQ(handler.player_runtime_snapshot(123)->pos_x,1500);
    EXPECT_EQ(handler.player_runtime_snapshot(123)->inventory_count,1);
}

TEST(MapHandlerTest, GroundDropCanBeClaimedExactlyOnce) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    const auto connection = mxh::net::make_connection_id(55);
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);
    const auto drop = handler.create_ground_drop_for_test(50000u, 77u, 3u, 10.0f, 20.0f);
    ASSERT_TRUE(drop.has_value());
    ASSERT_TRUE(handler.claim_ground_drop_for_test(123u, drop->object_id));
    EXPECT_FALSE(handler.claim_ground_drop_for_test(123u, drop->object_id));
    const auto snapshot = handler.player_runtime_snapshot(123u);
    ASSERT_TRUE(snapshot.has_value());
    EXPECT_EQ(snapshot->inventory_count, 1u);
}

TEST(MapHandlerTest, MonsterDeathCreatesNotifiesAndClaimsGroundDrop) {
    MockDbAdapter db;
    std::vector<mxh::net::Message> replies;
    mxh::server::MapHandler handler(db, 7,
        [&](mxh::net::ConnectionId, const mxh::net::Message& message) {
            replies.push_back(message);
        });
    const auto connection = mxh::net::make_connection_id(55);
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);

    mxh::server::DropTable table;
    table.drop_id = 9u;
    table.monster_kind = 77u;
    table.entries.push_back(mxh::server::DropItemEntry{501u, 10000u, 1u, 1u});
    handler.register_drop_table(table);

    mxh::game::MonsterInstance monster;
    monster.object_id = 88000u;
    monster.monster_kind = 77u;
    monster.map_num = 7u;
    monster.max_life = 20u;
    monster.current_life = 20u;
    monster.drop_item_id = 9u;
    monster.drop_item_ratio = 100u;
    monster.pos_x = 10.0f;
    monster.pos_z = 20.0f;
    ASSERT_TRUE(handler.add_monster_instance(monster));

    EXPECT_FALSE(handler.apply_monster_damage(123u, monster.object_id, 19u, 0u).has_value());
    const auto drop = handler.apply_monster_damage(123u, monster.object_id, 1u, 0u);
    ASSERT_TRUE(drop.has_value());
    EXPECT_EQ(drop->source_monster_id, monster.object_id);
    EXPECT_EQ(drop->item_id, 501u);
    EXPECT_EQ(drop->count, 1u);
    EXPECT_FLOAT_EQ(drop->pos_x, 10.0f);
    EXPECT_FLOAT_EQ(drop->pos_z, 20.0f);

    const auto notify = std::find_if(replies.begin(), replies.end(), [](const auto& message) {
        return message.header.category == static_cast<std::uint8_t>(mxh::proto::Category::Item) &&
            message.header.protocol == static_cast<std::uint8_t>(mxh::proto::ItemProtocol::MonsterObtainNotify);
    });
    ASSERT_NE(notify, replies.end());
    EXPECT_EQ(notify->header.object_id, 123u);
    ASSERT_EQ(notify->payload.size(), 20u);

    EXPECT_FALSE(handler.apply_monster_damage(123u, monster.object_id, 1u, 0u).has_value());
    ASSERT_TRUE(handler.claim_ground_drop_for_test(123u, drop->object_id));
    EXPECT_FALSE(handler.claim_ground_drop_for_test(123u, drop->object_id));
    ASSERT_TRUE(handler.player_runtime_snapshot(123u).has_value());
    EXPECT_EQ(handler.player_runtime_snapshot(123u)->inventory_count, 1u);
}

TEST(MapHandlerTest, MonsterDeathAwardsExperienceAndSendsLegacyNotification) {
    MockDbAdapter db; std::vector<mxh::net::Message> replies;
    db.backend = "mssql_odbc";
    mxh::server::MapHandler handler(db, 7, [&](mxh::net::ConnectionId, const auto& message) { replies.push_back(message); });
    const std::string exp_path = std::string(MXH_SOURCE_DIR) + "/data/PlayDH/Resource/CharacterExpPoint.bin";
    handler.load_experience_curve(exp_path);
    mxh::net::Message game_in; game_in.header.object_id = 123; game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(mxh::net::make_connection_id(55), game_in);
    mxh::game::MonsterInstance monster; monster.object_id=88001; monster.monster_kind=77; monster.max_life=1; monster.current_life=1; monster.exp_reward=123;
    ASSERT_TRUE(handler.add_monster_instance(monster));
    (void)handler.apply_monster_damage(123, monster.object_id, 1, 99);
    const auto exp = std::find_if(replies.begin(), replies.end(), [](const auto& message) { return message.header.category == 3u && message.header.protocol == 13u; });
    ASSERT_NE(exp, replies.end()); ASSERT_EQ(exp->payload.size(), 9u);
    // Current level-one table threshold is 20: 123 reward leaves 103 at level 2.
    std::int64_t amount = 0; std::memcpy(&amount, exp->payload.data(), sizeof(amount)); EXPECT_EQ(amount, 103);
    EXPECT_EQ(exp->payload[8],0u);
    const auto level = std::find_if(replies.begin(), replies.end(), [](const auto& message) {
        return message.header.category == 3u && message.header.protocol == 19u;
    });
    ASSERT_NE(level, replies.end());
    ASSERT_EQ(level->payload.size(), 18u);
    const std::uint16_t level_value = static_cast<std::uint16_t>(
        level->payload[0] | (static_cast<std::uint16_t>(level->payload[1]) << 8));
    std::int64_t level_exp = 0, level_max = 0;
    std::memcpy(&level_exp, level->payload.data() + 2, sizeof(level_exp));
    std::memcpy(&level_max, level->payload.data() + 10, sizeof(level_max));
    EXPECT_EQ(level_value, 2u);
    EXPECT_EQ(level_exp, 103);
    EXPECT_GT(level_max, 0);
    const auto state_write = std::find_if(db.executed_sql.begin(), db.executed_sql.end(),
        [](const auto& sql) { return sql.find("MERGE modern_player_state") != std::string::npos; });
    ASSERT_NE(state_write, db.executed_sql.end());
    EXPECT_EQ(state_write->find("ON CONFLICT"), std::string::npos);
}

TEST(MapHandlerTest, MonsterExperiencePersistsAndLevelRestoresOnRelogin) {
    auto db = mxh::db::make_adapter("sqlite"); mxh::db::ConnectionConfig cfg; cfg.path = ":memory:"; ASSERT_TRUE(db->connect(cfg).ok());
    ASSERT_TRUE(db->execute("CREATE TABLE character_info(chrid INTEGER PRIMARY KEY,charname TEXT,sex_type INTEGER,face_type INTEGER,hair_type INTEGER,height REAL,width REAL,level INTEGER,map_num INTEGER)").ok());
    ASSERT_TRUE(db->execute("INSERT INTO character_info VALUES(123,'Hero',0,1,1,1,1,1,7)").ok());
    ASSERT_TRUE(db->execute("CREATE TABLE modern_player_state(player_id INTEGER PRIMARY KEY,money INTEGER DEFAULT 0,level INTEGER DEFAULT 1,exp INTEGER DEFAULT 0,updated_at TEXT)").ok());
    ReplySpy reply;
    {
        mxh::server::MapHandler handler(*db, 7, make_reply_spy(reply));
        handler.load_experience_curve(std::string(MXH_SOURCE_DIR) + "/data/PlayDH/Resource/CharacterExpPoint.bin");
        mxh::net::Message in; in.header.object_id=123; in.header.category=static_cast<std::uint8_t>(mxh::proto::Category::UserConn); in.header.protocol=static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn); handler.on_message(mxh::net::make_connection_id(55), in);
        ASSERT_TRUE(handler.set_player_money_for_test(123,4321));
        mxh::game::MonsterInstance monster; monster.object_id=99001; monster.max_life=1; monster.current_life=1; monster.exp_reward=1000000; ASSERT_TRUE(handler.add_monster_instance(monster));
        (void)handler.apply_monster_damage(123, monster.object_id, 1, 99);
        mxh::db::ResultSet money;
        ASSERT_TRUE(db->query("SELECT money FROM modern_player_state WHERE player_id=123",money).ok());
        ASSERT_EQ(money.rows.size(),1u);
        EXPECT_EQ(std::get<std::int64_t>(money.rows[0][0]),4321);
        // Existing-row experience updates must not overwrite independently saved money.
        ASSERT_TRUE(db->execute("UPDATE modern_player_state SET money=5432 WHERE player_id=123").ok());
        monster.object_id=99003;
        ASSERT_TRUE(handler.add_monster_instance(monster));
        (void)handler.apply_monster_damage(123,monster.object_id,1,99);
        money.rows.clear();
        ASSERT_TRUE(db->query("SELECT money FROM modern_player_state WHERE player_id=123",money).ok());
        ASSERT_EQ(money.rows.size(),1u);
        EXPECT_EQ(std::get<std::int64_t>(money.rows[0][0]),5432);
        const auto notice=std::find_if(reply.messages.rbegin(),reply.messages.rend(),[](const auto& message){
            return message.header.category==3 && message.header.protocol==13;
        });
        ASSERT_NE(notice,reply.messages.rend());
        ASSERT_EQ(notice->payload.size(),9u);
        std::int64_t current_exp=0;
        std::memcpy(&current_exp,notice->payload.data(),8);
        EXPECT_EQ(current_exp,handler.player_runtime_snapshot(123)->level_exp);
        EXPECT_EQ(notice->payload[8],0u);
    }
    mxh::db::ResultSet saved; ASSERT_TRUE(db->query("SELECT level,exp FROM modern_player_state WHERE player_id=123", saved).ok()); ASSERT_EQ(saved.rows.size(),1u);
    const auto saved_level=std::get<std::int64_t>(saved.rows[0][0]); EXPECT_GT(saved_level,1);
    mxh::server::MapHandler restored(*db, 7, make_reply_spy(reply)); mxh::net::Message in; in.header.object_id=123; in.header.category=static_cast<std::uint8_t>(mxh::proto::Category::UserConn); in.header.protocol=static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn); restored.on_message(mxh::net::make_connection_id(56), in);
    const auto snapshot=restored.player_runtime_snapshot(123); ASSERT_TRUE(snapshot); EXPECT_EQ(snapshot->level, saved_level);
}

TEST(MapHandlerTest, MaximumLevelKillDoesNotWriteOrNotifyExperience) {
    auto db=mxh::db::make_adapter("sqlite"); mxh::db::ConnectionConfig cfg; cfg.path=":memory:";
    ASSERT_TRUE(db->connect(cfg).ok());
    ASSERT_TRUE(db->execute("CREATE TABLE character_info(chrid INTEGER PRIMARY KEY,charname TEXT,sex_type INTEGER,face_type INTEGER,hair_type INTEGER,height REAL,width REAL,level INTEGER,map_num INTEGER)").ok());
    ASSERT_TRUE(db->execute("INSERT INTO character_info VALUES(123,'Hero',0,1,1,1,1,121,7)").ok());
    ASSERT_TRUE(db->execute("CREATE TABLE modern_player_state(player_id INTEGER PRIMARY KEY,money INTEGER DEFAULT 0,level INTEGER DEFAULT 1,exp INTEGER DEFAULT 0,updated_at TEXT)").ok());
    ASSERT_TRUE(db->execute("INSERT INTO modern_player_state VALUES(123,4321,121,77,'baseline')").ok());
    std::vector<mxh::net::Message> replies;
    mxh::server::MapHandler handler(*db,7,[&](mxh::net::ConnectionId,const auto& message){replies.push_back(message);});
    handler.load_experience_curve(std::string(MXH_SOURCE_DIR)+"/data/PlayDH/Resource/CharacterExpPoint.bin");
    mxh::net::Message in; in.header.object_id=123;
    in.header.category=static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    in.header.protocol=static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message({55},in);
    ASSERT_TRUE(handler.player_runtime_snapshot(123));
    ASSERT_EQ(handler.player_runtime_snapshot(123)->level,121);
    ASSERT_TRUE(db->execute("CREATE TRIGGER reject_max_exp BEFORE INSERT ON modern_player_state BEGIN SELECT RAISE(ABORT,'unexpected maximum-level write'); END").ok());
    mxh::game::MonsterInstance monster; monster.object_id=99004; monster.max_life=1;
    monster.current_life=1; monster.exp_reward=1000;
    ASSERT_TRUE(handler.add_monster_instance(monster));
    (void)handler.apply_monster_damage(123,99004,1,99);
    EXPECT_FALSE(handler.is_draining());
    EXPECT_EQ(handler.player_runtime_snapshot(123)->level_exp,77u);
    EXPECT_TRUE(std::none_of(replies.begin(),replies.end(),[](const auto& message){
        return message.header.category==3 && message.header.protocol==13;
    }));
}

TEST(MapHandlerTest, FailedExperienceWriteDoesNotPublishRewardAndDrains) {
    auto db=mxh::db::make_adapter("sqlite"); mxh::db::ConnectionConfig cfg; cfg.path=":memory:";
    ASSERT_TRUE(db->connect(cfg).ok());
    ASSERT_TRUE(db->execute("CREATE TABLE character_info(chrid INTEGER PRIMARY KEY,charname TEXT,sex_type INTEGER,face_type INTEGER,hair_type INTEGER,height REAL,width REAL,level INTEGER,map_num INTEGER)").ok());
    ASSERT_TRUE(db->execute("INSERT INTO character_info VALUES(123,'Hero',0,1,1,1,1,1,7)").ok());
    ASSERT_TRUE(db->execute("CREATE TABLE modern_player_state(player_id INTEGER PRIMARY KEY,money INTEGER DEFAULT 0,level INTEGER DEFAULT 1,exp INTEGER DEFAULT 0,updated_at TEXT)").ok());
    std::vector<mxh::net::Message> replies;
    mxh::server::MapHandler handler(*db,7,[&](mxh::net::ConnectionId,const auto& message){replies.push_back(message);});
    handler.load_experience_curve(std::string(MXH_SOURCE_DIR)+"/data/PlayDH/Resource/CharacterExpPoint.bin");
    mxh::net::Message in; in.header.object_id=123;
    in.header.category=static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    in.header.protocol=static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message({55},in);
    const auto before=handler.player_runtime_snapshot(123); ASSERT_TRUE(before);
    ASSERT_TRUE(db->execute("CREATE TRIGGER reject_experience BEFORE INSERT ON modern_player_state BEGIN SELECT RAISE(ABORT,'injected reward failure'); END").ok());
    mxh::game::MonsterInstance monster; monster.object_id=99002; monster.max_life=1;
    monster.current_life=1; monster.exp_reward=1000000;
    ASSERT_TRUE(handler.add_monster_instance(monster));
    (void)handler.apply_monster_damage(123,99002,1,99);
    EXPECT_TRUE(handler.is_draining());
    const auto reply_count=replies.size();
    bool accepted=true;
    (void)handler.apply_monster_damage(123,99002,1,99,{},&accepted);
    EXPECT_FALSE(accepted);
    EXPECT_EQ(replies.size(),reply_count);
    const auto after=handler.player_runtime_snapshot(123); ASSERT_TRUE(after);
    EXPECT_EQ(after->level,before->level);
    EXPECT_EQ(after->level_exp,before->level_exp);
    EXPECT_EQ(after->total_exp,before->total_exp);
    EXPECT_TRUE(std::none_of(replies.begin(),replies.end(),[](const auto& message){
        return message.header.category==3 && message.header.protocol==13;
    }));
}

TEST(MapHandlerTest, UseConsumesActorItemAndUpdatesVitals) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    const auto connection = mxh::net::make_connection_id(55);
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);
    ASSERT_TRUE(handler.set_player_vitals_for_test(123u, 1u, 1u));
    ASSERT_TRUE(handler.add_player_item_for_test(123u, mxh::game::make_item(9002u, 1u, 0u)));
    ASSERT_EQ(handler.player_runtime_snapshot(123u)->inventory_count, 1u);
    mxh::net::Message use;
    use.header.object_id = 123u;
    use.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    use.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::UseSyn);
    use.payload.resize(2);
    const std::uint16_t pos = 0u;
    std::memcpy(use.payload.data(), &pos, sizeof(pos));
    handler.on_message(connection, use);
    const auto snapshot = handler.player_runtime_snapshot(123u);
    ASSERT_TRUE(snapshot.has_value());
    EXPECT_EQ(snapshot->inventory_count, 0u);
    EXPECT_GT(snapshot->current_hp, 1u);
    EXPECT_GE(reply.call_count.load(), 2);
}

TEST(MapHandlerTest, DiscardRemovesAuthoritativeInventoryItem) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    const auto connection = mxh::net::make_connection_id(55);
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);
    ASSERT_TRUE(handler.add_player_item_for_test(123u, mxh::game::make_item(9001u, 77u, 0u)));
    ASSERT_EQ(handler.player_runtime_snapshot(123u)->inventory_count, 1u);
    mxh::net::Message discard;
    discard.header.object_id = 123u;
    discard.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    discard.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::DiscardSyn);
    discard.payload.resize(2);
    const std::uint16_t pos = 0u;
    std::memcpy(discard.payload.data(), &pos, sizeof(pos));
    handler.on_message(connection, discard);
    EXPECT_EQ(handler.player_runtime_snapshot(123u)->inventory_count, 0u);
}

TEST(MapHandlerTest, MoveUpdatesAuthoritativePlayerPosition) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    ASSERT_TRUE(handler.install_fixed_tiles(clear_movement_fixture()));
    const auto connection = mxh::net::make_connection_id(55);
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);
    ASSERT_TRUE(handler.set_player_position_for_test(123u, 0.0f, 0.0f));

    mxh::net::Message move;
    move.header.object_id = 123u;
    move.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Move);
    move.header.protocol = static_cast<std::uint8_t>(mxh::proto::MoveProtocol::Target);
    move.payload.resize(4);
    const std::uint16_t x = 321u;
    const std::uint16_t z = 654u;
    std::memcpy(move.payload.data(), &x, sizeof(x));
    std::memcpy(move.payload.data() + 2, &z, sizeof(z));
    handler.on_message(connection, move);

    const auto snapshot = handler.player_runtime_snapshot(123u);
    ASSERT_TRUE(snapshot.has_value());
    EXPECT_FLOAT_EQ(snapshot->pos_x, 321.0f);
    EXPECT_FLOAT_EQ(snapshot->pos_z, 654.0f);
}
TEST(MapHandlerTest, ActorVitalsAreAuthoritativeForStateObservation) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(mxh::net::make_connection_id(55), game_in);
    ASSERT_TRUE(handler.set_player_vitals_for_test(123u, 1u, 1u));
    const auto snapshot = handler.player_runtime_snapshot(123u);
    ASSERT_TRUE(snapshot.has_value());
    EXPECT_EQ(snapshot->current_hp, 1u);
    EXPECT_EQ(snapshot->current_mp, 1u);
    EXPECT_LE(snapshot->current_hp, snapshot->max_hp);
    EXPECT_LE(snapshot->current_mp, snapshot->max_mp);
}
TEST(MapHandlerTest, RuntimeSnapshotRemovedOnDisconnect) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    const auto connection = mxh::net::make_connection_id(55);
    handler.on_message(connection, game_in);
    handler.on_disconnect(connection, mxh::net::NetError::Disconnected);
    EXPECT_EQ(handler.player_runtime_count(), 0u);
    EXPECT_FALSE(handler.player_runtime_snapshot(123u).has_value());
}
TEST(MapHandlerTest, InstalledAiGroupsDriveMonsterSpawns) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    mxh::server::AiGroupList groups;
    mxh::server::AiGroupDefinition group;
    group.group_id = 42u;
    mxh::server::AiSpawnDefinition first;
    first.object_kind = 3u;
    first.monster_kind = 101u;
    first.pos_x = 10.0f;
    first.pos_z = 20.0f;
    mxh::server::AiSpawnDefinition second = first;
    second.monster_kind = 102u;
    second.pos_x = 30.0f;
    group.spawns = {first, second};
    groups.groups.push_back(group);

    EXPECT_EQ(handler.install_ai_groups(groups), 2u);
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(mxh::net::make_connection_id(55), game_in);

    EXPECT_EQ(handler.monster_count_for_test(), groups.spawn_count());
}

void check_monster_groups(const std::filesystem::path& path) {
    ASSERT_TRUE(std::filesystem::exists(path)) << path.string();
    auto& ai = mxh::server::AISystem::instance();
    struct RestoreEmptyAi {
        mxh::server::AISystem& ai;
        ~RestoreEmptyAi() { ai.load_ai_group_list(); }
    } restore{ai};
    ASSERT_TRUE(ai.load_ai_group_list(path));
    EXPECT_EQ(ai.group_list().spawn_count(), 228u);

    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 10, make_reply_spy(reply));
    handler.set_allow_dev_monster_fallback(false);
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(mxh::net::make_connection_id(55), game_in);
    EXPECT_EQ(handler.monster_count_for_test(), 228u);
    const auto live = handler.live_monster_count_for_test();
    EXPECT_EQ(live, 228u);

    std::size_t monster_adds = 0;
    const mxh::net::Message* first_add = nullptr;
    for (const auto& message : reply.messages) {
        if (message.header.category ==
                static_cast<std::uint8_t>(mxh::proto::Category::UserConn) &&
            message.header.protocol ==
                static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::MonsterAdd)) {
            if (first_add == nullptr) first_add = &message;
            ++monster_adds;
        }
    }
    EXPECT_EQ(monster_adds, live);
    ASSERT_NE(first_add, nullptr);
    ASSERT_GE(first_add->payload.size(), 64u);

    mxh::client::CInGameState state;
    state.on_message({}, *first_add);
    ASSERT_EQ(state.monsters().size(), 1u);
    EXPECT_EQ(state.monsters().front().object_id, first_add->header.object_id);
    const auto kind = static_cast<std::uint16_t>(
        first_add->payload[43] | (static_cast<std::uint16_t>(first_add->payload[44]) << 8));
    const auto pos_x = static_cast<std::uint16_t>(
        first_add->payload[49] | (static_cast<std::uint16_t>(first_add->payload[50]) << 8));
    const auto pos_z = static_cast<std::uint16_t>(
        first_add->payload[51] | (static_cast<std::uint16_t>(first_add->payload[52]) << 8));
    EXPECT_EQ(state.monsters().front().monster_kind, kind);
    EXPECT_EQ(state.monsters().front().position_x, pos_x);
    EXPECT_EQ(state.monsters().front().position_z, pos_z);

    mxh::gx::EntityScene scene;
    mxh::gx::WorldSnapshot snapshot;
    snapshot.entities.push_back(mxh::gx::SceneEntity{
        state.monsters().front().object_id,
        state.monsters().front().monster_kind,
        static_cast<float>(state.monsters().front().position_x),
        0.0f,
        static_cast<float>(state.monsters().front().position_z),
        mxh::gx::SceneEntityType::Monster});
    scene.synchronize(snapshot);
    EXPECT_GE(scene.placeholderCount() + scene.loadedModelCount(), 1u);
    if (!scene.placeholders().empty()) {
        EXPECT_GT(scene.placeholders().front().radius, 0.0f);
    }
}

TEST(MapHandlerTest, SyntheticMonster10BinSpawnsAllGroups) {
    namespace fixture = mxh::test::resources;
    const fixture::TemporaryBin monsters(fixture::legacy(fixture::monster_text()));
    check_monster_groups(monsters.path);
}

TEST(MapHandlerReferenceTest, DISABLED_RecoveredMonster10BinSpawnsAllGroups) {
    check_monster_groups(std::filesystem::path(MXH_SOURCE_DIR).parent_path() /
        "reference/legacy-source/4dddd9a6/SWorking/Resource/Server/Monster_10.bin");
}

TEST(AgentHandlerTest, ForwardFromMapMonsterAddReachesRegisteredClient) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    handler.register_session(
        mxh::net::make_connection_id(7),
        /*user_id=*/3, /*char_id=*/100003, /*map_num=*/10);

    mxh::net::Message msg;
    msg.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    msg.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::MonsterAdd);
    msg.header.object_id = 50000u;
    msg.payload.assign(64, 0);
    msg.payload[0] = 0x50;
    msg.payload[43] = 105;
    handler.forward_from_map(mxh::net::make_connection_id(42), msg);

    ASSERT_EQ(reply.call_count.load(), 1);
    EXPECT_EQ(reply.last_id.value, 7u);
    EXPECT_EQ(reply.last_message.header.protocol,
              static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::MonsterAdd));
    EXPECT_EQ(reply.last_message.header.object_id, 50000u);
}

TEST(MapHandlerTest, GameInOnConnectionZeroSendsMonsterAdds) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    mxh::server::AiGroupList groups;
    mxh::server::AiGroupDefinition group;
    group.group_id = 1u;
    mxh::server::AiSpawnDefinition spawn;
    spawn.monster_kind = 105u;
    spawn.pos_x = 100.0f;
    spawn.pos_z = 200.0f;
    group.spawns = {spawn, spawn};
    groups.groups.push_back(group);
    ASSERT_EQ(handler.install_ai_groups(groups), 2u);

    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(mxh::net::make_connection_id(0), game_in);

    std::size_t monster_adds = 0;
    for (const auto& message : reply.messages) {
        if (message.header.category ==
                static_cast<std::uint8_t>(mxh::proto::Category::UserConn) &&
            message.header.protocol ==
                static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::MonsterAdd)) {
            ++monster_adds;
        }
    }
    EXPECT_EQ(monster_adds, 2u);
}

TEST(MapHandlerTest, ProductionModeKeepsValidEmptyRegenEmpty) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 12, make_reply_spy(reply));
    handler.set_allow_dev_monster_fallback(false);

    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(mxh::net::make_connection_id(55), game_in);

    EXPECT_EQ(handler.monster_count_for_test(), 0u);
}

TEST(MapHandlerTest, SkillDoesNotApplyToReplacedCasterOrTargetSession) {
    for (const auto skill_id : {1u, 3u}) {
        for (const auto replaced : {111u, 222u}) {
            MockDbAdapter db;
            MapHandler* active = nullptr;
            bool replace_on_ack = false;
            unsigned results = 0;
            const auto conn = mxh::net::make_connection_id(55);
            auto enter = [](std::uint32_t player) {
                mxh::net::Message m;
                m.header.object_id = player;
                m.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
                m.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
                return m;
            };
            MapHandler handler(db,10,[&](mxh::net::ConnectionId,const mxh::net::Message& reply) {
                if (reply.header.category != static_cast<std::uint8_t>(mxh::proto::Category::Skill)) return;
                if (reply.header.protocol == static_cast<std::uint8_t>(mxh::proto::SkillProtocol::SingleResult)) ++results;
                if (replace_on_ack && reply.header.protocol == static_cast<std::uint8_t>(mxh::proto::SkillProtocol::StartAck)) {
                    replace_on_ack = false;
                    // Same ID and same multiplexed connection: only session identity changes.
                    active->on_message(conn,enter(replaced));
                    EXPECT_TRUE(active->set_player_position_for_test(replaced,1000,1000));
                    EXPECT_TRUE(active->set_player_vitals_for_test(222,1,50));
                }
            });
            active = &handler;
            handler.on_message(conn,enter(111));
            handler.on_message(conn,enter(222));
            ASSERT_TRUE(handler.set_player_position_for_test(111,1000,1000));
            ASSERT_TRUE(handler.set_player_position_for_test(222,1000,1000));
            mxh::net::Message cast;
            cast.header.object_id = 111;
            cast.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Skill);
            cast.header.protocol = static_cast<std::uint8_t>(mxh::proto::SkillProtocol::StartSyn);
            cast.payload.resize(16);
            const std::uint32_t target = 222;
            std::memcpy(cast.payload.data(),&skill_id,4);
            std::memcpy(cast.payload.data()+4,&target,4);
            replace_on_ack = true;
            handler.on_message(conn,cast);
            EXPECT_FALSE(replace_on_ack);
            EXPECT_EQ(results,0u);
            EXPECT_EQ(handler.player_runtime_snapshot(222)->current_hp,1u);
            // The current session can still use the same ordinary skill path.
            handler.on_message(conn,cast);
            EXPECT_GT(results,0u);
        }
    }
}

void check_penalty_profile_switch(const std::filesystem::path& current,
                                  const std::filesystem::path& legacy) {
    MockDbAdapter db;
    ReplySpy reply;
    MapHandler handler(db,10,make_reply_spy(reply));
    ASSERT_TRUE(handler.load_exp_penalty(current, "playdh-current"));
    ASSERT_TRUE(handler.exp_penalties());
    EXPECT_FLOAT_EQ(handler.exp_penalties()->at(48).present_percent, 2.4f);
    EXPECT_FALSE(handler.load_exp_penalty(current, "sworking-2008-reference"));
    EXPECT_FALSE(handler.exp_penalties());
    ASSERT_TRUE(handler.load_exp_penalty(legacy, "sworking-2008-reference"));
    EXPECT_FLOAT_EQ(handler.exp_penalties()->at(48).login_percent, 1.9f);
    EXPECT_FALSE(handler.load_exp_penalty(legacy, "unknown-profile"));
    EXPECT_FALSE(handler.exp_penalties());
    ASSERT_TRUE(handler.load_exp_penalty(current, "playdh-current"));
    EXPECT_FALSE(handler.load_exp_penalty(current / "missing.bin", "playdh-current"));
    EXPECT_FALSE(handler.exp_penalties());
}

TEST(MapHandlerTest, ExpPenaltyLoadRequiresMatchingProfileAndClearsStaleTable) {
    namespace fixture = mxh::test::resources;
    const fixture::TemporaryBin current(fixture::current(fixture::penalty_text, fixture::penalty_key));
    const fixture::TemporaryBin legacy(fixture::legacy(fixture::penalty_text));
    check_penalty_profile_switch(current.path, legacy.path);
}

TEST(MapHandlerReferenceTest, DISABLED_ExpPenaltyLoadRequiresMatchingProfileAndClearsStaleTable) {
    check_penalty_profile_switch(std::filesystem::path(MXH_SOURCE_DIR) / "data/PlayDH/Resource/Server/ExpPenalty.bin",
        std::filesystem::path(MXH_SOURCE_DIR).parent_path() /
        "reference/legacy-source/4dddd9a6/SWorking/Resource/Server/ExpPenalty.bin");
}

TEST(MapHandlerTest, SkillMpReservationUpdatesRuntimeAndRejectsForeignConnection) {
    MockDbAdapter db;
    ReplySpy reply;
    MapHandler handler(db,10,make_reply_spy(reply));
    const auto owner = mxh::net::make_connection_id(55);
    mxh::net::Message enter;
    enter.header.object_id = 111;
    enter.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    enter.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(owner,enter);
    mxh::net::Message cast;
    cast.header.object_id = 111;
    cast.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Skill);
    cast.header.protocol = static_cast<std::uint8_t>(mxh::proto::SkillProtocol::StartSyn);
    cast.payload.resize(16);
    const std::uint32_t skill_id = 3;
    std::memcpy(cast.payload.data(),&skill_id,4);
    const auto before = handler.player_runtime_snapshot(111)->current_mp;
    reply.messages.clear();
    handler.on_message(mxh::net::make_connection_id(99),cast);
    EXPECT_TRUE(reply.messages.empty());
    EXPECT_EQ(handler.player_runtime_snapshot(111)->current_mp,before);
    handler.on_message(owner,cast);
    EXPECT_EQ(handler.player_runtime_snapshot(111)->current_mp,before-10);
    ASSERT_TRUE(handler.set_player_vitals_for_test(111,100,9));
    reply.messages.clear();
    handler.on_message(owner,cast);
    ASSERT_EQ(reply.messages.size(),1u);
    EXPECT_EQ(reply.messages.front().header.protocol,
        static_cast<std::uint8_t>(mxh::proto::SkillProtocol::StartNack));
    EXPECT_EQ(handler.player_runtime_snapshot(111)->current_mp,9u);
    ASSERT_TRUE(handler.set_player_vitals_for_test(111,0,50));
    reply.messages.clear();
    handler.on_message(owner,cast);
    ASSERT_EQ(reply.messages.size(),1u);
    EXPECT_EQ(reply.messages.front().header.protocol,
        static_cast<std::uint8_t>(mxh::proto::SkillProtocol::StartNack));
    EXPECT_EQ(reply.messages.front().payload, std::vector<std::uint8_t>{3});
    EXPECT_EQ(handler.player_runtime_snapshot(111)->current_mp,50u);
}

TEST(MapHandlerTest, SkillMonsterCommitRejectsReplacedCasterAndRemoveCanReenter) {
    MockDbAdapter db;
    MapHandler* active = nullptr;
    bool replace_on_ack = false;
    bool reenter_on_remove = false;
    unsigned results = 0;
    const auto conn = mxh::net::make_connection_id(55);
    const auto observer = mxh::net::make_connection_id(66);
    mxh::net::Message enter;
    enter.header.object_id = 111;
    enter.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    enter.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    mxh::net::Message cast;
    cast.header.object_id = 111;
    cast.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Skill);
    cast.header.protocol = static_cast<std::uint8_t>(mxh::proto::SkillProtocol::StartSyn);
    cast.payload.resize(16);
    const std::uint32_t skill = 1, target = 88000;
    std::memcpy(cast.payload.data(),&skill,4);
    std::memcpy(cast.payload.data()+4,&target,4);
    MapHandler handler(db,10,[&](mxh::net::ConnectionId,const mxh::net::Message& reply) {
        if (reply.header.category != static_cast<std::uint8_t>(mxh::proto::Category::Skill)) return;
        const auto proto = static_cast<mxh::proto::SkillProtocol>(reply.header.protocol);
        if (proto == mxh::proto::SkillProtocol::SingleResult) ++results;
        if (replace_on_ack && proto == mxh::proto::SkillProtocol::StartAck) {
            replace_on_ack = false;
            active->on_message(conn,enter);
            EXPECT_TRUE(active->set_player_position_for_test(111,1000,1000));
        }
        if (reenter_on_remove && proto == mxh::proto::SkillProtocol::SkillObjectRemove) {
            reenter_on_remove = false;
            active->on_message(conn,cast);
        }
    });
    active = &handler;
    handler.on_message(conn,enter);
    auto other = enter; other.header.object_id = 222;
    handler.on_message(observer,other);
    ASSERT_TRUE(handler.set_player_position_for_test(111,1000,1000));
    mxh::game::MonsterInstance monster;
    monster.object_id = target; monster.monster_kind = 77;
    monster.max_life = 100000; monster.current_life = 100000;
    monster.pos_x = 1000; monster.pos_z = 1000;
    ASSERT_TRUE(handler.add_monster_instance(monster));
    replace_on_ack = true;
    handler.on_message(conn,cast);
    EXPECT_FALSE(replace_on_ack);
    EXPECT_EQ(results,0u);
    reenter_on_remove = true;
    handler.on_message(conn,cast);
    EXPECT_FALSE(reenter_on_remove);
    EXPECT_EQ(results,2u);
}

TEST(MapHandlerTest, ProductionModeDisablesHardcodedSkillFallback) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 12, make_reply_spy(reply));
    ASSERT_NE(handler.find_skill(1u), nullptr);

    handler.set_allow_dev_skill_fallback(false);

    EXPECT_EQ(handler.find_skill(1u), nullptr);
}

TEST(MapHandlerTest, OnDisconnectDoesNotCrash) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, /*map_num=*/7, make_reply_spy(reply));
    handler.on_disconnect({}, mxh::net::NetError::Disconnected);
    EXPECT_EQ(reply.call_count.load(), 0);
}

namespace {
// Synthesize a minimal valid ItemList.bin (1:1 with legacy MHFile packed-text format)
// containing a single row.  Used by MapHandler R-8 tests below.
std::vector<std::uint8_t> synthesize_itemlist_bin(const std::string& single_row_text) {
    constexpr std::uint8_t type_byte = 1u;
    std::vector<std::uint8_t> decoded;
    decoded.reserve(single_row_text.size() + 2);
    for (char c : single_row_text) decoded.push_back(static_cast<std::uint8_t>(c));
    decoded.push_back(13);  // CR
    decoded.push_back(10);  // LF
    std::vector<std::uint8_t> encoded;
    encoded.reserve(decoded.size());
    for (std::size_t i = 0; i < decoded.size(); ++i) {
        const int adjusted = static_cast<int>(decoded[i])
            + static_cast<int>(i & 0xFFu)
            + static_cast<int>(type_byte);
        encoded.push_back(static_cast<std::uint8_t>(adjusted & 0xFFu));
    }
    std::uint8_t crc1 = type_byte;
    for (std::uint8_t b : encoded) crc1 = static_cast<std::uint8_t>(crc1 + b);
    const std::uint32_t file_size = static_cast<std::uint32_t>(encoded.size());
    std::vector<std::uint8_t> out;
    out.reserve(12 + 2 + encoded.size());
    auto append_u32 = [&out](std::uint32_t v) {
        for (int i = 0; i < 4; ++i) out.push_back(static_cast<std::uint8_t>((v >> (i * 8)) & 0xFFu));
    };
    append_u32(1u);  // dwVersion
    append_u32(1u);  // dwType
    append_u32(file_size);
    out.push_back(crc1);
    out.insert(out.end(), encoded.begin(), encoded.end());
    out.push_back(0u);  // crc2 - unused by load_item_list
    return out;
}
}  // anonymous namespace

TEST(MapHandlerTest, ItemManagerEmptyByDefault) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    EXPECT_EQ(handler.item_manager_for_test().size(), 0u);
    EXPECT_FALSE(handler.item_manager_for_test().exists(1u));
}

TEST(MapHandlerTest, LoadItemListPopulatesManagerFromBin) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    ASSERT_EQ(handler.item_manager_for_test().size(), 0u);
    const auto row_text = build_test_row_56(1u, 999u);
    const auto bin = synthesize_itemlist_bin(row_text);
    const auto path = write_temp_bin(bin);
    handler.load_item_list(path.string());
    std::error_code ec;
    std::filesystem::remove(path, ec);
    EXPECT_EQ(handler.item_manager_for_test().size(), 1u);
    EXPECT_TRUE(handler.item_manager_for_test().exists(1u));
    mxh::game::ItemInfo info{};
    ASSERT_TRUE(handler.item_manager_for_test().try_get(1u, info));
    EXPECT_EQ(info.ItemIdx, 1u);
    EXPECT_EQ(info.LifeRecover, 999u);
    EXPECT_FLOAT_EQ(info.LifeRecoverRate, 0.0f);
}

TEST(MapHandlerTest, UseAckReadsHpDeltaFromLoadedItemListBin) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    // Step 1: load a 1-row ItemList.bin with ItemIdx=1, LifeRecover=999.
    const auto row_text = build_test_row_56(1u, 999u);
    const auto bin = synthesize_itemlist_bin(row_text);
    const auto path = write_temp_bin(bin);
    handler.load_item_list(path.string());
    std::error_code ec;
    std::filesystem::remove(path, ec);
    // Step 2: enter map, set HP=1 MP=1, add inventory item with wIconIdx=1.
    const auto connection = mxh::net::make_connection_id(55);
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);
    ASSERT_TRUE(handler.set_player_vitals_for_test(123u, 1u, 1u));
    ASSERT_TRUE(handler.add_player_item_for_test(
        123u, mxh::game::make_item(9002u, 1u, 0u)));
    // Step 3: fire UseSyn, capture UseAck reply.
    mxh::net::Message use;
    use.header.object_id = 123u;
    use.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    use.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::UseSyn);
    use.payload.resize(2);
    const std::uint16_t pos = 0u;
    std::memcpy(use.payload.data(), &pos, sizeof(pos));
    handler.on_message(connection, use);
    // UseAck is the last Item-category reply; GameInAck was sent earlier.
    ASSERT_GE(reply.messages.size(), 1u);
    const mxh::net::Message* ack = nullptr;
    for (const auto& m : reply.messages) {
        if (m.header.category == static_cast<std::uint8_t>(mxh::proto::Category::Item)
            && m.header.protocol == static_cast<std::uint8_t>(mxh::proto::ItemProtocol::UseAck)) {
            ack = &m;
            break;
        }
    }
    ASSERT_NE(ack, nullptr);
    ASSERT_EQ(ack->payload.size(), 20u);
    // Layout (LE): u16 pos | u16 wIconIdx | i32 hp_delta | i32 mp_delta
    //              | u32 new_hp | u32 new_mp
    std::int32_t hp_delta = 0;
    std::memcpy(&hp_delta, ack->payload.data() + 4, 4);
    EXPECT_EQ(hp_delta, 999);
}

TEST(MapHandlerTest, UseSynConsumesItemAndAdvancesUseItemQuest) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    const auto connection = mxh::net::make_connection_id(55);
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);

    const auto item_path = write_temp_bin(synthesize_itemlist_bin(build_test_row_56(1u, 999u)));
    handler.load_item_list(item_path.string());
    std::error_code ec;
    std::filesystem::remove(item_path, ec);

    const auto quest_path = write_temp_bin(synthesize_dealitem_bin(
        "$QUEST 88 { $SUBQUEST 0 { #TRIGGER @USEITEM 1 1 *ENDQUEST 0 } }"));
    handler.load_quest_script(quest_path.string());
    std::filesystem::remove(quest_path, ec);

    mxh::net::Message start;
    start.header.object_id = 123u;
    start.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Quest);
    start.header.protocol = static_cast<std::uint8_t>(mxh::proto::QuestProtocol::StartSyn);
    start.payload.resize(2);
    const std::uint16_t quest_id = 88u;
    std::memcpy(start.payload.data(), &quest_id, sizeof(quest_id));
    handler.on_message(connection, start);
    ASSERT_EQ(handler.player_quest_count_for_test(123u), 1u);
    ASSERT_TRUE(handler.add_player_item_for_test(123u, mxh::game::make_item(9002u, 1u, 0u)));

    mxh::net::Message use;
    use.header.object_id = 123u;
    use.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    use.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::UseSyn);
    use.payload.resize(2);
    const std::uint16_t slot = 0u;
    std::memcpy(use.payload.data(), &slot, sizeof(slot));
    handler.on_message(connection, use);

    const auto progress = handler.quest_progress_for_test(123u, quest_id);
    ASSERT_TRUE(progress.has_value());
    ASSERT_EQ(progress->subs.size(), 1u);
    EXPECT_EQ(progress->subs[0].count, 1u);
    EXPECT_EQ(progress->state, mxh::server::QuestState::Complete);
    EXPECT_EQ(handler.player_runtime_snapshot(123u)->inventory_count, 0u);
    const auto inventory = std::find_if(reply.messages.begin(), reply.messages.end(), [](const auto& message) {
        return message.header.category == static_cast<std::uint8_t>(mxh::proto::Category::Item) &&
            message.header.protocol == static_cast<std::uint8_t>(mxh::proto::ItemProtocol::TotalInfoLocal);
    });
    ASSERT_NE(inventory, reply.messages.end());
    ASSERT_EQ(inventory->payload.size(), sizeof(mxh::game::ItemTotalInfo));
    mxh::game::ItemTotalInfo refreshed{};
    std::memcpy(&refreshed, inventory->payload.data(), sizeof(refreshed));
    EXPECT_TRUE(mxh::game::is_empty_slot(refreshed.Inventory[0]));
}

TEST(MapHandlerTest, QuestNpcTalkUsesSemanticNpcAndQuestContext) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    const auto connection = mxh::net::make_connection_id(55);
    const auto npc_path = write_temp_bin(synthesize_dealitem_bin(
        "7\t25\tQuestNpc\t38\t100\t100\t0\t\r\n"));
    handler.load_quest_npcs(npc_path.string());
    std::error_code ec;
    std::filesystem::remove(npc_path, ec);
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);
    ASSERT_TRUE(handler.set_player_position_for_test(123u, 1000.0f, 1000.0f));

    const auto quest_path = write_temp_bin(synthesize_dealitem_bin(
        "$QUEST 88 { $SUBQUEST 0 { #TRIGGER @TALKTONPC 38 88 *ENDQUEST 0 } }\n"
        "$QUEST 89 { $SUBQUEST 0 { #TRIGGER @TALKTONPC 38 89 *ENDQUEST 0 } }"));
    handler.load_quest_script(quest_path.string());
    std::filesystem::remove(quest_path, ec);
    for (const std::uint16_t quest_id : {std::uint16_t{88u}, std::uint16_t{89u}}) {
        mxh::net::Message start;
        start.header.object_id = 123u;
        start.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Quest);
        start.header.protocol = static_cast<std::uint8_t>(mxh::proto::QuestProtocol::StartSyn);
        start.payload.resize(2);
        std::memcpy(start.payload.data(), &quest_id, sizeof(quest_id));
        handler.on_message(connection, start);
    }

    mxh::net::Message talk;
    talk.header.object_id = 123u;
    talk.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Quest);
    talk.header.protocol = static_cast<std::uint8_t>(mxh::proto::QuestProtocol::NpcTalk);
    talk.payload.resize(4);
    const std::uint16_t npc = 38u;
    const std::uint16_t selected_quest = 89u;
    std::memcpy(talk.payload.data(), &npc, sizeof(npc));
    std::memcpy(talk.payload.data() + 2, &selected_quest, sizeof(selected_quest));
    reply.messages.clear();
    handler.on_message(connection, talk);

    EXPECT_EQ(handler.quest_progress_for_test(123u, 89u)->subs[0].count, 0u);
    ASSERT_FALSE(reply.messages.empty());
    EXPECT_EQ(reply.messages.front().header.protocol,
              static_cast<std::uint8_t>(mxh::proto::QuestProtocol::NpcTalkNack));

    ASSERT_TRUE(handler.set_player_position_for_test(123u, 100.0f, 100.0f));
    reply.messages.clear();
    handler.on_message(connection, talk);

    EXPECT_EQ(handler.quest_progress_for_test(123u, 88u)->subs[0].count, 0u);
    EXPECT_EQ(handler.quest_progress_for_test(123u, 89u)->subs[0].count, 1u);
    ASSERT_FALSE(reply.messages.empty());
    EXPECT_EQ(reply.messages.front().header.protocol,
              static_cast<std::uint8_t>(mxh::proto::QuestProtocol::NpcTalkAck));

    // A failed durable write must restore the in-memory quest state.
    const std::uint16_t first_quest = 88u;
    std::memcpy(talk.payload.data() + 2, &first_quest, sizeof(first_quest));
    db.fail_write_matching = "modern_player_quest_log";
    reply.messages.clear();
    handler.on_message(connection, talk);
    EXPECT_EQ(handler.quest_progress_for_test(123u, 88u)->subs[0].count, 0u);
    ASSERT_FALSE(reply.messages.empty());
    EXPECT_EQ(reply.messages.front().header.protocol,
              static_cast<std::uint8_t>(mxh::proto::QuestProtocol::NpcTalkNack));
}


// D1.3 call-site: synthesize a valid SkillList.bin row (1:1 with the 150-token
// legacy SKILLINFO layout).  Returns a tab-separated string.
std::string build_test_skill_row_150(std::uint16_t skill_idx,
                                std::uint16_t weapon_kind,
                                std::uint16_t first_phy) {
    std::vector<std::string> toks(150u, "0");
    toks[0] = std::to_string(skill_idx);
    toks[1] = "TestSkill";
    toks[7] = std::to_string(weapon_kind);
    toks[8] = "230";
    toks[16] = "0.0";
    toks[20] = "1";
    toks[25] = "1";
    toks[29] = "1000";
    toks[69] = "12";
    toks[70] = std::to_string(first_phy);
    for (std::size_t i = 71; i < 82; ++i) toks[i] = "0";
    for (std::size_t seg = 1; seg < 6; ++seg) {
        toks[69 + seg * 13] = "0";
    }
    toks[149] = "10001";
    std::ostringstream row;
    for (std::size_t i = 0; i < toks.size(); ++i) {
        if (i != 0) row << "	";
        row << toks[i];
    }
    return row.str();
}

TEST(MapHandlerTest, SkillManagerEmptyByDefault) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    EXPECT_EQ(handler.skill_manager_for_test().size(), 0u);
    EXPECT_FALSE(handler.skill_manager_for_test().exists(1u));
}

TEST(MapHandlerTest, LoadSkillListPopulatesManagerFromBin) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    ASSERT_EQ(handler.skill_manager_for_test().size(), 0u);
    const auto row_text = build_test_skill_row_150(1u, 2u, 777u);
    const auto bin = synthesize_itemlist_bin(row_text);
    const auto path = write_temp_bin(bin);
    handler.load_skill_list(path.string());
    std::error_code ec;
    std::filesystem::remove(path, ec);
    ASSERT_EQ(handler.skill_manager_for_test().size(), 1u);
    ASSERT_TRUE(handler.skill_manager_for_test().exists(1u));
    mxh::game::SkillInfo info{};
    ASSERT_TRUE(handler.skill_manager_for_test().try_get(1u, info));
    EXPECT_EQ(info.SkillIdx, 1u);
    EXPECT_EQ(info.WeaponKind, 2u);
    EXPECT_FLOAT_EQ(info.UpPhyAttack[0], 777.0f);
    const mxh::game::SkillInfo* found = handler.find_skill(1u);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->SkillIdx, 1u);
    EXPECT_EQ(found->WeaponKind, 2u);
}

TEST(MapHandlerTest, MonsterAiAttackUsesSkillDelayAndPublishesAuthoritativeDamage) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 10, make_reply_spy(reply));
    const auto skill_path = write_temp_bin(synthesize_itemlist_bin(
        build_test_skill_row_150(42u, 0u, 10u)));
    handler.load_skill_list(skill_path.string());
    std::error_code ec;
    std::filesystem::remove(skill_path, ec);

    const auto connection = mxh::net::make_connection_id(55);
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);
    ASSERT_TRUE(handler.set_player_position_for_test(123u, 0.0f, 0.0f));
    ASSERT_TRUE(handler.set_player_vitals_for_test(123u, 3u, 0u));

    mxh::game::MonsterInstance monster;
    monster.object_id = 88042u;
    monster.monster_kind = 73u;
    monster.map_num = 10u;
    monster.level = 1u;
    monster.max_life = monster.current_life = 100u;
    monster.attack_min = monster.attack_max = 10u;
    monster.attack_count = 1u;
    monster.attack_skills[0] = 42u;
    monster.search_period_ms = 1u;
    monster.ai_state = mxh::game::MonsterAIState::Attack;
    monster.target_object_id = 123u;
    ASSERT_TRUE(handler.add_monster_instance(monster));

    std::uint64_t now = 1000u; // DelayTime is 1000: strict boundary must not attack.
    handler.set_monster_ai_clock_for_test([&] { return now; });
    handler.set_monster_ai_rng_for_test([] { return 0u; });
    reply.messages.clear();
    handler.tick_monster_ai();
    EXPECT_TRUE(reply.messages.empty());

    now = 1001u;
    handler.tick_monster_ai();
    const auto has = [&](mxh::proto::Category category, std::uint8_t protocol) {
        return std::find_if(reply.messages.begin(), reply.messages.end(), [&](const auto& message) {
            return message.header.category == static_cast<std::uint8_t>(category) &&
                   message.header.protocol == protocol;
        });
    };
    const auto life = has(mxh::proto::Category::Character,
        static_cast<std::uint8_t>(mxh::proto::CharacterProtocol::LifeAck));
    ASSERT_NE(life, reply.messages.end());
    ASSERT_EQ(life->payload.size(), 4u);
    std::int32_t life_delta = 0;
    std::memcpy(&life_delta, life->payload.data(), 4u);
    EXPECT_EQ(life_delta, -3);
    EXPECT_NE(has(mxh::proto::Category::Skill,
                  static_cast<std::uint8_t>(mxh::proto::SkillProtocol::SkillObjectAdd)), reply.messages.end());
    EXPECT_NE(has(mxh::proto::Category::Skill,
                  static_cast<std::uint8_t>(mxh::proto::SkillProtocol::SingleResult)), reply.messages.end());
    EXPECT_NE(has(mxh::proto::Category::Skill,
                  static_cast<std::uint8_t>(mxh::proto::SkillProtocol::SkillObjectRemove)), reply.messages.end());
    const auto death = has(mxh::proto::Category::UserConn,
        static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::CharacterDie));
    ASSERT_NE(death, reply.messages.end());
    ASSERT_EQ(death->payload.size(), 8u);
    EXPECT_EQ(handler.player_runtime_snapshot(123u)->current_hp, 0u);

    EXPECT_EQ(handler.player_runtime_snapshot(123u)->lifecycle, mxh::server::PlayerLifecycle::Dead);

    reply.messages.clear();
    now = 3000u;
    handler.tick_monster_ai();
    EXPECT_TRUE(reply.messages.empty());
}

TEST(MapHandlerTest, LoadMonsterListRequiresAndAcceptsCanonicalPlayDh) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 10, make_reply_spy(reply));
    EXPECT_FALSE(handler.has_loaded_monster_list());
    handler.load_monster_list(
        (std::filesystem::path(MXH_SOURCE_DIR) / "data/PlayDH/Resource/MonsterList.bin").string());
    EXPECT_TRUE(handler.has_loaded_monster_list());
}

TEST(MapHandlerTest, FindSkillReadsFromLoadedSkillListBin) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    const auto row_text = build_test_skill_row_150(42u, 5u, 1234u);
    const auto bin = synthesize_itemlist_bin(row_text);
    const auto path = write_temp_bin(bin);
    handler.load_skill_list(path.string());
    std::error_code ec;
    std::filesystem::remove(path, ec);
    const mxh::game::SkillInfo* s42 = handler.find_skill(42u);
    ASSERT_NE(s42, nullptr);
    EXPECT_EQ(s42->SkillIdx, 42u);
    EXPECT_EQ(s42->WeaponKind, 5u);
    EXPECT_FLOAT_EQ(s42->UpPhyAttack[0], 1234.0f);
    const mxh::game::SkillInfo* s1 = handler.find_skill(1u);
    EXPECT_EQ(s1, nullptr);
}

namespace {
std::vector<std::uint8_t> synthesize_quest_text_bin(const std::string& text) {
    return synthesize_dealitem_bin(text);
}
}  // namespace

TEST(MapHandlerTest, LoadDealitemPopulatesCatalogFromBin) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    ASSERT_EQ(handler.dealitem_catalog_for_test().npcs.size(), 0u);
    const std::string text =
        "1 map 2 npc 7 10 20 0 1 tab 100 3 tab 101 -1\n"
        "1 map 2 npc 7 10 20 0 2 tab 200 0\n";
    const auto bin = synthesize_dealitem_bin(text);
    const auto path = write_temp_bin(bin);
    handler.load_dealitem(path.string());
    std::error_code ec;
    std::filesystem::remove(path, ec);
    EXPECT_EQ(handler.dealitem_catalog_for_test().npcs.size(), 1u);
    const auto* npc = handler.dealitem_catalog_for_test().find_npc(7);
    ASSERT_NE(npc, nullptr);
    ASSERT_EQ(npc->tabs.size(), 2u);
    EXPECT_EQ(npc->tabs[0].size(), 2u);
    EXPECT_EQ(npc->tabs[0][0].item_idx, 100u);
    EXPECT_EQ(npc->tabs[0][1].item_count, std::numeric_limits<std::uint32_t>::max());
    EXPECT_EQ(npc->tabs[1].size(), 1u);
    EXPECT_EQ(npc->tabs[1][0].item_idx, 200u);
}

TEST(MapHandlerTest, LoadQuestScriptPopulatesDefinitionsFromBin) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    ASSERT_EQ(handler.quest_definitions_for_test().quests.size(), 0u);
    const std::string text =
        "$QUEST 1 { $SUBQUEST 1 { #TRIGGER @HUNT 1 10 *ADDCOUNT 1 1 } }";
    const auto raw = synthesize_dealitem_bin(text);
    const auto path = write_temp_bin(raw);
    handler.load_quest_script(path.string());
    std::error_code ec;
    std::filesystem::remove(path, ec);
    ASSERT_EQ(handler.quest_definitions_for_test().quests.size(), 1u);
    const auto* def = handler.quest_definitions_for_test().find_quest(1u);
    ASSERT_NE(def, nullptr);
    ASSERT_EQ(def->subquests.size(), 1u);
    EXPECT_EQ(def->subquests[0].triggers.size(), 1u);
}
TEST(MapHandlerTest, BuyResolvesNearbyDealerWhenEarlierCatalogAlsoSellsItem) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    const auto path = write_temp_bin(synthesize_dealitem_bin(
        "1 map 1 npc 2 100 100 0 1 tab 555 10\n"
        "7 map 1 npc 3 25000 25000 0 1 tab 555 10\n"));
    handler.load_dealitem(path.string());
    std::error_code ec; std::filesystem::remove(path, ec);
    const auto connection = mxh::net::make_connection_id(55);
    mxh::net::Message message;
    message.header.object_id = 123u;
    message.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    message.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, message);
    ASSERT_TRUE(handler.set_player_money_for_test(123u, 1000u));
    message.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    message.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::BuySyn);
    message.payload = {0x2b, 0x02, 1, 0}; // item 555, quantity 1
    handler.on_message(connection, message);
    ASSERT_TRUE(handler.player_runtime_snapshot(123u).has_value());
    EXPECT_EQ(handler.player_runtime_snapshot(123u)->inventory_count, 1u);
    ASSERT_TRUE(handler.set_player_position_for_test(123u, 20000.0f, 20000.0f));
    handler.on_message(connection, message);
    EXPECT_EQ(handler.player_runtime_snapshot(123u)->inventory_count, 1u);
}

TEST(MapHandlerTest, RepeatedGameOutDoesNotOverwritePersistedMoneyWithZero) {
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg{};
    cfg.backend = "sqlite"; cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());
    ASSERT_TRUE(db.exec_multi("CREATE TABLE modern_player_state (player_id INTEGER PRIMARY KEY, money INTEGER NOT NULL, updated_at TEXT NOT NULL);").ok());
    ASSERT_TRUE(db.exec_multi("CREATE TABLE modern_player_item (player_id INTEGER,container INTEGER,slot INTEGER,db_idx INTEGER,item_idx INTEGER,durability INTEGER,rare_idx INTEGER,quick_position INTEGER,item_param INTEGER);").ok());
    ASSERT_TRUE(mxh::db::migrate_modern_schema(db).ok());
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    const auto connection = mxh::net::make_connection_id(55);
    mxh::net::Message message;
    message.header.object_id = 123u;
    message.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    message.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, message);
    ASSERT_TRUE(handler.set_player_money_for_test(123u, 999u));
    message.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameOutSyn);
    handler.on_message(connection, message);
    handler.on_message(connection, message);
    mxh::db::ResultSet rows;
    ASSERT_TRUE(db.query("SELECT money FROM modern_player_state WHERE player_id=123", {}, rows).ok());
    ASSERT_EQ(rows.rows.size(), 1u);
    EXPECT_EQ(std::get<std::int64_t>(rows.rows[0][0]), 999);
}

TEST(MapHandlerTest, BuySynOkArmDeductsMoneyAndInsertsInventory) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    const auto connection = mxh::net::make_connection_id(55);
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);
    ASSERT_TRUE(handler.set_player_money_for_test(123u, 1000u));
    // load a one-NPC dealitem catalog that sells item 555 for 10g each.
    const std::string deal_text = "1 map 2 npc 7 10 20 0 1 tab 555 10\n";
    const auto deal_bin = synthesize_dealitem_bin(deal_text);
    const auto deal_path = write_temp_bin(deal_bin);
    handler.load_dealitem(deal_path.string());
    std::error_code ec_deal; std::filesystem::remove(deal_path, ec_deal);
    mxh::net::Message buy;
    buy.header.object_id = 123u;
    buy.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    buy.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::BuySyn);
    buy.payload.resize(4);
    const std::uint16_t item = 555u; const std::uint16_t qty = 3u;
    std::memcpy(buy.payload.data(), &item, sizeof(item));
    std::memcpy(buy.payload.data() + 2, &qty, sizeof(qty));
    handler.on_message(connection, buy);
    EXPECT_EQ(handler.player_money_for_test(123u), 1000u); // price field not in dealitem row; total_price=0 for now
    const auto snap = handler.player_runtime_snapshot(123u);
    ASSERT_TRUE(snap.has_value());
    EXPECT_EQ(snap->inventory_count, 3u); // qty=3 -> 3 stack items

    db.fail_write_matching = "modern_player_state";
    reply.messages.clear();
    handler.on_message(connection, buy);
    ASSERT_EQ(reply.messages.size(), 1u);
    EXPECT_EQ(reply.messages[0].header.protocol,
              static_cast<std::uint8_t>(mxh::proto::ItemProtocol::BuyNack));
    EXPECT_EQ(handler.player_money_for_test(123u), 1000u);
    EXPECT_EQ(handler.player_runtime_snapshot(123u)->inventory_count, 3u);
    EXPECT_GE(db.rollback_count.load(), 1);
    EXPECT_FALSE(handler.is_draining());

    db.fail_write_matching.clear();
    db.fail_commit = true;
    reply.messages.clear();
    handler.on_message(connection, buy);
    ASSERT_EQ(reply.messages.size(), 1u);
    EXPECT_EQ(reply.messages[0].header.protocol,
              static_cast<std::uint8_t>(mxh::proto::ItemProtocol::BuyNack));
    EXPECT_EQ(handler.player_money_for_test(123u), 1000u);
    EXPECT_EQ(handler.player_runtime_snapshot(123u)->inventory_count, 3u);
    EXPECT_TRUE(handler.is_draining());
}

TEST(MapHandlerTest, SellSynUsesItemListSellPriceAndPublishesState) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    const auto connection = mxh::net::make_connection_id(55);
    const std::string deal_text = "7 map 2 npc 7 25000 25000 0 1 tab 555 10\n";
    const auto deal_path = write_temp_bin(synthesize_dealitem_bin(deal_text));
    handler.load_dealitem(deal_path.string());
    std::error_code ec_deal; std::filesystem::remove(deal_path, ec_deal);
    std::vector<std::string> tokens(56u, "0");
    tokens[0] = "555"; tokens[1] = "Potion"; tokens[5] = "0"; tokens[6] = "100";
    std::string row;
    for (const auto& token : tokens) { row += token; row.push_back('\t'); }
    row.push_back('\r'); row.push_back('\n');
    const auto item_path = write_temp_bin(synthesize_item_list_bin(row));
    handler.load_item_list(item_path.string());
    std::error_code ec_item; std::filesystem::remove(item_path, ec_item);
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);
    ASSERT_TRUE(handler.set_player_money_for_test(123u, 1000u));
    ASSERT_TRUE(handler.set_player_position_for_test(123u, 25000.0f, 25000.0f));

    mxh::net::Message buy;
    buy.header.object_id = 123u;
    buy.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    buy.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::BuySyn);
    buy.payload.resize(4);
    const std::uint16_t item = 555u, qty = 1u;
    std::memcpy(buy.payload.data(), &item, 2);
    std::memcpy(buy.payload.data() + 2, &qty, 2);
    handler.on_message(connection, buy);
    ASSERT_TRUE(handler.player_runtime_snapshot(123u).has_value());
    ASSERT_EQ(handler.player_runtime_snapshot(123u)->inventory_count, 1u);
    reply.messages.clear();

    mxh::net::Message sell;
    sell.header.object_id = 123u;
    sell.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    sell.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::SellSyn);
    sell.payload.resize(8);
    const std::uint16_t pos = 0u, dealer = 7u;
    std::memcpy(sell.payload.data(), &pos, 2);
    std::memcpy(sell.payload.data() + 2, &item, 2);
    std::memcpy(sell.payload.data() + 4, &qty, 2);
    std::memcpy(sell.payload.data() + 6, &dealer, 2);
    handler.on_message(connection, sell);

    ASSERT_GE(reply.messages.size(), 3u);
    EXPECT_EQ(reply.messages[0].header.protocol,
              static_cast<std::uint8_t>(mxh::proto::ItemProtocol::SellAck));
    EXPECT_EQ(reply.messages[1].header.protocol,
              static_cast<std::uint8_t>(mxh::proto::ItemProtocol::Money));
    EXPECT_EQ(reply.messages[2].header.protocol,
              static_cast<std::uint8_t>(mxh::proto::ItemProtocol::TotalInfoLocal));
    EXPECT_EQ(handler.player_money_for_test(123u), 1100u);
    EXPECT_EQ(handler.player_runtime_snapshot(123u)->inventory_count, 0u);

    // A failed money write must roll back the inventory transaction, restore
    // the live state and publish only a Nack. This prevents item loss with no
    // sale proceeds when either database backend rejects the second write.
    handler.on_message(connection, buy);
    ASSERT_EQ(handler.player_runtime_snapshot(123u)->inventory_count, 1u);
    const auto money_before_failure = handler.player_money_for_test(123u);
    db.fail_write_matching = "modern_player_state";
    reply.messages.clear();
    handler.on_message(connection, sell);
    ASSERT_EQ(reply.messages.size(), 1u);
    EXPECT_EQ(reply.messages[0].header.protocol,
              static_cast<std::uint8_t>(mxh::proto::ItemProtocol::SellNack));
    EXPECT_EQ(handler.player_money_for_test(123u), money_before_failure);
    EXPECT_EQ(handler.player_runtime_snapshot(123u)->inventory_count, 1u);
    EXPECT_GE(db.rollback_count.load(), 1);
    EXPECT_FALSE(handler.is_draining());

    db.fail_write_matching.clear();
    db.fail_commit = true;
    reply.messages.clear();
    handler.on_message(connection, sell);
    ASSERT_EQ(reply.messages.size(), 1u);
    EXPECT_EQ(reply.messages[0].header.protocol,
              static_cast<std::uint8_t>(mxh::proto::ItemProtocol::SellNack));
    EXPECT_EQ(handler.player_money_for_test(123u), money_before_failure);
    EXPECT_EQ(handler.player_runtime_snapshot(123u)->inventory_count, 1u);
    EXPECT_TRUE(handler.is_draining());
}

TEST(MapHandlerTest, StartSynOkArmAddsQuestToPlayerLog) {
    MockDbAdapter db;
    db.backend = "mssql_odbc";
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    const auto connection = mxh::net::make_connection_id(55);
    mxh::net::Message game_in;
    game_in.header.object_id = 456u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);
    const std::string quest_text = "$QUEST 99 { $SUBQUEST 1 { #TRIGGER @HUNT 1 10 *ADDCOUNT 1 1 } }";
    const auto qbin = synthesize_dealitem_bin(quest_text);
    const auto qpath = write_temp_bin(qbin);
    handler.load_quest_script(qpath.string());
    std::error_code ec_q; std::filesystem::remove(qpath, ec_q);
    ASSERT_EQ(handler.quest_definitions_for_test().quests.size(), 1u);
    EXPECT_EQ(handler.player_quest_count_for_test(456u), 0u);
    mxh::net::Message start;
    start.header.object_id = 456u;
    start.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Quest);
    start.header.protocol = static_cast<std::uint8_t>(mxh::proto::QuestProtocol::StartSyn);
    start.payload.resize(2);
    const std::uint16_t qid = 99u;
    std::memcpy(start.payload.data(), &qid, sizeof(qid));
    handler.on_message(connection, start);
    EXPECT_EQ(handler.player_quest_count_for_test(456u), 1u);
    const auto quest_write = std::find_if(db.executed_sql.begin(), db.executed_sql.end(),
        [](const auto& sql) { return sql.find("MERGE modern_player_quest_log") != std::string::npos; });
    ASSERT_NE(quest_write, db.executed_sql.end());
    EXPECT_EQ(quest_write->find("ON CONFLICT"), std::string::npos);
}

TEST(MapHandlerTest, StartSynRejectsLevelMismatchBeforePersistence) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    const auto connection = mxh::net::make_connection_id(55);
    mxh::net::Message game_in;
    game_in.header.object_id = 456u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);
    const auto qpath = write_temp_bin(synthesize_dealitem_bin(
        "$QUEST 173 { $SUBQUEST 0 {\n#LIMIT &LEVEL 48 53\n"
        "#TRIGGER @TALKTONPC 38 173 *ENDQUEST 0\n} }"));
    handler.load_quest_script(qpath.string());
    std::error_code ec; std::filesystem::remove(qpath, ec);
    mxh::net::Message start;
    start.header.object_id = 456u;
    start.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Quest);
    start.header.protocol = static_cast<std::uint8_t>(mxh::proto::QuestProtocol::StartSyn);
    start.payload.resize(2);
    const std::uint16_t qid = 173u;
    std::memcpy(start.payload.data(), &qid, sizeof(qid));
    reply.messages.clear();
    handler.on_message(connection, start);
    ASSERT_EQ(reply.messages.size(), 1u);
    EXPECT_EQ(reply.last_message.header.protocol,
              static_cast<std::uint8_t>(mxh::proto::QuestProtocol::StartNack));
    EXPECT_EQ(handler.player_quest_count_for_test(456u), 0u);
    EXPECT_EQ(db.begin_count, 0u);
}

TEST(MapHandlerTest, StartSynWriteFailureRestoresRuntimeAndReturnsNack) {
    MockDbAdapter db;
    db.backend = "mssql_odbc";
    db.fail_write_matching = "MERGE modern_player_quest_log";
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    const auto connection = mxh::net::make_connection_id(55);
    mxh::net::Message game_in;
    game_in.header.object_id = 456u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);
    const auto qpath = write_temp_bin(synthesize_dealitem_bin(
        "$QUEST 99 { $SUBQUEST 0 { #TRIGGER @HUNT 1 1 *ENDQUEST 0 } }"));
    handler.load_quest_script(qpath.string());
    std::error_code ec; std::filesystem::remove(qpath, ec);
    mxh::net::Message start;
    start.header.object_id = 456u;
    start.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Quest);
    start.header.protocol = static_cast<std::uint8_t>(mxh::proto::QuestProtocol::StartSyn);
    start.payload.resize(2);
    const std::uint16_t qid = 99u;
    std::memcpy(start.payload.data(), &qid, sizeof(qid));
    reply.messages.clear();
    handler.on_message(connection, start);
    ASSERT_EQ(reply.messages.size(), 1u);
    EXPECT_EQ(reply.last_message.header.protocol,
              static_cast<std::uint8_t>(mxh::proto::QuestProtocol::StartNack));
    EXPECT_EQ(handler.player_quest_count_for_test(456u), 0u);
    EXPECT_EQ(db.begin_count, 1u);
    EXPECT_EQ(db.rollback_count, 1u);
    EXPECT_FALSE(handler.is_draining());
}


// M3 D-stage: BuySyn money persistence to modern_player_state.
// End-to-end: real SqliteAdapter (in-memory) + inline CREATE TABLE +
// BuySyn Ok arm + SELECT verification.  Validates that the orchestrator
// actually hits the DB (not just a mock) and that the row is readable
// after the wire reply.
TEST(MapHandlerTest, BuySynOkArmPersistsMoneyToSqliteMemory) {
    // 1) Real in-memory SQLite.
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg{};
    cfg.backend = "sqlite";
    cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());

    // 2) Inline CREATE TABLE (same shape as the production schema in
    //    deploy/database/mx_modern_schema_mssql.sql + MoxianDbTool
    //    moxian_schema_sql()).
    auto ct = db.exec_multi(
        "CREATE TABLE modern_player_state ("
        " player_id  INTEGER PRIMARY KEY,"
        " money      INTEGER NOT NULL DEFAULT 0,"
        " level      INTEGER NOT NULL DEFAULT 1,"
        " exp        INTEGER NOT NULL DEFAULT 0,"
        " updated_at TEXT    NOT NULL"
        ");"
        "CREATE TABLE modern_player_item ("
        " player_id INTEGER NOT NULL, container INTEGER NOT NULL,"
        " slot INTEGER NOT NULL, db_idx INTEGER NOT NULL,"
        " item_idx INTEGER NOT NULL, durability INTEGER NOT NULL,"
        " rare_idx INTEGER NOT NULL, quick_position INTEGER NOT NULL,"
        " item_param INTEGER NOT NULL,"
        " PRIMARY KEY (player_id, container, slot),"
        " UNIQUE (player_id, db_idx)"
        ");");
    ASSERT_TRUE(ct.ok()) << ct.error_message;

    // 3) Build a MapHandler around the live db.  ReplySpy captures
    //    the BuyAck so we can confirm the wire shape did not change.
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    const auto connection = mxh::net::make_connection_id(55);

    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);

    ASSERT_TRUE(handler.set_player_money_for_test(123u, 1000u));

    // 4) Load a one-NPC dealitem catalog and send a BuySyn Ok path.
    const std::string deal_text = "1 map 2 npc 7 10 20 0 1 tab 555 10\n";
    const auto deal_bin = synthesize_dealitem_bin(deal_text);
    const auto deal_path = write_temp_bin(deal_bin);
    handler.load_dealitem(deal_path.string());
    std::error_code ec_deal; std::filesystem::remove(deal_path, ec_deal);

    mxh::net::Message buy;
    buy.header.object_id = 123u;
    buy.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    buy.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::BuySyn);
    buy.payload.resize(4);
    const std::uint16_t item = 555u; const std::uint16_t qty = 3u;
    std::memcpy(buy.payload.data(), &item, sizeof(item));
    std::memcpy(buy.payload.data() + 2, &qty, sizeof(qty));
    handler.on_message(connection, buy);

    // 5) Wire shape: BuyAck followed by an ITEM_TOTALINFO_LOCAL inventory
    //    refresh for the buyer.
    ASSERT_GE(reply.messages.size(), 2u);
    EXPECT_EQ(reply.messages[reply.messages.size() - 2].header.protocol,
              static_cast<std::uint8_t>(mxh::proto::ItemProtocol::BuyAck));
    EXPECT_EQ(reply.last_message.header.protocol,
              static_cast<std::uint8_t>(
                  mxh::proto::ItemProtocol::TotalInfoLocal));
    EXPECT_EQ(reply.last_message.payload.size(), sizeof(mxh::game::ItemTotalInfo));
    bool found_item = false;
    if (reply.last_message.payload.size() == sizeof(mxh::game::ItemTotalInfo)) {
        mxh::game::ItemTotalInfo total{};
        std::memcpy(&total, reply.last_message.payload.data(), sizeof(total));
        for (const auto& slot : total.Inventory) {
            found_item = found_item || slot.wIconIdx == item;
        }
    }
    EXPECT_TRUE(found_item) << "bought item must appear in the refreshed inventory";

    // 6) DB verification: a row for player_id=123 must exist with
    //    money=1000 (dealitem has no price field yet so total_price=0
    //    and the new money equals the old money).
    mxh::db::ResultSet rs;
    std::vector<mxh::db::Bind> qp = { mxh::db::bind(static_cast<std::int64_t>(123)) };
    auto sel = db.query(
        "SELECT money, updated_at FROM modern_player_state WHERE player_id = ?",
        qp, rs);
    ASSERT_TRUE(sel.ok()) << sel.error_message;
    ASSERT_EQ(rs.rows.size(), 1u);
    ASSERT_TRUE(std::holds_alternative<std::int64_t>(rs.rows[0][0]));
    EXPECT_EQ(std::get<std::int64_t>(rs.rows[0][0]), 1000);
    mxh::db::ResultSet item_rows;
    ASSERT_TRUE(db.query(
        "SELECT item_idx FROM modern_player_item WHERE player_id=? ORDER BY slot",
        qp, item_rows).ok());
    ASSERT_EQ(item_rows.rows.size(), 3u);
    for (const auto& row : item_rows.rows) {
        ASSERT_FALSE(row.empty());
        EXPECT_EQ(std::get<std::int64_t>(row[0]), item);
    }
}

TEST(MapHandlerTest, ItemPricesFillCatalogAndDeductRealMoney) {
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg{};
    cfg.backend = "sqlite";
    cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());
    ASSERT_TRUE(db.exec_multi(
        "CREATE TABLE modern_player_state ("
        " player_id INTEGER PRIMARY KEY, money INTEGER NOT NULL DEFAULT 0,"
        " level INTEGER NOT NULL DEFAULT 1, exp INTEGER NOT NULL DEFAULT 0,"
        " updated_at TEXT NOT NULL);"
        "CREATE TABLE modern_player_item ("
        " player_id INTEGER NOT NULL, container INTEGER NOT NULL,"
        " slot INTEGER NOT NULL, db_idx INTEGER NOT NULL, item_idx INTEGER NOT NULL,"
        " durability INTEGER NOT NULL, rare_idx INTEGER NOT NULL,"
        " quick_position INTEGER NOT NULL, item_param INTEGER NOT NULL,"
        " PRIMARY KEY (player_id, container, slot), UNIQUE (player_id, db_idx));").ok());
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    const auto connection = mxh::net::make_connection_id(55);

    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);
    ASSERT_TRUE(handler.set_player_money_for_test(123u, 20000u));

    const std::string deal_text = "1 map 2 npc 7 10 20 0 1 tab 555 10\n";
    const auto deal_bin = synthesize_dealitem_bin(deal_text);
    const auto deal_path = write_temp_bin(deal_bin);
    handler.load_dealitem(deal_path.string());
    std::error_code ec_deal; std::filesystem::remove(deal_path, ec_deal);

    // One ItemList row: ItemIdx=555, BuyPrice=12345 (token 5).
    std::vector<std::string> tokens(56u, "0");
    tokens[0] = "555";
    tokens[1] = "Potion";
    tokens[5] = "12345";
    std::string row;
    for (const auto& t : tokens) { row += t; row.push_back('\t'); }
    row.push_back('\r'); row.push_back('\n');
    const auto item_bin = synthesize_item_list_bin(row);
    const auto item_path = write_temp_bin(item_bin);
    handler.load_item_prices(item_path.string());
    std::error_code ec_item; std::filesystem::remove(item_path, ec_item);

    // Open the shop: the modern ShopList must carry the real price.
    mxh::net::Message talk;
    talk.header.object_id = 123u;
    talk.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Npc);
    talk.header.protocol = static_cast<std::uint8_t>(mxh::proto::NpcProtocol::SpeechSyn);
    talk.payload.resize(4);
    const std::uint32_t npc_id = 7u;
    std::memcpy(talk.payload.data(), &npc_id, sizeof(npc_id));
    reply.messages.clear();
    handler.on_message(connection, talk);
    ASSERT_FALSE(reply.messages.empty());
    const auto& shop = reply.last_message;
    EXPECT_EQ(shop.header.protocol, mxh::proto::kModernShopList);
    ASSERT_GE(shop.payload.size(), 6u);
    std::uint32_t price = 0;
    std::memcpy(&price, shop.payload.data() + 8, sizeof(price));  // first entry price
    EXPECT_EQ(price, 12345u);

    // Buy one: money 20000 -> 7655.
    reply.messages.clear();
    mxh::net::Message buy;
    buy.header.object_id = 123u;
    buy.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    buy.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::BuySyn);
    buy.payload.resize(4);
    const std::uint16_t item = 555u;
    const std::uint16_t qty = 1u;
    std::memcpy(buy.payload.data(), &item, sizeof(item));
    std::memcpy(buy.payload.data() + 2, &qty, sizeof(qty));
    handler.on_message(connection, buy);
    EXPECT_EQ(handler.player_money_for_test(123u), 7655u);
    ASSERT_FALSE(reply.messages.empty());
    EXPECT_EQ(reply.messages[reply.messages.size() - 2].header.protocol,
              static_cast<std::uint8_t>(mxh::proto::ItemProtocol::BuyAck));
}

// M3 D-stage: persist_player_money_for_test must hit the DB on
// every call, even when the wire-level BuySyn does not (no player
// connected).  This pins the helper in isolation.
TEST(MapHandlerTest, PersistPlayerMoneyForTestHitsDb) {
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg{};
    cfg.backend = "sqlite";
    cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());
    ASSERT_TRUE(db.exec_multi(
        "CREATE TABLE modern_player_state ("
        " player_id INTEGER PRIMARY KEY, money INTEGER NOT NULL DEFAULT 0,"
        " level INTEGER NOT NULL DEFAULT 1, exp INTEGER NOT NULL DEFAULT 0,"
        " updated_at TEXT NOT NULL);").ok());

    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    // No GameInSyn; no player in state.  The helper must still write
    // the row (DB persistence is decoupled from in-memory state).
    EXPECT_TRUE(handler.persist_player_money_for_test(777u, 4242u));

    mxh::db::ResultSet rs;
    std::vector<mxh::db::Bind> qp = { mxh::db::bind(static_cast<std::int64_t>(777)) };
    ASSERT_TRUE(db.query(
        "SELECT money FROM modern_player_state WHERE player_id = ?", qp, rs).ok());
    ASSERT_EQ(rs.rows.size(), 1u);
    EXPECT_EQ(std::get<std::int64_t>(rs.rows[0][0]), 4242);
}


// M3 D-stage: StartSyn quest_log persistence to modern_player_quest_log.
// End-to-end: real SqliteAdapter (in-memory) + inline CREATE TABLE +
// StartSyn Ok arm + SELECT verification.  Validates that the
// orchestrator actually hits the DB (not just a mock) and that
// the row is readable after the wire reply.
TEST(MapHandlerTest, StartSynOkArmPersistsQuestLogToSqliteMemory) {
    // 1) Real in-memory SQLite.
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg{};
    cfg.backend = "sqlite";
    cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());

    // 2) Inline CREATE TABLE (same shape as the production schema in
    //    deploy/database/mx_modern_schema_mssql.sql + MoxianDbTool
    //    moxian_schema_sql()).
    auto ct = db.exec_multi(
        "CREATE TABLE modern_player_quest_log ("
        " player_id        INTEGER NOT NULL,"
        " quest_id         INTEGER NOT NULL,"
        " state            INTEGER NOT NULL DEFAULT 0,"
        " accepted_time_ms INTEGER NOT NULL DEFAULT 0,"
        " updated_at       TEXT    NOT NULL,"
        " PRIMARY KEY (player_id, quest_id));"
        "CREATE TABLE modern_player_quest_sub ("
        " player_id INTEGER NOT NULL, quest_id INTEGER NOT NULL,"
        " sub_index INTEGER NOT NULL, kind INTEGER NOT NULL,"
        " target_id INTEGER NOT NULL, count INTEGER NOT NULL,"
        " target_count INTEGER NOT NULL,"
        " PRIMARY KEY (player_id, quest_id, sub_index));");
    ASSERT_TRUE(ct.ok()) << ct.error_message;

    // 3) Build a MapHandler around the live db.  ReplySpy captures
    //    the StartAck so we can confirm the wire shape did not change.
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    const auto connection = mxh::net::make_connection_id(55);

    mxh::net::Message game_in;
    game_in.header.object_id = 456u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);

    // 4) Load a quest script definition so StartSyn Ok arm fires.
    const std::string quest_text = "$QUEST 99 { $SUBQUEST 1 { #TRIGGER @HUNT 1 10 *ADDCOUNT 1 1 } }";
    const auto qbin = synthesize_dealitem_bin(quest_text);
    const auto qpath = write_temp_bin(qbin);
    handler.load_quest_script(qpath.string());
    std::error_code ec_q; std::filesystem::remove(qpath, ec_q);
    ASSERT_EQ(handler.quest_definitions_for_test().quests.size(), 1u);

    // 5) Send StartSyn for quest 99.
    mxh::net::Message start;
    start.header.object_id = 456u;
    start.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Quest);
    start.header.protocol = static_cast<std::uint8_t>(mxh::proto::QuestProtocol::StartSyn);
    start.payload.resize(2);
    const std::uint16_t qid = 99u;
    std::memcpy(start.payload.data(), &qid, sizeof(qid));
    handler.on_message(connection, start);

    // 6) Wire shape: a StartAck must have been sent.
    ASSERT_FALSE(reply.messages.empty());
    EXPECT_EQ(reply.last_message.header.protocol,
              static_cast<std::uint8_t>(mxh::proto::QuestProtocol::StartAck));

    // 7) DB verification: a row for (player_id=456, quest_id=99)
    //    must exist with state != 0 (Accepted/Active).
    mxh::db::ResultSet rs;
    std::vector<mxh::db::Bind> qp = { mxh::db::bind(static_cast<std::int64_t>(456)) };
    auto sel = db.query(
        "SELECT quest_id, state FROM modern_player_quest_log WHERE player_id = ?",
        qp, rs);
    ASSERT_TRUE(sel.ok()) << sel.error_message;
    ASSERT_EQ(rs.rows.size(), 1u);
    EXPECT_EQ(std::get<std::int64_t>(rs.rows[0][0]), 99);
    const auto state_value = std::get<std::int64_t>(rs.rows[0][1]);
    EXPECT_NE(state_value, 0) << "expected state != 0 (None) after StartSyn Ok";
}

TEST(MapHandlerTest, GameInRestoresPersistedQuestSubProgress) {
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg{};
    cfg.backend = "sqlite";
    cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());
    ASSERT_TRUE(db.exec_multi(
        "CREATE TABLE modern_player_quest_log (player_id INTEGER NOT NULL,quest_id INTEGER NOT NULL,"
        "state INTEGER NOT NULL,accepted_time_ms INTEGER NOT NULL,updated_at TEXT NOT NULL,"
        "PRIMARY KEY(player_id,quest_id));"
        "CREATE TABLE modern_player_quest_sub (player_id INTEGER NOT NULL,quest_id INTEGER NOT NULL,"
        "sub_index INTEGER NOT NULL,kind INTEGER NOT NULL,target_id INTEGER NOT NULL,count INTEGER NOT NULL,"
        "target_count INTEGER NOT NULL,PRIMARY KEY(player_id,quest_id,sub_index));"
        "INSERT INTO modern_player_quest_log VALUES(456,99,1,1234,'now');"
        "INSERT INTO modern_player_quest_sub VALUES(456,99,0,1,10,4,10);").ok());

    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    const std::string quest_text = "$QUEST 99 { $SUBQUEST 1 { #TRIGGER @HUNT 10 10 *ADDCOUNT 1 10 } }";
    const auto qpath = write_temp_bin(synthesize_dealitem_bin(quest_text));
    handler.load_quest_script(qpath.string());
    std::error_code ec; std::filesystem::remove(qpath, ec);

    mxh::net::Message game_in;
    game_in.header.object_id = 456u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(mxh::net::make_connection_id(55), game_in);

    const auto quest_snapshot = std::find_if(reply.messages.begin(), reply.messages.end(),
        [](const mxh::net::Message& message) {
            return message.header.category == static_cast<std::uint8_t>(mxh::proto::Category::Quest) &&
                   message.header.protocol == static_cast<std::uint8_t>(mxh::proto::QuestProtocol::TotalInfo) &&
                   message.payload.size() >= 8u;
        });
    ASSERT_NE(quest_snapshot, reply.messages.end());
    std::uint32_t snapshot_state = 0;
    std::memcpy(&snapshot_state, quest_snapshot->payload.data() + 4, 4);
    EXPECT_EQ(snapshot_state, static_cast<std::uint32_t>(mxh::server::QuestState::Accepted));

    const auto progress = handler.quest_progress_for_test(456u, 99u);
    ASSERT_TRUE(progress.has_value());
    ASSERT_EQ(progress->subs.size(), 1u);
    EXPECT_EQ(progress->accepted_time_ms, 1234u);
    EXPECT_EQ(progress->state, mxh::server::QuestState::Accepted);
    EXPECT_EQ(progress->subs[0].count, 4u);
    EXPECT_EQ(progress->subs[0].target, 10u);
}

TEST(MapHandlerTest, CompletedQuestRewardPersistsAndCannotBeClaimedTwice) {
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg{}; cfg.backend = "sqlite"; cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());
    ASSERT_TRUE(db.exec_multi(
        "CREATE TABLE character_info(chrid INTEGER PRIMARY KEY,charname TEXT,sex_type INTEGER,face_type INTEGER,"
        "hair_type INTEGER,height REAL,width REAL,level INTEGER,map_num INTEGER);"
        "CREATE TABLE modern_player_state(player_id INTEGER PRIMARY KEY,money INTEGER NOT NULL,level INTEGER NOT NULL,"
        "exp INTEGER NOT NULL,updated_at TEXT NOT NULL);"
        "CREATE TABLE modern_player_quest_log(player_id INTEGER NOT NULL,quest_id INTEGER NOT NULL,state INTEGER NOT NULL,"
        "accepted_time_ms INTEGER NOT NULL,updated_at TEXT NOT NULL,PRIMARY KEY(player_id,quest_id));"
        "CREATE TABLE modern_player_quest_sub(player_id INTEGER NOT NULL,quest_id INTEGER NOT NULL,sub_index INTEGER NOT NULL,"
        "kind INTEGER NOT NULL,target_id INTEGER NOT NULL,count INTEGER NOT NULL,target_count INTEGER NOT NULL,"
        "PRIMARY KEY(player_id,quest_id,sub_index));"
        "CREATE TABLE modern_player_item(player_id INTEGER NOT NULL,container INTEGER NOT NULL,slot INTEGER NOT NULL,"
        "db_idx INTEGER NOT NULL,item_idx INTEGER NOT NULL,durability INTEGER NOT NULL,rare_idx INTEGER NOT NULL,"
        "quick_position INTEGER NOT NULL,item_param INTEGER NOT NULL,PRIMARY KEY(player_id,container,slot),UNIQUE(player_id,db_idx));"
        "INSERT INTO character_info VALUES(123,'QuestHero',0,1,1,1,1,1,7);"
        "INSERT INTO modern_player_state VALUES(123,100,1,0,'now');").ok());
    ReplySpy reply; mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    handler.load_experience_curve(std::string(MXH_SOURCE_DIR) + "/data/PlayDH/Resource/CharacterExpPoint.bin");
    const std::string quest_text =
        "$QUEST 99 { $SUBQUEST 1 { #TRIGGER @HUNT 77 1 *GIVEMONEY 50 *TAKEEXP 25 *GIVEITEM 8000 2 *ENDQUEST 1 } }";
    const auto qpath = write_temp_bin(synthesize_dealitem_bin(quest_text));
    handler.load_quest_script(qpath.string()); std::error_code ec; std::filesystem::remove(qpath, ec);
    const auto connection = mxh::net::make_connection_id(55);
    mxh::net::Message in; in.header.object_id=123; in.header.category=static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    in.header.protocol=static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn); handler.on_message(connection,in);
    mxh::net::Message start; start.header.object_id=123; start.header.category=static_cast<std::uint8_t>(mxh::proto::Category::Quest);
    start.header.protocol=static_cast<std::uint8_t>(mxh::proto::QuestProtocol::StartSyn); start.payload.resize(2);
    const std::uint16_t qid=99; std::memcpy(start.payload.data(),&qid,sizeof(qid)); handler.on_message(connection,start);
    mxh::game::MonsterInstance monster; monster.object_id=99001; monster.monster_kind=77; monster.max_life=1; monster.current_life=1;
    ASSERT_TRUE(handler.add_monster_instance(monster)); (void)handler.apply_monster_damage(123,monster.object_id,1,99);
    mxh::net::Message end=start; end.header.protocol=static_cast<std::uint8_t>(mxh::proto::QuestProtocol::EndSyn);
    reply.messages.clear(); handler.on_message(connection,end);
    ASSERT_FALSE(reply.messages.empty());
    EXPECT_EQ(reply.last_message.header.protocol,static_cast<std::uint8_t>(mxh::proto::QuestProtocol::EndAck));
    mxh::db::ResultSet state; const std::vector<mxh::db::Bind> none;
    ASSERT_TRUE(db.query("SELECT money,level,exp FROM modern_player_state WHERE player_id=123",none,state).ok());
    ASSERT_EQ(state.rows.size(),1u);
    EXPECT_EQ(std::get<std::int64_t>(state.rows[0][0]),150);
    // Legacy CPlayer::SetPlayerExpPoint subtracts the level threshold on
    // level-up.  Monster (10) + quest (25) crosses level-1's 20-point
    // threshold, so the persisted current-level experience is 15 at level 2.
    EXPECT_EQ(std::get<std::int64_t>(state.rows[0][1]),2);
    EXPECT_EQ(std::get<std::int64_t>(state.rows[0][2]),15);
    mxh::db::ResultSet items; ASSERT_TRUE(db.query("SELECT item_idx,item_param FROM modern_player_item WHERE player_id=123",none,items).ok());
    ASSERT_EQ(items.rows.size(),1u); EXPECT_EQ(std::get<std::int64_t>(items.rows[0][0]),8000); EXPECT_EQ(std::get<std::int64_t>(items.rows[0][1]),2);
    mxh::db::ResultSet quest; ASSERT_TRUE(db.query("SELECT state FROM modern_player_quest_log WHERE player_id=123 AND quest_id=99",none,quest).ok());
    ASSERT_EQ(quest.rows.size(),1u); EXPECT_EQ(std::get<std::int64_t>(quest.rows[0][0]),static_cast<std::int64_t>(mxh::server::QuestState::Rewarded));
    reply.messages.clear(); handler.on_message(connection,end); ASSERT_FALSE(reply.messages.empty());
    EXPECT_EQ(reply.last_message.header.protocol,static_cast<std::uint8_t>(mxh::proto::QuestProtocol::EndNack));
}

TEST(MapHandlerTest, QuestRewardWriteFailureRestoresRuntimeAndReturnsNack) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    const auto connection = mxh::net::make_connection_id(55);
    mxh::net::Message in;
    in.header.object_id = 123;
    in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, in);
    const auto qpath = write_temp_bin(synthesize_dealitem_bin(
        "$QUEST 99 { $SUBQUEST 1 { #TRIGGER @HUNT 77 1 *GIVEMONEY 50 *ENDQUEST 1 } }"));
    handler.load_quest_script(qpath.string());
    std::error_code ec; std::filesystem::remove(qpath, ec);
    mxh::net::Message start;
    start.header.object_id = 123;
    start.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Quest);
    start.header.protocol = static_cast<std::uint8_t>(mxh::proto::QuestProtocol::StartSyn);
    start.payload.resize(2);
    const std::uint16_t qid = 99;
    std::memcpy(start.payload.data(), &qid, sizeof(qid));
    handler.on_message(connection, start);
    mxh::game::MonsterInstance monster;
    monster.object_id = 99001; monster.monster_kind = 77; monster.max_life = 1; monster.current_life = 1;
    ASSERT_TRUE(handler.add_monster_instance(monster));
    (void)handler.apply_monster_damage(123, monster.object_id, 1, 99);
    ASSERT_EQ(handler.quest_progress_for_test(123, 99)->state, mxh::server::QuestState::Complete);
    const auto money_before = handler.player_money_for_test(123);
    mxh::net::Message end = start;
    end.header.protocol = static_cast<std::uint8_t>(mxh::proto::QuestProtocol::EndSyn);
    db.fail_write_matching = "UPDATE modern_player_state";
    reply.messages.clear();
    handler.on_message(connection, end);
    ASSERT_EQ(reply.messages.size(), 1u);
    EXPECT_EQ(reply.messages[0].header.protocol,
              static_cast<std::uint8_t>(mxh::proto::QuestProtocol::EndNack));
    EXPECT_EQ(handler.quest_progress_for_test(123, 99)->state, mxh::server::QuestState::Complete);
    EXPECT_EQ(handler.player_money_for_test(123), money_before);
    EXPECT_EQ(db.rollback_count.load(), 1);
    EXPECT_FALSE(handler.is_draining());

    db.fail_write_matching.clear();
    reply.messages.clear();
    handler.on_message(connection, end);
    ASSERT_FALSE(reply.messages.empty());
    EXPECT_EQ(reply.messages.back().header.protocol,
              static_cast<std::uint8_t>(mxh::proto::QuestProtocol::EndAck));
    EXPECT_EQ(handler.quest_progress_for_test(123, 99)->state, mxh::server::QuestState::Rewarded);
    EXPECT_EQ(handler.player_money_for_test(123), money_before + 50);
}

// M3 D-stage: persist_quest_log_for_test must hit the DB on every
// call.  This pins the helper in isolation (no wire traffic).
TEST(MapHandlerTest, PersistQuestLogForTestHitsDb) {
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg{};
    cfg.backend = "sqlite";
    cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());
    ASSERT_TRUE(db.exec_multi(
        "CREATE TABLE modern_player_quest_log ("
        " player_id        INTEGER NOT NULL,"
        " quest_id         INTEGER NOT NULL,"
        " state            INTEGER NOT NULL DEFAULT 0,"
        " accepted_time_ms INTEGER NOT NULL DEFAULT 0,"
        " updated_at       TEXT    NOT NULL,"
        " PRIMARY KEY (player_id, quest_id));"
        "CREATE TABLE modern_player_quest_sub ("
        " player_id INTEGER NOT NULL, quest_id INTEGER NOT NULL,"
        " sub_index INTEGER NOT NULL, kind INTEGER NOT NULL,"
        " target_id INTEGER NOT NULL, count INTEGER NOT NULL,"
        " target_count INTEGER NOT NULL,"
        " PRIMARY KEY (player_id, quest_id, sub_index));").ok());

    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    const auto connection = mxh::net::make_connection_id(55);
    mxh::net::Message game_in;
    game_in.header.object_id = 888u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);

    // Inject one quest into the player's quest_log via StartSyn Ok.
    const std::string quest_text = "$QUEST 7 { $SUBQUEST 1 { #TRIGGER @HUNT 1 10 *ADDCOUNT 1 1 } }";
    const auto qbin = synthesize_dealitem_bin(quest_text);
    const auto qpath = write_temp_bin(qbin);
    handler.load_quest_script(qpath.string());
    std::error_code ec_q; std::filesystem::remove(qpath, ec_q);
    ASSERT_EQ(handler.quest_definitions_for_test().quests.size(), 1u);
    mxh::net::Message start;
    start.header.object_id = 888u;
    start.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Quest);
    start.header.protocol = static_cast<std::uint8_t>(mxh::proto::QuestProtocol::StartSyn);
    start.payload.resize(2);
    const std::uint16_t qid = 7u;
    std::memcpy(start.payload.data(), &qid, sizeof(qid));
    handler.on_message(connection, start);
    ASSERT_EQ(handler.persisted_quest_count_for_test(888u), 1u);

    // Clear the row and verify persist_quest_log_for_test rewrites it.
    ASSERT_TRUE(db.exec_multi("DELETE FROM modern_player_quest_log WHERE player_id = 888").ok());
    mxh::db::ResultSet rs0;
    std::vector<mxh::db::Bind> qp0 = { mxh::db::bind(static_cast<std::int64_t>(888)) };
    ASSERT_TRUE(db.query("SELECT COUNT(*) FROM modern_player_quest_log WHERE player_id = ?", qp0, rs0).ok());
    ASSERT_EQ(rs0.rows.size(), 1u);
    EXPECT_EQ(std::get<std::int64_t>(rs0.rows[0][0]), 0);

    EXPECT_TRUE(handler.persist_quest_log_for_test(888u));

    mxh::db::ResultSet rs;
    std::vector<mxh::db::Bind> qp = { mxh::db::bind(static_cast<std::int64_t>(888)) };
    ASSERT_TRUE(db.query(
        "SELECT quest_id FROM modern_player_quest_log WHERE player_id = ?", qp, rs).ok());
    ASSERT_EQ(rs.rows.size(), 1u);
    EXPECT_EQ(std::get<std::int64_t>(rs.rows[0][0]), 7);
}

// GameOut is the authoritative session boundary: live money must be flushed
// before the player runtime is removed, so a subsequent GameIn can observe it.
TEST(MapHandlerTest, SavedPositionLoadsAndLogoutPersistsMapUsedByCharacterSelect) {
    for (const bool incoming_transfer : {false, true}) {
    SCOPED_TRACE(incoming_transfer);
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg{};
    cfg.backend = "sqlite"; cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());
    ASSERT_TRUE(mxh::db::migrate_modern_schema(db).ok());
    ASSERT_TRUE(db.exec_multi("INSERT INTO character_info(chrid,userid,charname,map_num) VALUES(777,'123','PositionHero',10);"
        "INSERT INTO modern_player_position VALUES(777,2,7211,43329,CURRENT_TIMESTAMP);").ok());
    if (incoming_transfer)
        ASSERT_TRUE(db.exec_multi("UPDATE modern_player_position SET map_num=10,pos_x=46973,pos_z=4198;").ok());
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 2, make_reply_spy(reply));
    ASSERT_TRUE(handler.load_map_routes(std::filesystem::path(__FILE__).parent_path().parent_path().parent_path().parent_path()
        / "data" / "PlayDH" / "Resource" / "MapChange.bin"));
    handler.set_allow_dev_gamein_fallback(false);
    mxh::net::Message message;
    message.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    message.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    message.header.object_id = 777;
    message.payload.assign(16, 0); message.payload[0] = 123;
    handler.on_message({99}, message);
    auto ack = std::find_if(reply.messages.begin(), reply.messages.end(), [](const auto& item) {
        return item.header.protocol == static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInAck);
    });
    ASSERT_NE(ack, reply.messages.end());
    std::uint16_t x = 0, z = 0;
    std::memcpy(&x, ack->payload.data() + 207, 2);
    std::memcpy(&z, ack->payload.data() + 209, 2);
    EXPECT_EQ(x, 7211); EXPECT_EQ(z, 43329);
    ASSERT_TRUE(handler.set_player_position_for_test(777, 7250, 43350));
    message.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameOutSyn);
    handler.on_message({99}, message);
    EXPECT_EQ(reply.messages.back().header.protocol, static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameOutAck));
    mxh::db::ResultSet rows;
    ASSERT_TRUE(db.query("SELECT map_num,pos_x,pos_z FROM modern_player_position WHERE player_id=777", {}, rows).ok());
    ASSERT_EQ(rows.rows.size(), 1u);
    EXPECT_EQ(std::get<std::int64_t>(rows.rows[0][0]), 2);
    EXPECT_EQ(std::get<std::int64_t>(rows.rows[0][1]), 7250);
    EXPECT_EQ(std::get<std::int64_t>(rows.rows[0][2]), 43350);
    ASSERT_TRUE(db.query("SELECT map_num FROM character_info WHERE chrid=777", {}, rows).ok());
    ASSERT_EQ(rows.rows.size(), 1u);
    EXPECT_EQ(std::get<std::int64_t>(rows.rows[0][0]), 2);
}

}

TEST(MapHandlerTest, CrossMapEntryRejectsPositionAwayFromCanonicalExit) {
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg{};
    cfg.backend = "sqlite"; cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());
    ASSERT_TRUE(mxh::db::migrate_modern_schema(db).ok());
    ASSERT_TRUE(db.exec_multi("INSERT INTO character_info(chrid,userid,charname,map_num) VALUES(777,'123','PositionHero',10);"
        "INSERT INTO modern_player_position VALUES(777,10,25000,25000,CURRENT_TIMESTAMP);").ok());
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 2, make_reply_spy(reply));
    handler.set_allow_dev_gamein_fallback(false);
    ASSERT_TRUE(handler.load_map_routes(std::filesystem::path(__FILE__).parent_path().parent_path().parent_path().parent_path()
        / "data" / "PlayDH" / "Resource" / "MapChange.bin"));
    mxh::net::Message message;
    message.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    message.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    message.header.object_id = 777;
    message.payload.assign(16, 0); message.payload[0] = 123;
    handler.on_message({99}, message);
    ASSERT_EQ(reply.messages.size(), 1u);
    EXPECT_EQ(reply.messages[0].header.protocol, static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInNack));
    EXPECT_FALSE(handler.player_runtime_snapshot(777));
}

TEST(MapHandlerTest, GameOutSynPersistsLiveMoneyBeforeRuntimeRemoval) {
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg{};
    cfg.backend = "sqlite";
    cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());
    ASSERT_TRUE(db.exec_multi(
        "CREATE TABLE modern_player_state ("
        " player_id INTEGER PRIMARY KEY, money INTEGER NOT NULL DEFAULT 0,"
        " level INTEGER NOT NULL DEFAULT 1, exp INTEGER NOT NULL DEFAULT 0,"
        " updated_at TEXT NOT NULL);"
        "CREATE TABLE modern_player_item ("
        " player_id INTEGER NOT NULL, container INTEGER NOT NULL, slot INTEGER NOT NULL,"
        " db_idx INTEGER NOT NULL, item_idx INTEGER NOT NULL, durability INTEGER NOT NULL,"
        " rare_idx INTEGER NOT NULL, quick_position INTEGER NOT NULL, item_param INTEGER NOT NULL);"
    ).ok());
    ASSERT_TRUE(mxh::db::migrate_modern_schema(db).ok());

    ReplySpy reply;
    mxh::server::MapHandler handler(db, 10, make_reply_spy(reply));
    const auto connection = mxh::net::make_connection_id(99);
    mxh::net::Message game_in;
    game_in.header.object_id = 777u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);
    ASSERT_TRUE(handler.set_player_money_for_test(777u, 4242u));

    mxh::net::Message game_out;
    game_out.header.object_id = 777u;
    game_out.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_out.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameOutSyn);
    handler.on_message(connection, game_out);

    mxh::db::ResultSet rows;
    const std::vector<mxh::db::Bind> args{mxh::db::bind(static_cast<std::int64_t>(777))};
    ASSERT_TRUE(db.query("SELECT money FROM modern_player_state WHERE player_id=?", args, rows).ok());
    ASSERT_EQ(rows.rows.size(), 1u);
    EXPECT_EQ(std::get<std::int64_t>(rows.rows[0][0]), 4242);
    EXPECT_EQ(handler.player_runtime_count(), 0u);
    ASSERT_FALSE(reply.messages.empty());
    EXPECT_EQ(reply.last_message.header.protocol,
              static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameOutAck));
    EXPECT_EQ(reply.last_message.header.object_id, 777u);
    EXPECT_TRUE(reply.last_message.payload.empty());
}

TEST(MapHandlerTest, MultiplexedGameOutAcknowledgementsIdentifyEachCharacter) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 10, make_reply_spy(reply));
    const auto connection = mxh::net::make_connection_id(99);
    mxh::net::Message request;
    request.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    for (const std::uint32_t player : {777u, 888u}) {
        request.header.object_id = player;
        request.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
        handler.on_message(connection, request);
    }
    ASSERT_EQ(handler.player_runtime_count(), 2u);
    for (const std::uint32_t player : {888u, 777u}) {
        request.header.object_id = player;
        request.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameOutSyn);
        handler.on_message(connection, request);
        ASSERT_EQ(reply.last_id.value, connection.value);
        EXPECT_EQ(reply.last_message.header.protocol, static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameOutAck));
        EXPECT_EQ(reply.last_message.header.object_id, player);
        EXPECT_TRUE(reply.last_message.payload.empty());
    }
    EXPECT_EQ(handler.player_runtime_count(), 0u);
    handler.on_message(connection, request);
    EXPECT_EQ(reply.last_message.header.protocol, static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameOutNack));
    EXPECT_EQ(reply.last_message.header.object_id, 777u);
}

TEST(MapHandlerTest, StrictGameInRejectsMissingOwnedCharacterAndReadFailures) {
    const std::vector<std::string> missing_tables = {"", "", "", "character_info", "modern_player_state",
        "modern_player_item", "modern_player_quest_log", "modern_player_quest_sub", "modern_character_equipment"};
    for (std::size_t scenario = 0; scenario < missing_tables.size(); ++scenario) {
        SCOPED_TRACE(scenario);
        mxh::db::SqliteAdapter db;
        mxh::db::ConnectionConfig config{};
        config.backend = "sqlite"; config.path = ":memory:";
        ASSERT_TRUE(db.connect(config).ok());
        ASSERT_TRUE(mxh::db::migrate_modern_schema(db).ok());
        if (scenario != 1)
            ASSERT_TRUE(db.exec_multi("INSERT INTO character_info(charname,chrid,userid,map_num,start_area) VALUES('EntryTest',777,'123',10,10);").ok());
        if (!missing_tables[scenario].empty())
            ASSERT_TRUE(db.exec_multi("DROP TABLE " + missing_tables[scenario]).ok());
        ReplySpy reply;
        mxh::server::MapHandler handler(db, 10, make_reply_spy(reply));
        handler.set_allow_dev_gamein_fallback(false);
        mxh::net::Message entry;
        entry.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
        entry.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
        entry.header.object_id = 777;
        entry.payload.resize(16, 0);
        const std::uint32_t owner = scenario == 2 ? 124u : 123u;
        std::memcpy(entry.payload.data(), &owner, sizeof(owner));
        handler.on_message(mxh::net::make_connection_id(99), entry);
        ASSERT_FALSE(reply.messages.empty());
        if (scenario == 0) {
            EXPECT_EQ(handler.player_runtime_count(), 1u); // Empty inventory/quests are valid persisted state.
            EXPECT_EQ(reply.messages.front().header.protocol, static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInAck));
        } else {
            EXPECT_EQ(handler.player_runtime_count(), 0u);
            ASSERT_EQ(reply.messages.size(), 1u);
            EXPECT_EQ(reply.last_message.header.protocol, static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInNack));
            EXPECT_EQ(reply.last_message.header.object_id, 777u);
        }
    }
}

TEST(MapHandlerTest, GameOutWriteFailureRetriesButCommitUncertaintyDrains) {
    for (int phase = 0; phase < 4; ++phase) {
        SCOPED_TRACE(phase);
        MockDbAdapter db;
        ReplySpy reply;
        mxh::server::MapHandler handler(db, 10, make_reply_spy(reply));
        const auto connection = mxh::net::make_connection_id(99);
        mxh::net::Message request;
        request.header.object_id = 777;
        request.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
        request.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
        handler.on_message(connection, request);
        ASSERT_TRUE(handler.set_player_money_for_test(777, 4242));
        db.fail_begin = phase == 0;
        db.fail_commit = phase == 1;
        if (phase == 2) db.fail_write_matching = "DELETE FROM modern_player_item";
        if (phase == 3) db.fail_write_matching = "INSERT INTO modern_player_state";
        request.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameOutSyn);
        handler.on_message(connection, request);
        EXPECT_EQ(reply.last_message.header.protocol, static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameOutNack));
        EXPECT_EQ(reply.last_message.header.object_id, 777u);
        EXPECT_EQ(handler.player_runtime_count(), 1u);
        EXPECT_EQ(handler.player_money_for_test(777), 4242u);
        if (phase == 1) {
            EXPECT_TRUE(handler.is_draining());
            EXPECT_FALSE(handler.prepare_for_shutdown());
            const auto calls = reply.call_count.load();
            handler.on_message(connection, request);
            EXPECT_EQ(reply.call_count.load(), calls);
            continue;
        }
        db.fail_begin = db.fail_commit = false;
        db.fail_write_matching.clear();
        handler.on_message(connection, request);
        EXPECT_EQ(reply.last_message.header.protocol, static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameOutAck));
        EXPECT_EQ(handler.player_runtime_count(), 0u);
    }
}

TEST(MapHandlerTest, LateExitFailureRollsBackItemsMoneyAndPositionBeforeRetry) {
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg{};
    cfg.backend = "sqlite"; cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());
    ASSERT_TRUE(mxh::db::migrate_modern_schema(db).ok());
    ASSERT_TRUE(db.exec_multi(
        "INSERT INTO character_info(chrid,userid,charname,map_num) VALUES(777,'123','AtomicHero',10);"
        "INSERT INTO modern_player_position VALUES(777,10,25000,25000,CURRENT_TIMESTAMP);"
        "INSERT INTO modern_player_state(player_id,money,updated_at) VALUES(777,100,CURRENT_TIMESTAMP);").ok());
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 10, make_reply_spy(reply));
    handler.set_allow_dev_gamein_fallback(false);
    mxh::net::Message request;
    request.header.object_id = 777;
    request.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    request.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    request.payload.assign(16, 0); request.payload[0] = 123;
    handler.on_message({99}, request);
    ASSERT_EQ(handler.player_runtime_count(), 1u);
    ASSERT_TRUE(handler.set_player_money_for_test(777, 4242));
    ASSERT_TRUE(handler.set_player_position_for_test(777, 25100, 25200));
    // A database sentinel absent from the runtime proves the earlier DELETE is
    // rolled back, even though failure occurs after the position UPSERT.
    ASSERT_TRUE(db.exec_multi(
        "INSERT INTO modern_player_item(player_id,container,slot,db_idx,item_idx,durability,rare_idx,quick_position,item_param) "
        "VALUES(777,0,0,1234,53343,1,0,0,0);"
        "CREATE TRIGGER fail_exit_map BEFORE UPDATE OF map_num ON character_info "
        "BEGIN SELECT RAISE(ABORT,'injected late exit failure'); END;").ok());
    request.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameOutSyn);
    handler.on_message({99}, request);
    EXPECT_EQ(reply.last_message.header.protocol, static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameOutNack));
    EXPECT_EQ(handler.player_runtime_count(), 1u);
    mxh::db::ResultSet rows;
    ASSERT_TRUE(db.query("SELECT money FROM modern_player_state WHERE player_id=777", {}, rows).ok());
    EXPECT_EQ(std::get<std::int64_t>(rows.rows.at(0).at(0)), 100);
    ASSERT_TRUE(db.query("SELECT pos_x,pos_z FROM modern_player_position WHERE player_id=777", {}, rows).ok());
    EXPECT_EQ(std::get<std::int64_t>(rows.rows.at(0).at(0)), 25000);
    EXPECT_EQ(std::get<std::int64_t>(rows.rows.at(0).at(1)), 25000);
    ASSERT_TRUE(db.query("SELECT db_idx FROM modern_player_item WHERE player_id=777", {}, rows).ok());
    ASSERT_EQ(rows.rows.size(), 1u);
    EXPECT_EQ(std::get<std::int64_t>(rows.rows.at(0).at(0)), 1234);
    ASSERT_TRUE(db.exec_multi("DROP TRIGGER fail_exit_map;").ok());
    ASSERT_TRUE(handler.set_player_vitals_for_test(777,0,5));
    ASSERT_TRUE(db.exec_multi("CREATE TRIGGER fail_exit_vitals BEFORE UPDATE OF character_data ON character_info "
        "BEGIN SELECT RAISE(ABORT,'injected vitals exit failure'); END;").ok());
    handler.on_message({99}, request);
    EXPECT_EQ(reply.last_message.header.protocol, static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameOutNack));
    EXPECT_EQ(handler.player_runtime_count(), 1u);
    ASSERT_TRUE(db.query("SELECT money FROM modern_player_state WHERE player_id=777", {}, rows).ok());
    EXPECT_EQ(std::get<std::int64_t>(rows.rows.at(0).at(0)),100);
    ASSERT_TRUE(db.query("SELECT pos_x FROM modern_player_position WHERE player_id=777", {}, rows).ok());
    EXPECT_EQ(std::get<std::int64_t>(rows.rows.at(0).at(0)),25000);
    const auto failed_vitals=mxh::db::load_modern_shop_state(db,777,123);
    ASSERT_TRUE(failed_vitals); EXPECT_FALSE(failed_vitals->vitals);
    ASSERT_TRUE(db.exec_multi("DROP TRIGGER fail_exit_vitals;").ok());
    handler.on_message({99}, request);
    EXPECT_EQ(reply.last_message.header.protocol, static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameOutAck));
    EXPECT_EQ(handler.player_runtime_count(), 0u);
    ASSERT_TRUE(db.query("SELECT money FROM modern_player_state WHERE player_id=777", {}, rows).ok());
    EXPECT_EQ(std::get<std::int64_t>(rows.rows.at(0).at(0)), 4242);
    const auto saved_vitals=mxh::db::load_modern_shop_state(db,777,123);
    ASSERT_TRUE(saved_vitals); ASSERT_TRUE(saved_vitals->vitals);
    EXPECT_EQ(saved_vitals->vitals->life,0u);
    EXPECT_EQ(saved_vitals->vitals->naeryuk,5u);
    ASSERT_TRUE(db.query("SELECT pos_x,pos_z FROM modern_player_position WHERE player_id=777", {}, rows).ok());
    EXPECT_EQ(std::get<std::int64_t>(rows.rows.at(0).at(0)), 25100);
    EXPECT_EQ(std::get<std::int64_t>(rows.rows.at(0).at(1)), 25200);
    ASSERT_TRUE(db.query("SELECT db_idx FROM modern_player_item WHERE player_id=777", {}, rows).ok());
    EXPECT_TRUE(rows.empty());
}

TEST(MapHandlerTest, ExitAdapterExceptionRollsBackAndRollbackFailureStopsFurtherWork) {
    for (int rollback_mode = 0; rollback_mode < 3; ++rollback_mode) {
        SCOPED_TRACE(rollback_mode);
        MockDbAdapter db;
        ReplySpy reply;
        mxh::server::MapHandler handler(db, 10, make_reply_spy(reply));
        mxh::net::Message request;
        request.header.object_id = 777;
        request.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
        request.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
        handler.on_message({99}, request);
        ASSERT_EQ(handler.player_runtime_count(), 1u);
        db.throw_write = true;
        db.fail_rollback = rollback_mode == 1;
        db.throw_rollback = rollback_mode == 2;
        request.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameOutSyn);
        EXPECT_NO_THROW(handler.on_message({99}, request));
        EXPECT_EQ(reply.last_message.header.protocol, static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameOutNack));
        EXPECT_EQ(db.rollback_count.load(), 1);
        EXPECT_EQ(handler.player_runtime_count(), 1u);
        EXPECT_EQ(handler.is_draining(), rollback_mode != 0);
        const auto writes = db.exec_count.load();
        db.throw_write = db.fail_rollback = db.throw_rollback = false;
        handler.on_message({99}, request);
        if (rollback_mode == 0) {
            EXPECT_EQ(reply.last_message.header.protocol, static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameOutAck));
            EXPECT_EQ(handler.player_runtime_count(), 0u);
        } else {
            EXPECT_EQ(db.exec_count.load(), writes);
            EXPECT_FALSE(handler.prepare_for_shutdown());
        }
    }
}

TEST(MapHandlerTest, StatsReportsConnectedDrainingAndTimedMovement) {
    test::ReplySpy reply;
    auto db = mxh::db::make_adapter("sqlite");
    mxh::db::ConnectionConfig cfg;
    cfg.backend = "sqlite";
    cfg.path = ":memory:";
    ASSERT_TRUE(db->connect(cfg).ok());
    mxh::server::MapHandler handler(*db, /*map_num=*/12,
                                    test::make_reply_spy(reply));
    // set_timed_movement_enabled requires a non-empty FixedTileMap; install
    // the smallest valid one (2x2) so the toggle succeeds. Format: 4-byte
    // little-endian width, 4-byte height, then width*height*2 bytes of
    // WORD attributes (each cell gets two bytes: flag + value).
    std::vector<std::uint8_t> raw(8 + 2u * 2u * 2u, 0);
    raw[0] = 2; raw[4] = 2;  // width=2, height=2 (LE)
    std::string err;
    auto tiles = mxh::server::FixedTileMap::decode(raw, err);
    ASSERT_TRUE(tiles.has_value()) << err;
    ASSERT_TRUE(handler.install_fixed_tiles(std::move(*tiles)));
    ASSERT_TRUE(handler.set_timed_movement_enabled(true));

    // Empty handler reports zero.
    const auto initial = handler.stats();
    EXPECT_EQ(initial.map_num, 12u);
    EXPECT_EQ(initial.connected_players, 0u);
    EXPECT_EQ(initial.timed_movement_sessions, 0u);
    EXPECT_FALSE(initial.draining);

    // Marking draining flips the flag in stats.
    handler.prepare_for_shutdown();
    EXPECT_TRUE(handler.stats().draining);
}

TEST(MapHandlerTest, DrainingRejectsQueuedMessagesAndTickWithoutDatabaseWork) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 10, make_reply_spy(reply));
    mxh::net::Message request;
    request.header.object_id = 777;
    request.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    request.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message({99}, request);
    ASSERT_EQ(handler.player_runtime_count(), 1u);
    handler.prepare_for_shutdown();
    const auto writes = db.exec_count.load();
    const auto queries = db.query_count.load();
    const auto begins = db.begin_count.load();
    const auto replies = reply.call_count.load();
    // Already accepted TCP connections can still have messages queued when the
    // shutdown thread closes the database. None may admit or mutate players.
    request.header.object_id = 778;
    handler.on_message({99}, request);
    request.header.object_id = 777;
    request.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameOutSyn);
    handler.on_message({99}, request);
    handler.tick_monster_ai();
    EXPECT_EQ(handler.player_runtime_count(), 1u);
    EXPECT_EQ(reply.call_count.load(), replies);
    EXPECT_FALSE(handler.on_connect({100}, "127.0.0.1"));
    handler.on_disconnect({99}, mxh::net::NetError::Disconnected);
    EXPECT_EQ(handler.player_runtime_count(), 0u);
    EXPECT_EQ(db.exec_count.load(), writes);
    EXPECT_EQ(db.query_count.load(), queries);
    EXPECT_EQ(db.begin_count.load(), begins);
    handler.prepare_for_shutdown();
    EXPECT_EQ(db.exec_count.load(), writes);
}

TEST(MapHandlerTest, ShutdownSavesExitStateAndReportsLateFailureAfterDatabaseReopen) {
    for (const bool fail_save : {false, true}) {
        SCOPED_TRACE(fail_save);
        const auto path = std::filesystem::temp_directory_path() /
            ("mxh_shutdown_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".db");
        ASSERT_FALSE(std::filesystem::exists(path));
        struct Cleanup {
            std::filesystem::path path;
            ~Cleanup() { std::error_code ec; std::filesystem::remove(path, ec); }
        } cleanup{path};
        mxh::db::SqliteAdapter db;
        mxh::db::ConnectionConfig cfg{};
        cfg.backend = "sqlite"; cfg.path = path.string();
        ASSERT_TRUE(db.connect(cfg).ok());
        ASSERT_TRUE(mxh::db::migrate_modern_schema(db).ok());
        ASSERT_TRUE(db.exec_multi(
            "INSERT INTO character_info(chrid,userid,charname,map_num) VALUES(777,'123','ShutdownHero',10);"
            "INSERT INTO modern_player_position VALUES(777,10,25000,25000,CURRENT_TIMESTAMP);"
            "INSERT INTO modern_player_state(player_id,money,updated_at) VALUES(777,100,CURRENT_TIMESTAMP);").ok());
        ReplySpy reply;
        mxh::server::MapHandler handler(db, 10, make_reply_spy(reply));
        handler.set_allow_dev_gamein_fallback(false);
        mxh::net::Message message;
        message.header.object_id = 777;
        message.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
        message.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
        message.payload.assign(16, 0); message.payload[0] = 123;
        handler.on_message({99}, message);
        ASSERT_EQ(handler.player_runtime_count(), 1u);
        ASSERT_TRUE(handler.set_player_money_for_test(777, 4242));
        ASSERT_TRUE(handler.set_player_position_for_test(777, 25100, 25200));
        mxh::game::ItemBase item{};
        item.dwDBIdx = 1234; item.wIconIdx = 53343; item.Durability = 1;
        ASSERT_TRUE(handler.add_player_item_for_test(777, item));
        if (fail_save) ASSERT_TRUE(db.exec_multi(
            "CREATE TRIGGER fail_shutdown BEFORE UPDATE OF map_num ON character_info "
            "BEGIN SELECT RAISE(ABORT,'injected shutdown failure'); END;").ok());
        EXPECT_EQ(handler.prepare_for_shutdown(), !fail_save);
        EXPECT_FALSE(db.is_connected());
        EXPECT_EQ(handler.prepare_for_shutdown(), !fail_save);
        ASSERT_TRUE(db.connect(cfg).ok());
        mxh::db::ResultSet rows;
        ASSERT_TRUE(db.query("SELECT money FROM modern_player_state WHERE player_id=777", {}, rows).ok());
        EXPECT_EQ(std::get<std::int64_t>(rows.rows.at(0).at(0)), fail_save ? 100 : 4242);
        ASSERT_TRUE(db.query("SELECT pos_x FROM modern_player_position WHERE player_id=777", {}, rows).ok());
        EXPECT_EQ(std::get<std::int64_t>(rows.rows.at(0).at(0)), fail_save ? 25000 : 25100);
        ASSERT_TRUE(db.query("SELECT db_idx,item_idx FROM modern_player_item WHERE player_id=777", {}, rows).ok());
        if (fail_save) EXPECT_TRUE(rows.empty());
        else {
            ASSERT_EQ(rows.rows.size(), 1u);
            EXPECT_EQ(std::get<std::int64_t>(rows.rows.at(0).at(0)), 1234);
            EXPECT_EQ(std::get<std::int64_t>(rows.rows.at(0).at(1)), 53343);
        }
        db.disconnect();
    }
}

TEST(MapHandlerTest, DisconnectPersistsExitStateAndOldCallbackCannotRemoveReconnectedPlayer) {
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg{};
    cfg.backend = "sqlite"; cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());
    ASSERT_TRUE(mxh::db::migrate_modern_schema(db).ok());
    ASSERT_TRUE(db.exec_multi(
        "INSERT INTO character_info(chrid,userid,charname,map_num) VALUES(777,'123','DisconnectHero',10);").ok());
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 10, make_reply_spy(reply));
    handler.set_allow_dev_gamein_fallback(false);
    mxh::net::Message message;
    message.header.object_id = 777;
    message.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    message.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    message.payload.assign(16, 0); message.payload[0] = 123;
    handler.on_message({99}, message);
    ASSERT_EQ(handler.player_runtime_count(), 1u);
    ASSERT_TRUE(handler.set_player_money_for_test(777, 4242));
    ASSERT_TRUE(handler.set_player_position_for_test(777, 25100, 25200));
    mxh::game::ItemBase item{};
    item.dwDBIdx = 1234; item.wIconIdx = 53343; item.Durability = 1;
    ASSERT_TRUE(handler.add_player_item_for_test(777, item));
    handler.on_disconnect({99}, mxh::net::NetError::Disconnected);
    EXPECT_EQ(handler.player_runtime_count(), 0u);
    EXPECT_FALSE(handler.is_draining());
    mxh::db::ResultSet rows;
    ASSERT_TRUE(db.query("SELECT money FROM modern_player_state WHERE player_id=777", {}, rows).ok());
    EXPECT_EQ(std::get<std::int64_t>(rows.rows.at(0).at(0)), 4242);
    ASSERT_TRUE(db.query("SELECT pos_x,pos_z FROM modern_player_position WHERE player_id=777", {}, rows).ok());
    EXPECT_EQ(std::get<std::int64_t>(rows.rows.at(0).at(0)), 25100);
    EXPECT_EQ(std::get<std::int64_t>(rows.rows.at(0).at(1)), 25200);
    ASSERT_TRUE(db.query("SELECT db_idx FROM modern_player_item WHERE player_id=777", {}, rows).ok());
    EXPECT_EQ(std::get<std::int64_t>(rows.rows.at(0).at(0)), 1234);
    handler.on_message({100}, message);
    EXPECT_EQ(handler.player_money_for_test(777), 4242u);
    handler.on_disconnect({99}, mxh::net::NetError::Disconnected);
    EXPECT_EQ(handler.player_runtime_count(), 1u);
    EXPECT_FALSE(handler.is_draining());
}

TEST(MapHandlerTest, DisconnectSaveFailureDrainsInsteadOfAdmittingStaleReconnect) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 10, make_reply_spy(reply));
    mxh::net::Message message;
    message.header.object_id = 777;
    message.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    message.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message({99}, message);
    ASSERT_EQ(handler.player_runtime_count(), 1u);
    db.fail_write_matching = "INSERT INTO modern_player_state";
    handler.on_disconnect({99}, mxh::net::NetError::Disconnected);
    EXPECT_TRUE(handler.is_draining());
    EXPECT_FALSE(handler.prepare_for_shutdown());
    const auto writes = db.exec_count.load();
    handler.on_message({100}, message);
    EXPECT_EQ(handler.player_runtime_count(), 0u);
    EXPECT_EQ(db.exec_count.load(), writes);
}

}  // namespace mxh::server::test
