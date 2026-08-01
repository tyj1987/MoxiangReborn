#include "mxh/server/pyo_guk_manager.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>

namespace mxh::server {
namespace {

PyoGukManager make_manager() {
    PyoGukManager manager;
    for (std::size_t level = 1; level <= PYOGUK_LIST_COUNT; ++level) {
        manager.configure_level(
            level,
            static_cast<MoneyType>(level * 1000),
            static_cast<MoneyType>(level * 100));
    }
    return manager;
}

TEST(PyoGukConstants, LegacyPageAndSlotCounts) {
    EXPECT_EQ(PYOGUK_LIST_COUNT, 5u);
    EXPECT_EQ(PYOGUK_CELLS_PER_PAGE, 30u);
    EXPECT_EQ(PYOGUK_SLOT_COUNT, 150u);
}

TEST(PyoGukConstants, MoneyTypeRemainsDword) {
    EXPECT_EQ(sizeof(MoneyType), 4u);
    EXPECT_EQ(sizeof(PyoGukListInfo), 12u);
    EXPECT_EQ(PURSE_UNLIMITED, std::numeric_limits<std::uint32_t>::max());
}

TEST(PyoGukConfig, DefaultCellCountsGrowByThirty) {
    PyoGukManager manager;
    for (std::size_t level = 1; level <= PYOGUK_LIST_COUNT; ++level) {
        ASSERT_NE(manager.info_for(level), nullptr);
        EXPECT_EQ(manager.info_for(level)->max_cell_num, level * 30u);
    }
}

TEST(PyoGukConfig, ConfigureStoresMoneyAndPrice) {
    PyoGukManager manager;
    ASSERT_TRUE(manager.configure_level(3, 9000, 700));
    const auto* info = manager.info_for(3);
    ASSERT_NE(info, nullptr);
    EXPECT_EQ(info->max_cell_num, 90u);
    EXPECT_EQ(info->max_money, 9000u);
    EXPECT_EQ(info->buy_price, 700u);
}

TEST(PyoGukConfig, RejectsOutOfRangeLevels) {
    PyoGukManager manager;
    EXPECT_FALSE(manager.configure_level(0, 1, 1));
    EXPECT_FALSE(manager.configure_level(6, 1, 1));
    EXPECT_EQ(manager.info_for(0), nullptr);
    EXPECT_EQ(manager.info_for(6), nullptr);
}

TEST(PyoGukBuy, FirstPurchaseInitializesAndCharges) {
    auto manager = make_manager();
    PyoGukAccountState state;
    state.inventory_money = 1000;

    const auto result = manager.buy(state, PyoGukLocale::Korea);

    EXPECT_EQ(result.status, PyoGukPurchaseStatus::Ack);
    EXPECT_EQ(result.page_count, 1u);
    EXPECT_EQ(result.charged, 100u);
    EXPECT_EQ(state.inventory_money, 900u);
    EXPECT_EQ(state.warehouse_max_money, 1000u);
    EXPECT_TRUE(state.item_info_initialized);
}

TEST(PyoGukBuy, InsufficientMoneyNacksWithoutMutation) {
    auto manager = make_manager();
    PyoGukAccountState state;
    state.inventory_money = 99;

    const auto result = manager.buy(state, PyoGukLocale::Korea);

    EXPECT_EQ(result.status, PyoGukPurchaseStatus::Nack);
    EXPECT_EQ(state.page_count, 0u);
    EXPECT_EQ(state.inventory_money, 99u);
    EXPECT_FALSE(state.item_info_initialized);
}

TEST(PyoGukBuy, LaterPurchaseUsesNextLevelPriceAndLimit) {
    auto manager = make_manager();
    PyoGukAccountState state;
    state.page_count = 1;
    state.inventory_money = 1000;
    state.item_info_initialized = true;

    const auto result = manager.buy(state, PyoGukLocale::Korea);

    EXPECT_EQ(result.status, PyoGukPurchaseStatus::Ack);
    EXPECT_EQ(result.charged, 200u);
    EXPECT_EQ(state.page_count, 2u);
    EXPECT_EQ(state.inventory_money, 800u);
    EXPECT_EQ(state.warehouse_max_money, 2000u);
}

TEST(PyoGukBuy, FivePagesIsHardMaximum) {
    auto manager = make_manager();
    PyoGukAccountState state;
    state.page_count = 5;
    state.inventory_money = 5000;
    state.warehouse_max_money = 5000;

    const auto result = manager.buy(state, PyoGukLocale::Korea);

    EXPECT_EQ(result.status, PyoGukPurchaseStatus::Nack);
    EXPECT_EQ(state.page_count, 5u);
    EXPECT_EQ(state.inventory_money, 5000u);
}

TEST(PyoGukBuy, JapanStopsAtThreeGivenPages) {
    auto manager = make_manager();
    PyoGukAccountState state;
    state.page_count = 3;
    state.inventory_money = 5000;

    EXPECT_EQ(manager.buy(state, PyoGukLocale::Japan).status, PyoGukPurchaseStatus::Nack);
}

TEST(PyoGukBuy, JapanExtraSlotAllowsAnotherPage) {
    auto manager = make_manager();
    PyoGukAccountState state;
    state.page_count = 3;
    state.extra_slot_count = 1;
    state.inventory_money = 5000;

    EXPECT_EQ(manager.buy(state, PyoGukLocale::Japan).status, PyoGukPurchaseStatus::Ack);
    EXPECT_EQ(state.page_count, 4u);
}

TEST(PyoGukBuy, HongKongAndTaiwanStopAtTwoGivenPages) {
    auto manager = make_manager();
    PyoGukAccountState hong_kong;
    hong_kong.page_count = 2;
    hong_kong.inventory_money = 5000;
    PyoGukAccountState taiwan = hong_kong;

    EXPECT_EQ(manager.buy(hong_kong, PyoGukLocale::HongKong).status, PyoGukPurchaseStatus::Nack);
    EXPECT_EQ(manager.buy(taiwan, PyoGukLocale::Taiwan).status, PyoGukPurchaseStatus::Nack);
}

TEST(PyoGukBuy, KoreaAndChinaIgnoreGivenPageLimit) {
    auto manager = make_manager();
    PyoGukAccountState korea;
    korea.page_count = 2;
    korea.inventory_money = 5000;
    PyoGukAccountState china = korea;

    EXPECT_EQ(manager.buy(korea, PyoGukLocale::Korea).status, PyoGukPurchaseStatus::Ack);
    EXPECT_EQ(manager.buy(china, PyoGukLocale::China).status, PyoGukPurchaseStatus::Ack);
}

TEST(PyoGukDeposit, TransfersRequestedAmount) {
    auto manager = make_manager();
    PyoGukAccountState state;
    state.inventory_money = 800;
    state.warehouse_money = 100;
    state.warehouse_max_money = 1000;

    const auto result = manager.deposit(state, 300);

    EXPECT_EQ(result.status, PyoGukTransferStatus::Ack);
    EXPECT_EQ(result.transferred, 300u);
    EXPECT_EQ(result.warehouse_money, 400u);
    EXPECT_EQ(state.inventory_money, 500u);
}

TEST(PyoGukDeposit, ClampsToInventoryBalance) {
    auto manager = make_manager();
    PyoGukAccountState state;
    state.inventory_money = 250;
    state.warehouse_max_money = 1000;

    const auto result = manager.deposit(state, 900);

    EXPECT_EQ(result.transferred, 250u);
    EXPECT_EQ(state.inventory_money, 0u);
    EXPECT_EQ(state.warehouse_money, 250u);
}

TEST(PyoGukDeposit, ClampsToWarehouseCapacity) {
    auto manager = make_manager();
    PyoGukAccountState state;
    state.inventory_money = 500;
    state.warehouse_money = 900;
    state.warehouse_max_money = 1000;

    const auto result = manager.deposit(state, 500);

    EXPECT_EQ(result.transferred, 100u);
    EXPECT_EQ(state.inventory_money, 400u);
    EXPECT_EQ(state.warehouse_money, 1000u);
}

TEST(PyoGukDeposit, ZeroRequestReturnsNack) {
    auto manager = make_manager();
    PyoGukAccountState state;
    state.inventory_money = 500;
    state.warehouse_max_money = 1000;

    EXPECT_EQ(manager.deposit(state, 0).status, PyoGukTransferStatus::Nack);
}

TEST(PyoGukDeposit, FullWarehouseReturnsNackWithoutUnderflow) {
    auto manager = make_manager();
    PyoGukAccountState state;
    state.inventory_money = 500;
    state.warehouse_money = 1001;
    state.warehouse_max_money = 1000;

    const auto result = manager.deposit(state, 100);

    EXPECT_EQ(result.status, PyoGukTransferStatus::Nack);
    EXPECT_EQ(state.inventory_money, 500u);
    EXPECT_EQ(state.warehouse_money, 1001u);
}

TEST(PyoGukWithdraw, TransfersRequestedAmount) {
    auto manager = make_manager();
    PyoGukAccountState state;
    state.inventory_money = 100;
    state.inventory_max_money = 1000;
    state.warehouse_money = 600;

    const auto result = manager.withdraw(state, 300);

    EXPECT_EQ(result.status, PyoGukTransferStatus::Ack);
    EXPECT_EQ(result.transferred, 300u);
    EXPECT_EQ(result.warehouse_money, 300u);
    EXPECT_EQ(state.inventory_money, 400u);
}

TEST(PyoGukWithdraw, ClampsToWarehouseBalance) {
    auto manager = make_manager();
    PyoGukAccountState state;
    state.inventory_money = 100;
    state.inventory_max_money = 1000;
    state.warehouse_money = 200;

    const auto result = manager.withdraw(state, 900);

    EXPECT_EQ(result.transferred, 200u);
    EXPECT_EQ(state.inventory_money, 300u);
    EXPECT_EQ(state.warehouse_money, 0u);
}

TEST(PyoGukWithdraw, ClampsToInventoryCapacity) {
    auto manager = make_manager();
    PyoGukAccountState state;
    state.inventory_money = 950;
    state.inventory_max_money = 1000;
    state.warehouse_money = 500;

    const auto result = manager.withdraw(state, 400);

    EXPECT_EQ(result.transferred, 50u);
    EXPECT_EQ(state.inventory_money, 1000u);
    EXPECT_EQ(state.warehouse_money, 450u);
}

TEST(PyoGukWithdraw, ZeroTransferProducesNoResponse) {
    auto manager = make_manager();
    PyoGukAccountState state;
    state.inventory_money = 100;
    state.inventory_max_money = 1000;
    state.warehouse_money = 0;

    EXPECT_EQ(manager.withdraw(state, 100).status, PyoGukTransferStatus::NoResponse);
    EXPECT_EQ(manager.withdraw(state, 0).status, PyoGukTransferStatus::NoResponse);
}

TEST(PyoGukWithdraw, FullInventoryProducesNoResponseWithoutUnderflow) {
    auto manager = make_manager();
    PyoGukAccountState state;
    state.inventory_money = 1001;
    state.inventory_max_money = 1000;
    state.warehouse_money = 500;

    const auto result = manager.withdraw(state, 100);

    EXPECT_EQ(result.status, PyoGukTransferStatus::NoResponse);
    EXPECT_EQ(state.inventory_money, 1001u);
    EXPECT_EQ(state.warehouse_money, 500u);
}

TEST(PyoGukAccess, ShowWarehouseItemBypassesNpcRange) {
    EXPECT_TRUE(PyoGukManager::check_access(true, false));
}

TEST(PyoGukAccess, NpcRangeAllowsNormalAccess) {
    EXPECT_TRUE(PyoGukManager::check_access(false, true));
}

TEST(PyoGukAccess, MissingItemAndNpcRangeIsRejected) {
    EXPECT_FALSE(PyoGukManager::check_access(false, false));
}

} // namespace
} // namespace mxh::server
