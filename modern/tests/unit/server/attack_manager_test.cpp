// attack_manager_test.cpp - Phase D6 AttackManager 1:1 port tests.
//
// Tests pin legacy formula behaviour:
//   * GetJinbubDamage (true damage) rate + PvP reduction
//   * GetPenaltyDemege (5%/50% titan-vs-monster)
//   * recover_life / recover_shield / recover_naeryuk cap math
//   * apply_shield_cap, apply_combo_shield_split, apply_mugong_shield_split
//   * resolve_attack convenience pipeline
//   * damage_rate multiplier

#include "mxh/server/attack_manager.hpp"

#include <gtest/gtest.h>

namespace {

using mxh::server::AttackCalcParams;
using mxh::server::DamageSplit;
using mxh::server::ResolvedAttack;
using mxh::server::ResultInfo;
using mxh::server::apply_combo_shield_split;
using mxh::server::apply_damage_rate;
using mxh::server::apply_mugong_shield_split;
using mxh::server::apply_shield_cap;
using mxh::server::eObjectKind_Monster;
using mxh::server::eObjectKind_Player;
using mxh::server::eObjectKind_TitanMonster;
using mxh::server::get_jinbub_damage;
using mxh::server::get_penalty_damage;
using mxh::server::recover_life;
using mxh::server::recover_naeryuk;
using mxh::server::recover_shield;
using mxh::server::resolve_attack;
using mxh::server::SHIELD_COMBO_DAMAGE;
using mxh::server::SHIELD_OUT_MUGONG_DAMAGE;

TEST(AttackManagerJinbub, MultipliesByDecreaseRate) {
    EXPECT_EQ(get_jinbub_damage(1000u, 0.5f, eObjectKind_Player, eObjectKind_Monster, false), 500u);
}

TEST(AttackManagerJinbub, FullRateReturnsExact) {
    EXPECT_EQ(get_jinbub_damage(1234u, 1.0f, eObjectKind_Player, eObjectKind_Monster, false), 1234u);
}

TEST(AttackManagerJinbub, PvPHalvesInNonJapanBuild) {
    // CN/KR: PvP true damage is halved.
    EXPECT_EQ(get_jinbub_damage(1000u, 1.0f, eObjectKind_Player, eObjectKind_Player, false), 500u);
}

TEST(AttackManagerJinbub, PvPKeepsFullDamageInJapanBuild) {
    // _JAPAN_LOCAL_: PvP true damage is kept full.
    EXPECT_EQ(get_jinbub_damage(1000u, 1.0f, eObjectKind_Player, eObjectKind_Player, true), 1000u);
}

TEST(AttackManagerJinbub, PveKeepsFullDamage) {
    EXPECT_EQ(get_jinbub_damage(800u, 1.0f, eObjectKind_Monster, eObjectKind_Player, false), 800u);
}

TEST(AttackManagerPenalty, PlayerVsMonsterNoTitanKeepsDamage) {
    EXPECT_EQ(get_penalty_damage(1000u, eObjectKind_Player, eObjectKind_Monster, false), 1000u);
}

TEST(AttackManagerPenalty, PlayerOnTitanMonsterWithoutTitanGivesFivePercent) {
    // Legacy: 5% reduction.
    EXPECT_EQ(get_penalty_damage(1000u, eObjectKind_Player, eObjectKind_TitanMonster, false), 50u);
}

TEST(AttackManagerPenalty, PlayerInTitanVsNormalMonsterGivesFiftyPercent) {
    // Legacy: 50% reduction.
    EXPECT_EQ(get_penalty_damage(1000u, eObjectKind_Player, eObjectKind_Monster, true), 500u);
}

TEST(AttackManagerPenalty, PlayerInTitanVsTitanMonsterKeepsDamage) {
    EXPECT_EQ(get_penalty_damage(1000u, eObjectKind_Player, eObjectKind_TitanMonster, true), 1000u);
}

TEST(AttackManagerPenalty, MonsterVsAnythingKeepsDamage) {
    EXPECT_EQ(get_penalty_damage(777u, eObjectKind_Monster, eObjectKind_Player, false), 777u);
}

TEST(AttackManagerRecover, LifeCapsAtMaxMinusCurrent) {
    EXPECT_EQ(recover_life(500u, 100u, 1000u), 500u);
    EXPECT_EQ(recover_life(5000u, 100u, 1000u), 900u);  // capped at room
}

TEST(AttackManagerRecover, LifeAtMaxReturnsZero) {
    EXPECT_EQ(recover_life(500u, 1000u, 1000u), 0u);
}

TEST(AttackManagerRecover, ShieldCapsAtMaxMinusCurrent) {
    EXPECT_EQ(recover_shield(100u, 50u, 200u), 100u);
    EXPECT_EQ(recover_shield(500u, 100u, 200u), 100u);
}

TEST(AttackManagerRecover, NaeryukCapsAtMaxMinusCurrent) {
    EXPECT_EQ(recover_naeryuk(50u, 80u, 200u), 50u);
    EXPECT_EQ(recover_naeryuk(500u, 100u, 200u), 100u);
}

TEST(AttackManagerShield, ApplyShieldCapSplitsDamage) {
    auto split = apply_shield_cap(100u, 30u);
    EXPECT_EQ(split.shield_chunk, 30u);
    EXPECT_EQ(split.life_chunk, 70u);
}

TEST(AttackManagerShield, ApplyShieldCapNoShieldGoesToLife) {
    auto split = apply_shield_cap(100u, 0u);
    EXPECT_EQ(split.shield_chunk, 0u);
    EXPECT_EQ(split.life_chunk, 100u);
}

TEST(AttackManagerShield, ComboShieldSplitIsFiftyPercent) {
    EXPECT_FLOAT_EQ(SHIELD_COMBO_DAMAGE, 0.5f);
    auto split = apply_combo_shield_split(1000u);
    EXPECT_EQ(split.shield_chunk, 500u);
    EXPECT_EQ(split.life_chunk, 500u);
}

TEST(AttackManagerShield, MugongShieldSplitIsSeventyPercent) {
    EXPECT_FLOAT_EQ(SHIELD_OUT_MUGONG_DAMAGE, 0.7f);
    auto split = apply_mugong_shield_split(1000u);
    EXPECT_EQ(split.shield_chunk, 700u);
    EXPECT_EQ(split.life_chunk, 300u);
}

TEST(AttackManagerRate, ApplyDamageRateAt100Percent) {
    EXPECT_EQ(apply_damage_rate(500u, 100.0f), 500u);
}

TEST(AttackManagerRate, ApplyDamageRateAtZeroReturnsZero) {
    EXPECT_EQ(apply_damage_rate(500u, 0.0f), 0u);
}

TEST(AttackManagerRate, ApplyDamageRateAtFiftyPercent) {
    EXPECT_EQ(apply_damage_rate(500u, 50.0f), 250u);
}

TEST(AttackManagerResolve, RatePenaltyShieldPipeline) {
    AttackCalcParams params{};
    params.attacker_kind = eObjectKind_Player;
    params.target_kind = eObjectKind_TitanMonster;
    params.attacker_in_titan = false;
    params.target_shield = 40u;
    // raw=1000, rate=100% -> 1000, penalty 5% -> 50, shield 40 -> split.
    ResolvedAttack out = resolve_attack(1000u, 100.0f, params);
    EXPECT_EQ(out.real_damage, 50u);
    EXPECT_EQ(out.shield_chunk, 40u);
    EXPECT_EQ(out.life_chunk, 10u);
}

TEST(AttackManagerResolve, HalfRatePenaltyPipeline) {
    AttackCalcParams params{};
    params.attacker_kind = eObjectKind_Player;
    params.target_kind = eObjectKind_Monster;
    params.attacker_in_titan = true;
    params.target_shield = 100u;
    // raw=1000, rate=50% -> 500, penalty 50% -> 250, shield 100 -> split.
    ResolvedAttack out = resolve_attack(1000u, 50.0f, params);
    EXPECT_EQ(out.real_damage, 250u);
    EXPECT_EQ(out.shield_chunk, 100u);
    EXPECT_EQ(out.life_chunk, 150u);
}

}  // namespace
