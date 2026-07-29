// monster_ext_test.cpp - Phase 6.2 Monster + BossMonster + Drop + Speech tests.
//
// 1:1 byte/field invariants for the extended MonsterInstance, the
// BossMonster trio (state/info/manager), the FieldBossMonsterManager,
// the cMonsterSpeechManager and the DropTableRegistry.

#include "mxh/server/monster.hpp"
#include "mxh/server/boss_state.hpp"
#include "mxh/server/boss_monster.hpp"
#include "mxh/server/boss_monster_info.hpp"
#include "mxh/server/boss_monster_manager.hpp"
#include "mxh/server/field_boss_monster.hpp"
#include "mxh/server/c_monster_speech_manager.hpp"
#include "mxh/server/drop_item.hpp"
#include "mxh/game/monster_types.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <utility>
#include <vector>

namespace {

using mxh::server::AiState;
using mxh::server::MonsterInstance;
using mxh::server::BossMonsterInstance;
using mxh::server::BossPhase;
using mxh::server::BossMonsterInfo;
using mxh::server::BossMonsterManager;
using mxh::server::boss_phase_from_hp;
using mxh::server::boss_phase_transition;
using mxh::server::boss_stage_from_hp;
using mxh::server::update_boss_stage;
using mxh::server::apply_boss_damage;
using mxh::server::create_boss_from_template;
using mxh::server::pick_rage_target;
using mxh::server::FieldBossMonsterManager;
using mxh::server::cMonsterSpeechManager;
using mxh::server::MonsterSpeech;
using mxh::server::DropTableRegistry;
using mxh::server::DropTable;
using mxh::server::DropItemEntry;

mxh::game::MonsterTemplate make_template(std::uint16_t kind, std::uint32_t life) {
    mxh::game::MonsterTemplate t{};
    t.MonsterKind = kind;
    t.ObjectKind  = mxh::game::OBJECTKIND_MONSTER;
    std::strncpy(t.Name, "TestMonster", sizeof(t.Name) - 1);
    t.Level       = 10;
    t.Life        = life;
    t.Shield      = 0;
    t.ExpPoint    = 50;
    t.AttackMin   = 10;
    t.AttackMax   = 20;
    t.Defense     = 5;
    t.WalkSpeed   = 50.0f;
    t.RunSpeed    = 100.0f;
    t.SearchRange = 400.0f;
    t.DomainRange = 800.0f;
    return t;
}

}  // namespace

// --- MonsterInstance 1:1 fields ---
TEST(MonsterInstanceFields, LegacyDefaultsAreZero) {
    MonsterInstance m;
    EXPECT_EQ(m.drop_item_id, 0u);
    EXPECT_EQ(m.drop_item_ratio, 100u);
    EXPECT_EQ(m.sub_id, 0u);
    EXPECT_EQ(m.regen_num, 0u);
    EXPECT_EQ(m.suryun_group, 0);
    EXPECT_FALSE(m.event_mob);
    EXPECT_EQ(m.killer_player_id, 0u);
    EXPECT_EQ(m.last_attacker_id, 0u);
}

TEST(MonsterInstanceFields, NameBufferMatchesLegacy) {
    MonsterInstance m;
    std::size_t cap = sizeof(m.name);
    EXPECT_EQ(cap, mxh::game::MAX_MONSTER_NAME_LENGTH + 1u);
}

TEST(MonsterInstanceFields, MakeTotalInfoPacks14Bytes) {
    MonsterInstance m;
    m.current_hp     = 800;
    m.current_shield = 12;
    m.monster_kind   = 42;
    m.group          = 7;
    m.map_num        = 3;
    auto info = m.make_totalinfo();
    EXPECT_EQ(info.Life, 800u);
    EXPECT_EQ(info.Shield, 12u);
    EXPECT_EQ(info.MonsterKind, 42u);
    EXPECT_EQ(info.Group, 7u);
    EXPECT_EQ(info.MapNum, 3u);
    EXPECT_EQ(sizeof(info), 14u);
}

// --- BossState helpers ---
TEST(BossState, HpPercentMapsToExpectedPhase) {
    EXPECT_EQ(boss_phase_from_hp(100, 100), BossPhase::Combat);
    EXPECT_EQ(boss_phase_from_hp(76, 100),  BossPhase::Combat);
    EXPECT_EQ(boss_phase_from_hp(75, 100),  BossPhase::Enraged);
    EXPECT_EQ(boss_phase_from_hp(60, 100),  BossPhase::Enraged);
    EXPECT_EQ(boss_phase_from_hp(51, 100),  BossPhase::Enraged);
    EXPECT_EQ(boss_phase_from_hp(50, 100),  BossPhase::Phase2);
    EXPECT_EQ(boss_phase_from_hp(30, 100),  BossPhase::Phase2);
    EXPECT_EQ(boss_phase_from_hp(26, 100),  BossPhase::Phase2);
    EXPECT_EQ(boss_phase_from_hp(25, 100),  BossPhase::Rage);
    EXPECT_EQ(boss_phase_from_hp(1,  100),  BossPhase::Rage);
    EXPECT_EQ(boss_phase_from_hp(0,  100),  BossPhase::Dead);
}

TEST(BossState, TransitionFromSealedToIntroOnFirstHp) {
    EXPECT_EQ(boss_phase_transition(BossPhase::Sealed, 100, 100), BossPhase::Intro);
}

TEST(BossState, TransitionDyingWhenHpReachesZero) {
    EXPECT_EQ(boss_phase_transition(BossPhase::Combat, 0, 100), BossPhase::Dying);
}

TEST(BossState, TerminalOnlyForDead) {
    EXPECT_FALSE(boss_phase_is_terminal(BossPhase::Combat));
    EXPECT_FALSE(boss_phase_is_terminal(BossPhase::Enraged));
    EXPECT_TRUE(boss_phase_is_terminal(BossPhase::Dead));
}

// --- BossMonsterInstance: build + apply damage + stage ---
TEST(BossMonsterBuild, AssignsBaseFromTemplateAndInfo) {
    auto tpl = make_template(5, 1000);
    BossMonsterInfo info{};
    info.monster_kind = 5;
    info.is_field_boss = 0;
    info.time_limit_ms = 60000;
    info.killer_limit  = 20;
    info.speech_id_base = 1234;
    BossMonsterInstance b = create_boss_from_template(5, tpl, info,
                                                       7, 100, 0, 200, 3);
    EXPECT_EQ(b.base.object_id, 7u);
    EXPECT_EQ(b.base.monster_kind, 5u);
    EXPECT_EQ(b.base.current_hp, 1000u);
    EXPECT_EQ(b.base.max_hp, 1000u);
    EXPECT_EQ(b.base.exp_reward, 50u);
    EXPECT_EQ(b.base.behavior.is_boss, 1u);
    EXPECT_EQ(b.stage, 0u);
    EXPECT_EQ(b.is_field_boss, 0u);
    EXPECT_EQ(b.speech_id, 1234u);
    EXPECT_EQ(b.base.spawn_x, 100);
    EXPECT_EQ(b.base.spawn_y, 0);
    EXPECT_EQ(b.base.spawn_z, 200);
}

TEST(BossMonsterBuild, ApplyDamageAdvancesStages) {
    auto tpl = make_template(2, 1000);
    BossMonsterInfo info{};
    auto b = create_boss_from_template(2, tpl, info, 1, 0, 0, 0, 1);
    auto p1 = apply_boss_damage(b, 300, 1001, 100);  // 700/1000 = 70% -> Enraged
    EXPECT_EQ(p1, BossPhase::Enraged);
    EXPECT_EQ(b.stage, 1u);
    auto p2 = apply_boss_damage(b, 300, 1001, 100);  // 400/1000 = 40% -> Phase2
    EXPECT_EQ(p2, BossPhase::Phase2);
    EXPECT_EQ(b.stage, 2u);
    auto p3 = apply_boss_damage(b, 200, 1001, 100);  // 200/1000 = 20% -> Rage
    EXPECT_EQ(p3, BossPhase::Rage);
    EXPECT_EQ(b.stage, 3u);
    auto p4 = apply_boss_damage(b, 999, 1001, 100);
    EXPECT_EQ(p4, BossPhase::Dying);
    EXPECT_EQ(b.base.ai_state, AiState::Die);
    EXPECT_EQ(b.base.killer_player_id, 1001u);
}

TEST(BossMonsterBuild, PickRageTargetFindsHighestDamage) {
    std::vector<std::pair<std::uint32_t, std::uint32_t>> contrib = {
        {100, 50}, {200, 150}, {300, 120}, {400, 80},
    };
    EXPECT_EQ(pick_rage_target(contrib), 200u);
}

TEST(BossMonsterBuild, PickRageTargetBreaksTiesByLowerPlayerId) {
    std::vector<std::pair<std::uint32_t, std::uint32_t>> contrib = {
        {500, 99}, {200, 99},
    };
    EXPECT_EQ(pick_rage_target(contrib), 200u);
}

TEST(BossMonsterStage, HelperHitsAllStages) {
    EXPECT_EQ(boss_stage_from_hp(1000, 1000), 0u);
    EXPECT_EQ(boss_stage_from_hp(76, 100), 0u);
    EXPECT_EQ(boss_stage_from_hp(75, 100), 1u);
    EXPECT_EQ(boss_stage_from_hp(60, 100), 1u);
    EXPECT_EQ(boss_stage_from_hp(50, 100), 2u);
    EXPECT_EQ(boss_stage_from_hp(30, 100), 2u);
    EXPECT_EQ(boss_stage_from_hp(25, 100), 3u);
    EXPECT_EQ(boss_stage_from_hp(1,  100), 3u);
    EXPECT_EQ(boss_stage_from_hp(0,  100), 4u);
}

TEST(BossMonsterStage, UpdateHonorsTransitionsAndDie) {
    auto tpl = make_template(3, 1000);
    BossMonsterInfo info{};
    auto b = create_boss_from_template(3, tpl, info, 11, 0, 0, 0, 1);
    b.base.current_hp = 250;
    update_boss_stage(b, 100);
    EXPECT_EQ(b.stage, 3u);
    b.base.current_hp = 300;
    update_boss_stage(b, 200);
    EXPECT_EQ(b.stage, 2u);
    b.base.current_hp = 0;
    update_boss_stage(b, 300);
    EXPECT_EQ(b.base.ai_state, AiState::Die);
    EXPECT_EQ(b.stage, 4u);
}

// --- BossMonsterManager ---
TEST(BossMonsterManagerTest, SpawnAssignsObjectId) {
    auto tpl = make_template(1, 500);
    BossMonsterInfo info{};
    info.monster_kind = 1;
    info.time_limit_ms = 5000;
    BossMonsterManager mgr;
    mgr.register_info(1, info);
    auto id = mgr.spawn(1, tpl, 4242, 0, 0, 0, 1);
    EXPECT_EQ(id, 4242u);
    EXPECT_EQ(mgr.live_count(), 1u);
    auto* b = mgr.find(4242);
    ASSERT_NE(b, nullptr);
    EXPECT_EQ(b->base.monster_kind, 1u);
}

TEST(BossMonsterManagerTest, SpawnFailsWithoutInfo) {
    BossMonsterManager mgr;
    auto id = mgr.spawn(99, make_template(99, 1), 1, 0, 0, 0, 1);
    EXPECT_EQ(id, 0u);
    EXPECT_EQ(mgr.live_count(), 0u);
}

TEST(BossMonsterManagerTest, DamageAndErase) {
    auto tpl = make_template(2, 1000);
    BossMonsterInfo info{};
    BossMonsterManager mgr;
    mgr.register_info(2, info);
    mgr.spawn(2, tpl, 100, 0, 0, 0, 1);
    auto phase = mgr.damage(100, 600, 1234, 100);
    EXPECT_EQ(phase, BossPhase::Phase2);
    EXPECT_EQ(mgr.live_count(), 1u);
    phase = mgr.damage(100, 999, 1234, 100);
    EXPECT_EQ(phase, BossPhase::Dying);
    EXPECT_TRUE(mgr.erase(100));
    EXPECT_EQ(mgr.live_count(), 0u);
    EXPECT_EQ(mgr.erase(100), false);
}

// --- FieldBossMonsterManager ---
TEST(FieldBossMonsterManagerTest, ConfigureAndSchedule) {
    FieldBossMonsterManager mgr;
    mgr.configure_channel(0, 5, 60000, 100, 0, 0, 0, 1);
    auto* ch = mgr.channel(0);
    ASSERT_NE(ch, nullptr);
    EXPECT_EQ(ch->monster_kind, 5u);
    EXPECT_EQ(ch->respawn_interval_ms, 60000u);
    EXPECT_EQ(ch->next_spawn_ms, 100u);
    EXPECT_FALSE(ch->active);
}

TEST(FieldBossMonsterManagerTest, TickFiresOnceWhenReady) {
    FieldBossMonsterManager mgr;
    auto tpl = make_template(7, 100);
    mgr.configure_channel(0, 7, 60000, 100, 0, 0, 0, 1);
    bool fired = mgr.tick(150, {tpl}, 9000);
    EXPECT_TRUE(fired);
    EXPECT_EQ(mgr.last_spawn_object_id(), 9000u);
    auto* ch = mgr.channel(0);
    EXPECT_TRUE(ch->active);
    EXPECT_EQ(ch->current_object_id, 9000u);
    EXPECT_GE(ch->next_spawn_ms, 150u);
}

TEST(FieldBossMonsterManagerTest, TickNoFireWhenDisabled) {
    FieldBossMonsterManager mgr;
    auto tpl = make_template(7, 100);
    mgr.configure_channel(0, 0, 0, 0, 0, 0, 0, 0);
    EXPECT_FALSE(mgr.tick(150, {tpl}, 1));
}

TEST(FieldBossMonsterManagerTest, TickNoFireWhenTooEarly) {
    FieldBossMonsterManager mgr;
    auto tpl = make_template(7, 100);
    mgr.configure_channel(0, 7, 10000, 1000, 0, 0, 0, 1);
    EXPECT_FALSE(mgr.tick(150, {tpl}, 1));
    EXPECT_TRUE(mgr.tick(1500, {tpl}, 1));
}

// --- cMonsterSpeechManager ---
TEST(MonsterSpeechManagerTest, RegisterAndFind) {
    cMonsterSpeechManager mgr;
    MonsterSpeech s1{};
    s1.speech_id = 100;
    s1.monster_kind = 5;
    s1.trigger = 1;
    std::strncpy(s1.text, "I shall smite thee", sizeof(s1.text) - 1);
    mgr.register_speech(s1);
    MonsterSpeech s2{};
    s2.speech_id = 101;
    s2.monster_kind = 5;
    s2.trigger = 2;
    std::strncpy(s2.text, "Argh!", sizeof(s2.text) - 1);
    mgr.register_speech(s2);
    EXPECT_EQ(mgr.size(), 2u);
    auto* f = mgr.find(5, 1);
    ASSERT_NE(f, nullptr);
    EXPECT_EQ(f->speech_id, 100u);
    EXPECT_STREQ(f->text, "I shall smite thee");
    auto* d = mgr.death_speech(5);
    ASSERT_NE(d, nullptr);
    EXPECT_EQ(d->speech_id, 101u);
    EXPECT_EQ(mgr.find(99, 1), nullptr);
}

// --- DropTableRegistry ---
TEST(DropTableRegistryTest, AddAndFind) {
    DropTableRegistry reg;
    DropTable t{};
    t.drop_id = 1;
    t.monster_kind = 7;
    DropItemEntry e1; e1.item_id = 100; e1.ratio = 50; t.entries.push_back(e1);
    DropItemEntry e2; e2.item_id = 200; e2.ratio = 50; t.entries.push_back(e2);
    reg.add(t);
    EXPECT_EQ(reg.size(), 1u);
    auto* f = reg.find(7, 1);
    ASSERT_NE(f, nullptr);
    EXPECT_EQ(f->entries.size(), 2u);
    EXPECT_EQ(reg.find(7, 2), nullptr);
}

TEST(DropTableRegistryTest, RollReturnsExpectedItem) {
    DropTableRegistry reg;
    DropTable t{};
    t.drop_id = 1;
    t.monster_kind = 7;
    DropItemEntry e1; e1.item_id = 100; e1.ratio = 70; t.entries.push_back(e1);
    DropItemEntry e2; e2.item_id = 200; e2.ratio = 30; t.entries.push_back(e2);
    reg.add(t);
    EXPECT_EQ(reg.roll(7, 1, 0),    100u);
    EXPECT_EQ(reg.roll(7, 1, 69),   100u);
    EXPECT_EQ(reg.roll(7, 1, 70),   200u);
    EXPECT_EQ(reg.roll(7, 1, 99),   200u);
    EXPECT_EQ(reg.roll(8, 1, 0),    0u);
}

TEST(DropTableRegistryTest, RollEmptyTableReturnsZero) {
    DropTableRegistry reg;
    EXPECT_EQ(reg.roll(0, 0, 0), 0u);
}
