// economy_test.cpp - Phase D6 Economy 1:1 port tests.

#include "mxh/server/economy.hpp"

#include <gtest/gtest.h>

#include <cstring>

namespace {

using mxh::server::EconomyState;
using mxh::server::economy_init;
using mxh::server::economy_release;
using mxh::server::set_regist_economy;
using mxh::server::get_regist_economy;
using mxh::server::set_base_value;
using mxh::server::calcul_base;

using mxh::server::RegistBaseEconomy;

RegistBaseEconomy make_reg(std::uint16_t origin_num, std::uint16_t origin_price) {
    RegistBaseEconomy value{};
    value.OriginNum = origin_num;
    value.OriginPrice = origin_price;
    return value;
}

class EconomyFixture : public ::testing::Test {
protected:
    EconomyState s{};
    void SetUp() override { economy_init(s); }
    void TearDown() override { economy_release(s); }
};

TEST_F(EconomyFixture, InitCreatesState) {
    EXPECT_NE(get_regist_economy(s), nullptr);
    EXPECT_TRUE(s.initialized);
}

TEST_F(EconomyFixture, ReleaseClearsState) {
    economy_release(s);
    EXPECT_FALSE(s.initialized);
    EXPECT_EQ(get_regist_economy(s), nullptr);
}

TEST_F(EconomyFixture, SetRegistEconomyCopiesBytes) {
    auto in = make_reg(7u, 12345u);
    EXPECT_TRUE(set_regist_economy(s, &in, sizeof(RegistBaseEconomy)));
}

TEST_F(EconomyFixture, SetRegistEconomyRejectsWrongSize) {
    auto in = make_reg(7u, 12345u);
    EXPECT_FALSE(set_regist_economy(s, &in, sizeof(RegistBaseEconomy) - 1));
}

TEST_F(EconomyFixture, SetBaseValueCopiesBytes) {
    char buf[16] = "hello";
    EXPECT_TRUE(set_base_value(s, buf, sizeof(buf)));
}

TEST_F(EconomyFixture, CalculBaseIsLegacyNoOp) {
    auto in = make_reg(7u, 5000u);
    ASSERT_TRUE(set_regist_economy(s, &in, sizeof(RegistBaseEconomy)));
    auto res = calcul_base(s, 7u);
    EXPECT_EQ(res.base_price, 0u);
    EXPECT_EQ(res.buy_price, 0u);
}

TEST_F(EconomyFixture, CalculBaseMismatchedOriginReturnsZero) {
    auto in = make_reg(7u, 5000u);
    ASSERT_TRUE(set_regist_economy(s, &in, sizeof(RegistBaseEconomy)));
    auto res = calcul_base(s, 99u);
    EXPECT_EQ(res.base_price, 0u);
    EXPECT_EQ(res.buy_price,  0u);
}

TEST(EconomyNoInit, SetRegistReturnsFalse) {
    EconomyState s{};
    auto in = make_reg(1u, 1u);
    EXPECT_FALSE(set_regist_economy(s, &in, sizeof(RegistBaseEconomy)));
}

TEST(EconomyNoInit, CalculBaseReturnsZero) {
    EconomyState s{};
    auto res = calcul_base(s, 1u);
    EXPECT_EQ(res.base_price, 0u);
    EXPECT_EQ(res.buy_price, 0u);
}

TEST(EconomyLayout, RegistBaseEconomyMatchesLegacyBytes) {
    EXPECT_EQ(sizeof(RegistBaseEconomy), 16u);
    EXPECT_EQ(offsetof(RegistBaseEconomy, MapNum), 0u);
    EXPECT_EQ(offsetof(RegistBaseEconomy, OriginNum), 2u);
    EXPECT_EQ(offsetof(RegistBaseEconomy, OriginPrice), 4u);
    EXPECT_EQ(offsetof(RegistBaseEconomy, BuyRates), 14u);
    EXPECT_EQ(offsetof(RegistBaseEconomy, SellRates), 15u);
}

TEST_F(EconomyFixture, SetRegistEconomyCopiesEveryLegacyField) {
    RegistBaseEconomy in{};
    in.MapNum = 1;
    in.OriginNum = 2;
    in.OriginPrice = 3;
    in.OriginAmount = 4;
    in.RequireNum = 5;
    in.RequirePrice = 6;
    in.RequireAmount = 7;
    in.BuyRates = 8;
    in.SellRates = 9;
    ASSERT_TRUE(set_regist_economy(s, &in, sizeof(in)));
    EXPECT_EQ(std::memcmp(get_regist_economy(s), &in, sizeof(in)), 0);
}

TEST_F(EconomyFixture, NullInputsAreIgnoredSafely) {
    EXPECT_FALSE(set_regist_economy(s, nullptr, sizeof(RegistBaseEconomy)));
    EXPECT_FALSE(set_base_value(s, nullptr, 16));
    EXPECT_FALSE(set_base_value(s, &s, 0));
}

TEST_F(EconomyFixture, SetBaseValueResizesWithoutOverflow) {
    const std::uint32_t first = 0x11223344u;
    const std::uint64_t second = 0x0102030405060708ull;
    ASSERT_TRUE(set_base_value(s, &first, sizeof(first)));
    EXPECT_EQ(s.m_SpacialItemBaseSize, sizeof(first));
    EXPECT_EQ(std::memcmp(s.m_SpacialItemBase, &first, sizeof(first)), 0);
    ASSERT_TRUE(set_base_value(s, &second, sizeof(second)));
    EXPECT_EQ(s.m_SpacialItemBaseSize, sizeof(second));
    EXPECT_EQ(std::memcmp(s.m_SpacialItemBase, &second, sizeof(second)), 0);
}

TEST(EconomyLifecycle, InitAndReleaseAreIdempotent) {
    EconomyState state{};
    economy_init(state);
    const auto* first = get_regist_economy(state);
    economy_init(state);
    EXPECT_EQ(get_regist_economy(state), first);
    economy_release(state);
    economy_release(state);
    EXPECT_FALSE(state.initialized);
    EXPECT_EQ(state.m_SpacialItemBaseSize, 0u);
}

}  // namespace
