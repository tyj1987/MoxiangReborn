// regen_prototype_test.cpp - Phase D6 RegenPrototype 1:1 port tests.

#include "mxh/server/regen_prototype.hpp"

#include <gtest/gtest.h>

namespace {

using mxh::server::RegenPrototype;
using mxh::server::RegenObject;
using mxh::server::Vector3;
using mxh::server::regen_object_init_prototype;
using mxh::server::regen_object_init_help_type;

TEST(RegenPrototype, DefaultPrototypeFieldValues) {
    RegenPrototype p{};
    EXPECT_EQ(p.RegenType, 0);
    EXPECT_EQ(p.ObjectKind, 0);
    EXPECT_EQ(p.wMonsterKind, 0);
    EXPECT_EQ(p.dwObjectID, 0u);
    EXPECT_FLOAT_EQ(p.vPos.x, 0.0f);
    EXPECT_FLOAT_EQ(p.vPos.y, 0.0f);
    EXPECT_FLOAT_EQ(p.vPos.z, 0.0f);
    EXPECT_EQ(p.InitHelpType, 0);
    EXPECT_FALSE(p.bHearing);
    EXPECT_EQ(p.HearingDistance, 0u);
}

TEST(RegenPrototype, DefaultObjectZeroedFields) {
    RegenObject o{};
    EXPECT_EQ(o.m_dwObjectID, 0u);
    EXPECT_EQ(o.m_dwSubObjectID, 0u);
    EXPECT_EQ(o.m_dwGridID, 0u);
    EXPECT_EQ(o.m_dwGroupID, 0u);
    EXPECT_EQ(o.m_CurHelpType, 0);
    EXPECT_EQ(o.m_pPrototype, nullptr);
}

TEST(RegenObject, InitPrototypeStoresPointer) {
    RegenPrototype p{};
    p.wMonsterKind = 42;
    RegenObject o{};
    regen_object_init_prototype(o, &p);
    EXPECT_TRUE(o.has_prototype());
    EXPECT_EQ(o.get_monster_kind(), 42u);
}

TEST(RegenObject, GetterFailsBeforePrototype) {
    RegenObject o{};
    EXPECT_FALSE(o.has_prototype());
}

TEST(RegenObject, HearingAndDistance) {
    RegenPrototype p{};
    p.bHearing = true;
    p.HearingDistance = 1000u;
    RegenObject o{};
    regen_object_init_prototype(o, &p);
    EXPECT_TRUE(o.is_hearing());
    EXPECT_EQ(o.get_hearing_distance(), 1000u);
}

TEST(RegenObject, InitHelpTypeCopiesFromPrototype) {
    RegenPrototype p{};
    p.InitHelpType = 7;
    RegenObject o{};
    regen_object_init_prototype(o, &p);
    regen_object_init_help_type(o);
    EXPECT_EQ(o.get_cur_help_type(), 7);
}

TEST(RegenObject, InitHelpTypeWithoutPrototypeNoOp) {
    RegenObject o{};
    o.m_CurHelpType = 99;
    regen_object_init_help_type(o);
    EXPECT_EQ(o.get_cur_help_type(), 99);
}

TEST(RegenObject, SetCurHelpTypeOverrides) {
    RegenObject o{};
    o.set_cur_help_type(11);
    EXPECT_EQ(o.get_cur_help_type(), 11);
}

TEST(RegenObject, GetterAccessorsReturnStoredValues) {
    RegenPrototype p{};
    p.ObjectKind = 7;
    p.vPos = Vector3{1.0f, 2.0f, 3.0f};
    RegenObject o{};
    o.m_dwObjectID = 99;
    o.m_dwSubObjectID = 88;
    o.m_dwGridID = 200;
    o.m_dwGroupID = 300;
    regen_object_init_prototype(o, &p);
    EXPECT_EQ(o.get_object_kind(), 7);
    EXPECT_EQ(o.get_sub_id(), 88u);
    EXPECT_EQ(o.get_group_id(), 300u);
    EXPECT_FLOAT_EQ(o.get_pos()->x, 1.0f);
    EXPECT_FLOAT_EQ(o.get_pos()->y, 2.0f);
}

}  // namespace
