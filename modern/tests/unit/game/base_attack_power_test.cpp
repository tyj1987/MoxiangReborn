#include "mxh/game/base_attack_power.hpp"
#include <gtest/gtest.h>

using mxh::game::base_attack_power;
using mxh::game::BaseAttackLocale;

TEST(BaseAttackPower, SourceGoldenVectorsUseDoubleAndTruncateOnce) {
    // Manually evaluated recovered formula: 13.247700, 25.804020, 32.082180.
    // These are explicit stat=10 examples, NOT approved character defaults.
    EXPECT_EQ(base_attack_power(10, 5), 13u);
    EXPECT_EQ(base_attack_power(10, 13), 25u);
    EXPECT_EQ(base_attack_power(10, 17), 32u);
    EXPECT_EQ(base_attack_power(200, 100), 528u);
}

TEST(BaseAttackPower, StatAdjustmentChangesSignAndCapsAtThirtySeven) {
    EXPECT_EQ(base_attack_power(11, 13), 27u);
    EXPECT_EQ(base_attack_power(12, 13), 29u);
    EXPECT_EQ(base_attack_power(36, 13), 74u);
    EXPECT_EQ(base_attack_power(37, 13), 76u);
    EXPECT_EQ(base_attack_power(38, 13), 76u);
}

TEST(BaseAttackPower, WordMaximumDoesNotOverflowIntermediateArithmetic) {
    EXPECT_EQ(base_attack_power(65535, 65535), 2121103139u);
}

TEST(BaseAttackPower, JapanUsesSeparateFormulaWithoutCallerBonuses) {
    EXPECT_EQ(base_attack_power(10, 13, BaseAttackLocale::Japan), 26u);
    EXPECT_EQ(base_attack_power(12, 13, BaseAttackLocale::Japan), 29u);
    EXPECT_EQ(base_attack_power(0, 0, BaseAttackLocale::Japan), 0u);
    EXPECT_EQ(base_attack_power(65535, 65535, BaseAttackLocale::Japan), 152915u);
}

TEST(BaseAttackPower, InvalidNegativeResultDoesNotBecomeHugeUnsignedAttack) {
    EXPECT_FALSE(base_attack_power(0, 0).has_value());
    EXPECT_FALSE(base_attack_power(0, 5).has_value());
    EXPECT_EQ(base_attack_power(0, 9), 1u);
}
