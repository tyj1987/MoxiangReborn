// message_test.cpp - Phase 10.16 net message + connection types
//
// Covers the wire-format + connection-id types in
// modern/include/mxh/net/net.hpp:
//   - MsgHeader (8 bytes, already pinned by version_test.cpp's
//     MsgHeaderLayout but we add a dedicated test here that
//     exercises the field layout, not just sizeof)
//   - MsgRoot (4 bytes, similarly re-pinned)
//   - Message (header + payload, with total_size())
//   - ConnectionId (opaque 64-bit id, valid(), comparison)
//   - NetError enum values + to_string
//   - ServerConfig / ClientConfig default values
//
// What is NOT covered here:
//   - TcpServer / TcpClient full lifecycle (start/stop/send) —
//     those need a real listening socket + worker thread and are
//     exercised by the net_test.cpp + socket_test.cpp integration
//     tests.
//   - IConnectionHandler dispatch — covered by server/*.cpp's
//     login/agent/map handler tests, not here.
//   - IEncryptor concrete impls (HSEL / AES-GCM) — covered by
//     hsel_stream_test.cpp + aes_gcm_test.cpp.

#include "mxh/net/net.hpp"

#include <gtest/gtest.h>

#include <type_traits>

namespace mxh::net::test {

// ===========================================================================
// MsgHeader wire format
// ===========================================================================

TEST(MsgHeaderTest, WireFormatSizeIs8Bytes) {
    // 1 + 1 + 1 + 1 + 4 = 8 bytes. The struct uses #pragma pack(push, 1)
    // so no padding. Pinning here catches a future change that drops
    // the pragma or adds a field.
    EXPECT_EQ(sizeof(mxh::net::MsgHeader), 8u);
}

TEST(MsgHeaderTest, FieldsAreAccessibleAndSettable) {
    mxh::net::MsgHeader h{};
    h.checksum = 0xAB;
    h.code = -1;
    h.category = 7;     // UserConn
    h.protocol = 5;     // NotifyUserLoginAck
    h.object_id = 0xDEADBEEFu;
    EXPECT_EQ(h.checksum, 0xABu);
    EXPECT_EQ(h.code, -1);
    EXPECT_EQ(h.category, 7u);
    EXPECT_EQ(h.protocol, 5u);
    EXPECT_EQ(h.object_id, 0xDEADBEEFu);
}

TEST(MsgHeaderTest, DefaultConstructionIsZeroed) {
    mxh::net::MsgHeader h{};
    EXPECT_EQ(h.checksum, 0u);
    EXPECT_EQ(h.code, 0);
    EXPECT_EQ(h.category, 0u);
    EXPECT_EQ(h.protocol, 0u);
    EXPECT_EQ(h.object_id, 0u);
}

// ===========================================================================
// MsgRoot wire format
// ===========================================================================

TEST(MsgRootTest, WireFormatSizeIs4Bytes) {
    // 1 + 1 + 1 + 1 = 4 bytes. MSGROOT in the legacy protocol is the
    // first 4 bytes of MSGBASE (no object_id).
    EXPECT_EQ(sizeof(mxh::net::MsgRoot), 4u);
}

TEST(MsgRootTest, FieldsAreAccessibleAndSettable) {
    mxh::net::MsgRoot r{};
    r.checksum = 0xCD;
    r.code = 42;
    r.category = 6;     // Chat
    r.protocol = 1;
    EXPECT_EQ(r.checksum, 0xCDu);
    EXPECT_EQ(r.code, 42);
    EXPECT_EQ(r.category, 6u);
    EXPECT_EQ(r.protocol, 1u);
}

// ===========================================================================
// Message + total_size
// ===========================================================================

TEST(MessageTest, DefaultConstructionIsEmptyHeaderAndEmptyPayload) {
    mxh::net::Message m;
    EXPECT_EQ(m.header.checksum, 0u);
    EXPECT_EQ(m.header.object_id, 0u);
    EXPECT_TRUE(m.payload.empty());
}

TEST(MessageTest, TotalSizeIsHeaderPlusPayload) {
    // Empty payload: 8 bytes (just the header).
    mxh::net::Message m;
    EXPECT_EQ(m.total_size(), sizeof(mxh::net::MsgHeader));
    EXPECT_EQ(m.total_size(), 8u);

    // 100-byte payload: 108 bytes.
    m.payload.resize(100, 0xCC);
    EXPECT_EQ(m.total_size(), 108u);

    // 65536-byte payload: 65544 bytes.
    m.payload.resize(65536, 0x00);
    EXPECT_EQ(m.total_size(), 65536u + sizeof(mxh::net::MsgHeader));
}

TEST(MessageTest, PayloadIsByteExact) {
    // The payload is std::vector<std::uint8_t>, so a sequence of bytes
    // round-trips exactly. No reordering, no padding.
    mxh::net::Message m;
    m.payload = {0x01, 0x02, 0x03, 0xFF, 0xFE, 0xFD};
    ASSERT_EQ(m.payload.size(), 6u);
    EXPECT_EQ(m.payload[0], 0x01u);
    EXPECT_EQ(m.payload[1], 0x02u);
    EXPECT_EQ(m.payload[2], 0x03u);
    EXPECT_EQ(m.payload[3], 0xFFu);
    EXPECT_EQ(m.payload[4], 0xFEu);
    EXPECT_EQ(m.payload[5], 0xFDu);
}

// ===========================================================================
// ConnectionId
// ===========================================================================

TEST(ConnectionIdTest, DefaultConstructionIsInvalid) {
    mxh::net::ConnectionId id;
    EXPECT_FALSE(id.valid());
    EXPECT_FALSE(static_cast<bool>(id));
    EXPECT_EQ(id.value, 0u);
}

TEST(ConnectionIdTest, NonZeroIsValid) {
    mxh::net::ConnectionId id = mxh::net::make_connection_id(42);
    EXPECT_TRUE(id.valid());
    EXPECT_TRUE(static_cast<bool>(id));
    EXPECT_EQ(id.value, 42u);
}

TEST(ConnectionIdTest, EqualityIsValueBased) {
    mxh::net::ConnectionId a = mxh::net::make_connection_id(100);
    mxh::net::ConnectionId b = mxh::net::make_connection_id(100);
    mxh::net::ConnectionId c = mxh::net::make_connection_id(200);
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
    EXPECT_FALSE(a < b);  // equal → neither less-than
    EXPECT_TRUE(a < c);
}

TEST(ConnectionIdTest, InvalidConnectionIdIsZero) {
    EXPECT_EQ(mxh::net::kInvalidConnectionId.value, 0u);
    EXPECT_FALSE(mxh::net::kInvalidConnectionId.valid());
}

TEST(ConnectionIdTest, UnderlyingTypeIsUint64) {
    // ConnectionId is a thin wrapper over uint64_t. Pinning the
    // underlying type so a future change to a wider type (e.g. to
    // add a thread-id or session-id) is caught here.
    static_assert(std::is_same_v<decltype(std::declval<mxh::net::ConnectionId>().value),
                                 std::uint64_t>);
    SUCCEED();
}

// ===========================================================================
// NetError enum
// ===========================================================================

TEST(NetErrorTest, OkIsZero) {
    // Ok = 0 is the convention so callers can write `if (e == NetError::Ok)`.
    EXPECT_EQ(static_cast<int>(mxh::net::NetError::Ok), 0);
}

TEST(NetErrorTest, ToStringReturnsNonEmptyForAllValues) {
    // to_string must never return nullptr or empty; it has a fallback
    // for unknown enum values.
    for (int i = 0; i <= static_cast<int>(mxh::net::NetError::Unknown); ++i) {
        auto e = static_cast<mxh::net::NetError>(i);
        const char* s = mxh::net::to_string(e);
        ASSERT_NE(s, nullptr) << "to_string returned nullptr for NetError(" << i << ")";
        EXPECT_GT(std::char_traits<char>::length(s), 0u)
            << "to_string returned empty for NetError(" << i << ")";
    }
}

TEST(NetErrorTest, ToStringOkReturnsOkString) {
    // The exact string is "Ok" or "ok" depending on the implementation;
    // we don't pin the case to keep the test robust to cosmetic
    // changes. The key invariant is that the string mentions "ok"
    // (case-insensitive) so log output is searchable.
    std::string s = mxh::net::to_string(mxh::net::NetError::Ok);
    std::string lower;
    for (char c : s) lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    EXPECT_NE(lower.find("ok"), std::string::npos) << "got: " << s;
}

TEST(NetErrorTest, ToStringFallbackForUnknownValue) {
    // Cast an out-of-range integer to NetError; to_string must not
    // return nullptr or crash. It should return something like
    // "Unknown(<n>)".
    auto bad = static_cast<mxh::net::NetError>(99999);
    const char* s = mxh::net::to_string(bad);
    ASSERT_NE(s, nullptr);
    EXPECT_GT(std::char_traits<char>::length(s), 0u);
}

// ===========================================================================
// ServerConfig defaults
// ===========================================================================

TEST(ServerConfigTest, DefaultsAreReasonable) {
    // The default config should bind to all interfaces on port 6001
    // (the conventional Moxian login port), with 4 worker threads
    // and reasonable buffer sizes. A future change to these
    // defaults shows up here.
    mxh::net::ServerConfig c;
    EXPECT_EQ(c.bind_address, "0.0.0.0");
    EXPECT_EQ(c.port, 6001);
    EXPECT_EQ(c.worker_thread_count, 4);
    EXPECT_EQ(c.max_connections, 4096);
    EXPECT_EQ(c.recv_buffer_size, 65536);
    EXPECT_EQ(c.send_buffer_size, 65536);
    EXPECT_FALSE(c.use_encryption);
    EXPECT_FALSE(c.use_legacy_framing);
    EXPECT_EQ(c.idle_timeout, std::chrono::milliseconds(120000));
}

TEST(ServerConfigTest, FieldsAreSettable) {
    mxh::net::ServerConfig c;
    c.bind_address = "127.0.0.1";
    c.port = 7000;
    c.worker_thread_count = 8;
    c.max_connections = 1024;
    c.use_encryption = true;
    c.use_legacy_framing = true;
    c.idle_timeout = std::chrono::milliseconds(60000);
    EXPECT_EQ(c.bind_address, "127.0.0.1");
    EXPECT_EQ(c.port, 7000);
    EXPECT_EQ(c.worker_thread_count, 8);
    EXPECT_EQ(c.max_connections, 1024);
    EXPECT_TRUE(c.use_encryption);
    EXPECT_TRUE(c.use_legacy_framing);
    EXPECT_EQ(c.idle_timeout, std::chrono::milliseconds(60000));
}

// ===========================================================================
// ClientConfig defaults
// ===========================================================================

TEST(ClientConfigTest, DefaultsAreReasonable) {
    // The default client config has port=0 (caller must set), no
    // encryption, and a 5s connect timeout.
    mxh::net::ClientConfig c;
    EXPECT_EQ(c.port, 0u);
    EXPECT_FALSE(c.use_encryption);
    EXPECT_FALSE(c.use_legacy_framing);
    EXPECT_EQ(c.connect_timeout, std::chrono::milliseconds(5000));
}

TEST(ClientConfigTest, RemoteAddressIsSettable) {
    mxh::net::ClientConfig c;
    c.remote_address = "192.168.1.100";
    c.port = 6001;
    EXPECT_EQ(c.remote_address, "192.168.1.100");
    EXPECT_EQ(c.port, 6001);
}

}  // namespace mxh::net::test
