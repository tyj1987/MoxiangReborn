// ctabdialog_test.cpp — 1:1 port verification tests for cTabDialog.

#include "ctabdialog.hpp"
#include "cpushupbutton.hpp"
#include "cwindow.hpp"
#include "cdialog.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>

using mxh::ui::cTabDialog;
using mxh::ui::cPushupButton;
using mxh::ui::cDialog;
using mxh::ui::cWindow;
using mxh::ui::CMouse;

namespace {

std::unique_ptr<cTabDialog> MakeDialog() {
    auto d = std::make_unique<cTabDialog>();
    d->Init(0, 0, 200, 100, nullptr, 0);
    return d;
}

std::unique_ptr<cPushupButton> MakeTabBtn(int id) {
    auto b = std::make_unique<cPushupButton>();
    b->Init(0, 0, 30, 20, nullptr, nullptr, nullptr, nullptr, nullptr, id);
    return b;
}

std::unique_ptr<cWindow> MakeTabSheet(int id) {
    auto s = std::make_unique<cWindow>();
    s->Init(0, 0, 100, 80, nullptr, id);
    return s;
}

class CTabDialogTest : public ::testing::Test {
protected:
    void SetUp() override { cTabDialog::ClearTestInjections(); }
    void TearDown() override { cTabDialog::ClearTestInjections(); }
};

}  // namespace

// ---------------------------------------------------------------------------
// Constants + construction
// ---------------------------------------------------------------------------

TEST_F(CTabDialogTest, DefaultConstructionHasNoTabs) {
    auto d = MakeDialog();
    EXPECT_EQ(d->GetTabNum(), 0u);
    EXPECT_EQ(d->GetCurTabNum(), 0u);
    EXPECT_EQ(d->curIdx1(), 0u);
    EXPECT_EQ(d->curIdx2(), 0u);
}

TEST_F(CTabDialogTest, DefaultGetTabBtnReturnsNull) {
    auto d = MakeDialog();
    EXPECT_EQ(d->GetTabBtn(0), nullptr);
    EXPECT_EQ(d->GetTabSheet(0), nullptr);
}

// ---------------------------------------------------------------------------
// InitTab
// ---------------------------------------------------------------------------

TEST_F(CTabDialogTest, InitTabSetsCapacity) {
    auto d = MakeDialog();
    d->InitTab(4);
    EXPECT_EQ(d->GetTabNum(), 4u);
}

TEST_F(CTabDialogTest, InitTabResetsSelectedToZero) {
    auto d = MakeDialog();
    d->InitTab(3);
    EXPECT_EQ(d->GetCurTabNum(), 0u);
}

TEST_F(CTabDialogTest, InitTabResetsCurIdxToZero) {
    auto d = MakeDialog();
    d->InitTab(3);
    EXPECT_EQ(d->curIdx1(), 0u);
    EXPECT_EQ(d->curIdx2(), 0u);
}

TEST_F(CTabDialogTest, InitTabAllowsReinit) {
    // 1:1 quirk: legacy InitTab overwrites m_ppPushupTabBtn +
    // m_ppWindowTabSheet (assumes previous was empty). Modern
    // port: clear() + resize(N) does the same. Reinit from 2
    // tabs to 5 tabs is a valid operation.
    auto d = MakeDialog();
    d->InitTab(2);
    d->InitTab(5);
    EXPECT_EQ(d->GetTabNum(), 5u);
}

TEST_F(CTabDialogTest, InitTabZeroIsValid) {
    auto d = MakeDialog();
    d->InitTab(0);
    EXPECT_EQ(d->GetTabNum(), 0u);
    EXPECT_EQ(d->GetTabBtn(0), nullptr);
}

// ---------------------------------------------------------------------------
// AddTabBtn + AddTabSheet
// ---------------------------------------------------------------------------

TEST_F(CTabDialogTest, AddTabBtnStoresAtIndex) {
    auto d = MakeDialog();
    d->InitTab(3);
    d->AddTabBtn(1, MakeTabBtn(101));
    ASSERT_NE(d->GetTabBtn(1), nullptr);
    EXPECT_EQ(d->GetTabBtn(1)->id(), 101);
}

TEST_F(CTabDialogTest, AddTabBtnOutOfRangeIsSilent) {
    // 1:1 quirk: legacy ASSERT(idx < m_bTabNum) — modern port
    // silent no-op (no exceptions in modern UI).
    auto d = MakeDialog();
    d->InitTab(2);
    d->AddTabBtn(5, MakeTabBtn(101));
    EXPECT_EQ(d->GetTabBtn(5), nullptr);
    SUCCEED();
}

TEST_F(CTabDialogTest, AddTabBtnNullIsTolerated) {
    auto d = MakeDialog();
    d->InitTab(2);
    d->AddTabBtn(0, nullptr);
    EXPECT_EQ(d->GetTabBtn(0), nullptr);
    SUCCEED();
}

TEST_F(CTabDialogTest, AddTabSheetStoresAtIndex) {
    auto d = MakeDialog();
    d->InitTab(3);
    d->AddTabSheet(2, MakeTabSheet(202));
    ASSERT_NE(d->GetTabSheet(2), nullptr);
    EXPECT_EQ(d->GetTabSheet(2)->id(), 202);
}

TEST_F(CTabDialogTest, AddTabSheetOutOfRangeIsSilent) {
    auto d = MakeDialog();
    d->InitTab(2);
    d->AddTabSheet(99, MakeTabSheet(999));
    EXPECT_EQ(d->GetTabSheet(99), nullptr);
    SUCCEED();
}

// ---------------------------------------------------------------------------
// SelectTab
// ---------------------------------------------------------------------------

TEST_F(CTabDialogTest, SelectTabPushesSelectedButton) {
    auto d = MakeDialog();
    d->InitTab(3);
    d->AddTabBtn(0, MakeTabBtn(100));
    d->AddTabBtn(1, MakeTabBtn(101));
    d->AddTabBtn(2, MakeTabBtn(102));
    d->SelectTab(1);
    EXPECT_TRUE(d->GetTabBtn(1)->IsPushed());
    EXPECT_FALSE(d->GetTabBtn(0)->IsPushed());
    EXPECT_FALSE(d->GetTabBtn(2)->IsPushed());
}

TEST_F(CTabDialogTest, SelectTabActivatesSelectedSheet) {
    auto d = MakeDialog();
    d->InitTab(3);
    d->AddTabSheet(0, MakeTabSheet(200));
    d->AddTabSheet(1, MakeTabSheet(201));
    d->AddTabSheet(2, MakeTabSheet(202));
    d->SelectTab(2);
    EXPECT_TRUE(d->GetTabSheet(2)->isVisible());
    EXPECT_FALSE(d->GetTabSheet(0)->isVisible());
    EXPECT_FALSE(d->GetTabSheet(1)->isVisible());
}

TEST_F(CTabDialogTest, SelectTabUpdatesSelTabNum) {
    auto d = MakeDialog();
    d->InitTab(3);
    d->SelectTab(2);
    EXPECT_EQ(d->GetCurTabNum(), 2u);
}

TEST_F(CTabDialogTest, SelectTabOutOfRangeIsSilent) {
    // 1:1 quirk: legacy `if (idx >= m_bTabNum) return;` guard.
    auto d = MakeDialog();
    d->InitTab(3);
    d->AddTabBtn(0, MakeTabBtn(100));
    d->SelectTab(99);
    EXPECT_EQ(d->GetCurTabNum(), 0u);  // unchanged
    SUCCEED();
}

TEST_F(CTabDialogTest, SelectTabWithNullButtonsIsTolerated) {
    // 1:1 quirk: legacy would crash on null tab btn; modern
    // port is defensive.
    auto d = MakeDialog();
    d->InitTab(3);
    d->SelectTab(1);
    EXPECT_EQ(d->GetCurTabNum(), 1u);
    SUCCEED();
}

// ---------------------------------------------------------------------------
// SetActive override
// ---------------------------------------------------------------------------

TEST_F(CTabDialogTest, SetActiveTrueCascadesToAllTabBtns) {
    auto d = MakeDialog();
    d->InitTab(3);
    d->AddTabBtn(0, MakeTabBtn(100));
    d->AddTabBtn(1, MakeTabBtn(101));
    d->SetActive(true);
    EXPECT_TRUE(d->GetTabBtn(0)->isVisible());
    EXPECT_TRUE(d->GetTabBtn(1)->isVisible());
}

TEST_F(CTabDialogTest, SetActiveTrueActivatesOnlySelectedSheet) {
    // 1:1 quirk: legacy only activates the selected tab's sheet
    // on val==TRUE; the other sheets stay inactive.
    auto d = MakeDialog();
    d->InitTab(3);
    d->AddTabSheet(0, MakeTabSheet(200));
    d->AddTabSheet(1, MakeTabSheet(201));
    d->AddTabSheet(2, MakeTabSheet(202));
    d->SelectTab(1);
    d->SetActive(true);
    EXPECT_FALSE(d->GetTabSheet(0)->isVisible());
    EXPECT_TRUE(d->GetTabSheet(1)->isVisible());
    EXPECT_FALSE(d->GetTabSheet(2)->isVisible());
}

TEST_F(CTabDialogTest, SetActiveFalseDeactivatesAllSheets) {
    auto d = MakeDialog();
    d->InitTab(3);
    d->AddTabSheet(0, MakeTabSheet(200));
    d->AddTabSheet(1, MakeTabSheet(201));
    d->SelectTab(0);
    d->SetActive(true);
    d->SetActive(false);
    EXPECT_FALSE(d->GetTabSheet(0)->isVisible());
    EXPECT_FALSE(d->GetTabSheet(1)->isVisible());
}

// ---------------------------------------------------------------------------
// SetAbsXY cascade
// ---------------------------------------------------------------------------

TEST_F(CTabDialogTest, SetAbsXYCascadesToTabBtns) {
    // 1:1 with legacy: SetAbsXY(x, y) moves the tab btns by
    // the same delta. Initial position 0,0; new position 50,30;
    // each tab btn should now be at (50 + relX, 30 + relY).
    auto d = MakeDialog();
    d->InitTab(2);
    auto btn0 = MakeTabBtn(100);
    btn0->SetRelXY(10, 5);
    auto btn1 = MakeTabBtn(101);
    btn1->SetRelXY(20, 8);
    d->AddTabBtn(0, std::move(btn0));
    d->AddTabBtn(1, std::move(btn1));
    d->SetAbsXY(50, 30);
    EXPECT_EQ(d->GetTabBtn(0)->absX(), 60);  // 50 + 10
    EXPECT_EQ(d->GetTabBtn(0)->absY(), 35);  // 30 + 5
    EXPECT_EQ(d->GetTabBtn(1)->absX(), 70);  // 50 + 20
    EXPECT_EQ(d->GetTabBtn(1)->absY(), 38);  // 30 + 8
}

TEST_F(CTabDialogTest, SetAbsXYCascadesToTabSheets) {
    auto d = MakeDialog();
    d->InitTab(2);
    auto sheet0 = MakeTabSheet(200);
    sheet0->SetRelXY(5, 5);
    auto sheet1 = MakeTabSheet(201);
    sheet1->SetRelXY(15, 10);
    d->AddTabSheet(0, std::move(sheet0));
    d->AddTabSheet(1, std::move(sheet1));
    d->SetAbsXY(100, 50);
    EXPECT_EQ(d->GetTabSheet(0)->absX(), 105);
    EXPECT_EQ(d->GetTabSheet(0)->absY(), 55);
    EXPECT_EQ(d->GetTabSheet(1)->absX(), 115);
    EXPECT_EQ(d->GetTabSheet(1)->absY(), 60);
}

// ---------------------------------------------------------------------------
// GetWindowForID override
// ---------------------------------------------------------------------------

TEST_F(CTabDialogTest, FindAnyWindowForIDFindsDirectChild) {
    // 1:1 with legacy: parent cDialog's GetWindowForID is
    // consulted first; if it finds a direct child, return
    // that. Modern port: FindAnyWindowForID searches
    // findWindowById first (which finds direct children
    // added via Add).
    auto d = MakeDialog();
    d->InitTab(2);
    auto btn0 = MakeTabBtn(100);
    d->AddTabBtn(0, std::move(btn0));
    cWindow* found = d->FindAnyWindowForID(100);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->id(), 100);
}

TEST_F(CTabDialogTest, FindAnyWindowForIDFindsTabSheet) {
    // 1:1 with legacy: if the direct cDialog lookup misses,
    // iterate the tab btns + sheets.
    auto d = MakeDialog();
    d->InitTab(2);
    d->AddTabSheet(0, MakeTabSheet(200));
    cWindow* found = d->FindAnyWindowForID(200);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->id(), 200);
}

TEST_F(CTabDialogTest, FindAnyWindowForIDReturnsNullForUnknown) {
    auto d = MakeDialog();
    d->InitTab(2);
    d->AddTabBtn(0, MakeTabBtn(100));
    EXPECT_EQ(d->FindAnyWindowForID(999), nullptr);
}

// ---------------------------------------------------------------------------
// Render stubs
// ---------------------------------------------------------------------------

TEST_F(CTabDialogTest, RenderIsNoOp) {
    // 1:1 quirk: legacy Render() called cDialog::RenderWindow +
    // cTabDialog::RenderTabComponent + cDialog::RenderComponent,
    // but modern cDialog has only Render (no separate Window /
    // Component). Modern port: Render + RenderTabComponent are
    // no-op stubs (Phase 6.x render deferred).
    auto d = MakeDialog();
    d->InitTab(2);
    d->SetActive(true);
    d->Render();
    d->RenderTabComponent();
    SUCCEED();
}

TEST_F(CTabDialogTest, RenderTabComponentWhenInactiveIsNoOp) {
    auto d = MakeDialog();
    d->InitTab(2);
    d->RenderTabComponent();
    SUCCEED();
}

TEST_F(CTabDialogTest, ActionEventReturnsZeroForInactiveDialog) {
    // 1:1 with legacy: `if (!m_bActive) return we;` returns
    // WE_NULL=0.
    auto d = MakeDialog();
    d->InitTab(2);
    EXPECT_EQ(d->ActionEvent(nullptr), 0u);
}

// ---------------------------------------------------------------------------
// Dtor (1:1 quirk: legacy SAFE_DELETE → modern unique_ptr auto)
// ---------------------------------------------------------------------------

TEST_F(CTabDialogTest, DtorReleasesOwnedTabBtnsAndSheets) {
    // 1:1 quirk: legacy dtor manually SAFE_DELETE each btn +
    // sheet. Modern port: std::vector<std::unique_ptr<...>>
    // releases on dtor. The test verifies no crash + state
    // cleared (vector is destroyed with the dialog).
    auto d = MakeDialog();
    d->InitTab(3);
    d->AddTabBtn(0, MakeTabBtn(100));
    d->AddTabBtn(1, MakeTabBtn(101));
    d->AddTabSheet(0, MakeTabSheet(200));
    d->AddTabSheet(1, MakeTabSheet(201));
    d.reset();  // invokes ~cTabDialog
    SUCCEED();
}
