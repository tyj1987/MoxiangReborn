// add_using_shop_item_runtime_test.cpp
//
// Verifies apply_add_using_shop_item() (the runtime orchestrator for
// AddUsingShopItem) applies the data-plane decision to a real
// ShopItemManager. Locks the Ok / KeyZero / AlreadyPresent paths.
//
// Pattern mirrors agent_dispatch.hpp (D4.R1): data plane in header,
// runtime orchestrator also inline in header, tests verify behavior
// through the public surface (ShopItemManager.add_using_item etc).

#include <mxh/server/add_using_shop_item.hpp>
#include <mxh/server/shop_item_manager.hpp>
#include <mxh/game/shop_item_types.hpp>

#include <gtest/gtest.h>

#include <cstdint>

namespace {

using mxh::server::AddUsingShopItemStatus;
using mxh::server::ShopItemManager;
using mxh::server::apply_add_using_shop_item;
using mxh::game::ShopItemWithTime;

ShopItemWithTime make_row(std::uint16_t icon, std::uint32_t param,
                        std::uint32_t remain) {
    ShopItemWithTime row{};
    row.ShopItem.ItemBase.wIconIdx = icon;
    row.ShopItem.Param = param;
    row.ShopItem.Remaintime = remain;
    return row;
}

}  // namespace

// Ok path: orchestrator inserts the row into the table.
TEST(ApplyAddUsingShopItem, OkInsertsRowIntoShopItemManager) {
    ShopItemManager mgr;
    mgr.init(nullptr);
    auto row = make_row(/*icon=*/10, /*param=*/1, /*remain=*/2);
    auto status = apply_add_using_shop_item(mgr, row, /*dw_item_index=*/10);
    EXPECT_EQ(status, AddUsingShopItemStatus::Ok);
    EXPECT_EQ(mgr.using_item_count(), 1u);
    EXPECT_TRUE(mgr.has_using_item(10));
    const auto* entry = mgr.find_using_item(10);
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->Data.ShopItem.ItemBase.wIconIdx, 10u);
    EXPECT_EQ(entry->Data.ShopItem.Param, 1u);
    EXPECT_EQ(entry->Data.ShopItem.Remaintime, 2u);
}

// KeyZero path: orchestrator does not insert.
TEST(ApplyAddUsingShopItem, KeyZeroDoesNotInsert) {
    ShopItemManager mgr;
    mgr.init(nullptr);
    auto row = make_row(/*icon=*/10, /*param=*/1, /*remain=*/2);
    auto status = apply_add_using_shop_item(mgr, row, /*dw_item_index=*/0);
    EXPECT_EQ(status, AddUsingShopItemStatus::KeyZero);
    EXPECT_EQ(mgr.using_item_count(), 0u);
}

// AlreadyPresent path: orchestrator sees the prior insert and rejects.
TEST(ApplyAddUsingShopItem, AlreadyPresentRejectsSecondInsert) {
    ShopItemManager mgr;
    mgr.init(nullptr);
    auto row = make_row(/*icon=*/10, /*param=*/1, /*remain=*/2);
    EXPECT_EQ(apply_add_using_shop_item(mgr, row, 10),
              AddUsingShopItemStatus::Ok);
    // Same key -> second insert must be rejected and table unchanged.
    auto row2 = make_row(/*icon=*/10, /*param=*/99, /*remain=*/99);
    EXPECT_EQ(apply_add_using_shop_item(mgr, row2, 10),
              AddUsingShopItemStatus::AlreadyPresent);
    EXPECT_EQ(mgr.using_item_count(), 1u);
    // The row must still be the original (param=1, remain=2), not the
    // second attempt (param=99, remain=99).
    const auto* entry = mgr.find_using_item(10);
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->Data.ShopItem.Param, 1u);
    EXPECT_EQ(entry->Data.ShopItem.Remaintime, 2u);
}

// Multiple distinct keys coexist in the table.
TEST(ApplyAddUsingShopItem, MultipleDistinctKeysCoexist) {
    ShopItemManager mgr;
    mgr.init(nullptr);
    EXPECT_EQ(apply_add_using_shop_item(mgr, make_row(10, 1, 2), 10),
              AddUsingShopItemStatus::Ok);
    EXPECT_EQ(apply_add_using_shop_item(mgr, make_row(20, 3, 4), 20),
              AddUsingShopItemStatus::Ok);
    EXPECT_EQ(apply_add_using_shop_item(mgr, make_row(30, 5, 6), 30),
              AddUsingShopItemStatus::Ok);
    EXPECT_EQ(mgr.using_item_count(), 3u);
    EXPECT_TRUE(mgr.has_using_item(10));
    EXPECT_TRUE(mgr.has_using_item(20));
    EXPECT_TRUE(mgr.has_using_item(30));
}

// Key may differ from wIconIdx (legacy AddShopItem sometimes keys by
// dwDBIdx even when wIconIdx is a different value).
TEST(ApplyAddUsingShopItem, KeyMayDifferFromIcon) {
    ShopItemManager mgr;
    mgr.init(nullptr);
    auto row = make_row(/*icon=*/10, /*param=*/1, /*remain=*/2);
    EXPECT_EQ(apply_add_using_shop_item(mgr, row, /*dw_item_index=*/42),
              AddUsingShopItemStatus::Ok);
    EXPECT_TRUE(mgr.has_using_item(42));
    const auto* entry = mgr.find_using_item(42);
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->ItemIdx, 42u);
    EXPECT_EQ(entry->Data.ShopItem.ItemBase.wIconIdx, 10u);
}

// Uninitialized manager: orchestrator still works because add_using_item
// does not touch the player pointer. (The init() call is purely for
// the table-size invariants; using_item_count starts at 0 either way.)
TEST(ApplyAddUsingShopItem, WorksOnUninitializedManager) {
    ShopItemManager mgr;
    // Intentionally skip init(); the using-items table is a plain map
    // and works without a player pointer.
    auto row = make_row(/*icon=*/10, /*param=*/1, /*remain=*/2);
    EXPECT_EQ(apply_add_using_shop_item(mgr, row, 10),
              AddUsingShopItemStatus::Ok);
    EXPECT_EQ(mgr.using_item_count(), 1u);
}