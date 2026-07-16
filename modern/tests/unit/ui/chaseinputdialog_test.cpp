// chaseinputdialog_test.cpp - Phase 12.x P2-12 Tier 2 dialog 1:1 port
// contract test for modern cChaseInputDialog (chase input dialog:
// enter target player name for wanted chase item).
//
// Covers modern/src/ui/chaseinputdialog.{hpp,cpp}, a 1:1 port of
//   墨香【源码】\[Client]MH\ChaseinputDialog.h (497 B) and
//   `墨香【源码】\[Client]MH\ChaseinputDialog.cpp`.
//
// What's tested:
//   - Default construction: 1 child pointer null, m_dwItemIdx
//     = 0, m_LastChktime = 0.
//   - Linking resolves the cEditBox child (kEditNameId=300)
//     by id + SetValidCheck(VCM_CHARNAME alias = 2).
//   - SetActive override calls base SetActive + clears
//     edit text + resets m_dwItemIdx when val=true.
//   - SetActive false does not modify edit text or item
//     idx (1:1 quirk: clear only on val=true).
//   - SetItemIdx updates m_dwItemIdx.
//   - SetItemIdx default + edge values.
//   - WantedChaseSyn is a no-op (6-singleton dispatch
//     TODO).
//   - Accessors return the linked child pointer + item
//     idx.
//   - VcmCharnameAlias is 2 (1:1 quirk).
//   - Defensive null-checks: Linking + SetActive +
//     WantedChaseSyn without link are safe.
//
// 1:1 quirks preserved:
//   - Ctor drops m_type = WT_CHASEINPUT_DLG (legacy
//     cWindow type tag removed in Phase 6).
//   - Linking calls SetValidCheck(VCM_CHARNAME alias = 2)
//     — closest modern equivalent for the legacy
//     cIMEex character-name validator (same as
//     cMiniFriendDialog).
//   - SetActive matches base noexcept (R-12 polymorphic
//     virtual required).
//   - SetActive only clears edit text + resets m_dwItemIdx
//     when val=true (1:1 with legacy).
//   - WantedChaseSyn is documented as TODO (6-singleton
//     dispatch deferred).

#include "chaseinputdialog.hpp"
#include "ceditbox.hpp"
#include "cdialog.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>

namespace mxh::ui::test {

// ===========================================================================
// Construction
// ===========================================================================

TEST(CChaseInputDialogTest, DefaultConstructionHasNullAndZero) {
    cChaseInputDialog dlg;
    EXPECT_EQ(dlg.GetEditName(), nullptr);
    EXPECT_EQ(dlg.GetItemIdx(),  0u);
}

TEST(CChaseInputDialogTest, IdConstantMatchesExpectedLocalRange) {
    // 1:1 quirk: pick 300 to avoid collisions with other
    // Tier 2 dialog id ranges (cCharMakeDlg 200-203,
    // cGuildJoinDialog 210-212, cCharStateDialog 220-224,
    // cSOSDialog 230-231, cMiniFriendDialog 240-243,
    // cReviveDialog 250-252, cMPNoticeDialog 260-261,
    // cEventNotifyDialog 270-271, cGuildCreateDialog
    // 280-284, cGuildUnionCreateDialog 290-292).
    EXPECT_EQ(cChaseInputDialog::kEditNameId, 300);
}

TEST(CChaseInputDialogTest, VcmCharnameAliasIsTwo) {
    // 1:1 quirk: VCM_CHARNAME alias = 2 (alpha only,
    // closest modern equivalent to legacy's cIMEex
    // character-name validator).
    EXPECT_EQ(cChaseInputDialog::kVcmCharnameAlias, 2);
}

// ===========================================================================
// Linking
// ===========================================================================

namespace {

// Build a cChaseInputDialog with 1 cEditBox child wired
// in the modern id range (300). Returns the raw pointer
// via the out param; ownership lives in the dlg (child
// is added via cWindow::Add).
void BuildDlgWithEdit(cChaseInputDialog& dlg, cEditBox*& out_edit) {
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    auto edit = std::make_unique<cEditBox>();
    edit->Init(0, 0, 200, 14, nullptr, nullptr,
               cChaseInputDialog::kEditNameId);
    // InitEditbox(50, 64) enables SetEditText
    // (cEditBox::m_maxBytes > 0).
    edit->InitEditbox(50, 64);
    out_edit = edit.get();
    dlg.Add(std::unique_ptr<cWindow>(edit.release()));
    dlg.Linking();
}

}  // namespace

TEST(CChaseInputDialogTest, LinkingResolvesEditBox) {
    cChaseInputDialog dlg;
    cEditBox* raw_edit = nullptr;
    BuildDlgWithEdit(dlg, raw_edit);

    EXPECT_EQ(dlg.GetEditName(), raw_edit);
}

TEST(CChaseInputDialogTest, LinkingConfiguresValidCheck) {
    // 1:1 quirk: legacy calls
    // m_pEditName->SetValidCheck(VCM_CHARNAME = 2).
    // Modern port uses kVcmCharnameAlias = 2.
    cChaseInputDialog dlg;
    cEditBox* raw_edit = nullptr;
    BuildDlgWithEdit(dlg, raw_edit);
    ASSERT_NE(dlg.GetEditName(), nullptr);
    EXPECT_EQ(dlg.GetEditName()->GetValidCheckMethod(), 2);
}

TEST(CChaseInputDialogTest, LinkingWithoutChildLeavesPointerNull) {
    cChaseInputDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    EXPECT_EQ(dlg.GetEditName(), nullptr);
}

TEST(CChaseInputDialogTest, LinkingBeforeInitDoesNotCrash) {
    cChaseInputDialog dlg;
    dlg.Linking();
    EXPECT_EQ(dlg.GetEditName(), nullptr);
}

// ===========================================================================
// SetActive (1:1 override, base + clear on val=true)
// ===========================================================================

TEST(CChaseInputDialogTest, SetActiveTrueUpdatesBaseState) {
    cChaseInputDialog dlg;
    cEditBox* raw_edit = nullptr;
    BuildDlgWithEdit(dlg, raw_edit);
    EXPECT_FALSE(dlg.isActive());

    dlg.SetActive(true);
    EXPECT_TRUE(dlg.isActive());
}

TEST(CChaseInputDialogTest, SetActiveFalseUpdatesBaseState) {
    cChaseInputDialog dlg;
    cEditBox* raw_edit = nullptr;
    BuildDlgWithEdit(dlg, raw_edit);
    dlg.SetActive(true);
    ASSERT_TRUE(dlg.isActive());

    dlg.SetActive(false);
    EXPECT_FALSE(dlg.isActive());
}

TEST(CChaseInputDialogTest, SetActiveTrueClearsEditText) {
    // 1:1 with legacy: SetActive(true) clears the
    // edit text + resets m_dwItemIdx.
    cChaseInputDialog dlg;
    cEditBox* raw_edit = nullptr;
    BuildDlgWithEdit(dlg, raw_edit);
    ASSERT_NE(dlg.GetEditName(), nullptr);

    // Set some text + item idx before activating.
    dlg.GetEditName()->SetEditText("TargetPlayer");
    dlg.SetItemIdx(42);
    ASSERT_EQ(dlg.GetItemIdx(), 42u);

    dlg.SetActive(true);

    EXPECT_EQ(dlg.GetEditName()->editText(), "");
    EXPECT_EQ(dlg.GetItemIdx(), 0u);
}

TEST(CChaseInputDialogTest, SetActiveFalseDoesNotClearState) {
    // 1:1 quirk: clear only happens on val=true.
    cChaseInputDialog dlg;
    cEditBox* raw_edit = nullptr;
    BuildDlgWithEdit(dlg, raw_edit);

    dlg.GetEditName()->SetEditText("KeepMe");
    dlg.SetItemIdx(99);
    dlg.SetActive(false);

    EXPECT_EQ(dlg.GetEditName()->editText(), "KeepMe");
    EXPECT_EQ(dlg.GetItemIdx(), 99u);
}

TEST(CChaseInputDialogTest, SetActiveWithoutLinkIsSafe) {
    cChaseInputDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    dlg.SetActive(true);
    dlg.SetActive(false);
    SUCCEED();
}

// ===========================================================================
// SetItemIdx
// ===========================================================================

TEST(CChaseInputDialogTest, SetItemIdxUpdatesValue) {
    cChaseInputDialog dlg;
    cEditBox* raw_edit = nullptr;
    BuildDlgWithEdit(dlg, raw_edit);
    dlg.SetItemIdx(42);
    EXPECT_EQ(dlg.GetItemIdx(), 42u);
    dlg.SetItemIdx(0);
    EXPECT_EQ(dlg.GetItemIdx(), 0u);
    dlg.SetItemIdx(0xFFFFFFFFu);
    EXPECT_EQ(dlg.GetItemIdx(), 0xFFFFFFFFu);
}

TEST(CChaseInputDialogTest, SetItemIdxBeforeInitDoesNotCrash) {
    cChaseInputDialog dlg;
    dlg.SetItemIdx(100);
    EXPECT_EQ(dlg.GetItemIdx(), 100u);
}

// ===========================================================================
// WantedChaseSyn (TODO 6-singleton dispatch)
// ===========================================================================

TEST(CChaseInputDialogTest, WantedChaseSynIsNoOp) {
    // 1:1 quirk: legacy WantedChaseSyn dispatches to
    // 6 singletons (gCurTime/CHATMGR/HERO/FILTERTABLE/
    // WANTEDMGR/NETWORK). Modern port: no-op until
    // those singletons are ported.
    cChaseInputDialog dlg;
    cEditBox* raw_edit = nullptr;
    BuildDlgWithEdit(dlg, raw_edit);
    dlg.WantedChaseSyn();
    // No observable state change (the body is a
    // no-op).
    SUCCEED();
}

TEST(CChaseInputDialogTest, WantedChaseSynWithoutLinkIsSafe) {
    cChaseInputDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.Linking();
    dlg.WantedChaseSyn();
    SUCCEED();
}

}  // namespace mxh::ui::test
