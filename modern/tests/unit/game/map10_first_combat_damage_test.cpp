// TYJ-156 Map10 first-combat repro. Synthetic stats only.
//
// The published 2026-10-15 fresh-starter log shows object 50143 staying at
// life 6150 while its shield falls through the 2200s, with SkillSingleResult
// damage 2-4. KNOWN_BUGS records that target's defense as 541. This file
// does not read MonsterList.bin, ItemList.bin, or any .pak.
//
// mxh_server does not build on Linux (windows.h / IOCP), so the shield step
// below is the same arithmetic as MapHandler::apply_monster_damage
// (modern/src/server/map_handler.cpp, the block that subtracts
// min(damage, current_shield) before life). The Windows sibling
// Map10FirstCombat.Level1StarterHitShouldReduceLifeAndKillThroughShield
// calls that function directly.

#include "mxh/game/battle_factory.hpp"
#include "mxh/server/skill_caster.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <random>

namespace {

struct Vitals {
    std::uint32_t life = 0;
    std::uint32_t shield = 0;
    bool dead = false;
};

Vitals apply_shield_before_life(Vitals monster, std::uint32_t damage) {
    if (monster.dead || damage == 0u) return monster;
    const auto shield_damage = std::min(damage, monster.shield);
    monster.shield -= shield_damage;
    const auto life_damage = damage - shield_damage;
    monster.life = life_damage >= monster.life ? 0u : monster.life - life_damage;
    monster.dead = monster.life == 0u;
    return monster;
}

mxh::game::PlayerCombatStats level1_starter() {
    // PlayerEquipmentStats rebuilds the synthetic 11000 sword
    // (MeleeAttackMin 13, MeleeAttackMax 17, gen/min/che/sim 12) to this range.
    mxh::game::PlayerCombatStats attacker;
    attacker.level = 1;
    attacker.has_physical_range = true;
    attacker.phy_attack_min = 29;
    attacker.phy_attack_max = 35;
    attacker.critical_rate = 0;
    attacker.dodge_rate = 0;
    return attacker;
}

mxh::game::PlayerCombatStats map10_defence() {
    mxh::game::PlayerCombatStats monster;
    monster.level = 1;
    monster.phy_defence = 541;
    monster.dodge_rate = 0;
    return monster;
}

}  // namespace

TEST(Map10FirstCombatDamage, Level1StarterHitShouldReduceLifeAndKillThroughShield) {
    constexpr std::uint32_t kLife = 6150u;
    constexpr std::uint32_t kShield = 2291u;
    const auto attacker = level1_starter();
    const auto monster_stats = map10_defence();
    mxh::game::SkillInfo skill{};
    std::mt19937 rng(156u);

    const auto defence = mxh::game::compute_phy_defence_level(
        monster_stats.phy_defence, attacker.level);
    EXPECT_DOUBLE_EQ(defence, 0.9);

    const auto rolled = mxh::server::skill_caster_calculate_monster_damage(
        attacker, monster_stats, skill, rng);
    ASSERT_GT(rolled.damage, 0);
    ASSERT_LT(static_cast<std::uint32_t>(rolled.damage), kShield);

    const auto after = apply_shield_before_life(
        Vitals{kLife, kShield, false},
        static_cast<std::uint32_t>(rolled.damage));
    EXPECT_LT(after.shield, kShield)
        << "rolled damage " << rolled.damage << " was not applied";
    EXPECT_LT(after.life, kLife)
        << "life damage was not applied; rolled=" << rolled.damage
        << " life=" << after.life << " shield=" << after.shield
        << " dead=" << after.dead;

    Vitals fragile{8u, kShield, false};
    std::int32_t last_roll = 0;
    for (int hit = 0; hit < 30 && !fragile.dead; ++hit) {
        const auto blow = mxh::server::skill_caster_calculate_monster_damage(
            attacker, monster_stats, skill, rng);
        last_roll = blow.damage;
        ASSERT_GT(blow.damage, 0) << "hit=" << hit;
        fragile = apply_shield_before_life(
            fragile, static_cast<std::uint32_t>(blow.damage));
    }
    EXPECT_TRUE(fragile.dead)
        << "kill was not triggered after 30 hits; last roll=" << last_roll
        << " life=" << fragile.life << " shield=" << fragile.shield;
}
