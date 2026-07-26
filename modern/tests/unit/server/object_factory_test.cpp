// object_factory_test.cpp - Phase D6 ObjectFactory 1:1 port tests.

#include "mxh/server/object_factory.hpp"

#include <gtest/gtest.h>

namespace {

using mxh::server::object_factory_init;
using mxh::server::object_factory_release;
using mxh::server::object_factory_can_make;
using mxh::server::object_factory_make_new;
using mxh::server::object_factory_release_object;
using mxh::server::object_factory_make_new_ex;
using mxh::server::ObjectFactoryState;
using mxh::server::ObjectKind;
using mxh::server::MakeObjectResult;
using mxh::server::MAX_TOTAL_PLAYER_NUM;
using mxh::server::MAX_TOTAL_MONSTER_NUM;
using mxh::server::MAX_TOTAL_BOSSMONSTER_NUM;

TEST(ObjectFactory, CapsMatchLegacy) {
    ObjectFactoryState s;
    object_factory_init(s);
    EXPECT_EQ(s.cap[static_cast<std::size_t>(ObjectKind::Player)], MAX_TOTAL_PLAYER_NUM);
    EXPECT_EQ(s.cap[static_cast<std::size_t>(ObjectKind::Monster)], MAX_TOTAL_MONSTER_NUM);
    EXPECT_EQ(s.cap[static_cast<std::size_t>(ObjectKind::BossMonster)], MAX_TOTAL_BOSSMONSTER_NUM);
}

TEST(ObjectFactory, InitZeroesActiveAndMarksInitialized) {
    ObjectFactoryState s;
    object_factory_init(s);
    EXPECT_EQ(s.total_active(), 0u);
    EXPECT_TRUE(s.initialized);
}

TEST(ObjectFactory, ReleaseClearsState) {
    ObjectFactoryState s;
    object_factory_init(s);
    object_factory_release(s);
    EXPECT_FALSE(s.initialized);
    EXPECT_EQ(s.total_active(), 0u);
    EXPECT_EQ(s.total_cap(), 0u);
}

TEST(ObjectFactory, MakeNewIncrementsActive) {
    ObjectFactoryState s;
    object_factory_init(s);
    EXPECT_TRUE(object_factory_make_new(s, ObjectKind::Player));
    EXPECT_EQ(s.active[static_cast<std::size_t>(ObjectKind::Player)], 1u);
    EXPECT_TRUE(object_factory_make_new(s, ObjectKind::Player));
    EXPECT_EQ(s.active[static_cast<std::size_t>(ObjectKind::Player)], 2u);
}

TEST(ObjectFactory, ReleaseObjectSaturatesAtZero) {
    ObjectFactoryState s;
    object_factory_init(s);
    object_factory_release_object(s, ObjectKind::Pet);
    EXPECT_EQ(s.active[static_cast<std::size_t>(ObjectKind::Pet)], 0u);
}

TEST(ObjectFactory, CanMakeEnforcesCap) {
    ObjectFactoryState s;
    object_factory_init(s);
    s.cap[static_cast<std::size_t>(ObjectKind::BossMonster)] = 2u;
    EXPECT_TRUE(object_factory_can_make(s, ObjectKind::BossMonster));
    EXPECT_TRUE(object_factory_make_new(s, ObjectKind::BossMonster));
    EXPECT_TRUE(object_factory_make_new(s, ObjectKind::BossMonster));
    EXPECT_FALSE(object_factory_can_make(s, ObjectKind::BossMonster));
    EXPECT_FALSE(object_factory_make_new(s, ObjectKind::BossMonster));
}

TEST(ObjectFactory, MakeNewExReturnsNotInitBeforeInit) {
    ObjectFactoryState s;
    EXPECT_EQ(object_factory_make_new_ex(s, 1), MakeObjectResult::NotInit);
}

TEST(ObjectFactory, MakeNewExDispatchesByKindByte) {
    ObjectFactoryState s;
    object_factory_init(s);
    EXPECT_EQ(object_factory_make_new_ex(s, 1), MakeObjectResult::Created);
    EXPECT_EQ(s.active[static_cast<std::size_t>(ObjectKind::Player)], 1u);
    EXPECT_EQ(object_factory_make_new_ex(s, 32), MakeObjectResult::Created);
    EXPECT_EQ(s.active[static_cast<std::size_t>(ObjectKind::Monster)], 1u);
}

TEST(ObjectFactory, MakeNewExReturnsUnknownKind) {
    ObjectFactoryState s;
    object_factory_init(s);
    EXPECT_EQ(object_factory_make_new_ex(s, 7), MakeObjectResult::UnknownKind);
}

TEST(ObjectFactory, TotalActiveTracksAcrossKinds) {
    ObjectFactoryState s;
    object_factory_init(s);
    object_factory_make_new(s, ObjectKind::Player);
    object_factory_make_new(s, ObjectKind::Pet);
    object_factory_make_new(s, ObjectKind::Monster);
    object_factory_make_new(s, ObjectKind::Monster);
    EXPECT_EQ(s.total_active(), 4u);
}

}  // namespace
