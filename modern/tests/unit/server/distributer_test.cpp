// distributer_test.cpp - Phase D5 Distributer 1:1 port tests.

#include "mxh/server/distributer.hpp"
#include <gtest/gtest.h>
#include <string>

namespace {
using mxh::server::DistributerState;
using mxh::server::DamageObj;
using mxh::server::MAX_POINTACCEPTOBJECT_NUM;
using mxh::server::make_distributer;
using mxh::server::distributer_release;
using mxh::server::damage_init;
using mxh::server::set_plus_total_damage;
using mxh::server::get_total_damage;
using mxh::server::add_damage_object;
using mxh::server::choose_one;
using mxh::server::delete_damaged_player;
using mxh::server::calc_obtain_exp;
using mxh::server::calc_obtain_ability_exp;
}

mxh::server::PlayerMonsterPointTable test_point_table() {
    std::string text;
    for (std::uint16_t level = 1; level <= mxh::server::MAX_PLAYER_LEVEL_NUM; ++level) {
        for (std::int32_t column = 0; column < static_cast<std::int32_t>(mxh::server::PLAYER_MONSTER_POINT_COLUMN_COUNT); ++column) {
            text += std::to_string(static_cast<unsigned>(level) * 100u + static_cast<unsigned>(column));
            text += ' ';
        }
    }
    return mxh::server::PlayerMonsterPointTable::load_from_text(text);
}
// ---- Constants 1:1 ----

TEST(DistributerConstants, MaxPointAcceptObjectNumMatchesLegacy) {
    EXPECT_EQ(MAX_POINTACCEPTOBJECT_NUM, 2u);
}

// ---- Init / Release ----

TEST(DistributerInit, DefaultIsZeroed) {
    auto s = make_distributer();
    EXPECT_EQ(s.m_TotalDamage, 0u);
    EXPECT_EQ(s.m_1stPlayerID, 0u);
    EXPECT_EQ(s.m_PlusDamage, 0u);
    EXPECT_TRUE(s.m_DamageObjectTableSolo.empty());
    EXPECT_TRUE(s.m_DamageObjectTableParty.empty());
}

TEST(DistributerInit, ReleaseClearsAll) {
    auto s = make_distributer();
    add_damage_object(s, 1u, 100u, false);
    distributer_release(s);
    EXPECT_TRUE(s.m_DamageObjectTableSolo.empty());
    EXPECT_EQ(s.m_TotalDamage, 0u);
}

TEST(DistributerInit, DamageInitClearsAll) {
    auto s = make_distributer();
    s.m_1stPlayerID = 999u;
    add_damage_object(s, 1u, 50u, false);
    damage_init(s);
    // legacy damage_init zeroes 1stPlayerID and the damage tables.
    EXPECT_EQ(s.m_1stPlayerID, 0u);
    EXPECT_TRUE(s.m_DamageObjectTableSolo.empty());
    EXPECT_EQ(s.m_TotalDamage, 0u);
    EXPECT_EQ(s.m_PlusDamage, 0u);
}

// ---- Add damage ----

TEST(DistributerAdd, SoloRecordsDamage) {
    auto s = make_distributer();
    add_damage_object(s, 1u, 100u, /*in_party*/false);
    EXPECT_EQ(get_total_damage(s), 100u);
    EXPECT_EQ(s.m_DamageObjectTableSolo.size(), 1u);
    EXPECT_EQ(s.m_DamageObjectTableSolo[1u].dwData, 100u);
    EXPECT_TRUE(s.m_DamageObjectTableParty.empty());
}

TEST(DistributerAdd, PartyRecordsDamage) {
    auto s = make_distributer();
    add_damage_object(s, 1u, 100u, /*in_party*/true);
    EXPECT_EQ(s.m_DamageObjectTableParty.size(), 1u);
    EXPECT_TRUE(s.m_DamageObjectTableSolo.empty());
}

TEST(DistributerAdd, RepeatedAddAccumulates) {
    auto s = make_distributer();
    add_damage_object(s, 1u, 100u, false);
    add_damage_object(s, 1u, 50u, false);
    EXPECT_EQ(s.m_DamageObjectTableSolo[1u].dwData, 150u);
    EXPECT_EQ(get_total_damage(s), 150u);
}

TEST(DistributerAdd, FirstAttackerRecorded) {
    auto s = make_distributer();
    add_damage_object(s, 42u, 10u, false);
    EXPECT_EQ(s.m_1stPlayerID, 42u);
    add_damage_object(s, 100u, 999u, false);
    EXPECT_EQ(s.m_1stPlayerID, 42u);  // first stays
}

// ---- SetPlusTotalDamage ----

TEST(DistributerPlus, SetPlusStoresAndUpdatesTotal) {
    auto s = make_distributer();
    set_plus_total_damage(s, 500u);
    EXPECT_EQ(s.m_PlusDamage, 500u);
    EXPECT_EQ(get_total_damage(s), 500u);
}

// ---- ChooseOne ----

TEST(DistributerChooseOne, SoloPicksBiggest) {
    auto s = make_distributer();
    add_damage_object(s, 1u, 100u, false);
    add_damage_object(s, 2u, 200u, false);
    add_damage_object(s, 3u, 50u, false);
    EXPECT_EQ(choose_one(s, false), 2u);
}

TEST(DistributerChooseOne, PartyPicksBiggest) {
    auto s = make_distributer();
    add_damage_object(s, 1u, 100u, true);
    add_damage_object(s, 2u, 200u, true);
    EXPECT_EQ(choose_one(s, true), 2u);
}

TEST(DistributerChooseOne, EmptyReturnsNullopt) {
    auto s = make_distributer();
    EXPECT_FALSE(choose_one(s, false).has_value());
    EXPECT_FALSE(choose_one(s, true).has_value());
}

// ---- DeleteDamagedPlayer ----

TEST(DistributerDelete, RemovesFromSoloAndUpdatesTotal) {
    auto s = make_distributer();
    add_damage_object(s, 1u, 100u, false);
    add_damage_object(s, 2u, 200u, false);
    delete_damaged_player(s, 1u);
    EXPECT_EQ(s.m_DamageObjectTableSolo.size(), 1u);
    EXPECT_EQ(get_total_damage(s), 200u);
}

TEST(DistributerDelete, RemovesFromPartyAndUpdatesTotal) {
    auto s = make_distributer();
    add_damage_object(s, 1u, 100u, true);
    delete_damaged_player(s, 1u);
    EXPECT_EQ(s.m_DamageObjectTableParty.size(), 0u);
    EXPECT_EQ(get_total_damage(s), 0u);
}

TEST(DistributerDelete, UnknownIsNoOp) {
    auto s = make_distributer();
    add_damage_object(s, 1u, 100u, false);
    delete_damaged_player(s, 999u);
    EXPECT_EQ(get_total_damage(s), 100u);
}

TEST(DistributerDelete, ResetsFirstAttackerIfMatch) {
    auto s = make_distributer();
    add_damage_object(s, 42u, 10u, false);
    delete_damaged_player(s, 42u);
    EXPECT_EQ(s.m_1stPlayerID, 0u);
}

// ---- CalcObtainExp formula ----

TEST(DistributerExp, ZeroLifeReturnsZero) {
    EXPECT_EQ(calc_obtain_exp(test_point_table(), 50, 50, 0u, 100u, 1u), 0u);
}

TEST(DistributerExp, UsesPlayerMonsterPointTableAtFullDamage) {
    const auto table = test_point_table();
    EXPECT_EQ(calc_obtain_exp(table, 50, 50, 1000u, 1000u, 1u),
              table.get_player_point(50, 0));
}

TEST(DistributerExp, ZeroDamageReturnsZero) {
    EXPECT_EQ(calc_obtain_exp(test_point_table(), 50, 50, 1000u, 0u, 1u), 0u);
}

TEST(DistributerExp, DamageBandsUseLegacyFloorPercentages) {
    const auto table = test_point_table();
    const auto point = table.get_player_point(50, 0);
    EXPECT_EQ(calc_obtain_exp(table, 50, 50, 1000u, 800u, 1u),
              static_cast<std::uint32_t>(point * 0.8));
    EXPECT_EQ(calc_obtain_exp(table, 50, 50, 1000u, 600u, 1u),
              static_cast<std::uint32_t>(point * 0.6));
    EXPECT_EQ(calc_obtain_exp(table, 50, 50, 1000u, 400u, 1u),
              static_cast<std::uint32_t>(point * 0.4));
    EXPECT_EQ(calc_obtain_exp(table, 50, 50, 1000u, 200u, 1u),
              static_cast<std::uint32_t>(point * 0.2));
    EXPECT_EQ(calc_obtain_exp(table, 50, 50, 1000u, 199u, 1u), 0u);
}

TEST(DistributerExp, LevelRestrictionsMatchLegacy) {
    const auto table = test_point_table();
    EXPECT_EQ(calc_obtain_exp(table, 50, 56, 1000u, 1000u, 1u), 0u);
    EXPECT_EQ(calc_obtain_exp(table, 50, 40, 1000u, 1000u, 1u),
              table.get_player_point(49, 9));
}

TEST(DistributerExp, NormalMapIgnoresPartyCountParameter) {
    const auto table = test_point_table();
    const auto solo = calc_obtain_exp(table, 50, 50, 1000u, 1000u, 1u);
    const auto party = calc_obtain_exp(table, 50, 50, 1000u, 1000u, 2u);
    EXPECT_EQ(party, solo);
}

// ---- CalcObtainAbilityExp ----

TEST(DistributerAbilityExp, SameLevelUsesPlusFiveTimesTen) {
    EXPECT_EQ(calc_obtain_ability_exp(50, 50), 50u);
}

TEST(DistributerAbilityExp, HigherMonsterLevelUsesLevelDifference) {
    EXPECT_EQ(calc_obtain_ability_exp(60, 50), 140u);
}

TEST(DistributerAbilityExp, MoreThanFiveLevelsAbovePlayerReturnsZero) {
    EXPECT_EQ(calc_obtain_ability_exp(40, 50), 0u);
}

TEST(DistributerAbilityExp, MonsterLevelIsCappedAtPlayerPlusNine) {
    EXPECT_EQ(calc_obtain_ability_exp(120, 95), 140u);
}