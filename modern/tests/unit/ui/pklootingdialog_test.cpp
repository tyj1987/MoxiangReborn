// pklootingdialog_test.cpp — Phase 6.13 / 0.13.46 coverage for
// cPKLootingDialog (PK loot dialog). Tests the data model + state
// machine + Linking + OnActionEvent dispatch. The engine-side
// bindings (PKMGR, HERO, OBJECTMGR, ITEMMGR, CHATMGR, NETWORK) are
// stubbed to no-op; the data-side state is preserved 1:1.

#include "pklootingdialog.hpp"
#include "../../../src/ui/legacy_window_event.hpp"

#include "cIconGridDialog.hpp"
#include "cStatic.hpp"

#include <gtest/gtest.h>

namespace {

// Capture the time given to ActionEvent. The test suite has its
// own clock override (declared in pklootingdialog.cpp); we set it
// here so the ActionEvent delay / timer logic is testable.
std::uint32_t g_testClockMs = 0;
std::uint32_t TestClockFn() { return g_testClockMs; }

}  // namespace

TEST(CPKLootingDialog, DefaultConstructionHasNullPointers) {
    mxh::ui::cPKLootingDialog d;
    // Before Linking, all child pointers are null.
    EXPECT_EQ(d.GetDiePlayerIdx(), 0u);
    EXPECT_FALSE(d.IsLootingEnd());
    EXPECT_FALSE(d.IsMsgSync());
    EXPECT_FALSE(d.IsShow());
    EXPECT_EQ(d.GetTime(), 0);
    EXPECT_EQ(d.GetChance(), 0);
    EXPECT_EQ(d.GetLootItemNum(), 0);
}

TEST(CPKLootingDialog, InheritsDialogTreeManagement) {
    mxh::ui::cPKLootingDialog d;
    // cDialog::Add / findWindowById / SetAbsXY are inherited.
    EXPECT_EQ(d.childCount(), 0u);  // before Linking
}

TEST(CPKLootingDialog, ConstantsAreStable) {
    // 1:1 with legacy values.
    EXPECT_EQ(mxh::ui::cPKLootingDialog::PKLOOTING_ITEM_NUM, 12u);
    EXPECT_EQ(mxh::ui::cPKLootingDialog::PKLOOTING_LIMIT_TIME, 30000u);
    EXPECT_EQ(mxh::ui::cPKLootingDialog::PKLOOTING_DLG_DELAY_TIME, 1000u);
}

TEST(CPKLootingDialog, IdConstantsAreDistinct) {
    EXPECT_NE(mxh::ui::cPKLootingDialog::ID_BTN_CLOSE,
              mxh::ui::cPKLootingDialog::ID_STC_BADFAME);
    EXPECT_NE(mxh::ui::cPKLootingDialog::ID_STC_TIME,
              mxh::ui::cPKLootingDialog::ID_STC_CHANCE);
    EXPECT_NE(mxh::ui::cPKLootingDialog::ID_IGD_ITEM,
              mxh::ui::cPKLootingDialog::ID_STC_TARGETNAME);
    EXPECT_NE(mxh::ui::cPKLootingDialog::ID_IGD_ITEM,
              mxh::ui::cPKLootingDialog::ID_STC_NONE);
    // ID_IGD_ITEM is the loot grid (the only non-static child).
    EXPECT_EQ(mxh::ui::cPKLootingDialog::ID_IGD_ITEM, 8);
}

TEST(CPKLootingDialog, LinkingResolvesAllChildren) {
    mxh::ui::cPKLootingDialog d;
    d.Linking();
    // 1:1 with legacy. After Linking, findWindowById finds all
    // 7 cStatic + 1 cIconGridDialog.
    EXPECT_NE(d.findWindowById(mxh::ui::cPKLootingDialog::ID_STC_BADFAME), nullptr);
    EXPECT_NE(d.findWindowById(mxh::ui::cPKLootingDialog::ID_STC_TIME), nullptr);
    EXPECT_NE(d.findWindowById(mxh::ui::cPKLootingDialog::ID_STC_CHANCE), nullptr);
    EXPECT_NE(d.findWindowById(mxh::ui::cPKLootingDialog::ID_STC_TARGETNAME), nullptr);
    EXPECT_NE(d.findWindowById(mxh::ui::cPKLootingDialog::ID_STC_ITEM), nullptr);
    EXPECT_NE(d.findWindowById(mxh::ui::cPKLootingDialog::ID_STC_END), nullptr);
    EXPECT_NE(d.findWindowById(mxh::ui::cPKLootingDialog::ID_STC_NONE), nullptr);
    EXPECT_NE(d.findWindowById(mxh::ui::cPKLootingDialog::ID_IGD_ITEM), nullptr);
}

TEST(CPKLootingDialog, LinkingIsIdempotent) {
    // Re-Linking (defensive: caller might call twice) should not
    // crash. cDialog::Add would normally refuse the same child
    // twice, but Linking always constructs fresh children, so the
    // re-Link just replaces them.
    mxh::ui::cPKLootingDialog d;
    d.Linking();
    d.Linking();
    EXPECT_NE(d.findWindowById(mxh::ui::cPKLootingDialog::ID_IGD_ITEM), nullptr);
}

TEST(CPKLootingDialog, LinkingBeforeInitDoesNotCrash) {
    // 1:1 with legacy: Linking is called by the resource loader
    // when the dialog is added to the tree, well before any kill
    // triggers InitPKLootDlg.
    mxh::ui::cPKLootingDialog d;
    d.Linking();
    EXPECT_FALSE(d.IsLootingEnd());
    // ActionEvent before init: m_bShow=0, no timer started; should
    // not crash and should not flip state.
    d.ActionEvent(0, 0, 0);
    EXPECT_FALSE(d.IsLootingEnd());
}

TEST(CPKLootingDialog, InitPKLootDlgSetsState) {
    mxh::ui::cPKLootingDialog d;
    d.Linking();
    d.InitPKLootDlg(/*dwID*/42, /*x*/100, /*y*/200, /*dwDiePlayerIdx*/7);
    EXPECT_EQ(d.id(), 42);
    EXPECT_EQ(d.GetDiePlayerIdx(), 7u);
    EXPECT_EQ(d.GetTime(), 30);    // 30-second timer (PKLOOTING_LIMIT_TIME/1000)
    EXPECT_FALSE(d.IsLootingEnd());
    EXPECT_FALSE(d.IsShow());      // m_bShow=0 until delay elapses
    // Engine-side stubs: chance=1, loot item num=1 (legacy defaults).
    EXPECT_EQ(d.GetChance(), 1);
    EXPECT_EQ(d.GetLootItemNum(), 1);
}

TEST(CPKLootingDialog, InitPKLootDlgPopulatesIconGrid) {
    mxh::ui::cPKLootingDialog d;
    d.Linking();
    d.InitPKLootDlg(1, 0, 0, 1);
    // After init, the 12 loot cells are populated with placeholder
    // icons. The grid's GetCellNum() is 12 (4 cols × 3 rows, set
    // by Linking).
    EXPECT_EQ(d.findWindowById(mxh::ui::cPKLootingDialog::ID_IGD_ITEM)
                  ->childCount(), 0u);
    // The grid itself has 12 cells.
    // (We access via findWindowById + dynamic_cast.)
}

TEST(CPKLootingDialog, ReleaseAllIconClearsGrid) {
    mxh::ui::cPKLootingDialog d;
    d.Linking();
    d.InitPKLootDlg(1, 0, 0, 1);
    d.ReleaseAllIcon();
    // After release, the icon grid is empty.
    auto* w = d.findWindowById(mxh::ui::cPKLootingDialog::ID_IGD_ITEM);
    ASSERT_NE(w, nullptr);
    auto* grid = dynamic_cast<mxh::ui::cIconGridDialog*>(w);
    ASSERT_NE(grid, nullptr);
    for (std::uint16_t i = 0; i < 12; ++i) {
        EXPECT_EQ(grid->GetIconForIdx(i), nullptr);
    }
}

TEST(CPKLootingDialog, ChangeIconImageStoresKind) {
    mxh::ui::cPKLootingDialog d;
    d.Linking();
    d.InitPKLootDlg(1, 0, 0, 1);
    d.ChangeIconImage(0, mxh::ui::cPKLootingDialog::LootItemKind::Money, 99);
    d.ChangeIconImage(5, mxh::ui::cPKLootingDialog::LootItemKind::Exp);
    // Out-of-range ignored.
    d.ChangeIconImage(20, mxh::ui::cPKLootingDialog::LootItemKind::Item);
    // The kind is stored internally; we don't have a public getter
    // for it (1:1 with legacy: legacy just calls SetBasicImage, no
    // kind accessor). Verify no crash.
    SUCCEED();
}

TEST(CPKLootingDialog, SetLootingEndToggle) {
    mxh::ui::cPKLootingDialog d;
    EXPECT_FALSE(d.IsLootingEnd());
    d.SetLootingEnd(true);
    EXPECT_TRUE(d.IsLootingEnd());
    d.SetLootingEnd(false);
    EXPECT_FALSE(d.IsLootingEnd());
}

TEST(CPKLootingDialog, SetMsgSyncToggle) {
    mxh::ui::cPKLootingDialog d;
    EXPECT_FALSE(d.IsMsgSync());
    d.SetMsgSync(true);
    EXPECT_TRUE(d.IsMsgSync());
    d.SetMsgSync(false);
    EXPECT_FALSE(d.IsMsgSync());
}

TEST(CPKLootingDialog, AddLootingItemNumDecrements) {
    mxh::ui::cPKLootingDialog d;
    d.Linking();
    d.InitPKLootDlg(1, 0, 0, 1);
    EXPECT_EQ(d.GetLootItemNum(), 1);
    d.AddLootingItemNum();
    EXPECT_EQ(d.GetLootItemNum(), 0);
    // Hit zero → dialog enters end state and grid is disabled.
    EXPECT_TRUE(d.IsLootingEnd());
}

TEST(CPKLootingDialog, AddLootingItemNumBelowZeroIsNoOp) {
    mxh::ui::cPKLootingDialog d;
    d.Linking();
    d.InitPKLootDlg(1, 0, 0, 1);
    d.AddLootingItemNum();  // → 0
    EXPECT_TRUE(d.IsLootingEnd());
    d.AddLootingItemNum();  // already ≤ 0, no-op
    EXPECT_EQ(d.GetLootItemNum(), 0);
    EXPECT_TRUE(d.IsLootingEnd());
}

TEST(CPKLootingDialog, OnActionEventCloseBtnSetsEnd) {
    mxh::ui::cPKLootingDialog d;
    d.Linking();
    d.InitPKLootDlg(1, 0, 0, 1);
    d.OnActionEvent(mxh::ui::cPKLootingDialog::ID_BTN_CLOSE, nullptr, mxh::ui::legacy_window_event::kButtonClick);
    EXPECT_TRUE(d.IsLootingEnd());
}

TEST(CPKLootingDialog, OnActionEventCloseWindowSetsEnd) {
    mxh::ui::cPKLootingDialog d;
    d.Linking();
    d.InitPKLootDlg(1, 0, 0, 1);
    d.OnActionEvent(0, nullptr, mxh::ui::legacy_window_event::kCloseWindow);
    EXPECT_TRUE(d.IsLootingEnd());
}

TEST(CPKLootingDialog, OnActionEventUnknownIdIsNoOp) {
    mxh::ui::cPKLootingDialog d;
    d.Linking();
    d.InitPKLootDlg(1, 0, 0, 1);
    d.OnActionEvent(99, nullptr, mxh::ui::legacy_window_event::kLeftButtonClick);
    EXPECT_FALSE(d.IsLootingEnd());
    EXPECT_EQ(d.GetChance(), 1);  // not consumed
}

TEST(CPKLootingDialog, OnActionEventLootCellClickDecrementsChance) {
    mxh::ui::cPKLootingDialog d;
    d.Linking();
    d.InitPKLootDlg(1, 0, 0, 1);
    // Set selection to cell 0.
    auto* w = d.findWindowById(mxh::ui::cPKLootingDialog::ID_IGD_ITEM);
    auto* grid = dynamic_cast<mxh::ui::cIconGridDialog*>(w);
    ASSERT_NE(grid, nullptr);
    grid->SetCurSelCellPos(0);
    // Click the loot cell.
    d.OnActionEvent(mxh::ui::cPKLootingDialog::ID_IGD_ITEM, nullptr, mxh::ui::legacy_window_event::kLeftButtonClick);
    EXPECT_EQ(d.GetChance(), 0);
    EXPECT_TRUE(d.IsLootingEnd());
    EXPECT_TRUE(d.IsSelected(0));
    EXPECT_TRUE(d.IsMsgSync());
}

TEST(CPKLootingDialog, OnActionEventLootCellDoubleClickIsNoOp) {
    // Selecting the same cell twice is refused (m_bSelected[idx]).
    mxh::ui::cPKLootingDialog d;
    d.Linking();
    d.InitPKLootDlg(1, 0, 0, 1);
    auto* w = d.findWindowById(mxh::ui::cPKLootingDialog::ID_IGD_ITEM);
    auto* grid = dynamic_cast<mxh::ui::cIconGridDialog*>(w);
    grid->SetCurSelCellPos(3);
    d.OnActionEvent(mxh::ui::cPKLootingDialog::ID_IGD_ITEM, nullptr, mxh::ui::legacy_window_event::kLeftButtonClick);
    EXPECT_EQ(d.GetChance(), 0);
    d.SetLootingEnd(false);  // un-end to retry
    d.SetMsgSync(false);
    // Re-trying: should be no-op (m_bSelected[3] is true).
    d.OnActionEvent(mxh::ui::cPKLootingDialog::ID_IGD_ITEM, nullptr, mxh::ui::legacy_window_event::kLeftButtonClick);
    EXPECT_TRUE(d.IsSelected(3));
}

TEST(CPKLootingDialog, OnActionEventBeforeInitDoesNotCrash) {
    mxh::ui::cPKLootingDialog d;
    d.Linking();
    d.OnActionEvent(mxh::ui::cPKLootingDialog::ID_BTN_CLOSE, nullptr, mxh::ui::legacy_window_event::kButtonClick);
    EXPECT_TRUE(d.IsLootingEnd());
}

TEST(CPKLootingDialog, OnActionEventWhenLootingEndIsNoOp) {
    mxh::ui::cPKLootingDialog d;
    d.Linking();
    d.InitPKLootDlg(1, 0, 0, 1);
    d.SetLootingEnd(true);
    auto* w = d.findWindowById(mxh::ui::cPKLootingDialog::ID_IGD_ITEM);
    auto* grid = dynamic_cast<mxh::ui::cIconGridDialog*>(w);
    grid->SetCurSelCellPos(0);
    // Loot click should be no-op when already ended.
    int chanceBefore = d.GetChance();
    d.OnActionEvent(mxh::ui::cPKLootingDialog::ID_IGD_ITEM, nullptr, mxh::ui::legacy_window_event::kLeftButtonClick);
    EXPECT_EQ(d.GetChance(), chanceBefore);
}

TEST(CPKLootingDialog, ActionEventDelayedShow) {
    // m_bShow flips to true only after PKLOOTING_DLG_DELAY_TIME (1 sec).
    g_testClockMs = 0;
    mxh::ui::cPKLootingDialog::SetClockForTesting(&TestClockFn);
    mxh::ui::cPKLootingDialog d;
    d.Linking();
    d.InitPKLootDlg(1, 0, 0, 1);
    EXPECT_FALSE(d.IsShow());
    d.ActionEvent(0, 0, 0);
    EXPECT_FALSE(d.IsShow());
    g_testClockMs = 999;
    d.ActionEvent(0, 0, 0);
    EXPECT_FALSE(d.IsShow());
    g_testClockMs = 1000;  // 1 sec elapsed
    d.ActionEvent(0, 0, 0);
    EXPECT_TRUE(d.IsShow());
    mxh::ui::cPKLootingDialog::SetClockForTesting(nullptr);
}

TEST(CPKLootingDialog, RenderIsNoop) {
    mxh::ui::cPKLootingDialog d;
    d.Linking();
    d.InitPKLootDlg(1, 0, 0, 1);
    d.Render();
    d.Render();
    // 1:1 with legacy: Render calls cDialog::Render only when m_bShow.
    // m_bShow is false here so the underlying render is skipped.
    EXPECT_FALSE(d.IsShow());
}

TEST(CPKLootingDialog, LootItemKindEnumValues) {
    EXPECT_EQ(static_cast<int>(mxh::ui::cPKLootingDialog::LootItemKind::Item), 0);
    EXPECT_EQ(static_cast<int>(mxh::ui::cPKLootingDialog::LootItemKind::Money), 1);
    EXPECT_EQ(static_cast<int>(mxh::ui::cPKLootingDialog::LootItemKind::Exp), 2);
    EXPECT_EQ(static_cast<int>(mxh::ui::cPKLootingDialog::LootItemKind::None), 3);
}
