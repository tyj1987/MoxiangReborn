// purse_test.cpp - Phase D6 Purse 1:1 port tests.

#include "mxh/server/purse.hpp"

#include <gtest/gtest.h>

namespace {

using mxh::server::purse_init;
using mxh::server::purse_release;
using mxh::server::purse_addition;
using mxh::server::purse_subtraction;
using mxh::server::purse_is_addition_enough;
using mxh::server::purse_is_enough_money;
using mxh::server::purse_set_max_money;
using mxh::server::purse_set_zero_money;
using mxh::server::purse_get_cur_money;
using mxh::server::purse_get_max_money;
using mxh::server::PurseState;
using mxh::server::MoneyType;
using mxh::server::PURSE_UNLIMITED;

TEST(Purse, InitClampsMoneyToMax) {
    PurseState s{};
    EXPECT_TRUE(purse_init(s, nullptr, 1000u, 500u));
    EXPECT_EQ(purse_get_cur_money(s), 500u);
    EXPECT_EQ(purse_get_max_money(s), 500u);
}

TEST(Purse, InitRejectsDuplicate) {
    PurseState s{};
    ASSERT_TRUE(purse_init(s, nullptr, 0u, 100u));
    EXPECT_FALSE(purse_init(s, nullptr, 0u, 100u));
}

TEST(Purse, ReleaseClearsInit) {
    PurseState s{};
    purse_init(s, nullptr, 100u, 1000u);
    purse_release(s);
    EXPECT_FALSE(s.m_bInit);
}

TEST(Purse, AdditionCapsAtRoom) {
    PurseState s{};
    purse_init(s, nullptr, 800u, 1000u);
    EXPECT_EQ(purse_addition(s, 500u), 200u);  // cap at 200
    EXPECT_EQ(purse_get_cur_money(s), 1000u);
}

TEST(Purse, AdditionAtMaxReturnsZero) {
    PurseState s{};
    purse_init(s, nullptr, 1000u, 1000u);
    EXPECT_EQ(purse_addition(s, 1u), 0u);
    EXPECT_EQ(purse_get_cur_money(s), 1000u);
}

TEST(Purse, AdditionUnlimitedGrowsWithoutBound) {
    PurseState s{};
    purse_init(s, nullptr, 0u, PURSE_UNLIMITED);
    EXPECT_EQ(purse_addition(s, 12345u), 12345u);
}

TEST(Purse, SubtractionFailsIfNotEnough) {
    PurseState s{};
    purse_init(s, nullptr, 100u, 1000u);
    EXPECT_EQ(purse_subtraction(s, 500u), 0u);
    EXPECT_EQ(purse_get_cur_money(s), 100u);
}

TEST(Purse, SubtractionSucceedsWhenEnough) {
    PurseState s{};
    purse_init(s, nullptr, 1000u, 1000u);
    EXPECT_EQ(purse_subtraction(s, 300u), 300u);
    EXPECT_EQ(purse_get_cur_money(s), 700u);
}

TEST(Purse, SubtractionAtBoundary) {
    PurseState s{};
    purse_init(s, nullptr, 100u, 1000u);
    EXPECT_EQ(purse_subtraction(s, 100u), 100u);
    EXPECT_EQ(purse_get_cur_money(s), 0u);
}

TEST(Purse, IsEnoughMoneyReflectsBalance) {
    PurseState s{};
    purse_init(s, nullptr, 50u, 1000u);
    EXPECT_TRUE(purse_is_enough_money(s, 50u));
    EXPECT_FALSE(purse_is_enough_money(s, 51u));
}

TEST(Purse, IsAdditionEnoughReflectsRoom) {
    PurseState s{};
    purse_init(s, nullptr, 800u, 1000u);
    EXPECT_TRUE(purse_is_addition_enough(s, 200u));
    EXPECT_FALSE(purse_is_addition_enough(s, 201u));
}

TEST(Purse, IsAdditionEnoughUnlimitedAlways) {
    PurseState s{};
    purse_init(s, nullptr, 0u, PURSE_UNLIMITED);
    EXPECT_TRUE(purse_is_addition_enough(s, 1000000000ull));
}

TEST(Purse, SetMaxRefusesWhenCurExceeds) {
    PurseState s{};
    purse_init(s, nullptr, 1000u, 1000u);
    EXPECT_FALSE(purse_set_max_money(s, 500u));
    EXPECT_EQ(purse_get_max_money(s), 1000u);
}

TEST(Purse, SetMaxAcceptsWhenLarger) {
    PurseState s{};
    purse_init(s, nullptr, 100u, 1000u);
    EXPECT_TRUE(purse_set_max_money(s, 5000u));
    EXPECT_EQ(purse_get_max_money(s), 5000u);
}

TEST(Purse, SetZeroMoneyClearsToZero) {
    PurseState s{};
    purse_init(s, nullptr, 999u, 1000u);
    purse_set_zero_money(s);
    EXPECT_EQ(purse_get_cur_money(s), 0u);
}

TEST(Purse, AdditionBeforeInitReturnsZero) {
    PurseState s{};
    EXPECT_EQ(purse_addition(s, 100u), 0u);
}

TEST(Purse, SubtractionBeforeInitReturnsZero) {
    PurseState s{};
    EXPECT_EQ(purse_subtraction(s, 100u), 0u);
}

}  // namespace
