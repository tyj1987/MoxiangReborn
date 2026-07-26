// titan_manager_test.cpp - Phase D5 TitanManager 1:1 port tests.

#include "mxh/server/titan_manager.hpp"
#include <gtest/gtest.h>

namespace {
using mxh::server::TitanManagerState;
using mxh::server::TitanTotalInfo;
using mxh::server::TitanEnduranceItemInfo;
using mxh::server::TitanCalcStats;
using mxh::server::TitanShopitemOption;
using mxh::server::GetOffReason;
using mxh::server::EnduranceException;
using mxh::server::EnduranceCalcPoint;
using mxh::server::TitanWearedInfo;
using mxh::server::kTitanWearedItemMax;
using mxh::server::MAX_TITANGRADE;
using mxh::server::TITAN_EQUIPITEM_ENDURANCE_MAX;
using mxh::server::TITAN_STATE_CHECKTIME;
using mxh::server::make_titan_manager;
using mxh::server::init_titan_manager;
using mxh::server::add_titan_total_info;
using mxh::server::remove_titan_total_info;
using mxh::server::find_titan_total_info;
using mxh::server::upgrade_titan;
using mxh::server::start_titan_recall;
using mxh::server::is_titan_recall_active;
using mxh::server::init_titan_recall;
using mxh::server::set_recall_check_time;
using mxh::server::check_recall_available;
using mxh::server::add_endurance;
using mxh::server::plus_item_endurance;
using mxh::server::minus_item_endurance;
using mxh::server::set_weared_info;
using mxh::server::clear_weared_info;
using mxh::server::add_cur_titan_fuel;
using mxh::server::add_cur_titan_spell;
using mxh::server::add_cur_titan_fuel_as_rate;
using mxh::server::add_cur_titan_spell_as_rate;

static TitanTotalInfo make_titan(std::uint32_t db_idx = 1u,
                                 std::uint16_t kind = 1u,
                                 std::uint16_t grade = 1u,
                                 std::uint32_t fuel = 1000u,
                                 std::uint32_t spell = 500u) {
    TitanTotalInfo t;
    t.TitanCallItemDBIdx = db_idx;
    t.TitanKind = kind;
    t.TitanGrade = grade;
    t.TitanFuel = fuel;
    t.TitanSpell = spell;
    return t;
}
}

// ---- Constants 1:1 ----

TEST(TitanManagerConstants, MaxGradeMatchesLegacy) {
    EXPECT_EQ(MAX_TITANGRADE, 3u);
}

TEST(TitanManagerConstants, EnduranceMaxMatchesLegacy) {
    EXPECT_EQ(TITAN_EQUIPITEM_ENDURANCE_MAX, 10000000u);
}

TEST(TitanManagerConstants, StateCheckTimeMatchesLegacy) {
    EXPECT_EQ(TITAN_STATE_CHECKTIME, 10000u);
}

TEST(TitanManagerConstants, WearedItemMaxMatchesLegacy) {
    EXPECT_EQ(kTitanWearedItemMax, 7);
}

// ---- Default / Init state ----

TEST(TitanManagerInit, DefaultIsZeroed) {
    auto s = make_titan_manager();
    EXPECT_FALSE(s.m_pCurRidingTitan.has_value());
    EXPECT_EQ(s.m_dwCurRegistTitanCallItemDBIdx, 0u);
    EXPECT_EQ(s.TitanScaleForNewOne, 100);
    EXPECT_TRUE(s.m_bAvaliableEndurance);
    EXPECT_FALSE(s.m_bTitanRecall);
    EXPECT_TRUE(s.m_TitanInfoList.empty());
    EXPECT_TRUE(s.m_ItemEnduranceList.empty());
}

TEST(TitanManagerInit, InitResetsAllFields) {
    auto s = make_titan_manager();
    add_titan_total_info(s, make_titan(7u));
    s.m_bTitanRecall = true;
    init_titan_manager(s);
    EXPECT_TRUE(s.m_TitanInfoList.empty());
    EXPECT_FALSE(s.m_bTitanRecall);
    EXPECT_EQ(s.TitanScaleForNewOne, 100);
}

// ---- Add / Remove / Find titan total info ----

TEST(TitanManagerAddRemove, AddAppends) {
    auto s = make_titan_manager();
    add_titan_total_info(s, make_titan(1u));
    add_titan_total_info(s, make_titan(2u));
    EXPECT_EQ(s.m_TitanInfoList.size(), 2u);
}

TEST(TitanManagerAddRemove, FindByCallItemDBIdx) {
    auto s = make_titan_manager();
    add_titan_total_info(s, make_titan(1u));
    add_titan_total_info(s, make_titan(7u, 2u));
    auto* t = find_titan_total_info(s, 7u);
    ASSERT_NE(t, nullptr);
    EXPECT_EQ(t->TitanKind, 2u);
    EXPECT_EQ(t->TitanCallItemDBIdx, 7u);
}

TEST(TitanManagerAddRemove, RemoveClearsCurRiding) {
    auto s = make_titan_manager();
    add_titan_total_info(s, make_titan(1u));
    s.m_pCurRidingTitan = 1u;
    remove_titan_total_info(s, 1u);
    EXPECT_FALSE(s.m_pCurRidingTitan.has_value());
}

// ---- Grade up ----

TEST(TitanManagerUpgrade, BumpsGrade) {
    auto t = make_titan(1u, 1u, /*grade*/1u);
    EXPECT_TRUE(upgrade_titan(t));
    EXPECT_EQ(t.TitanGrade, 2u);
    EXPECT_TRUE(upgrade_titan(t));
    EXPECT_EQ(t.TitanGrade, 3u);
}

TEST(TitanManagerUpgrade, AtMaxFails) {
    auto t = make_titan(1u, 1u, MAX_TITANGRADE);
    EXPECT_FALSE(upgrade_titan(t));
    EXPECT_EQ(t.TitanGrade, MAX_TITANGRADE);
}

// ---- Recall ----

TEST(TitanManagerRecall, StartSetsFlagAndTime) {
    auto s = make_titan_manager();
    start_titan_recall(s, 5000u);
    EXPECT_TRUE(is_titan_recall_active(s));
    EXPECT_EQ(s.m_dwTitanRecallProcessTime, 5000u);
}

TEST(TitanManagerRecall, InitResets) {
    auto s = make_titan_manager();
    start_titan_recall(s, 5000u);
    init_titan_recall(s);
    EXPECT_FALSE(is_titan_recall_active(s));
    EXPECT_EQ(s.m_dwTitanRecallProcessTime, 0u);
}

TEST(TitanManagerRecall, RecallAvailableIfUnset) {
    auto s = make_titan_manager();
    EXPECT_TRUE(check_recall_available(s, 1000000u));
}

TEST(TitanManagerRecall, RecallBlockedWithin30Sec) {
    auto s = make_titan_manager();
    set_recall_check_time(s, 1000u);
    EXPECT_FALSE(check_recall_available(s, 1000u + 29999u));
}

TEST(TitanManagerRecall, RecallAllowedAfter30Sec) {
    auto s = make_titan_manager();
    set_recall_check_time(s, 1000u);
    EXPECT_TRUE(check_recall_available(s, 1000u + 30000u));
}

// ---- Endurance ----

TEST(TitanManagerEndurance, AddCreatesRecord) {
    auto s = make_titan_manager();
    add_endurance(s, 1u, 5000u);
    EXPECT_EQ(s.m_ItemEnduranceList.size(), 1u);
    EXPECT_EQ(s.m_ItemEnduranceList[0].Endurance, 5000u);
}

TEST(TitanManagerEndurance, PlusClampsAtMax) {
    auto s = make_titan_manager();
    add_endurance(s, 1u, TITAN_EQUIPITEM_ENDURANCE_MAX - 100u);
    plus_item_endurance(s, 1u, 500u);
    EXPECT_EQ(s.m_ItemEnduranceList[0].Endurance, TITAN_EQUIPITEM_ENDURANCE_MAX);
}

TEST(TitanManagerEndurance, MinusClampsAtZero) {
    auto s = make_titan_manager();
    add_endurance(s, 1u, 100u);
    minus_item_endurance(s, 1u, 500u);
    EXPECT_EQ(s.m_ItemEnduranceList[0].Endurance, 0u);
}

TEST(TitanManagerEndurance, PlusUnknownItemIsNoOp) {
    auto s = make_titan_manager();
    plus_item_endurance(s, 999u, 100u);
    EXPECT_TRUE(s.m_ItemEnduranceList.empty());
}

// ---- Weared info ----

TEST(TitanManagerWearedInfo, SetStoresIdxAndDBIdx) {
    auto s = make_titan_manager();
    set_weared_info(s, /*slot*/0, /*item_idx*/100u, /*db_idx*/900u);
    EXPECT_EQ(s.m_TitanWearedInfo[0].TitanEquipItemIdx, 100u);
    EXPECT_EQ(s.m_TitanWearedInfo[0].TitanEquipItemDBIdx, 900u);
}

TEST(TitanManagerWearedInfo, ClearResetsSlot) {
    auto s = make_titan_manager();
    set_weared_info(s, 0, 100u, 900u);
    clear_weared_info(s, 0);
    EXPECT_EQ(s.m_TitanWearedInfo[0].TitanEquipItemIdx, 0u);
    EXPECT_EQ(s.m_TitanWearedInfo[0].TitanEquipItemDBIdx, 0u);
}

TEST(TitanManagerWearedInfo, OutOfRangeIsNoOp) {
    auto s = make_titan_manager();
    set_weared_info(s, -1, 1u, 1u);
    set_weared_info(s, kTitanWearedItemMax, 1u, 1u);
    for (auto& w : s.m_TitanWearedInfo) {
        EXPECT_EQ(w.TitanEquipItemIdx, 0u);
    }
}

// ---- Fuel / Spell ----

TEST(TitanManagerFuelSpell, AddFuelClampsAtMax) {
    auto t = make_titan(1u, 1u, 1u, 90u);
    add_cur_titan_fuel(t, 100u, 100u);
    EXPECT_EQ(t.TitanFuel, 100u);
}

TEST(TitanManagerFuelSpell, AddSpellClampsAtMax) {
    auto t = make_titan(1u, 1u, 1u, 0u, 90u);
    add_cur_titan_spell(t, 100u, 100u);
    EXPECT_EQ(t.TitanSpell, 100u);
}

TEST(TitanManagerFuelSpell, FuelAsRateAdds) {
    auto t = make_titan(1u, 1u, 1u, 0u);
    add_cur_titan_fuel_as_rate(t, 0.5f, 1000u);
    EXPECT_EQ(t.TitanFuel, 500u);
}

TEST(TitanManagerFuelSpell, SpellAsRateAdds) {
    auto t = make_titan(1u, 1u, 1u, 0u, 0u);
    add_cur_titan_spell_as_rate(t, 0.5f, 1000u);
    EXPECT_EQ(t.TitanSpell, 500u);
}

TEST(TitanManagerFuelSpell, NegativeRateIsNoOp) {
    auto t = make_titan(1u, 1u, 1u, 100u, 100u);
    add_cur_titan_fuel_as_rate(t, -0.5f, 1000u);
    EXPECT_EQ(t.TitanFuel, 100u);
}

// ---- Enum 1:1 ----

TEST(TitanManagerEnum, GetOffReasonValues) {
    EXPECT_EQ(static_cast<int>(GetOffReason::Normal),         0);
    EXPECT_EQ(static_cast<int>(GetOffReason::FromUser),       1);
    EXPECT_EQ(static_cast<int>(GetOffReason::MasterLifeRate), 2);
    EXPECT_EQ(static_cast<int>(GetOffReason::ExhaustFuel),    3);
    EXPECT_EQ(static_cast<int>(GetOffReason::ExhaustSpell),   4);
}

TEST(TitanManagerEnum, EnduranceExceptionValues) {
    EXPECT_EQ(static_cast<int>(EnduranceException::None),   0);
    EXPECT_EQ(static_cast<int>(EnduranceException::Inven),  1);
    EXPECT_EQ(static_cast<int>(EnduranceException::Pyoguk), 2);
}

TEST(TitanManagerEnum, EnduranceCalcPointValues) {
    EXPECT_EQ(static_cast<int>(EnduranceCalcPoint::WhenTitanAttack),  0);
    EXPECT_EQ(static_cast<int>(EnduranceCalcPoint::WhenTitanDefense), 1);
}
