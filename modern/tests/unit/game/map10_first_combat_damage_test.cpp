// TYJ-156 combat formula. Synthetic numbers only, copied from a read-only
// MonsterList.bin column 18 confirmation. No file bytes.
//
// kind 102 鬼吊 (Map10): level 59, life 6150, table shield 2294
// (a 2026-10-15 LifeNotify showed 2291), defence 541. Per-hit life
// decrease only. Not a kill target.

#include "mxh/game/battle_factory.hpp"
#include "mxh/server/skill_caster.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <random>

namespace {

mxh::game::PlayerCombatStats defender() {
    mxh::game::PlayerCombatStats monster;
    monster.level = 59;
    monster.phy_defence = 541;
    monster.dodge_rate = 0;
    return monster;
}

}  // namespace

TEST(Map10FirstCombatDamage, ComboSplitForStarterRange29Through35) {
    const auto defence = mxh::game::compute_phy_defence_level(541.0, 1);
    EXPECT_DOUBLE_EQ(defence, 0.9);
    constexpr std::uint32_t kShield = 2294u;
    const struct {
        std::uint32_t attack;
        std::uint32_t shield;
        std::uint32_t life;
    } rows[] = {
        {29u, 14u, 1u}, {30u, 15u, 1u}, {31u, 15u, 1u}, {32u, 16u, 1u},
        {33u, 16u, 1u}, {34u, 17u, 1u}, {35u, 17u, 1u},
    };
    std::uint32_t life = 6150u;
    std::uint32_t shield = kShield;
    mxh::game::PlayerCombatStats attacker;
    attacker.level = 1;
    attacker.has_physical_range = true;
    attacker.critical_rate = 0;
    attacker.dodge_rate = 0;
    mxh::game::SkillInfo skill{};  // SkillKind::Combo, ratio 0.5
    const auto stats = defender();
    for (const auto& row : rows) {
        const auto split = mxh::server::original_monster_physical_split(
            static_cast<double>(row.attack), 0.5f, shield, defence);
        EXPECT_EQ(split.shield_damage, row.shield) << row.attack;
        EXPECT_EQ(split.shield_absorbed, row.shield) << row.attack;
        EXPECT_EQ(split.real_damage, row.life) << row.attack;
        attacker.phy_attack_min = attacker.phy_attack_max = row.attack;
        std::mt19937 rng(1u);
        const auto rolled = mxh::server::skill_caster_calculate_monster_damage(
            attacker, stats, skill, rng, shield);
        EXPECT_EQ(rolled.damage, static_cast<std::int32_t>(row.life));
        EXPECT_EQ(rolled.shield_damage, row.shield);
        EXPECT_EQ(rolled.shield_absorbed, row.shield);
        EXPECT_GE(life - row.life, 1u);
        life -= row.life;
        shield -= row.shield;
    }
    EXPECT_EQ(life, 6143u);
    EXPECT_EQ(shield, 2294u - (14u + 15u + 15u + 16u + 16u + 17u + 17u));
}

// Isolated so a later spawn-map finding can replace the row.
// Source: MonsterList.bin column 18, values only.
struct BeginnerColumn18 {
    const char* name;  // empty when the confirmation omitted the display name
    std::uint16_t kind;
    std::uint8_t level;
    std::uint32_t life;
    std::uint32_t shield;
    std::uint16_t defense;
};

TEST(BeginnerColumn18Kill, OriginalComboSplitKillsConfirmedRows) {
    const BeginnerColumn18 rows[] = {
        // Map17 start area.
        {"小呆呆", 1u, 1u, 53u, 14u, 10u},
        // Nearest monster to the classic Distribute login point (8303,9403).
        // Display name was not part of the confirmation.
        {"", 6u, 6u, 188u, 92u, 25u},
    };
    mxh::game::PlayerCombatStats attacker;
    attacker.level = 1;
    attacker.has_physical_range = true;
    attacker.phy_attack_min = 29;
    attacker.phy_attack_max = 35;
    attacker.critical_rate = 0;
    attacker.dodge_rate = 0;
    mxh::game::SkillInfo skill{};
    for (const auto& row : rows) {
        std::uint32_t life = row.life;
        std::uint32_t shield = row.shield;
        mxh::game::PlayerCombatStats monster;
        monster.level = row.level;
        monster.phy_defence = row.defense;
        monster.dodge_rate = 0;
        bool died = false;
        for (int hit = 0; hit < 40 && !died; ++hit) {
            std::mt19937 rng(static_cast<std::uint32_t>(hit + 1));
            const auto rolled = mxh::server::skill_caster_calculate_monster_damage(
                attacker, monster, skill, rng, shield);
            ASSERT_GE(rolled.damage, 1) << "kind=" << row.kind << " hit=" << hit;
            const auto absorbed = std::min(rolled.shield_absorbed, shield);
            shield -= absorbed;
            const auto life_damage = static_cast<std::uint32_t>(rolled.damage);
            life = life_damage >= life ? 0u : life - life_damage;
            died = life == 0u;
        }
        EXPECT_TRUE(died) << "kind=" << row.kind << " name=" << row.name
                          << " life=" << life << " shield=" << shield;
    }
}

TEST(Map10FirstCombatDamage, ShortShieldUsesObjectCalcShieldDamage) {
    // Object.cpp:525-528: reported ShieldDamage stays the request, the
    // shield is zeroed, and the attack is reduced only by (WORD)Shield.
    const auto defence = mxh::game::compute_phy_defence_level(541.0, 1);
    const auto split = mxh::server::original_monster_physical_split(
        29.0, 0.5f, 10u, defence);
    EXPECT_EQ(split.shield_damage, 14u);
    EXPECT_EQ(split.shield_absorbed, 10u);
    EXPECT_EQ(split.real_damage, 1u);
}
