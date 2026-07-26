// distributer_test.cpp - Phase D5 Distributer 1:1 port tests.

#include "mxh/server/distributer.hpp"
#include <gtest/gtest.h>

namespace {
using mxh::server::DistributerState;
using mxh::server::DamageObj;
using mxh::server::MAX_POINTACCEPTOBJECT_NUM;
using mxh::server::make_distributer;
using mxh::server::distributer_release;
using mxh::server::damage_init;
using mxh::server::set_plus_total_damage;
using mxh::server::get_total_damage;
using mxh::server::add_damage_object;
using mxh::server::choose_one;
using mxh::server::delete_damaged_player;
using mxh::server::calc_obtain_exp;
using mxh::server::calc_obtain_ability_exp;
}

// ---- Constants 1:1 ----

TEST(DistributerConstants, MaxPointAcceptObjectNumMatchesLegacy) {
    EXPECT_EQ(MAX_POINTACCEPTOBJECT_NUM, 2u);
}

// ---- Init / Release ----

TEST(DistributerInit, DefaultIsZeroed) {
    auto s = make_distributer();
    EXPECT_EQ(s.m_TotalDamage, 0u);
    EXPECT_EQ(s.m_1stPlayerID, 0u);
    EXPECT_EQ(s.m_PlusDamage, 0u);
    EXPECT_TRUE(s.m_DamageObjectTableSolo.empty());
    EXPECT_TRUE(s.m_DamageObjectTableParty.empty());
}

TEST(DistributerInit, ReleaseClearsAll) {
    auto s = make_distributer();
    add_damage_object(s, 1u, 100u, false);
    distributer_release(s);
    EXPECT_TRUE(s.m_DamageObjectTableSolo.empty());
    EXPECT_EQ(s.m_TotalDamage, 0u);
}

TEST(DistributerInit, DamageInitClearsAll) {
    auto s = make_distributer();
    s.m_1stPlayerID = 999u;
    add_damage_object(s, 1u, 50u, false);
    damage_init(s);
    // legacy damage_init zeroes 1stPlayerID and the damage tables.
    EXPECT_EQ(s.m_1stPlayerID, 0u);
    EXPECT_TRUE(s.m_DamageObjectTableSolo.empty());
    EXPECT_EQ(s.m_TotalDamage, 0u);
    EXPECT_EQ(s.m_PlusDamage, 0u);
}

// ---- Add damage ----

TEST(DistributerAdd, SoloRecordsDamage) {
    auto s = make_distributer();
    add_damage_object(s, 1u, 100u, /*in_party*/false);
    EXPECT_EQ(get_total_damage(s), 100u);
    EXPECT_EQ(s.m_DamageObjectTableSolo.size(), 1u);
    EXPECT_EQ(s.m_DamageObjectTableSolo[1u].dwData, 100u);
    EXPECT_TRUE(s.m_DamageObjectTableParty.empty());
}

TEST(DistributerAdd, PartyRecordsDamage) {
    auto s = make_distributer();
    add_damage_object(s, 1u, 100u, /*in_party*/true);
    EXPECT_EQ(s.m_DamageObjectTableParty.size(), 1u);
    EXPECT_TRUE(s.m_DamageObjectTableSolo.empty());
}

TEST(DistributerAdd, RepeatedAddAccumulates) {
    auto s = make_distributer();
    add_damage_object(s, 1u, 100u, false);
    add_damage_object(s, 1u, 50u, false);
    EXPECT_EQ(s.m_DamageObjectTableSolo[1u].dwData, 150u);
    EXPECT_EQ(get_total_damage(s), 150u);
}

TEST(DistributerAdd, FirstAttackerRecorded) {
    auto s = make_distributer();
    add_damage_object(s, 42u, 10u, false);
    EXPECT_EQ(s.m_1stPlayerID, 42u);
    add_damage_object(s, 100u, 999u, false);
    EXPECT_EQ(s.m_1stPlayerID, 42u);  // first stays
}

// ---- SetPlusTotalDamage ----

TEST(DistributerPlus, SetPlusStoresAndUpdatesTotal) {
    auto s = make_distributer();
    set_plus_total_damage(s, 500u);
    EXPECT_EQ(s.m_PlusDamage, 500u);
    EXPECT_EQ(get_total_damage(s), 500u);
}

// ---- ChooseOne ----

TEST(DistributerChooseOne, SoloPicksBiggest) {
    auto s = make_distributer();
    add_damage_object(s, 1u, 100u, false);
    add_damage_object(s, 2u, 200u, false);
    add_damage_object(s, 3u, 50u, false);
    EXPECT_EQ(choose_one(s, false), 2u);
}

TEST(DistributerChooseOne, PartyPicksBiggest) {
    auto s = make_distributer();
    add_damage_object(s, 1u, 100u, true);
    add_damage_object(s, 2u, 200u, true);
    EXPECT_EQ(choose_one(s, true), 2u);
}

TEST(DistributerChooseOne, EmptyReturnsNullopt) {
    auto s = make_distributer();
    EXPECT_FALSE(choose_one(s, false).has_value());
    EXPECT_FALSE(choose_one(s, true).has_value());
}

// ---- DeleteDamagedPlayer ----

TEST(DistributerDelete, RemovesFromSoloAndUpdatesTotal) {
    auto s = make_distributer();
    add_damage_object(s, 1u, 100u, false);
    add_damage_object(s, 2u, 200u, false);
    delete_damaged_player(s, 1u);
    EXPECT_EQ(s.m_DamageObjectTableSolo.size(), 1u);
    EXPECT_EQ(get_total_damage(s), 200u);
}

TEST(DistributerDelete, RemovesFromPartyAndUpdatesTotal) {
    auto s = make_distributer();
    add_damage_object(s, 1u, 100u, true);
    delete_damaged_player(s, 1u);
    EXPECT_EQ(s.m_DamageObjectTableParty.size(), 0u);
    EXPECT_EQ(get_total_damage(s), 0u);
}

TEST(DistributerDelete, UnknownIsNoOp) {
    auto s = make_distributer();
    add_damage_object(s, 1u, 100u, false);
    delete_damaged_player(s, 999u);
    EXPECT_EQ(get_total_damage(s), 100u);
}

TEST(DistributerDelete, ResetsFirstAttackerIfMatch) {
    auto s = make_distributer();
    add_damage_object(s, 42u, 10u, false);
    delete_damaged_player(s, 42u);
    EXPECT_EQ(s.m_1stPlayerID, 0u);
}

// ---- CalcObtainExp formula ----

TEST(DistributerExp, ZeroLifeReturnsZero) {
    EXPECT_EQ(calc_obtain_exp(50, 50, 0u, 100u, 1u), 0u);
}

TEST(DistributerExp, ZeroDamageReturnsZero) {
    EXPECT_EQ(calc_obtain_exp(50, 50, 1000u, 0u, 1u), 0u);
}

TEST(DistributerExp, DamageGreaterThanLifeClamps) {
    // 1500 damage, 1000 life -> ratio = 1.0
    const std::uint32_t e = calc_obtain_exp(50, 50, 1000u, 1500u, 1u);
    // base=500, level_diff=0, mult=100, ratio_bp=1000
    // exp = 500*100*1000 / 1000 / 100 = 500
    EXPECT_EQ(e, 500u);
}

TEST(DistributerExp, SameLevelSoloFullDamage) {
    // monster=50, killer=50, total=1000, damage=1000, players=1
    const std::uint32_t e = calc_obtain_exp(50, 50, 1000u, 1000u, 1u);
    EXPECT_EQ(e, 500u);
}

TEST(DistributerExp, PartyDoublesDamage) {
    const std::uint32_t solo  = calc_obtain_exp(50, 50, 1000u, 1000u, 1u);
    const std::uint32_t party = calc_obtain_exp(50, 50, 1000u, 1000u, 2u);
    EXPECT_EQ(party, solo / 2u);
}

TEST(DistributerExp, HigherMonsterLevelBoostsExp) {
    // diff +5 (clamped)
    const std::uint32_t same  = calc_obtain_exp(50, 50, 1000u, 1000u, 1u);
    const std::uint32_t above = calc_obtain_exp(60, 50, 1000u, 1000u, 1u);
    EXPECT_GT(above, same);
}

TEST(DistributerExp, LowerMonsterLevelGivesLessExp) {
    // base scales with monster_level, so lower monster gives less exp.
    const std::uint32_t same  = calc_obtain_exp(50, 50, 1000u, 1000u, 1u);
    const std::uint32_t below = calc_obtain_exp(40, 50, 1000u, 1000u, 1u);
    EXPECT_LT(below, same);
}

TEST(DistributerExp, HalfDamageHalfExp) {
    const std::uint32_t full = calc_obtain_exp(50, 50, 1000u, 1000u, 1u);
    const std::uint32_t half = calc_obtain_exp(50, 50, 1000u, 500u, 1u);
    // half should be ~full/2 (integer rounding)
    EXPECT_GE(half * 2u, full - 1u);
    EXPECT_LE(half * 2u, full + 1u);
}

// ---- CalcObtainAbilityExp ----

TEST(DistributerAbilityExp, SameLevelGivesBase) {
    // monster=50, killer=50: base=250, mult=100, exp=250
    EXPECT_EQ(calc_obtain_ability_exp(50, 50), 250u);
}

TEST(DistributerAbilityExp, HigherMonsterBoost) {
    EXPECT_GT(calc_obtain_ability_exp(60, 50), calc_obtain_ability_exp(50, 50));
}

TEST(DistributerAbilityExp, LowerMonsterGivesLess) {
    EXPECT_LT(calc_obtain_ability_exp(40, 50), calc_obtain_ability_exp(50, 50));
}

TEST(DistributerAbilityExp, ClampSaturatesMultiplier) {
    // exp scales with monster_level * (100 + clamped_diff * 5).
    // For diff == +5 and diff == +50 (both clamped), the multiplier is
    // identical, so the exp ratio equals the monster_level ratio.
    // (100/60) = 5/3; verify ratio of results reflects that.
    const std::uint32_t lo = calc_obtain_ability_exp(60, 95);  // diff=0 (clamped=0)
    const std::uint32_t hi = calc_obtain_ability_exp(100, 95); // diff=5 (clamped=5)
    // hi > lo because higher base AND higher multiplier
    EXPECT_GT(hi, lo);
    // And at monster_level=200, diff=105 -> clamped=5; exp should be 2x of the +60 case
    const std::uint32_t hi2 = calc_obtain_ability_exp(120, 95);
    EXPECT_EQ(hi2, hi * 120 / 100);
}
