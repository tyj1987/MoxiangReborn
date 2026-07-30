// ability_group_test.cpp

#include "mxh/server/ability_group.hpp"
#include <gtest/gtest.h>

namespace {
using mxh::server::AbilityGroup;
using mxh::server::DelayGroup;
using mxh::server::EffectEntry;

EffectEntry make_effect(std::uint32_t idx, std::uint8_t kind, std::uint16_t val,
                         std::uint32_t start, std::uint32_t dur) {
    EffectEntry e;
    e.effect_idx = idx;
    e.kind = kind;
    e.value = val;
    e.start_ms = start;
    e.duration_ms = dur;
    return e;
}
}

TEST(AbilityGroup, PushAndSize) {
    AbilityGroup g;
    EXPECT_TRUE(g.push(make_effect(1, 2, 10, 100, 1000)));
    EXPECT_TRUE(g.push(make_effect(2, 2, 20, 100, 1000)));
    EXPECT_EQ(g.size(), 2u);
}

TEST(AbilityGroup, Remove) {
    AbilityGroup g;
    g.push(make_effect(10, 1, 5, 0, 1000));
    g.push(make_effect(11, 1, 5, 0, 1000));
    EXPECT_TRUE(g.remove(10));
    EXPECT_EQ(g.size(), 1u);
    EXPECT_FALSE(g.remove(99));
}

TEST(AbilityGroup, TickExpireDropsExpiredEffects) {
    AbilityGroup g;
    g.push(make_effect(20, 1, 5, 100, 1000));   // alive at 1099, expires 1100
    g.push(make_effect(21, 1, 5, 100, 10000));  // still alive
    g.tick(1099);
    EXPECT_EQ(g.size(), 2u);
    g.tick(1100);
    EXPECT_EQ(g.size(), 1u);
    g.tick(20000);
    EXPECT_EQ(g.size(), 0u);
}

TEST(AbilityGroup, TotalValueSumsByKind) {
    AbilityGroup g;
    g.push(make_effect(30, 1, 5, 0, 100));
    g.push(make_effect(31, 1, 7, 0, 100));
    g.push(make_effect(32, 2, 99, 0, 100));   // different kind
    EXPECT_EQ(g.total_value(1), 12u);
    EXPECT_EQ(g.total_value(2), 99u);
    EXPECT_EQ(g.total_value(3), 0u);
}

TEST(DelayGroup, ActivateThenRemaining) {
    DelayGroup d;
    d.activate(7, 100);
    EXPECT_EQ(d.remaining_ms(7, 100, 1000), 1000u);
    EXPECT_EQ(d.remaining_ms(7, 599,  1000), 501u);
    EXPECT_EQ(d.remaining_ms(7, 1100, 1000),   0u);
}

TEST(DelayGroup, ClearAndUnusedReturnsZero) {
    DelayGroup d;
    d.activate(7, 100);
    d.clear(7);
    EXPECT_EQ(d.remaining_ms(7, 100, 1000), 0u);
    EXPECT_EQ(d.remaining_ms(8, 100, 1000), 0u);  // never activated
}

TEST(DelayGroup, ModuloCollisionsAreKeyed) {
    DelayGroup d;
    d.activate(7,    100);  // bucket 7
    d.activate(107,  500);  // bucket 7 -- overwrites
    EXPECT_EQ(d.remaining_ms(7,    550, 1000), 0u);     // bucket now points at 107
    EXPECT_EQ(d.remaining_ms(107,  550, 1000), 950u);
}

