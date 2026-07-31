//
// Unit tests for mxh::ui::cDebugDlg (Phase C dialog port).
//
// Locks down the 1:1 surface:
//   * DebugType enum: Attack=0, Item=1, Move=2, Mugong=3,
//                    Chat=4, UserConn=5
//   * Default construction: 6 flags false
//   * SetAttackBtnFlag / SetItemBtnFlag / SetMoveBtnFlag /
//     SetMugongBtnFlag / SetChatBtnFlag / SetUserConnBtnFlag
//     store the corresponding bool
//   * GetAttackBtnFalg (typo preserved) + GetAttackBtnFlag
//     (correct spelling) both return the Attack flag
//   * GetItemBtnFlag / GetMoveBtnFlag / GetMugongBtnFlag /
//     GetChatBtnFlag / GetUserConnBtnFlag
//   * DebugMsgParser(ATTACK) with flag off: no AddItem
//   * DebugMsgParser(ATTACK) with flag on: AddItem("ATTACK: ...")
//   * DebugMsgParser(ITEM) with flag on: AddItem("ITEM: ...")
//   * DebugMsgParser(MOVE) with flag on: AddItem("MOVE: ...")
//   * DebugMsgParser(MUGONG) with flag on: AddItem("MUGONG: ...")
//   * DebugMsgParser(CHAT) with flag on: AddItem("CHAT: ...")
//   * DebugMsgParser(USERCONN=5) with all flags off: falls
//     through to default branch (NORMAL: ...) (legacy
//     didn't include USERCONN case in switch)
//   * DebugMsgParser(unknown type): AddItem("NORMAL: ...")
//   * DebugMsgParser formats the va_list args
//   * Multiple flags independent
//   * NonCopyable
//

#include "mxh/ui/cdebugdlg.hpp"
#include "mxh/ui/cListDialog.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <type_traits>

using mxh::ui::cDebugDlg;
using mxh::ui::DebugType;
using mxh::ui::cListDialog;

TEST(CDebugDlg, ConstantsMatchLegacy) {
    EXPECT_EQ(static_cast<std::uint8_t>(DebugType::Attack),   0);
    EXPECT_EQ(static_cast<std::uint8_t>(DebugType::Item),     1);
    EXPECT_EQ(static_cast<std::uint8_t>(DebugType::Move),     2);
    EXPECT_EQ(static_cast<std::uint8_t>(DebugType::Mugong),   3);
    EXPECT_EQ(static_cast<std::uint8_t>(DebugType::Chat),     4);
    EXPECT_EQ(static_cast<std::uint8_t>(DebugType::UserConn), 5);
}

TEST(CDebugDlg, DefaultConstructionAllFlagsFalse) {
    cDebugDlg d;
    EXPECT_FALSE(d.GetAttackBtnFlag());
    EXPECT_FALSE(d.GetItemBtnFlag());
    EXPECT_FALSE(d.GetMoveBtnFlag());
    EXPECT_FALSE(d.GetMugongBtnFlag());
    EXPECT_FALSE(d.GetChatBtnFlag());
    EXPECT_FALSE(d.GetUserConnBtnFlag());
    EXPECT_EQ(d.AddItemCount(), 0u);
}

TEST(CDebugDlg, SetAttackBtnFlagStoresValue) {
    cDebugDlg d;
    d.SetAttackBtnFlag(true);
    EXPECT_TRUE(d.GetAttackBtnFlag());
    EXPECT_TRUE(d.GetAttackBtnFalg());
    d.SetAttackBtnFlag(false);
    EXPECT_FALSE(d.GetAttackBtnFlag());
}

TEST(CDebugDlg, SetItemBtnFlagStoresValue) {
    cDebugDlg d;
    d.SetItemBtnFlag(true);
    EXPECT_TRUE(d.GetItemBtnFlag());
    d.SetItemBtnFlag(false);
    EXPECT_FALSE(d.GetItemBtnFlag());
}

TEST(CDebugDlg, SetMoveBtnFlagStoresValue) {
    cDebugDlg d;
    d.SetMoveBtnFlag(true);
    EXPECT_TRUE(d.GetMoveBtnFlag());
    d.SetMoveBtnFlag(false);
    EXPECT_FALSE(d.GetMoveBtnFlag());
}

TEST(CDebugDlg, SetMugongBtnFlagStoresValue) {
    cDebugDlg d;
    d.SetMugongBtnFlag(true);
    EXPECT_TRUE(d.GetMugongBtnFlag());
    d.SetMugongBtnFlag(false);
    EXPECT_FALSE(d.GetMugongBtnFlag());
}

TEST(CDebugDlg, SetChatBtnFlagStoresValue) {
    cDebugDlg d;
    d.SetChatBtnFlag(true);
    EXPECT_TRUE(d.GetChatBtnFlag());
    d.SetChatBtnFlag(false);
    EXPECT_FALSE(d.GetChatBtnFlag());
}

TEST(CDebugDlg, SetUserConnBtnFlagStoresValue) {
    cDebugDlg d;
    d.SetUserConnBtnFlag(true);
    EXPECT_TRUE(d.GetUserConnBtnFlag());
    d.SetUserConnBtnFlag(false);
    EXPECT_FALSE(d.GetUserConnBtnFlag());
}

TEST(CDebugDlg, MultipleFlagsAreIndependent) {
    cDebugDlg d;
    d.SetAttackBtnFlag(true);
    EXPECT_FALSE(d.GetItemBtnFlag());
    EXPECT_FALSE(d.GetMoveBtnFlag());
    EXPECT_FALSE(d.GetChatBtnFlag());
}

TEST(CDebugDlg, AttackParserPrefixWhenFlagOn) {
    cDebugDlg d;
    d.SetAttackBtnFlag(true);
    d.DebugMsgParser(static_cast<std::uint8_t>(DebugType::Attack),
                     "hello %d", 42);
    EXPECT_EQ(d.AddItemCount(), 1u);
    EXPECT_EQ(d.LastAddedText(), "ATTACK: hello 42");
}

TEST(CDebugDlg, AttackParserNoAddItemWhenFlagOff) {
    cDebugDlg d;
    d.DebugMsgParser(static_cast<std::uint8_t>(DebugType::Attack), "x");
    EXPECT_EQ(d.AddItemCount(), 0u);
}

TEST(CDebugDlg, ItemParserPrefixWhenFlagOn) {
    cDebugDlg d;
    d.SetItemBtnFlag(true);
    d.DebugMsgParser(static_cast<std::uint8_t>(DebugType::Item),
                     "item %s", "name");
    EXPECT_EQ(d.LastAddedText(), "ITEM: item name");
}

TEST(CDebugDlg, MoveParserPrefixWhenFlagOn) {
    cDebugDlg d;
    d.SetMoveBtnFlag(true);
    d.DebugMsgParser(static_cast<std::uint8_t>(DebugType::Move), "M");
    EXPECT_EQ(d.LastAddedText(), "MOVE: M");
}

TEST(CDebugDlg, MugongParserPrefixWhenFlagOn) {
    cDebugDlg d;
    d.SetMugongBtnFlag(true);
    d.DebugMsgParser(static_cast<std::uint8_t>(DebugType::Mugong), "m");
    EXPECT_EQ(d.LastAddedText(), "MUGONG: m");
}

TEST(CDebugDlg, ChatParserPrefixWhenFlagOn) {
    cDebugDlg d;
    d.SetChatBtnFlag(true);
    d.DebugMsgParser(static_cast<std::uint8_t>(DebugType::Chat), "hello");
    EXPECT_EQ(d.LastAddedText(), "CHAT: hello");
}

TEST(CDebugDlg, UserConnParserRoutesToDefaultBranch) {
    // 1:1 quirk: legacy code's DBG_USERCONN value
    // (= 5) is not in the switch's case list, so
    // it falls through to the default branch (always
    // fires regardless of any flag).
    cDebugDlg d;
    d.DebugMsgParser(static_cast<std::uint8_t>(DebugType::UserConn), "uc");
    EXPECT_EQ(d.AddItemCount(), 1u);
    EXPECT_EQ(d.LastAddedText(), "NORMAL: uc");
}

TEST(CDebugDlg, UnknownTypeFallsToDefaultBranch) {
    cDebugDlg d;
    d.DebugMsgParser(/*type=*/255, "weird");
    EXPECT_EQ(d.AddItemCount(), 1u);
    EXPECT_EQ(d.LastAddedText(), "NORMAL: weird");
}

TEST(CDebugDlg, DefaultBranchIgnoresAllFlags) {
    cDebugDlg d;
    // No flags set, but the default branch always fires.
    d.DebugMsgParser(255, "default");
    EXPECT_EQ(d.AddItemCount(), 1u);
    EXPECT_EQ(d.LastAddedText(), "NORMAL: default");
}

TEST(CDebugDlg, ParserRespectsLongStrings) {
    cDebugDlg d;
    d.SetAttackBtnFlag(true);
    char big[400];
    std::memset(big, 'x', sizeof(big) - 1);
    big[sizeof(big) - 1] = '\0';
    d.DebugMsgParser(static_cast<std::uint8_t>(DebugType::Attack),
                     "%s", big);
    EXPECT_EQ(d.AddItemCount(), 1u);
    EXPECT_FALSE(d.LastAddedText().empty());
    EXPECT_NE(d.LastAddedText().find("ATTACK:"), std::string::npos);
}

TEST(CDebugDlg, ParserMultipleBranchesAccumulate) {
    cDebugDlg d;
    d.SetAttackBtnFlag(true);
    d.SetChatBtnFlag(true);
    d.DebugMsgParser(static_cast<std::uint8_t>(DebugType::Attack), "a");
    d.DebugMsgParser(static_cast<std::uint8_t>(DebugType::Chat),   "c");
    d.DebugMsgParser(static_cast<std::uint8_t>(DebugType::Move),   "m");
    EXPECT_EQ(d.AddItemCount(), 2u);
    EXPECT_EQ(d.LastAddedText(), "CHAT: c");
}

TEST(CDebugDlg, ParserWithNullMsgDoesNotCrash) {
    cDebugDlg d;
    d.SetAttackBtnFlag(true);
    d.DebugMsgParser(static_cast<std::uint8_t>(DebugType::Attack),
                     nullptr);
    EXPECT_EQ(d.AddItemCount(), 0u);
}


TEST(CDebugDlg, NonCopyable) {
    static_assert(!std::is_copy_constructible<cDebugDlg>::value,
                  "cDebugDlg must not be copyable");
    static_assert(!std::is_copy_assignable<cDebugDlg>::value,
                  "cDebugDlg must not be copy-assignable");
    SUCCEED();
}

TEST(CDebugDlg, IscListDialog) {
    static_assert(std::is_base_of<cListDialog, cDebugDlg>::value,
                  "cDebugDlg must inherit from cListDialog");
    SUCCEED();
}
