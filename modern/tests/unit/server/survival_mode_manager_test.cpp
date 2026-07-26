// survival_mode_manager_test.cpp - Phase D5 SurvivalModeManager 1:1 port tests.

#include "mxh/server/survival_mode_manager.hpp"
#include <gtest/gtest.h>

namespace {
using mxh::server::SurvivalModeManagerState;
using mxh::server::SurvivalModeState;
using mxh::server::SVVMOD_TIME_READY;
using mxh::server::SVVMOD_TIME_END;
using mxh::server::make_survival_manager;
using mxh::server::survival_init;
using mxh::server::survival_release;
using mxh::server::set_cur_mode_state;
using mxh::server::get_cur_mode_state;
using mxh::server::change_state_to;
using mxh::server::survival_tick;
using mxh::server::check_remain_time;
using mxh::server::add_sv_mode_user;
using mxh::server::remove_sv_mode_user;
using mxh::server::is_sv_mode_user;
using mxh::server::sv_mode_user_count;
using mxh::server::add_alive_user;
using mxh::server::remove_alive_user;
using mxh::server::get_alive_user_count;
using mxh::server::set_using_count_limit;
using mxh::server::get_using_count_limit;
using mxh::server::add_item_using_count;
using mxh::server::get_item_using_count;
using mxh::server::ready_to_survival_mode;
using mxh::server::return_to_map;
}

// ---- Constants 1:1 ----

TEST(SurvivalModeConstants, ReadyTimeMatchesLegacy) {
    EXPECT_EQ(SVVMOD_TIME_READY, 15000u);
}

TEST(SurvivalModeConstants, EndTimeMatchesLegacy) {
    EXPECT_EQ(SVVMOD_TIME_END, 10000u);
}

TEST(SurvivalModeEnum, Values) {
    EXPECT_EQ(static_cast<std::uint16_t>(SurvivalModeState::None),  0);
    EXPECT_EQ(static_cast<std::uint16_t>(SurvivalModeState::Ready), 1);
    EXPECT_EQ(static_cast<std::uint16_t>(SurvivalModeState::Fight), 2);
    EXPECT_EQ(static_cast<std::uint16_t>(SurvivalModeState::End),   3);
}

// ---- Init ----

TEST(SurvivalModeInit, DefaultIsNone) {
    auto s = make_survival_manager();
    EXPECT_EQ(s.m_wModeState, 0u);
    EXPECT_EQ(s.m_dwStateRemainTime, 0u);
    EXPECT_EQ(s.m_nUserAlive, 0);
    EXPECT_EQ(sv_mode_user_count(s), 0u);
}

TEST(SurvivalModeInit, InitResetsAll) {
    auto s = make_survival_manager();
    add_sv_mode_user(s, 1u);
    add_alive_user(s, 1u);
    survival_init(s);
    EXPECT_EQ(s.m_wModeState, 0u);
    EXPECT_EQ(s.m_nUserAlive, 0);
    EXPECT_EQ(sv_mode_user_count(s), 0u);
}

// ---- Change state ----

TEST(SurvivalModeChangeState, ReadySetsTimer) {
    auto s = make_survival_manager();
    change_state_to(s, SurvivalModeState::Ready);
    EXPECT_EQ(s.m_dwStateRemainTime, SVVMOD_TIME_READY);
    EXPECT_EQ(get_cur_mode_state(s), SurvivalModeState::Ready);
}

TEST(SurvivalModeChangeState, EndSetsTimer) {
    auto s = make_survival_manager();
    change_state_to(s, SurvivalModeState::End);
    EXPECT_EQ(s.m_dwStateRemainTime, SVVMOD_TIME_END);
}

TEST(SurvivalModeChangeState, FightHasNoTimer) {
    auto s = make_survival_manager();
    change_state_to(s, SurvivalModeState::Fight);
    EXPECT_EQ(s.m_dwStateRemainTime, 0u);
}

TEST(SurvivalModeChangeState, NoneHasNoTimer) {
    auto s = make_survival_manager();
    change_state_to(s, SurvivalModeState::None);
    EXPECT_EQ(s.m_dwStateRemainTime, 0u);
}

// ---- Tick ----

TEST(SurvivalModeTick, DecreasesRemaining) {
    auto s = make_survival_manager();
    change_state_to(s, SurvivalModeState::Ready);
    survival_tick(s, 5000u);
    EXPECT_EQ(s.m_dwStateRemainTime, SVVMOD_TIME_READY - 5000u);
}

TEST(SurvivalModeTick, ReadyToFightOnExpiry) {
    auto s = make_survival_manager();
    change_state_to(s, SurvivalModeState::Ready);
    const bool transitioned = survival_tick(s, SVVMOD_TIME_READY);
    EXPECT_TRUE(transitioned);
    EXPECT_EQ(get_cur_mode_state(s), SurvivalModeState::Fight);
}

TEST(SurvivalModeTick, FightToEndOnExpiry) {
    auto s = make_survival_manager();
    change_state_to(s, SurvivalModeState::Fight);
    // Fight state has no timer; emulate setting one.
    s.m_dwStateRemainTime = 1000u;
    survival_tick(s, 1000u);
    EXPECT_EQ(get_cur_mode_state(s), SurvivalModeState::End);
}

TEST(SurvivalModeTick, EndToNoneOnExpiry) {
    auto s = make_survival_manager();
    change_state_to(s, SurvivalModeState::End);
    survival_tick(s, SVVMOD_TIME_END);
    EXPECT_EQ(get_cur_mode_state(s), SurvivalModeState::None);
}

TEST(SurvivalModeCheckRemain, TrueWhenActive) {
    auto s = make_survival_manager();
    change_state_to(s, SurvivalModeState::Ready);
    EXPECT_TRUE(check_remain_time(s));
}

TEST(SurvivalModeCheckRemain, FalseWhenInactive) {
    auto s = make_survival_manager();
    EXPECT_FALSE(check_remain_time(s));
}

// ---- User roster ----

TEST(SurvivalModeUsers, AddDetects) {
    auto s = make_survival_manager();
    add_sv_mode_user(s, 100u);
    EXPECT_TRUE(is_sv_mode_user(s, 100u));
    EXPECT_EQ(sv_mode_user_count(s), 1u);
}

TEST(SurvivalModeUsers, RemoveCleans) {
    auto s = make_survival_manager();
    add_sv_mode_user(s, 100u);
    remove_sv_mode_user(s, 100u);
    EXPECT_FALSE(is_sv_mode_user(s, 100u));
    EXPECT_EQ(sv_mode_user_count(s), 0u);
}

TEST(SurvivalModeUsers, RemoveCleansAliveList) {
    auto s = make_survival_manager();
    add_sv_mode_user(s, 100u);
    add_alive_user(s, 100u);
    remove_sv_mode_user(s, 100u);
    EXPECT_FALSE(is_sv_mode_user(s, 100u));
    EXPECT_EQ(get_alive_user_count(s), 0);
}

// ---- Alive count ----

TEST(SurvivalModeAlive, AddOnlyForRegistered) {
    auto s = make_survival_manager();
    add_alive_user(s, 100u);
    EXPECT_EQ(get_alive_user_count(s), 0);
    add_sv_mode_user(s, 100u);
    add_alive_user(s, 100u);
    EXPECT_EQ(get_alive_user_count(s), 1);
}

TEST(SurvivalModeAlive, IdempotentAdd) {
    auto s = make_survival_manager();
    add_sv_mode_user(s, 100u);
    add_alive_user(s, 100u);
    add_alive_user(s, 100u);
    EXPECT_EQ(get_alive_user_count(s), 1);
}

TEST(SurvivalModeAlive, RemoveDecrements) {
    auto s = make_survival_manager();
    add_sv_mode_user(s, 100u);
    add_alive_user(s, 100u);
    remove_alive_user(s, 100u);
    EXPECT_EQ(get_alive_user_count(s), 0);
}

TEST(SurvivalModeAlive, RemoveUnknownNoEffect) {
    auto s = make_survival_manager();
    remove_alive_user(s, 999u);
    EXPECT_EQ(get_alive_user_count(s), 0);
}

// ---- Item using counter ----

TEST(SurvivalModeItemCount, ZeroLimitAllowsAny) {
    auto s = make_survival_manager();
    set_using_count_limit(s, 0u);
    EXPECT_TRUE(add_item_using_count(s, 100u));
    EXPECT_TRUE(add_item_using_count(s, 100u));
    EXPECT_TRUE(add_item_using_count(s, 100u));
    EXPECT_EQ(get_item_using_count(s, 100u), 3u);
}

TEST(SurvivalModeItemCount, LimitEnforced) {
    auto s = make_survival_manager();
    set_using_count_limit(s, 2u);
    EXPECT_TRUE(add_item_using_count(s, 100u));
    EXPECT_TRUE(add_item_using_count(s, 100u));
    EXPECT_FALSE(add_item_using_count(s, 100u));
    EXPECT_EQ(get_item_using_count(s, 100u), 3u);
}

TEST(SurvivalModeItemCount, PerPlayerIsolated) {
    auto s = make_survival_manager();
    set_using_count_limit(s, 1u);
    EXPECT_TRUE(add_item_using_count(s, 100u));
    EXPECT_TRUE(add_item_using_count(s, 200u));
    EXPECT_FALSE(add_item_using_count(s, 100u));
}

// ---- High-level flow ----

TEST(SurvivalModeFlow, ReadyToSurvivalModeStarts15s) {
    auto s = make_survival_manager();
    ready_to_survival_mode(s);
    EXPECT_EQ(s.m_dwStateRemainTime, SVVMOD_TIME_READY);
    EXPECT_EQ(get_cur_mode_state(s), SurvivalModeState::Ready);
}

TEST(SurvivalModeFlow, ReturnToMapClearsAliveAndSetsNone) {
    auto s = make_survival_manager();
    add_sv_mode_user(s, 100u);
    add_alive_user(s, 100u);
    change_state_to(s, SurvivalModeState::Fight);
    return_to_map(s);
    EXPECT_EQ(get_alive_user_count(s), 0);
    EXPECT_EQ(get_cur_mode_state(s), SurvivalModeState::None);
}
