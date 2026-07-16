// charstatedialog_test.cpp - Phase 12.x P2-12 Tier 2 dialog 1:1 port
// contract test for modern cCharStateDialog (character state bar:
// PK / Move / KyungGong / PeaceWar / Ungi mode buttons).
//
// Covers modern/src/ui/charstatedialog.{hpp,cpp}, a 1:1 port of
//   墨香【源码】\[Client]MH\CharStateDialog.h (701 B) and
//   墨香【源码】\[Client]MH\CharStateDialog.cpp.
//
// What's tested:
//   - Default construction: all 5 cPushupButton pointers are null.
//   - Linking resolves the 5 cPushupButton children (PK / Move /
//     KyungGong / PeaceWar / Ungi) by id range 220-224.
//   - Linking sets each resolved button to passive (1:1 with
//     legacy SetPassive(TRUE) for all 5).
//   - Linking without children leaves pointers null and is safe.
//   - 5 SetXxxMode methods push / unpush the linked buttons
//     (REAL, no singleton — these are testable end-to-end).
//   - SetPeaceWarMode inverts its argument (1:1 quirk: legacy
//     stores PeaceWar as the OPPOSITE of the underlying "peace"
//     flag, so SetPush(!bPeace)).
//   - SetXxxMode with unlinked children is safe (no crash, no
//     state change).
//   - OnActionEvent is a no-op until MACROMGR + PKMGR are ported
//     (4 button ids + WE_PUSHUP/ WE_PUSHDOWN state-machine shape
//     preserved).
//   - Refresh is a no-op until SCRIPTMGR + RESRCMGR + MACROMGR +
//     GAMEIN are ported.
//   - Accessors return the linked cPushupButton pointers.
//
// 1:1 quirks preserved:
//   - Linking SetPassive(TRUE) on all 5 buttons (user can't
//     toggle, code alone flips state).
//   - SetPeaceWarMode inverts the argument (button shows war,
//     API is peace).
//   - KyungGong + Ungi OnActionEvent branches are commented out
//     in legacy (macros not implemented).
//   - Refresh's KyungGong + Ungi tooltip rebuilds are commented
//     out in legacy (same reason).

#include "charstatedialog.hpp"
#include "cpushupbutton.hpp"
#include "cdialog.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace mxh::ui::test {

// ===========================================================================
// Construction
// ===========================================================================

TEST(CCharStateDialogTest, DefaultConstructionHasNullPointers) {
    cCharStateDialog dlg;
    EXPECT_EQ(dlg.GetPKBtn(),         nullptr);
    EXPECT_EQ(dlg.GetMoveBtn(),       nullptr);
    EXPECT_EQ(dlg.GetKyungGongBtn(),  nullptr);
    EXPECT_EQ(dlg.GetPeaceWarBtn(),   nullptr);
    EXPECT_EQ(dlg.GetUngiBtn(),       nullptr);
}

// ===========================================================================
// Id constants
// ===========================================================================

TEST(CCharStateDialogTest, IdConstantsAreDistinct) {
    EXPECT_NE(cCharStateDialog::kBtnPKId,        cCharStateDialog::kBtnMoveId);
    EXPECT_NE(cCharStateDialog::kBtnPKId,        cCharStateDialog::kBtnKyungGongId);
    EXPECT_NE(cCharStateDialog::kBtnPKId,        cCharStateDialog::kBtnPeaceWarId);
    EXPECT_NE(cCharStateDialog::kBtnPKId,        cCharStateDialog::kBtnUngiId);
    EXPECT_NE(cCharStateDialog::kBtnMoveId,      cCharStateDialog::kBtnKyungGongId);
    EXPECT_NE(cCharStateDialog::kBtnMoveId,      cCharStateDialog::kBtnPeaceWarId);
    EXPECT_NE(cCharStateDialog::kBtnMoveId,      cCharStateDialog::kBtnUngiId);
    EXPECT_NE(cCharStateDialog::kBtnKyungGongId, cCharStateDialog::kBtnPeaceWarId);
    EXPECT_NE(cCharStateDialog::kBtnKyungGongId, cCharStateDialog::kBtnUngiId);
    EXPECT_NE(cCharStateDialog::kBtnPeaceWarId,  cCharStateDialog::kBtnUngiId);
}

TEST(CCharStateDialogTest, IdConstantsMatchExpectedLocalRange) {
    // 1:1 quirk: pick 220-224 to avoid collisions with
    // other Tier 2 dialog id ranges (cCharMakeDlg uses
    // 200-203, cGuildJoinDialog uses 210-212).
    EXPECT_EQ(cCharStateDialog::kBtnPKId,        220);
    EXPECT_EQ(cCharStateDialog::kBtnMoveId,      221);
    EXPECT_EQ(cCharStateDialog::kBtnKyungGongId, 222);
    EXPECT_EQ(cCharStateDialog::kBtnPeaceWarId,  223);
    EXPECT_EQ(cCharStateDialog::kBtnUngiId,      224);
}

// ===========================================================================
// Linking
// ===========================================================================

namespace {

// Build a cCharStateDialog with 5 cPushupButton children wired
// in the modern id range (220-224). Returns raw pointers to
// the 5 buttons via the out vector; ownership lives in the
// dlg (the buttons are children, not separately managed).
void BuildDlgWithPushups(cCharStateDialog& dlg, std::vector<cPushupButton*>& raws) {
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    raws.assign(5, nullptr);
    for (int i = 0; i < 5; ++i) {
        auto s = std::make_unique<cPushupButton>();
        s->Init(0, 0, 50, 14, nullptr, nullptr, nullptr, nullptr, nullptr, 220 + i);
        raws[i] = s.get();
        dlg.Add(std::unique_ptr<cWindow>(s.release()));
    }
    dlg.Linking();
}

}  // namespace

TEST(CCharStateDialogTest, LinkingResolvesAllFivePushups) {
    cCharStateDialog dlg;
    std::vector<cPushupButton*> raws;
    BuildDlgWithPushups(dlg, raws);

    EXPECT_EQ(dlg.GetPKBtn(),         raws[0]);
    EXPECT_EQ(dlg.GetMoveBtn(),       raws[1]);
    EXPECT_EQ(dlg.GetKyungGongBtn(),  raws[2]);
    EXPECT_EQ(dlg.GetPeaceWarBtn(),   raws[3]);
    EXPECT_EQ(dlg.GetUngiBtn(),       raws[4]);
}

TEST(CCharStateDialogTest, LinkingSetsAllButtonsPassive) {
    // 1:1 with legacy SetPassive(TRUE) on all 5 buttons.
    cCharStateDialog dlg;
    std::vector<cPushupButton*> raws;
    BuildDlgWithPushups(dlg, raws);
    for (auto* btn : raws) {
        ASSERT_NE(btn, nullptr);
        EXPECT_TRUE(btn->IsPassive());
    }
}

TEST(CCharStateDialogTest, LinkingWithoutChildrenLeavesPointersNull) {
    cCharStateDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    EXPECT_EQ(dlg.GetPKBtn(),         nullptr);
    EXPECT_EQ(dlg.GetMoveBtn(),       nullptr);
    EXPECT_EQ(dlg.GetKyungGongBtn(),  nullptr);
    EXPECT_EQ(dlg.GetPeaceWarBtn(),   nullptr);
    EXPECT_EQ(dlg.GetUngiBtn(),       nullptr);
}

// ===========================================================================
// SetXxxMode (REAL — no singleton, end-to-end testable)
// ===========================================================================

TEST(CCharStateDialogTest, SetPKModePushesUnpushesBtn) {
    cCharStateDialog dlg;
    std::vector<cPushupButton*> raws;
    BuildDlgWithPushups(dlg, raws);
    ASSERT_NE(dlg.GetPKBtn(), nullptr);

    dlg.SetPKMode(true);
    EXPECT_TRUE(dlg.GetPKBtn()->IsPushed());
    dlg.SetPKMode(false);
    EXPECT_FALSE(dlg.GetPKBtn()->IsPushed());
}

TEST(CCharStateDialogTest, SetMoveModePushesUnpushesBtn) {
    cCharStateDialog dlg;
    std::vector<cPushupButton*> raws;
    BuildDlgWithPushups(dlg, raws);
    ASSERT_NE(dlg.GetMoveBtn(), nullptr);

    dlg.SetMoveMode(true);
    EXPECT_TRUE(dlg.GetMoveBtn()->IsPushed());
    dlg.SetMoveMode(false);
    EXPECT_FALSE(dlg.GetMoveBtn()->IsPushed());
}

TEST(CCharStateDialogTest, SetKyungGongModePushesUnpushesBtn) {
    cCharStateDialog dlg;
    std::vector<cPushupButton*> raws;
    BuildDlgWithPushups(dlg, raws);
    ASSERT_NE(dlg.GetKyungGongBtn(), nullptr);

    dlg.SetKyungGongMode(true);
    EXPECT_TRUE(dlg.GetKyungGongBtn()->IsPushed());
    dlg.SetKyungGongMode(false);
    EXPECT_FALSE(dlg.GetKyungGongBtn()->IsPushed());
}

TEST(CCharStateDialogTest, SetPeaceWarModeInvertsArgument) {
    // 1:1 quirk: SetPeaceWarMode inverts its argument.
    // bPeace = true (peace) → button NOT pushed (shows war is OFF).
    // bPeace = false (war)  → button pushed (shows war is ON).
    cCharStateDialog dlg;
    std::vector<cPushupButton*> raws;
    BuildDlgWithPushups(dlg, raws);
    ASSERT_NE(dlg.GetPeaceWarBtn(), nullptr);

    dlg.SetPeaceWarMode(/*bPeace=*/true);
    EXPECT_FALSE(dlg.GetPeaceWarBtn()->IsPushed());
    dlg.SetPeaceWarMode(/*bPeace=*/false);
    EXPECT_TRUE(dlg.GetPeaceWarBtn()->IsPushed());
}

TEST(CCharStateDialogTest, SetUngiModePushesUnpushesBtn) {
    cCharStateDialog dlg;
    std::vector<cPushupButton*> raws;
    BuildDlgWithPushups(dlg, raws);
    ASSERT_NE(dlg.GetUngiBtn(), nullptr);

    dlg.SetUngiMode(true);
    EXPECT_TRUE(dlg.GetUngiBtn()->IsPushed());
    dlg.SetUngiMode(false);
    EXPECT_FALSE(dlg.GetUngiBtn()->IsPushed());
}

TEST(CCharStateDialogTest, SetXxxModeOnlyAffectsTargetButton) {
    // Each SetXxxMode must only affect its own button —
    // setting Move mode must not push PK.
    cCharStateDialog dlg;
    std::vector<cPushupButton*> raws;
    BuildDlgWithPushups(dlg, raws);
    for (auto* btn : raws) ASSERT_NE(btn, nullptr);

    dlg.SetMoveMode(true);
    EXPECT_TRUE(dlg.GetMoveBtn()->IsPushed());
    EXPECT_FALSE(dlg.GetPKBtn()->IsPushed());
    EXPECT_FALSE(dlg.GetKyungGongBtn()->IsPushed());
    EXPECT_FALSE(dlg.GetPeaceWarBtn()->IsPushed());
    EXPECT_FALSE(dlg.GetUngiBtn()->IsPushed());
}

TEST(CCharStateDialogTest, SetXxxModeWithoutLinksIsSafe) {
    // Calling SetXxxMode when no children are linked
    // must not crash. The modern port null-checks each
    // pointer before SetPush (1:1 with legacy spirit,
    // modern port is more defensive).
    cCharStateDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    dlg.SetPKMode(true);
    dlg.SetMoveMode(true);
    dlg.SetKyungGongMode(true);
    dlg.SetPeaceWarMode(true);
    dlg.SetUngiMode(true);
    SUCCEED();
}

TEST(CCharStateDialogTest, AllFiveSetXxxModeIndependently) {
    // Sanity: pushing all 5 modes leaves all 5 buttons pushed.
    cCharStateDialog dlg;
    std::vector<cPushupButton*> raws;
    BuildDlgWithPushups(dlg, raws);
    for (auto* btn : raws) ASSERT_NE(btn, nullptr);

    dlg.SetPKMode(true);
    dlg.SetMoveMode(true);
    dlg.SetKyungGongMode(true);
    dlg.SetPeaceWarMode(true);  // bPeace=true → button NOT pushed
    dlg.SetUngiMode(true);
    EXPECT_TRUE(dlg.GetPKBtn()->IsPushed());
    EXPECT_TRUE(dlg.GetMoveBtn()->IsPushed());
    EXPECT_TRUE(dlg.GetKyungGongBtn()->IsPushed());
    EXPECT_FALSE(dlg.GetPeaceWarBtn()->IsPushed());  // inverted
    EXPECT_TRUE(dlg.GetUngiBtn()->IsPushed());
}

// ===========================================================================
// OnActionEvent (deferred to MACROMGR + PKMGR port)
// ===========================================================================

TEST(CCharStateDialogTest, OnActionEventIsNoOpUntilMacroManagerPort) {
    // 1:1 quirk: OnActionEvent in the modern port is a
    // no-op until MACROMGR + PKMGR singletons are
    // ported. The 5 button ids + WE_PUSHUP/ WE_PUSHDOWN
    // state-machine shape is preserved.
    cCharStateDialog dlg;
    std::vector<cPushupButton*> raws;
    BuildDlgWithPushups(dlg, raws);
    for (auto* btn : raws) ASSERT_NE(btn, nullptr);

    // Test all 5 button ids with WE_PUSHUP + WE_PUSHDOWN.
    const std::uint32_t we_pushup   = 0x01;  // legacy WE_PUSHUP
    const std::uint32_t we_pushdown = 0x02;  // legacy WE_PUSHDOWN
    dlg.OnActionEvent(cCharStateDialog::kBtnPKId,        nullptr, we_pushup);
    dlg.OnActionEvent(cCharStateDialog::kBtnMoveId,      nullptr, we_pushup);
    dlg.OnActionEvent(cCharStateDialog::kBtnKyungGongId, nullptr, we_pushup);
    dlg.OnActionEvent(cCharStateDialog::kBtnPeaceWarId,  nullptr, we_pushdown);
    dlg.OnActionEvent(cCharStateDialog::kBtnUngiId,      nullptr, we_pushdown);
    // No observable state change (the OnActionEvent
    // body is a no-op).
    EXPECT_FALSE(dlg.GetPKBtn()->IsPushed());
    EXPECT_FALSE(dlg.GetMoveBtn()->IsPushed());
    EXPECT_FALSE(dlg.GetKyungGongBtn()->IsPushed());
    EXPECT_FALSE(dlg.GetPeaceWarBtn()->IsPushed());
    EXPECT_FALSE(dlg.GetUngiBtn()->IsPushed());
}

TEST(CCharStateDialogTest, OnActionEventUnknownIdIsNoOp) {
    cCharStateDialog dlg;
    std::vector<cPushupButton*> raws;
    BuildDlgWithPushups(dlg, raws);
    dlg.OnActionEvent(/*unknown=*/99999, nullptr, /*we=*/0x01);
    SUCCEED();
}

// ===========================================================================
// Refresh (deferred to SCRIPTMGR + RESRCMGR + MACROMGR + GAMEIN port)
// ===========================================================================

TEST(CCharStateDialogTest, RefreshIsNoOpUntilScriptManagerPort) {
    // 1:1 quirk: Refresh in the modern port is a no-op
    // until SCRIPTMGR + RESRCMGR + MACROMGR + GAMEIN
    // singletons are ported. The 3-active-tooltip
    // pattern (Move + PeaceWar active, KyungGong + Ungi
    // commented out) is preserved in the TODO.
    cCharStateDialog dlg;
    std::vector<cPushupButton*> raws;
    BuildDlgWithPushups(dlg, raws);
    dlg.Refresh();
    // No crash, no observable state change.
    SUCCEED();
}

TEST(CCharStateDialogTest, RefreshBeforeLinkingIsSafe) {
    // Defensive: Refresh before Linking should still be
    // a safe no-op (the modern port's body doesn't touch
    // any state).
    cCharStateDialog dlg;
    dlg.Refresh();
    SUCCEED();
}

}  // namespace mxh::ui::test
