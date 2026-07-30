// pk_manager_test.cpp

#include "mxh/server/pk_manager.hpp"
#include <gtest/gtest.h>

namespace {
using mxh::server::PKManager;
using mxh::server::PKLootingManager;
using mxh::server::PkMode;
using mxh::server::LootDrop;
}

TEST(PKManager, DefaultIsPeace) {
    PKManager m;
    EXPECT_TRUE(m.set_peace(100));
    auto* s = m.find(100);
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(s->mode, PkMode::Peace);
    EXPECT_EQ(s->kill_count, 0u);
}

TEST(PKManager, AllowPkSetsMode) {
    PKManager m;
    EXPECT_TRUE(m.allow_pk(100));
    EXPECT_EQ(m.find(100)->mode, PkMode::PkAllow);
}

TEST(PKManager, ThreeKillsTriggersPenalty) {
    PKManager m;
    EXPECT_TRUE(m.set_peace(100));
    m.on_kill(100, 1000);
    m.on_kill(100, 1100);
    EXPECT_EQ(m.find(100)->mode, PkMode::Peace);  // count 2 < 3
    m.on_kill(100, 1200);
    EXPECT_EQ(m.find(100)->mode, PkMode::KillerPenalty);
    EXPECT_GT(m.find(100)->penalty_until_ms, 0u);
}

TEST(PKManager, TickReleasesPenalty) {
    PKManager m;
    m.set_peace(100);
    m.on_kill(100, 1000);
    m.on_kill(100, 1100);
    m.on_kill(100, 1200);
    EXPECT_EQ(m.find(100)->mode, PkMode::KillerPenalty);
    const std::uint32_t end = m.find(100)->penalty_until_ms;
    EXPECT_FALSE(m.tick(end - 1));            // not yet
    EXPECT_EQ(m.find(100)->mode, PkMode::KillerPenalty);
    EXPECT_TRUE(m.tick(end));                 // expires at the boundary
    EXPECT_EQ(m.find(100)->mode, PkMode::Peace);
    EXPECT_FALSE(m.tick(end + 1));            // nothing changed
}

TEST(PKLootingManager, SetAndRoll) {
    PKLootingManager m;
    LootDrop d1; d1.item_idx=100; d1.ratio=70;
    LootDrop d2; d2.item_idx=200; d2.ratio=30;
    EXPECT_TRUE(m.set_drops(7, {d1, d2}));
    EXPECT_EQ(m.roll(7,  0),  100u);   // 0 < 70 -> item 100
    EXPECT_EQ(m.roll(7, 69),  100u);   // 69 < 70
    EXPECT_EQ(m.roll(7, 70),  200u);   // 70..99
    EXPECT_EQ(m.roll(7, 99),  200u);
    EXPECT_EQ(m.roll(8, 0), 0u);       // unknown victim
}

TEST(PKLootingManager, RejectEmptyDrops) {
    PKLootingManager m;
    EXPECT_FALSE(m.set_drops(7, {}));
}

