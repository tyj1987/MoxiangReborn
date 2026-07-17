// clistdialog_test.cpp — Phase 6.12 coverage for cListDialog (text list).

#include "cListDialog.hpp"

#include <gtest/gtest.h>

TEST(CListDialog, DefaultState) {
    mxh::ui::cListDialog l;
    EXPECT_EQ(l.RowCount(), 0u);
    EXPECT_EQ(l.GetCurSelectedRowIdx(), -1);
    EXPECT_EQ(l.GetTopListItemIdx(), 0);
    EXPECT_FALSE(l.IsMaxLineOver());
    EXPECT_EQ(l.GetMaxLine(), 0u);
}

TEST(CListDialog, InitListStoresConfig) {
    mxh::ui::cListDialog l;
    l.InitList(20, 5, 10, 100, 200);
    EXPECT_EQ(l.GetMaxLine(), 20u);
    EXPECT_FALSE(l.IsMaxLineOver());
}

TEST(CListDialog, AddItemAppendsToEnd) {
    mxh::ui::cListDialog l;
    l.InitList(5, 0, 0, 100, 100);
    l.AddItem("a");
    l.AddItem("b");
    l.AddItem("c");
    EXPECT_EQ(l.RowCount(), 3u);
}

TEST(CListDialog, AddItemAtSpecificLine) {
    mxh::ui::cListDialog l;
    l.InitList(5, 0, 0, 100, 100);
    l.AddItem("a");
    l.AddItem("c");
    l.AddItem("b", 0, 1);  // insert at line 1
    EXPECT_EQ(l.RowCount(), 3u);
}

TEST(CListDialog, IsMaxLineOverDetectsOverflow) {
    mxh::ui::cListDialog l;
    l.InitList(2, 0, 0, 100, 100);
    EXPECT_FALSE(l.IsMaxLineOver());
    l.AddItem("a");
    l.AddItem("b");
    EXPECT_FALSE(l.IsMaxLineOver());
    l.AddItem("c");
    EXPECT_TRUE(l.IsMaxLineOver());
}

TEST(CListDialog, RemoveAllClearsRowsAndSelection) {
    mxh::ui::cListDialog l;
    l.InitList(5, 0, 0, 100, 100);
    l.AddItem("a"); l.AddItem("b");
    l.SetCurSelectedRowIdx(1);
    l.RemoveAll();
    EXPECT_EQ(l.RowCount(), 0u);
    EXPECT_EQ(l.GetCurSelectedRowIdx(), -1);
    EXPECT_EQ(l.GetTopListItemIdx(), 0);
}

TEST(CListDialog, SelectionSetterGetter) {
    mxh::ui::cListDialog l;
    l.InitList(5, 0, 0, 100, 100);
    l.AddItem("a");
    l.SetCurSelectedRowIdx(0);
    EXPECT_EQ(l.GetCurSelectedRowIdx(), 0);
    EXPECT_EQ(l.GetSelectRowIdx(), 0);
    l.SetCurSelectedRowIdx(-1);
    EXPECT_EQ(l.GetCurSelectedRowIdx(), -1);
}

TEST(CListDialog, TopListItemIdxClamps) {
    mxh::ui::cListDialog l;
    l.InitList(5, 0, 0, 100, 100);
    l.AddItem("a"); l.AddItem("b");
    l.SetTopListItemIdx(99);  // out of range
    EXPECT_EQ(l.GetTopListItemIdx(), 1);
    l.SetTopListItemIdx(-5);  // out of range
    EXPECT_EQ(l.GetTopListItemIdx(), 0);
}

TEST(CListDialog, OnUpwardItemDecAndScrollUp) {
    mxh::ui::cListDialog l;
    l.InitList(5, 0, 0, 100, 100);
    l.AddItem("a"); l.AddItem("b"); l.AddItem("c");
    l.SetCurSelectedRowIdx(2);
    l.SetTopListItemIdx(1);
    l.OnUpwardItem();
    EXPECT_EQ(l.GetCurSelectedRowIdx(), 1);
    EXPECT_EQ(l.GetTopListItemIdx(), 1);
    l.OnUpwardItem();
    EXPECT_EQ(l.GetCurSelectedRowIdx(), 0);
    EXPECT_EQ(l.GetTopListItemIdx(), 0);
    l.OnUpwardItem();  // already at 0
    EXPECT_EQ(l.GetCurSelectedRowIdx(), 0);
}

TEST(CListDialog, OnDownwardItemIncAndScrollDown) {
    mxh::ui::cListDialog l;
    l.InitList(5, 0, 0, 100, 100);
    l.AddItem("a"); l.AddItem("b"); l.AddItem("c");
    l.SetCurSelectedRowIdx(0);
    l.SetTopListItemIdx(0);
    l.SetLineHeight(10);
    l.OnDownwardItem();
    EXPECT_EQ(l.GetCurSelectedRowIdx(), 1);
    l.OnDownwardItem();
    EXPECT_EQ(l.GetCurSelectedRowIdx(), 2);
    l.OnDownwardItem();  // already at end
    EXPECT_EQ(l.GetCurSelectedRowIdx(), 2);
}

TEST(CListDialog, PtIdxInRowRespectsClip) {
    mxh::ui::cListDialog l;
    l.InitList(5, 10, 20, 100, 100);
    l.AddItem("a"); l.AddItem("b"); l.AddItem("c");
    l.SetLineHeight(20);
    // Inside the clip rect: row 0 at y=20, row 1 at y=40, row 2 at y=60.
    EXPECT_EQ(l.PtIdxInRow(50, 20),  0);
    EXPECT_EQ(l.PtIdxInRow(50, 40),  1);
    EXPECT_EQ(l.PtIdxInRow(50, 60),  2);
    // Outside the clip rect.
    EXPECT_EQ(l.PtIdxInRow(50, 0),  -1);
    EXPECT_EQ(l.PtIdxInRow(50, 200), -1);
    EXPECT_EQ(l.PtIdxInRow(0,  40), -1);
}

TEST(CListDialog, AutoScrollSetterGetter) {
    // 1:1 with legacy cListDialog::SetAutoScroll + SetShowSelect.
    // Both setters existed before this fix but the AutoScroll getter
    // did not (mirrors the cButton 87e831a / cStatic 7cf011e pattern).
    // Setter/getter pairs must always be present in modern UI; the
    // test upgrades from a weak "no getter" form (just verifies the
    // call doesn't crash) to a 4-assertion round-trip.
    mxh::ui::cListDialog l;
    l.InitList(5, 0, 0, 100, 100);
    // Defaults: auto-scroll off, show-select on.
    EXPECT_FALSE(l.IsAutoScroll());
    EXPECT_TRUE(l.IsShowSelect());
    l.SetAutoScroll(true);
    l.SetShowSelect(false);
    EXPECT_TRUE(l.IsAutoScroll());
    EXPECT_FALSE(l.IsShowSelect());
    // Toggle back: setter is a re-write, not an add.
    l.SetAutoScroll(false);
    l.SetShowSelect(true);
    EXPECT_FALSE(l.IsAutoScroll());
    EXPECT_TRUE(l.IsShowSelect());
}

TEST(CListDialog, LineHeightSetterGetter) {
    // 1:1 with legacy cListDialog::SetLineHeight: the line height is
    // the row pitch used by PtIdxInRow and by OnUpwardItem /
    // OnDownwardItem scroll math. Setter existed before this fix but
    // the getter did not (same pattern as cButton 87e831a and
    // cStatic 7cf011e).
    mxh::ui::cListDialog l;
    l.InitList(5, 0, 0, 100, 100);
    // Default line height is 14 (legacy cListDialog default).
    EXPECT_EQ(l.GetLineHeight(), 14);
    l.SetLineHeight(20);
    EXPECT_EQ(l.GetLineHeight(), 20);
    // Re-Init must not clobber the line height.
    l.InitList(5, 0, 0, 100, 100);
    EXPECT_EQ(l.GetLineHeight(), 20);
    // Negative / zero line heights must round-trip verbatim (the
    // legacy engine stores the value as-is; the hit-test math
    // produces 0 / negative row indices, which PtIdxInRow treats
    // as "outside the clip" and returns -1).
    l.SetLineHeight(0);
    EXPECT_EQ(l.GetLineHeight(), 0);
    l.SetLineHeight(-1);
    EXPECT_EQ(l.GetLineHeight(), -1);
}
