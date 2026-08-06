// extra_slot_count_test.cpp - 1:1 data-plane tests for the
// legacy CInventorySlot::SetExtraSlotCount / CPyogukSlot::SetExtraSlotCount
// cell-count arithmetic from [Server]Map/InventorySlot.cpp +
// PyogukSlot.cpp.

#include <mxh/server/extra_slot_count.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(ExtraSlotCount, BaseInventoryNoExtensionIs40) {
    // legacy: 20 cells/tab * (2 given + 0 extra) = 40
    EXPECT_EQ(compute_inventory_slot_total(0), 40u);
}

TEST(ExtraSlotCount, InventoryExtra1Adds20Cells) {
    // 20 * (2 + 1) = 60
    EXPECT_EQ(compute_inventory_slot_total(1), 60u);
}

TEST(ExtraSlotCount, InventoryExtra2Adds40Cells) {
    // 20 * (2 + 2) = 80 (matches legacy SLOT_INVENTORY_NUM = 80)
    EXPECT_EQ(compute_inventory_slot_total(2), 80u);
}

TEST(ExtraSlotCount, PyogukDefaultNoExtensionIs60) {
    // 30 * (2 + 0) = 60
    EXPECT_EQ(compute_pyoguk_slot_total_default(0), 60u);
}

TEST(ExtraSlotCount, PyogukJpNoExtensionIs90) {
    // JP legacy GIVEN_PYOGUK_SLOT = 3; 30 * 3 = 90
    EXPECT_EQ(compute_pyoguk_slot_total_jp(0), 90u);
}

TEST(ExtraSlotCount, PyogukJpExtra1Adds30Cells) {
    // 30 * (3 + 1) = 120
    EXPECT_EQ(compute_pyoguk_slot_total_jp(1), 120u);

}

TEST(ExtraSlotCount, PyogukJpExtra5Gives150) {
    // 30 * (3 + 2) = 150 (matches legacy SLOT_PYOGUK_NUM = 150)
    EXPECT_EQ(compute_pyoguk_slot_total_jp(2), 150u);
}

TEST(ExtraSlotCount, GenericExtraSlotTotalArithmetic) {
    // Generic helper: 25 cells * (5 + 3) = 200
    EXPECT_EQ(compute_extra_slot_total(25, 5, 3), 200u);
}

TEST(ExtraSlotCount, ZeroExtraZeroGivenIsZero) {
    EXPECT_EQ(compute_extra_slot_total(20, 0, 0), 0u);
}

TEST(ExtraSlotCount, SaturatesAt32BitOnHugeExtra) {
    // Legacy uses DWORD (32-bit); verify wrap behaviour matches.
    // 20 * (2 + 0xFFFFFFFF) = 20 * 0x00000001 = 20 (with 32-bit wrap
    EXPECT_EQ(compute_inventory_slot_total(0xFFFFFFFFu), 20u);
}