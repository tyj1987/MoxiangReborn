// boss_monster_test.cpp - Phase D6 boss state machine regression tests.
//
// Covers BossPhase transitions, create_boss_from_template field mapping,
// apply_boss_damage stage progression, and pick_rage_target tie-break.
// Each TEST pins one observable invariant of the modern boss engine so
// later refactors cannot drift away from the per-monster data + HP%
// thresholds documented in the modern port header files.

#include <gtest/gtest.h>

#include "mxh/server/boss_monster.hpp"
#include "mxh/server/boss_monster_info.hpp"
#include "mxh/server/boss_state.hpp"

#include <cstring>
#include <utility>
#include <vector>

namespace {
using mxh::server::BossMonsterInfo;
using mxh::server::BossMonsterInstance;
using mxh::server::BossPhase;
using mxh::server::apply_boss_damage;
using mxh::server::boss_phase_from_hp;
using mxh::server::boss_phase_is_terminal;
using mxh::server::boss_phase_transition;
using mxh::server::create_boss_from_template;
using mxh::server::pick_rage_target;
using mxh::game::MonsterTemplate;
using mxh::server::AiState;

static MonsterTemplate make_template(std::uint32_t life, std::uint32_t shield = 0) {
    MonsterTemplate t{};
    t.MonsterKind = 1234;
    t.ObjectKind  = mxh::game::OBJECTKIND_BOSS_MONSTER;
    std::strncpy(t.Name, "Boss1", sizeof(t.Name));
    t.Level       = 50;
    t.Life        = life;
    t.Shield      = shield;
    t.ExpPoint    = 999;
    t.AttackMin   = 100;
    t.AttackMax   = 200;
    t.Defense     = 50;
    t.WalkSpeed   = 50.0f;
    t.RunSpeed    = 100.0f;
    t.SearchRange = 600.0f;
    t.DomainRange = 1500.0f;
    t.Aggressive  = true;
    return t;
}

static BossMonsterInfo make_info(std::uint8_t field_boss, std::uint32_t speech = 0) {
    BossMonsterInfo info{};
    info.monster_kind    = 1234;
    info.is_field_boss   = field_boss;
    info.time_limit_ms   = 600000;
    info.killer_limit    = 10;
    info.announce_msg_id = 1;
    info.speech_id_base  = speech;
    return info;
}
}  // namespace

TEST(BossPhaseFromHp, MaxHpZeroReturnsSealed) {
    EXPECT_EQ(boss_phase_from_hp(0, 0), BossPhase::Sealed);
}

TEST(BossPhaseFromHp, ZeroHpReturnsDead) {
    EXPECT_EQ(boss_phase_from_hp(0, 1000), BossPhase::Dead);
}

TEST(BossPhaseFromHp, FullHpReturnsCombat) {
    EXPECT_EQ(boss_phase_from_hp(1000, 1000), BossPhase::Combat);
}

TEST(BossPhaseFromHp, At76PctReturnsCombat) {
    EXPECT_EQ(boss_phase_from_hp(760, 1000), BossPhase::Combat);
}

TEST(BossPhaseFromHp, At75PctReturnsEnraged) {
    EXPECT_EQ(boss_phase_from_hp(750, 1000), BossPhase::Enraged);
}

TEST(BossPhaseFromHp, At51PctReturnsEnraged) {
    EXPECT_EQ(boss_phase_from_hp(510, 1000), BossPhase::Enraged);
}

TEST(BossPhaseFromHp, At50PctReturnsPhase2) {
    EXPECT_EQ(boss_phase_from_hp(500, 1000), BossPhase::Phase2);
}

TEST(BossPhaseFromHp, At26PctReturnsPhase2) {
    EXPECT_EQ(boss_phase_from_hp(260, 1000), BossPhase::Phase2);
}

TEST(BossPhaseFromHp, At25PctReturnsRage) {
    EXPECT_EQ(boss_phase_from_hp(250, 1000), BossPhase::Rage);
}

TEST(BossPhaseFromHp, At1PctReturnsRage) {
    EXPECT_EQ(boss_phase_from_hp(1, 1000), BossPhase::Rage);
}

// ---- boss_phase_is_terminal ----

TEST(BossPhaseTerminal, DeadIsTerminal) {
    EXPECT_TRUE(boss_phase_is_terminal(BossPhase::Dead));
}

TEST(BossPhaseTerminal, NonDeadPhasesNotTerminal) {
    EXPECT_FALSE(boss_phase_is_terminal(BossPhase::Sealed));
    EXPECT_FALSE(boss_phase_is_terminal(BossPhase::Intro));
    EXPECT_FALSE(boss_phase_is_terminal(BossPhase::Combat));
    EXPECT_FALSE(boss_phase_is_terminal(BossPhase::Enraged));
    EXPECT_FALSE(boss_phase_is_terminal(BossPhase::Phase2));
    EXPECT_FALSE(boss_phase_is_terminal(BossPhase::Rage));
    EXPECT_FALSE(boss_phase_is_terminal(BossPhase::Dying));
    EXPECT_FALSE(boss_phase_is_terminal(BossPhase::Recovering));
}

// ---- boss_phase_transition ----

TEST(BossPhaseTransition, DeadStaysDead) {
    EXPECT_EQ(boss_phase_transition(BossPhase::Dead, 0, 1000), BossPhase::Dead);
}

TEST(BossPhaseTransition, SealedZeroHpStaysSealed) {
    EXPECT_EQ(boss_phase_transition(BossPhase::Sealed, 0, 1000), BossPhase::Sealed);
}

TEST(BossPhaseTransition, SealedWithHpAdvancesToIntro) {
    EXPECT_EQ(boss_phase_transition(BossPhase::Sealed, 1000, 1000), BossPhase::Intro);
}

TEST(BossPhaseTransition, CombatAt50PctGoesPhase2) {
    EXPECT_EQ(boss_phase_transition(BossPhase::Combat, 500, 1000), BossPhase::Phase2);
}

TEST(BossPhaseTransition, CombatAt10PctGoesRage) {
    EXPECT_EQ(boss_phase_transition(BossPhase::Combat, 100, 1000), BossPhase::Rage);
}

TEST(BossPhaseTransition, ZeroHpReturnsDying) {
    EXPECT_EQ(boss_phase_transition(BossPhase::Combat, 0, 1000), BossPhase::Dying);
}

// ---- create_boss_from_template ----

TEST(CreateBossFromTemplate, CopiesTemplateVitals) {
    auto tpl = make_template(2000, 500);
    auto info = make_info(0, 777);
    BossMonsterInstance b = create_boss_from_template(1234, tpl, info, 99, 100, 200, 300, 17);
    EXPECT_EQ(b.base.object_id, 99u);
    EXPECT_EQ(b.base.monster_kind, 1234u);
    EXPECT_EQ(b.base.object_kind, mxh::game::OBJECTKIND_BOSS_MONSTER);
    EXPECT_EQ(b.base.map_num, 17u);
    EXPECT_EQ(b.base.pos_x, 100);
    EXPECT_EQ(b.base.pos_y, 200);
    EXPECT_EQ(b.base.pos_z, 300);
    EXPECT_EQ(b.base.spawn_x, 100);
    EXPECT_EQ(b.base.spawn_y, 200);
    EXPECT_EQ(b.base.spawn_z, 300);
    EXPECT_EQ(b.base.current_hp, 2000u);
    EXPECT_EQ(b.base.max_hp, 2000u);
    EXPECT_EQ(b.base.current_shield, 500u);
    EXPECT_EQ(b.base.max_shield, 500u);
    EXPECT_EQ(b.base.exp_reward, 999u);
    EXPECT_EQ(b.base.ai_state, AiState::Stand);
    EXPECT_EQ(b.base.behavior.is_boss, 1u);
    EXPECT_EQ(b.base.behavior.flee_hp_percent, 0u);
    EXPECT_EQ(b.base.behavior.attack_interval_ms, 2000u);
}

TEST(CreateBossFromTemplate, FieldBossFlag) {
    auto tpl = make_template(1000);
    auto info_field = make_info(1, 42);
    BossMonsterInstance f = create_boss_from_template(1, tpl, info_field, 1, 0, 0, 0, 1);
    EXPECT_EQ(f.is_field_boss, 1u);
    EXPECT_EQ(f.speech_id, 42u);
    EXPECT_EQ(f.stage, 0u);
}

TEST(CreateBossFromTemplate, MapBossFlag) {
    auto tpl = make_template(1000);
    auto info_map = make_info(0, 5);
    BossMonsterInstance m = create_boss_from_template(1, tpl, info_map, 2, 0, 0, 0, 1);
    EXPECT_EQ(m.is_field_boss, 0u);
    EXPECT_EQ(m.speech_id, 5u);
}

TEST(CreateBossFromTemplate, NameCopied) {
    auto tpl = make_template(1000);
    auto info = make_info(0);
    BossMonsterInstance b = create_boss_from_template(1, tpl, info, 1, 0, 0, 0, 1);
    EXPECT_STREQ(b.base.name, "Boss1");
}

// ---- apply_boss_damage ----

TEST(ApplyBossDamage, ReducesHp) {
    auto tpl = make_template(1000);
    auto info = make_info(0);
    BossMonsterInstance b = create_boss_from_template(1, tpl, info, 1, 0, 0, 0, 1);
    apply_boss_damage(b, 100, 42u, 1000u);
    EXPECT_EQ(b.base.current_hp, 900u);
}

TEST(ApplyBossDamage, RecordsLastAttacker) {
    auto tpl = make_template(1000);
    auto info = make_info(0);
    BossMonsterInstance b = create_boss_from_template(1, tpl, info, 1, 0, 0, 0, 1);
    apply_boss_damage(b, 100, 42u, 1000u);
    EXPECT_EQ(b.base.last_attacker_id, 42u);
}

TEST(ApplyBossDamage, IgnoresZeroAttacker) {
    auto tpl = make_template(1000);
    auto info = make_info(0);
    BossMonsterInstance b = create_boss_from_template(1, tpl, info, 1, 0, 0, 0, 1);
    apply_boss_damage(b, 100, 0u, 1000u);
    EXPECT_EQ(b.base.last_attacker_id, 0u);
}

TEST(ApplyBossDamage, LethalDamageMarksDead) {
    auto tpl = make_template(1000);
    auto info = make_info(0);
    BossMonsterInstance b = create_boss_from_template(1, tpl, info, 1, 0, 0, 0, 1);
    auto phase = apply_boss_damage(b, 5000, 42u, 1000u);
    EXPECT_EQ(b.base.current_hp, 0u);
    EXPECT_EQ(b.base.ai_state, AiState::Die);
    EXPECT_EQ(b.base.state, 2u);
    EXPECT_EQ(b.stage, 4u);
    EXPECT_EQ(b.base.killer_player_id, 42u);
    EXPECT_EQ(phase, BossPhase::Dying);
}

TEST(ApplyBossDamage, DeadBossReturnsDeadPhase) {
    auto tpl = make_template(1000);
    auto info = make_info(0);
    BossMonsterInstance b = create_boss_from_template(1, tpl, info, 1, 0, 0, 0, 1);
    apply_boss_damage(b, 5000, 42u, 1000u);
    auto phase = apply_boss_damage(b, 1, 99u, 2000u);
    EXPECT_EQ(phase, BossPhase::Dead);
}

TEST(ApplyBossDamage, AdvancesStageOnPhase2) {
    auto tpl = make_template(1000);
    auto info = make_info(0);
    BossMonsterInstance b = create_boss_from_template(1, tpl, info, 1, 0, 0, 0, 1);
    apply_boss_damage(b, 500, 42u, 1000u);
    EXPECT_EQ(b.stage, 2u);
    EXPECT_EQ(b.base.behavior.attack_interval_ms, 1000u);
}

TEST(ApplyBossDamage, AdvancesStageToRage) {
    auto tpl = make_template(1000);
    auto info = make_info(0);
    BossMonsterInstance b = create_boss_from_template(1, tpl, info, 1, 0, 0, 0, 1);
    apply_boss_damage(b, 800, 42u, 1000u);
    EXPECT_EQ(b.stage, 3u);
    EXPECT_EQ(b.base.behavior.attack_interval_ms, 700u);
}

// ---- pick_rage_target ----

TEST(PickRageTarget, EmptyListReturnsZero) {
    EXPECT_EQ(pick_rage_target({}), 0u);
}

TEST(PickRageTarget, SingleEntryReturnsItsId) {
    std::vector<std::pair<std::uint32_t, std::uint32_t>> dmg = {{7, 100}};
    EXPECT_EQ(pick_rage_target(dmg), 7u);
}

TEST(PickRageTarget, HighestDamageWins) {
    std::vector<std::pair<std::uint32_t, std::uint32_t>> dmg = {{1, 50}, {2, 999}, {3, 200}};
    EXPECT_EQ(pick_rage_target(dmg), 2u);
}

TEST(PickRageTarget, TieBrokenBySmallerId) {
    std::vector<std::pair<std::uint32_t, std::uint32_t>> dmg = {{99, 100}, {7, 100}, {42, 100}};
    EXPECT_EQ(pick_rage_target(dmg), 7u);
}

TEST(PickRageTarget, HigherDamageWinsEvenWithLargerId) {
    std::vector<std::pair<std::uint32_t, std::uint32_t>> dmg = {{1, 100}, {2, 100}, {3, 200}};
    EXPECT_EQ(pick_rage_target(dmg), 3u);
}