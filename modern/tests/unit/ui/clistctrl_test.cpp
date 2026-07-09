// tests/unit/ui/clistctrl_test.cpp
// Phase 6.5 unit tests for the modern mxh::ui::cListCtrl widget.
#include <gtest/gtest.h>

#include <vector>

#include "cListCtrl.hpp"
#include "cWindow.hpp"

using mxh::ui::cListCtrl;
using mxh::ui::cWindow;

namespace {
int g_basicImg = 1;
int g_headImg  = 2;
int g_bodyImg  = 3;
int g_overImg  = 4;
} // namespace

TEST(CListCtrl, DefaultState) {
    cListCtrl l;
    EXPECT_EQ(l.columnCount(), 0u);
    EXPECT_EQ(l.rowCount(), 0u);
    EXPECT_EQ(l.selectedRowIdx(), -1);
    EXPECT_EQ(l.overRowIdx(), -1);
    EXPECT_EQ(l.topItemIdx(), 0);
    EXPECT_EQ(l.linePerPage(), 0);
    EXPECT_EQ(l.headLineHeight(), 0u);
    EXPECT_EQ(l.bodyLineHeight(), 0u);
}

TEST(CListCtrl, InitConfiguresLayout) {
    cListCtrl l;
    l.Init(10, 20, 200, 300, &g_basicImg, 42);
    EXPECT_EQ(l.id(), 42);
    EXPECT_EQ(l.absX(), 10);
    EXPECT_EQ(l.absY(), 20);
    EXPECT_EQ(l.width(), 200u);
    EXPECT_EQ(l.height(), 300u);
    EXPECT_EQ(l.basicImage(), &g_basicImg);
}

TEST(CListCtrl, InitListCtrlAndImages) {
    cListCtrl l;
    l.Init(0, 0, 200, 300, &g_basicImg);
    l.InitListCtrl(3, 10);
    EXPECT_EQ(l.linePerPage(), 10);
    l.InitListCtrlImage(&g_headImg, 25, &g_bodyImg, 20, &g_overImg);
    EXPECT_EQ(l.headImage(),      &g_headImg);
    EXPECT_EQ(l.bodyImage(),      &g_bodyImg);
    EXPECT_EQ(l.overImage(),      &g_overImg);
    EXPECT_EQ(l.headLineHeight(), 25u);
    EXPECT_EQ(l.bodyLineHeight(), 20u);
}

TEST(CListCtrl, ColumnsAndRows) {
    cListCtrl l;
    l.Init(0, 0, 200, 300, &g_basicImg);
    l.InitListCtrl(3, 10);
    l.SetColumns({
        {80, "Name", 0xFF000000},
        {60, "Level", 0xFF000000},
        {60, "Class", 0xFF000000},
    });
    EXPECT_EQ(l.columnCount(), 3u);
    l.AddRow({{"Alice", "10", "Warrior"}, {0xFF000000, 0xFF000000, 0xFF000000}});
    l.AddRow({{"Bob",   "20", "Mage"},    {0xFF000000, 0xFF000000, 0xFF000000}});
    EXPECT_EQ(l.rowCount(), 2u);
    EXPECT_EQ(l.rowAt(0).texts[0], "Alice");
    EXPECT_EQ(l.rowAt(1).texts[1], "20");
}

TEST(CListCtrl, RemoveAllClearsState) {
    cListCtrl l;
    l.Init(0, 0, 200, 300, &g_basicImg);
    l.InitListCtrl(2, 5);
    l.SetColumns({{100, "A"}, {100, "B"}});
    l.AddRow({{"x", "y"}});
    l.AddRow({{"p", "q"}});
    l.SetSelectedRowIdx(1);
    l.SetOverRowIdx(0);
    EXPECT_EQ(l.rowCount(), 2u);
    l.RemoveAll();
    EXPECT_EQ(l.rowCount(), 0u);
    EXPECT_EQ(l.selectedRowIdx(), -1);
    EXPECT_EQ(l.overRowIdx(), -1);
    EXPECT_EQ(l.topItemIdx(), 0);
}

TEST(CListCtrl, RemoveRowAtShiftsSelection) {
    cListCtrl l;
    l.Init(0, 0, 200, 300, &g_basicImg);
    l.InitListCtrl(1, 10);
    l.AddRow({{"a"}}); l.AddRow({{"b"}}); l.AddRow({{"c"}});
    l.SetSelectedRowIdx(2);
    l.RemoveRowAt(0);  // remove 'a', 'b' shifts to idx 1
    EXPECT_EQ(l.rowCount(), 2u);
    EXPECT_EQ(l.selectedRowIdx(), 1);
    EXPECT_EQ(l.rowAt(1).texts[0], "c");
}

TEST(CListCtrl, RemoveSelectedRowClearsSelection) {
    cListCtrl l;
    l.Init(0, 0, 200, 300, &g_basicImg);
    l.InitListCtrl(1, 10);
    l.AddRow({{"a"}}); l.AddRow({{"b"}});
    l.SetSelectedRowIdx(0);
    // Removing the selected row itself must clear the selection (the
    // data is gone — we can't keep pointing at the same index after
    // erase because the next row has shifted up).
    l.RemoveRowAt(0);
    EXPECT_EQ(l.rowCount(), 1u);
    EXPECT_EQ(l.selectedRowIdx(), -1);
    // Remove the only remaining row.
    l.RemoveRowAt(0);
    EXPECT_EQ(l.rowCount(), 0u);
    EXPECT_EQ(l.selectedRowIdx(), -1);
}

TEST(CListCtrl, SetTopItemIdxClampsToRange) {
    cListCtrl l;
    l.Init(0, 0, 200, 300, &g_basicImg);
    l.InitListCtrl(1, 5);  // 5 visible rows
    l.AddRow({{"a"}}); l.AddRow({{"b"}}); l.AddRow({{"c"}});
    l.SetTopItemIdx(99);
    EXPECT_EQ(l.topItemIdx(), 0);  // maxTop = 3 - 5 = -2 → 0
    l.SetTopItemIdx(-5);
    EXPECT_EQ(l.topItemIdx(), 0);
}

TEST(CListCtrl, SetTopItemIdxAllowsValidOffset) {
    cListCtrl l;
    l.Init(0, 0, 200, 300, &g_basicImg);
    l.InitListCtrl(1, 2);
    l.AddRow({{"a"}}); l.AddRow({{"b"}}); l.AddRow({{"c"}});
    l.AddRow({{"d"}}); l.AddRow({{"e"}});
    l.SetTopItemIdx(1);
    EXPECT_EQ(l.topItemIdx(), 1);
    l.SetTopItemIdx(3);   // maxTop = 5 - 2 = 3
    EXPECT_EQ(l.topItemIdx(), 3);
    l.SetTopItemIdx(4);
    EXPECT_EQ(l.topItemIdx(), 3);
}

TEST(CListCtrl, PtIdxInRowInsideBody) {
    cListCtrl l;
    l.Init(10, 20, 200, 300, &g_basicImg);
    l.InitListCtrlImage(nullptr, 25, nullptr, 20, nullptr);
    l.InitListCtrl(1, 5);
    // Body starts at y = 20 + 25 = 45. Rows are 20 px tall, so row 0 is
    // y in (45, 65], row 1 is (65, 85], etc.
    EXPECT_EQ(l.PtIdxInRow(50, 50), 0u);
    EXPECT_EQ(l.PtIdxInRow(50, 70), 1u);
    EXPECT_EQ(l.PtIdxInRow(50, 90), 2u);
    // Header area: y = 25..45 is above the body.
    EXPECT_GT(l.PtIdxInRow(50, 30), 5u);
    // Below body.
    EXPECT_GT(l.PtIdxInRow(50, 200), 5u);
    // Outside x range.
    EXPECT_GT(l.PtIdxInRow(5, 50), 5u);
    EXPECT_GT(l.PtIdxInRow(250, 50), 5u);
}

TEST(CListCtrl, ActionEventUpdatesOverAndSelection) {
    cListCtrl l;
    l.Init(10, 20, 200, 300, &g_basicImg);
    l.InitListCtrlImage(nullptr, 25, nullptr, 20, nullptr);
    l.InitListCtrl(1, 5);
    l.AddRow({{"a"}}); l.AddRow({{"b"}}); l.AddRow({{"c"}});
    int clickRow = -1;
    int clickCount = 0;
    l.SetClickFunc([&](cListCtrl&, std::int32_t r, void*) {
        clickRow = r; ++clickCount;
    });
    // Hover over row 1 (y in (65, 85]).
    l.ActionEvent(50, 70, 0);
    EXPECT_EQ(l.overRowIdx(), 1);
    EXPECT_EQ(l.selectedRowIdx(), -1);  // not clicked yet
    // Click on row 1.
    l.ActionEvent(50, 70, cWindow::MouseFlagLButton);
    EXPECT_EQ(l.selectedRowIdx(), 1);
    EXPECT_EQ(clickRow, 1);
    EXPECT_EQ(clickCount, 1);
}

TEST(CListCtrl, ActionEventClickOutsideClearsSelection) {
    cListCtrl l;
    l.Init(0, 0, 200, 300, &g_basicImg);
    l.InitListCtrlImage(nullptr, 25, nullptr, 20, nullptr);
    l.InitListCtrl(1, 5);
    l.AddRow({{"a"}});
    l.SetSelectedRowIdx(0);
    l.ActionEvent(500, 500, cWindow::MouseFlagLButton);
    EXPECT_EQ(l.selectedRowIdx(), -1);
}

TEST(CListCtrl, SetSelectOptionSwitchesHighlightMode) {
    cListCtrl l;
    l.Init(0, 0, 200, 300, &g_basicImg);
    EXPECT_EQ(l.selectOption(), 0);
    l.SetSelectOption(1);
    EXPECT_EQ(l.selectOption(), 1);
}

TEST(CListCtrl, SetMarginAndAccessor) {
    cListCtrl l;
    l.Init(0, 0, 200, 300, &g_basicImg);
    EXPECT_EQ(l.marginLeft(), 3);
    EXPECT_EQ(l.marginTop(),  4);
    l.SetMargin(10, 12);
    EXPECT_EQ(l.marginLeft(), 10);
    EXPECT_EQ(l.marginTop(),  12);
}

TEST(CListCtrl, ActionEventDisabledReturnsNull) {
    cListCtrl l;
    l.Init(0, 0, 200, 300, &g_basicImg);
    l.InitListCtrlImage(nullptr, 25, nullptr, 20, nullptr);
    l.InitListCtrl(1, 5);
    l.AddRow({{"a"}});
    l.SetDisable(true);
    EXPECT_EQ(l.ActionEvent(50, 50, cWindow::MouseFlagLButton),
              static_cast<std::uint32_t>(cWindow::WindowEvent::Null));
}

TEST(CListCtrl, SetColumnsCapsRowVectors) {
    // Legacy contract: changing the column count shrinks any existing
    // rows' cell vectors to match.
    cListCtrl l;
    l.Init(0, 0, 200, 300, &g_basicImg);
    l.InitListCtrl(5, 5);
    l.AddRow({{"a", "b", "c", "d", "e", "f"}, {0,0,0,0,0,0}});
    EXPECT_EQ(l.rowAt(0).texts.size(), 6u);
    l.SetColumns({{50, "x"}, {50, "y"}});
    EXPECT_EQ(l.rowAt(0).texts.size(), 2u);
    EXPECT_EQ(l.rowAt(0).texts[0], "a");
    EXPECT_EQ(l.rowAt(0).texts[1], "b");
}
