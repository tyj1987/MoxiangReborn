// mxh/tests/unit/client/ccharselect_state_test.cpp
// Unit tests for mxh::client::CCharSelectState (Phase B.2.2).
//
// Coverage:
//   * legacy_character_list_syn_payload — 8B wire format.
//   * legacy_character_select_syn_payload — 2B wire format.
//   * parse_legacy_character_list_ack — 889B layout (no _CRYPTCHECK_).
//   * parse_legacy_character_select_ack — 1B map number.
//   * CCharSelectState default state.

#include "CCharSelectState.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstring>
#include <span>

using mxh::client::CharacterSlot;
using mxh::client::legacy_character_list_syn_payload;
using mxh::client::legacy_character_select_syn_payload;
using mxh::client::parse_legacy_character_list_ack;
using mxh::client::parse_legacy_character_select_ack;

// -------------------------------------------------------------------------
// legacy_character_list_syn_payload — 1:1 with agent_handler.cpp:526-538
//   [user_id: u32 LE] [dist_auth_key: u32 LE] = 8 bytes
// -------------------------------------------------------------------------

TEST(CharSelectWire, ListSynPayloadShape) {
    const auto pl = legacy_character_list_syn_payload(0x01020304u, 0xDEADBEEFu);
    ASSERT_EQ(pl.size(), 8u);

    // user_id at offset 0, little-endian.
    EXPECT_EQ(pl[0], 0x04u);
    EXPECT_EQ(pl[1], 0x03u);
    EXPECT_EQ(pl[2], 0x02u);
    EXPECT_EQ(pl[3], 0x01u);

    // dist_auth_key at offset 4, little-endian.
    EXPECT_EQ(pl[4], 0xEFu);
    EXPECT_EQ(pl[5], 0xBEu);
    EXPECT_EQ(pl[6], 0xADu);
    EXPECT_EQ(pl[7], 0xDEu);
}

TEST(CharSelectWire, ListSynPayloadZeros) {
    const auto pl = legacy_character_list_syn_payload(0u, 0u);
    for (auto b : pl) EXPECT_EQ(b, 0u);
}

// -------------------------------------------------------------------------
// legacy_character_select_syn_payload — minimal 2B (channel=0)
// -------------------------------------------------------------------------

TEST(CharSelectWire, SelectSynPayloadShape) {
    const auto pl = legacy_character_select_syn_payload(0u);
    ASSERT_EQ(pl.size(), 2u);
    EXPECT_EQ(pl[0], 0u);
    EXPECT_EQ(pl[1], 0u);
}

TEST(CharSelectWire, SelectSynPayloadNonzeroChannel) {
    const auto pl = legacy_character_select_syn_payload(0x0102u);
    EXPECT_EQ(pl[0], 0x02u);
    EXPECT_EQ(pl[1], 0x01u);
}

// -------------------------------------------------------------------------
// parse_legacy_character_list_ack — 1:1 with agent_handler.cpp lines
// 593-684 (no _CRYPTCHECK_ in CHINA locale, kMaxCharSlots=5).
//   [0..4)    CharNum (i32 LE)
//   [4..14)   StandingArrayNum[5]
//   [14..189) BaseObjectInfo[5]   (5 * 35B, chrid first 4B per slot)
//   [189..889) ChrTotalInfo[5]   (5 * 140B)
// = 889 bytes total
// -------------------------------------------------------------------------

TEST(CharSelectWire, ListAckEmptyList) {
    // 889B with char_count = 0; all chrid fields are 0, no slot valid.
    std::array<std::uint8_t, 889> buf{};
    auto list = parse_legacy_character_list_ack(
        std::span<const std::uint8_t>(buf.data(), buf.size()));
    ASSERT_TRUE(list.has_value());
    ASSERT_EQ(list->size(), 5u);
    for (const auto& slot : *list) {
        EXPECT_FALSE(slot.valid);
        EXPECT_EQ(slot.chrid, 0u);
    }
}

TEST(CharSelectWire, ListAckSingleChar) {
    // 889B with char_count = 1; only slot 0 has a non-zero chrid.
    std::array<std::uint8_t, 889> buf{};
    // CharNum = 1 LE.
    buf[0] = 0x01u; buf[1] = 0x00u; buf[2] = 0x00u; buf[3] = 0x00u;
    // BaseObjectInfo[0].chrid = 42 LE (offset 14 + 0 = 14).
    buf[14] = 0x2Au; buf[15] = 0x00u; buf[16] = 0x00u; buf[17] = 0x00u;

    auto list = parse_legacy_character_list_ack(
        std::span<const std::uint8_t>(buf.data(), buf.size()));
    ASSERT_TRUE(list.has_value());
    EXPECT_TRUE((*list)[0].valid);
    EXPECT_EQ((*list)[0].chrid, 42u);
    for (std::size_t i = 1; i < 5; ++i) {
        EXPECT_FALSE((*list)[i].valid);
        EXPECT_EQ((*list)[i].chrid, 0u);
    }
}

TEST(CharSelectWire, ListAckThreeChars) {
    std::array<std::uint8_t, 889> buf{};
    buf[0] = 0x03u;  // CharNum = 3
    // chrid values: 100, 200, 300 in slots 0, 1, 2.
    auto put_chrid = [&buf](std::size_t slot, std::uint32_t v) {
        const std::size_t off = 14 + slot * 35;
        buf[off + 0] = static_cast<std::uint8_t>(v & 0xFF);
        buf[off + 1] = static_cast<std::uint8_t>((v >> 8) & 0xFF);
        buf[off + 2] = static_cast<std::uint8_t>((v >> 16) & 0xFF);
        buf[off + 3] = static_cast<std::uint8_t>((v >> 24) & 0xFF);
    };
    put_chrid(0, 100);
    put_chrid(1, 200);
    put_chrid(2, 300);

    auto list = parse_legacy_character_list_ack(
        std::span<const std::uint8_t>(buf.data(), buf.size()));
    ASSERT_TRUE(list.has_value());
    EXPECT_TRUE((*list)[0].valid);  EXPECT_EQ((*list)[0].chrid, 100u);
    EXPECT_TRUE((*list)[1].valid);  EXPECT_EQ((*list)[1].chrid, 200u);
    EXPECT_TRUE((*list)[2].valid);  EXPECT_EQ((*list)[2].chrid, 300u);
    EXPECT_FALSE((*list)[3].valid);
    EXPECT_FALSE((*list)[4].valid);
}

TEST(CharSelectWire, ListAckTooShort) {
    std::array<std::uint8_t, 3> buf{};
    auto list = parse_legacy_character_list_ack(
        std::span<const std::uint8_t>(buf.data(), buf.size()));
    EXPECT_FALSE(list.has_value());
}

TEST(CharSelectWire, ListAckTruncatedSlotsStillParsed) {
    // 100B: long enough to declare char_count but shorter than the full
    // 889B layout.  Parser should defensively read what's available.
    std::array<std::uint8_t, 100> buf{};
    buf[0] = 0x05u;  // CharNum = 5
    auto list = parse_legacy_character_list_ack(
        std::span<const std::uint8_t>(buf.data(), buf.size()));
    ASSERT_TRUE(list.has_value());
    EXPECT_EQ(list->size(), 5u);
    // With only 100B, the parser can read at most 2 complete slots
    // (14 + 2*35 = 84 ≤ 100), so slots 0, 1 get default zero chrid
    // (no .valid) and slots 2-4 are also zero.
    for (const auto& slot : *list) {
        EXPECT_FALSE(slot.valid);
        EXPECT_EQ(slot.chrid, 0u);
    }
}

// -------------------------------------------------------------------------
// parse_legacy_character_select_ack — 1B map number.
// -------------------------------------------------------------------------

TEST(CharSelectWire, SelectAckMapNumber) {
    std::array<std::uint8_t, 1> buf{ 12u };
    auto map = parse_legacy_character_select_ack(
        std::span<const std::uint8_t>(buf.data(), buf.size()));
    ASSERT_TRUE(map.has_value());
    EXPECT_EQ(*map, 12u);
}

TEST(CharSelectWire, SelectAckEmptyPayload) {
    auto map = parse_legacy_character_select_ack({});
    EXPECT_FALSE(map.has_value());
}

// -------------------------------------------------------------------------
// CCharSelectState default state — no Start, no list, no selection.
// -------------------------------------------------------------------------

TEST(CCharSelectStateDefaults, AllFieldsZero) {
    mxh::client::CCharSelectState s;
    EXPECT_FALSE(s.is_connected());
    EXPECT_EQ(s.selected_chrid(), 0u);
    EXPECT_EQ(s.selected_map(),  0u);
    EXPECT_TRUE(s.character_list().empty());
}
