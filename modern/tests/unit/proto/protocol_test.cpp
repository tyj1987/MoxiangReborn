// protocol_test.cpp - Phase 10.12 protocol layer wire-format tests
//
// Covers the typed-enum replacement of [CC]Header/Protocol.h in
// modern/include/mxh/proto/protocol.hpp:
//   - 12 enum classes (Category, UserConnProtocol, ServerProtocol,
//     MoveProtocol, ChatProtocol, ItemProtocol, MonsterProtocol,
//     BossMonsterProtocol, NpcProtocol, SkillProtocol, BattleProtocol,
//     VersionRejectReason)
//   - 2 version constants (kProtocolVersion, kMinProtocolVersion)
//   - 3 modern version-negotiation constants (kModernCheckVersion,
//     kModernNotifyVersionAck, kModernNotifyVersionNack)
//
// Why these tests matter:
//   The modern enum classes are 1:1 wire-compatible with the legacy
//   [CC]Header/Protocol.h #defines. If a future change accidentally
//   re-orders Category or shifts a UserConnProtocol value by one,
//   every wire packet using that category/protocol stops talking to
//   legacy clients/servers — silent breakage with no compiler
//   warning because the enum class is uint8_t underneath.
//
//   These tests pin the byte values for the categories we actually
//   send packets on (UserConn, Move, Chat, Item, etc.) and for the
//   modern-only constants that the version-negotiation layer
//   (Phase 3.4 / Phase 7.5) depends on. The full enum (60+ entries)
//   is not exhaustively pinned because that would make the test
//   noisy on every additive change; instead, the layout integrity
//   tests (sequential, non-overlapping, monotonic) catch real
//   regressions.

#include "mxh/proto/protocol.hpp"

#include <gtest/gtest.h>

#include <type_traits>

namespace mxh::proto::test {

// ===========================================================================
// Underlying type + size invariants
// ===========================================================================

TEST(ProtocolTypeTest, EnumsAreUint8) {
    // All 12 enums use std::uint8_t as the underlying type. Pinning
    // this here catches a future change to a wider type (which would
    // change the wire byte size and break 1:1 interop with the
    // legacy 2003-era C enums).
    static_assert(std::is_same_v<std::underlying_type_t<Category>,
                                 std::uint8_t>);
    static_assert(std::is_same_v<std::underlying_type_t<UserConnProtocol>,
                                 std::uint8_t>);
    static_assert(std::is_same_v<std::underlying_type_t<ServerProtocol>,
                                 std::uint8_t>);
    static_assert(std::is_same_v<std::underlying_type_t<MoveProtocol>,
                                 std::uint8_t>);
    static_assert(std::is_same_v<std::underlying_type_t<ChatProtocol>,
                                 std::uint8_t>);
    static_assert(std::is_same_v<std::underlying_type_t<ItemProtocol>,
                                 std::uint8_t>);
    static_assert(std::is_same_v<std::underlying_type_t<MonsterProtocol>,
                                 std::uint8_t>);
    static_assert(std::is_same_v<std::underlying_type_t<BossMonsterProtocol>,
                                 std::uint8_t>);
    static_assert(std::is_same_v<std::underlying_type_t<NpcProtocol>,
                                 std::uint8_t>);
    static_assert(std::is_same_v<std::underlying_type_t<SkillProtocol>,
                                 std::uint8_t>);
    static_assert(std::is_same_v<std::underlying_type_t<BattleProtocol>,
                                 std::uint8_t>);
    static_assert(std::is_same_v<std::underlying_type_t<VersionRejectReason>,
                                 std::uint8_t>);
    SUCCEED();
}

// ===========================================================================
// Version negotiation constants
// ===========================================================================

TEST(ProtocolVersionTest, CurrentVersionIsOne) {
    // Current server protocol version. Any client whose kProtocolVersion
    // differs from this should be rejected with VersionRejectReason.
    EXPECT_EQ(kProtocolVersion, 1);
}

TEST(ProtocolVersionTest, MinVersionIsZeroForLegacyCompat) {
    // Minimum accepted version. 0 = legacy clients (pre-versioning)
    // are accepted. If you bump this to 1, every legacy client gets
    // bumped with VersionRejectReason::TooOld.
    EXPECT_EQ(kMinProtocolVersion, 0);
}

TEST(ProtocolVersionTest, CurrentIsAtLeastMin) {
    // The current version must always be >= the minimum, otherwise
    // we'd reject ourselves. (Trivially true; pins the invariant.)
    EXPECT_GE(kProtocolVersion, kMinProtocolVersion);
}

TEST(VersionRejectReasonTest, ValuesAreDistinct) {
    // The 3 reject reasons must be distinct so dispatch on the wire
    // is unambiguous.
    EXPECT_NE(static_cast<std::uint8_t>(VersionRejectReason::TooOld),
              static_cast<std::uint8_t>(VersionRejectReason::TooNew));
    EXPECT_NE(static_cast<std::uint8_t>(VersionRejectReason::TooOld),
              static_cast<std::uint8_t>(VersionRejectReason::ServerFull));
    EXPECT_NE(static_cast<std::uint8_t>(VersionRejectReason::TooNew),
              static_cast<std::uint8_t>(VersionRejectReason::ServerFull));
}

TEST(VersionRejectReasonTest, TooOldIsZero) {
    // TooOld=0 matches the original [CC]Header/Protocol.h convention
    // and is what the legacy client-side state machine expects to
    // see first when a version check fails for the "you're too old"
    // case. Reordering the enum would break clients that special-
    // case the 0 value.
    EXPECT_EQ(static_cast<std::uint8_t>(VersionRejectReason::TooOld), 0u);
}

TEST(VersionRejectReasonTest, TooNewIsOne) {
    EXPECT_EQ(static_cast<std::uint8_t>(VersionRejectReason::TooNew), 1u);
}

TEST(VersionRejectReasonTest, ServerFullIsTwo) {
    EXPECT_EQ(static_cast<std::uint8_t>(VersionRejectReason::ServerFull), 2u);
}

// ===========================================================================
// Category enum — critical wire-format values
// ===========================================================================

TEST(CategoryTest, ServerIsOne) {
    // Category::Server = 1 is the first enum value and the base of
    // every protocol category. The legacy [CC]Header/Protocol.h
    // starts with MP_CATEGORY_SERVER = 1; 1:1 mapping.
    EXPECT_EQ(static_cast<std::uint8_t>(Category::Server), 1u);
}

TEST(CategoryTest, UserConnIsSeven) {
    // Category::UserConn = 7. This is the category every login /
    // game-in / character-select / connection-check packet uses.
    // Moving it by even one byte would break every legacy login.
    EXPECT_EQ(static_cast<std::uint8_t>(Category::UserConn), 7u);
}

TEST(CategoryTest, MoveIsEight) {
    // Category::Move = 8. Walk/run/stop/target packets.
    EXPECT_EQ(static_cast<std::uint8_t>(Category::Move), 8u);
}

TEST(CategoryTest, ChatIsSix) {
    // Category::Chat = 6. All chat packets use this category.
    EXPECT_EQ(static_cast<std::uint8_t>(Category::Chat), 6u);
}

TEST(CategoryTest, ItemIsFive) {
    // Category::Item = 5. Inventory / equipment / shop packets.
    EXPECT_EQ(static_cast<std::uint8_t>(Category::Item), 5u);
}

TEST(CategoryTest, CharacterIsThree) {
    // Category::Character = 3. Level-up / stat-point / etc.
    EXPECT_EQ(static_cast<std::uint8_t>(Category::Character), 3u);
}

TEST(CategoryTest, BattleIsThirtyOne) {
    // Category::Battle = 31. PvP / damage / kill packets.
    EXPECT_EQ(static_cast<std::uint8_t>(Category::Battle), 31u);
}

TEST(CategoryTest, MonsterIsThirtyFive) {
    // Category::Monster = 35. Monster AI / damage / state packets.
    EXPECT_EQ(static_cast<std::uint8_t>(Category::Monster), 35u);
}

TEST(CategoryTest, NpcIsThirtySeven) {
    // Category::Npc = 37. NPC dialogue / quest packets.
    EXPECT_EQ(static_cast<std::uint8_t>(Category::Npc), 37u);
}

TEST(CategoryTest, CategoriesAreSequential) {
    // The first 9 categories (Server, PowerUp, Character, Map, Item,
    // Chat, UserConn, Move, Mugong) must be sequential 1..9 with no
    // gaps. A future change that inserts or reorders a category
    // would shift every subsequent value and break interop.
    EXPECT_EQ(static_cast<std::uint8_t>(Category::Server),     1u);
    EXPECT_EQ(static_cast<std::uint8_t>(Category::PowerUp),    2u);
    EXPECT_EQ(static_cast<std::uint8_t>(Category::Character),  3u);
    EXPECT_EQ(static_cast<std::uint8_t>(Category::Map),        4u);
    EXPECT_EQ(static_cast<std::uint8_t>(Category::Item),       5u);
    EXPECT_EQ(static_cast<std::uint8_t>(Category::Chat),       6u);
    EXPECT_EQ(static_cast<std::uint8_t>(Category::UserConn),   7u);
    EXPECT_EQ(static_cast<std::uint8_t>(Category::Move),       8u);
    EXPECT_EQ(static_cast<std::uint8_t>(Category::Mugong),     9u);
}

// ===========================================================================
// UserConnProtocol — first 8 protocol values pinned (login flow)
// ===========================================================================

TEST(UserConnProtocolTest, DistConnectSuccessIsZero) {
    // First UserConn protocol: Distribute->Client: auth-key-in-dwObjectID.
    // Value 0 matches the original.
    EXPECT_EQ(static_cast<std::uint8_t>(UserConnProtocol::DistConnectSuccess), 0u);
}

TEST(UserConnProtocolTest, RequestLoginIsOne) {
    // Client -> Distribute: id + password.
    EXPECT_EQ(static_cast<std::uint8_t>(UserConnProtocol::RequestLogin), 1u);
}

TEST(UserConnProtocolTest, LoginMessagesUnchanged) {
    // Quick pin of the 9 UserConn messages used in the login flow.
    // If any of these shift, the legacy client / server stops
    // understanding the other side.
    EXPECT_EQ(static_cast<std::uint8_t>(UserConnProtocol::DistConnectSuccess),     0u);
    EXPECT_EQ(static_cast<std::uint8_t>(UserConnProtocol::RequestLogin),           1u);
    EXPECT_EQ(static_cast<std::uint8_t>(UserConnProtocol::NotifyUserLoginAck),      2u);
    EXPECT_EQ(static_cast<std::uint8_t>(UserConnProtocol::NotifyUserLoginNack),     3u);
    EXPECT_EQ(static_cast<std::uint8_t>(UserConnProtocol::NotifyUserLoginSyn),      4u);
    EXPECT_EQ(static_cast<std::uint8_t>(UserConnProtocol::NotifyUserLoginAck2),     5u);
    EXPECT_EQ(static_cast<std::uint8_t>(UserConnProtocol::NotifyUserLoginNack2),    6u);
    EXPECT_EQ(static_cast<std::uint8_t>(UserConnProtocol::NotifyOverlappedLogin),   7u);
    EXPECT_EQ(static_cast<std::uint8_t>(UserConnProtocol::AgentConnectSuccess),     8u);
}

// ===========================================================================
// Modern version-negotiation constants (kModern*)
// ===========================================================================

TEST(ModernVersionTest, CheckVersionIsTwoHundred) {
    // kModernCheckVersion = 200. This is the protocol ID the modern
    // server uses for the version-check Syn message. P10.4.9
    // collateral fix: version_test.cpp used to reference
    // UserConnProtocol::CheckVersion (which does not exist in the
    // legacy enum) and failed to compile. The correct reference is
    // kModernCheckVersion. Pinning 200 here documents the value and
    // catches a future change.
    EXPECT_EQ(kModernCheckVersion, 200);
}

TEST(ModernVersionTest, NotifyVersionAckIsTwoHundredOne) {
    EXPECT_EQ(kModernNotifyVersionAck, 201);
}

TEST(ModernVersionTest, NotifyVersionNackIsTwoHundredTwo) {
    EXPECT_EQ(kModernNotifyVersionNack, 202);
}

TEST(ModernVersionTest, ModernConstantsAreAboveUserConnRange) {
    // The kModern* protocol IDs (200..202) are deliberately placed
    // outside the legacy UserConnProtocol range (0..114) so they
    // never collide with a real UserConn message. If you ever see
    // one of these < 200 or one of the UserConn messages >= 200,
    // something has been re-ordered and the version-negotiation
    // layer is at risk of being misinterpreted as a real login
    // packet.
    EXPECT_GT(kModernCheckVersion, 114u);
    EXPECT_GT(kModernNotifyVersionAck, 114u);
    EXPECT_GT(kModernNotifyVersionNack, 114u);
}

TEST(ModernVersionTest, ModernConstantsAreDistinct) {
    EXPECT_NE(kModernCheckVersion, kModernNotifyVersionAck);
    EXPECT_NE(kModernCheckVersion, kModernNotifyVersionNack);
    EXPECT_NE(kModernNotifyVersionAck, kModernNotifyVersionNack);
}

}  // namespace mxh::proto::test


namespace mxh::proto::test {
TEST(CategoryTest, LegacyTailCategoriesAreByteCompatible) {
    EXPECT_EQ(static_cast<std::uint8_t>(Category::HackCheck), 42u);
    EXPECT_EQ(static_cast<std::uint8_t>(Category::RMTool_Connect), 43u);
    EXPECT_EQ(static_cast<std::uint8_t>(Category::RMTool_Item), 51u);
    EXPECT_EQ(static_cast<std::uint8_t>(Category::Wanted), 52u);
    EXPECT_EQ(static_cast<std::uint8_t>(Category::GTournament), 60u);
    EXPECT_EQ(static_cast<std::uint8_t>(Category::HackShield), 67u);
    EXPECT_EQ(static_cast<std::uint8_t>(Category::NProtect), 69u);
    EXPECT_EQ(static_cast<std::uint8_t>(Category::FortWar), 77u);
    EXPECT_EQ(static_cast<std::uint8_t>(Category::Max), 78u);
}

TEST(CategoryTest, CategoryNamesIdentifySecurityCategories) {
    EXPECT_STREQ(category_name(Category::HackShield), "HackShield");
    EXPECT_STREQ(category_name(Category::NProtect), "NProtect");
    EXPECT_STREQ(category_name(Category::HackCheck), "HackCheck");
}

TEST(CategoryTest, CompleteLegacyCategoryTableIsSequential) {
    constexpr Category categories[] = {
        Category::Server, Category::PowerUp, Category::Character,
        Category::Map, Category::Item, Category::Chat, Category::UserConn,
        Category::Move, Category::Mugong, Category::AuctionBoard, Category::Cheat,
        Category::Quick, Category::PackedData, Category::Party,
        Category::PeaceWarMode, Category::UngiJosik, Category::Auction,
        Category::AutoPatch, Category::Signal, Category::Tactic, Category::Munpa,
        Category::Skill, Category::KyungGong, Category::SimBub,
        Category::MornitorTool, Category::MornitorServer,
        Category::MornitorMapServer, Category::Exchange, Category::StreetStall,
        Category::Pyoguk, Category::Battle, Category::CharRevive,
        Category::Friend, Category::BossMonster, Category::Monster,
        Category::Option, Category::Npc, Category::MurimNet, Category::Quest,
        Category::Debug, Category::Pk, Category::HackCheck,
        Category::RMTool_Connect, Category::RMTool_User, Category::RMTool_Munpa,
        Category::RMTool_GameLog, Category::RMTool_OperLog,
        Category::RMTool_Statistics, Category::RMTool_Admin,
        Category::RMTool_Character, Category::RMTool_Item, Category::Wanted,
        Category::Journal, Category::Suryun, Category::SocietyAct, Category::Guild,
        Category::GuildFieldWar, Category::Note, Category::PartyWar,
        Category::GTournament, Category::Jackpot, Category::GuildUnion,
        Category::SiegeWar, Category::SiegeWar_Profit, Category::Weather,
        Category::Pet, Category::HackShield, Category::RMTool_Pet,
        Category::NProtect, Category::RMTool_DelChar, Category::Survival,
        Category::Titan, Category::ItemExt, Category::Bobusang, Category::ItemLimit,
        Category::AutoNote, Category::FortWar,
    };
    ASSERT_EQ((sizeof(categories) / sizeof(categories[0])), 77u);
    for (std::size_t i = 0; i < (sizeof(categories) / sizeof(categories[0])); ++i) {
        EXPECT_EQ(static_cast<std::uint8_t>(categories[i]),
                  static_cast<std::uint8_t>(i + 1));
    }
}

}  // namespace mxh::proto::test
