#include "mxh/game/battle.hpp"
#include <gtest/gtest.h>
namespace mxh::game {
TEST(BattleRuntime, ResolvesDamageAndConsumesMana) {
 BattleContext c{}; c.attacker_min_damage=10; c.attacker_max_damage=20; c.attacker_mana=50; c.mana_cost=7; c.random_gap=3;
 auto r=resolve_physical_attack(c); EXPECT_TRUE(r.executed); EXPECT_EQ(r.damage,13u); EXPECT_EQ(r.mana_before,50u); EXPECT_EQ(r.mana_after,43u);
}
TEST(BattleRuntime, RejectsInsufficientManaWithoutDamage) {
 BattleContext c{}; c.attacker_min_damage=10; c.attacker_max_damage=20; c.attacker_mana=6; c.mana_cost=7;
 auto r=resolve_physical_attack(c); EXPECT_FALSE(r.executed); EXPECT_EQ(r.damage,0u); EXPECT_EQ(r.mana_after,6u);
}
TEST(BattleRuntime, CriticalUsesLockedFormula) {
 BattleContext c{}; c.attacker_min_damage=10; c.attacker_max_damage=10; c.attacker_mana=1; c.critical=true;
 EXPECT_EQ(resolve_physical_attack(c).damage,15u);
}
}
