// tactic_ability_info_test.cpp - Phase D6 TaticAbilityInfo 1:1 port tests.

#include "mxh/server/tactic_ability_info.hpp"

#include <gtest/gtest.h>

namespace {

using mxh::server::TaticAbilityInfoData;
using mxh::server::tactic_ability_info_copy;
using mxh::server::tactic_ability_get_attack;
using mxh::server::tactic_ability_get_recover;
using mxh::server::tactic_ability_get_buff_rate;
using mxh::server::tactic_ability_get_buff;
using mxh::server::MAX_TATIC_ABILITY_NUM;

TaticAbilityInfoData make_fixture() {
    TaticAbilityInfoData d{};
    for (std::size_t i = 0; i < MAX_TATIC_ABILITY_NUM; ++i) {
        d.wTypeAttack[i]  = static_cast<std::uint16_t>(10u + i);
        d.wTypeRecover[i] = static_cast<std::uint16_t>(20u + i);
        d.fTypeBuffRate[i] = 0.1f * static_cast<float>(i + 1);
        d.wTypeBuff[i]    = static_cast<std::uint16_t>(30u + i);
    }
    return d;
}

TEST(TacticAbility, GetAttackReturnsLevelMinusOne) {
    auto d = make_fixture();
    EXPECT_EQ(tactic_ability_get_attack(d, 1), 10u);
    EXPECT_EQ(tactic_ability_get_attack(d, 5), 14u);
    EXPECT_EQ(tactic_ability_get_attack(d, MAX_TATIC_ABILITY_NUM),
              10u + MAX_TATIC_ABILITY_NUM - 1u);
}

TEST(TacticAbility, GetRecoverReturnsLevelMinusOne) {
    auto d = make_fixture();
    EXPECT_EQ(tactic_ability_get_recover(d, 3), 22u);
}

TEST(TacticAbility, GetBuffRateReturnsFloatLevelMinusOne) {
    auto d = make_fixture();
    EXPECT_FLOAT_EQ(tactic_ability_get_buff_rate(d, 1), 0.1f);
    EXPECT_FLOAT_EQ(tactic_ability_get_buff_rate(d, 6), 0.6f);
}

TEST(TacticAbility, GetBuffReturnsLevelMinusOne) {
    auto d = make_fixture();
    EXPECT_EQ(tactic_ability_get_buff(d, 2), 31u);
}

TEST(TacticAbility, LevelZeroReturnsZero) {
    auto d = make_fixture();
    EXPECT_EQ(tactic_ability_get_attack(d, 0), 0u);
    EXPECT_EQ(tactic_ability_get_recover(d, 0), 0u);
    EXPECT_FLOAT_EQ(tactic_ability_get_buff_rate(d, 0), 0.0f);
    EXPECT_EQ(tactic_ability_get_buff(d, 0), 0u);
}

TEST(TacticAbility, OverflowLevelClampedToMax) {
    auto d = make_fixture();
    EXPECT_EQ(tactic_ability_get_attack(d, 99), 10u + MAX_TATIC_ABILITY_NUM - 1u);
    EXPECT_EQ(tactic_ability_get_recover(d, 999), 20u + MAX_TATIC_ABILITY_NUM - 1u);
}

TEST(TacticAbility, CopyFromRightSizedBuffer) {
    auto d_src = make_fixture();
    TaticAbilityInfoData d_dst{};
    tactic_ability_info_copy(d_dst, &d_src, sizeof(d_src));
    EXPECT_EQ(tactic_ability_get_attack(d_dst, 1), 10u);
}

TEST(TacticAbility, CopyFromWrongSizeClears) {
    auto d_src = make_fixture();
    TaticAbilityInfoData d_dst{};
    d_dst.wTypeAttack[0] = 0xBEEF;
    tactic_ability_info_copy(d_dst, &d_src, sizeof(d_src) - 1);
    EXPECT_EQ(tactic_ability_get_attack(d_dst, 1), 0u);
}

TEST(TacticAbility, CopyFromNullClears) {
    TaticAbilityInfoData d{};
    tactic_ability_info_copy(d, nullptr, sizeof(d));
    EXPECT_EQ(tactic_ability_get_attack(d, 1), 0u);
}

}  // namespace
