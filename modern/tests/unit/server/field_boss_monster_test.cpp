//
// 1:1 lock tests for mxh::server::FieldBossMonsterManager (Phase D6 Boss 刷新).
//
// FieldBossMonsterManager implements the legacy [Server]Map/FieldBossMonster.h
// manager: a flat array of channels, each carrying a per-channel spawn timer,
// respawn interval, and spawn coords. tick() advances the timer and spawns
// one boss per call when the timer expires.
//
// Coverage:
//   * Defaults: empty manager has 0 channels
//   * configure_channel stores all 7 fields
//   * configure_channel auto-resizes the channel vector
//   * channel(idx) returns nullptr out-of-range
//   * tick() with no enabled channels returns false
//   * tick() before next_spawn_ms returns false
//   * tick() at next_spawn_ms spawns (active=1, id assigned)
//   * tick() reschedules next_spawn_ms = now_ms + interval
//   * tick() skips channels with active=1 (alive)
//   * tick() skips channels with monster_kind=0 (disabled)
//   * tick() skips channels with respawn_interval_ms=0 (no-respawn)
//   * tick() skips when template not found in pool
//   * tick() returns true after spawn, last_spawn_object_id updated
//   * multi-channel: only one spawn per tick (legacy uses break)
//   * tick() after death resets active=0 and respawns after interval
//   * 1:1 quirks: monster_kind=0 == disabled; respawn_interval_ms=0 == no-respawn
//

#include "mxh/server/field_boss_monster.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

namespace {

using mxh::server::FieldBossMonsterManager;
using mxh::server::FieldBossChannel;
using mxh::game::MonsterTemplate;

MonsterTemplate make_template(std::int32_t monster_kind) {
    MonsterTemplate t{};
    t.MonsterKind = monster_kind;
    return t;
}

}  // namespace

TEST(FieldBossMonsterTest, DefaultManagerHasZeroChannels) {
    FieldBossMonsterManager mgr;
    EXPECT_EQ(mgr.channel_count(), 0u);
    EXPECT_EQ(mgr.channel(0), nullptr);
    EXPECT_EQ(mgr.last_spawn_object_id(), 0u);
}

TEST(FieldBossMonsterTest, ConfigureChannelStoresAllFields) {
    FieldBossMonsterManager mgr;
    mgr.configure_channel(0, /*kind=*/4001, /*interval=*/60000,
                          /*first_spawn=*/0,
                          /*x=*/100, /*y=*/200, /*z=*/50,
                          /*map=*/7);
    EXPECT_EQ(mgr.channel_count(), 1u);
    const auto* ch = mgr.channel(0);
    ASSERT_NE(ch, nullptr);
    EXPECT_EQ(ch->monster_kind, 4001u);
    EXPECT_EQ(ch->respawn_interval_ms, 60000u);
    EXPECT_EQ(ch->next_spawn_ms, 0u);
    EXPECT_EQ(ch->spawn_x, 100);
    EXPECT_EQ(ch->spawn_y, 200);
    EXPECT_EQ(ch->spawn_z, 50);
    EXPECT_EQ(ch->map_num, 7);
    EXPECT_EQ(ch->active, 0u);
    EXPECT_EQ(ch->current_object_id, 0u);
}

TEST(FieldBossMonsterTest, ConfigureChannelAutoResizes) {
    FieldBossMonsterManager mgr;
    mgr.configure_channel(5, 4001, 60000, 0, 0, 0, 0, 7);
    EXPECT_EQ(mgr.channel_count(), 6u);
    // Channels 0..4 are default-initialized (monster_kind=0 => disabled).
    for (std::size_t i = 0; i < 5; ++i) {
        const auto* ch = mgr.channel(i);
        ASSERT_NE(ch, nullptr);
        EXPECT_EQ(ch->monster_kind, 0u);
    }
    const auto* ch5 = mgr.channel(5);
    ASSERT_NE(ch5, nullptr);
    EXPECT_EQ(ch5->monster_kind, 4001u);
}

TEST(FieldBossMonsterTest, ChannelOutOfRangeReturnsNull) {
    FieldBossMonsterManager mgr;
    mgr.configure_channel(0, 4001, 60000, 0, 0, 0, 0, 7);
    EXPECT_EQ(mgr.channel(1), nullptr);
    EXPECT_EQ(mgr.channel(99), nullptr);
}

TEST(FieldBossMonsterTest, TickWithNoEnabledChannelsReturnsFalse) {
    FieldBossMonsterManager mgr;
    mgr.configure_channel(0, /*kind=*/0, 60000, 0, 0, 0, 0, 7);  // disabled
    std::vector<MonsterTemplate> tpls = {make_template(4001)};
    EXPECT_FALSE(mgr.tick(0, tpls, 100));
    EXPECT_EQ(mgr.last_spawn_object_id(), 0u);
}

TEST(FieldBossMonsterTest, TickBeforeNextSpawnMsReturnsFalse) {
    FieldBossMonsterManager mgr;
    mgr.configure_channel(0, 4001, 60000, /*first_spawn=*/1000,
                          0, 0, 0, 7);
    std::vector<MonsterTemplate> tpls = {make_template(4001)};
    EXPECT_FALSE(mgr.tick(500, tpls, 100));  // before 1000
    EXPECT_FALSE(mgr.tick(999, tpls, 100));  // last frame before
    EXPECT_EQ(mgr.last_spawn_object_id(), 0u);
}

TEST(FieldBossMonsterTest, TickAtNextSpawnMsFires) {
    FieldBossMonsterManager mgr;
    mgr.configure_channel(0, 4001, 60000, /*first_spawn=*/1000,
                          0, 0, 0, 7);
    std::vector<MonsterTemplate> tpls = {make_template(4001)};
    EXPECT_TRUE(mgr.tick(1000, tpls, 100));
    EXPECT_EQ(mgr.last_spawn_object_id(), 100u);

    const auto* ch = mgr.channel(0);
    ASSERT_NE(ch, nullptr);
    EXPECT_EQ(ch->active, 1u);
    EXPECT_EQ(ch->current_object_id, 100u);
    // 1:1 with legacy: next_spawn_ms = now_ms + respawn_interval_ms.
    EXPECT_EQ(ch->next_spawn_ms, 1000 + 60000);
}

TEST(FieldBossMonsterTest, TickAfterSpawnReschedules) {
    FieldBossMonsterManager mgr;
    mgr.configure_channel(0, 4001, /*interval=*/10000, /*first_spawn=*/0,
                          0, 0, 0, 7);
    std::vector<MonsterTemplate> tpls = {make_template(4001)};

    EXPECT_TRUE(mgr.tick(0, tpls, 100));
    const auto* ch = mgr.channel(0);
    EXPECT_EQ(ch->active, 1u);
    EXPECT_EQ(ch->next_spawn_ms, 10000u);

    // While alive, tick returns false.
    EXPECT_FALSE(mgr.tick(5000, tpls, 200));
    EXPECT_FALSE(mgr.tick(9999, tpls, 200));
    EXPECT_EQ(mgr.last_spawn_object_id(), 100u);  // unchanged
}

TEST(FieldBossMonsterTest, TickSkipsAliveChannel) {
    FieldBossMonsterManager mgr;
    mgr.configure_channel(0, 4001, 10000, 0, 0, 0, 0, 7);
    std::vector<MonsterTemplate> tpls = {make_template(4001)};

    mgr.tick(0, tpls, 100);  // spawn
    // Channel is now alive; even after next_spawn_ms passes, no re-spawn.
    EXPECT_FALSE(mgr.tick(100000, tpls, 200));
    EXPECT_EQ(mgr.channel(0)->active, 1u);
}

TEST(FieldBossMonsterTest, TickSkipsDisabledChannel) {
    FieldBossMonsterManager mgr;
    mgr.configure_channel(0, /*kind=*/0, 10000, 0, 0, 0, 0, 7);  // disabled
    std::vector<MonsterTemplate> tpls = {make_template(4001)};
    EXPECT_FALSE(mgr.tick(0, tpls, 100));
    EXPECT_EQ(mgr.channel(0)->active, 0u);
}

TEST(FieldBossMonsterTest, TickSkipsChannelWithZeroRespawnInterval) {
    FieldBossMonsterManager mgr;
    mgr.configure_channel(0, 4001, /*interval=*/0, /*first_spawn=*/0,
                          0, 0, 0, 7);
    std::vector<MonsterTemplate> tpls = {make_template(4001)};
    EXPECT_FALSE(mgr.tick(0, tpls, 100));
    EXPECT_EQ(mgr.channel(0)->active, 0u);
}

TEST(FieldBossMonsterTest, TickSkipsWhenTemplateMissing) {
    FieldBossMonsterManager mgr;
    mgr.configure_channel(0, 4001, 10000, 0, 0, 0, 0, 7);
    std::vector<MonsterTemplate> tpls = {make_template(9999)};  // different kind
    EXPECT_FALSE(mgr.tick(0, tpls, 100));
    EXPECT_EQ(mgr.channel(0)->active, 0u);
}

TEST(FieldBossMonsterTest, TickWithEmptyTemplatePoolSkipsSpawn) {
    FieldBossMonsterManager mgr;
    mgr.configure_channel(0, 4001, 10000, 0, 0, 0, 0, 7);
    std::vector<MonsterTemplate> tpls;  // empty
    EXPECT_FALSE(mgr.tick(0, tpls, 100));
}

TEST(FieldBossMonsterTest, TickUpdatesLastSpawnObjectId) {
    FieldBossMonsterManager mgr;
    mgr.configure_channel(0, 4001, 10000, 0, 0, 0, 0, 7);
    std::vector<MonsterTemplate> tpls = {make_template(4001)};
    EXPECT_TRUE(mgr.tick(0, tpls, 99999));
    EXPECT_EQ(mgr.last_spawn_object_id(), 99999u);
}

TEST(FieldBossMonsterTest, TickOneSpawnPerCallEvenWithMultipleChannels) {
    // 1:1 quirk: legacy uses `break` after the first spawn, so only
    // one boss spawns per tick() call. Multi-channel spawns are
    // spread across multiple ticks.
    FieldBossMonsterManager mgr;
    mgr.configure_channel(0, 4001, 10000, 0, 0, 0, 0, 7);
    mgr.configure_channel(1, 4002, 10000, 0, 0, 0, 0, 7);
    mgr.configure_channel(2, 4003, 10000, 0, 0, 0, 0, 7);
    std::vector<MonsterTemplate> tpls = {
        make_template(4001), make_template(4002), make_template(4003)
    };

    int spawned_count = 0;
    for (int i = 0; i < 100; ++i) {
        if (mgr.tick(i * 1000, tpls, 100 + i)) {
            ++spawned_count;
        }
    }
    // Each tick fires at most one. With 3 channels and 100 ticks at 1s
    // apart, all 3 should fire (at tick 0, then 10s later, etc).
    EXPECT_EQ(spawned_count, 3);
    EXPECT_EQ(mgr.channel(0)->active, 1u);
    EXPECT_EQ(mgr.channel(1)->active, 1u);
    EXPECT_EQ(mgr.channel(2)->active, 1u);
}

TEST(FieldBossMonsterTest, TickRespawnsAfterActiveCleared) {
    // 1:1 with legacy: death clears active=0 and the next tick after
    // the timer expires fires a new spawn.
    FieldBossMonsterManager mgr;
    mgr.configure_channel(0, 4001, /*interval=*/1000, /*first_spawn=*/0,
                          0, 0, 0, 7);
    std::vector<MonsterTemplate> tpls = {make_template(4001)};

    mgr.tick(0, tpls, 100);  // first spawn
    EXPECT_EQ(mgr.channel(0)->active, 1u);

    // Death: external code resets active=0 (the manager doesn't own
    // death; legacy hooks via the boss's HP=0 -> m_pFieldBossMonster->Die).
    mgr.channel(0)->active = 0;

    // Before next_spawn_ms: no spawn.
    EXPECT_FALSE(mgr.tick(500, tpls, 200));
    EXPECT_EQ(mgr.channel(0)->active, 0u);

    // At next_spawn_ms: spawn again.
    EXPECT_TRUE(mgr.tick(1000, tpls, 200));
    EXPECT_EQ(mgr.channel(0)->active, 1u);
    EXPECT_EQ(mgr.channel(0)->current_object_id, 200u);
    EXPECT_EQ(mgr.last_spawn_object_id(), 200u);
}

TEST(FieldBossMonsterTest, SpawnedChannelRemembersPosition) {
    FieldBossMonsterManager mgr;
    mgr.configure_channel(0, 4001, 10000, 0,
                          /*x=*/1500, /*y=*/2500, /*z=*/-100, /*map=*/42);
    std::vector<MonsterTemplate> tpls = {make_template(4001)};
    mgr.tick(0, tpls, 100);

    const auto* ch = mgr.channel(0);
    EXPECT_EQ(ch->spawn_x, 1500);
    EXPECT_EQ(ch->spawn_y, 2500);
    EXPECT_EQ(ch->spawn_z, -100);
    EXPECT_EQ(ch->map_num, 42);
}

TEST(FieldBossMonsterTest, MultipleChannelsWithDifferentIntervals) {
    FieldBossMonsterManager mgr;
    // Channel 0 not yet (first_spawn far future); channel 1 fires at tick(0).
    // 1:1 with legacy break: only one channel fires per tick, so set
    // first_spawn so only channel 1 is eligible.
    mgr.configure_channel(0, 4001, /*interval=*/1000, /*first_spawn=*/10000, 0, 0, 0, 7);
    mgr.configure_channel(1, 4002, /*interval=*/500,  /*first_spawn=*/0, 0, 0, 0, 7);
    std::vector<MonsterTemplate> tpls = {make_template(4001), make_template(4002)};

    mgr.tick(0, tpls, 100);  // fires channel 1 (channel 0 not yet)
    // Channel 1 just spawned: next_spawn = 0 + 500 = 500.
    EXPECT_EQ(mgr.channel(1)->next_spawn_ms, 500u);
    EXPECT_EQ(mgr.channel(1)->active, 1u);
    // Channel 0 unchanged (still waiting for first_spawn).
    EXPECT_EQ(mgr.channel(0)->next_spawn_ms, 10000u);
    EXPECT_EQ(mgr.channel(0)->active, 0u);
}

TEST(FieldBossMonsterTest, FieldBossChannelDefaultZero) {
    FieldBossChannel ch{};
    EXPECT_EQ(ch.monster_kind, 0u);
    EXPECT_EQ(ch.next_spawn_ms, 0u);
    EXPECT_EQ(ch.respawn_interval_ms, 0u);
    EXPECT_EQ(ch.active, 0u);
    EXPECT_EQ(ch.current_object_id, 0u);
    EXPECT_EQ(ch.spawn_x, 0);
    EXPECT_EQ(ch.spawn_y, 0);
    EXPECT_EQ(ch.spawn_z, 0);
    EXPECT_EQ(ch.map_num, 0);
}
