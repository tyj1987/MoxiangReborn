// cicondialog_test.cpp — Phase 6.11 coverage for cIconDialog (icon grid).

#include "cIconDialog.hpp"

#include <gtest/gtest.h>

namespace {

// We use the cIcon type as an opaque tag — the data model tests don't
// need a real cIcon implementation. Cast dummy pointers through a
// helper to keep the tests independent of the cIcon worker module.
mxh::ui::cIcon* MakeIcon(void* tag) {
    return reinterpret_cast<mxh::ui::cIcon*>(tag);
}

}  // namespace

TEST(CIconDialog, DefaultStateHasZeroCells) {
    mxh::ui::cIconDialog d;
    EXPECT_EQ(d.GetCellNum(), 0u);
    EXPECT_EQ(d.GetCurSelCellPos(), -1);
    EXPECT_EQ(d.GetAcceptableIconType(), 0xFFFFFFFFu);
}

TEST(CIconDialog, SetCellNumCreatesEmptyCells) {
    mxh::ui::cIconDialog d;
    d.SetCellNum(10);
    EXPECT_EQ(d.GetCellNum(), 10u);
    for (std::uint16_t i = 0; i < 10; ++i) {
        EXPECT_EQ(d.GetIconForIdx(i), nullptr);
        EXPECT_TRUE(d.IsAddable(i));
        // IsAcceptable is a uint-and-mask check; default value 0xFFFFFFFF accepts all.
        EXPECT_TRUE(d.IsAcceptable(0xFFFFFFFFu));
        EXPECT_TRUE(d.IsAcceptable(0x1u));
    }
}

TEST(CIconDialog, AddIconThenIsAddableFalse) {
    mxh::ui::cIconDialog d;
    d.SetCellNum(3);
    auto* icon = MakeIcon(reinterpret_cast<void*>(0xA1));
    EXPECT_TRUE(d.AddIcon(1, icon));
    EXPECT_FALSE(d.IsAddable(1));
    EXPECT_TRUE(d.IsAddable(0));
    EXPECT_TRUE(d.IsAddable(2));
    EXPECT_EQ(d.GetIconForIdx(1), icon);
}

TEST(CIconDialog, AddIconDoubleAddRefuses) {
    mxh::ui::cIconDialog d;
    d.SetCellNum(2);
    EXPECT_TRUE(d.AddIcon(0, MakeIcon(reinterpret_cast<void*>(0xA1))));
    EXPECT_FALSE(d.AddIcon(0, MakeIcon(reinterpret_cast<void*>(0xA2))));
    EXPECT_EQ(d.GetIconForIdx(0), MakeIcon(reinterpret_cast<void*>(0xA1)));
}

TEST(CIconDialog, AddIconOutOfRangeRefuses) {
    mxh::ui::cIconDialog d;
    d.SetCellNum(1);
    EXPECT_FALSE(d.AddIcon(5, MakeIcon(reinterpret_cast<void*>(0xA1))));
    EXPECT_FALSE(d.AddIcon(255, nullptr));
}

TEST(CIconDialog, DeleteIconClearsCell) {
    mxh::ui::cIconDialog d;
    d.SetCellNum(2);
    auto* icon = MakeIcon(reinterpret_cast<void*>(0xB1));
    ASSERT_TRUE(d.AddIcon(0, icon));
    mxh::ui::cIcon* out = nullptr;
    EXPECT_TRUE(d.DeleteIcon(0, &out));
    EXPECT_EQ(out, icon);
    EXPECT_EQ(d.GetIconForIdx(0), nullptr);
    EXPECT_TRUE(d.IsAddable(0));
}

TEST(CIconDialog, DeleteIconWithoutOutParam) {
    mxh::ui::cIconDialog d;
    d.SetCellNum(1);
    d.AddIcon(0, MakeIcon(reinterpret_cast<void*>(0xB1)));
    EXPECT_TRUE(d.DeleteIcon(0));
    EXPECT_EQ(d.GetIconForIdx(0), nullptr);
}

TEST(CIconDialog, DeleteIconOnEmptyCellIsFalse) {
    mxh::ui::cIconDialog d;
    d.SetCellNum(2);
    EXPECT_FALSE(d.DeleteIcon(0));
    EXPECT_FALSE(d.DeleteIcon(99));
}

TEST(CIconDialog, DeleteIconAllClearsEverything) {
    mxh::ui::cIconDialog d;
    d.SetCellNum(5);
    for (std::uint16_t i = 0; i < 5; ++i) {
        d.AddIcon(i, MakeIcon(reinterpret_cast<void*>(0x10 + i)));
    }
    d.DeleteIconAll();
    for (std::uint16_t i = 0; i < 5; ++i) {
        EXPECT_EQ(d.GetIconForIdx(i), nullptr);
        EXPECT_TRUE(d.IsAddable(i));
    }
}

TEST(CIconDialog, AddIconCellSetsRect) {
    mxh::ui::cIconDialog d;
    d.SetCellNum(3);
    d.AddIconCell(10, 20, 30, 40);
    d.AddIconCell(50, 70, 30, 40);
    // Walk internal state by querying through PtInCell. The dialog is at (0,0)
    // initially (no Init call in this test), so cells are at (10,20)-(40,60)
    // and (50,70)-(80,110).
    EXPECT_TRUE (d.PtInCell(20, 30));
    EXPECT_TRUE (d.PtInCell(60, 80));
    EXPECT_FALSE(d.PtInCell(0, 0));      // outside both cells
    EXPECT_FALSE(d.PtInCell(200, 200));  // well outside
}

TEST(CIconDialog, AddIconCellRespectsDialogAbsXAbsY) {
    mxh::ui::cIconDialog d;
    d.Init(100, 200, 400, 300, nullptr, 5);
    d.SetCellNum(1);
    d.AddIconCell(0, 0, 50, 50);
    // Cell should be at dialog absX+0..absX+50, absY+0..absY+50.
    EXPECT_TRUE (d.PtInCell(120, 230));
    EXPECT_TRUE (d.PtInCell(150, 250));
    EXPECT_FALSE(d.PtInCell(99, 230));
    EXPECT_FALSE(d.PtInCell(151, 230));
}

TEST(CIconDialog, GetPositionForXYRefReturnsIndex) {
    mxh::ui::cIconDialog d;
    d.SetCellNum(4);
    d.AddIconCell(  0,   0, 50, 50);  // cell 0: top-left
    d.AddIconCell( 50,   0, 50, 50);  // cell 1: top-right
    d.AddIconCell(  0,  50, 50, 50);  // cell 2: bot-left
    d.AddIconCell( 50,  50, 50, 50);  // cell 3: bot-right

    std::uint16_t pos = 99;
    EXPECT_TRUE (d.GetPositionForXYRef(20, 20, pos));
    EXPECT_EQ   (pos, 0u);

    EXPECT_TRUE (d.GetPositionForXYRef(80, 20, pos));
    EXPECT_EQ   (pos, 1u);

    EXPECT_TRUE (d.GetPositionForXYRef(80, 80, pos));
    EXPECT_EQ   (pos, 3u);

    EXPECT_FALSE(d.GetPositionForXYRef(200, 200, pos));
    // pos unchanged when not found (we leave it untouched as the input).
}

TEST(CIconDialog, AcceptableTypeMaskWorks) {
    mxh::ui::cIconDialog d;
    d.SetAcceptableIconType(0x3u);    // accept types 1 and 2 only.
    EXPECT_TRUE (d.IsAcceptable(0x1u));
    EXPECT_TRUE (d.IsAcceptable(0x2u));
    EXPECT_TRUE (d.IsAcceptable(0x3u));
    EXPECT_FALSE(d.IsAcceptable(0x4u));
    EXPECT_FALSE(d.IsAcceptable(0x0u));
}

TEST(CIconDialog, SelectionSetterGetter) {
    mxh::ui::cIconDialog d;
    d.SetCellNum(3);
    EXPECT_EQ(d.GetCurSelCellPos(), -1);
    d.SetCurSelCellPos(2);
    EXPECT_EQ(d.GetCurSelCellPos(), 2);
    d.SetCurSelCellPos(-1);  // deselect
    EXPECT_EQ(d.GetCurSelCellPos(), -1);
}

TEST(CIconDialog, SetAbsXYMovesDialogAbsPosition) {
    mxh::ui::cIconDialog d;
    d.Init(10, 20, 200, 200, nullptr, 5);
    d.SetCellNum(1);
    d.AddIconCell(0, 0, 50, 50);
    // Initially the cell is at absX=10,absY=20.
    EXPECT_TRUE(d.PtInCell(20, 30));
    // Move the dialog: cells should still be hit-testable relative to
    // the new dialog position.
    d.SetAbsXY(100, 200);
    EXPECT_TRUE(d.PtInCell(120, 220));
    // Old absolute position should no longer be inside a cell.
    EXPECT_FALSE(d.PtInCell(20, 30));
    EXPECT_EQ(d.absX(), 100);
    EXPECT_EQ(d.absY(), 200);
}
