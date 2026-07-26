// battle_factory_test.cpp - 1:1 numeric locks for the BattleFactory formulas.
//
// Each test pins a single output of the formula to a hand-computed value
// (or to the output of running the legacy function with equivalent inputs).
#include "mxh/game/battle_factory.hpp"
#include <gtest/gtest.h>
#include <cmath>
namespace mxh::game {
namespace {
// Helper: legacy uses ceil(Exp * 10) > floor(Exp) * 10. We model the same logic.
bool approx_eq(double a, double b, double eps = 1e-6) {
    return std::fabs(a - b) < eps;
}
}  // namespace
// ---- Critical rate (AttackCalc.cpp L79) ----
TEST(BattleFactory, CriticalBaseClampsAt020) {
    // attacker_crit=80, target_level=10: (80+20) / (10*5+100) = 100/150 = 0.6667
    double rate = compute_critical_rate(80, 5, 10, 0.0);
    EXPECT_DOUBLE_EQ(rate, 0.2);  // clamped
}

TEST(BattleFactory, CriticalBaseLowerBound) {
    // attacker_crit=20, target_level=70: (20+20) / (70*5+100) = 40/450 = 0.0889
    double rate = compute_critical_rate(20, 70, 70, 0.0);
    EXPECT_NEAR(rate, 40.0 / 1700.0, 1e-9);  // not clamped
}

TEST(BattleFactory, CriticalUnderLeveledAttackerPenalty) {
    // target_level=50, attacker_level=40, fCriticalRate=0.05, base=0.05
    // penalty = (50-40)*0.02 = 0.2  =>  rate=0.05+0.05-0.2 = -0.1  (clamped to 0)
    double base = static_cast<double>(20 + 20) / (50 * 20 + 300);
    double rate = compute_critical_rate(20, 40, 50, 0.05);
    EXPECT_EQ(rate, 0.0);
}

TEST(BattleFactory, CriticalHigherLeveledAttackerBonus) {
    // attacker_level=50, target_level=40, base=0.1, bonus=(10)*0.004=0.04
    double rate = compute_critical_rate(30, 50, 40, 0.0);
    double expected = static_cast<double>(30 + 20) / (40 * 20 + 300)
;
    EXPECT_NEAR(rate, expected, 1e-9);
}

TEST(BattleFactory, CriticalAllowsLegacyBonusBeyondBaseCeiling) {
    // attacker_crit=300, target_level=1: (300+20)/(5+100) = 320/105 = 3.05 -> 0.2
    double rate = compute_critical_rate(300, 50, 1, 0.20);
    EXPECT_NEAR(rate, 0.596, 1e-9);  // legacy bonus is added after base ceiling
}

TEST(BattleFactory, RollCriticalWithinBounds) {
    EXPECT_TRUE(roll_critical(0.5, 49));    // 49 < 50
    EXPECT_FALSE(roll_critical(0.5, 50));   // 50 < 50 is false
    EXPECT_TRUE(roll_critical(0.99, 98));
    EXPECT_FALSE(roll_critical(0.99, 99));
    EXPECT_FALSE(roll_critical(0.0, 0));    // rate=0 always false
    EXPECT_FALSE(roll_critical(-1.0, 50));  // negative rate always false
}

// ---- Decisive (same formula, different source stat) ----
TEST(BattleFactory, DecisiveHasSameFormulaAsCritical) {
    double a = compute_critical_rate(120, 50, 40, 0.05);
    double b = compute_decisive_rate(120, 50, 40, 0.05);
    EXPECT_DOUBLE_EQ(a, b);
}

// ---- Physical attack power (AttackCalc.cpp L217) ----
TEST(BattleFactory, PhysicalAttackNoCriticalMinVal) {
    // base_atk_bonus = -0.3 -> val = 0.7 (clamped >= 0)
    // min=10,max=50: scale=(10*0.7)+0.5=7, mx=(50*0.7)+0.5=35
    // gap=29; rand_gap=0 -> rolled=7; *1.0 = 7
    EXPECT_EQ(compute_player_physical_attack(10, 50, -0.3, 1.0, false, 0), 7u);
}

TEST(BattleFactory, PhysicalAttackCritical15x) {
    // min=max=10, base_atk_bonus=0, rate=1.0, bCritical=true
    // mx<=scale -> rolled=10; * 1.0; * 1.5 = 15
    EXPECT_EQ(compute_player_physical_attack(10, 10, 0.0, 1.0, true, 0), 15u);
}

TEST(BattleFactory, PhysicalAttackSkillRateApplied) {
    // min=max=100, base_atk_bonus=0, rate=2.5, no crit
    EXPECT_EQ(compute_player_physical_attack(100, 100, 0.0, 2.5, false, 0), 250u);
}

TEST(BattleFactory, PhysicalAttackRandomGapHonored) {
    // min=10,max=20; scale=10, mx=20, gap=11
    // rolled = 10 + (5 % 11) = 15
    EXPECT_EQ(compute_player_physical_attack(10, 20, 0.0, 1.0, false, 5), 15u);
}

// ---- Attribute attack (AttackCalc.cpp L260, KR/CN branch) ----
TEST(BattleFactory, AttributeAttackZeroRateZero) {
    EXPECT_EQ(compute_player_attribute_attack(50, 100, 10, 20, 0.0), 0u);
}

TEST(BattleFactory, AttributeAttackSimMekBelow12) {
    // sim_mek=5 => cap = max(5-12, 0) = 0
    // sim_div5 = 1, midterm = (5+200)/100 = 2.05
    // minlvv=50-50+5-5=50, maxlvv=50-50+5+5=10  (legacy level+5-5/+5+5)
    auto v = compute_player_attribute_attack(50, 5, 0, 0, 1.0);
    // minv = 50 * 1.0 * 2.05 + 1 + 0 = 103.5
    // maxv = 60 * 1.0 * 2.05 + 1 + 0 = 124.0  (level+5+5=60)
    // avg = (103.5 + 124.0) * 0.5 + 0 = 113.75 -> truncated -> 113
    EXPECT_EQ(v, 113u);
}

TEST(BattleFactory, AttributeAttackSimMekHighCapsAt25) {
    // sim_mek=200 => cap = min(200-12, 25) = 25
    // sim_div5 = 40, midterm = 4.0, level 30: minlvv=30, maxlvv=40
    auto v = compute_player_attribute_attack(30, 200, 0, 0, 1.0);
    // minv = 30*4.0 + 40 + 25 = 185; maxv = 40*4.0 + 40 + 25 = 225
    // avg = (185+225)/2 = 205
    EXPECT_EQ(v, 205u);
}

// ---- EXP point gain (AttackCalc.cpp L45) ----
TEST(BattleFactory, ExpPointFarBelowLevel) {
    // gap < -8 => exp * 1.5; ceil to integer
    EXPECT_EQ(compute_player_exp_point(-9, 100), 150u);
    EXPECT_EQ(compute_player_exp_point(-100, 100), 150u);
}

TEST(BattleFactory, ExpPointEvenLevel) {
    // gap == 0 => exp (no bonus)
    EXPECT_EQ(compute_player_exp_point(0, 200), 200u);
}

TEST(BattleFactory, ExpPointOneBelowLevel) {
    // gap = -1 => exp * 1.05 => 105
    EXPECT_EQ(compute_player_exp_point(-1, 100), 105u);
}

TEST(BattleFactory, ExpPointFourAboveLevel) {
    // gap = 4 => exp * (5-4) * 0.2 = exp * 0.2
    EXPECT_EQ(compute_player_exp_point(4, 500), 100u);
}

TEST(BattleFactory, ExpPointFiveAboveLevelIsTenPercent) {
    // gap = 5 => exp * 0.1
    EXPECT_EQ(compute_player_exp_point(5, 200), 20u);
}

TEST(BattleFactory, ExpPointFarAboveLevelIsZero) {
    // gap > 5 => 0
    EXPECT_EQ(compute_player_exp_point(6, 200), 0u);
    EXPECT_EQ(compute_player_exp_point(1000, 200), 0u);
}

TEST(BattleFactory, ExpPointRoundsUpAtFirstDecimal) {
    // gap = -4 => exp * 1.2 (e.g. 333 * 1.2 = 399.6); floor10=3996, floored*10=3990
    // 3996 > 3990 -> +1 -> 400
    EXPECT_EQ(compute_player_exp_point(-4, 333), 400u);
}

// ---- Clamp lookup (AttackCalc.cpp L28) ----
static std::uint32_t fake_lookup(std::int32_t level, std::int32_t gap) {
    return static_cast<std::uint32_t>((level * 100) + gap);
}

TEST(BattleFactory, MaxLevelShortCircuitsToZero) {
    EXPECT_EQ(clamp_player_x_monster_lookup(99, 0, fake_lookup), 0u);
}

TEST(BattleFactory, NegativeLevelIsZero) {
    EXPECT_EQ(clamp_player_x_monster_lookup(0, 5, fake_lookup), 0u);
    EXPECT_EQ(clamp_player_x_monster_lookup(-1, 5, fake_lookup), 0u);
}

TEST(BattleFactory, LevelGapClampedToMinus8) {
    // gap < -8 -> -8
    EXPECT_EQ(clamp_player_x_monster_lookup(50, -100, fake_lookup), 5000u - 8);  // 50*100 + (-8)
}

TEST(BattleFactory, LevelGapClampedToPlus12) {
    // gap >= 12 -> 12 (legacy uses 12 as sentinel upper bound)
    EXPECT_EQ(clamp_player_x_monster_lookup(50, 100, fake_lookup), 5000u + 12);
    EXPECT_EQ(clamp_player_x_monster_lookup(50, 12, fake_lookup), 5000u + 12);
}

// ---- Phy defence level (AttackCalc.cpp getPhyDefenceLevel, KR/CN branch) ----
TEST(BattleFactory, PhyDefenceLevelMatchesLegacyFormula) {
    // (phy*2 + 50) / (lvl*20 + 150), clamp [0, 0.9].
    // defender_phy=0, attacker_lvl=1 -> (0+50)/(20+150) = 50/170 ~= 0.294
    double r0 = compute_phy_defence_level(0.0, 1);
    EXPECT_NEAR(r0, 50.0 / 170.0, 1e-9);
    // defender_phy=100, attacker_lvl=50 -> (250)/(1150) ~= 0.217
    double r1 = compute_phy_defence_level(100.0, 50);
    EXPECT_NEAR(r1, 250.0 / 1150.0, 1e-9);
}

TEST(BattleFactory, PhyDefenceLevelClampedAt09) {
    // Very high defence must clamp at 0.9 (KR/CN ceiling).
    EXPECT_DOUBLE_EQ(compute_phy_defence_level(100000.0, 1), 0.9);
    EXPECT_DOUBLE_EQ(compute_phy_defence_level(9999.0, 1), 0.9);
}

TEST(BattleFactory, PhyDefenceLevelClampedAtZeroForNegative) {
    // attacker_level < 1 is forced to 1, so the formula never returns < 0.
    EXPECT_GE(compute_phy_defence_level(0.0, 0), 0.0);
    EXPECT_GE(compute_phy_defence_level(-100.0, 1), 0.0);
}

TEST(BattleFactory, PhyDefenceLevelTitanBranchCapsAt08) {
    // titan_defense_override > 0 activates the SW070127 Titan branch.
    EXPECT_DOUBLE_EQ(compute_phy_defence_level(0.0, 1, 0.5), 0.5);
    EXPECT_DOUBLE_EQ(compute_phy_defence_level(0.0, 1, 0.81), 0.8);
    EXPECT_DOUBLE_EQ(compute_phy_defence_level(0.0, 1, 1.0), 0.8);
}

// ---- Received damage ----
TEST(BattleFactory, ReceivedDamageFollowsOneMinusDefence) {
    // dmg = attack * (1 - defLevel), floor at 1.
    EXPECT_EQ(compute_received_damage(1000u, 0.0), 1000u);
    EXPECT_EQ(compute_received_damage(1000u, 0.1), 900u);
    EXPECT_EQ(compute_received_damage(1000u, 0.9), 99u);
    // 1000 * 0.1 = 99.999... -> truncates to 99 (FP rounding to nearest).

}

TEST(BattleFactory, ReceivedDamageHasFloorOne) {
    // dmg < 1.0 must round up to 1 (legacy minimum).
    EXPECT_EQ(compute_received_damage(1u, 0.95), 1u);
    EXPECT_EQ(compute_received_damage(5u, 0.99), 1u);
}

TEST(BattleFactory, ReceivedDamageZeroAttackYieldsZero) {
    EXPECT_EQ(compute_received_damage(0u, 0.5), 0u);
}

// ---- Monster physical attack (AttackCalc.cpp getMonsterPhysicalAttackPower) ----
TEST(BattleFactory, MonsterPhysicalAttackMinEqualsMaxYieldsMin) {
    EXPECT_EQ(compute_monster_physical_attack(50u, 50u, 1.0, 7), 50u);
    EXPECT_EQ(compute_monster_physical_attack(50u, 30u, 1.0, 7), 50u);  // max<min
}

TEST(BattleFactory, MonsterPhysicalAttackHonoursRandGap) {
    // gap = max-min+1 = 11; val = min + (rand_gap % 11).
    // rand_gap = 0 -> 50; rand_gap = 10 -> 60; rand_gap = 11 -> 50 (wrap).
    EXPECT_EQ(compute_monster_physical_attack(50u, 60u, 1.0, 0), 50u);
    EXPECT_EQ(compute_monster_physical_attack(50u, 60u, 1.0, 10), 60u);
    EXPECT_EQ(compute_monster_physical_attack(50u, 60u, 1.0, 11), 50u);
}

TEST(BattleFactory, MonsterPhysicalAttackMultipliesByRate) {
    // min=100, max=100, rate=2.5 -> 250.
    EXPECT_EQ(compute_monster_physical_attack(100u, 100u, 2.5, 0), 250u);
    EXPECT_EQ(compute_monster_physical_attack(100u, 100u, 0.5, 0), 50u);
}

}  // namespace mxh::game



