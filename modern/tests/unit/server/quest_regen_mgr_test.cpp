// quest_regen_mgr_test.cpp - Phase D5 QuestRegenMgr 1:1 port tests.

#include "mxh/server/quest_regen_mgr.hpp"
#include <gtest/gtest.h>

namespace {
using mxh::server::QuestRegenMgrState;
using mxh::server::QuestRegenInfo;
using mxh::server::QRPos;
using mxh::server::Vec3;
using mxh::server::make_quest_regen_mgr;
using mxh::server::quest_regen_mgr_init;
using mxh::server::quest_regen_mgr_release;
using mxh::server::add_regen_info;
using mxh::server::find_regen_info;
using mxh::server::regen_info_count;
using mxh::server::regen_info_total_monsters;
using mxh::server::pick_regen_pos;
using mxh::server::pick_regen_radius;

static QuestRegenInfo make_info(std::uint16_t count = 5u, std::uint16_t kind = 7u,
                                Vec3 one = Vec3{0.0f, 0.0f, 0.0f},
                                std::uint16_t radius = 100u) {
    QuestRegenInfo i;
    i.m_bCondition = 1;
    i.m_wMonsterCount = count;
    i.m_wMonsterKind = kind;
    i.m_wRadius = radius;
    i.m_vOnePos = one;
    return i;
}
}

// ---- Default / Init ----

TEST(QuestRegenMgrInit, DefaultEmpty) {
    auto s = make_quest_regen_mgr();
    EXPECT_EQ(regen_info_count(s), 0u);
}

TEST(QuestRegenMgrInit, InitClearsAll) {
    auto s = make_quest_regen_mgr();
    add_regen_info(s, 1u, make_info());
    quest_regen_mgr_init(s);
    EXPECT_EQ(regen_info_count(s), 0u);
}

// ---- Add / Find ----

TEST(QuestRegenMgrAddFind, RegistersAndLooksUp) {
    auto s = make_quest_regen_mgr();
    add_regen_info(s, 42u, make_info(3u, 8u));
    auto* info = find_regen_info(s, 42u);
    ASSERT_NE(info, nullptr);
    EXPECT_EQ(info->m_wMonsterCount, 3u);
    EXPECT_EQ(info->m_wMonsterKind, 8u);
}

TEST(QuestRegenMgrAddFind, MissingReturnsNull) {
    auto s = make_quest_regen_mgr();
    EXPECT_EQ(find_regen_info(s, 9999u), nullptr);
}

TEST(QuestRegenMgrAddFind, OverwriteSameId) {
    auto s = make_quest_regen_mgr();
    add_regen_info(s, 1u, make_info(3u, 8u));
    add_regen_info(s, 1u, make_info(5u, 9u));
    auto* info = find_regen_info(s, 1u);
    ASSERT_NE(info, nullptr);
    EXPECT_EQ(info->m_wMonsterCount, 5u);
    EXPECT_EQ(info->m_wMonsterKind, 9u);
}

// ---- Total monsters ----

TEST(QuestRegenMgrCount, TotalMonstersReturnsCount) {
    auto i = make_info(7u, 8u);
    EXPECT_EQ(regen_info_total_monsters(i), 7u);
}

// ---- Pick position (single) ----

TEST(QuestRegenMgrPickPos, SinglePosReturnsOnePos) {
    auto i = make_info(5u, 8u, Vec3{10.0f, 0.0f, 20.0f}, 100u);
    Vec3 p = pick_regen_pos(i, /*player*/ 12345u);
    EXPECT_FLOAT_EQ(p.x, 10.0f);
    EXPECT_FLOAT_EQ(p.z, 20.0f);
}

TEST(QuestRegenMgrPickRadius, SinglePosReturnsRadius) {
    auto i = make_info(5u, 8u, Vec3{}, 250u);
    EXPECT_EQ(pick_regen_radius(i, 1u), 250u);
}

// ---- Pick position (multi) ----

TEST(QuestRegenMgrPickPos, MultiPosCyclesByPlayerId) {
    auto i = make_info(5u, 8u);
    QRPos p0; p0.vPos = Vec3{10.0f, 0.0f, 0.0f}; p0.wRadius = 100u;
    QRPos p1; p1.vPos = Vec3{20.0f, 0.0f, 0.0f}; p1.wRadius = 200u;
    QRPos p2; p2.vPos = Vec3{30.0f, 0.0f, 0.0f}; p2.wRadius = 300u;
    i.m_pPos.push_back(p0);
    i.m_pPos.push_back(p1);
    i.m_pPos.push_back(p2);

    EXPECT_FLOAT_EQ(pick_regen_pos(i, 0u).x, 10.0f);
    EXPECT_FLOAT_EQ(pick_regen_pos(i, 1u).x, 20.0f);
    EXPECT_FLOAT_EQ(pick_regen_pos(i, 2u).x, 30.0f);
    EXPECT_FLOAT_EQ(pick_regen_pos(i, 3u).x, 10.0f);  // wraps
    EXPECT_FLOAT_EQ(pick_regen_pos(i, 100u).x, 20.0f);
}

TEST(QuestRegenMgrPickRadius, MultiPosReturnsPerSlot) {
    auto i = make_info();
    QRPos p0; p0.vPos = Vec3{}; p0.wRadius = 100u;
    QRPos p1; p1.vPos = Vec3{}; p1.wRadius = 200u;
    i.m_pPos.push_back(p0);
    i.m_pPos.push_back(p1);
    EXPECT_EQ(pick_regen_radius(i, 0u), 100u);
    EXPECT_EQ(pick_regen_radius(i, 1u), 200u);
    EXPECT_EQ(pick_regen_radius(i, 2u), 100u);
}
