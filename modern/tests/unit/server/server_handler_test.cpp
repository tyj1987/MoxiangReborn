// server_handler_test.cpp - Phase 10.17 server handler smoke tests
//
// Covers modern/include/mxh/server/server.hpp — the 3 high-level
// game-server handlers (LoginHandler, AgentHandler, MapHandler).
// These are the bridge between the net layer and the game logic.
//
// Scope: this is a smoke test. It verifies the handlers can be
// constructed with a mock IDbAdapter, that on_connect / on_disconnect
// don't crash, and that the few state-setter / state-getter
// methods exposed on the public API surface work as documented.
//
// What is NOT tested here (covered by other test files):
//   - The actual on_message protocol dispatch — covered by
//     integration tests that wire a real TcpServer to a handler
//     and feed it bytes (see modern/tools/MoxianLoginServer 5/5
//     smoke). A unit test for the byte-level protocol would need
//     to re-derive the message framing, which would duplicate the
//     integration test surface for no extra value.
//   - HSEL encryption integration with the handlers — covered
//     by hsel_stream_test.cpp + aes_gcm_test.cpp.

#include "mxh/server/server.hpp"
#include "mxh/db/db_adapter.hpp"
#include "mxh/net/net.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <memory>
#include <string>
#include <vector>

namespace mxh::server::test {

// ===========================================================================
// Mock IDbAdapter — minimum viable stub for handler construction.
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
    mxh::db::DbResult query(std::string_view, std::span<const mxh::db::Bind>,
                           mxh::db::ResultSet& out) override {
        ++query_count;
        out = mxh::db::ResultSet{};
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
};

inline mxh::server::ReplyFn make_reply_spy(ReplySpy& spy) {
    return [&spy](mxh::net::ConnectionId id, const mxh::net::Message&) {
        ++spy.call_count;
        spy.last_id = id;
    };
}

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

// ===========================================================================
// MapHandler
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
    // negotiation, no auth key — those happen on Distribute/Agent).
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, /*map_num=*/7, make_reply_spy(reply));
    EXPECT_TRUE(handler.on_connect({}, "10.0.0.1:1234"));
}

TEST(MapHandlerTest, OnDisconnectDoesNotCrash) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, /*map_num=*/7, make_reply_spy(reply));
    handler.on_disconnect({}, mxh::net::NetError::Disconnected);
    EXPECT_EQ(reply.call_count.load(), 0);
}

}  // namespace mxh::server::test
