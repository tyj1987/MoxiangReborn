// regen_manager_test.cpp - Phase D5 RegenManager 1:1 port tests.

#include "mxh/server/regen_manager.hpp"
#include <gtest/gtest.h>

namespace {
using mxh::server::RegenManagerState;
using mxh::server::RegenPrototype;
using mxh::server::Vec3;
using mxh::server::MONSTER_REGEN_RANDOM_RANGE;
using mxh::server::make_regen_manager;
using mxh::server::release;
using mxh::server::add_prototype;
using mxh::server::get_prototype;
using mxh::server::range_pos_at_orig;
using mxh::server::regen_object;
using mxh::server::regen_group_count;
using mxh::server::regen_group_contains;
using mxh::server::drop_ratio_passes;
using mxh::server::in_random_range;

static RegenPrototype make_proto(std::uint32_t id = 1u, std::uint32_t group = 100u,
                                 std::uint32_t obj_id = 1000u,
                                 std::uint16_t kind = 7u, std::uint16_t monster = 5u) {
    RegenPrototype p;
    p.m_dwID = id;
    p.m_dwGroupID = group;
    p.m_dwObjectID = obj_id;
    p.m_wObjectKind = kind;
    p.m_wMonsterKind = monster;
    p.m_DropItemID = 0;
    p.m_dwDropRatio = 100u;
    return p;
}
}

// ---- Constants 1:1 ----

TEST(RegenManagerConstants, RandomRangeMatchesLegacy) {
    EXPECT_EQ(MONSTER_REGEN_RANDOM_RANGE, 1500u);
}

// ---- Prototype CRUD ----

TEST(RegenManagerCRUD, AddAndFind) {
    auto s = make_regen_manager();
    add_prototype(s, make_proto(7u, 100u, 1000u));
    auto* p = get_prototype(s, 7u);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->m_dwGroupID, 100u);
    EXPECT_EQ(p->m_dwObjectID, 1000u);
}

TEST(RegenManagerCRUD, GetMissingReturnsNull) {
    auto s = make_regen_manager();
    EXPECT_EQ(get_prototype(s, 999u), nullptr);
}

TEST(RegenManagerCRUD, AddOverwritesSameId) {
    auto s = make_regen_manager();
    add_prototype(s, make_proto(1u, 100u, 1000u, 7u));
    add_prototype(s, make_proto(1u, 200u, 2000u, 8u));
    auto* p = get_prototype(s, 1u);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->m_dwGroupID, 200u);
    EXPECT_EQ(p->m_dwObjectID, 2000u);
    EXPECT_EQ(p->m_wObjectKind, 8u);
}

TEST(RegenManagerCRUD, ReleaseClearsAll) {
    auto s = make_regen_manager();
    add_prototype(s, make_proto(1u));
    add_prototype(s, make_proto(2u));
    release(s);
    EXPECT_EQ(get_prototype(s, 1u), nullptr);
}

// ---- RangePosAtOrig ----

TEST(RegenManagerRangePos, ZeroRangeKeepsOrigin) {
    Vec3 orig{1.0f, 2.0f, 3.0f};
    Vec3 out;
    range_pos_at_orig(orig, 0, out, /*rand_x*/100, /*rand_z*/100);
    EXPECT_FLOAT_EQ(out.x, 1.0f);
    EXPECT_FLOAT_EQ(out.y, 2.0f);
    EXPECT_FLOAT_EQ(out.z, 3.0f);
}

TEST(RegenManagerRangePos, HalfRangeAppliesOffset) {
    Vec3 orig{0.0f, 0.0f, 0.0f};
    Vec3 out;
    range_pos_at_orig(orig, /*range*/1000, out, /*rand_x*/1000, /*rand_z*/500);
    // half = 500; dx = 1000-500 = 500; dz = 500-500 = 0
    EXPECT_FLOAT_EQ(out.x, 500.0f);
    EXPECT_FLOAT_EQ(out.z, 0.0f);
}

TEST(RegenManagerRangePos, PreservesY) {
    Vec3 orig{0.0f, 7.5f, 0.0f};
    Vec3 out;
    range_pos_at_orig(orig, 100, out, 50, 50);
    EXPECT_FLOAT_EQ(out.y, 7.5f);
}

// ---- RegenObject ----

TEST(RegenManagerRegenObject, RegistersUnderObjectId) {
    auto s = make_regen_manager();
    RegenPrototype p = make_proto(1u, 100u, 7777u);
    EXPECT_TRUE(regen_object(s, p));
    auto* got = get_prototype(s, 7777u);
    ASSERT_NE(got, nullptr);
    EXPECT_EQ(got->m_dwID, 1u);
}

// ---- RegenGroup ----

TEST(RegenManagerGroup, CountsMatchingGroup) {
    auto s = make_regen_manager();
    add_prototype(s, make_proto(1u, 100u));
    add_prototype(s, make_proto(2u, 100u));
    add_prototype(s, make_proto(3u, 200u));
    EXPECT_EQ(regen_group_count(s, 100u), 2u);
    EXPECT_EQ(regen_group_count(s, 200u), 1u);
    EXPECT_EQ(regen_group_count(s, 999u), 0u);
}

TEST(RegenManagerGroup, ContainsDetects) {
    auto s = make_regen_manager();
    add_prototype(s, make_proto(1u, 100u));
    EXPECT_TRUE(regen_group_contains(s, 100u));
    EXPECT_FALSE(regen_group_contains(s, 200u));
}

// ---- Drop ratio ----

TEST(RegenManagerDropRatio, ZeroAlwaysFails) {
    EXPECT_FALSE(drop_ratio_passes(0u, 0u));
    EXPECT_FALSE(drop_ratio_passes(0u, 9999u));
}

TEST(RegenManagerDropRatio, HundredAlwaysPasses) {
    EXPECT_TRUE(drop_ratio_passes(100u, 0u));
    EXPECT_TRUE(drop_ratio_passes(100u, 9999u));
}

TEST(RegenManagerDropRatio, FiftyAtFiftyFiftyBoundary) {
    EXPECT_TRUE(drop_ratio_passes(50u, 4999u));
    EXPECT_FALSE(drop_ratio_passes(50u, 5000u));
}

TEST(RegenManagerDropRatio, TwentyPassesBelow20Percent) {
    EXPECT_TRUE(drop_ratio_passes(20u, 1999u));
    EXPECT_FALSE(drop_ratio_passes(20u, 2000u));
}

// ---- in_random_range ----

TEST(RegenManagerInRange, SamePointAlwaysInRange) {
    Vec3 a{0.0f, 0.0f, 0.0f};
    Vec3 b{0.0f, 0.0f, 0.0f};
    EXPECT_TRUE(in_random_range(a, b, 100u));
}

TEST(RegenManagerInRange, WithinRadiusReturnsTrue) {
    Vec3 a{0.0f, 0.0f, 0.0f};
    Vec3 b{50.0f, 0.0f, 50.0f};
    EXPECT_TRUE(in_random_range(a, b, 100u));
}

TEST(RegenManagerInRange, OutsideRadiusReturnsFalse) {
    Vec3 a{0.0f, 0.0f, 0.0f};
    Vec3 b{1000.0f, 0.0f, 1000.0f};
    EXPECT_FALSE(in_random_range(a, b, 100u));
}
