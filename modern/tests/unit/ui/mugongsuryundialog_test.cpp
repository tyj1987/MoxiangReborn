// mugongsuryundialog_test.cpp — 1:1 port verification tests for
// cMugongSuryunDialog.

#include "mugongsuryundialog.hpp"
#include "ctabdialog.hpp"
#include "cpushupbutton.hpp"
#include "cwindow.hpp"
#include "cdialog.hpp"
#include "cmsgbox.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>

using mxh::ui::cMugongSuryunDialog;
using mxh::ui::cTabDialog;
using mxh::ui::cPushupButton;
using mxh::ui::cWindow;
using mxh::ui::cDialog;
using mxh::ui::cIcon;
using mxh::ui::CMugongDialog;
using mxh::ui::CSuryunDialog;
using mxh::ui::kMbiMugongDelete;

namespace {

std::unique_ptr<cMugongSuryunDialog> MakeDialog() {
    auto d = std::make_unique<cMugongSuryunDialog>();
    d->Init(0, 0, 200, 100, nullptr, 0);
    d->InitTab(4);
    return d;
}

std::unique_ptr<cPushupButton> MakeTabBtn(int id) {
    auto b = std::make_unique<cPushupButton>();
    b->Init(0, 0, 30, 20, nullptr, nullptr, nullptr, nullptr, nullptr, id);
    return b;
}

std::unique_ptr<CMugongDialog> MakeMugongDlg(int id) {
    auto d = std::make_unique<CMugongDialog>();
    d->Init(0, 0, 100, 80, nullptr, id);
    return d;
}

std::unique_ptr<CSuryunDialog> MakeSuryunDlg(int id) {
    auto d = std::make_unique<CSuryunDialog>();
    d->Init(0, 0, 100, 80, nullptr, id);
    return d;
}

class CMugongSuryunDialogTest : public ::testing::Test {
protected:
    void SetUp() override { cMugongSuryunDialog::ClearTestInjections(); }
    void TearDown() override { cMugongSuryunDialog::ClearTestInjections(); }
};

}  // namespace

// ---------------------------------------------------------------------------
// Constants + construction
// ---------------------------------------------------------------------------

TEST_F(CMugongSuryunDialogTest, IdConstantMatchesLocalRange) {
    EXPECT_EQ(kMbiMugongDelete, 2300);
    // 1:1 with legacy MBI_MUGONGDELETE (from WindowIDEnum.h).
}

TEST_F(CMugongSuryunDialogTest, DefaultChildPointersAreNull) {
    auto d = MakeDialog();
    EXPECT_EQ(d->GetMugongDialog(), nullptr);
    EXPECT_EQ(d->GetSuryunDialog(), nullptr);
}

TEST_F(CMugongSuryunDialogTest, DefaultCountersAreZero) {
    auto d = MakeDialog();
    EXPECT_EQ(d->addCallCount(), 0u);
    EXPECT_EQ(d->onActionEventCallCount(), 0u);
    EXPECT_EQ(d->fakeMoveIconCallCount(), 0u);
    EXPECT_EQ(d->setDisableFalseCount(), 0u);
    EXPECT_EQ(d->msgboxDismissCount(), 0u);
}

// ---------------------------------------------------------------------------
// Add() — legacy 1:1 quirk: m_pSuryunDlg is NEVER set (legacy bug)
// ---------------------------------------------------------------------------

TEST_F(CMugongSuryunDialogTest, AddMugongDialogCapturesMugongDlg) {
    // 1:1 with legacy: WT_MUGONGDIALOG → m_pMugongDlg.
    auto d = MakeDialog();
    auto mugong = MakeMugongDlg(100);
    CMugongDialog* rawMugong = mugong.get();
    d->Add(mugong.release());
    EXPECT_EQ(d->GetMugongDialog(), rawMugong);
}

TEST_F(CMugongSuryunDialogTest, AddMugongDialogIncrementsCurIdx2) {
    // 1:1 with legacy: second dispatch (WT_MUGONGDIALOG ||
    // WT_SURYUNDIALOG) routes to AddTabSheet + curIdx2++.
    auto d = MakeDialog();
    d->Add(MakeMugongDlg(100).release());
    EXPECT_EQ(d->curIdx2(), 1u);
    EXPECT_NE(d->GetTabSheet(0), nullptr);
}

TEST_F(CMugongSuryunDialogTest, AddSuryunDialogDoesNotCaptureSuryunDlgLegacyBug) {
    // 1:1 quirk: legacy Add has a TYPO bug — the first if-
    // pair's second branch checks `WT_MUGONGDIALOG` (not
    // `WT_SURYUNDIALOG`), so m_pSuryunDlg is never set.
    // The second if-pair (curIdx2++ branch) DOES catch
    // WT_SURYUNDIALOG correctly, so the tab sheet is added
    // but m_pSuryunDlg raw ptr stays nullptr.
    //
    // Modern port: 1:1 preserve the bug — m_pSuryunDlg
    // stays nullptr even after a SuryunDialog is added.
    auto d = MakeDialog();
    auto suryun = MakeSuryunDlg(200);
    d->Add(suryun.release());
    // 1:1 quirk: the legacy bug means m_pSuryunDlg is null.
    EXPECT_EQ(d->GetSuryunDialog(), nullptr);
    // The tab sheet IS added (second if-pair works correctly).
    EXPECT_EQ(d->curIdx2(), 1u);
    EXPECT_NE(d->GetTabSheet(0), nullptr);
}

TEST_F(CMugongSuryunDialogTest, AddPushupButtonRoutesToTabBtn) {
    auto d = MakeDialog();
    auto btn = MakeTabBtn(101);
    d->Add(btn.release());
    EXPECT_NE(d->GetTabBtn(0), nullptr);
    EXPECT_EQ(d->curIdx1(), 1u);
}

TEST_F(CMugongSuryunDialogTest, AddNullIsTolerated) {
    // 1:1 quirk: legacy `window->GetType()` would crash on
    // null. Modern port: defensive null guard (1:1 quirk
    // documented).
    auto d = MakeDialog();
    d->Add(nullptr);
    EXPECT_EQ(d->addCallCount(), 1u);  // count incremented
    SUCCEED();
}

TEST_F(CMugongSuryunDialogTest, AddIncrementsAddCallCount) {
    auto d = MakeDialog();
    d->Add(MakeTabBtn(101).release());
    d->Add(MakeMugongDlg(100).release());
    EXPECT_EQ(d->addCallCount(), 2u);
}

// ---------------------------------------------------------------------------
// SetActive override
// ---------------------------------------------------------------------------

TEST_F(CMugongSuryunDialogTest, SetActiveTrueDoesNotDismissMsgbox) {
    // 1:1 with legacy: the `if(!val)` block only runs on
    // val==FALSE. val==TRUE skips the msgbox-dismissal +
    // SetDisable(FALSE) path.
    auto d = MakeDialog();
    cMugongSuryunDialog::SetMsgboxPresentForTesting(true);
    d->SetActive(true);
    EXPECT_EQ(d->msgboxDismissCount(), 0u);
    EXPECT_EQ(d->setDisableFalseCount(), 0u);
}

TEST_F(CMugongSuryunDialogTest, SetActiveFalseDismissesMsgboxWhenPresent) {
    // 1:1 with legacy: val==FALSE dismisses the
    // MBI_MUGONGDELETE msgbox via WINDOWMGR (stubbed).
    auto d = MakeDialog();
    cMugongSuryunDialog::SetMsgboxPresentForTesting(true);
    d->SetActive(false);
    EXPECT_EQ(d->msgboxDismissCount(), 1u);
}

TEST_F(CMugongSuryunDialogTest, SetActiveFalseNoMsgboxSkipsDismiss) {
    // 1:1 with legacy: if the msgbox is not present, the
    // dismiss counter is not incremented.
    auto d = MakeDialog();
    cMugongSuryunDialog::SetMsgboxPresentForTesting(false);
    d->SetActive(false);
    EXPECT_EQ(d->msgboxDismissCount(), 0u);
}

TEST_F(CMugongSuryunDialogTest, SetActiveFalseCallsSetDisableFalse) {
    // 1:1 with legacy: SetDisable(FALSE) is called on self
    // when val==FALSE (self-undo).
    auto d = MakeDialog();
    d->SetActive(false);
    EXPECT_EQ(d->setDisableFalseCount(), 1u);
}

TEST_F(CMugongSuryunDialogTest, SetActiveFalseDelegatesToBaseLast) {
    // 1:1 with legacy: cTabDialog::SetActive(val) is called
    // LAST (after the msgbox-dismissal + self-undo).
    auto d = MakeDialog();
    d->SetActive(false);
    EXPECT_FALSE(d->isActive());
}

TEST_F(CMugongSuryunDialogTest, SetActiveTrueDelegatesToBase) {
    auto d = MakeDialog();
    d->SetActive(true);
    EXPECT_TRUE(d->isActive());
}

// ---------------------------------------------------------------------------
// FakeMoveIcon
// ---------------------------------------------------------------------------

TEST_F(CMugongSuryunDialogTest, FakeMoveIconForwardsToMugongDlg) {
    // 1:1 with legacy: FakeMoveIcon wraps
    // m_pMugongDlg->FakeMoveIcon(x, y, icon).
    auto d = MakeDialog();
    auto mugong = MakeMugongDlg(100);
    d->Add(mugong.release());
    d->FakeMoveIcon(0, 0, nullptr);
    EXPECT_EQ(CMugongDialog::fakeMoveIconCallCount(), 1u);
    EXPECT_EQ(d->fakeMoveIconCallCount(), 1u);  // outer counter
}

TEST_F(CMugongSuryunDialogTest, FakeMoveIconWithoutMugongDlgIsTolerated) {
    // 1:1 quirk: legacy UB if m_pMugongDlg is null. Modern
    // port: defensive null guard returning false.
    auto d = MakeDialog();
    EXPECT_FALSE(d->FakeMoveIcon(0, 0, nullptr));
    SUCCEED();
}

// ---------------------------------------------------------------------------
// OnActionEvent (empty body)
// ---------------------------------------------------------------------------

TEST_F(CMugongSuryunDialogTest, OnActionEventIsNoOp) {
    // 1:1 quirk: legacy OnActionEvent body is empty. Modern
    // port: increment counter for test inspection, but no
    // observable side effect on dialog state.
    auto d = MakeDialog();
    d->OnActionEvent(12345, nullptr, 0u);
    EXPECT_EQ(d->onActionEventCallCount(), 1u);
}

// ---------------------------------------------------------------------------
// Test-injection cleanup
// ---------------------------------------------------------------------------

TEST_F(CMugongSuryunDialogTest, ClearTestInjectionsResetsAllState) {
    cMugongSuryunDialog::SetMsgboxPresentForTesting(true);
    cMugongSuryunDialog::ClearTestInjections();
    EXPECT_FALSE(cMugongSuryunDialog::msgboxPresentForTesting());
    EXPECT_EQ(cMugongSuryunDialog::msgboxPresentForTesting(), false);
}
