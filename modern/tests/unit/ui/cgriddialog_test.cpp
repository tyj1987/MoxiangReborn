//
// Unit tests for mxh::ui::cGridDialog
//   (Phase C 1:1 port of legacy cGridDialog from [Client]MH/interface).
//
// Locks down the 1:1 surface documented in cgriddialog.hpp:
//   * ctor: m_pWindowCell = nullptr, m_wCellNum = 0.
//   * dtor: empty body (1:1).
//   * Init(x, y, w, h, basicImage, pCellWindow, cellNum, id=0):
//       - calls cDialog::Init with the position/size/image/id args.
//       - records m_pWindowCell = pCellWindow (no copy, raw ptr).
//       - records m_wCellNum = cellNum.
//       - adds cellNum cloned cPushupButton children to the tree.
//       - clones m_pushed + m_passive from each source cell.
//   * 1:1 quirks: m_type=WT_GRIDDIALOG drop, memcpy->member-wise copy,
//                 source-array not freed (caller-managed).

#include "mxh/ui/cgriddialog.hpp"
#include "mxh/ui/cDialog.hpp"
#include "mxh/ui/cWindow.hpp"
#include "mxh/ui/cPushupButton.hpp"

#include <gtest/gtest.h>

#include <type_traits>
#include <vector>

using mxh::ui::cDialog;
using mxh::ui::cGridDialog;
using mxh::ui::cPushupButton;
using mxh::ui::cWindow;

// ---- 1:1 class invariants -------------------------------------------

TEST(GridDialogTest, InheritsFromCDialog) {
    static_assert(std::is_base_of_v<cDialog, cGridDialog>,
                  "cGridDialog must inherit from cDialog");
    SUCCEED();
}

TEST(GridDialogTest, IsAlsoAWindow) {
    static_assert(std::is_base_of_v<cWindow, cGridDialog>,
                  "cGridDialog must be a cWindow (transitively)");
    SUCCEED();
}

TEST(GridDialogTest, NonCopyable) {
    static_assert(!std::is_copy_constructible_v<cGridDialog>,
                  "cGridDialog must be non-copyable");
    static_assert(!std::is_copy_assignable_v<cGridDialog>,
                  "cGridDialog must be non-copy-assignable");
    SUCCEED();
}

// ---- 1:1 ctor defaults ----------------------------------------------

TEST(GridDialogTest, DefaultCtorZeroesState) {
    cGridDialog d;
    EXPECT_EQ(d.WindowCell(), nullptr);
    EXPECT_EQ(d.CellNum(), 0u);
}

TEST(GridDialogTest, DefaultCtorNoChildren) {
    cGridDialog d;
    EXPECT_EQ(d.childCount(), 0u);
}

TEST(GridDialogTest, DtorDoesNotCrash) {
    cGridDialog d;
    SUCCEED();  // destruction is a no-op (1:1 with legacy).
}

// ---- 1:1 Init: base delegation + cellNum / m_pWindowCell ----------

TEST(GridDialogTest, InitStoresBaseDialogAttributes) {
    cGridDialog d;
    d.Init(10, 20, 200, 100, nullptr, nullptr, 0, 7);
    EXPECT_EQ(d.absX(), 10);
    EXPECT_EQ(d.absY(), 20);
    EXPECT_EQ(d.width(), 200u);
    EXPECT_EQ(d.height(), 100u);
    EXPECT_EQ(d.id(), 7);
}

TEST(GridDialogTest, InitRecordsCellNumAndWindowCell) {
    cGridDialog d;
    std::vector<cPushupButton> cells(3);
    d.Init(0, 0, 100, 100, nullptr, cells.data(), 3, 1);
    EXPECT_EQ(d.CellNum(), 3u);
    // WindowCell() returns the raw source pointer (1:1 legacy lock).
    EXPECT_EQ(d.WindowCell(), cells.data());
}

TEST(GridDialogTest, InitWithCellNumZeroAddsNoChildren) {
    cGridDialog d;
    std::vector<cPushupButton> cells(1);
    d.Init(0, 0, 100, 100, nullptr, cells.data(), 0, 1);
    EXPECT_EQ(d.childCount(), 0u);
    EXPECT_EQ(d.CellNum(), 0u);
    // Source pointer is still recorded (1:1 legacy lock).
    EXPECT_EQ(d.WindowCell(), cells.data());
}

TEST(GridDialogTest, InitWithNullSourceArrayAndZeroCount) {
    cGridDialog d;
    d.Init(0, 0, 50, 50, nullptr, nullptr, 0, 1);
    EXPECT_EQ(d.childCount(), 0u);
    EXPECT_EQ(d.WindowCell(), nullptr);
    EXPECT_EQ(d.CellNum(), 0u);
}

// ---- 1:1 Init: child cloning ---------------------------------------

TEST(GridDialogTest, InitAddsCellNumChildren) {
    cGridDialog d;
    std::vector<cPushupButton> cells(5);
    d.Init(0, 0, 200, 200, nullptr, cells.data(), 5, 1);
    EXPECT_EQ(d.childCount(), 5u);
}

TEST(GridDialogTest, InitClonesPushedState) {
    cGridDialog d;
    std::vector<cPushupButton> cells(3);
    cells[0].SetPush(false);
    cells[1].SetPush(true);
    cells[2].SetPush(false);
    d.Init(0, 0, 100, 100, nullptr, cells.data(), 3, 1);
    ASSERT_EQ(d.childCount(), 3u);
    // Each child is a cPushupButton; verify the cloned pushed state.
    for (std::size_t i = 0; i < 3; ++i) {
        auto* pb = dynamic_cast<cPushupButton*>(d.childAt(i));
        ASSERT_NE(pb, nullptr);
        EXPECT_EQ(pb->IsPushed(), cells[i].IsPushed())
            << "pushed state mismatch at cell " << i;
    }
}

TEST(GridDialogTest, InitClonesPassiveState) {
    cGridDialog d;
    std::vector<cPushupButton> cells(3);
    cells[0].SetPassive(false);
    cells[1].SetPassive(false);
    cells[2].SetPassive(true);
    d.Init(0, 0, 100, 100, nullptr, cells.data(), 3, 1);
    ASSERT_EQ(d.childCount(), 3u);
    for (std::size_t i = 0; i < 3; ++i) {
        auto* pb = dynamic_cast<cPushupButton*>(d.childAt(i));
        ASSERT_NE(pb, nullptr);
        EXPECT_EQ(pb->IsPassive(), cells[i].IsPassive())
            << "passive state mismatch at cell " << i;
    }
}

TEST(GridDialogTest, InitChildrenAreCPushupButtonSubclass) {
    cGridDialog d;
    std::vector<cPushupButton> cells(2);
    d.Init(0, 0, 100, 100, nullptr, cells.data(), 2, 1);
    ASSERT_EQ(d.childCount(), 2u);
    for (std::size_t i = 0; i < 2; ++i) {
        // 1:1 quirk: cDialog::Add took ownership of cPushupButton
        // clones, so the child must be a cPushupButton (RTTI).
        auto* pb = dynamic_cast<cPushupButton*>(d.childAt(i));
        EXPECT_NE(pb, nullptr);
    }
}

// ---- 1:1 quirk: source array ownership --------------------------

TEST(GridDialogTest, InitDoesNotFreeSourceArray) {
    // 1:1 quirk: legacy SAFE_DELETE_ARRAY(pCellWindow) is dropped in
    // the modern port (caller owns the source). Verify by inspecting
    // the cells vector after Init runs -- the data must still be
    // accessible.
    cGridDialog d;
    std::vector<cPushupButton> cells(3);
    cells[0].SetPush(true);
    cells[1].SetPush(true);
    cells[2].SetPush(true);
    d.Init(0, 0, 100, 100, nullptr, cells.data(), 3, 1);
    // Source vector is still alive (3 entries, all pushed).
    ASSERT_EQ(cells.size(), 3u);
    EXPECT_TRUE(cells[0].IsPushed());
    EXPECT_TRUE(cells[1].IsPushed());
    EXPECT_TRUE(cells[2].IsPushed());
}

// ---- 1:1 quirk: m_type=WT_GRIDDIALOG is dropped -----------------

TEST(GridDialogTest, NoWindowTypeEnumExposed) {
    // 1:1 quirk: legacy cGridDialog set m_type=WT_GRIDDIALOG in
    // ctor + Init; modern cWindow no longer has a m_type field. We
    // verify the dialog is still a valid cDialog / cWindow and that
    // no type-tag accessor exists. (No getter is exposed -- this
    // test exists purely to document the 1:1 quirk.)
    cGridDialog d;
    d.Init(0, 0, 50, 50, nullptr, nullptr, 0, 1);
    SUCCEED();
}

// ---- 1:1: Init default id argument ---------------------------------

TEST(GridDialogTest, InitDefaultIdIsZero) {
    cGridDialog d;
    d.Init(0, 0, 50, 50, nullptr, nullptr, 0);  // no id arg
    EXPECT_EQ(d.id(), 0);
}

// ---- 1:1: re-Init appends children -------------------------------

TEST(GridDialogTest, ReInitAppendsToChildTree) {
    // Legacy cGridDialog::Init does NOT clear the child tree -- it just
    // appends new cloned cells on top of whatever was there. The modern
    // port preserves that 1:1 lock (cDialog::Add unconditionally adds;
    // the dialog does not pre-clear). Verify that re-Init with 5 cells
    // after a 3-cell Init leaves 8 children (3+5) and updates the
    // m_wCellNum + m_pWindowCell fields to the *new* call's values.
    cGridDialog d;
    std::vector<cPushupButton> cells1(3);
    d.Init(0, 0, 100, 100, nullptr, cells1.data(), 3, 1);
    ASSERT_EQ(d.childCount(), 3u);

    std::vector<cPushupButton> cells2(5);
    d.Init(0, 0, 100, 100, nullptr, cells2.data(), 5, 1);
    EXPECT_EQ(d.childCount(), 8u);
    EXPECT_EQ(d.CellNum(), 5u);
    EXPECT_EQ(d.WindowCell(), cells2.data());
}
