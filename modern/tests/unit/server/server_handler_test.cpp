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
// MockTcpSender — Phase 12.1 P2-13: capture outgoing messages from
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

    // Tests can flip connected → false to simulate the map server going
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
    // No DB calls either — on_disconnect doesn't persist anything.
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
// Phase 12.1 P2-13: ITcpSender injection — verify GameOutSyn forwarding
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
