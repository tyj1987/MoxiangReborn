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

#include "mxh/game/hero_total_layout.hpp"
#include "mxh/server/server.hpp"
#include "mxh/game/item_manager.hpp"
#include "mxh/game/item_list_parser.hpp"
#include "mxh/compat/mh_file_ex.hpp"
#include "mxh/server/dealitem_parser.hpp"
#include "cstdint"
#include "filesystem"
#include "fstream"
#include "sstream"
#include "vector"
#include "mxh/db/db_adapter.hpp"
#include "mxh/net/net.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <memory>
#include <string>
#include <vector>

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
    mxh::db::DbResult execute(std::string_view, std::span<const mxh::db::Bind>) override {
        ++exec_count;
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

    mxh::db::DbResult begin_transaction() override { return {}; }
    mxh::db::DbResult commit() override { return {}; }
    mxh::db::DbResult rollback() override { return {}; }
    std::string backend_name() const noexcept override { return "mock"; }

    // Counters for tests that want to verify "handler did NOT call
    // the DB on a no-DB code path". Public so the gtest bodies can
    // read them directly.
    std::atomic<int> exec_count{0};
    std::atomic<int> query_count{0};
};

// Helper: a ReplyFn-compatible spy that counts calls.
struct ReplySpy {
    std::atomic<int> call_count{0};
    mxh::net::ConnectionId last_id{};
    mxh::net::Message last_message{};
    std::vector<mxh::net::Message> messages;
};

inline mxh::server::ReplyFn make_reply_spy(ReplySpy& spy) {
    return [&spy](mxh::net::ConnectionId id, const mxh::net::Message& message) {
        ++spy.call_count;
        spy.last_id = id;
        spy.last_message = message;
        spy.messages.push_back(message);
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
        if (!connected) return mxh::net::NetError::SendFailed;
        sent_msgs.push_back(msg);
        return mxh::net::NetError::Ok;
    }
    [[nodiscard]] bool is_connected() const noexcept override { return connected; }

    // Tests can flip connected ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¾ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ false to simulate the map server going
    // down between set_map_server() and on_disconnect().
    void set_connected(bool c) noexcept { connected = c; }

    std::atomic<int> send_count{0};
    std::vector<mxh::net::Message> sent_msgs;
    mxh::net::Message last_message{};
private:
    bool connected = true;
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

TEST(MapHandlerTest, GameInAckEmbedsCurrentItemLayoutWithoutLocalItemPacket) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);

    handler.on_message(mxh::net::make_connection_id(55), game_in);

    ASSERT_FALSE(reply.messages.empty());
    EXPECT_EQ(reply.call_count.load(), static_cast<int>(reply.messages.size()));
    const auto& ack = reply.messages.front();
    EXPECT_EQ(ack.header.category,
              static_cast<std::uint8_t>(mxh::proto::Category::UserConn));
    EXPECT_EQ(ack.header.protocol,
              static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInAck));
    ASSERT_EQ(ack.payload.size(), mxh::game::HERO_TOTAL_EMPTY_PAYLOAD_SIZE);
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
    const auto connection = mxh::net::make_connection_id(55);
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);

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
std::filesystem::path write_temp_bin(const std::vector<std::uint8_t>& bytes) {
    const auto path = std::filesystem::temp_directory_path() / "mxh_map_handler_load_test.bin";
    std::ofstream ofs(path, std::ios::binary);
    ofs.write(reinterpret_cast<const char*>(bytes.data()),
               static_cast<std::streamsize>(bytes.size()));
    return path;
}
std::string build_test_row_56(std::uint16_t item_idx, std::uint16_t life_recover) {
    // 56 columns matching ItemList.bin common-row layout (D6.x field order).
    std::vector<std::string> toks(56u);
    toks[0] = std::to_string(item_idx);
    toks[1] = "HpPotion";
    toks[2] = "0";
    toks[3] = "0";
    toks[4] = "0";;  // ItemKind
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
        if (i != 0) row << "	";
        row << toks[i];
    }
    return row.str();
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
std::vector<std::uint8_t> synthesize_dealitem_bin(const std::string& text) {
    std::vector<std::uint8_t> payload(text.begin(), text.end());
    const auto encrypted = mxh::compat::encrypt_bin_payload(payload, 42);
    mxh::compat::MhFileHeader header{1, 42, static_cast<std::uint32_t>(payload.size())};
    std::vector<std::uint8_t> raw(sizeof(header) + 1 + encrypted.size() + 1);
    std::memcpy(raw.data(), &header, sizeof(header));
    std::copy(encrypted.begin(), encrypted.end(), raw.begin() + sizeof(header) + 1);
    return raw;
}
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

}  // namespace mxh::server::test
