// wearedexdialog_test.cpp - Phase 12.x P2-12 Tier 2 dialog 1:1 port
// contract test for modern cWearedExDialog (equipment slot dialog:
// 10 equipment slots + 4 Titan equipment slots).
//
// Covers modern/src/ui/wearedexdialog.{hpp,cpp}, a 1:1 port of
//   墨香【源码】\[Client]MH\WearedExDialog.h (714 B) and
//   墨香【源码】\[Client]MH\WearedExDialog.cpp.
//
// What's tested:
//   - Default construction: cWearedExDialog is a cIconDialog
//     and inherits its cell layout.
//   - AddItem wraps cIconDialog::AddIcon (REAL):
//     * Returns true when AddIcon succeeds (cell is empty
//       and cellIdx < num cells).
//     * Returns false when cellIdx out of range.
//     * Returns false when cell is already in use (legacy
//       refuses double-add).
//   - DeleteItem wraps cIconDialog::DeleteIcon (REAL):
//     * Returns true when DeleteIcon succeeds (cell is
//       in use).
//     * Returns false when cell is not in use.
//     * DeleteItem's outIcon is set to the previous icon.
//   - AddItem + DeleteItem round-trip: AddItem puts an icon
//     in cell 0, DeleteItem removes it.
//   - AddItem on multiple cells: 10 equipment slots + 4
//     Titan slots = 14 cells (1:1 with legacy WT_WEAREDDIALOG
//     layout).
//   - Singleton dispatch (Titan vs normal branch) is TODO
//     (7-singleton path deferred). AddItem + DeleteItem
//     return true on base success but no singleton side
//     effects occur — verified by no crash and no observable
//     state change in the dialog itself.
//
// 1:1 quirks preserved:
//   - Ctor drops the m_type = WT_WEAREDDIALOG /
//     m_nIconType = WT_ITEM assignments (1:1 quirk: legacy
//     cWindow type tags removed in Phase 6).
//   - AddItem's 7-singleton dispatch (Titan vs normal
//     branch based on item->GetItemKind() &
//     eTITAN_EQUIPITEM) is TODO. The 1:1 quirk of
//     pHero->SetCurComboNum(SKILL_COMBO_NUM) on weapon
//     swap (1:1 quirk: weapon swap resets combo to 0)
//     is documented in the TODO.
//   - DeleteItem's 7-singleton dispatch is the same
//     pattern as AddItem's TODO.
//   - AddItem + DeleteItem return false on base failure
//     (1:1 with legacy return FALSE / 0 contract).
//   - AddItem + DeleteItem return true on base success
//     (1:1 with legacy return TRUE / 1 contract).

#include "wearedexdialog.hpp"
#include "cIconDialog.hpp"

#include <gtest/gtest.h>

#include <cstdint>

namespace mxh::ui::test {

// ===========================================================================
// Construction
// ===========================================================================

TEST(CWearedExDialogTest, DefaultConstructionIsValid) {
    cWearedExDialog dlg;
    // 1:1 quirk: ctor body is empty (m_type /
    // m_nIconType don't exist in modern cWindow /
    // cIconDialog). The dialog is a valid cIconDialog
    // base.
    SUCCEED();
}

TEST(CWearedExDialogTest, InheritsIconDialogCellLayout) {
    cWearedExDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetCellNum(10);
    EXPECT_EQ(dlg.GetCellNum(), 10u);
    // Add 4 more cells for the 4 Titan equipment slots
    // (1:1 with legacy 10 + 4 = 14 cell layout).
    dlg.SetCellNum(14);
    EXPECT_EQ(dlg.GetCellNum(), 14u);
}

// ===========================================================================
// AddItem (1:1 wrap of cIconDialog::AddIcon)
// ===========================================================================

namespace {

// Opaque cIcon* factory for tests. cIcon is forward-
// declared in cIconDialog.hpp (no modern port yet —
// R-12.x deferred), so we just use reinterpret_cast
// to a non-null opaque pointer.
cIcon* MakeOpaqueIcon() {
    return reinterpret_cast<cIcon*>(0x1);
}

}  // namespace

TEST(CWearedExDialogTest, AddItemSuccessReturnsTrue) {
    cWearedExDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetCellNum(10);

    EXPECT_TRUE(dlg.AddItem(/*relPos=*/0, MakeOpaqueIcon()));
}

TEST(CWearedExDialogTest, AddItemOutOfRangeReturnsFalse) {
    cWearedExDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetCellNum(10);

    EXPECT_FALSE(dlg.AddItem(/*relPos=*/10, MakeOpaqueIcon()));
    EXPECT_FALSE(dlg.AddItem(/*relPos=*/100, MakeOpaqueIcon()));
}

TEST(CWearedExDialogTest, AddItemDoubleAddRefusesSecond) {
    // 1:1 quirk: legacy AddIcon refuses double-add (if
    // cell is already in use, AddIcon returns FALSE).
    // Modern AddItem returns false on the second call.
    cWearedExDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetCellNum(10);

    EXPECT_TRUE(dlg.AddItem(/*relPos=*/0, MakeOpaqueIcon()));
    EXPECT_FALSE(dlg.AddItem(/*relPos=*/0, MakeOpaqueIcon()));
}

TEST(CWearedExDialogTest, AddItemDifferentCellsAllSucceed) {
    cWearedExDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetCellNum(14);  // 10 equip + 4 Titan slots

    for (std::uint16_t i = 0; i < 14; ++i) {
        EXPECT_TRUE(dlg.AddItem(i, MakeOpaqueIcon()));
    }
    // All 14 cells now occupied.
    for (std::uint16_t i = 0; i < 14; ++i) {
        EXPECT_FALSE(dlg.AddItem(i, MakeOpaqueIcon()));
    }
}

// ===========================================================================
// DeleteItem (1:1 wrap of cIconDialog::DeleteIcon)
// ===========================================================================

TEST(CWearedExDialogTest, DeleteItemSuccessReturnsTrue) {
    cWearedExDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetCellNum(10);
    ASSERT_TRUE(dlg.AddItem(/*relPos=*/0, MakeOpaqueIcon()));

    cIcon* out = nullptr;
    EXPECT_TRUE(dlg.DeleteItem(/*relPos=*/0, &out));
}

TEST(CWearedExDialogTest, DeleteItemEmptyCellReturnsFalse) {
    cWearedExDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetCellNum(10);

    cIcon* out = nullptr;
    EXPECT_FALSE(dlg.DeleteItem(/*relPos=*/0, &out));
}

TEST(CWearedExDialogTest, DeleteItemOutOfRangeReturnsFalse) {
    cWearedExDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetCellNum(10);

    cIcon* out = nullptr;
    EXPECT_FALSE(dlg.DeleteItem(/*relPos=*/10, &out));
    EXPECT_FALSE(dlg.DeleteItem(/*relPos=*/100, &out));
}

TEST(CWearedExDialogTest, DeleteItemSetsOutIconToPrevious) {
    // 1:1 quirk: legacy DeleteIcon sets *outIcon to
    // the cell's previous icon before clearing. Modern
    // DeleteItem passes the outIcon pointer through to
    // base DeleteIcon which sets it.
    cWearedExDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetCellNum(10);
    cIcon* original = MakeOpaqueIcon();
    ASSERT_TRUE(dlg.AddItem(/*relPos=*/0, original));

    cIcon* out = nullptr;
    ASSERT_TRUE(dlg.DeleteItem(/*relPos=*/0, &out));
    EXPECT_EQ(out, original);
}

// ===========================================================================
// AddItem + DeleteItem round-trip
// ===========================================================================

TEST(CWearedExDialogTest, AddDeleteRoundTrip) {
    cWearedExDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetCellNum(10);

    // Add to cell 0.
    EXPECT_TRUE(dlg.AddItem(/*relPos=*/0, MakeOpaqueIcon()));
    EXPECT_FALSE(dlg.AddItem(/*relPos=*/0, MakeOpaqueIcon()));  // double-add fails

    // Delete from cell 0.
    cIcon* out = nullptr;
    EXPECT_TRUE(dlg.DeleteItem(/*relPos=*/0, &out));

    // Re-add to cell 0 succeeds (cell is empty again).
    EXPECT_TRUE(dlg.AddItem(/*relPos=*/0, MakeOpaqueIcon()));
}

TEST(CWearedExDialogTest, AddDeleteIndependenceBetweenCells) {
    cWearedExDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetCellNum(10);

    EXPECT_TRUE(dlg.AddItem(/*relPos=*/0, MakeOpaqueIcon()));
    EXPECT_TRUE(dlg.AddItem(/*relPos=*/5, MakeOpaqueIcon()));

    // Delete cell 0, cell 5 still occupied.
    cIcon* out = nullptr;
    EXPECT_TRUE(dlg.DeleteItem(/*relPos=*/0, &out));

    // Cell 5 still can't be re-added (it's occupied).
    EXPECT_FALSE(dlg.AddItem(/*relPos=*/5, MakeOpaqueIcon()));
    // Cell 0 can be re-added (it's now empty).
    EXPECT_TRUE(dlg.AddItem(/*relPos=*/0, MakeOpaqueIcon()));
}

}  // namespace mxh::ui::test
