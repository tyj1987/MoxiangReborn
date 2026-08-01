#include "mxh/server/item_slot.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace {

using mxh::game::ItemBase;
using mxh::server::InventoryItemSlot;
using mxh::server::ItemError;
using mxh::server::ItemSlot;
using mxh::server::SlotInfo;

ItemBase make_item(std::uint32_t db_idx,
                   std::uint16_t icon_idx,
                   std::uint16_t position = 0) {
    ItemBase item{};
    item.dwDBIdx = db_idx;
    item.wIconIdx = icon_idx;
    item.Position = position;
    item.QuickPosition = 7;
    item.Durability = 15;
    item.RareIdx = 9;
    item.ItemParam = 0x12345678u;
    return item;
}

class ItemSlotTest : public ::testing::Test {
protected:
    void SetUp() override {
        ASSERT_TRUE(slot.init(10, 4, items, slot_info));
    }

    std::array<ItemBase, 600> items{};
    std::array<SlotInfo, 600> slot_info{};
    ItemSlot slot;
};

}  // namespace

TEST(ItemSlotConstants, NumericValuesMatchLegacy) {
    EXPECT_EQ(mxh::server::UB_DBIDX, 1u);
    EXPECT_EQ(mxh::server::UB_ICONIDX, 2u);
    EXPECT_EQ(mxh::server::UB_ABSPOS, 4u);
    EXPECT_EQ(mxh::server::UB_QABSPOS, 8u);
    EXPECT_EQ(mxh::server::UB_DURA, 16u);
    EXPECT_EQ(mxh::server::UB_RARE, 32u);
    EXPECT_EQ(mxh::server::UB_ALL, 63u);
    EXPECT_EQ(mxh::server::SS_NONE, 0u);
    EXPECT_EQ(mxh::server::SS_PREINSERT, 1u);
    EXPECT_EQ(mxh::server::SS_LOCKOMIT, 2u);
    EXPECT_EQ(mxh::server::SS_CHKDBIDX, 4u);
    EXPECT_EQ(static_cast<unsigned>(ItemError::Success), 0u);
    EXPECT_EQ(static_cast<unsigned>(ItemError::OutOfPosition), 1u);
    EXPECT_EQ(static_cast<unsigned>(ItemError::DataMismatch), 2u);
    EXPECT_EQ(static_cast<unsigned>(ItemError::AlreadyExists), 3u);
    EXPECT_EQ(static_cast<unsigned>(ItemError::NotFound), 4u);
    EXPECT_EQ(static_cast<unsigned>(ItemError::Locked), 5u);
    EXPECT_EQ(static_cast<unsigned>(ItemError::Password), 6u);
    EXPECT_EQ(static_cast<unsigned>(ItemError::NotEnoughMoney), 7u);
    EXPECT_EQ(static_cast<unsigned>(ItemError::NoSpace), 8u);
    EXPECT_EQ(static_cast<unsigned>(ItemError::MaxMoney), 9u);
}

TEST(ItemSlotConstants, SlotInfoMatchesPackedLegacyLayout) {
    EXPECT_EQ(sizeof(SlotInfo), 8u);
    EXPECT_EQ(offsetof(SlotInfo, bLock), 0u);
    EXPECT_EQ(offsetof(SlotInfo, wPassword), 4u);
    EXPECT_EQ(offsetof(SlotInfo, wState), 6u);
}

TEST(ItemSlotInit, RejectsBackingArraysShorterThanAbsoluteRange) {
    std::array<ItemBase, 12> items{};
    std::array<SlotInfo, 12> slot_info{};
    ItemSlot slot;
    EXPECT_FALSE(slot.init(10, 4, items, slot_info));
}

TEST_F(ItemSlotTest, AbsoluteRangeAndLookupMatchLegacy) {
    EXPECT_EQ(slot.start_position(), 10u);
    EXPECT_EQ(slot.slot_count(), 4u);
    EXPECT_EQ(slot.get_item_info_abs(9), nullptr);
    EXPECT_EQ(slot.get_item_info_abs(14), nullptr);
    EXPECT_EQ(slot.get_item_info_abs(10), &items[10]);
    EXPECT_FALSE(slot.set_lock(9, true));
}

TEST_F(ItemSlotTest, GetAndSetAllUseOnlyConfiguredWindow) {
    std::array<ItemBase, 4> input{};
    std::array<ItemBase, 4> output{};
    for (std::uint16_t index = 0; index < input.size(); ++index) {
        input[index] = make_item(100 + index, 200 + index, index);
    }
    items[9] = make_item(999, 999, 9);
    ASSERT_TRUE(slot.set_item_info_all(input));
    ASSERT_TRUE(slot.get_item_info_all(output));
    EXPECT_EQ(std::memcmp(output.data(), input.data(), sizeof(input)), 0);
    EXPECT_EQ(items[9].dwDBIdx, 999u);
}

TEST_F(ItemSlotTest, GetAndSetAllRejectShortBuffers) {
    std::array<ItemBase, 3> short_buffer{};
    EXPECT_FALSE(slot.set_item_info_all(short_buffer));
    EXPECT_FALSE(slot.get_item_info_all(short_buffer));
}

TEST_F(ItemSlotTest, InsertCoercesInputToAbsolutePosition) {
    auto item = make_item(1, 100, 77);
    EXPECT_EQ(slot.insert_item_abs(10, item), ItemError::Success);
    EXPECT_EQ(item.Position, 10u);
    EXPECT_EQ(items[10].Position, 10u);
}

TEST_F(ItemSlotTest, InsertRejectsOccupiedAfterCoercingInputPosition) {
    items[10] = make_item(1, 100, 10);
    auto replacement = make_item(2, 200, 77);
    EXPECT_EQ(slot.insert_item_abs(10, replacement), ItemError::AlreadyExists);
    EXPECT_EQ(replacement.Position, 10u);
    EXPECT_EQ(items[10].dwDBIdx, 1u);
}

TEST_F(ItemSlotTest, DestinationStateMakesOccupiedSlotInsertable) {
    items[10] = make_item(1, 100, 10);
    slot_info[10].wState = mxh::server::SS_PREINSERT;
    auto replacement = make_item(2, 200, 10);
    EXPECT_EQ(slot.insert_item_abs(10, replacement), ItemError::Success);
    EXPECT_EQ(items[10].dwDBIdx, 2u);
}

TEST_F(ItemSlotTest, InsertRejectsLockedSlot) {
    slot_info[10].bLock = 1;
    auto item = make_item(1, 100, 10);
    EXPECT_EQ(slot.insert_item_abs(10, item), ItemError::Locked);
}

TEST_F(ItemSlotTest, InsertLockOmitAllowsWriteAndClearsLock) {
    slot_info[10].bLock = 1;
    auto item = make_item(1, 100, 10);
    EXPECT_EQ(slot.insert_item_abs(10, item, mxh::server::SS_LOCKOMIT),
              ItemError::Success);
    EXPECT_EQ(slot_info[10].bLock, 0);
    EXPECT_EQ(slot_info[10].wState, mxh::server::SS_NONE);
}

TEST_F(ItemSlotTest, InsertRejectsPasswordEvenWithLockOmit) {
    slot_info[10].bLock = 1;
    slot_info[10].wPassword = 1234;
    auto item = make_item(1, 100, 10);
    EXPECT_EQ(slot.insert_item_abs(10, item, mxh::server::SS_LOCKOMIT),
              ItemError::Password);
}

TEST_F(ItemSlotTest, InsertPreInsertBypassesDuplicateLimitCheck) {
    ASSERT_TRUE(slot.init(10, 4, items, slot_info,
                          [](std::uint16_t icon_idx) { return icon_idx == 700; }));
    items[10].wIconIdx = 700;
    items[10].Durability = mxh::server::MAX_YOUNGYAKITEM_DUPNUM + 1;
    auto item = make_item(1, 100, 10);
    EXPECT_EQ(slot.insert_item_abs(10, item, mxh::server::SS_PREINSERT),
              ItemError::Success);
}

TEST_F(ItemSlotTest, InsertDuplicateCheckReadsDestinationBeforeCopy) {
    ASSERT_TRUE(slot.init(10, 4, items, slot_info,
                          [](std::uint16_t icon_idx) { return icon_idx == 700; }));
    items[10].wIconIdx = 700;
    items[10].Durability = mxh::server::MAX_YOUNGYAKITEM_DUPNUM + 1;
    auto item = make_item(1, 100, 10);
    EXPECT_EQ(slot.insert_item_abs(10, item), ItemError::DataMismatch);
    EXPECT_EQ(items[10].dwDBIdx, 0u);
    EXPECT_EQ(items[10].wIconIdx, 700u);
}

TEST_F(ItemSlotTest, InsertStoresStateWithoutLockOmitBit) {
    auto item = make_item(1, 100, 10);
    const auto state = static_cast<std::uint16_t>(
        mxh::server::SS_PREINSERT | mxh::server::SS_LOCKOMIT |
        mxh::server::SS_CHKDBIDX);
    EXPECT_EQ(slot.insert_item_abs(10, item, state), ItemError::Success);
    EXPECT_EQ(slot_info[10].wState,
              mxh::server::SS_PREINSERT | mxh::server::SS_CHKDBIDX);
}

TEST_F(ItemSlotTest, UpdateRejectsOutOfRange) {
    EXPECT_EQ(slot.update_item_abs(9, 1, 2, 9, 3, 4),
              ItemError::OutOfPosition);
}

TEST_F(ItemSlotTest, UpdateCoercesAbsolutePositionAndIgnoresDbIdxFlag) {
    items[10] = make_item(111, 100, 10);
    const auto flags = static_cast<std::uint16_t>(
        mxh::server::UB_DBIDX | mxh::server::UB_ABSPOS);
    EXPECT_EQ(slot.update_item_abs(10, 999, 200, 77, 8, 30, flags),
              ItemError::Success);
    EXPECT_EQ(items[10].dwDBIdx, 111u);
    EXPECT_EQ(items[10].Position, 10u);
}

TEST_F(ItemSlotTest, UpdateRejectsLockedSlot) {
    items[10] = make_item(1, 100, 10);
    slot_info[10].bLock = 1;
    EXPECT_EQ(slot.update_item_abs(10, 1, 200, 10, 8, 30),
              ItemError::Locked);
}

TEST_F(ItemSlotTest, UpdateLockOmitAllowsWriteAndClearsLock) {
    items[10] = make_item(1, 100, 10);
    slot_info[10].bLock = 1;
    EXPECT_EQ(slot.update_item_abs(10, 1, 200, 10, 8, 30,
                                   mxh::server::UB_ALL,
                                   mxh::server::SS_LOCKOMIT),
              ItemError::Success);
    EXPECT_EQ(slot_info[10].bLock, 0);
}

TEST_F(ItemSlotTest, UpdateCheckDbIdxRejectsMismatchBeforeMutation) {
    items[10] = make_item(1, 100, 10);
    EXPECT_EQ(slot.update_item_abs(10, 2, 200, 10, 8, 30,
                                   mxh::server::UB_ALL,
                                   mxh::server::SS_CHKDBIDX),
              ItemError::DataMismatch);
    EXPECT_EQ(items[10].wIconIdx, 100u);
}

TEST_F(ItemSlotTest, UpdateRejectsPassword) {
    items[10] = make_item(1, 100, 10);
    slot_info[10].wPassword = 99;
    EXPECT_EQ(slot.update_item_abs(10, 1, 200, 10, 8, 30),
              ItemError::Password);
}

TEST_F(ItemSlotTest, UpdateAppliesOnlySelectedFields) {
    items[10] = make_item(1, 100, 10);
    const auto old_parameter = items[10].ItemParam;
    const auto flags = static_cast<std::uint16_t>(
        mxh::server::UB_ICONIDX | mxh::server::UB_QABSPOS |
        mxh::server::UB_DURA | mxh::server::UB_RARE);
    EXPECT_EQ(slot.update_item_abs(10, 999, 200, 77, 8, 30,
                                   flags, mxh::server::SS_NONE, 55),
              ItemError::Success);
    EXPECT_EQ(items[10].dwDBIdx, 1u);
    EXPECT_EQ(items[10].wIconIdx, 200u);
    EXPECT_EQ(items[10].Position, 10u);
    EXPECT_EQ(items[10].QuickPosition, 8u);
    EXPECT_EQ(items[10].Durability, 30u);
    EXPECT_EQ(items[10].RareIdx, 55u);
    EXPECT_EQ(items[10].ItemParam, old_parameter);
}

TEST_F(ItemSlotTest, UpdateDuplicateFailureKeepsEarlierPartialWrites) {
    ASSERT_TRUE(slot.init(10, 4, items, slot_info,
                          [](std::uint16_t icon_idx) { return icon_idx == 200; }));
    items[10] = make_item(1, 100, 10);
    items[10].Durability = mxh::server::MAX_YOUNGYAKITEM_DUPNUM + 1;
    const auto flags = static_cast<std::uint16_t>(
        mxh::server::UB_ICONIDX | mxh::server::UB_QABSPOS |
        mxh::server::UB_DURA);
    EXPECT_EQ(slot.update_item_abs(10, 1, 200, 10, 8, 5, flags),
              ItemError::DataMismatch);
    EXPECT_EQ(items[10].wIconIdx, 200u);
    EXPECT_EQ(items[10].QuickPosition, 8u);
    EXPECT_EQ(items[10].Durability,
              mxh::server::MAX_YOUNGYAKITEM_DUPNUM + 1);
}

TEST_F(ItemSlotTest, UpdateStripsLockOmitAndCheckDbStateBits) {
    items[10] = make_item(1, 100, 10);
    const auto state = static_cast<std::uint16_t>(
        mxh::server::SS_PREINSERT | mxh::server::SS_LOCKOMIT |
        mxh::server::SS_CHKDBIDX);
    EXPECT_EQ(slot.update_item_abs(10, 1, 100, 10, 7, 15,
                                   mxh::server::UB_ALL, state),
              ItemError::Success);
    EXPECT_EQ(slot_info[10].wState, mxh::server::SS_PREINSERT);
}

TEST_F(ItemSlotTest, DeleteRejectsOutOfRangeAndEmptySlots) {
    EXPECT_EQ(slot.delete_item_abs(9), ItemError::OutOfPosition);
    EXPECT_EQ(slot.delete_item_abs(10), ItemError::NotFound);
}

TEST_F(ItemSlotTest, DeleteRejectsLockedRegularSlot) {
    items[10] = make_item(1, 100, 10);
    slot_info[10].bLock = 1;
    EXPECT_EQ(slot.delete_item_abs(10), ItemError::Locked);
}

TEST(ItemSlotDelete, ShopRangesBypassLock) {
    std::array<ItemBase, 600> items{};
    std::array<SlotInfo, 600> slot_info{};
    ItemSlot slot;
    ASSERT_TRUE(slot.init(mxh::game::TP_SHOPITEM_START, 1, items, slot_info));
    items[mxh::game::TP_SHOPITEM_START] =
        make_item(1, 100, mxh::game::TP_SHOPITEM_START);
    slot_info[mxh::game::TP_SHOPITEM_START].bLock = 1;
    EXPECT_EQ(slot.delete_item_abs(mxh::game::TP_SHOPITEM_START),
              ItemError::Success);
}

TEST(ItemSlotDelete, ShopInventoryRangeAlsoBypassesLock) {
    std::array<ItemBase, 600> items{};
    std::array<SlotInfo, 600> slot_info{};
    ItemSlot slot;
    ASSERT_TRUE(slot.init(mxh::game::TP_SHOPINVEN_START, 1, items, slot_info));
    items[mxh::game::TP_SHOPINVEN_START] =
        make_item(1, 100, mxh::game::TP_SHOPINVEN_START);
    slot_info[mxh::game::TP_SHOPINVEN_START].bLock = 1;
    EXPECT_EQ(slot.delete_item_abs(mxh::game::TP_SHOPINVEN_START),
              ItemError::Success);
}

TEST(ItemSlotDelete, PasswordStillBlocksShopRange) {
    std::array<ItemBase, 600> items{};
    std::array<SlotInfo, 600> slot_info{};
    ItemSlot slot;
    ASSERT_TRUE(slot.init(mxh::game::TP_SHOPITEM_START, 1, items, slot_info));
    items[mxh::game::TP_SHOPITEM_START] =
        make_item(1, 100, mxh::game::TP_SHOPITEM_START);
    slot_info[mxh::game::TP_SHOPITEM_START].wPassword = 7;
    EXPECT_EQ(slot.delete_item_abs(mxh::game::TP_SHOPITEM_START),
              ItemError::Password);
}

TEST_F(ItemSlotTest, DeleteReturnsOriginalAndPreservesStaleItemParam) {
    items[10] = make_item(1, 100, 10);
    ItemBase output{};
    ASSERT_EQ(slot.delete_item_abs(10, &output), ItemError::Success);
    EXPECT_EQ(output.dwDBIdx, 1u);
    EXPECT_EQ(output.ItemParam, 0x12345678u);
    EXPECT_EQ(items[10].dwDBIdx, 0u);
    EXPECT_EQ(items[10].wIconIdx, 0u);
    EXPECT_EQ(items[10].Position, 0u);
    EXPECT_EQ(items[10].QuickPosition, 0u);
    EXPECT_EQ(items[10].Durability, 0u);
    EXPECT_EQ(items[10].RareIdx, 0u);
    EXPECT_EQ(items[10].ItemParam, 0x12345678u);
}

TEST_F(ItemSlotTest, DeleteClearsAllSlotMetadata) {
    items[10] = make_item(1, 100, 10);
    slot_info[10].bLock = 1;
    slot_info[10].wPassword = 3;
    slot_info[10].wState = mxh::server::SS_NONE;
    ASSERT_EQ(slot.delete_item_abs(10, nullptr, mxh::server::SS_LOCKOMIT),
              ItemError::Password);
    slot_info[10].wPassword = 0;
    ASSERT_EQ(slot.delete_item_abs(10, nullptr, mxh::server::SS_LOCKOMIT),
              ItemError::Success);
    EXPECT_EQ(slot_info[10].bLock, 0);
    EXPECT_EQ(slot_info[10].wPassword, 0u);
    EXPECT_EQ(slot_info[10].wState, 0u);
}

TEST_F(ItemSlotTest, EmptyRequiresNoLockNoStateAndZeroDbIdx) {
    EXPECT_TRUE(slot.is_empty(10));
    slot_info[10].bLock = 1;
    EXPECT_FALSE(slot.is_empty(10));
    slot_info[10].bLock = 0;
    slot_info[10].wState = mxh::server::SS_PREINSERT;
    EXPECT_FALSE(slot.is_empty(10));
    slot_info[10].wState = mxh::server::SS_NONE;
    items[10].dwDBIdx = 1;
    EXPECT_FALSE(slot.is_empty(10));
}

TEST_F(ItemSlotTest, ItemCountTreatsLockedOrReservedEmptySlotsAsOccupied) {
    slot_info[10].bLock = 1;
    slot_info[11].wState = mxh::server::SS_PREINSERT;
    items[12] = make_item(1, 100, 12);
    EXPECT_EQ(slot.item_count(), 3u);
}

TEST(InventoryItemSlot, GetEmptyCellZeroNeedReturnsZero) {
    std::array<ItemBase, 20> items{};
    std::array<SlotInfo, 20> slot_info{};
    InventoryItemSlot slot;
    ASSERT_TRUE(slot.init(10, 4, items, slot_info));
    std::array<std::uint16_t, 4> positions{};
    EXPECT_EQ(slot.get_empty_cell(positions.data(), 0), 0u);
}

TEST(InventoryItemSlot, GetEmptyCellUsesAbsolutePositionsAndSkipsUnavailable) {
    std::array<ItemBase, 20> items{};
    std::array<SlotInfo, 20> slot_info{};
    InventoryItemSlot slot;
    ASSERT_TRUE(slot.init(10, 4, items, slot_info));
    slot_info[10].bLock = 1;
    slot_info[11].wState = mxh::server::SS_PREINSERT;
    items[12] = make_item(1, 100, 12);
    std::array<std::uint16_t, 2> positions{};
    EXPECT_EQ(slot.get_empty_cell(positions.data(), 2), 1u);
    EXPECT_EQ(positions[0], 13u);
}

TEST(InventoryItemSlot, NullOutputPreservesLegacyZeroCountQuirk) {
    std::array<ItemBase, 4> items{};
    std::array<SlotInfo, 4> slot_info{};
    InventoryItemSlot slot;
    ASSERT_TRUE(slot.init(0, 4, items, slot_info));
    EXPECT_EQ(slot.get_empty_cell(nullptr, 4), 0u);
}

TEST(InventoryItemSlot, QuickPositionCheckMatchesIconWithoutDbIdxCheck) {
    std::array<ItemBase, 4> items{};
    std::array<SlotInfo, 4> slot_info{};
    InventoryItemSlot slot;
    ASSERT_TRUE(slot.init(0, 4, items, slot_info));
    items[2].wIconIdx = 500;
    items[2].QuickPosition = 9;
    EXPECT_FALSE(slot.check_quick_position_for_item(500));
    EXPECT_TRUE(slot.check_quick_position_for_item(501));
}

TEST(InventoryItemSlot, ItemLockCheckMatchesIconWithoutDbIdxCheck) {
    std::array<ItemBase, 4> items{};
    std::array<SlotInfo, 4> slot_info{};
    InventoryItemSlot slot;
    ASSERT_TRUE(slot.init(0, 4, items, slot_info));
    items[2].wIconIdx = 500;
    slot_info[2].bLock = 1;
    EXPECT_FALSE(slot.check_item_lock_for_item(500));
    EXPECT_TRUE(slot.check_item_lock_for_item(501));
}

TEST(InventoryItemSlot, ExtraSlotCountUsesTwoGivenTwentyCellTabs) {
    std::array<ItemBase, 80> items{};
    std::array<SlotInfo, 80> slot_info{};
    InventoryItemSlot slot;
    ASSERT_TRUE(slot.init(0, 80, items, slot_info));
    ASSERT_TRUE(slot.set_extra_slot_count(0));
    EXPECT_EQ(slot.slot_count(), 40u);
    ASSERT_TRUE(slot.set_extra_slot_count(1));
    EXPECT_EQ(slot.slot_count(), 60u);
    ASSERT_TRUE(slot.set_extra_slot_count(2));
    EXPECT_EQ(slot.slot_count(), 80u);
    EXPECT_EQ(slot.extra_slot_count(), 2u);
}

TEST(InventoryItemSlot, ExtraSlotCountRejectsBeyondBackingStorage) {
    std::array<ItemBase, 80> items{};
    std::array<SlotInfo, 80> slot_info{};
    InventoryItemSlot slot;
    ASSERT_TRUE(slot.init(0, 80, items, slot_info));
    EXPECT_FALSE(slot.set_extra_slot_count(3));
    EXPECT_EQ(slot.slot_count(), 80u);
    EXPECT_EQ(slot.extra_slot_count(), 0u);
}
