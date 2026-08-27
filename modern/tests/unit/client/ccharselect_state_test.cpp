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

#include "mxh/proto/protocol.hpp"

using mxh::client::CharacterSlot;
using mxh::client::legacy_character_list_syn_payload;
using mxh::client::legacy_character_select_syn_payload;
using mxh::client::legacy_character_remove_syn_payload;
using mxh::client::legacy_character_disconnect_syn_message;
using mxh::client::parse_legacy_character_list_ack;
using mxh::client::parse_legacy_character_select_ack;

// -------------------------------------------------------------------------
// legacy_character_list_syn_payload — 1:1 with agent_handler.cpp:526-538
//   [user_id: u32 LE] [dist_auth_key: u32 LE] = 8 bytes
// -------------------------------------------------------------------------

TEST(CharSelectState, AutoSelectDisabledByDefault) {
    mxh::client::CCharSelectState state;
    EXPECT_FALSE(state.auto_select_for_test());
}

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

TEST(CharSelectWire, RemoveSynPayloadIsLegacyMsgDword) {
    const auto pl = legacy_character_remove_syn_payload(0x78563412u);
    ASSERT_EQ(pl.size(), 4u);
    EXPECT_EQ(pl[0], 0x12u);
    EXPECT_EQ(pl[1], 0x34u);
    EXPECT_EQ(pl[2], 0x56u);
    EXPECT_EQ(pl[3], 0x78u);
}

TEST(CharSelectWire, DisconnectSynIsEmptyLegacyMessage) {
    const auto message = legacy_character_disconnect_syn_message();
    EXPECT_EQ(message.header.category, static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn));
    EXPECT_EQ(message.header.protocol, static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::DisconnectSyn));
    EXPECT_EQ(message.header.object_id, 0u);
    EXPECT_TRUE(message.payload.empty());
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
    constexpr char kName[] = "InkHero";
    std::memcpy(buf.data() + 22, kName, sizeof(kName));

    auto list = parse_legacy_character_list_ack(
        std::span<const std::uint8_t>(buf.data(), buf.size()));
    ASSERT_TRUE(list.has_value());
    EXPECT_TRUE((*list)[0].valid);
    EXPECT_EQ((*list)[0].chrid, 42u);
    EXPECT_EQ((*list)[0].name, "InkHero");
    for (std::size_t i = 1; i < 5; ++i) {
        EXPECT_FALSE((*list)[i].valid);
        EXPECT_EQ((*list)[i].chrid, 0u);
    }
}

TEST(CharSelectWire, ListAckCarriesAppearanceAndMapData) {
    std::array<std::uint8_t, 889> buf{};
    buf[0] = 1;
    buf[14] = 0x2A;
    buf[189 + 16] = 1; // female
    buf[189 + 17] = 3; // face
    buf[189 + 18] = 4; // hair
    buf[189 + 19] = 0x34; buf[189 + 20] = 0x12;
    buf[189 + 40] = 12; // level
    buf[189 + 42] = 10; // map
    const auto list = parse_legacy_character_list_ack(buf);
    ASSERT_TRUE(list.has_value());
    EXPECT_EQ((*list)[0].gender, 1);
    EXPECT_EQ((*list)[0].face_type, 3);
    EXPECT_EQ((*list)[0].hair_type, 4);
    EXPECT_EQ((*list)[0].weared_item_idx[0], 0x1234);
    EXPECT_EQ((*list)[0].level, 12);
    EXPECT_EQ((*list)[0].map_num, 10);
}

TEST(CharSelectWire, CharacterPreviewUsesServerAppearance) {
    mxh::client::CharacterSlot slot;
    slot.valid = true;
    slot.chrid = 99;
    slot.gender = 1;
    slot.face_type = 2;
    slot.hair_type = 4;
    slot.weared_item_idx[3] = 777;
    const auto preview = mxh::client::make_character_preview(slot, 1.0f, 2.0f, 3.0f);
    ASSERT_TRUE(preview.has_value());
    EXPECT_EQ(preview->object_id, 99u);
    EXPECT_EQ(preview->gender, 1);
    EXPECT_EQ(preview->face_type, 2);
    EXPECT_EQ(preview->hair_type, 4);
    EXPECT_EQ(preview->weared_item_idx[3], 777);
    EXPECT_FLOAT_EQ(preview->world_x, 1.0f);
    EXPECT_FALSE(mxh::client::make_character_preview({}));
}

TEST(CharSelectWire, ListAckNameUsesAllSeventeenBytesWithoutTerminator) {
    std::array<std::uint8_t, 889> buf{};
    buf[0] = 1;
    buf[14] = 1;
    constexpr char kName[] = "12345678901234567";
    static_assert(sizeof(kName) - 1 == 17);
    std::memcpy(buf.data() + 22, kName, 17);

    auto list = parse_legacy_character_list_ack(buf);
    ASSERT_TRUE(list.has_value());
    EXPECT_EQ((*list)[0].name, kName);
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

TEST(CharSelectWire, ListAckHonorsAdvertisedCharacterCount) {
    std::array<std::uint8_t, 889> buf{};
    buf[0] = 1; // only slot zero is occupied according to the legacy header
    const std::uint32_t second_id = 99;
    std::memcpy(buf.data() + 14 + 35, &second_id, sizeof(second_id));
    const auto list = parse_legacy_character_list_ack(buf);
    ASSERT_TRUE(list.has_value());
    ASSERT_EQ(list->size(), 5u);
    EXPECT_FALSE((*list)[1].valid);
    EXPECT_EQ((*list)[1].chrid, 0u);
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

TEST(CharSelectSelection, RequiresLiveListedCharacter) {
    std::array<mxh::client::CharacterSlot, 2> slots{};
    slots[0].valid = true;
    slots[0].chrid = 42;
    slots[1].valid = false;
    slots[1].chrid = 99;
    EXPECT_TRUE(mxh::client::is_listed_character(slots, 42));
    EXPECT_FALSE(mxh::client::is_listed_character(slots, 99));
    EXPECT_FALSE(mxh::client::is_listed_character(slots, 0));
}

// -------------------------------------------------------------------------
// CCharSelectState default state — no Start, no list, no selection.
// -------------------------------------------------------------------------

TEST(CCharSelectStateDefaults, AllFieldsZero) {
    mxh::client::CCharSelectState s;
    EXPECT_FALSE(s.is_connected());
    EXPECT_FALSE(s.is_failed());
    EXPECT_TRUE(s.failure_reason().empty());
    EXPECT_EQ(s.selected_chrid(), 0u);
    EXPECT_EQ(s.selected_map(),  0u);
    EXPECT_TRUE(s.character_list().empty());
    EXPECT_FALSE(s.logout_pending());
    EXPECT_FALSE(s.RequestLogout());
}

TEST(CCharSelectState, CharacterListNackExposesRecoverableFailure) {
    mxh::client::CCharSelectState state;
    mxh::net::Message message;
    message.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    message.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::CharacterListNack);
    state.on_message({}, message);
    EXPECT_TRUE(state.is_failed());
    EXPECT_EQ(state.failure_reason(), "CharacterListNack received");
}

TEST(CharSelectUiCommand, ResolvesLegacyIdsAndFunctions) {
    using Kind = mxh::client::CharSelectUiCommandKind;
    mxh::client::ClientUiActivation activation;
    activation.legacy_id = "MT_THIRDCHOSEBTN";
    auto command = mxh::client::resolve_char_select_ui_command(activation);
    EXPECT_EQ(command.kind, Kind::SelectSlot);
    EXPECT_EQ(command.slot_index, 2u);

    activation = {};
    activation.legacy_func = "CS_BtnFuncCreateChar";
    EXPECT_EQ(mxh::client::resolve_char_select_ui_command(activation).kind,
              Kind::Create);
    activation.legacy_func = "CS_BtnFuncDeleteChar";
    EXPECT_EQ(mxh::client::resolve_char_select_ui_command(activation).kind,
              Kind::Delete);
    activation.legacy_func = "CS_BtnFuncLogOut";
    EXPECT_EQ(mxh::client::resolve_char_select_ui_command(activation).kind,
              Kind::Logout);
}

TEST(CCharSelectState, CharacterListDoesNotPreselectWithoutTestFlag) {
    mxh::client::CCharSelectState state;
    mxh::net::Message message;
    message.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    message.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::CharacterListAck);
    message.payload.resize(889);
    message.payload[0] = 1;
    message.payload[14] = 42;
    state.on_message({}, message);
    ASSERT_TRUE(state.has_character_list());
    EXPECT_EQ(state.selected_chrid(), 0u);
    ASSERT_TRUE(state.SelectSlot(0));
    EXPECT_EQ(state.selected_chrid(), 42u);
}

TEST(CCharSelectState, CharacterRemoveAckClearsSelectedSlot) {
    mxh::client::CCharSelectState state;
    mxh::net::Message list;
    list.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    list.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::CharacterListAck);
    list.payload.resize(889);
    list.payload[0] = 1;
    list.payload[14] = 42;
    state.on_message({}, list);
    ASSERT_TRUE(state.SelectSlot(0));

    mxh::net::Message ack;
    ack.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    ack.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::CharacterRemoveAck);
    state.on_message({}, ack);

    ASSERT_EQ(state.character_list().size(), 5u);
    EXPECT_FALSE(state.character_list()[0].valid);
    EXPECT_EQ(state.selected_chrid(), 0u);
    EXPECT_FALSE(state.deletion_pending());
}
