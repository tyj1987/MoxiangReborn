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

struct FakeRegist {
    std::uint32_t origin_num;
    std::uint32_t origin_price;
};

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
    FakeRegist in{7u, 12345u};
    EXPECT_TRUE(set_regist_economy(s, &in, sizeof(FakeRegist)));
}

TEST_F(EconomyFixture, SetRegistEconomyRejectsWrongSize) {
    FakeRegist in{7u, 12345u};
    EXPECT_FALSE(set_regist_economy(s, &in, sizeof(FakeRegist) - 1));
}

TEST_F(EconomyFixture, SetBaseValueCopiesBytes) {
    char buf[16] = "hello";
    EXPECT_TRUE(set_base_value(s, buf, sizeof(buf)));
}

TEST_F(EconomyFixture, CalculBaseReturnsPrice) {
    FakeRegist in{7u, 5000u};
    ASSERT_TRUE(set_regist_economy(s, &in, sizeof(FakeRegist)));
    auto res = calcul_base(s, 7u);
    EXPECT_EQ(res.base_price, 5000u);
    EXPECT_EQ(res.buy_price,  5000u);
}

TEST_F(EconomyFixture, CalculBaseMismatchedOriginReturnsZero) {
    FakeRegist in{7u, 5000u};
    ASSERT_TRUE(set_regist_economy(s, &in, sizeof(FakeRegist)));
    auto res = calcul_base(s, 99u);
    EXPECT_EQ(res.base_price, 0u);
    EXPECT_EQ(res.buy_price,  0u);
}

TEST(EconomyNoInit, SetRegistReturnsFalse) {
    EconomyState s{};
    FakeRegist in{1u, 1u};
    EXPECT_FALSE(set_regist_economy(s, &in, sizeof(FakeRegist)));
}

TEST(EconomyNoInit, CalculBaseReturnsZero) {
    EconomyState s{};
    auto res = calcul_base(s, 1u);
    EXPECT_EQ(res.base_price, 0u);
    EXPECT_EQ(res.buy_price, 0u);
}

}  // namespace
