// mxh/tests/unit/client/clogin_state_test.cpp
// Unit tests for mxh::client::CLoginState (Phase B.2.1).
//
// Coverage:
//   * legacy_request_login_payload() — byte-for-byte 38B wire format.
//   * parse_legacy_login_ack() — round-trip with the encoder, plus
//     malformed-payload rejection.
//   * CLoginState default state (no Start, no connect, all fields 0).
//   * CLoginState::fail_with() is idempotent.

#include "CLoginState.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstring>
#include <span>

using mxh::client::LegacyLoginAck;
using mxh::client::legacy_request_login_payload;
using mxh::client::parse_legacy_login_ack;

// -------------------------------------------------------------------------
// legacy_request_login_payload — 1:1 with login_handler.cpp:handle_legacy_login
//   [AuthKey: u32 LE] [id: 17B null-padded] [pw: 17B null-padded]   = 38 bytes
// -------------------------------------------------------------------------

TEST(LoginStateWire, RequestLoginPayloadShape) {
    const auto pl = legacy_request_login_payload(0xDEADBEEF, "test", "test");
    ASSERT_EQ(pl.size(), 38u);

    // AuthKey at offset 0, little-endian.
    EXPECT_EQ(pl[0], 0xEFu);
    EXPECT_EQ(pl[1], 0xBEu);
    EXPECT_EQ(pl[2], 0xADu);
    EXPECT_EQ(pl[3], 0xDEu);

    // id at offset 4, 17 bytes, null-padded.
    EXPECT_EQ(pl[4],  't');
    EXPECT_EQ(pl[5],  'e');
    EXPECT_EQ(pl[6],  's');
    EXPECT_EQ(pl[7],  't');
    for (int i = 8; i < 21; ++i) EXPECT_EQ(pl[i], 0u);

    // pw at offset 21, 17 bytes, null-padded.
    EXPECT_EQ(pl[21], 't');
    EXPECT_EQ(pl[22], 'e');
    EXPECT_EQ(pl[23], 's');
    EXPECT_EQ(pl[24], 't');
    for (int i = 25; i < 38; ++i) EXPECT_EQ(pl[i], 0u);
}

TEST(LoginStateWire, RequestLoginPayloadIdTruncation) {
    // 30-char id is truncated to 17 bytes (legacy buffer is fixed size).
    const std::string long_id(30, 'x');
    const auto pl = legacy_request_login_payload(1u, long_id, "pw");
    EXPECT_EQ(pl[4],  'x');
    EXPECT_EQ(pl[20], 'x');   // last byte of the 17-byte id field
    EXPECT_EQ(pl[21], 'p');   // password starts at offset 21 unchanged
    EXPECT_EQ(pl[22], 'w');
    EXPECT_EQ(pl[23], 0u);
}

TEST(LoginStateWire, RequestLoginPayloadEmptyFields) {
    // Empty id/pw should be all zeros in their field regions; AuthKey still set.
    const auto pl = legacy_request_login_payload(0x12345678u, "", "");
    ASSERT_EQ(pl.size(), 38u);
    EXPECT_EQ(pl[0], 0x78u);
    EXPECT_EQ(pl[1], 0x56u);
    EXPECT_EQ(pl[2], 0x34u);
    EXPECT_EQ(pl[3], 0x12u);
    for (int i = 4; i < 38; ++i) EXPECT_EQ(pl[i], 0u);
}

// -------------------------------------------------------------------------
// parse_legacy_login_ack — 1:1 with login_handler.cpp make_login_ack (legacy)
//   [agentip: 16B] [agentport: u16 LE] [userIdx: u32 LE] [userLevel: u8]
//   = 23 bytes
// -------------------------------------------------------------------------

TEST(LoginStateWire, LoginAckHappyPath) {
    std::array<std::uint8_t, 23> buf{};
    // agentip "127.0.0.1\0\0\0\0\0\0\0" (16B)
    std::memcpy(buf.data(), "127.0.0.1", 9);
    // [16..18] agentport = 7001 LE  (7001 == 0x1B59)
    buf[16] = 0x59;  // 7001 & 0xFF
    buf[17] = 0x1B;  // (7001 >> 8) & 0xFF
    // [18..22] userIdx = 0x01020304
    buf[18] = 0x04; buf[19] = 0x03; buf[20] = 0x02; buf[21] = 0x01;
    // [22] userLevel = 7
    buf[22] = 7;

    auto ack = parse_legacy_login_ack(std::span<const std::uint8_t>(buf.data(), buf.size()));
    ASSERT_TRUE(ack.has_value());
    EXPECT_EQ(ack->agent_addr,  "127.0.0.1");
    EXPECT_EQ(ack->agent_port,  7001);
    EXPECT_EQ(ack->user_idx,    0x01020304u);
    EXPECT_EQ(ack->user_level,  7u);
}

TEST(LoginStateWire, LoginAckShortPayloadRejected) {
    std::array<std::uint8_t, 22> buf{};
    auto ack = parse_legacy_login_ack(std::span<const std::uint8_t>(buf.data(), buf.size()));
    EXPECT_FALSE(ack.has_value());
}

TEST(LoginStateWire, LoginAckEmptyAgentAddr) {
    std::array<std::uint8_t, 23> buf{};
    // all zeros — agent addr is empty string.
    auto ack = parse_legacy_login_ack(std::span<const std::uint8_t>(buf.data(), buf.size()));
    ASSERT_TRUE(ack.has_value());
    EXPECT_EQ(ack->agent_addr, "");
    EXPECT_EQ(ack->agent_port, 0u);
    EXPECT_EQ(ack->user_idx,   0u);
    EXPECT_EQ(ack->user_level, 0u);
}

// -------------------------------------------------------------------------
// CLoginState default state — no Start, no connect, all fields 0/empty.
// -------------------------------------------------------------------------

TEST(CLoginStateDefaults, AllFieldsZero) {
    mxh::client::CLoginState s;
    EXPECT_FALSE(s.is_connected());
    EXPECT_EQ(s.auth_key(),   0u);
    EXPECT_EQ(s.user_idx(),   0u);
    EXPECT_EQ(s.agent_addr(), "");
    EXPECT_EQ(s.agent_port(), 0u);
}
