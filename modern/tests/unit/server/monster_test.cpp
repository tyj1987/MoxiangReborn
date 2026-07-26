// monster_test.cpp - unit tests for Monster + AI state machine.

#include "mxh/server/monster.hpp"
#include <gtest/gtest.h>

namespace {
using mxh::server::AiState;
using mxh::server::AiBehavior;
using mxh::server::MonsterInstance;
using mxh::server::BossMonsterInstance;
using mxh::server::should_flee;
using mxh::server::transition_idle_to_chase;
using mxh::server::transition_chase_to_attack;
using mxh::server::attack_cooldown_elapsed;
using mxh::server::transition_on_damage;
using mxh::server::kill_monster;
using mxh::server::ai_tick;

static MonsterInstance make_basic_monster() {
    MonsterInstance m;
    m.max_hp = 1000;
    m.current_hp = 1000;
    m.behavior.search_radius = 5;
    m.behavior.chase_radius = 8;
    m.behavior.attack_interval_ms = 2000;
    m.behavior.flee_hp_percent = 30;
    return m;
}
}

// ---- should_flee ----
TEST(ShouldFlee, BelowThresholdReturnsTrue) {
    EXPECT_TRUE(should_flee(100, 1000, 30));  // 10% < 30%
}

TEST(ShouldFlee, AboveThresholdReturnsFalse) {
    EXPECT_FALSE(should_flee(500, 1000, 30));  // 50% > 30%
}

TEST(ShouldFlee, ZeroFleePctNeverFlees) {
    EXPECT_FALSE(should_flee(10, 1000, 0));
}

TEST(ShouldFlee, ZeroMaxHpNeverFlees) {
    EXPECT_FALSE(should_flee(0, 0, 30));
}

TEST(ShouldFlee, ExactThresholdNotFlee) {
    EXPECT_FALSE(should_flee(300, 1000, 30));  // 30% >= 30%
}

// ---- transition_idle_to_chase ----
TEST(TransitionIdleChase, FromStandWithPlayerEntersRun) {
    auto m = make_basic_monster();
    m.ai_state = AiState::Stand;
    EXPECT_EQ(transition_idle_to_chase(m, 0, 1001), AiState::Run);
}

TEST(TransitionIdleChase, FromWalkWithPlayerEntersRun) {
    auto m = make_basic_monster();
    m.ai_state = AiState::Walk;
    EXPECT_EQ(transition_idle_to_chase(m, 0, 1001), AiState::Run);
}

TEST(TransitionIdleChase, NoPlayerKeepsState) {
    auto m = make_basic_monster();
    m.ai_state = AiState::Stand;
    EXPECT_EQ(transition_idle_to_chase(m, 0, 0), AiState::Stand);
}

TEST(TransitionIdleChase, FromAttackDoesNotChange) {
    auto m = make_basic_monster();
    m.ai_state = AiState::Attack;
    EXPECT_EQ(transition_idle_to_chase(m, 0, 1001), AiState::Attack);
}

TEST(TransitionIdleChase, ZeroSearchRadiusStays) {
    auto m = make_basic_monster();
    m.behavior.search_radius = 0;
    m.ai_state = AiState::Stand;
    EXPECT_EQ(transition_idle_to_chase(m, 0, 1001), AiState::Stand);
}

// ---- attack_cooldown_elapsed ----
TEST(AttackCooldown, ZeroLastAttackAlwaysElapses) {
    auto m = make_basic_monster();
    m.last_attack_ms = 0;
    EXPECT_TRUE(attack_cooldown_elapsed(m, 100));
}

TEST(AttackCooldown, BeforeIntervalNotElapses) {
    auto m = make_basic_monster();
    m.last_attack_ms = 1000;
    // interval = 2000ms; now=1500 not elapsed
    EXPECT_FALSE(attack_cooldown_elapsed(m, 1500));
}

TEST(AttackCooldown, AfterIntervalElapses) {
    auto m = make_basic_monster();
    m.last_attack_ms = 1000;
    EXPECT_TRUE(attack_cooldown_elapsed(m, 3500));  // 2500 >= 2000
}

TEST(AttackCooldown, UsesDefaultIfZero) {
    auto m = make_basic_monster();
    m.behavior.attack_interval_ms = 0;
    m.last_attack_ms = 1000;
    // default = 2000ms
    EXPECT_FALSE(attack_cooldown_elapsed(m, 2500));
    EXPECT_TRUE(attack_cooldown_elapsed(m, 4000));
}

// ---- transition_on_damage ----
TEST(TransitionOnDamage, MinorHitStaysInState) {
    auto m = make_basic_monster();
    m.ai_state = AiState::Run;
    m.target_player_id = 1001;
    auto next = transition_on_damage(m, 100, 1000);
    EXPECT_EQ(next, AiState::Run);
    EXPECT_EQ(m.current_hp, 900u);
}

TEST(TransitionOnDamage, KillingHitEntersDie) {
    auto m = make_basic_monster();
    m.ai_state = AiState::Run;
    auto next = transition_on_damage(m, 1000, 1000);
    EXPECT_EQ(next, AiState::Die);
    EXPECT_EQ(m.current_hp, 0u);
}

TEST(TransitionOnDamage, FleeThresholdEntersRunAway) {
    auto m = make_basic_monster();
    m.ai_state = AiState::Run;
    auto next = transition_on_damage(m, 800, 1000);  // hp 200 = 20% < 30%
    EXPECT_EQ(next, AiState::RunAway);
    EXPECT_EQ(m.current_hp, 200u);
}

TEST(TransitionOnDamage, ZeroDamageIsNoOp) {
    auto m = make_basic_monster();
    m.ai_state = AiState::Run;
    auto next = transition_on_damage(m, 0, 1000);
    EXPECT_EQ(next, AiState::Run);
    EXPECT_EQ(m.current_hp, 1000u);
}

// ---- ai_tick ----
TEST(AiTick, NoneTransitionsToWalk) {
    auto m = make_basic_monster();
    m.ai_state = AiState::None;
    EXPECT_EQ(ai_tick(m, 0), AiState::Walk);
}

TEST(AiTick, StandWithTargetEntersRun) {
    auto m = make_basic_monster();
    m.ai_state = AiState::Stand;
    m.target_player_id = 1001;
    EXPECT_EQ(ai_tick(m, 100), AiState::Run);
}

TEST(AiTick, WalkWithTargetEntersRun) {
    auto m = make_basic_monster();
    m.ai_state = AiState::Walk;
    m.target_player_id = 1001;
    m.last_attack_ms = 0;  // cooldown elapsed
    EXPECT_EQ(ai_tick(m, 100), AiState::Run);
}

TEST(AiTick, RunWithTargetEntersAttack) {
    auto m = make_basic_monster();
    m.ai_state = AiState::Run;
    m.target_player_id = 1001;
    m.last_attack_ms = 0;
    EXPECT_EQ(ai_tick(m, 100), AiState::Attack);
}

TEST(AiTick, AttackToRunWithTarget) {
    auto m = make_basic_monster();
    m.ai_state = AiState::Attack;
    m.target_player_id = 1001;
    EXPECT_EQ(ai_tick(m, 100), AiState::Run);
    EXPECT_EQ(m.last_attack_ms, 100u);
}

TEST(AiTick, AttackToStandNoTarget) {
    auto m = make_basic_monster();
    m.ai_state = AiState::Attack;
    m.target_player_id = 0;
    EXPECT_EQ(ai_tick(m, 100), AiState::Stand);
}

TEST(AiTick, RunWithNoTargetReturnsStand) {
    auto m = make_basic_monster();
    m.ai_state = AiState::Run;
    m.target_player_id = 0;
    EXPECT_EQ(ai_tick(m, 100), AiState::Stand);
}

TEST(AiTick, DieStaysDie) {
    auto m = make_basic_monster();
    m.ai_state = AiState::Die;
    EXPECT_EQ(ai_tick(m, 100), AiState::Die);
}

TEST(AiTick, RunAwayWithRecoveredHpReturnsStand) {
    auto m = make_basic_monster();
    m.ai_state = AiState::RunAway;
    m.current_hp = 800;  // recovered above flee threshold
    EXPECT_EQ(ai_tick(m, 100), AiState::Stand);
}

TEST(AiTick, RunAwayBelowThresholdStays) {
    auto m = make_basic_monster();
    m.ai_state = AiState::RunAway;
    m.current_hp = 100;  // still < 30%
    EXPECT_EQ(ai_tick(m, 100), AiState::RunAway);
}

TEST(AiTick, SkillToRunWithTarget) {
    auto m = make_basic_monster();
    m.ai_state = AiState::Skill;
    m.target_player_id = 1001;
    EXPECT_EQ(ai_tick(m, 100), AiState::Run);
}

// ---- kill_monster ----
TEST(KillMonster, SetsHpToZeroAndStateDie) {
    auto m = make_basic_monster();
    m.current_hp = 500;
    kill_monster(m, 123);
    EXPECT_EQ(m.current_hp, 0u);
    EXPECT_EQ(m.ai_state, AiState::Die);
    EXPECT_EQ(m.state, 2u);
    EXPECT_EQ(m.state_entered_ms, 123u);
}

// ---- BossMonsterInstance ----
TEST(BossMonster, InheritsFromMonsterInstance) {
    BossMonsterInstance b;
    b.base.max_hp = 5000;
    b.base.current_hp = 5000;
    b.stage = 1;
    b.is_field_boss = 1;
    EXPECT_EQ(b.base.max_hp, 5000u);
    EXPECT_EQ(b.stage, 1u);
    EXPECT_EQ(b.is_field_boss, 1u);
}

TEST(BossMonster, CanUseAiStateTransitions) {
    BossMonsterInstance b;
    b.base.max_hp = 1000;
    b.base.current_hp = 1000;
    b.base.behavior.attack_interval_ms = 2000;
    b.base.ai_state = AiState::Run;
    b.base.target_player_id = 1001;
    auto next = ai_tick(b.base, 100);
    EXPECT_EQ(next, AiState::Attack);
}

