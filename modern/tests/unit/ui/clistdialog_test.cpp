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
    mxh::ui::cListDialog l;
    l.SetAutoScroll(true);
    l.SetShowSelect(false);
    EXPECT_FALSE(l.IsShowSelect());
}
