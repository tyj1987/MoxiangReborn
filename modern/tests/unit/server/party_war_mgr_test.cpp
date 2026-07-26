// party_war_mgr_test.cpp - Phase D5 PartyWarMgr 1:1 port tests.

#include "mxh/server/party_war_mgr.hpp"
#include <gtest/gtest.h>

namespace {
using mxh::server::PartyWarMgrState;
using mxh::server::PartyWar;
using mxh::server::PartyWarTeam;
using mxh::server::PartyWarState;
using mxh::server::PWMember;
using mxh::server::Vec3;
using mxh::server::MAX_PARTY_LISTNUM;
using mxh::server::MAX_NAME_LENGTH;
using mxh::server::make_party_war_mgr;
using mxh::server::party_war_mgr_init;
using mxh::server::party_war_mgr_release;
using mxh::server::party_war_team_set_party_idx;
using mxh::server::party_war_team_get_party_idx;
using mxh::server::party_war_team_is_alive;
using mxh::server::party_war_team_set_lock;
using mxh::server::party_war_team_is_locked;
using mxh::server::party_war_team_set_ready;
using mxh::server::party_war_team_is_ready;
using mxh::server::party_war_team_set_master_name;
using mxh::server::party_war_team_init_member;
using mxh::server::party_war_team_add_member;
using mxh::server::party_war_team_is_addable_member;
using mxh::server::party_war_team_is_war_member;
using mxh::server::party_war_team_remove_member;
using mxh::server::party_war_team_member_die;
using mxh::server::party_war_init;
using mxh::server::party_war_get_index;
using mxh::server::party_war_get_state;
using mxh::server::party_war_get_party_indices;
using mxh::server::party_war_is_member;
using mxh::server::party_war_is_enemy;
using mxh::server::party_war_player_die;
using mxh::server::party_war_remove_player;
using mxh::server::party_war_process;
using mxh::server::register_party_war;
using mxh::server::find_party_war_by_id;
using mxh::server::unregister_party_war;
}

// ---- Constants 1:1 ----

TEST(PartyWarMgrConstants, MaxPartyListNumMatchesLegacy) {
    EXPECT_EQ(MAX_PARTY_LISTNUM, 7u);
}

TEST(PartyWarMgrConstants, MaxNameLengthMatchesLegacy) {
    EXPECT_EQ(MAX_NAME_LENGTH, 17u);
}

// ---- Team accessors ----

TEST(PartyWarTeamAccessors, PartyIdxRoundTrip) {
    PartyWarTeam t;
    party_war_team_set_party_idx(t, 100u);
    EXPECT_EQ(party_war_team_get_party_idx(t), 100u);
}

TEST(PartyWarTeamAccessors, IsAliveReflectsAliveNum) {
    PartyWarTeam t;
    EXPECT_FALSE(party_war_team_is_alive(t));
    t.m_nAliveNum = 1;
    EXPECT_TRUE(party_war_team_is_alive(t));
}

TEST(PartyWarTeamAccessors, LockToggleRoundTrip) {
    PartyWarTeam t;
    party_war_team_set_lock(t, true);
    EXPECT_TRUE(party_war_team_is_locked(t));
    party_war_team_set_lock(t, false);
    EXPECT_FALSE(party_war_team_is_locked(t));
}

TEST(PartyWarTeamAccessors, ReadyToggleRoundTrip) {
    PartyWarTeam t;
    party_war_team_set_ready(t, true);
    EXPECT_TRUE(party_war_team_is_ready(t));
}

TEST(PartyWarTeamAccessors, MasterNameCopiesWithNul) {
    PartyWarTeam t;
    party_war_team_set_master_name(t, "Alice");
    EXPECT_EQ(t.m_sMasterName[0], 'A');
    EXPECT_EQ(t.m_sMasterName[5], 0);
}

// ---- Member init / add / remove ----

TEST(PartyWarTeamMember, InitIncrementsAlive) {
    PartyWarTeam t;
    party_war_team_init_member(t, 100u, 0);
    EXPECT_EQ(t.m_Member[0].dwMemberIdx, 100u);
    EXPECT_TRUE(t.m_Member[0].bEnableWar);
    EXPECT_EQ(t.m_nAliveNum, 1);
}

TEST(PartyWarTeamMember, AddAlsoIncrements) {
    PartyWarTeam t;
    party_war_team_add_member(t, 100u, 0);
    party_war_team_add_member(t, 200u, 1);
    EXPECT_EQ(t.m_nAliveNum, 2);
    EXPECT_TRUE(party_war_team_is_war_member(t, 100u));
    EXPECT_TRUE(party_war_team_is_war_member(t, 200u));
}

TEST(PartyWarTeamMember, OutOfRangeInitIsNoOp) {
    PartyWarTeam t;
    party_war_team_init_member(t, 100u, -1);
    party_war_team_init_member(t, 100u, static_cast<int>(MAX_PARTY_LISTNUM));
    EXPECT_EQ(t.m_nAliveNum, 0);
}

TEST(PartyWarTeamMember, IsAddableRejectsOccupied) {
    PartyWarTeam t;
    party_war_team_init_member(t, 100u, 0);
    EXPECT_FALSE(party_war_team_is_addable_member(t, 200u, 0));
    EXPECT_TRUE(party_war_team_is_addable_member(t, 200u, 1));
}

TEST(PartyWarTeamMember, IsAddableRejectsDuplicateAcrossSlots) {
    PartyWarTeam t;
    party_war_team_init_member(t, 100u, 0);
    EXPECT_FALSE(party_war_team_is_addable_member(t, 100u, 1));
}

TEST(PartyWarTeamMember, IsAddableRejectsOutOfRange) {
    PartyWarTeam t;
    EXPECT_FALSE(party_war_team_is_addable_member(t, 100u, -1));
    EXPECT_FALSE(party_war_team_is_addable_member(t, 100u, static_cast<int>(MAX_PARTY_LISTNUM)));
}

TEST(PartyWarTeamMember, IsWarMemberFalseWhenDisabled) {
    PartyWarTeam t;
    party_war_team_init_member(t, 100u, 0);
    t.m_Member[0].bEnableWar = false;
    EXPECT_FALSE(party_war_team_is_war_member(t, 100u));
}

TEST(PartyWarTeamMember, IsWarMemberFalseForUnknown) {
    PartyWarTeam t;
    EXPECT_FALSE(party_war_team_is_war_member(t, 999u));
}

TEST(PartyWarTeamMember, RemoveMemberDecrementsAlive) {
    PartyWarTeam t;
    party_war_team_init_member(t, 100u, 0);
    party_war_team_remove_member(t, 100u, 0);
    EXPECT_EQ(t.m_nAliveNum, 0);
    EXPECT_EQ(t.m_Member[0].dwMemberIdx, 0u);
}

TEST(PartyWarTeamMember, RemoveMemberWrongIndexIsNoOp) {
    PartyWarTeam t;
    party_war_team_init_member(t, 100u, 0);
    party_war_team_remove_member(t, 999u, 0);  // wrong member
    EXPECT_EQ(t.m_nAliveNum, 1);
}

TEST(PartyWarTeamMember, RemoveMemberWrongSlotIsNoOp) {
    PartyWarTeam t;
    party_war_team_init_member(t, 100u, 0);
    party_war_team_remove_member(t, 100u, 5);
    EXPECT_EQ(t.m_nAliveNum, 1);
}

TEST(PartyWarTeamMember, MemberDieFlipsFlagAndDecrementsAlive) {
    PartyWarTeam t;
    party_war_team_init_member(t, 100u, 0);
    EXPECT_TRUE(party_war_team_member_die(t, 100u));
    EXPECT_EQ(t.m_nAliveNum, 0);
    EXPECT_FALSE(t.m_Member[0].bEnableWar);
    EXPECT_FALSE(party_war_team_member_die(t, 100u));  // already dead
}

TEST(PartyWarTeamMember, MemberDieUnknownIsFalse) {
    PartyWarTeam t;
    EXPECT_FALSE(party_war_team_member_die(t, 999u));
}

// ---- PartyWar init ----

TEST(PartyWarInit, SeedsPartyIndicesAndState) {
    PartyWar w;
    party_war_init(w, 100u, 200u, 5u);
    EXPECT_EQ(w.m_dwIdx, 5u);
    EXPECT_EQ(w.m_Team1.m_dwPartyIdx, 100u);
    EXPECT_EQ(w.m_Team2.m_dwPartyIdx, 200u);
    EXPECT_EQ(w.m_nState, static_cast<int>(PartyWarState::PreWait));
}

TEST(PartyWarInit, GetPartyIndices) {
    PartyWar w;
    party_war_init(w, 100u, 200u, 1u);
    std::uint32_t p1 = 0, p2 = 0;
    party_war_get_party_indices(w, p1, p2);
    EXPECT_EQ(p1, 100u);
    EXPECT_EQ(p2, 200u);
}

// ---- IsMember / IsEnemy ----

TEST(PartyWarMembers, IsMemberTeam1) {
    PartyWar w;
    party_war_init(w, 100u, 200u, 1u);
    party_war_team_init_member(w.m_Team1, 11u, 0);
    EXPECT_EQ(party_war_is_member(w, 11u, 100u), 1);
    EXPECT_EQ(party_war_is_member(w, 11u, 200u), 0);
    EXPECT_EQ(party_war_is_member(w, 99u, 100u), 0);
}

TEST(PartyWarMembers, IsMemberTeam2) {
    PartyWar w;
    party_war_init(w, 100u, 200u, 1u);
    party_war_team_init_member(w.m_Team2, 22u, 0);
    EXPECT_EQ(party_war_is_member(w, 22u, 200u), 2);
}

TEST(PartyWarEnemy, CrossTeamIsEnemy) {
    PartyWar w;
    party_war_init(w, 100u, 200u, 1u);
    party_war_team_init_member(w.m_Team1, 11u, 0);
    party_war_team_init_member(w.m_Team2, 22u, 0);
    EXPECT_TRUE(party_war_is_enemy(w, 11u, 22u));
}

TEST(PartyWarEnemy, SameTeamIsNotEnemy) {
    PartyWar w;
    party_war_init(w, 100u, 200u, 1u);
    party_war_team_init_member(w.m_Team1, 11u, 0);
    party_war_team_init_member(w.m_Team1, 12u, 1);
    EXPECT_FALSE(party_war_is_enemy(w, 11u, 12u));
}

TEST(PartyWarEnemy, OneNotInWarIsNotEnemy) {
    PartyWar w;
    party_war_init(w, 100u, 200u, 1u);
    party_war_team_init_member(w.m_Team1, 11u, 0);
    EXPECT_FALSE(party_war_is_enemy(w, 11u, 99u));
}

TEST(PartyWarPlayerDie, MarksDeadAndDecrements) {
    PartyWar w;
    party_war_init(w, 100u, 200u, 1u);
    party_war_team_init_member(w.m_Team1, 11u, 0);
    party_war_team_init_member(w.m_Team1, 12u, 1);
    EXPECT_TRUE(party_war_player_die(w, 11u, 100u));
    EXPECT_EQ(w.m_Team1.m_nAliveNum, 1);
}

TEST(PartyWarRemovePlayer, ClearsSlot) {
    PartyWar w;
    party_war_init(w, 100u, 200u, 1u);
    party_war_team_init_member(w.m_Team1, 11u, 0);
    party_war_remove_player(w, 11u, 100u);
    EXPECT_EQ(w.m_Team1.m_nAliveNum, 0);
}

TEST(PartyWarRemovePlayer, WrongPartyIsNoOp) {
    PartyWar w;
    party_war_init(w, 100u, 200u, 1u);
    party_war_team_init_member(w.m_Team1, 11u, 0);
    party_war_remove_player(w, 11u, 200u);
    EXPECT_EQ(w.m_Team1.m_nAliveNum, 1);
}

// ---- State machine ----

TEST(PartyWarProcess, PreWaitToWaitAfter10s) {
    PartyWar w;
    party_war_init(w, 100u, 200u, 1u);
    EXPECT_FALSE(party_war_process(w, 5000u));
    EXPECT_EQ(w.m_nState, static_cast<int>(PartyWarState::PreWait));
    EXPECT_TRUE(party_war_process(w, 5000u));
    EXPECT_EQ(w.m_nState, static_cast<int>(PartyWarState::Wait));
}

TEST(PartyWarProcess, WaitToReadyWhenBothLocked) {
    PartyWar w;
    party_war_init(w, 100u, 200u, 1u);
    party_war_team_set_lock(w.m_Team1, true);
    party_war_team_set_lock(w.m_Team2, true);
    w.m_nState = static_cast<int>(PartyWarState::Wait);
    EXPECT_TRUE(party_war_process(w, 0u));
    EXPECT_EQ(w.m_nState, static_cast<int>(PartyWarState::Ready));
}

TEST(PartyWarProcess, WaitStaysWhenNotLocked) {
    PartyWar w;
    party_war_init(w, 100u, 200u, 1u);
    w.m_nState = static_cast<int>(PartyWarState::Wait);
    EXPECT_FALSE(party_war_process(w, 0u));
    EXPECT_EQ(w.m_nState, static_cast<int>(PartyWarState::Wait));
}

TEST(PartyWarProcess, ReadyToFightWhenBothReady) {
    PartyWar w;
    party_war_init(w, 100u, 200u, 1u);
    party_war_team_set_ready(w.m_Team1, true);
    party_war_team_set_ready(w.m_Team2, true);
    w.m_nState = static_cast<int>(PartyWarState::Ready);
    EXPECT_TRUE(party_war_process(w, 0u));
    EXPECT_EQ(w.m_nState, static_cast<int>(PartyWarState::Fight));
}

TEST(PartyWarProcess, FightTeam2WinsWhenTeam1Zero) {
    PartyWar w;
    party_war_init(w, 100u, 200u, 1u);
    party_war_team_init_member(w.m_Team2, 22u, 0);
    w.m_nState = static_cast<int>(PartyWarState::Fight);
    EXPECT_TRUE(party_war_process(w, 0u));
    EXPECT_EQ(w.m_dwWinner, 2u);
    EXPECT_EQ(w.m_nState, static_cast<int>(PartyWarState::Result));
}

TEST(PartyWarProcess, FightTeam1WinsWhenTeam2Zero) {
    PartyWar w;
    party_war_init(w, 100u, 200u, 1u);
    party_war_team_init_member(w.m_Team1, 11u, 0);
    w.m_nState = static_cast<int>(PartyWarState::Fight);
    EXPECT_TRUE(party_war_process(w, 0u));
    EXPECT_EQ(w.m_dwWinner, 1u);
}

TEST(PartyWarProcess, ResultTransitionsToEnd) {
    PartyWar w;
    party_war_init(w, 100u, 200u, 1u);
    w.m_nState = static_cast<int>(PartyWarState::Result);
    EXPECT_TRUE(party_war_process(w, 0u));
    EXPECT_EQ(w.m_nState, static_cast<int>(PartyWarState::End));
}

// ---- Manager register / find ----

TEST(PartyWarMgrRegister, AssignsIncrementingId) {
    auto s = make_party_war_mgr();
    auto idx1 = register_party_war(s, 100u, 200u);
    auto idx2 = register_party_war(s, 300u, 400u);
    ASSERT_TRUE(idx1.has_value());
    ASSERT_TRUE(idx2.has_value());
    EXPECT_NE(*idx1, *idx2);
    EXPECT_EQ(s.m_PartyWarTable.size(), 2u);
}

TEST(PartyWarMgrFind, FindsById) {
    auto s = make_party_war_mgr();
    auto idx = register_party_war(s, 100u, 200u);
    ASSERT_TRUE(idx.has_value());
    PartyWar* w = find_party_war_by_id(s, *idx);
    ASSERT_NE(w, nullptr);
    EXPECT_EQ(w->m_dwIdx, *idx);
}

TEST(PartyWarMgrFind, MissingReturnsNull) {
    auto s = make_party_war_mgr();
    EXPECT_EQ(find_party_war_by_id(s, 9999u), nullptr);
}

TEST(PartyWarMgrUnregister, RemovesById) {
    auto s = make_party_war_mgr();
    auto idx = register_party_war(s, 100u, 200u);
    ASSERT_TRUE(idx.has_value());
    EXPECT_TRUE(unregister_party_war(s, *idx));
    EXPECT_TRUE(s.m_PartyWarTable.empty());
}

TEST(PartyWarMgrInit, ResetsAll) {
    auto s = make_party_war_mgr();
    register_party_war(s, 100u, 200u);
    party_war_mgr_init(s);
    EXPECT_TRUE(s.m_PartyWarTable.empty());
    EXPECT_EQ(s.m_dwPartyWarTableIdx, 1u);
}
