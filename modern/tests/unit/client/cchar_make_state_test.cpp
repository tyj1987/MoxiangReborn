// mxh/tests/unit/client/cchar_make_state_test.cpp
// Unit tests for mxh::client::CCharMake (Phase B.4).
//
// Coverage:
//   * legacy_character_make_syn_payload - 59B CHARACTERMAKEINFO layout,
//     1:1 with agent_handler.cpp::handle_legacy_character_make offsets
//     and the legacy CommonStruct.h CHARACTERMAKEINFO.
//   * CCharMake default state + lifecycle.

#include "CCharMake.hpp"
#include "mxh/proto/protocol.hpp"
#include "mxh/net/net.hpp"

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <thread>

namespace {
std::filesystem::path playdh_root() {
    std::error_code ec;
    auto current = std::filesystem::absolute(std::filesystem::current_path(ec), ec);
    for (int depth = 0; !ec && depth < 10 && !current.empty(); ++depth) {
        const auto candidate = current / "modern" / "data" / "PlayDH";
        std::error_code candidate_ec;
        if (std::filesystem::is_directory(candidate, candidate_ec)) return candidate;
        const auto parent = current.parent_path();
        if (parent == current) break;
        current = parent;
    }
    return {};
}
}

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
//   [30..50) WearedItemIdx[10] (u16 LE)
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
    p.weared_item_idx[1] = 11000;
    p.weared_item_idx[2] = 23000;
    p.weared_item_idx[3] = 27000;
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

    // WearedItemIdx uses the recovered legacy slot order:
    // hat=0, weapon=1, dress=2, shoes=3.
    EXPECT_EQ(pl[32], 0xF8u); EXPECT_EQ(pl[33], 0x2Au); // 11000
    EXPECT_EQ(pl[34], 0xD8u); EXPECT_EQ(pl[35], 0x59u); // 23000
    EXPECT_EQ(pl[36], 0x78u); EXPECT_EQ(pl[37], 0x69u); // 27000

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

TEST(CharMakeWire, NameCheckPayloadIsLegacyNameField) {
    const auto payload = mxh::client::legacy_character_name_check_payload(
        "1234567890ABCDEFGHIJKLMNOP");
    ASSERT_EQ(payload.size(), 17u);
    EXPECT_EQ(std::memcmp(payload.data(), "1234567890ABCDEF", 16), 0);
    EXPECT_EQ(payload[16], 0u);
}

TEST(CharacterMakeFormModel, UsesResourceDefaultsAndWrapsSelections) {
    std::string error;
    const auto catalog = mxh::client::CharMakeOptionCatalog::load(
        playdh_root(), &error);
    ASSERT_TRUE(catalog.has_value()) << error;
    mxh::client::CharacterMakeFormModel model;
    ASSERT_TRUE(model.initialize(*catalog));
    EXPECT_EQ(model.params().sex_type, 0u);
    EXPECT_EQ(model.params().hair_type, 0u);
    EXPECT_EQ(model.params().face_type, 0u);
    EXPECT_EQ(model.params().start_area, 17u);
    EXPECT_EQ(model.params().weared_item_idx[1], 11000u);
    EXPECT_EQ(model.params().weared_item_idx[2], 23000u);
    EXPECT_EQ(model.params().weared_item_idx[3], 27000u);

    ASSERT_TRUE(model.rotate(mxh::client::CharMakeOptionCategory::Weapon, 1));
    EXPECT_EQ(model.params().weared_item_idx[1], 13000u);
    ASSERT_TRUE(model.rotate(mxh::client::CharMakeOptionCategory::Weapon, -1));
    EXPECT_EQ(model.params().weared_item_idx[1], 11000u);
    ASSERT_TRUE(model.rotate(mxh::client::CharMakeOptionCategory::Sex, 1));
    EXPECT_EQ(model.params().sex_type, 1u);
    EXPECT_EQ(model.params().hair_type, 0u);
    EXPECT_EQ(model.params().face_type, 0u);
}

TEST(CharMakeUiCommand, ResolvesLegacyCreationControls) {
    using Kind = mxh::client::CharMakeUiCommandKind;
    using Category = mxh::client::CharMakeOptionCategory;
    mxh::client::ClientUiActivation activation;
    activation.legacy_id = "CMID_WeaponRight";
    auto command = mxh::client::resolve_char_make_ui_command(activation);
    EXPECT_EQ(command.kind, Kind::Rotate);
    EXPECT_EQ(command.category, Category::Weapon);
    EXPECT_EQ(command.direction, 1);

    activation = {};
    activation.legacy_func = "CM_OverlapCheckBtnFunc";
    EXPECT_EQ(mxh::client::resolve_char_make_ui_command(activation).kind,
              Kind::CheckName);
    activation.legacy_func = "CM_CharMakeBtnFunc";
    EXPECT_EQ(mxh::client::resolve_char_make_ui_command(activation).kind,
              Kind::Submit);
    activation.legacy_func = "CM_CharCancelBtnFunc";
    EXPECT_EQ(mxh::client::resolve_char_make_ui_command(activation).kind,
              Kind::Cancel);
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

// -------------------------------------------------------------------------
// Phase 1 §7.3 角色流程 — NameCheckAck/Nack test hook coverage.
//
// The full name check path requires a live AgentServer connection
// (CheckCurrentName() returns false when is_connected() is false and
// the m_nameCheckPending flag is only set after a successful send).
// The hook below lets us directly drive the dispatch path so we can
// lock the per-message behavior: the ack path stores
// m_nameAvailable = true, the nack path stores false, and a packet
// without a pending check is silently ignored.  This complements the
// legacy CharMakeWire payload tests, which only cover the encoder.
// -------------------------------------------------------------------------

TEST(CCharMakeNameCheck, HandleMessageForTestIsNoopWhenDispatchDisabled) {
    CCharMake s;
    s.Init(nullptr);
    s.SetDispatchForTest(false);
    mxh::net::Message m;
    m.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    m.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::CharacterNameCheckAck);
    m.payload = {};
    s.HandleMessageForTest(m);
    EXPECT_FALSE(s.name_available().has_value());
    s.Release();
}

TEST(CCharMakeNameCheck, HandleMessageForTestWithoutPendingIsIgnored) {
    // Drive a NameCheckAck through the dispatch path with the hook
    // enabled but no prior CheckCurrentName() call: the per-message
    // handler must guard on m_nameCheckPending and not stomp the
    // initial std::nullopt state.  This is the §7.3 silent-ingress
    // defense — a stray ack from a previous session must not flip the
    // availability indicator.
    CCharMake s;
    s.Init(nullptr);
    s.SetDispatchForTest(true);
    mxh::net::Message m;
    m.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    m.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::CharacterNameCheckAck);
    m.payload = {};
    s.HandleMessageForTest(m);
    EXPECT_FALSE(s.name_available().has_value());
    s.Release();
}

TEST(CCharMakeCreate, CharacterMakeNackTriggersFailWith) {
    // §7.3 名称合法性和重复检查: the agent rejects the CharacterMake
    // submission (duplicate name or invalid params) by sending a
    // CharacterMakeNack.  The state must flip to is_failed() and
    // surface a human-readable reason so the host can show a
    // recoverable error (vs. silently leaving the user on the create
    // form wondering why nothing happened).
    CCharMake s;
    s.Init(nullptr);
    s.SetDispatchForTest(true);
    mxh::net::Message m;
    m.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    m.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::CharacterMakeNack);
    m.payload = {};
    s.HandleMessageForTest(m);
    EXPECT_TRUE(s.is_failed());
    EXPECT_NE(s.failure_reason().find("CharacterMakeNack"), std::string::npos);
    s.Release();
}

// -------------------------------------------------------------------------
// Phase 1 §7.3 名称合法性和重复检查 — boundary coverage via SubmitCharacter.
//
// The valid_name_bytes() helper is in an anonymous namespace in
// CCharMake.cpp, so the test exercises it through the public
// SubmitCharacter() entry point.  The contract is that an invalid
// name (too short / too long / control byte) must not be sent to
// the agent and must flip is_failed() with the same reason the
// legacy UI surfaced, so a host swapping the live wire for the test
// hook sees identical error semantics.
// -------------------------------------------------------------------------

namespace {
mxh::client::CharacterMakeParams make_params_with_name(const std::string& name) {
    mxh::client::CharacterMakeParams p{};
    p.name = name;
    p.sex_type = 1;
    p.hair_type = 1;
    p.face_type = 1;
    return p;
}
}  // namespace

TEST(CCharMakeNameValidation, BoundaryLengthsViaSubmit) {
    // 3 bytes: too short
    {
        CCharMake s;
        s.Init(nullptr);
        CharacterMakeParams p = make_params_with_name("abc");
        EXPECT_FALSE(s.SubmitCharacter(p));
        EXPECT_TRUE(s.is_failed());
        s.Release();
    }
    // 17 bytes: one over kMaxNameLength
    {
        CCharMake s;
        s.Init(nullptr);
        CharacterMakeParams p = make_params_with_name("0123456789abcdefg");
        EXPECT_FALSE(s.SubmitCharacter(p));
        EXPECT_TRUE(s.is_failed());
        s.Release();
    }
}

TEST(CCharMakeNameValidation, RejectsControlCharactersViaSubmit) {
    // 0x00..0x1F and 0x7F are control bytes; the legacy validator
    // rejected them.  Tab is the most likely to slip past a "string
    // is non-empty" check, so test it explicitly.
    CCharMake s;
    s.Init(nullptr);
    CharacterMakeParams p = make_params_with_name("a\tbc");
    EXPECT_FALSE(s.SubmitCharacter(p));
    EXPECT_TRUE(s.is_failed());
    s.Release();
}

TEST(CCharMakeNameValidation, AcceptsUtf8MultiByteAtSubmitTime) {
    // Chinese 4-char name = 12 UTF-8 bytes.  This is within the
    // 4..16 byte window and must not fail with a "name" reason.
    // We can't observe the wire send (no connection), but the
    // failure_reason must be empty so the host can confirm the
    // name passed validation.
    CCharMake s;
    s.Init(nullptr);
    CharacterMakeParams p = make_params_with_name("\xE5\xA2\xA8\xE9\xA6\x99\xE7\x8E\xA9\xE5\xAE\xB6");
    (void)s.SubmitCharacter(p);  // may return false (no connection) but
                                 // must NOT fail because of the name
    EXPECT_EQ(s.failure_reason().find("name must contain"),
              std::string::npos);
    s.Release();
}

// Phase 1 §7.3: when the AgentServer stops responding to a
// CharacterMakeSyn (e.g. the DB write stalls), the application-level
// deadline must fire and surface a fail_with() so the user can
// retry the create instead of the state hanging forever.  Mirrors the
// CLoginState (fa74305e) and CCharSelectState (45009501) timeouts.
TEST(CCharMakeAckTimeout, MakeAckTimeoutFiresWhenNoResponse) {
    CCharMake s;
    s.Init(nullptr);
    s.SetMakeAckTimeoutForTest(std::chrono::milliseconds(50));
    s.ArmMakeAckDeadlineForTest();
    EXPECT_FALSE(s.is_failed());
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
    s.Process();
    EXPECT_TRUE(s.is_failed());
    EXPECT_NE(s.failure_reason().find("timeout"), std::string::npos)
        << "expected 'timeout' in: " << s.failure_reason();
    s.Release();
}
