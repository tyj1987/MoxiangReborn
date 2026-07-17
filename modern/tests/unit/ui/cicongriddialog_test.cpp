// cicongriddialog_test.cpp — Phase 6.13 coverage for cIconGridDialog
// (2D icon grid with drag-drop semantics). Tests the data model + cell
// math + selection + SetAbsXY. Render / cbWindowFunc dispatch /
// IsDragOverDraw are no-ops in the modern port and are exercised
// separately by the 6.6 cImage seam follow-up.

#include "cIconGridDialog.hpp"

#include <gtest/gtest.h>

#include <cstdint>

namespace {

// We use cIcon as an opaque tag — the data model tests don't need a
// real cIcon implementation. Cast dummy pointers through a helper to
// keep the tests independent of the cIcon worker module.
mxh::ui::cIcon* MakeIcon(void* tag) {
    return reinterpret_cast<mxh::ui::cIcon*>(tag);
}

}  // namespace

TEST(CIconGridDialog, DefaultStateHasZeroCells) {
    mxh::ui::cIconGridDialog d;
    EXPECT_EQ(d.GetCellNum(), 0u);
    EXPECT_EQ(d.row(), 0u);
    EXPECT_EQ(d.col(), 0u);
    EXPECT_EQ(d.GetCurSelCellPos(), -1);
    EXPECT_EQ(d.GetAcceptableIconType(), 0xFFFFFFFFu);
    EXPECT_FALSE(d.IsShowGrid());
}

TEST(CIconGridDialog, InitAllocatesEmptyGrid) {
    mxh::ui::cIconGridDialog d;
    d.Init(0, 0, 200, 200, nullptr, /*col*/4, /*row*/3, /*id*/7);
    EXPECT_EQ(d.GetCellNum(), 12u);     // 4 cols × 3 rows
    EXPECT_EQ(d.row(), 3u);
    EXPECT_EQ(d.col(), 4u);
    EXPECT_EQ(d.id(), 7);
    for (std::uint16_t i = 0; i < 12; ++i) {
        EXPECT_EQ(d.GetIconForIdx(i), nullptr);
        EXPECT_TRUE(d.IsAddable(i));
    }
}

TEST(CIconGridDialog, InitGridComputesCellRect) {
    mxh::ui::cIconGridDialog d;
    d.Init(0, 0, 200, 200, nullptr, 4, 3);
    d.InitGrid(/*gridX*/10, /*gridY*/20,
               /*cellWid*/40, /*cellHei*/40,
               /*borderX*/4,  /*borderY*/4);
    auto rect = d.GetCellRect();
    // 1:1 with legacy: rect = (gridX, gridY, gridX + col*cellWid +
    // borderX*(col+1), gridY + row*cellHei + borderY*(row+1)).
    EXPECT_EQ(rect.left, 10);
    EXPECT_EQ(rect.top,  20);
    EXPECT_EQ(rect.right, 10 + 4 * 40 + 4 * 5);   // 10 + 160 + 20 = 190
    EXPECT_EQ(rect.bottom, 20 + 3 * 40 + 4 * 4);  // 20 + 120 + 16 = 156
}

TEST(CIconGridDialog, AddIconLinear) {
    mxh::ui::cIconGridDialog d;
    d.Init(0, 0, 200, 200, nullptr, 4, 3);
    auto* icon = MakeIcon(reinterpret_cast<void*>(0xA1));
    EXPECT_TRUE(d.AddIcon(0, icon));
    EXPECT_EQ(d.GetIconForIdx(0), icon);
    EXPECT_FALSE(d.IsAddable(0));
    EXPECT_TRUE(d.IsAddable(1));
    // Double-add refused.
    EXPECT_FALSE(d.AddIcon(0, MakeIcon(reinterpret_cast<void*>(0xA2))));
    EXPECT_EQ(d.GetIconForIdx(0), icon);
}

TEST(CIconGridDialog, AddIcon2D) {
    mxh::ui::cIconGridDialog d;
    d.Init(0, 0, 200, 200, nullptr, 4, 3);
    // Linear pos = cellY * col + cellX. For col=4, pos=7 → cellX=3, cellY=1.
    EXPECT_TRUE(d.AddIcon(/*cellX*/3, /*cellY*/1, MakeIcon(reinterpret_cast<void*>(0xC1))));
    EXPECT_EQ(d.GetIconForIdx(7), MakeIcon(reinterpret_cast<void*>(0xC1)));
    // Adding by linear (7) is the same cell — refused.
    EXPECT_FALSE(d.AddIcon(7, MakeIcon(reinterpret_cast<void*>(0xC2))));
    EXPECT_EQ(d.GetIconForIdx(7), MakeIcon(reinterpret_cast<void*>(0xC1)));
}

TEST(CIconGridDialog, AddIconOutOfRange) {
    mxh::ui::cIconGridDialog d;
    d.Init(0, 0, 200, 200, nullptr, 4, 3);
    EXPECT_FALSE(d.AddIcon(12, MakeIcon(reinterpret_cast<void*>(0xA1))));
    EXPECT_FALSE(d.AddIcon(99, MakeIcon(reinterpret_cast<void*>(0xA1))));
    EXPECT_FALSE(d.AddIcon(0, nullptr));
}

TEST(CIconGridDialog, GetPositionForCell) {
    mxh::ui::cIconGridDialog d;
    d.Init(0, 0, 200, 200, nullptr, 4, 3);
    EXPECT_EQ(d.GetPositionForCell(0, 0), 0u);
    EXPECT_EQ(d.GetPositionForCell(3, 0), 3u);
    EXPECT_EQ(d.GetPositionForCell(0, 1), 4u);
    EXPECT_EQ(d.GetPositionForCell(3, 2), 11u);
}

TEST(CIconGridDialog, DeleteIconLinear) {
    mxh::ui::cIconGridDialog d;
    d.Init(0, 0, 200, 200, nullptr, 4, 3);
    auto* icon = MakeIcon(reinterpret_cast<void*>(0xB1));
    ASSERT_TRUE(d.AddIcon(5, icon));
    mxh::ui::cIcon* out = nullptr;
    EXPECT_TRUE(d.DeleteIcon(5, &out));
    EXPECT_EQ(out, icon);
    EXPECT_EQ(d.GetIconForIdx(5), nullptr);
    EXPECT_TRUE(d.IsAddable(5));
}

TEST(CIconGridDialog, DeleteIconByPointer) {
    mxh::ui::cIconGridDialog d;
    d.Init(0, 0, 200, 200, nullptr, 4, 3);
    auto* icon = MakeIcon(reinterpret_cast<void*>(0xB2));
    ASSERT_TRUE(d.AddIcon(2, icon));
    EXPECT_TRUE(d.DeleteIcon(icon));
    EXPECT_EQ(d.GetIconForIdx(2), nullptr);
    EXPECT_FALSE(d.DeleteIcon(icon));  // already gone
    EXPECT_FALSE(d.DeleteIcon(nullptr));
}

TEST(CIconGridDialog, DeleteIcon2D) {
    mxh::ui::cIconGridDialog d;
    d.Init(0, 0, 200, 200, nullptr, 4, 3);
    auto* icon = MakeIcon(reinterpret_cast<void*>(0xB3));
    ASSERT_TRUE(d.AddIcon(2, 1, icon));
    EXPECT_TRUE(d.DeleteIcon(2, 1, nullptr));
    EXPECT_EQ(d.GetIconForIdx(6), nullptr);
}

TEST(CIconGridDialog, MoveIcon) {
    mxh::ui::cIconGridDialog d;
    d.Init(0, 0, 200, 200, nullptr, 4, 3);
    auto* icon = MakeIcon(reinterpret_cast<void*>(0xD1));
    ASSERT_TRUE(d.AddIcon(0, icon));
    EXPECT_TRUE(d.MoveIcon(2, 2, icon));
    EXPECT_EQ(d.GetIconForIdx(0), nullptr);
    EXPECT_EQ(d.GetIconForIdx(10), icon);
    // Move to a busy cell refused.
    ASSERT_TRUE(d.AddIcon(5, MakeIcon(reinterpret_cast<void*>(0xD2))));
    EXPECT_FALSE(d.MoveIcon(1, 1, icon));  // destination occupied
    // Source keeps the icon.
    EXPECT_EQ(d.GetIconForIdx(10), icon);
}

TEST(CIconGridDialog, IsAddable2DRequiresTypeMatch) {
    mxh::ui::cIconGridDialog d;
    d.Init(0, 0, 200, 200, nullptr, 4, 3);
    // Modern cIcon is opaque, so the type-mask check is no-op
    // (acceptableIconType is stored but not consulted). The
    // 2D overload only checks "cell in use" + "cell in bounds".
    auto* icon = MakeIcon(reinterpret_cast<void*>(0xE1));
    EXPECT_TRUE(d.IsAddable(2, 1, icon));
    EXPECT_TRUE(d.AddIcon(2, 1, icon));
    EXPECT_FALSE(d.IsAddable(2, 1, icon));
    EXPECT_FALSE(d.IsAddable(99, 99, icon));   // out of bounds
    EXPECT_FALSE(d.IsAddable(0, 99, icon));    // out of bounds
}

TEST(CIconGridDialog, AcceptableTypeSetterGetter) {
    mxh::ui::cIconGridDialog d;
    EXPECT_EQ(d.GetAcceptableIconType(), 0xFFFFFFFFu);
    d.SetAcceptableIconType(0x3u);
    EXPECT_EQ(d.GetAcceptableIconType(), 0x3u);
}

TEST(CIconGridDialog, GetCellPosition) {
    mxh::ui::cIconGridDialog d;
    d.Init(0, 0, 200, 200, nullptr, 4, 3);
    d.InitGrid(0, 0, 40, 40, 4, 4);
    // Cell (0, 0) is at (4, 4) - (44, 44); hit test uses DEFAULT_CELLSIZE=40.
    std::uint16_t cx = 99, cy = 99;
    EXPECT_TRUE(d.GetCellPosition(20, 20, cx, cy));
    EXPECT_EQ(cx, 0u);
    EXPECT_EQ(cy, 0u);
    // Cell (3, 0) is at absX+borderX*(3+1)+3*cellWid = 0+16+120 = 136.
    EXPECT_TRUE(d.GetCellPosition(150, 20, cx, cy));
    EXPECT_EQ(cx, 3u);
    EXPECT_EQ(cy, 0u);
    // Out of bounds.
    EXPECT_FALSE(d.GetCellPosition(200, 200, cx, cy));
    EXPECT_FALSE(d.GetCellPosition(-1, 20, cx, cy));
}

TEST(CIconGridDialog, GetPositionForXYRefLinearises) {
    mxh::ui::cIconGridDialog d;
    d.Init(0, 0, 200, 200, nullptr, 4, 3);
    d.InitGrid(0, 0, 40, 40, 4, 4);
    std::uint16_t pos = 99;
    EXPECT_TRUE(d.GetPositionForXYRef(20, 20, pos));
    EXPECT_EQ(pos, 0u);
    EXPECT_TRUE(d.GetPositionForXYRef(150, 20, pos));
    EXPECT_EQ(pos, 3u);
    EXPECT_TRUE(d.GetPositionForXYRef(150, 50, pos));
    EXPECT_EQ(pos, 7u);  // cell (3, 1) → 1*4+3
    EXPECT_FALSE(d.GetPositionForXYRef(500, 500, pos));
}

TEST(CIconGridDialog, GetCellAbsPos) {
    mxh::ui::cIconGridDialog d;
    d.Init(50, 100, 200, 200, nullptr, 4, 3);
    d.InitGrid(0, 0, 40, 40, 4, 4);
    ASSERT_TRUE(d.AddIcon(0, MakeIcon(reinterpret_cast<void*>(0x10))));
    // Cell pos 0: cellX=0, cellY=0, absX=50, absY=100.
    // 1:1 with legacy: cellpX = absX + cellRect.left + borderX*(cellX+1)
    //                            + cellX*cellWid.
    // cellRect is computed from InitGrid: left=0, top=0.
    int x = 0, y = 0;
    ASSERT_TRUE(d.GetCellAbsPos(0, x, y));
    EXPECT_EQ(x, 50 + 0 + 4 * 1 + 0 * 40);  // 54
    EXPECT_EQ(y, 100 + 0 + 4 * 1 + 0 * 40); // 104
    // Empty cell — GetCellAbsPos returns false (legacy behavior).
    EXPECT_FALSE(d.GetCellAbsPos(11, x, y));
    // Out-of-range.
    EXPECT_FALSE(d.GetCellAbsPos(99, x, y));
}

TEST(CIconGridDialog, PtInCellReturnsTrueInsideAnOccupiedCell) {
    mxh::ui::cIconGridDialog d;
    d.Init(0, 0, 200, 200, nullptr, 4, 3);
    d.InitGrid(0, 0, 40, 40, 4, 4);
    // No icons — PtInCell is false.
    EXPECT_FALSE(d.PtInCell(20, 20));
    // Add an icon, then PtInCell is true inside its cell.
    ASSERT_TRUE(d.AddIcon(0, MakeIcon(reinterpret_cast<void*>(0xF1))));
    EXPECT_TRUE(d.PtInCell(20, 20));
    EXPECT_FALSE(d.PtInCell(150, 20));   // cell 3, no icon
    EXPECT_FALSE(d.PtInCell(200, 200));  // outside grid
}

TEST(CIconGridDialog, SelectionSetterGetter) {
    mxh::ui::cIconGridDialog d;
    EXPECT_EQ(d.GetCurSelCellPos(), -1);
    d.SetCurSelCellPos(5);
    EXPECT_EQ(d.GetCurSelCellPos(), 5);
    d.SetCurSelCellPos(-1);
    EXPECT_EQ(d.GetCurSelCellPos(), -1);
}

TEST(CIconGridDialog, ShowGridFlag) {
    mxh::ui::cIconGridDialog d;
    EXPECT_FALSE(d.IsShowGrid());
    d.SetShowGrid(true);
    EXPECT_TRUE(d.IsShowGrid());
    d.SetShowGrid(false);
    EXPECT_FALSE(d.IsShowGrid());
}

TEST(CIconGridDialog, DragOverIconTypeSetterGetter) {
    mxh::ui::cIconGridDialog d;
    EXPECT_EQ(d.GetDragOverIconType(), 0);
    d.SetDragOverIconType(7);
    EXPECT_EQ(d.GetDragOverIconType(), 7);
}

TEST(CIconGridDialog, IsDragOverDrawAlwaysFalseInModernPort) {
    // The modern port's IsDragOverDraw is a no-op stub. The real
    // check needs cWindowManager's drag-window state, which lands
    // with 6.6.
    mxh::ui::cIconGridDialog d;
    d.Init(0, 0, 200, 200, nullptr, 4, 3);
    EXPECT_FALSE(d.IsDragOverDraw());
    d.SetDragOverIconType(99);  // even with a type set
    EXPECT_FALSE(d.IsDragOverDraw());
}

TEST(CIconGridDialog, SetCellRectGetter) {
    mxh::ui::cIconGridDialog d;
    d.SetCellRect(1, 2, 3, 4);
    auto r = d.GetCellRect();
    EXPECT_EQ(r.left, 1);
    EXPECT_EQ(r.top, 2);
    EXPECT_EQ(r.right, 3);
    EXPECT_EQ(r.bottom, 4);
}

TEST(CIconGridDialog, RenderIsNoop) {
    // Render is a no-op stub in the modern port (cImage seam 6.6).
    // Verify the call doesn't crash and doesn't change any state.
    mxh::ui::cIconGridDialog d;
    d.Init(0, 0, 200, 200, nullptr, 4, 3);
    d.Render();   // must not crash
    d.Render();   // idempotent
    EXPECT_EQ(d.GetCellNum(), 12u);
}

TEST(CIconGridDialog, SetAbsXYMovesDialog) {
    // 1:1 with legacy SetAbsXY. Modern cIcon is opaque, so the icon
    // cascade is a no-op stub. The dialog's own abs position
    // updates and PtInCell re-anchors to the new position.
    mxh::ui::cIconGridDialog d;
    d.Init(0, 0, 200, 200, nullptr, 4, 3);
    d.InitGrid(0, 0, 40, 40, 4, 4);
    ASSERT_TRUE(d.AddIcon(0, MakeIcon(reinterpret_cast<void*>(0x11))));
    d.SetAbsXY(100, 200);
    EXPECT_EQ(d.absX(), 100);
    EXPECT_EQ(d.absY(), 200);
    // Cell 0 is now at absX+borderX*1+0*cellWid = 100+4 = 104, but
    // PtInCell uses cellWid-based hit range.
    EXPECT_TRUE(d.PtInCell(120, 220));
    EXPECT_FALSE(d.PtInCell(20, 30));
}
