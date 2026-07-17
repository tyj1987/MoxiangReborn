// maindialog_test.cpp — 1:1 port tests for 墨香
// CMainDialog (main UI button bar dialog).
//
// Verifies:
//   - ctor does not crash (1:1 quirk: m_type = WT_MAINDIALOG drop)
//   - Dtor does not crash
//   - 4 id constants (kIdCharBtn / kIdInventoryBtn /
//     kIdMugongBtn / kIdPartyBtn) match expected
//     local range 530-533
//   - 4 idx constants (kIdxChar=0 / kIdxInventory=1 /
//     kIdxMugong=2 / kIdxParty=3) match expected
//   - kNumBtns = 4
//   - Inherits from cDialog (tree management
//     passes through)
//   - Linking creates 4 cPushupButton and stores
//     them at the 4 valid indices
//   - GetPushupBtn returns the right pointer for
//     each of the 4 indices
//   - GetPushupBtn returns nullptr for OOB idx
//     (defensive; legacy is UB)
//   - GetPushupBtn before Linking returns nullptr
//   - Each cPushupButton has the right id
//     (matches kIdXxxBtn)
//   - Each cPushupButton is unique (different
//     pointer addresses)
//   - Each cPushupButton is non-null

#include "maindialog.hpp"
#include "cdialog.hpp"
#include "cpushupbutton.hpp"
#include "cbutton.hpp"
#include "cwindow.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <type_traits>

using mxh::ui::cButton;
using mxh::ui::cDialog;
using mxh::ui::cMainDialog;
using mxh::ui::cPushupButton;
using mxh::ui::cWindow;

namespace {

// helper: build a cMainDialog + Linking() for tests
// (cMainDialog is move-only since cDialog is non-copyable, so
// we expose the test on the original instance instead of
// returning by value)
void LinkDialog(cMainDialog& dlg) {
    dlg.Init(0, 0, 100, 100, nullptr, 0);
    dlg.Linking();
}

}  // namespace

// ---------- ctor / dtor ----------

TEST(CMainDialogTest, CtorDoesNotCrash) {
    cMainDialog dlg;
    SUCCEED();
}

TEST(CMainDialogTest, DtorDoesNotCrash) {
    cMainDialog dlg;
    SUCCEED();
}

TEST(CMainDialogTest, InheritsFromCDialog) {
    static_assert(std::is_base_of_v<cDialog, cMainDialog>,
                  "cMainDialog must inherit from cDialog");
    SUCCEED();
}

TEST(CMainDialogTest, IsACDialog) {
    cMainDialog dlg;
    cDialog* base = &dlg;
    EXPECT_NE(base, nullptr);
}

// ---------- id range ----------

TEST(CMainDialogTest, IdConstantsMatchExpectedLocalRange) {
    EXPECT_EQ(cMainDialog::kIdInventoryBtn, 530);
    EXPECT_EQ(cMainDialog::kIdMugongBtn, 531);
    EXPECT_EQ(cMainDialog::kIdCharBtn, 532);
    EXPECT_EQ(cMainDialog::kIdPartyBtn, 533);
}

TEST(CMainDialogTest, IdConstantsAreUnique) {
    EXPECT_NE(cMainDialog::kIdInventoryBtn, cMainDialog::kIdMugongBtn);
    EXPECT_NE(cMainDialog::kIdInventoryBtn, cMainDialog::kIdCharBtn);
    EXPECT_NE(cMainDialog::kIdInventoryBtn, cMainDialog::kIdPartyBtn);
    EXPECT_NE(cMainDialog::kIdMugongBtn, cMainDialog::kIdCharBtn);
    EXPECT_NE(cMainDialog::kIdMugongBtn, cMainDialog::kIdPartyBtn);
    EXPECT_NE(cMainDialog::kIdCharBtn, cMainDialog::kIdPartyBtn);
}

TEST(CMainDialogTest, IdxConstantsMatchExpectedValues) {
    EXPECT_EQ(cMainDialog::kIdxChar, 0);
    EXPECT_EQ(cMainDialog::kIdxInventory, 1);
    EXPECT_EQ(cMainDialog::kIdxMugong, 2);
    EXPECT_EQ(cMainDialog::kIdxParty, 3);
}

TEST(CMainDialogTest, NumBtnsIsFour) {
    EXPECT_EQ(cMainDialog::kNumBtns, 4);
}

// ---------- Linking ----------

TEST(CMainDialogTest, LinkingPopulatesAllFourPushupButtons) {
    cMainDialog dlg;
    dlg.Init(0, 0, 100, 100, nullptr, 0);
    dlg.Linking();
    EXPECT_NE(dlg.GetPushupBtn(cMainDialog::kIdxChar), nullptr);
    EXPECT_NE(dlg.GetPushupBtn(cMainDialog::kIdxInventory), nullptr);
    EXPECT_NE(dlg.GetPushupBtn(cMainDialog::kIdxMugong), nullptr);
    EXPECT_NE(dlg.GetPushupBtn(cMainDialog::kIdxParty), nullptr);
}

TEST(CMainDialogTest, LinkingBeforeInitDoesNotCrash) {
    cMainDialog dlg;
    // cMainDialog::Linking does not use any cDialog state, so calling
    // it before Init is safe in the modern port (synth-only).
    dlg.Linking();
    EXPECT_NE(dlg.GetPushupBtn(cMainDialog::kIdxChar), nullptr);
}

TEST(CMainDialogTest, EachLinkedBtnHasCorrectId) {
    cMainDialog dlg;
    dlg.Init(0, 0, 100, 100, nullptr, 0);
    dlg.Linking();
    EXPECT_EQ(dlg.GetPushupBtn(cMainDialog::kIdxInventory)->id(),
              cMainDialog::kIdInventoryBtn);
    EXPECT_EQ(dlg.GetPushupBtn(cMainDialog::kIdxMugong)->id(),
              cMainDialog::kIdMugongBtn);
    EXPECT_EQ(dlg.GetPushupBtn(cMainDialog::kIdxChar)->id(),
              cMainDialog::kIdCharBtn);
    EXPECT_EQ(dlg.GetPushupBtn(cMainDialog::kIdxParty)->id(),
              cMainDialog::kIdPartyBtn);
}

TEST(CMainDialogTest, LinkedBtnsAreUniqueInstances) {
    cMainDialog dlg;
    dlg.Init(0, 0, 100, 100, nullptr, 0);
    dlg.Linking();
    cPushupButton* a = dlg.GetPushupBtn(cMainDialog::kIdxChar);
    cPushupButton* b = dlg.GetPushupBtn(cMainDialog::kIdxInventory);
    cPushupButton* c = dlg.GetPushupBtn(cMainDialog::kIdxMugong);
    cPushupButton* p = dlg.GetPushupBtn(cMainDialog::kIdxParty);
    EXPECT_NE(a, b);
    EXPECT_NE(a, c);
    EXPECT_NE(a, p);
    EXPECT_NE(b, c);
    EXPECT_NE(b, p);
    EXPECT_NE(c, p);
}

TEST(CMainDialogTest, LinkedBtnsAreCButtons) {
    cMainDialog dlg;
    dlg.Init(0, 0, 100, 100, nullptr, 0);
    dlg.Linking();
    // cPushupButton is a cButton; verify the dynamic_cast works.
    cPushupButton* p = dlg.GetPushupBtn(cMainDialog::kIdxChar);
    cButton* base = static_cast<cButton*>(p);
    EXPECT_NE(base, nullptr);
    cWindow* winBase = static_cast<cWindow*>(p);
    EXPECT_NE(winBase, nullptr);
}

// ---------- GetPushupBtn ----------

TEST(CMainDialogTest, GetPushupBtnBeforeLinkingReturnsNull) {
    cMainDialog dlg;
    dlg.Init(0, 0, 100, 100, nullptr, 0);
    // Before Linking, all 4 are nullptr.
    EXPECT_EQ(dlg.GetPushupBtn(cMainDialog::kIdxChar), nullptr);
    EXPECT_EQ(dlg.GetPushupBtn(cMainDialog::kIdxInventory), nullptr);
    EXPECT_EQ(dlg.GetPushupBtn(cMainDialog::kIdxMugong), nullptr);
    EXPECT_EQ(dlg.GetPushupBtn(cMainDialog::kIdxParty), nullptr);
}

TEST(CMainDialogTest, GetPushupBtnOutOfRangeReturnsNull) {
    cMainDialog dlg;
    dlg.Init(0, 0, 100, 100, nullptr, 0);
    dlg.Linking();
    // OOB: 4, 5, 99, 65535 — all should be nullptr (defensive).
    EXPECT_EQ(dlg.GetPushupBtn(4), nullptr);
    EXPECT_EQ(dlg.GetPushupBtn(5), nullptr);
    EXPECT_EQ(dlg.GetPushupBtn(99), nullptr);
    EXPECT_EQ(dlg.GetPushupBtn(65535), nullptr);
}

TEST(CMainDialogTest, GetPushupBtnReturnsSamePointerOnRepeatedCalls) {
    cMainDialog dlg;
    dlg.Init(0, 0, 100, 100, nullptr, 0);
    dlg.Linking();
    cPushupButton* a = dlg.GetPushupBtn(cMainDialog::kIdxChar);
    cPushupButton* b = dlg.GetPushupBtn(cMainDialog::kIdxChar);
    EXPECT_EQ(a, b);
}

TEST(CMainDialogTest, LinkDialogHelperProducesValidState) {
    cMainDialog dlg;
    LinkDialog(dlg);
    EXPECT_NE(dlg.GetPushupBtn(cMainDialog::kIdxChar), nullptr);
    EXPECT_NE(dlg.GetPushupBtn(cMainDialog::kIdxInventory), nullptr);
    EXPECT_NE(dlg.GetPushupBtn(cMainDialog::kIdxMugong), nullptr);
    EXPECT_NE(dlg.GetPushupBtn(cMainDialog::kIdxParty), nullptr);
}

TEST(CMainDialogTest, LinkedBtnHasDefaultPushupStateFalse) {
    cMainDialog dlg;
    dlg.Init(0, 0, 100, 100, nullptr, 0);
    dlg.Linking();
    cPushupButton* p = dlg.GetPushupBtn(cMainDialog::kIdxChar);
    // Default m_pushed = false (matches legacy initial state).
    EXPECT_FALSE(p->IsPushed());
    EXPECT_FALSE(p->IsPassive());
}
