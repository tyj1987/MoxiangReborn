// mxh/tests/unit/client/cchar_make_state_test.cpp
// Unit tests for mxh::client::CCharMake (Phase B.4).
//
// Coverage:
//   * legacy_character_make_syn_payload - 59B CHARACTERMAKEINFO layout,
//     1:1 with agent_handler.cpp::handle_legacy_character_make offsets
//     and the legacy CommonStruct.h CHARACTERMAKEINFO.
//   * CCharMake default state + lifecycle.

#include "CCharMake.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstring>

using mxh::client::CharacterMakeParams;
using mxh::client::CCharMake;
using mxh::client::legacy_character_make_syn_payload;

// -------------------------------------------------------------------------
// legacy_character_make_syn_payload - 1:1 with CommonStruct.h
// CHARACTERMAKEINFO (59 bytes after MSGBASE):
//   [0..17)  Name[17]
//   [17..21) UserID (u32 LE)
//   [21]     SexType
//   [22]     BodyType
//   [23]     HairType
//   [24]     FaceType
//   [25]     StartArea
//   [26..30) bDuplCheck (u32 LE) = FALSE
//   [30..50) WearedItemIdx[10] (u16 LE) = 0
//   [50]     StandingArrayNum = 0xFF (legacy sends -1)
//   [51..55) Height (f32 LE)
//   [55..59) Width (f32 LE)
// -------------------------------------------------------------------------

TEST(CharMakeWire, PayloadShape) {
    CharacterMakeParams p;
    p.name       = "Hero";
    p.sex_type   = 1;
    p.body_type  = 2;
    p.hair_type  = 3;
    p.face_type  = 4;
    p.start_area = 18;
    p.height     = 1.0f;
    p.width      = 0.9f;

    const auto pl = legacy_character_make_syn_payload(p, 0x01020304u);
    ASSERT_EQ(pl.size(), 59u);

    // Name at [0..17), NUL-padded.
    EXPECT_EQ(std::memcmp(pl.data(), "Hero", 4), 0);
    EXPECT_EQ(pl[4], 0u);
    EXPECT_EQ(pl[16], 0u);

    // UserID little-endian at [17..21).
    EXPECT_EQ(pl[17], 0x04u);
    EXPECT_EQ(pl[18], 0x03u);
    EXPECT_EQ(pl[19], 0x02u);
    EXPECT_EQ(pl[20], 0x01u);

    // Appearance bytes at [21..26).
    EXPECT_EQ(pl[21], 1u);
    EXPECT_EQ(pl[22], 2u);
    EXPECT_EQ(pl[23], 3u);
    EXPECT_EQ(pl[24], 4u);
    EXPECT_EQ(pl[25], 18u);

    // bDuplCheck = FALSE at [26..30).
    for (std::size_t i = 26; i < 30; ++i) EXPECT_EQ(pl[i], 0u);

    // WearedItemIdx[10] all zero at [30..50).
    for (std::size_t i = 30; i < 50; ++i) EXPECT_EQ(pl[i], 0u);

    // StandingArrayNum = 0xFF (legacy client sends -1).
    EXPECT_EQ(pl[50], 0xFFu);

    // Height = 1.0f at [51..55): 0x3F800000 LE.
    EXPECT_EQ(pl[51], 0x00u);
    EXPECT_EQ(pl[52], 0x00u);
    EXPECT_EQ(pl[53], 0x80u);
    EXPECT_EQ(pl[54], 0x3Fu);

    // Width = 0.9f at [55..59): 0x3F666666 LE.
    EXPECT_EQ(pl[55], 0x66u);
    EXPECT_EQ(pl[56], 0x66u);
    EXPECT_EQ(pl[57], 0x66u);
    EXPECT_EQ(pl[58], 0x3Fu);
}

TEST(CharMakeWire, PayloadZeros) {
    CharacterMakeParams p;
    p.name      = "";
    p.height    = 0.0f;
    p.width     = 0.0f;
    const auto pl = legacy_character_make_syn_payload(p, 0u);
    ASSERT_EQ(pl.size(), 59u);
    // Everything zero except the legacy StandingArrayNum=-1 sentinel.
    for (std::size_t i = 0; i < pl.size(); ++i) {
        if (i == 50) EXPECT_EQ(pl[i], 0xFFu);
        else         EXPECT_EQ(pl[i], 0u);
    }
}

TEST(CharMakeWire, NameTruncatedTo16Chars) {
    CharacterMakeParams p;
    p.name = "ThisNameIsDefinitelyLongerThanSixteenCharacters";
    const auto pl = legacy_character_make_syn_payload(p, 0u);
    ASSERT_EQ(pl.size(), 59u);
    EXPECT_EQ(std::memcmp(pl.data(), "ThisNameIsDefinit", 16), 0);
    // [16] must be NUL so the 17-byte field stays terminated.
    EXPECT_EQ(pl[16], 0u);
}

TEST(CharMakeWire, Exact16CharNameFillsField) {
    CharacterMakeParams p;
    p.name = "1234567890ABCDEF";  // exactly 16
    const auto pl = legacy_character_make_syn_payload(p, 0u);
    ASSERT_EQ(pl.size(), 59u);
    EXPECT_EQ(std::memcmp(pl.data(), "1234567890ABCDEF", 16), 0);
    EXPECT_EQ(pl[16], 0u);
}

TEST(CharMakeWire, AppearanceBoundaries) {
    CharacterMakeParams p;
    p.sex_type  = 0;  // male
    p.hair_type = 4;  // max valid
    p.face_type = 4;  // max valid
    const auto pl = legacy_character_make_syn_payload(p, 0xDEADBEEFu);
    ASSERT_EQ(pl.size(), 59u);
    EXPECT_EQ(pl[21], 0u);
    EXPECT_EQ(pl[23], 4u);
    EXPECT_EQ(pl[24], 4u);
    EXPECT_EQ(pl[17], 0xEFu);  // user id LE
    EXPECT_EQ(pl[20], 0xDEu);
}

// -------------------------------------------------------------------------
// CCharMake state lifecycle (mirrors the shared stub contract).
// -------------------------------------------------------------------------

TEST(CCharMake, DefaultState) {
    CCharMake s;
    EXPECT_FALSE(s.isInitialized());
    EXPECT_FALSE(s.is_connected());
    EXPECT_FALSE(s.is_submitted());
    EXPECT_FALSE(s.is_failed());
    EXPECT_TRUE(s.failure_reason().empty());
}

TEST(CCharMake, Lifecycle) {
    CCharMake s;
    s.Init(nullptr);
    EXPECT_TRUE(s.isInitialized());
    s.Process();  // must not crash
    s.Release();
    EXPECT_FALSE(s.isInitialized());
    EXPECT_FALSE(s.is_submitted());
    EXPECT_FALSE(s.is_failed());
}

TEST(CCharMake, SubmitBeforeConnectFails) {
    CCharMake s;
    s.Init(nullptr);
    mxh::client::LoginResult lr;
    lr.agent_addr = "127.0.0.1";
    lr.agent_port = 1;  // nothing listens here; connect stays pending
    s.SetLoginResult(lr);
    s.Start(nullptr, false);
    CharacterMakeParams p;
    p.name = "Hero";
    EXPECT_FALSE(s.SubmitCharacter(p));
    EXPECT_TRUE(s.is_failed());
    EXPECT_NE(s.failure_reason().find("not connected"),
              std::string::npos);
    s.Release();
}

TEST(CCharMake, InvalidAppearanceRejected) {
    CCharMake s;
    s.Init(nullptr);
    CharacterMakeParams p;
    p.name      = "Hero";
    p.sex_type  = 2;   // > 1 -> invalid
    p.hair_type = 0;
    p.face_type = 0;
    EXPECT_FALSE(s.SubmitCharacter(p));
    EXPECT_TRUE(s.is_failed());
    EXPECT_NE(s.failure_reason().find("invalid appearance"),
              std::string::npos);
    s.Release();
}

TEST(CCharMake, EmptyNameRejected) {
    CCharMake s;
    s.Init(nullptr);
    EXPECT_FALSE(s.SubmitCharacter(CharacterMakeParams{}));
    EXPECT_TRUE(s.is_failed());
    EXPECT_NE(s.failure_reason().find("empty name"),
              std::string::npos);
    s.Release();
}
