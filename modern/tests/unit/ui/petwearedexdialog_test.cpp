// petwearedexdialog_test.cpp - Phase 12.x P2-12 Tier 2 dialog 1:1 port
// contract test for modern cPetWearedExDialog (pet equipment slot
// dialog: 3 pet equipment slots).
//
// Covers modern/src/ui/petwearedexdialog.{hpp,cpp}, a 1:1 port of
//   墨香【源码】\[Client]MH\PetWearedExDialog.h (445 B) and
//   墨香【源码】\[Client]MH\PetWearedExDialog.cpp.
//
// What's tested:
//   - Default construction: cPetWearedExDialog is a
//     cIconDialog and inherits its cell layout.
//   - Pet equipment slot count: 3 cells (1:1 with
//     legacy SLOT_PETWEAR_NUM = 3).
//   - kTpPetWearStart = 490 (1:1 with legacy
//     TP_PETWEAR_START = 490 / TP_PETINVEN_END).
//   - AddItem wraps cIconDialog::AddIcon (REAL):
//     * Returns true when AddIcon succeeds (cell is
//       empty and cellIdx < 3).
//     * Returns false when cellIdx out of range.
//     * Returns false when cell is already in use
//       (legacy refuses double-add).
//   - DeleteItem wraps cIconDialog::DeleteIcon (REAL):
//     * Returns true when DeleteIcon succeeds.
//     * Returns false when cell is not in use.
//     * DeleteItem's outIcon is set to the previous
//       icon (1:1 quirk: legacy DeleteIcon sets
//       *outIcon to the cell's previous icon before
//       clearing).
//   - AddItem + DeleteItem round-trip on cell 0/1/2.
//   - GetBlankPositionRestrictRef:
//     * Returns false on a fully empty dialog (no cells
//       configured). Note: 1:1 with legacy behavior
//       when GetCellNum() < SLOT_PETWEAR_NUM (the
//       legacy uses SLOT_PETWEAR_NUM as the upper
//       bound, so even with 0 cells configured the
//       loop simply exits and returns FALSE).
//     * Returns false on a fully occupied dialog (all
//       3 cells in use).
//     * Returns true with absPos = 490 + i when
//       cell i is the first addable cell.
//     * Returns the FIRST addable cell (not any later
//       one), 1:1 with legacy scan order.
//   - CheckDuplication: returns false (TODO, cItem
//     not ported — R-12.x deferred). When cItem is
//     ported, this should be replaced with the real
//     CItem*::GetItemIdx() comparison.
//
// 1:1 quirks preserved:
//   - Ctor body is empty (1:1 with legacy empty
//     CPetWearedExDialog ctor).
//   - AddItem's "!!!복사본 옵션 적용" Korean TODO
//     comment is preserved as a doc-only no-op (no
//     code in the legacy either).
//   - DeleteItem's same Korean comment is preserved.
//   - GetBlankPositionRestrictRef uses the inlined
//     kSlotPetWearNum=3 / kTpPetWearStart=490 constants
//     (1:1 with legacy [CC]Header/CommonGameDefine.h
//     enum values).
//   - CheckDuplication returns false unconditionally
//     (TODO until cItem port — same constraint as
//     cWearedExDialog's Titan-vs-normal branch).
//   - AddItem + DeleteItem return false on base failure
//     (1:1 with legacy return FALSE / 0 contract).
//   - AddItem + DeleteItem return true on base success
//     (1:1 with legacy return TRUE / 1 contract).

#include "petwearedexdialog.hpp"
#include "cIconDialog.hpp"

#include <gtest/gtest.h>

#include <cstdint>

namespace mxh::ui::test {

// ===========================================================================
// Construction + constants
// ===========================================================================

TEST(CPetWearedExDialogTest, DefaultConstructionIsValid) {
    cPetWearedExDialog dlg;
    // 1:1 quirk: ctor body is empty (legacy also has
    // empty CPetWearedExDialog() ctor). The dialog is
    // a valid cIconDialog base.
    SUCCEED();
}

TEST(CPetWearedExDialogTest, InheritsIconDialogCellLayout) {
    cPetWearedExDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    // 1:1 with legacy SLOT_PETWEAR_NUM = 3 (3 pet
    // equipment slots).
    dlg.SetCellNum(3);
    EXPECT_EQ(dlg.GetCellNum(), 3u);
}

TEST(CPetWearedExDialogTest, PetWearSlotCountIsThree) {
    // 1:1 with legacy SLOT_PETWEAR_NUM = 3 (defined
    // in [CC]Header/CommonGameDefine.h enum, line
    // 1215 / 1321 / 1428 across the locale variants).
    EXPECT_EQ(cPetWearedExDialog::kSlotPetWearNum, 3u);
}

TEST(CPetWearedExDialogTest, TpPetWearStartIsFourNinety) {
    // 1:1 with legacy TP_PETWEAR_START = 490 (defined
    // as TP_PETINVEN_END in the legacy enum). The
    // value 490 is the start of the pet-equip tab
    // position range.
    EXPECT_EQ(cPetWearedExDialog::kTpPetWearStart, 490u);
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

cIcon* MakeOpaqueIconAlt() {
    // Distinct opaque pointer value (0x2) so tests
    // can verify which icon is at which cell.
    return reinterpret_cast<cIcon*>(0x2);
}

}  // namespace

TEST(CPetWearedExDialogTest, AddItemSuccessReturnsTrue) {
    cPetWearedExDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetCellNum(3);

    EXPECT_TRUE(dlg.AddItem(/*relPos=*/0, MakeOpaqueIcon()));
}

TEST(CPetWearedExDialogTest, AddItemOutOfRangeReturnsFalse) {
    cPetWearedExDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetCellNum(3);

    // Legacy SLOT_PETWEAR_NUM = 3, so relPos >= 3 is
    // out of range.
    EXPECT_FALSE(dlg.AddItem(/*relPos=*/3, MakeOpaqueIcon()));
    EXPECT_FALSE(dlg.AddItem(/*relPos=*/100, MakeOpaqueIcon()));
}

TEST(CPetWearedExDialogTest, AddItemDoubleAddRefusesSecond) {
    // 1:1 quirk: legacy AddIcon refuses double-add (if
    // cell is already in use, AddIcon returns FALSE).
    // Modern AddItem returns false on the second call.
    cPetWearedExDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetCellNum(3);

    EXPECT_TRUE(dlg.AddItem(/*relPos=*/0, MakeOpaqueIcon()));
    EXPECT_FALSE(dlg.AddItem(/*relPos=*/0, MakeOpaqueIcon()));
}

TEST(CPetWearedExDialogTest, AddItemAllThreeCellsSucceed) {
    // All 3 pet equip slots can be filled (1:1 with
    // legacy SLOT_PETWEAR_NUM = 3).
    cPetWearedExDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetCellNum(3);

    for (std::uint16_t i = 0; i < 3; ++i) {
        EXPECT_TRUE(dlg.AddItem(i, MakeOpaqueIcon()));
    }
    // All 3 cells now occupied.
    for (std::uint16_t i = 0; i < 3; ++i) {
        EXPECT_FALSE(dlg.AddItem(i, MakeOpaqueIcon()));
    }
}

// ===========================================================================
// DeleteItem (1:1 wrap of cIconDialog::DeleteIcon)
// ===========================================================================

TEST(CPetWearedExDialogTest, DeleteItemSuccessReturnsTrue) {
    cPetWearedExDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetCellNum(3);
    ASSERT_TRUE(dlg.AddItem(/*relPos=*/0, MakeOpaqueIcon()));

    cIcon* out = nullptr;
    EXPECT_TRUE(dlg.DeleteItem(/*relPos=*/0, &out));
}

TEST(CPetWearedExDialogTest, DeleteItemEmptyCellReturnsFalse) {
    cPetWearedExDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetCellNum(3);

    cIcon* out = nullptr;
    EXPECT_FALSE(dlg.DeleteItem(/*relPos=*/0, &out));
}

TEST(CPetWearedExDialogTest, DeleteItemOutOfRangeReturnsFalse) {
    cPetWearedExDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetCellNum(3);

    cIcon* out = nullptr;
    EXPECT_FALSE(dlg.DeleteItem(/*relPos=*/3, &out));
    EXPECT_FALSE(dlg.DeleteItem(/*relPos=*/100, &out));
}

TEST(CPetWearedExDialogTest, DeleteItemSetsOutIconToPrevious) {
    // 1:1 quirk: legacy DeleteIcon sets *outIcon to
    // the cell's previous icon before clearing.
    cPetWearedExDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetCellNum(3);
    cIcon* original = MakeOpaqueIcon();
    ASSERT_TRUE(dlg.AddItem(/*relPos=*/0, original));

    cIcon* out = nullptr;
    ASSERT_TRUE(dlg.DeleteItem(/*relPos=*/0, &out));
    EXPECT_EQ(out, original);
}

// ===========================================================================
// AddItem + DeleteItem round-trip
// ===========================================================================

TEST(CPetWearedExDialogTest, AddDeleteRoundTrip) {
    cPetWearedExDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetCellNum(3);

    // Add to cell 0.
    EXPECT_TRUE(dlg.AddItem(/*relPos=*/0, MakeOpaqueIcon()));
    EXPECT_FALSE(dlg.AddItem(/*relPos=*/0, MakeOpaqueIcon()));  // double-add fails

    // Delete from cell 0.
    cIcon* out = nullptr;
    EXPECT_TRUE(dlg.DeleteItem(/*relPos=*/0, &out));

    // Re-add to cell 0 succeeds (cell is empty again).
    EXPECT_TRUE(dlg.AddItem(/*relPos=*/0, MakeOpaqueIcon()));
}

TEST(CPetWearedExDialogTest, AddDeleteIndependenceBetweenCells) {
    cPetWearedExDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetCellNum(3);

    EXPECT_TRUE(dlg.AddItem(/*relPos=*/0, MakeOpaqueIcon()));
    EXPECT_TRUE(dlg.AddItem(/*relPos=*/2, MakeOpaqueIconAlt()));

    // Delete cell 0, cell 2 still occupied.
    cIcon* out = nullptr;
    EXPECT_TRUE(dlg.DeleteItem(/*relPos=*/0, &out));
    EXPECT_EQ(out, MakeOpaqueIcon());

    // Cell 2 still can't be re-added (it's occupied).
    EXPECT_FALSE(dlg.AddItem(/*relPos=*/2, MakeOpaqueIcon()));
    // Cell 0 can be re-added (it's now empty).
    EXPECT_TRUE(dlg.AddItem(/*relPos=*/0, MakeOpaqueIcon()));
}

// ===========================================================================
// GetBlankPositionRestrictRef (1:1 with legacy)
// ===========================================================================

TEST(CPetWearedExDialogTest, GetBlankPositionRestrictRefEmptyDialogReturnsFalse) {
    // 1:1 with legacy: when no cells are configured,
    // IsAddable(i) returns false for all i in [0, 3),
    // so the loop exits and the method returns FALSE.
    cPetWearedExDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    // No SetCellNum call — 0 cells configured.
    dlg.SetCellNum(0);

    std::uint16_t absPos = 0;
    EXPECT_FALSE(dlg.GetBlankPositionRestrictRef(absPos));
}

TEST(CPetWearedExDialogTest, GetBlankPositionRestrictRefAllOccupiedReturnsFalse) {
    // 1:1 with legacy: when all 3 cells are occupied,
    // the loop exits without finding an addable cell
    // and returns FALSE.
    cPetWearedExDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetCellNum(3);
    for (std::uint16_t i = 0; i < 3; ++i) {
        ASSERT_TRUE(dlg.AddItem(i, MakeOpaqueIcon()));
    }

    std::uint16_t absPos = 0;
    EXPECT_FALSE(dlg.GetBlankPositionRestrictRef(absPos));
}

TEST(CPetWearedExDialogTest, GetBlankPositionRestrictRefEmptyDialogReturnsFirstCell) {
    // 1:1 with legacy: when all cells are empty,
    // IsAddable(0) returns true and the method
    // returns absPos = kTpPetWearStart + 0 = 490.
    cPetWearedExDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetCellNum(3);

    std::uint16_t absPos = 0;
    EXPECT_TRUE(dlg.GetBlankPositionRestrictRef(absPos));
    EXPECT_EQ(absPos, cPetWearedExDialog::kTpPetWearStart + 0u);
    EXPECT_EQ(absPos, 490u);
}

TEST(CPetWearedExDialogTest, GetBlankPositionRestrictRefSkipsOccupied) {
    // 1:1 with legacy: when cell 0 and cell 1 are
    // occupied but cell 2 is empty, the method
    // returns absPos = kTpPetWearStart + 2 = 492.
    cPetWearedExDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetCellNum(3);
    ASSERT_TRUE(dlg.AddItem(/*relPos=*/0, MakeOpaqueIcon()));
    ASSERT_TRUE(dlg.AddItem(/*relPos=*/1, MakeOpaqueIcon()));
    // Cell 2 is empty.

    std::uint16_t absPos = 0;
    EXPECT_TRUE(dlg.GetBlankPositionRestrictRef(absPos));
    EXPECT_EQ(absPos, cPetWearedExDialog::kTpPetWearStart + 2u);
    EXPECT_EQ(absPos, 492u);
}

TEST(CPetWearedExDialogTest, GetBlankPositionRestrictRefReturnsFirstAddable) {
    // 1:1 with legacy scan order: the method returns
    // the FIRST addable cell, not the last. When
    // cell 0 is empty (cells 1 and 2 occupied), the
    // method returns absPos = 490.
    cPetWearedExDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetCellNum(3);
    ASSERT_TRUE(dlg.AddItem(/*relPos=*/1, MakeOpaqueIcon()));
    ASSERT_TRUE(dlg.AddItem(/*relPos=*/2, MakeOpaqueIcon()));
    // Cell 0 is empty.

    std::uint16_t absPos = 0;
    EXPECT_TRUE(dlg.GetBlankPositionRestrictRef(absPos));
    EXPECT_EQ(absPos, cPetWearedExDialog::kTpPetWearStart + 0u);
    EXPECT_EQ(absPos, 490u);
}

// ===========================================================================
// CheckDuplication (TODO until cItem port)
// ===========================================================================

TEST(CPetWearedExDialogTest, CheckDuplicationReturnsFalse) {
    // TODO until cItem port (R-12.x deferred). The
    // legacy would iterate cells and compare
    // CItem::GetItemIdx() to the input. The modern
    // port returns false unconditionally.
    cPetWearedExDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetCellNum(3);
    ASSERT_TRUE(dlg.AddItem(/*relPos=*/0, MakeOpaqueIcon()));
    ASSERT_TRUE(dlg.AddItem(/*relPos=*/1, MakeOpaqueIconAlt()));

    // Any item idx — returns false until cItem port.
    EXPECT_FALSE(dlg.CheckDuplication(/*ItemIdx=*/0u));
    EXPECT_FALSE(dlg.CheckDuplication(/*ItemIdx=*/12345u));
    EXPECT_FALSE(dlg.CheckDuplication(/*ItemIdx=*/0xFFFFFFFFu));
}

TEST(CPetWearedExDialogTest, CheckDuplicationEmptyDialogReturnsFalse) {
    // Empty dialog — legacy would return false
    // (no cells to scan). Modern returns false (TODO
    // marker).
    cPetWearedExDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetCellNum(3);
    // No AddItem calls — all cells empty.

    EXPECT_FALSE(dlg.CheckDuplication(/*ItemIdx=*/42u));
}


// ===========================================================================
// C-Batch-2.51: CheckDuplication host GetItemIdxFn dispatch
// ===========================================================================

namespace {

std::uint32_t TagGetItemIdx(cIcon* icon, void* userData) {
    auto* table = static_cast<std::map<cIcon*, std::uint32_t>*>(userData);
    auto it = table->find(icon);
    if (it == table->end()) return 0xDEADBEEFu;
    return it->second;
}

}  // namespace

TEST(CPetWearedExDialogTest, CheckDuplicationWithHostCallbackFindsMatch) {
    cPetWearedExDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetCellNum(3);
    ASSERT_TRUE(dlg.AddItem(0, MakeOpaqueIcon()));
    ASSERT_TRUE(dlg.AddItem(1, MakeOpaqueIconAlt()));
    std::map<cIcon*, std::uint32_t> tag = {
        {MakeOpaqueIcon(),     0xAAAAAAAAu},
        {MakeOpaqueIconAlt(),  0xBBBBBBBBu},
    };
    dlg.SetItemIdxCallback(&TagGetItemIdx, &tag);
    EXPECT_TRUE(dlg.CheckDuplication(0xAAAAAAAAu));
    EXPECT_TRUE(dlg.CheckDuplication(0xBBBBBBBBu));
}

TEST(CPetWearedExDialogTest, CheckDuplicationReturnsFalseOnNoMatch) {
    cPetWearedExDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetCellNum(3);
    ASSERT_TRUE(dlg.AddItem(0, MakeOpaqueIcon()));
    std::map<cIcon*, std::uint32_t> tag = {
        {MakeOpaqueIcon(), 0xAAAAAAAAu},
    };
    dlg.SetItemIdxCallback(&TagGetItemIdx, &tag);
    EXPECT_FALSE(dlg.CheckDuplication(0x11111111u));
    EXPECT_FALSE(dlg.CheckDuplication(0xFFFFFFFFu));
}

TEST(CPetWearedExDialogTest, CheckDuplicationSkipsNullCells) {
    cPetWearedExDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetCellNum(3);
    ASSERT_TRUE(dlg.AddItem(0, MakeOpaqueIcon()));
    std::map<cIcon*, std::uint32_t> tag = {
        {MakeOpaqueIcon(), 0xAAAAAAAAu},
    };
    dlg.SetItemIdxCallback(&TagGetItemIdx, &tag);
    EXPECT_TRUE(dlg.CheckDuplication(0xAAAAAAAAu));
    EXPECT_FALSE(dlg.CheckDuplication(0xDEADBEEFu));
}

TEST(CPetWearedExDialogTest, CheckDuplicationReturnsTrueOnFirstMatch) {
    cPetWearedExDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetCellNum(3);
    ASSERT_TRUE(dlg.AddItem(0, MakeOpaqueIcon()));
    ASSERT_TRUE(dlg.AddItem(1, MakeOpaqueIcon()));
    ASSERT_TRUE(dlg.AddItem(2, MakeOpaqueIcon()));
    std::map<cIcon*, std::uint32_t> tag = {
        {MakeOpaqueIcon(), 0xCAFEu},
    };
    dlg.SetItemIdxCallback(&TagGetItemIdx, &tag);
    EXPECT_TRUE(dlg.CheckDuplication(0xCAFEu));
}

TEST(CPetWearedExDialogTest, CheckDuplicationCallbackReplacementSwap) {
    cPetWearedExDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetCellNum(3);
    ASSERT_TRUE(dlg.AddItem(0, MakeOpaqueIcon()));
    std::map<cIcon*, std::uint32_t> tagA = {
        {MakeOpaqueIcon(), 0x111u},
    };
    std::map<cIcon*, std::uint32_t> tagB = {
        {MakeOpaqueIcon(), 0x222u},
    };
    dlg.SetItemIdxCallback(&TagGetItemIdx, &tagA);
    EXPECT_TRUE(dlg.CheckDuplication(0x111u));
    dlg.SetItemIdxCallback(&TagGetItemIdx, &tagB);
    EXPECT_FALSE(dlg.CheckDuplication(0x111u));
    EXPECT_TRUE(dlg.CheckDuplication(0x222u));
}

TEST(CPetWearedExDialogTest, CheckDuplicationCallbacksAllowNullUserData) {
    cPetWearedExDialog dlg;
    dlg.Init(0, 0, 400, 400, nullptr, 0);
    dlg.SetCellNum(3);
    ASSERT_TRUE(dlg.AddItem(0, MakeOpaqueIcon()));
    auto getIdx = [](cIcon*, void*) -> std::uint32_t { return 0xCAFEu; };
    dlg.SetItemIdxCallback(getIdx);
    EXPECT_TRUE(dlg.CheckDuplication(0xCAFEu));
}
}  // namespace mxh::ui::test
