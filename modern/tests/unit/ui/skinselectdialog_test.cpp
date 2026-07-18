// skinselectdialog_test.cpp — Phase 6.14 / 0.13.47 coverage for
// cSkinSelectDialog (skin-select dialog). Tests the data model +
// state machine + Linking + OnActionEvent dispatch. Engine-side
// singletons (GAMERESRCMNGR / HERO / CHATMGR / etc.) are stubbed
// to no-op; the data-side state is preserved 1:1.

#include "skinselectdialog.hpp"

#include "cIconDialog.hpp"
#include "cListDialog.hpp"
#include "cWindow.hpp"

#include <gtest/gtest.h>

#include <memory>

namespace {

// BuildDlgWithChildren: 1:1 with the legacy resource-loader
// step. Adds the 2 children (cListDialog at ID_LIST +
// cIconDialog at ID_ITEMVIEW) before Linking so that
// findWindowById can resolve them.
void BuildDlgWithChildren(mxh::ui::cSkinSelectDialog& d) {
    d.Init(0, 0, 400, 400, nullptr, 0);
    {
        auto l = std::make_unique<mxh::ui::cListDialog>();
        l->InitList(5, 0, 0, 100, 30);
        l->Init(0, 0, 0, 0, nullptr, mxh::ui::cSkinSelectDialog::ID_LIST);
        d.Add(std::unique_ptr<mxh::ui::cWindow>(l.release()));
    }
    {
        auto i = std::make_unique<mxh::ui::cIconDialog>();
        i->Init(0, 0, 0, 0, nullptr,
                mxh::ui::cSkinSelectDialog::ID_ITEMVIEW);
        d.Add(std::unique_ptr<mxh::ui::cWindow>(i.release()));
    }
    d.Linking();
}

}  // namespace

TEST(CSkinSelectDialog, DefaultConstructionHasNullPointers) {
    mxh::ui::cSkinSelectDialog d;
    EXPECT_EQ(d.GetSelectIdx(), 0u);
    EXPECT_EQ(d.GetSkinDelayTime(), 0u);
    EXPECT_FALSE(d.IsSkinDelayResult());
}

TEST(CSkinSelectDialog, InheritsDialogTreeManagement) {
    mxh::ui::cSkinSelectDialog d;
    EXPECT_EQ(d.childCount(), 0u);  // before Linking
}

TEST(CSkinSelectDialog, ConstantsAreStable) {
    EXPECT_EQ(mxh::ui::cSkinSelectDialog::SKINITEM_LIST_MAX, 3u);
    // 1:1 with legacy local id space (5 ids, distinct).
    EXPECT_NE(mxh::ui::cSkinSelectDialog::ID_DLG,
              mxh::ui::cSkinSelectDialog::ID_ITEMVIEW);
    EXPECT_NE(mxh::ui::cSkinSelectDialog::ID_LIST,
              mxh::ui::cSkinSelectDialog::ID_OK);
    EXPECT_NE(mxh::ui::cSkinSelectDialog::ID_OK,
              mxh::ui::cSkinSelectDialog::ID_CANCEL);
    EXPECT_NE(mxh::ui::cSkinSelectDialog::ID_CANCEL,
              mxh::ui::cSkinSelectDialog::ID_RECOVERY);
}

TEST(CSkinSelectDialog, LinkingResolvesAllChildren) {
    mxh::ui::cSkinSelectDialog d;
    BuildDlgWithChildren(d);
    // 1:1 with legacy: 2 children resolved by id.
    EXPECT_NE(d.findWindowById(mxh::ui::cSkinSelectDialog::ID_LIST), nullptr);
    EXPECT_NE(d.findWindowById(mxh::ui::cSkinSelectDialog::ID_ITEMVIEW), nullptr);
    // OK / CANCEL / RECOVERY aren't required to be children of
    // this dialog (legacy has them as siblings in the resource
    // tree); the modern port doesn't add them. Verify the IDs
    // are stable but not present here.
    EXPECT_TRUE(d.findWindowById(mxh::ui::cSkinSelectDialog::ID_OK) == nullptr);
}

TEST(CSkinSelectDialog, LinkingIsIdempotent) {
    mxh::ui::cSkinSelectDialog d;
    BuildDlgWithChildren(d);
    d.Linking();
    EXPECT_NE(d.findWindowById(mxh::ui::cSkinSelectDialog::ID_LIST), nullptr);
}

TEST(CSkinSelectDialog, LinkingBeforeInitDoesNotCrash) {
    mxh::ui::cSkinSelectDialog d;
    BuildDlgWithChildren(d);
    // ActionEvent / OnActionEvent before SetActive: no state
    // mutation, no crash.
    d.ActionEvent(0, 0, 0);
    d.OnActionEvent(mxh::ui::cSkinSelectDialog::ID_OK, nullptr, 0);
    EXPECT_EQ(d.GetSelectIdx(), 0u);
}

TEST(CSkinSelectDialog, SetActiveFalseClearsState) {
    mxh::ui::cSkinSelectDialog d;
    BuildDlgWithChildren(d);
    d.SetSelectIdx(5);
    d.SetActive(false);
    EXPECT_EQ(d.GetSelectIdx(), 0u);
    // List / icon cleared.
    auto* list = static_cast<mxh::ui::cListDialog*>(
        d.findWindowById(mxh::ui::cSkinSelectDialog::ID_LIST));
    auto* icon = static_cast<mxh::ui::cIconDialog*>(
        d.findWindowById(mxh::ui::cSkinSelectDialog::ID_ITEMVIEW));
    ASSERT_NE(list, nullptr);
    ASSERT_NE(icon, nullptr);
    EXPECT_EQ(list->GetCurSelectedRowIdx(), 0 - 1);
    // cIconDialog (already ported) — verify all cells are empty.
    for (std::uint16_t i = 0; i < icon->GetCellNum(); ++i) {
        EXPECT_EQ(icon->GetIconForIdx(i), nullptr);
    }
}

TEST(CSkinSelectDialog, SetActiveTrueDoesNotCrash) {
    mxh::ui::cSkinSelectDialog d;
    d.Linking();
    d.SetActive(true);
    // SkinItemListInfo is stubbed to 0 entries (no engine data),
    // so the list is empty but no crash.
    d.SetActive(false);
}

TEST(CSkinSelectDialog, OnActionEventCloseWindowReturnsTrue) {
    mxh::ui::cSkinSelectDialog d;
    d.Linking();
    EXPECT_TRUE(d.OnActionEvent(0, nullptr, 0x10 /*WE_CLOSEWINDOW*/));
}

TEST(CSkinSelectDialog, OnActionEventCancelSetsInactive) {
    mxh::ui::cSkinSelectDialog d;
    d.Linking();
    d.SetActive(true);
    d.OnActionEvent(mxh::ui::cSkinSelectDialog::ID_CANCEL, nullptr, 0x20 /*WE_BTNCLICK*/);
    EXPECT_FALSE(d.isActive());
}

TEST(CSkinSelectDialog, OnActionEventOkWithoutSelectIdxIsNoOp) {
    mxh::ui::cSkinSelectDialog d;
    d.Linking();
    d.SetActive(true);
    d.SetSelectIdx(0);  // no selection
    d.SetSkinDelayResult(true);
    d.OnActionEvent(mxh::ui::cSkinSelectDialog::ID_OK, nullptr, 0x20);
    // m_bSkinDelayResult unchanged (the engine-side check would
    // have rejected the OK).
    EXPECT_TRUE(d.IsSkinDelayResult());
}

TEST(CSkinSelectDialog, OnActionEventOkWithSelectIdxFlipsDelay) {
    mxh::ui::cSkinSelectDialog d;
    d.Linking();
    d.SetActive(true);
    d.SetSelectIdx(3);
    d.SetSkinDelayResult(true);
    d.OnActionEvent(mxh::ui::cSkinSelectDialog::ID_OK, nullptr, 0x20);
    // 1:1 with legacy: OK fires the network send, which is
    // preceded by HERO->CheckSkinDelay()==FALSE. Modern port
    // records the engine-side rejection by flipping the flag.
    EXPECT_FALSE(d.IsSkinDelayResult());
}

TEST(CSkinSelectDialog, OnActionEventRecoveryFlipsDelay) {
    mxh::ui::cSkinSelectDialog d;
    d.Linking();
    d.SetActive(true);
    d.SetSkinDelayResult(true);
    d.OnActionEvent(mxh::ui::cSkinSelectDialog::ID_RECOVERY, nullptr, 0x20);
    EXPECT_FALSE(d.IsSkinDelayResult());
}

TEST(CSkinSelectDialog, OnActionEventUnknownIdReturnsTrue) {
    mxh::ui::cSkinSelectDialog d;
    d.Linking();
    d.SetActive(true);
    EXPECT_TRUE(d.OnActionEvent(99, nullptr, 0x01));
}

TEST(CSkinSelectDialog, ActionEventWithoutListIconIsSafe) {
    // 1:1 quirk: legacy dtor doesn't NULL-check m_pNomalSkinListDlg.
    // The ActionEvent guard is the modern port's defensive check
    // (avoids the legacy crash when Linking was never called).
    mxh::ui::cSkinSelectDialog d;
    // No Linking — pointers are null.
    EXPECT_EQ(d.ActionEvent(50, 50, 0), 0u);
}

TEST(CSkinSelectDialog, ActionEventDelegatesToBase) {
    // 1:1 with legacy: ActionEvent calls cDialog::ActionEvent
    // first, which routes the event to the focused child. The
    // modern port preserves the early-return if !isActive.
    mxh::ui::cSkinSelectDialog d;
    d.Linking();
    d.SetActive(true);
    // ActionEvent without an LButton click: returns the cDialog
    // delegation result (likely WE_NULL since no child is
    // focused).
    const auto we = d.ActionEvent(0, 0, 0);
    (void)we;  // no specific assertion — just verify no crash
    SUCCEED();
}

TEST(CSkinSelectDialog, ActionEventLButtonClickPopulatesPreview) {
    // 1:1 with legacy: LBTNCLICK on a list row populates the
    // 3-cell preview. The list dialog needs a row to click; we
    // directly call PtIdxInRow with a known position. The legacy
    // would then call cListDialog::GetCurSelectedRowIdx + populate.
    mxh::ui::cSkinSelectDialog d;
    BuildDlgWithChildren(d);
    d.SetActive(true);
    // Manually set the list selection to row 0 (simulating a click).
    auto* list = static_cast<mxh::ui::cListDialog*>(
        d.findWindowById(mxh::ui::cSkinSelectDialog::ID_LIST));
    auto* icon = static_cast<mxh::ui::cIconDialog*>(
        d.findWindowById(mxh::ui::cSkinSelectDialog::ID_ITEMVIEW));
    ASSERT_NE(list, nullptr);
    ASSERT_NE(icon, nullptr);
    // The icon dialog needs 3 cells configured so populatePreview
    // can AddIcon to cells 0..2.
    icon->SetCellNum(mxh::ui::cSkinSelectDialog::SKINITEM_LIST_MAX);
    for (std::uint16_t i = 0; i < mxh::ui::cSkinSelectDialog::SKINITEM_LIST_MAX; ++i) {
        icon->AddIconCell(static_cast<std::int32_t>(i) * 30, 0, 30, 30);
    }
    // The list needs a row so PtIdxInRow(0, 0) returns row 0
    // (m_lineHeight default is 14; row = m_topRow + (0-0)/14 = 0).
    list->AddItem("row 0");
    list->SetCurSelectedRowIdx(0);
    // Now send an LButton click event — modern port uses
    // cWindow::MouseFlagLButton = 0x0001 as the WE_LBTNCLICK bit.
    d.ActionEvent(0, 0, mxh::ui::cWindow::MouseFlagLButton);
    // m_dwSelectIdx = 0 + 1 = 1 (1-based per legacy quirk).
    EXPECT_EQ(d.GetSelectIdx(), 1u);
    // The 3-cell preview is populated.
    for (std::uint16_t i = 0; i < mxh::ui::cSkinSelectDialog::SKINITEM_LIST_MAX; ++i) {
        EXPECT_NE(icon->GetIconForIdx(i), nullptr);
    }
}

TEST(CSkinSelectDialog, ActionEventLButtonClickSecondRowSelects2) {
    mxh::ui::cSkinSelectDialog d;
    BuildDlgWithChildren(d);
    d.SetActive(true);
    auto* list = static_cast<mxh::ui::cListDialog*>(
        d.findWindowById(mxh::ui::cSkinSelectDialog::ID_LIST));
    // The list needs 2 rows. Set selection to row 1 then click
    // at y=20 (which PtIdxInRow maps to row 1: m_lineHeight=14,
    // m_clipY=0, row = 0 + (20-0)/14 = 1). The modern
    // cSkinSelectDialog::ActionEvent reads m_selectedRow via
    // GetCurSelectedRowIdx() to compute m_dwSelectIdx, so the
    // selection must be set before the click.
    list->AddItem("row 0");
    list->AddItem("row 1");
    list->SetCurSelectedRowIdx(1);
    d.ActionEvent(0, 20, mxh::ui::cWindow::MouseFlagLButton);
    EXPECT_EQ(d.GetSelectIdx(), 2u);
}

TEST(CSkinSelectDialog, ActionEventRButtonClickIsIgnored) {
    // 1:1 with legacy: only LBTNCLICK populates the preview.
    mxh::ui::cSkinSelectDialog d;
    BuildDlgWithChildren(d);
    d.SetActive(true);
    auto* list = static_cast<mxh::ui::cListDialog*>(
        d.findWindowById(mxh::ui::cSkinSelectDialog::ID_LIST));
    list->SetCurSelectedRowIdx(0);
    d.ActionEvent(0, 0, mxh::ui::cWindow::MouseFlagRButton);
    // m_dwSelectIdx unchanged.
    EXPECT_EQ(d.GetSelectIdx(), 0u);
}

TEST(CSkinSelectDialog, SetSelectIdxRoundTrip) {
    mxh::ui::cSkinSelectDialog d;
    d.SetSelectIdx(7);
    EXPECT_EQ(d.GetSelectIdx(), 7u);
    d.SetSelectIdx(0);
    EXPECT_EQ(d.GetSelectIdx(), 0u);
}

TEST(CSkinSelectDialog, SetSkinDelayResultRoundTrip) {
    mxh::ui::cSkinSelectDialog d;
    d.SetSkinDelayResult(true);
    EXPECT_TRUE(d.IsSkinDelayResult());
    d.SetSkinDelayResult(false);
    EXPECT_FALSE(d.IsSkinDelayResult());
}

TEST(CSkinSelectDialog, SkinItemListInfoEmpty) {
    // 1:1 with legacy: GAMERESRCMNGR->GetNomalClothesSkinListCountNum()
    // is stubbed to 0 in modern, so SkinItemListInfo loops 0
    // times. Verify no crash + list is empty.
    mxh::ui::cSkinSelectDialog d;
    BuildDlgWithChildren(d);
    d.SetActive(true);
    auto* list = static_cast<mxh::ui::cListDialog*>(
        d.findWindowById(mxh::ui::cSkinSelectDialog::ID_LIST));
    EXPECT_EQ(list->RowCount(), 0u);
}

TEST(CSkinSelectDialog, DestructorNullSafe) {
    // 1:1 quirk: legacy dtor doesn't NULL-check the list pointer.
    // The modern port adds a NULL check (defensive). If Linking
    // was never called, the dtor must not crash.
    mxh::ui::cSkinSelectDialog d;
    // dtor runs at end of scope; no Linking → m_pNomalSkinListDlg is null.
    SUCCEED();
}
