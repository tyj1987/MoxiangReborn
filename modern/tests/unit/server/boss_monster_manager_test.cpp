// 1:1 lock tests for BossMonsterManager (Phase D6 Boss 刷新).
// The manager aggregates per-MonsterKind BossMonsterInfo (static, registered once)
// and BossMonsterInstance (live, spawned and erased at runtime). All tests here are
// pure logic: no DB, no network, no threads.

#include "mxh/server/boss_monster.hpp"
#include "mxh/server/boss_monster_info.hpp"
#include "mxh/server/boss_monster_manager.hpp"
#include "mxh/server/boss_state.hpp"

#include <gtest/gtest.h>

#include <cstring>

namespace {

using mxh::server::BossMonsterInfo;
using mxh::server::BossMonsterInstance;
using mxh::server::BossMonsterManager;
using mxh::server::BossPhase;
using mxh::server::BossReward;
using mxh::game::MonsterTemplate;

MonsterTemplate make_template(std::uint32_t kind, std::uint32_t life = 1000) {
    MonsterTemplate t{};
    t.MonsterKind = static_cast<std::int32_t>(kind);
    t.ObjectKind  = mxh::game::OBJECTKIND_BOSS_MONSTER;
    std::strncpy(t.Name, "BossMgrTpl", sizeof(t.Name));
    t.Level       = 50;
    t.Life        = life;
    t.Shield      = 0;
    t.ExpPoint    = 100;
    t.AttackMin   = 100;
    t.AttackMax   = 200;
    t.Defense     = 50;
    return t;
}

BossMonsterInfo make_info(std::uint32_t kind, std::uint8_t field = 0) {
    BossMonsterInfo i{};
    i.monster_kind    = kind;
    i.is_field_boss   = field;
    i.time_limit_ms   = 60000;
    i.killer_limit    = 10;
    i.announce_msg_id = 7;
    i.speech_id_base  = 100;
    return i;
}

}  // namespace

// -------- defaults --------

TEST(BossMonsterManagerTest, DefaultManagerIsEmpty) {
    BossMonsterManager mgr;
    EXPECT_EQ(mgr.live_count(), 0u);
    EXPECT_EQ(mgr.info_for(1234), nullptr);
    EXPECT_EQ(mgr.find(9999), nullptr);
}

TEST(BossMonsterManagerTest, FindUnknownOidReturnsNull) {
    BossMonsterManager mgr;
    EXPECT_EQ(mgr.find(1), nullptr);
    EXPECT_EQ(mgr.find(0xFFFFFFFF), nullptr);
}

// -------- register_info / info_for --------

TEST(BossMonsterManagerTest, RegisterInfoStoresData) {
    BossMonsterManager mgr;
    BossMonsterInfo info = make_info(1234, /*field=*/1);
    info.time_limit_ms = 12345;
    mgr.register_info(1234, info);
    const BossMonsterInfo* got = mgr.info_for(1234);
    ASSERT_NE(got, nullptr);
    EXPECT_EQ(got->monster_kind, 1234u);
    EXPECT_EQ(got->is_field_boss, 1u);
    EXPECT_EQ(got->time_limit_ms, 12345u);
    EXPECT_EQ(got->killer_limit, 10u);
    EXPECT_EQ(got->announce_msg_id, 7u);
    EXPECT_EQ(got->speech_id_base, 100u);
}

TEST(BossMonsterManagerTest, InfoForUnknownKindReturnsNull) {
    BossMonsterManager mgr;
    mgr.register_info(1234, make_info(1234));
    EXPECT_EQ(mgr.info_for(5678), nullptr);
    EXPECT_EQ(mgr.info_for(0), nullptr);
}

TEST(BossMonsterManagerTest, RegisterMultipleKinds) {
    BossMonsterManager mgr;
    mgr.register_info(100, make_info(100));
    mgr.register_info(200, make_info(200));
    mgr.register_info(300, make_info(300));
    EXPECT_NE(mgr.info_for(100), nullptr);
    EXPECT_NE(mgr.info_for(200), nullptr);
    EXPECT_NE(mgr.info_for(300), nullptr);
    EXPECT_EQ(mgr.info_for(100)->monster_kind, 100u);
    EXPECT_EQ(mgr.info_for(200)->monster_kind, 200u);
    EXPECT_EQ(mgr.info_for(300)->monster_kind, 300u);
}

TEST(BossMonsterManagerTest, RegisterInfoOverwritesByKind) {
    BossMonsterManager mgr;
    BossMonsterInfo a = make_info(1234);
    a.time_limit_ms = 1000;
    mgr.register_info(1234, a);
    BossMonsterInfo b = make_info(1234);
    b.time_limit_ms = 2000;
    mgr.register_info(1234, b);
    EXPECT_EQ(mgr.info_for(1234)->time_limit_ms, 2000u);
}

// -------- spawn --------

TEST(BossMonsterManagerTest, SpawnWithoutInfoReturnsZero) {
    BossMonsterManager mgr;
    MonsterTemplate tpl = make_template(1234);
    EXPECT_EQ(mgr.spawn(1234, tpl, /*oid=*/100, 0, 0, 0, 0), 0u);
    EXPECT_EQ(mgr.live_count(), 0u);
}

TEST(BossMonsterManagerTest, SpawnCreatesLiveInstance) {
    BossMonsterManager mgr;
    mgr.register_info(1234, make_info(1234));
    MonsterTemplate tpl = make_template(1234, /*life=*/2000);
    EXPECT_EQ(mgr.spawn(1234, tpl, /*oid=*/100,
                        /*x=*/1500, /*y=*/2500, /*z=*/-100, /*map=*/42), 100u);
    EXPECT_EQ(mgr.live_count(), 1u);
    const BossMonsterInstance* inst = mgr.find(100);
    ASSERT_NE(inst, nullptr);
    EXPECT_EQ(inst->base.monster_kind, 1234u);
    EXPECT_EQ(inst->base.map_num, 42);
    EXPECT_EQ(inst->base.spawn_x, 1500);
    EXPECT_EQ(inst->base.spawn_y, 2500);
    EXPECT_EQ(inst->base.spawn_z, -100);
    EXPECT_EQ(inst->base.max_hp, 2000u);
    EXPECT_EQ(inst->base.current_hp, 2000u);
}

TEST(BossMonsterManagerTest, SpawnMultipleDistinctOids) {
    BossMonsterManager mgr;
    mgr.register_info(1234, make_info(1234));
    MonsterTemplate tpl = make_template(1234);
    EXPECT_EQ(mgr.spawn(1234, tpl, 100, 0, 0, 0, 7), 100u);
    EXPECT_EQ(mgr.spawn(1234, tpl, 200, 0, 0, 0, 7), 200u);
    EXPECT_EQ(mgr.spawn(1234, tpl, 300, 0, 0, 0, 7), 300u);
    EXPECT_EQ(mgr.live_count(), 3u);
    EXPECT_NE(mgr.find(100), nullptr);
    EXPECT_NE(mgr.find(200), nullptr);
    EXPECT_NE(mgr.find(300), nullptr);
}

TEST(BossMonsterManagerTest, SpawnSameOidReplaces) {
    // 1:1 with legacy CBossMonsterManager: re-spawning the same object_id
    // overwrites the previous boss (no duplicate protection).
    BossMonsterManager mgr;
    mgr.register_info(1234, make_info(1234));
    MonsterTemplate tpl = make_template(1234, /*life=*/1000);
    EXPECT_EQ(mgr.spawn(1234, tpl, 100, 0, 0, 0, 7), 100u);
    EXPECT_EQ(mgr.live_count(), 1u);
    MonsterTemplate tpl2 = make_template(1234, /*life=*/2000);
    EXPECT_EQ(mgr.spawn(1234, tpl2, 100, 0, 0, 0, 7), 100u);
    EXPECT_EQ(mgr.live_count(), 1u);
    const BossMonsterInstance* inst = mgr.find(100);
    ASSERT_NE(inst, nullptr);
    EXPECT_EQ(inst->base.max_hp, 2000u);  // replaced with new template life
}

TEST(BossMonsterManagerTest, InfoKeepsRewardsIntact) {
    // Rewards live on the per-kind BossMonsterInfo (looked up via info_for),
    // not on the live BossMonsterInstance. The manager exposes info_for so
    // drop logic can pull them at spawn time.
    BossMonsterManager mgr;
    BossMonsterInfo info = make_info(1234);
    info.rewards[0] = BossReward{/*item=*/77001, /*ratio=*/50};
    info.rewards[1] = BossReward{/*item=*/77002, /*ratio=*/30};
    info.rewards[2] = BossReward{/*item=*/77003, /*ratio=*/20};
    mgr.register_info(1234, info);
    const BossMonsterInfo* got = mgr.info_for(1234);
    ASSERT_NE(got, nullptr);
    EXPECT_EQ(got->rewards[0].item_id, 77001u);
    EXPECT_EQ(got->rewards[0].ratio, 50u);
    EXPECT_EQ(got->rewards[1].item_id, 77002u);
    EXPECT_EQ(got->rewards[1].ratio, 30u);
    EXPECT_EQ(got->rewards[2].item_id, 77003u);
    EXPECT_EQ(got->rewards[2].ratio, 20u);
}

// -------- find --------

TEST(BossMonsterManagerTest, FindConstAndNonConstReturnSamePointer) {
    BossMonsterManager mgr;
    mgr.register_info(1234, make_info(1234));
    mgr.spawn(1234, make_template(1234), 100, 0, 0, 0, 7);
    const BossMonsterManager& cmgr = mgr;
    const BossMonsterInstance* a = cmgr.find(100);
    BossMonsterInstance* b = mgr.find(100);
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);
    EXPECT_EQ(a, b);
}

// -------- damage --------

TEST(BossMonsterManagerTest, DamageUnknownOidReturnsSealed) {
    BossMonsterManager mgr;
    EXPECT_EQ(mgr.damage(999, 100, /*attacker=*/1, /*now_ms=*/0),
              BossPhase::Sealed);
}

TEST(BossMonsterManagerTest, DamageKnownOidReturnsValidPhase) {
    BossMonsterManager mgr;
    mgr.register_info(1234, make_info(1234));
    mgr.spawn(1234, make_template(1234, /*life=*/1000), 100, 0, 0, 0, 7);
    // Tiny damage at full HP keeps it in Combat.
    EXPECT_EQ(mgr.damage(100, 10, /*attacker=*/1, /*now_ms=*/0),
              BossPhase::Combat);
    // Push HP below 75%% -> Enraged.
    EXPECT_EQ(mgr.damage(100, 300, /*attacker=*/1, /*now_ms=*/1),
              BossPhase::Enraged);
    // Push HP below 50%% -> Phase2.
    EXPECT_EQ(mgr.damage(100, 200, /*attacker=*/1, /*now_ms=*/2),
              BossPhase::Phase2);
    // Push HP below 25%% (490-300=190, 19%%) -> Rage.
    EXPECT_EQ(mgr.damage(100, 300, /*attacker=*/1, /*now_ms=*/3),
              BossPhase::Rage);
}

// -------- erase --------

TEST(BossMonsterManagerTest, EraseUnknownOidReturnsFalse) {
    BossMonsterManager mgr;
    EXPECT_FALSE(mgr.erase(999));
    EXPECT_FALSE(mgr.erase(0));
}

TEST(BossMonsterManagerTest, EraseKnownOidReturnsTrueAndRemoves) {
    BossMonsterManager mgr;
    mgr.register_info(1234, make_info(1234));
    mgr.spawn(1234, make_template(1234), 100, 0, 0, 0, 7);
    EXPECT_EQ(mgr.live_count(), 1u);
    EXPECT_TRUE(mgr.erase(100));
    EXPECT_EQ(mgr.live_count(), 0u);
    EXPECT_EQ(mgr.find(100), nullptr);
    EXPECT_FALSE(mgr.erase(100));  // idempotent
}

TEST(BossMonsterManagerTest, EraseOneKeepsOthers) {
    BossMonsterManager mgr;
    mgr.register_info(1234, make_info(1234));
    mgr.spawn(1234, make_template(1234), 100, 0, 0, 0, 7);
    mgr.spawn(1234, make_template(1234), 200, 0, 0, 0, 7);
    EXPECT_EQ(mgr.live_count(), 2u);
    EXPECT_TRUE(mgr.erase(100));
    EXPECT_EQ(mgr.live_count(), 1u);
    EXPECT_EQ(mgr.find(100), nullptr);
    EXPECT_NE(mgr.find(200), nullptr);
}

TEST(BossMonsterManagerTest, EraseThenSpawnReusesOid) {
    BossMonsterManager mgr;
    mgr.register_info(1234, make_info(1234));
    mgr.spawn(1234, make_template(1234), 100, 0, 0, 0, 7);
    mgr.erase(100);
    EXPECT_EQ(mgr.spawn(1234, make_template(1234), 100, 0, 0, 0, 7), 100u);
    EXPECT_NE(mgr.find(100), nullptr);
}

// -------- live_count --------

TEST(BossMonsterManagerTest, LiveCountTracksSpawnAndErase) {
    BossMonsterManager mgr;
    EXPECT_EQ(mgr.live_count(), 0u);
    mgr.register_info(1234, make_info(1234));
    MonsterTemplate tpl = make_template(1234);
    mgr.spawn(1234, tpl, 1, 0, 0, 0, 7);
    EXPECT_EQ(mgr.live_count(), 1u);
    mgr.spawn(1234, tpl, 2, 0, 0, 0, 7);
    EXPECT_EQ(mgr.live_count(), 2u);
    mgr.spawn(1234, tpl, 3, 0, 0, 0, 7);
    EXPECT_EQ(mgr.live_count(), 3u);
    mgr.erase(2);
    EXPECT_EQ(mgr.live_count(), 2u);
    mgr.erase(1);
    mgr.erase(3);
    EXPECT_EQ(mgr.live_count(), 0u);
}
