// ai_group_prototype_test.cpp - Phase D6 AIGroupPrototype 1:1 port tests.

#include "mxh/server/ai_group_prototype.hpp"

#include <gtest/gtest.h>

namespace {

using mxh::server::AIGroup;
using mxh::server::PendingDeaths;
using mxh::server::AIGROUP_REGEN_HASH_CAPACITY;

TEST(AIGroupPrototype, DefaultStateIsZero) {
    AIGroup g{};
    EXPECT_EQ(g.m_dwGroupID, 0u);
    EXPECT_EQ(g.m_dwGridID, 0u);
    EXPECT_EQ(g.get_max_object_num(), 0u);
}

TEST(AIGroupPrototype, AddRegenObjectIncrementsMax) {
    AIGroup g{};
    g.add_regen_object(101u);
    g.add_regen_object(202u);
    EXPECT_EQ(g.get_max_object_num(), 2u);
    EXPECT_TRUE(g.get_regen_object(101u));
    EXPECT_TRUE(g.get_regen_object(202u));
    EXPECT_FALSE(g.get_regen_object(999u));
}

TEST(AIGroupPrototype, SetRandomGridIdUpdatesGroupAndSlots) {
    AIGroup g{};
    g.add_regen_object(11u);
    g.add_regen_object(22u);
    g.set_random_grid_id(7u);
    EXPECT_EQ(g.m_dwGridID, 7u);
}

TEST(AIGroupPrototype, DieDoesNotCrashWithoutRegenInfo) {
    AIGroup g{};
    g.die(42u);
    SUCCEED();
}

TEST(AIGroupPrototype, AliveDoesNotCrashWithoutRegenInfo) {
    AIGroup g{};
    g.alive(42u);
    SUCCEED();
}

TEST(PendingDeaths, AddIncreasesCount) {
    PendingDeaths p{};
    EXPECT_EQ(p.count(), 0u);
    p.add(1u);
    p.add(2u);
    EXPECT_EQ(p.count(), 2u);
}

TEST(PendingDeaths, RemoveDecreasesCount) {
    PendingDeaths p{};
    p.add(1u);
    p.add(2u);
    p.remove(1u);
    EXPECT_EQ(p.count(), 1u);
}

TEST(PendingDeaths, RemoveUnknownIsNoOp) {
    PendingDeaths p{};
    p.add(1u);
    p.remove(99u);
    EXPECT_EQ(p.count(), 1u);
}

TEST(PendingDeaths, HashCapacityMatchesLegacy) {
    PendingDeaths p{};
    EXPECT_EQ(p.ids.size(), AIGROUP_REGEN_HASH_CAPACITY);
    EXPECT_EQ(AIGROUP_REGEN_HASH_CAPACITY, 10u);
}

TEST(AIGroupPrototype, RegenFlowDoesNotCrash) {
    AIGroup g{};
    g.regen_check();
    g.regen_process();
    g.force_regen();
    SUCCEED();
}

}  // namespace
