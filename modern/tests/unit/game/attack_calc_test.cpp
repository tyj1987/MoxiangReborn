#include "mxh/game/attack_calc.hpp"

#include <gtest/gtest.h>

namespace mxh::game {
namespace {

TEST(AttackCalc, GetPercentUsesLevelGapAndTruncates) {
    EXPECT_EQ(legacy_get_percent(0.5f, 20, 10), 75u);
    EXPECT_EQ(legacy_get_percent(0.04f, 10, 12), 0u);
    EXPECT_EQ(legacy_get_percent(0.126f, 10, 10), 12u);
}

TEST(AttackCalc, CriticalPercentUsesKoreaChinaDenominator) {
    CriticalInput input{20, 50, 40, 0.0f, 0};
    EXPECT_EQ(legacy_critical_percent(input), 3u);
}

TEST(AttackCalc, CriticalPercentAddsSeedPercent) {
    CriticalInput input{20, 50, 40, 0.1f, 0};
    EXPECT_EQ(legacy_critical_percent(input), 38u);
}

TEST(AttackCalc, CriticalPercentClampsBaseAtTwentyPercent) {
    CriticalInput input{500, 1, 1, 0.0f, 0};
    EXPECT_EQ(legacy_critical_percent(input), 20u);
}

TEST(AttackCalc, CriticalUniqueRateCanReduceToZero) {
    CriticalInput input{0, 1, 1, 0.0f, -100};
    EXPECT_EQ(legacy_critical_percent(input), 0u);
}

TEST(AttackCalc, CriticalRollUsesStrictLessThan) {
    EXPECT_TRUE(legacy_roll_percent(50, 49));
    EXPECT_FALSE(legacy_roll_percent(50, 50));
    EXPECT_TRUE(legacy_roll_percent(200, 99));
}

TEST(AttackCalc, JapanCriticalUsesFifteenPercentBaseCeiling) {
    CriticalInput input{500, 10, 10, 0.0f, 0};
    EXPECT_FLOAT_EQ(legacy_japan_critical_rate(input), 0.15f);
}

TEST(AttackCalc, JapanCriticalUnderLevelPenaltyIsClamped) {
    CriticalInput input{0, 1, 20, 0.0f, 0};
    EXPECT_FLOAT_EQ(legacy_japan_critical_rate(input), 0.0f);
}

TEST(AttackCalc, JapanCriticalOverLevelBonusIsAppliedWithoutSeed) {
    CriticalInput input{0, 20, 10, 0.0f, 0};
    EXPECT_FLOAT_EQ(legacy_japan_critical_rate(input), 0.17333333f);
}

TEST(AttackCalc, JapanCriticalRollUsesHundredthSample) {
    EXPECT_TRUE(legacy_roll_japan_rate(0.5f, 50));
    EXPECT_FALSE(legacy_roll_japan_rate(0.5f, 51));
}

TEST(AttackCalc, PlayerPhysicalRollsInclusiveRange) {
    PhysicalAttackInput input{10, 20, 1.0f, 0.0f, false,
                              AttackCalcLocale::KoreaChina, 10};
    EXPECT_DOUBLE_EQ(legacy_player_physical_attack(input), 20.0);
}

TEST(AttackCalc, PlayerPhysicalAppliesStatOptionAndCritical) {
    PhysicalAttackInput input{10, 10, 2.0f, 0.5f, true,
                              AttackCalcLocale::KoreaChina, 0};
    EXPECT_DOUBLE_EQ(legacy_player_physical_attack(input), 45.0);
}

TEST(AttackCalc, PlayerPhysicalJapanCriticalUses225Multiplier) {
    PhysicalAttackInput input{10, 10, 1.0f, 0.0f, true,
                              AttackCalcLocale::Japan, 0};
    EXPECT_DOUBLE_EQ(legacy_player_physical_attack(input), 22.5);
}

TEST(AttackCalc, PlayerPhysicalNegativeStatOptionClampsBaseRate) {
    PhysicalAttackInput input{10, 20, 1.0f, -2.0f, false,
                              AttackCalcLocale::KoreaChina, 0};
    EXPECT_DOUBLE_EQ(legacy_player_physical_attack(input), 0.0);
}

TEST(AttackCalc, MonsterPhysicalIgnoresRateAndCritical) {
    PhysicalAttackInput input{50, 60, 9.0f, 0.0f, true,
                              AttackCalcLocale::KoreaChina, 10};
    EXPECT_DOUBLE_EQ(legacy_monster_physical_attack(input), 60.0);
}

TEST(AttackCalc, MonsterPhysicalDegenerateRangeReturnsMinimum) {
    PhysicalAttackInput input{80, 20, 1.0f, 0.0f, false,
                              AttackCalcLocale::KoreaChina, 4};
    EXPECT_DOUBLE_EQ(legacy_monster_physical_attack(input), 80.0);
}

TEST(AttackCalc, TitanPhysicalAppliesRateAndCritical) {
    PhysicalAttackInput input{20, 20, 1.5f, 0.0f, true,
                              AttackCalcLocale::KoreaChina, 0};
    EXPECT_DOUBLE_EQ(legacy_titan_physical_attack(input), 45.0);
}

TEST(AttackCalc, PlayerAttributeUsesSimMekAndLevelRange) {
    PlayerAttributeAttackInput input{50, 5, 10, 20, 1.0f, 0.0f, 0};
    EXPECT_DOUBLE_EQ(legacy_player_attribute_attack(input), 113.0);
}

TEST(AttackCalc, PlayerAttributeRandomCanReachMaximum) {
    PlayerAttributeAttackInput input{50, 5, 10, 20, 1.0f, 0.0f, 31};
    EXPECT_DOUBLE_EQ(legacy_player_attribute_attack(input), 144.0);
}

TEST(AttackCalc, PlayerAttributeRateZeroStillAddsFlatAttack) {
    PlayerAttributeAttackInput input{50, 20, 7, 9, 0.0f, 0.0f, 1};
    EXPECT_DOUBLE_EQ(legacy_player_attribute_attack(input), 8.0);
}

TEST(AttackCalc, PlayerAttributePlusScalesFlatAndLevelPower) {
    PlayerAttributeAttackInput input{30, 200, 0, 0, 1.0f, 0.1f, 0};
    EXPECT_DOUBLE_EQ(legacy_player_attribute_attack(input), 203.0);
}

TEST(AttackCalc, JapanAttributeUsesSimMekHalfAndAttributePlus) {
    PlayerAttributeAttackInput input{20, 10, 2, 4, 1.0f, 3.0f, 0};
    EXPECT_DOUBLE_EQ(legacy_player_attribute_attack_japan(input), 30.0);
}

TEST(AttackCalc, MonsterAttributeUsesInclusiveRange) {
    EXPECT_DOUBLE_EQ(legacy_monster_attribute_attack(5, 10, 0), 5.0);
    EXPECT_DOUBLE_EQ(legacy_monster_attribute_attack(5, 10, 5), 10.0);
}

TEST(AttackCalc, TitanAttributeUsesIntegerBaseAnd074Rate) {
    EXPECT_DOUBLE_EQ(legacy_titan_attribute_attack(100, 20, 1.0f, 0), 25.0);
}

TEST(AttackCalc, TitanPlayerPhysicalAddsSixtyPercentMasterPower) {
    PhysicalAttackInput player{10, 10, 1.0f, 0.0f, false,
                               AttackCalcLocale::KoreaChina, 0};
    PhysicalAttackInput titan{20, 20, 1.0f, 0.0f, false,
                              AttackCalcLocale::KoreaChina, 0};
    EXPECT_DOUBLE_EQ(legacy_titan_player_physical_attack(player, titan), 26.0);
}

TEST(AttackCalc, TitanPlayerAttributeAddsSixtyPercentMasterPower) {
    PlayerAttributeAttackInput player{1, 0, 10, 10, 0.0f, 0.0f, 0};
    EXPECT_DOUBLE_EQ(legacy_titan_player_attribute_attack(player, 0, 0,
                                                           1.0f, 0), 6.0);
}

TEST(AttackCalc, DefenceUsesKoreaChinaFormula) {
    DefenceInput input{};
    input.physical_defence = 100.0;
    input.attacker_level = 50;
    EXPECT_DOUBLE_EQ(legacy_phy_defence_level(input), 250.0 / 1150.0);
}

TEST(AttackCalc, DefenceAppliesAttackerUniqueReduction) {
    DefenceInput input{};
    input.physical_defence = 100.0;
    input.attacker_level = 50;
    input.attacker_is_player = true;
    input.enemy_defence_percent = 20;
    EXPECT_DOUBLE_EQ(legacy_phy_defence_level(input), 210.0 / 1150.0);
}

TEST(AttackCalc, DefenceAppliesPlayerAndPartyModifiers) {
    DefenceInput input{};
    input.physical_defence = 100.0;
    input.attacker_level = 50;
    input.target_is_player = true;
    input.target_regist_phys_percent = 10.0;
    input.target_skill_phy_def = 0.2;
    input.party_defence_rate = 1.1;
    EXPECT_DOUBLE_EQ(legacy_phy_defence_level(input), 340.4 / 1150.0);
}

TEST(AttackCalc, DefenceClampsKoreaChinaCeiling) {
    DefenceInput input{};
    input.physical_defence = 100000.0;
    input.attacker_level = 1;
    EXPECT_DOUBLE_EQ(legacy_phy_defence_level(input), 0.9);
}

TEST(AttackCalc, DefenceJapanUsesDifferentDenominatorAndCeiling) {
    DefenceInput input{};
    input.physical_defence = 100.0;
    input.attacker_level = 10;
    input.locale = AttackCalcLocale::Japan;
    EXPECT_DOUBLE_EQ(legacy_phy_defence_level(input), 0.4);
}

TEST(AttackCalc, TitanDefencePreservesLegacyWordComparisonQuirk) {
    DefenceInput input{};
    input.physical_defence = 100.0;
    input.attacker_level = 10;
    input.target_is_player = true;
    input.target_in_titan = true;
    input.titan_defence = 1;
    EXPECT_DOUBLE_EQ(legacy_phy_defence_level(input), 0.8);
}

}  // namespace
}  // namespace mxh::game
