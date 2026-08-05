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
using mxh::server::serialize_exp_point_wire;
using mxh::server::serialize_ability_exp_point_wire;
using mxh::server::DistributeRecipient;
using mxh::server::distribute_decide_recipient;
using mxh::server::allocate_party_exp_per_member;
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

// Legacy 1:1 lock: SetPlusTotalDamage uses `m_TotalDamage += Damage;`
// rather than assignment. The modern port had been assigning, which
// silently dropped cumulative plus damage when a second crit hit landed
// while the previous plus damage was still in flight (e.g. back-to-back
// SetPlusTotalDamage in the field-boss reward path). Pin the += form
// for both the empty state and the additive case.
TEST(DistributerPlus, SetPlusAccumulatesOnExistingTotal) {
    auto s = make_distributer();
    set_plus_total_damage(s, 100u);
    EXPECT_EQ(s.m_TotalDamage, 100u);
    set_plus_total_damage(s, 250u);
    // Legacy: m_TotalDamage += 250 -> 350. m_PlusDamage overwrites.
    EXPECT_EQ(s.m_TotalDamage, 350u);
    EXPECT_EQ(s.m_PlusDamage, 250u);
}

TEST(DistributerPlus, SetPlusAccumulatesOverSoloDamage) {
    // Mimics the legacy flow: a player hits, AddDamageObject records the
    // base damage into m_TotalDamage, then SetPlusTotalDamage folds the
    // crit/plus on top. The new code must keep both, not overwrite.
    auto s = make_distributer();
    add_damage_object(s, 7u, 400u, false);
    EXPECT_EQ(s.m_TotalDamage, 400u);
    set_plus_total_damage(s, 150u);
    EXPECT_EQ(s.m_TotalDamage, 550u);
    EXPECT_EQ(s.m_PlusDamage, 150u);
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

// Legacy 1:1 tie-break: identical damage triggers a `rand()%2 == 1` flip
// in CDistributer::ChooseOne. We pin the two outcomes below by passing
// `rand_0_1` directly (0 keeps the first winner, 1 replaces it).
TEST(DistributerChooseOne, TieKeepsFirstWinnerWhenRandIsZero) {
    auto s = make_distributer();
    add_damage_object(s, 1u, 100u, false);
    add_damage_object(s, 2u, 100u, false);
    add_damage_object(s, 3u, 100u, false);
    // Default rand_0_1 = 0: every tie is a no-op, so the iteration-order
    // first id (1) wins. This matches the legacy behaviour for the head
    // of the legacy hash bucket (the first inserted id won when the coin
    // flip returned 0).
    EXPECT_EQ(choose_one(s, false), 1u);
    // Explicitly passing 0 must yield the same outcome as the default.
    EXPECT_EQ(choose_one(s, false, 0), 1u);
}

TEST(DistributerChooseOne, TieReplacesWithLastIdWhenRandIsOne) {
    auto s = make_distributer();
    add_damage_object(s, 1u, 100u, false);
    add_damage_object(s, 2u, 100u, false);
    add_damage_object(s, 3u, 100u, false);
    // rand_0_1 = 1 (== legacy rand()%2 == 1): every tie replaces, so the
    // iteration-order last id (3) wins. The legacy ChooseOne logic is
    // `if (BigDamage < obj.dwData) replace; else if equal && rand%2==1
    // replace;` -- chaining it on a triple-tie flips the winner twice
    // and lands on the final id seen in iteration order.
    EXPECT_EQ(choose_one(s, false, 1), 3u);
}

TEST(DistributerChooseOne, TieReplacesOnlyEqualEntries) {
    auto s = make_distributer();
    add_damage_object(s, 1u, 200u, false);
    add_damage_object(s, 2u, 100u, false);
    add_damage_object(s, 3u, 100u, false);
    // Damage 200 beats 100 outright; the tie-break is only consulted
    // between the two 100-damage entries, leaving id=1 as the winner.
    EXPECT_EQ(choose_one(s, false, 1), 1u);
}

TEST(DistributerChooseOne, PartyTableAppliesTieBreak) {
    auto s = make_distributer();
    add_damage_object(s, 10u, 50u, true);
    add_damage_object(s, 20u, 50u, true);
    EXPECT_EQ(choose_one(s, true, 0), 10u);
    EXPECT_EQ(choose_one(s, true, 1), 20u);
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

TEST(DistributerExpWire, CharacterExpPointUsesLegacyPackedLayout) {
    const auto wire = serialize_exp_point_wire(0x01020304u, 0x0102030405060708LL, 1u);
    ASSERT_EQ(wire.size(), 17u);
    EXPECT_EQ(wire[2], 3u);
    EXPECT_EQ(wire[3], 13u);
    EXPECT_EQ(wire[4], 0x04u);
    EXPECT_EQ(wire[7], 0x01u);
    EXPECT_EQ(wire[8], 0x08u);
    EXPECT_EQ(wire[15], 0x01u);
    EXPECT_EQ(wire[16], 1u);
}

TEST(DistributerExpWire, CharacterExpPointPreservesNegativeSignedValue) {
    const auto wire = serialize_exp_point_wire(7u, -1, 2u, 99u, 88u);
    ASSERT_EQ(wire.size(), 17u);
    EXPECT_EQ(wire[2], 99u);
    EXPECT_EQ(wire[3], 88u);
    for (std::size_t i = 8; i < 16; ++i) EXPECT_EQ(wire[i], 0xffu);
    EXPECT_EQ(wire[16], 2u);
}

TEST(DistributerExpWire, AbilityExpPointUsesLegacyDwordLayout) {
    const auto wire = serialize_ability_exp_point_wire(0xaabbccddu, 0x01020304u, 0u);
    ASSERT_EQ(wire.size(), 13u);
    EXPECT_EQ(wire[2], 3u);
    EXPECT_EQ(wire[3], 29u);
    EXPECT_EQ(wire[4], 0xddu);
    EXPECT_EQ(wire[7], 0xaau);
    EXPECT_EQ(wire[8], 0x04u);
    EXPECT_EQ(wire[11], 0x01u);
    EXPECT_EQ(wire[12], 0u);
}

// ---- distribute_decide_recipient 1:1 lock ----
TEST(DistributerRecipient, PartyDamageLessGivesPersonalToBigPlayer) {
    const auto d = distribute_decide_recipient(
        /*big_player_id*/ 11u, /*big_player_damage*/ 200u,
        /*big_party_id*/ 22u, /*big_party_damage*/ 100u,
        /*party_contains_big_player*/ false);
    EXPECT_EQ(d.kind, DistributeRecipient::Personal);
    EXPECT_EQ(d.player_id, 11u);
    EXPECT_EQ(d.party_id, 0u);
}

TEST(DistributerRecipient, PartyDamageGreaterGivesParty) {
    const auto d = distribute_decide_recipient(
        11u, 50u, 22u, 300u, false);
    EXPECT_EQ(d.kind, DistributeRecipient::Party);
    EXPECT_EQ(d.party_id, 22u);
    EXPECT_EQ(d.player_id, 0u);
}

TEST(DistributerRecipient, TieWithBigPlayerInPartySkipsCoinFlip) {
    // 1:1 quirk: tie + member -> Party; the host still draws rand() first.
    const auto d = distribute_decide_recipient(
        11u, 100u, 22u, 100u, /*party_contains_big_player*/ true, 0);
    EXPECT_EQ(d.kind, DistributeRecipient::Party);
    EXPECT_EQ(d.party_id, 22u);
}

TEST(DistributerRecipient, MissingPartyDoesNotDispatchOnTie) {
    const auto d = distribute_decide_recipient(
        11u, 100u, 22u, 100u, std::nullopt, 1);
    EXPECT_EQ(d.kind, DistributeRecipient::None);
    EXPECT_EQ(d.party_id, 0u);
    EXPECT_EQ(d.player_id, 0u);
}

TEST(DistributerRecipient, TieWithBigPlayerNotInPartyUsesCoin) {
    const auto d_personal = distribute_decide_recipient(
        11u, 100u, 22u, 100u, /*party_contains_big_player*/ false, 0);
    EXPECT_EQ(d_personal.kind, DistributeRecipient::Personal);
    EXPECT_EQ(d_personal.player_id, 11u);
    const auto d_party = distribute_decide_recipient(
        11u, 100u, 22u, 100u, false, 1);
    EXPECT_EQ(d_party.kind, DistributeRecipient::Party);
    EXPECT_EQ(d_party.party_id, 22u);
}

// ---- allocate_party_exp_per_member 1:1 lock ----
TEST(DistributerPartyExp, ZeroPartyExpIsNoOp) {
    const std::vector<std::pair<std::uint32_t, std::uint32_t>> members{
        {1u, 50u}, {2u, 60u}};
    const auto out = allocate_party_exp_per_member(members, 0u);
    EXPECT_EQ(out.online_count, 0u);
    EXPECT_TRUE(out.members.empty());
}

TEST(DistributerPartyExp, SingleMemberGetsEntirePartyExp) {
    // online == 1 -> the whole partyexp goes to that one member
    // unchanged (legacy `if (onlinenumconfirm != 1) ... else exp =
    // partyexp;` quirk).
    const std::vector<std::pair<std::uint32_t, std::uint32_t>> members{{7u, 45u}};
    const auto out = allocate_party_exp_per_member(members, 1000u);
    ASSERT_EQ(out.online_count, 1u);
    EXPECT_EQ(out.max_level, 45u);
    EXPECT_FLOAT_EQ(out.level_average, 45.0f);
    ASSERT_EQ(out.members.size(), 1u);
    EXPECT_EQ(out.members[0].member_id, 7u);
    EXPECT_EQ(out.members[0].exp, 1000u);
}

TEST(DistributerPartyExp, TwoMembersShareByLevelWeight) {
    // online=2, levelavg=(50+70)/2=60, multiplier=(cur*12/9/avg)/2.
    //   member 50: 50*12/9/60/2 = 600/1080 = 0.5555...; * 1000 = 555.
    //   member 70: 70*12/9/60/2 = 840/1080 = 0.7777...; * 1000 = 777.
    const std::vector<std::pair<std::uint32_t, std::uint32_t>> members{
        {1u, 50u}, {2u, 70u}};
    const auto out = allocate_party_exp_per_member(members, 1000u);
    ASSERT_EQ(out.online_count, 2u);
    EXPECT_EQ(out.max_level, 70u);
    EXPECT_FLOAT_EQ(out.level_average, 60.0f);
    ASSERT_EQ(out.members.size(), 2u);
    EXPECT_EQ(out.members[0].member_id, 1u);
    EXPECT_EQ(out.members[0].exp, 555u);
    EXPECT_EQ(out.members[1].member_id, 2u);
    EXPECT_EQ(out.members[1].exp, 777u);
}

TEST(DistributerPartyExp, ZeroComputedExpIsNotSent) {
    const std::vector<std::pair<std::uint32_t, std::uint32_t>> members{
        {1u, 1u}, {2u, 1u}};
    const auto out = allocate_party_exp_per_member(members, 1u);
    EXPECT_EQ(out.online_count, 2u);
    EXPECT_TRUE(out.members.empty());
}

TEST(DistributerPartyExp, ZeroMemberIdIsSkipped) {
    // legacy loop: `if (PlayerID == 0) continue;` so an empty slot in
    // the party array must be ignored entirely.
    const std::vector<std::pair<std::uint32_t, std::uint32_t>> members{
        {0u, 0u}, {1u, 60u}, {0u, 0u}, {2u, 90u}};
    const auto out = allocate_party_exp_per_member(members, 600u);
    EXPECT_EQ(out.online_count, 2u);
    EXPECT_EQ(out.max_level, 90u);
    EXPECT_FLOAT_EQ(out.level_average, 75.0f);
    ASSERT_EQ(out.members.size(), 2u);
    EXPECT_EQ(out.members[0].member_id, 1u);
    EXPECT_EQ(out.members[1].member_id, 2u);
}
