// mxh/tests/unit/client/cingame_state_test.cpp
// Unit tests for mxh::client::CInGameState (Phase B.2.3).
//
// Coverage:
//   * parse_legacy_gamein_ack — 3000-byte SEND_HERO_TOTALINFO layout
//     (matches map_handler.cpp make_gamein_ack).
//   * CInGameState default state.

#include "CInGameState.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstring>
#include <span>

using mxh::client::GameInInfo;
using mxh::client::parse_legacy_gamein_ack;

// -------------------------------------------------------------------------
// Helpers — small put_*() copies of map_handler.cpp's put_u32/put_u16 so
// we can construct realistic GameInAck payloads without linking the
// server side.
// -------------------------------------------------------------------------
namespace {
inline void put_u32(std::uint8_t* p, std::uint32_t v) {
    p[0] = static_cast<std::uint8_t>(v);
    p[1] = static_cast<std::uint8_t>(v >> 8);
    p[2] = static_cast<std::uint8_t>(v >> 16);
    p[3] = static_cast<std::uint8_t>(v >> 24);
}
inline void put_u16(std::uint8_t* p, std::uint16_t v) {
    p[0] = static_cast<std::uint8_t>(v);
    p[1] = static_cast<std::uint8_t>(v >> 8);
}
inline void put_str(std::uint8_t* p, const char* s, std::size_t n) {
    const std::size_t slen = std::strlen(s);
    const std::size_t copy = slen < n ? slen : n;
    if (copy) std::memcpy(p, s, copy);
    if (copy < n) std::memset(p + copy, 0, n - copy);
}
} // namespace

// -------------------------------------------------------------------------
// parse_legacy_gamein_ack — matches map_handler.cpp make_gamein_ack.
// -------------------------------------------------------------------------

TEST(InGameWire, GameInAckAllZerosDefault) {
    // 3000B zero-filled: player_id=0, name="", level=0, etc.
    std::array<std::uint8_t, 3000> buf{};
    auto info = parse_legacy_gamein_ack(
        std::span<const std::uint8_t>(buf.data(), buf.size()));
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->player_id, 0u);
    EXPECT_EQ(info->user_id,   0u);
    EXPECT_EQ(info->name,      "");
    EXPECT_EQ(info->level,     0u);
    EXPECT_EQ(info->map_num,   0u);
    EXPECT_EQ(info->life,      0u);
    EXPECT_EQ(info->max_life,  0u);
    EXPECT_EQ(info->gender,    0u);
}

TEST(InGameWire, GameInAckRealisticFields) {
    // Build a 3000B payload matching make_gamein_ack's writes:
    //   BASEOBJECT_INFO[0..35):
    //     +0  dwObjectID (u32) = 42
    //     +4  dwUserID   (u32) = 42
    //     +8  name       (char[17]) = "TestPlayer"
    //   CHARACTER_TOTALINFO[35..147):
    //     +0  Life (u32)    = 100
    //     +4  MaxLife (u32) = 200
    //     +16 gender (u8)   = 1
    //     +40 level (u16)   = 25
    //     +42 map_num (u16) = 12
    //   SYSTEMTIME ServerTime[2965..2981):
    //     +0  year (u16)  = 2026
    //     +2  month (u16) = 7
    //     +6  day (u16)   = 25
    //     +8  hour (u16)  = 14
    std::array<std::uint8_t, 3000> buf{};
    put_u32(buf.data() + 0,  42);          // player_id
    put_u32(buf.data() + 4,  42);          // user_id
    put_str(buf.data() + 8,  "TestPlayer", 17);
    put_u32(buf.data() + 35 + 0,  100);     // life
    put_u32(buf.data() + 35 + 4,  200);     // max_life
    buf[35 + 16] = 1;                      // gender
    put_u16(buf.data() + 35 + 40, 25);      // level
    put_u16(buf.data() + 35 + 42, 12);      // map_num
    put_u16(buf.data() + 2965 + 0, 2026);   // year
    put_u16(buf.data() + 2965 + 2, 7);      // month
    put_u16(buf.data() + 2965 + 6, 25);     // day
    put_u16(buf.data() + 2965 + 8, 14);     // hour

    auto info = parse_legacy_gamein_ack(
        std::span<const std::uint8_t>(buf.data(), buf.size()));
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->player_id, 42u);
    EXPECT_EQ(info->user_id,   42u);
    EXPECT_EQ(info->name,      "TestPlayer");
    EXPECT_EQ(info->life,      100u);
    EXPECT_EQ(info->max_life,  200u);
    EXPECT_EQ(info->gender,    1u);
    EXPECT_EQ(info->level,     25u);
    EXPECT_EQ(info->map_num,   12u);
    EXPECT_EQ(info->server_year,  2026u);
    EXPECT_EQ(info->server_month, 7u);
    EXPECT_EQ(info->server_day,   25u);
    EXPECT_EQ(info->server_hour,  14u);
}

TEST(InGameWire, GameInAckNameShortIsTruncated) {
    // 5-char name: parser must stop at the first NUL within 17B.
    std::array<std::uint8_t, 3000> buf{};
    put_str(buf.data() + 8, "Alice", 17);
    auto info = parse_legacy_gamein_ack(
        std::span<const std::uint8_t>(buf.data(), buf.size()));
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->name, "Alice");
}

TEST(InGameWire, GameInAckNameMaxLength) {
    // 17-char name (exactly fills the field).
    std::array<std::uint8_t, 3000> buf{};
    put_str(buf.data() + 8, "12345678901234567", 17);
    auto info = parse_legacy_gamein_ack(
        std::span<const std::uint8_t>(buf.data(), buf.size()));
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->name, "12345678901234567");
}

TEST(InGameWire, GameInAckTooShort) {
    std::array<std::uint8_t, 1000> buf{};
    auto info = parse_legacy_gamein_ack(
        std::span<const std::uint8_t>(buf.data(), buf.size()));
    EXPECT_FALSE(info.has_value());
}

// -------------------------------------------------------------------------
// CInGameState default state — no Start, no in-game, all fields zero.
// -------------------------------------------------------------------------

TEST(CInGameStateDefaults, AllFieldsZero) {
    mxh::client::CInGameState s;
    EXPECT_FALSE(s.is_connected());
    EXPECT_FALSE(s.is_in_game());
    EXPECT_EQ(s.player_id(), 0u);
    EXPECT_EQ(s.map_num(),   0u);
    EXPECT_EQ(s.game_info().name, "");
    EXPECT_EQ(s.game_info().level, 0u);
}
