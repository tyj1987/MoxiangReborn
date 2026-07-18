// debugdlg_test.cpp — 1:1 port tests for 墨香
// CDebugDlg (debug flag display dialog).

#include "debugdlg.hpp"
#include "clistdialog.hpp"
#include "cdialog.hpp"

#include <gtest/gtest.h>

#include <type_traits>

using mxh::ui::cDebugDlg;
using mxh::ui::cDialog;
using mxh::ui::cListDialog;

TEST(CDebugDlgTest, CtorDoesNotCrash) {
    cDebugDlg dlg;
    SUCCEED();
}

TEST(CDebugDlgTest, DtorDoesNotCrash) {
    cDebugDlg dlg;
    SUCCEED();
}

TEST(CDebugDlgTest, InheritsFromCListDialog) {
    static_assert(std::is_base_of_v<cListDialog, cDebugDlg>,
                  "cDebugDlg must inherit from cListDialog");
    SUCCEED();
}

TEST(CDebugDlgTest, InheritsFromCDialog) {
    static_assert(std::is_base_of_v<cDialog, cDebugDlg>,
                  "cDebugDlg must inherit from cDialog");
    SUCCEED();
}

// ---------- enum constants ----------

TEST(CDebugDlgTest, DbgEnumConstantsMatchLegacyValues) {
    EXPECT_EQ(cDebugDlg::kDbgAttack, 0);
    EXPECT_EQ(cDebugDlg::kDbgItem, 1);
    EXPECT_EQ(cDebugDlg::kDbgMove, 2);
    EXPECT_EQ(cDebugDlg::kDbgMugong, 3);
    EXPECT_EQ(cDebugDlg::kDbgChat, 4);
    EXPECT_EQ(cDebugDlg::kDbgUserConn, 5);
}

// ---------- default flag state ----------

TEST(CDebugDlgTest, AllFlagsDefaultFalse) {
    cDebugDlg dlg;
    EXPECT_FALSE(dlg.GetAttackBtnFlag());
    EXPECT_FALSE(dlg.GetItemBtnFlag());
    EXPECT_FALSE(dlg.GetMoveBtnFlag());
    EXPECT_FALSE(dlg.GetMugongBtnFlag());
    EXPECT_FALSE(dlg.GetChatBtnFlag());
    EXPECT_FALSE(dlg.GetUserConnBtnFlag());
}

// ---------- Attack flag ----------

TEST(CDebugDlgTest, AttackFlagSetGet) {
    cDebugDlg dlg;
    dlg.SetAttackBtnFlag(true);
    EXPECT_TRUE(dlg.GetAttackBtnFlag());
    dlg.SetAttackBtnFlag(false);
    EXPECT_FALSE(dlg.GetAttackBtnFlag());
}

// ---------- Item flag ----------

TEST(CDebugDlgTest, ItemFlagSetGet) {
    cDebugDlg dlg;
    dlg.SetItemBtnFlag(true);
    EXPECT_TRUE(dlg.GetItemBtnFlag());
    dlg.SetItemBtnFlag(false);
    EXPECT_FALSE(dlg.GetItemBtnFlag());
}

// ---------- Move flag ----------

TEST(CDebugDlgTest, MoveFlagSetGet) {
    cDebugDlg dlg;
    dlg.SetMoveBtnFlag(true);
    EXPECT_TRUE(dlg.GetMoveBtnFlag());
    dlg.SetMoveBtnFlag(false);
    EXPECT_FALSE(dlg.GetMoveBtnFlag());
}

// ---------- Mugong flag ----------

TEST(CDebugDlgTest, MugongFlagSetGet) {
    cDebugDlg dlg;
    dlg.SetMugongBtnFlag(true);
    EXPECT_TRUE(dlg.GetMugongBtnFlag());
    dlg.SetMugongBtnFlag(false);
    EXPECT_FALSE(dlg.GetMugongBtnFlag());
}

// ---------- Chat flag ----------

TEST(CDebugDlgTest, ChatFlagSetGet) {
    cDebugDlg dlg;
    dlg.SetChatBtnFlag(true);
    EXPECT_TRUE(dlg.GetChatBtnFlag());
    dlg.SetChatBtnFlag(false);
    EXPECT_FALSE(dlg.GetChatBtnFlag());
}

// ---------- UserConn flag ----------

TEST(CDebugDlgTest, UserConnFlagSetGet) {
    cDebugDlg dlg;
    dlg.SetUserConnBtnFlag(true);
    EXPECT_TRUE(dlg.GetUserConnBtnFlag());
    dlg.SetUserConnBtnFlag(false);
    EXPECT_FALSE(dlg.GetUserConnBtnFlag());
}

// ---------- multi-flag ----------

TEST(CDebugDlgTest, MultipleFlagsIndependent) {
    cDebugDlg dlg;
    dlg.SetAttackBtnFlag(true);
    dlg.SetItemBtnFlag(true);
    EXPECT_TRUE(dlg.GetAttackBtnFlag());
    EXPECT_TRUE(dlg.GetItemBtnFlag());
    EXPECT_FALSE(dlg.GetMoveBtnFlag());
    EXPECT_FALSE(dlg.GetMugongBtnFlag());
    EXPECT_FALSE(dlg.GetChatBtnFlag());
    EXPECT_FALSE(dlg.GetUserConnBtnFlag());
}

TEST(CDebugDlgTest, AllFlagsCanBeSet) {
    cDebugDlg dlg;
    dlg.SetAttackBtnFlag(true);
    dlg.SetItemBtnFlag(true);
    dlg.SetMoveBtnFlag(true);
    dlg.SetMugongBtnFlag(true);
    dlg.SetChatBtnFlag(true);
    dlg.SetUserConnBtnFlag(true);
    EXPECT_TRUE(dlg.GetAttackBtnFlag());
    EXPECT_TRUE(dlg.GetItemBtnFlag());
    EXPECT_TRUE(dlg.GetMoveBtnFlag());
    EXPECT_TRUE(dlg.GetMugongBtnFlag());
    EXPECT_TRUE(dlg.GetChatBtnFlag());
    EXPECT_TRUE(dlg.GetUserConnBtnFlag());
}

// ---------- DebugMsgParser ----------

TEST(CDebugDlgTest, DebugMsgParserIsNoOp) {
    cDebugDlg dlg;
    // TODO: 1:1 with legacy variadic + 6-branch dispatch
    //       (R-12.x deferred). Modern port is no-op.
    dlg.DebugMsgParser(cDebugDlg::kDbgAttack, "Test %d", 42);
    SUCCEED();
}

TEST(CDebugDlgTest, DebugMsgParserBeforeInitIsSafe) {
    cDebugDlg dlg;
    dlg.DebugMsgParser(cDebugDlg::kDbgItem, "Test");
    SUCCEED();
}
