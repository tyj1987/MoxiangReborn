// D4.34 AddUsingShopItem data-plane tests.

#include <mxh/server/add_using_shop_item.hpp>

#include <gtest/gtest.h>

#include <cstdint>

using namespace mxh::server;
using namespace mxh::game;

namespace {

ShopItemWithTime make_row(std::uint16_t icon, std::uint32_t param,
                          std::uint32_t remain) {
    ShopItemWithTime row{};
    row.ShopItem.ItemBase.wIconIdx = icon;
    row.ShopItem.Param = param;
    row.ShopItem.Remaintime = remain;
    return row;
}

}  // namespace

TEST(AddUsingShopItem, ZeroKeyIsRejected) {
    auto row = make_row(/*icon=*/0, /*param=*/1, /*remain=*/2);
    auto out = add_using_shop_item_decision(
        row, /*dw_item_index=*/0, /*already_present=*/false);
    EXPECT_EQ(out.status, AddUsingShopItemStatus::KeyZero);
}

TEST(AddUsingShopItem, AlreadyPresentIsRejected) {
    auto row = make_row(/*icon=*/10, /*param=*/1, /*remain=*/2);
    auto out = add_using_shop_item_decision(
        row, /*dw_item_index=*/10, /*already_present=*/true);
    EXPECT_EQ(out.status, AddUsingShopItemStatus::AlreadyPresent);
}

TEST(AddUsingShopItem, NewRowIsAcceptedAndKeyRecorded) {
    auto row = make_row(/*icon=*/10, /*param=*/1, /*remain=*/2);
    auto out = add_using_shop_item_decision(
        row, /*dw_item_index=*/10, /*already_present=*/false);
    EXPECT_EQ(out.status, AddUsingShopItemStatus::Ok);
    EXPECT_EQ(out.entry.item_idx, 10u);
    EXPECT_EQ(out.entry.data.ShopItem.ItemBase.wIconIdx, 10u);
    EXPECT_EQ(out.entry.data.ShopItem.Param, 1u);
    EXPECT_EQ(out.entry.data.ShopItem.Remaintime, 2u);
}

TEST(AddUsingShopItem, KeyMayDifferFromIcon) {
    auto row = make_row(/*icon=*/10, /*param=*/1, /*remain=*/2);
    auto out = add_using_shop_item_decision(
        row, /*dw_item_index=*/42, /*already_present=*/false);
    EXPECT_EQ(out.status, AddUsingShopItemStatus::Ok);
    EXPECT_EQ(out.entry.item_idx, 42u);
    EXPECT_EQ(out.entry.data.ShopItem.ItemBase.wIconIdx, 10u);
}
