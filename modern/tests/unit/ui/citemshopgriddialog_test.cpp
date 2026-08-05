// citemshopgriddialog_test.cpp - 1:1 lock + behavior tests for the modern
// cItemShopGridDialog (1:1 port of legacy CItemShopGridDialog).
//
// The legacy CItemShopGridDialog is a cIconGridDialog-derived tabbed
// per-cell shop grid with TabNumber + modular GetRelativePosition +
// FakeMoveItem / FakeGeneralItemMove / CanBeMoved guard + a host-injected
// SendMoveSynFn callback in place of legacy NETWORK->Send. The tests lock
// these surfaces 1:1 against the modern port.

#include "citemshopgriddialog.hpp"
#include "cIconGridDialog.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <type_traits>
#include <vector>

namespace {

// Helper to make opaque cIcon* without depending on real cIcon impl.
mxh::ui::cIcon* MakeIcon(void* tag) {
    return reinterpret_cast<mxh::ui::cIcon*>(tag);
}

mxh::ui::cIcon* MakeIconN(std::uintptr_t i) {
    return reinterpret_cast<mxh::ui::cIcon*>(reinterpret_cast<void*>(i));
}

}  // namespace

// ----------------------------------------------------------------------
// Inheritance + construction
// ----------------------------------------------------------------------
TEST(CItemShopGridDialogTest, InheritsCIconGridDialog) {
    mxh::ui::cItemShopGridDialog d;
    mxh::ui::cIconGridDialog* base = &d;
    EXPECT_NE(base, nullptr);
    EXPECT_EQ(d.GetTabNumber(), 0u);
}

TEST(CItemShopGridDialogTest, ConstantsMatchLegacy) {
    EXPECT_EQ(mxh::ui::cItemShopGridDialog::kShopTabCount, 5u);
    EXPECT_EQ(mxh::ui::cItemShopGridDialog::kShopCellPerTab, 30u);
    EXPECT_EQ(mxh::ui::cItemShopGridDialog::IG_SHOPITEM_MAXINDEX, 999u);
}

TEST(CItemShopGridDialogTest, DestructorIsSafe) {
    {
        mxh::ui::cItemShopGridDialog d;
        d.Init(0, 0, 200, 200, nullptr, 1);
        d.AddItem(MakeIconN(1));
    }
    EXPECT_TRUE(true);
}

TEST(CItemShopGridDialogTest, CannotCopy) {
    EXPECT_FALSE(std::is_copy_constructible_v<mxh::ui::cItemShopGridDialog>);
    EXPECT_FALSE(std::is_copy_assignable_v<mxh::ui::cItemShopGridDialog>);
}

// ----------------------------------------------------------------------
// Init
// ----------------------------------------------------------------------
TEST(CItemShopGridDialogTest, InitAllocatesCells) {
    mxh::ui::cItemShopGridDialog d;
    EXPECT_EQ(d.CellCount(), 0u);
    d.Init(10, 20, 200, 200, nullptr, 0);
    EXPECT_EQ(d.CellCount(), 150u);
    EXPECT_EQ(d.GetTabNumber(), 0u);
}

TEST(CItemShopGridDialogTest, InitResetsTabNumber) {
    mxh::ui::cItemShopGridDialog d;
    d.SetTabNumber(3);
    d.Init(0, 0, 200, 200, nullptr, 0);
    EXPECT_EQ(d.GetTabNumber(), 0u);
}

// ----------------------------------------------------------------------
// TabNumber setter/getter (1:1 with legacy inline SetTabNumber)
// ----------------------------------------------------------------------
TEST(CItemShopGridDialogTest, SetTabNumberStoresValue) {
    mxh::ui::cItemShopGridDialog d;
    d.Init(0, 0, 200, 200, nullptr, 0);
    d.SetTabNumber(2);
    EXPECT_EQ(d.GetTabNumber(), 2u);
    d.SetTabNumber(4);
    EXPECT_EQ(d.GetTabNumber(), 4u);
}

// ----------------------------------------------------------------------
// GetRelativePosition (1:1 with legacy modular math)
// ----------------------------------------------------------------------
TEST(CItemShopGridDialogTest, GetRelativePositionModular) {
    // 1:1 with legacy CItemShopGridDialog::GetRelativePosition:
    //     return (absPos - TP_SHOPITEM_START) % TABCELL_SHOPITEM_NUM
    using mxh::ui::cItemShopGridDialog;
    using mxh::game::TP_SHOPITEM_START;
    const std::uint16_t start = TP_SHOPITEM_START;
    EXPECT_EQ(cItemShopGridDialog::GetRelativePosition(start + 0),  0u);
    EXPECT_EQ(cItemShopGridDialog::GetRelativePosition(start + 29), 29u);
    EXPECT_EQ(cItemShopGridDialog::GetRelativePosition(start + 30), 0u);
    EXPECT_EQ(cItemShopGridDialog::GetRelativePosition(start + 31), 1u);
    EXPECT_EQ(cItemShopGridDialog::GetRelativePosition(start + 60), 0u);
}

TEST(CItemShopGridDialogTest, GetRelativePositionClampsBelowStart) {
    // absPos < TP_SHOPITEM_START returns 0 (clamped).
    EXPECT_EQ(mxh::ui::cItemShopGridDialog::GetRelativePosition(0), 0u);
}

// ----------------------------------------------------------------------
// AddItem / DeleteItem / ShopItemDelete
// ----------------------------------------------------------------------
TEST(CItemShopGridDialogTest, AddItemFillsFirstFreeCell) {
    mxh::ui::cItemShopGridDialog d;
    d.Init(0, 0, 200, 200, nullptr, 0);
    EXPECT_TRUE(d.AddItem(MakeIconN(1)));
    EXPECT_TRUE(d.AddItem(MakeIconN(2)));
    EXPECT_EQ(d.GetIconForIdx(0), MakeIconN(1));
    EXPECT_EQ(d.GetIconForIdx(1), MakeIconN(2));
}

TEST(CItemShopGridDialogTest, AddItemRejectsNull) {
    mxh::ui::cItemShopGridDialog d;
    d.Init(0, 0, 200, 200, nullptr, 0);
    EXPECT_FALSE(d.AddItem(nullptr));
}

TEST(CItemShopGridDialogTest, AddItemBeforeInitIsFalse) {
    mxh::ui::cItemShopGridDialog d;
    EXPECT_EQ(d.CellCount(), 0u);
    EXPECT_FALSE(d.AddItem(MakeIconN(7)));
}

TEST(CItemShopGridDialogTest, AddItemSaturatesWhenFull) {
    mxh::ui::cItemShopGridDialog d;
    d.Init(0, 0, 200, 200, nullptr, 0);
    for (std::uint16_t i = 0; i < 150; ++i) {
        EXPECT_TRUE(d.AddItem(MakeIconN(i + 1))) << "i=" << i;
    }
    EXPECT_FALSE(d.AddItem(MakeIconN(999)));
}

TEST(CItemShopGridDialogTest, DeleteItemReturnsIcon) {
    mxh::ui::cItemShopGridDialog d;
    d.Init(0, 0, 200, 200, nullptr, 0);
    d.AddItem(MakeIconN(42));
    mxh::ui::cIcon* out = nullptr;
    EXPECT_TRUE(d.DeleteItem(0, &out));
    EXPECT_EQ(out, MakeIconN(42));
}

TEST(CItemShopGridDialogTest, DeleteItemEmptyCellReturnsFalse) {
    mxh::ui::cItemShopGridDialog d;
    d.Init(0, 0, 200, 200, nullptr, 0);
    mxh::ui::cIcon* out = nullptr;
    EXPECT_FALSE(d.DeleteItem(0, &out));
}

TEST(CItemShopGridDialogTest, ShopItemDeleteIsNoOp) {
    // 1:1 with legacy empty-body quirk.
    mxh::ui::cItemShopGridDialog d;
    d.Init(0, 0, 200, 200, nullptr, 0);
    EXPECT_NO_THROW(d.ShopItemDelete(123, 456, 789));
}

// ----------------------------------------------------------------------
// CanBeMoved gate (1:1 with legacy range gate)
// ----------------------------------------------------------------------
TEST(CItemShopGridDialogTest, CanBeMovedRequiresShopRange) {
    // 1:1 with legacy CanBeMoved: pos must be in [TP_SHOPITEM_START, TP_SHOPINVEN_END).
    mxh::ui::cItemShopGridDialog d;
    const std::uint16_t start = mxh::game::TP_SHOPITEM_START;
    const std::uint16_t end   = mxh::game::TP_SHOPINVEN_END;
    EXPECT_FALSE(d.CanBeMoved(MakeIconN(1), 0));
    EXPECT_FALSE(d.CanBeMoved(MakeIconN(1), start - 1));
    EXPECT_TRUE(d.CanBeMoved(MakeIconN(1), start));
    EXPECT_TRUE(d.CanBeMoved(MakeIconN(1), start + 50));
    EXPECT_TRUE(d.CanBeMoved(MakeIconN(1), end - 1));
    EXPECT_FALSE(d.CanBeMoved(MakeIconN(1), end));
    EXPECT_FALSE(d.CanBeMoved(MakeIconN(1), 65535));
}

// ----------------------------------------------------------------------
// FakeMoveItem / FakeGeneralItemMove (host callback dispatch)
// ----------------------------------------------------------------------
TEST(CItemShopGridDialogTest, FakeGeneralItemMoveRejectsWithoutCallback) {
    mxh::ui::cItemShopGridDialog d;
    d.Init(0, 0, 200, 200, nullptr, 0);
    EXPECT_FALSE(d.FakeGeneralItemMove(mxh::game::TP_SHOPITEM_START, MakeIconN(1), nullptr));
}

TEST(CItemShopGridDialogTest, FakeGeneralItemMoveDispatchesCallback) {
    mxh::ui::cItemShopGridDialog d;
    d.Init(0, 0, 200, 200, nullptr, 0);
    bool called = false;
    int  callCount = 0;
    d.SetSendMoveSynFn([&](const mxh::ui::ItemShopGridMovePayload& p) {
        called = true;
        ++callCount;
        (void)p;
        return true;
    });
    EXPECT_TRUE(d.FakeGeneralItemMove(mxh::game::TP_SHOPITEM_START + 5, MakeIconN(7), nullptr));
    EXPECT_TRUE(called);
    EXPECT_EQ(callCount, 1);
}

TEST(CItemShopGridDialogTest, FakeGeneralItemMoveAbortsOnCanBeMovedFalse) {
    // Out-of-range pos triggers CanBeMoved = false, which short-circuits the move.
    mxh::ui::cItemShopGridDialog d;
    d.Init(0, 0, 200, 200, nullptr, 0);
    int calls = 0;
    d.SetSendMoveSynFn([&](const mxh::ui::ItemShopGridMovePayload&) {
        ++calls;
        return true;
    });
    EXPECT_FALSE(d.FakeGeneralItemMove(0, MakeIconN(1), nullptr));
    EXPECT_FALSE(d.FakeGeneralItemMove(65535, MakeIconN(1), nullptr));
    EXPECT_EQ(calls, 0);
}

TEST(CItemShopGridDialogTest, FakeGeneralItemMoveCallbackAcksReturn) {
    mxh::ui::cItemShopGridDialog d;
    d.Init(0, 0, 200, 200, nullptr, 0);
    d.SetSendMoveSynFn([&](const mxh::ui::ItemShopGridMovePayload&) { return false; });
    EXPECT_FALSE(d.FakeGeneralItemMove(mxh::game::TP_SHOPITEM_START + 5, MakeIconN(1), nullptr));
}

TEST(CItemShopGridDialogTest, FakeMoveItemRejectsNullSource) {
    mxh::ui::cItemShopGridDialog d;
    d.Init(0, 0, 200, 200, nullptr, 0);
    EXPECT_FALSE(d.FakeMoveItem(0, 0, nullptr));
    d.SetSendMoveSynFn([&](const mxh::ui::ItemShopGridMovePayload&) { return true; });
    EXPECT_FALSE(d.FakeMoveItem(0, 0, nullptr));
}

// ----------------------------------------------------------------------
// GetItemForPos range gate (1:1 with legacy GetItemForPos body)
// ----------------------------------------------------------------------
TEST(CItemShopGridDialogTest, GetItemForPosOutOfRangeReturnsNull) {
    mxh::ui::cItemShopGridDialog d;
    d.Init(0, 0, 200, 200, nullptr, 0);
    EXPECT_EQ(d.GetItemForPos(mxh::game::TP_SHOPITEM_START - 1), nullptr);
    EXPECT_EQ(d.GetItemForPos(mxh::game::TP_SHOPITEM_END), nullptr);
}

TEST(CItemShopGridDialogTest, GetItemForPosInRangeReturnsCellIcon) {
    mxh::ui::cItemShopGridDialog d;
    d.Init(0, 0, 200, 200, nullptr, 0);
    d.SetTabNumber(0);
    d.AddItem(MakeIconN(8));
    // The first added icon should sit at relPos 0; absolute pos =
    // TP_SHOPITEM_START + 0 for tab 0.
    EXPECT_EQ(d.GetItemForPos(mxh::game::TP_SHOPITEM_START), MakeIconN(8));
}

TEST(CItemShopGridDialogTest, GetItemForPosRespectsTabNumber) {
    // With TabNumber = 2, the cell at absPos = TP_SHOPITEM_START + 30 maps
    // to relPos 0 (= base cell of tab 2).
    mxh::ui::cItemShopGridDialog d;
    d.Init(0, 0, 200, 200, nullptr, 0);
    d.SetTabNumber(2);
    d.AddItem(MakeIconN(11));
    const std::uint16_t absPos = mxh::game::TP_SHOPITEM_START +
                                  mxh::ui::cItemShopGridDialog::kShopCellPerTab * 2;
    EXPECT_EQ(d.GetItemForPos(absPos), MakeIconN(11));
}
