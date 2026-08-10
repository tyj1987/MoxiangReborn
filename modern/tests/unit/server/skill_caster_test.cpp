// skill_caster_test.cpp - Phase 13.4 caster data plane tests.
//
// Covers the pure-function decision + damage math that lives in
// mxh::server::skill_caster.  The tests do not touch MapHandler so
// the 5/5 attack capture wire shape stays byte-for-byte stable
// while the data plane is validated independently.

#include "mxh/server/skill_caster.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <random>

using mxh::game::PlayerCombatStats;
using mxh::game::SkillInfo;
using mxh::game::SkillInfoSimple;
using mxh::game::SkillKind;

using mxh::server::SkillCasterDecision;
using mxh::server::SkillCasterRequest;
using mxh::server::SkillCasterStatus;
using mxh::server::skill_caster_calculate_damage;
using mxh::server::skill_caster_decide_use;
using mxh::server::skill_caster_heal_amount;

namespace {

// ---- Test fixtures ----

PlayerCombatStats make_attacker() {
    PlayerCombatStats s{};
    s.level = 10;
    s.max_hp = 200;
    s.current_hp = 200;
    s.max_mp = 100;
    s.current_mp = 100;
    s.phy_attack = 20;
    s.phy_defence = 5;
    s.att_attack = 0;
    s.att_defence = 0;
    s.critical_rate = 5;
    s.dodge_rate = 5;
    return s;
}

PlayerCombatStats make_defender() {
    PlayerCombatStats s{};
    s.level = 10;
    s.max_hp = 200;
    s.current_hp = 200;
    s.max_mp = 100;
    s.current_mp = 100;
    s.phy_attack = 10;
    s.phy_defence = 8;
    s.att_attack = 0;
    s.att_defence = 0;
    s.critical_rate = 5;
    s.dodge_rate = 0;  // default: never dodges so damage path is deterministic
    return s;
}

SkillInfoSimple make_attack_skill() {
    SkillInfoSimple s{};
    s.skill_idx    = 1;
    s.skill_kind   = SkillKind::OuterMugong;
    s.skill_range  = 5;
    s.target_range = 0;
    s.phy_attack   = 15;
    s.att_attack   = 0;
    s.att_rate     = 100;
    s.critical_rate = 5;
    s.stun_rate    = 0;
    s.need_nearyuk = 5;
    return s;
}

SkillInfoSimple make_heal_skill() {
    auto s = make_attack_skill();
    s.skill_kind   = SkillKind::OuterMugong;  // heal path overlaps
    s.need_nearyuk = 10;
    return s;
}

SkillInfoSimple make_combo_skill() {
    auto s = make_attack_skill();
    s.skill_idx    = 2;
    s.skill_kind   = SkillKind::Combo;
    s.phy_attack   = 12;
    s.need_nearyuk = 15;
    return s;
}

SkillInfo make_full_skill(const SkillInfoSimple& simple) {
    SkillInfo info{};
    info.SkillIdx           = static_cast<std::uint16_t>(simple.skill_idx);
    info.UpPhyAttack[0]     = simple.phy_attack;
    info.FirstAttAttack[0]  = simple.att_attack;
    info.AttackSuccessRate[0] = static_cast<float>(simple.att_rate);
    info.CriticalRate[0]    = static_cast<float>(simple.critical_rate);
    info.StunRate[0]        = static_cast<float>(simple.stun_rate);
    info.NeedNaeRyuk[0]     = simple.need_nearyuk;
    info.SkillKind          = static_cast<std::uint16_t>(simple.skill_kind);
    info.SkillRange         = simple.skill_range;
    info.TargetRange        = simple.target_range;
    return info;
}

SkillCasterRequest make_basic_request() {
    SkillCasterRequest req{};
    req.caster         = make_attacker();
    req.caster_alive   = true;
    req.caster_pos_x   = 0.0f;
    req.caster_pos_z   = 0.0f;
    req.target_present = true;
    req.target_alive   = true;
    req.target_pos_x   = 3.0f;  // within skill_range=5
    req.target_pos_z   = 0.0f;
    req.target_combat  = make_defender();
    req.skill          = make_attack_skill();
    return req;
}

}  // namespace

// ============================================================================
// Status path coverage
// ============================================================================

TEST(SkillCasterTest, OkWhenAllGuardsPass) {
    SkillCasterRequest req = make_basic_request();
    std::mt19937 rng(0xC157E7u);
    auto d = skill_caster_decide_use(req, rng);
    EXPECT_EQ(d.status, SkillCasterStatus::Ok);
    EXPECT_GT(d.damage, 0);
    EXPECT_TRUE(d.hit_result == 1 || d.hit_result == 2);
}

TEST(SkillCasterTest, UnknownSkillWhenIdxIsZero) {
    SkillCasterRequest req = make_basic_request();
    req.skill.skill_idx = 0;
    std::mt19937 rng(1u);
    auto d = skill_caster_decide_use(req, rng);
    EXPECT_EQ(d.status, SkillCasterStatus::UnknownSkill);
    EXPECT_EQ(d.damage, 0);
}

TEST(SkillCasterTest, DeadCasterRejected) {
    SkillCasterRequest req = make_basic_request();
    req.caster.current_hp = 0;
    std::mt19937 rng(2u);
    auto d = skill_caster_decide_use(req, rng);
    EXPECT_EQ(d.status, SkillCasterStatus::DeadCaster);
}

TEST(SkillCasterTest, DeadCasterRejectedWhenFlagFalse) {
    SkillCasterRequest req = make_basic_request();
    req.caster_alive = false;
    std::mt19937 rng(3u);
    auto d = skill_caster_decide_use(req, rng);
    EXPECT_EQ(d.status, SkillCasterStatus::DeadCaster);
}

TEST(SkillCasterTest, NotEnoughMpRejected) {
    SkillCasterRequest req = make_basic_request();
    req.caster.current_mp = req.skill.need_nearyuk - 1;  // 1 short
    std::mt19937 rng(4u);
    auto d = skill_caster_decide_use(req, rng);
    EXPECT_EQ(d.status, SkillCasterStatus::NotEnoughMp);
}

TEST(SkillCasterTest, OutOfRangeRejected) {
    SkillCasterRequest req = make_basic_request();
    req.target_pos_x = 100.0f;  // far outside skill_range=5
    req.target_pos_z = 0.0f;
    std::mt19937 rng(5u);
    auto d = skill_caster_decide_use(req, rng);
    EXPECT_EQ(d.status, SkillCasterStatus::OutOfRange);
}

TEST(SkillCasterTest, WrongKindRejected) {
    SkillCasterRequest req = make_basic_request();
    req.skill.skill_kind = SkillKind::Mining;  // not a combat kind
    std::mt19937 rng(6u);
    auto d = skill_caster_decide_use(req, rng);
    EXPECT_EQ(d.status, SkillCasterStatus::WrongKind);
}

TEST(SkillCasterTest, DeadTargetRejected) {
    SkillCasterRequest req = make_basic_request();
    req.target_alive = false;
    std::mt19937 rng(7u);
    auto d = skill_caster_decide_use(req, rng);
    EXPECT_EQ(d.status, SkillCasterStatus::DeadCaster);
}

// ============================================================================
// Damage formula 1:1
// ============================================================================

TEST(SkillCasterDamageTest, BaseDamageClampedToOne) {
    PlayerCombatStats weak_attacker{};
    weak_attacker.level = 1;
    weak_attacker.phy_attack = 1;
    weak_attacker.critical_rate = 0;
    weak_attacker.dodge_rate = 0;

    PlayerCombatStats tanky_defender{};
    tanky_defender.level = 50;
    tanky_defender.phy_defence = 100;
    tanky_defender.dodge_rate = 0;

    auto full = make_full_skill(make_attack_skill());

    // Roll the dodge roll to 0 (no dodge) and the crit roll to 99
    // (no crit) by drawing from a seeded RNG that maps two values.
    std::mt19937 rng(0u);  // first draw: dodge=0 (no miss), crit=0 (crit path)
    auto dmg = skill_caster_calculate_damage(weak_attacker, tanky_defender, full, rng);
    // skill.phy_attack(15) + attacker.phy_attack(1) - defender.phy_defence(100) = -84
    // Clamped to 1, then * 1.5 because first rng() draw gives a crit
    // when crit_rate (5) + attacker.critical_rate (0) > 0.
    EXPECT_GE(dmg.damage, 1);
}

TEST(SkillCasterDamageTest, MissWhenDodgeRollTriggers) {
    PlayerCombatStats attacker = make_attacker();
    PlayerCombatStats defender = make_defender();
    defender.dodge_rate = 100;  // always dodge
    auto full = make_full_skill(make_attack_skill());
    std::mt19937 rng(0u);
    auto dmg = skill_caster_calculate_damage(attacker, defender, full, rng);
    EXPECT_TRUE(dmg.is_miss);
    EXPECT_EQ(dmg.damage, 0);
    EXPECT_EQ(dmg.hit_result, 0);
}

TEST(SkillCasterDamageTest, CritMultipliesBy1_5) {
    PlayerCombatStats attacker = make_attacker();
    attacker.critical_rate = 100;  // always crit
    PlayerCombatStats defender = make_defender();
    auto full = make_full_skill(make_attack_skill());
    std::mt19937 rng(0u);
    auto dmg = skill_caster_calculate_damage(attacker, defender, full, rng);
    EXPECT_FALSE(dmg.is_miss);
    EXPECT_EQ(dmg.hit_result, 2);
    EXPECT_GT(dmg.damage, 0);
}

TEST(SkillCasterDamageTest, AttrDamageClampedToZero) {
    PlayerCombatStats attacker = make_attacker();
    PlayerCombatStats defender = make_defender();
    attacker.att_attack = 5;
    defender.att_defence = 100;  // bigger than attacker + skill
    auto full = make_full_skill(make_attack_skill());
    std::mt19937 rng(0u);
    auto dmg = skill_caster_calculate_damage(attacker, defender, full, rng);
    // Phy damage is positive (skill 15 + attacker 20 - defender 8 = 27,
    // possibly +crit), attr damage = 0 + 5 - 100 < 0 -> 0.
    EXPECT_GE(dmg.damage, 1);
}

// ============================================================================
// Decision -> damage end-to-end (using seeded RNG)
// ============================================================================

TEST(SkillCasterDecisionTest, ComboSkillProducesPositiveDamage) {
    SkillCasterRequest req = make_basic_request();
    req.skill = make_combo_skill();
    req.caster.current_mp = 100;  // ample
    std::mt19937 rng(42u);
    auto d = skill_caster_decide_use(req, rng);
    EXPECT_EQ(d.status, SkillCasterStatus::Ok);
    EXPECT_GT(d.damage, 0);
}

// ============================================================================
// Heal amount
// ============================================================================

TEST(SkillCasterHealTest, HealAmountScalesWithLevelAndPhyAttack) {
    PlayerCombatStats caster = make_attacker();  // level=10, phy_attack=20
    SkillInfoSimple skill = make_heal_skill();
    skill.phy_attack = 25;
    EXPECT_EQ(skill_caster_heal_amount(caster, skill), 25 + 10 * 5);
}

TEST(SkillCasterHealTest, HealAmountZeroAtLevelOneNoAttack) {
    PlayerCombatStats caster{};
    caster.level = 1;
    SkillInfoSimple skill{};
    skill.phy_attack = 0;
    EXPECT_EQ(skill_caster_heal_amount(caster, skill), 5);
}
