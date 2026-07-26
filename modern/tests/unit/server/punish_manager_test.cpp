// punish_manager_test.cpp - Phase 6.3 PunishManager 1:1 port tests.

#include "mxh/server/punish_manager.hpp"

#include <gtest/gtest.h>

namespace {

using mxh::server::PunishKind;
using mxh::server::PunishManager;
using mxh::server::PunishUnit;
using mxh::server::add_punish_unit;
using mxh::server::get_auto_block_time;
using mxh::server::get_auto_note_use_time;
using mxh::server::get_punish_unit;
using mxh::server::make_punish_manager;
using mxh::server::punish_manager_init;
using mxh::server::punish_manager_release;
using mxh::server::punish_total_count;
using mxh::server::punish_unit_get_remain_time;
using mxh::server::punish_unit_init;
using mxh::server::punish_unit_is_time_end;
using mxh::server::remove_punish_unit;
using mxh::server::remove_punish_unit_all;
using mxh::server::set_auto_block_time;
using mxh::server::set_auto_note_use_time;
using mxh::server::sweep_expired;

} // namespace

// ---- Enums 1:1 ----

TEST(PunishKindEnum, ValuesMatchLegacy) {
    EXPECT_EQ(static_cast<int>(PunishKind::ePunish_Login),      0);
    EXPECT_EQ(static_cast<int>(PunishKind::ePunish_AutoNoteUse), 1);
    EXPECT_EQ(static_cast<int>(PunishKind::ePunish_Chat),       2);
    EXPECT_EQ(static_cast<int>(PunishKind::ePunish_Trade),      3);
    EXPECT_EQ(static_cast<int>(PunishKind::ePunish_Max),        4);
}

// ---- POD 1:1 ----

TEST(PunishPOD, DefaultsAreZero) {
    PunishUnit u;
    EXPECT_EQ(u.dwUserIdx,   0u);
    EXPECT_EQ(u.nPunishKind, 0);
    EXPECT_EQ(u.dwEndTime,   0u);
}

TEST(PunishPOD, InitSetsFields) {
    PunishUnit u;
    punish_unit_init(u, 42u, 2, 1000u);
    EXPECT_EQ(u.dwUserIdx,   42u);
    EXPECT_EQ(u.nPunishKind, 2);
    EXPECT_EQ(u.dwEndTime,   1000u);
}

// ---- Unit time helpers ----

TEST(PunishUnit, IsTimeEndBeforeDeadlineIsFalse) {
    PunishUnit u;
    punish_unit_init(u, 1u, 0, 1000u);
    EXPECT_FALSE(punish_unit_is_time_end(u, 500u));
    EXPECT_FALSE(punish_unit_is_time_end(u, 999u));
}

TEST(PunishUnit, IsTimeEndAfterDeadlineIsTrue) {
    PunishUnit u;
    punish_unit_init(u, 1u, 0, 1000u);
    EXPECT_TRUE(punish_unit_is_time_end(u, 1001u));
}

TEST(PunishUnit, IsTimeEndAtExactDeadlineIsFalse) {
    PunishUnit u;
    punish_unit_init(u, 1u, 0, 1000u);
    EXPECT_FALSE(punish_unit_is_time_end(u, 1000u));
}

TEST(PunishUnit, GetRemainTimeBeforeDeadline) {
    PunishUnit u;
    punish_unit_init(u, 1u, 0, 1000u);
    EXPECT_EQ(punish_unit_get_remain_time(u, 200u), 800u);
}

TEST(PunishUnit, GetRemainTimeAfterDeadlineReturnsZero) {
    PunishUnit u;
    punish_unit_init(u, 1u, 0, 1000u);
    EXPECT_EQ(punish_unit_get_remain_time(u, 2000u), 0u);
}

// ---- Lifecycle ----

TEST(PunishInit, MakeIsEmpty) {
    auto m = make_punish_manager();
    EXPECT_EQ(punish_total_count(m), 0u);
}

TEST(PunishInit, InitSetsDefaultAutoTimes) {
    auto m = make_punish_manager();
    punish_manager_init(m);
    EXPECT_EQ(get_auto_note_use_time(m), 60u);
    EXPECT_EQ(get_auto_block_time(m),   60u);
}

TEST(PunishRelease, ReleaseClearsEverything) {
    auto m = make_punish_manager();
    add_punish_unit(m, 1u, PunishKind::ePunish_Chat, 60u, /*now*/ 0u);
    ASSERT_EQ(punish_total_count(m), 1u);
    punish_manager_release(m);
    EXPECT_EQ(punish_total_count(m), 0u);
}

// ---- AddPunishUnit ----

TEST(PunishAdd, NewUnitStoresEndTimeAsNowPlusSecondsTimes1000) {
    auto m = make_punish_manager();
    add_punish_unit(m, 1u, PunishKind::ePunish_Chat,
                    /*seconds*/ 60u, /*now_ms*/ 1000u);
    const PunishUnit* u = get_punish_unit(m, 1u, PunishKind::ePunish_Chat);
    ASSERT_NE(u, nullptr);
    EXPECT_EQ(u->dwUserIdx,   1u);
    EXPECT_EQ(u->nPunishKind, static_cast<int>(PunishKind::ePunish_Chat));
    EXPECT_EQ(u->dwEndTime,   1000u + 60u * 1000u);
}

TEST(PunishAdd, ExistingUnitOverwritesEndTime) {
    auto m = make_punish_manager();
    add_punish_unit(m, 1u, PunishKind::ePunish_Chat, 60u, /*now*/ 0u);
    add_punish_unit(m, 1u, PunishKind::ePunish_Chat, 120u, /*now*/ 5000u);
    const PunishUnit* u = get_punish_unit(m, 1u, PunishKind::ePunish_Chat);
    ASSERT_NE(u, nullptr);
    EXPECT_EQ(u->dwEndTime, 5000u + 120u * 1000u);
}

TEST(PunishAdd, DifferentKindsAreIsolated) {
    auto m = make_punish_manager();
    add_punish_unit(m, 1u, PunishKind::ePunish_Chat, 60u, 0u);
    add_punish_unit(m, 1u, PunishKind::ePunish_Trade, 60u, 0u);
    EXPECT_NE(get_punish_unit(m, 1u, PunishKind::ePunish_Chat),  nullptr);
    EXPECT_NE(get_punish_unit(m, 1u, PunishKind::ePunish_Trade), nullptr);
}

TEST(PunishAdd, DifferentUsersAreIsolated) {
    auto m = make_punish_manager();
    add_punish_unit(m, 1u, PunishKind::ePunish_Chat, 60u, 0u);
    add_punish_unit(m, 2u, PunishKind::ePunish_Chat, 60u, 0u);
    EXPECT_EQ(punish_total_count(m), 2u);
}

// ---- Remove ----

TEST(PunishRemove, RemoveExistingReturnsTrue) {
    auto m = make_punish_manager();
    add_punish_unit(m, 1u, PunishKind::ePunish_Chat, 60u, 0u);
    EXPECT_TRUE(remove_punish_unit(m, 1u, PunishKind::ePunish_Chat));
    EXPECT_EQ(get_punish_unit(m, 1u, PunishKind::ePunish_Chat), nullptr);
}

TEST(PunishRemove, RemoveMissingReturnsFalse) {
    auto m = make_punish_manager();
    EXPECT_FALSE(remove_punish_unit(m, 999u, PunishKind::ePunish_Chat));
}

TEST(PunishRemove, RemoveAllDropsAcrossKinds) {
    auto m = make_punish_manager();
    add_punish_unit(m, 1u, PunishKind::ePunish_Chat,        60u, 0u);
    add_punish_unit(m, 1u, PunishKind::ePunish_Trade,       60u, 0u);
    add_punish_unit(m, 1u, PunishKind::ePunish_AutoNoteUse, 60u, 0u);
    EXPECT_EQ(punish_total_count(m), 3u);

    remove_punish_unit_all(m, 1u);
    EXPECT_EQ(punish_total_count(m), 0u);
}

// ---- sweep_expired ----

TEST(PunishSweep, ExpiredUnitsRemoved) {
    auto m = make_punish_manager();
    add_punish_unit(m, 1u, PunishKind::ePunish_Chat,  60u, /*now*/ 0u);    // ends 60000
    add_punish_unit(m, 2u, PunishKind::ePunish_Chat,  60u, /*now*/ 0u);    // ends 60000
    add_punish_unit(m, 3u, PunishKind::ePunish_Trade, 60u, /*now*/ 0u);    // ends 60000

    std::size_t removed = sweep_expired(m, /*now*/ 70000u);
    // legacy Process uses `break` after the first removal per kind, so we
    // remove one Chat unit and one Trade unit per pass.
    EXPECT_EQ(removed, 2u);
    EXPECT_EQ(punish_total_count(m), 1u);
}

TEST(PunishSweep, NoneExpiredNoOp) {
    auto m = make_punish_manager();
    add_punish_unit(m, 1u, PunishKind::ePunish_Chat, 60u, 0u);
    EXPECT_EQ(sweep_expired(m, /*now*/ 1000u), 0u);
    EXPECT_EQ(punish_total_count(m), 1u);
}

// ---- AutoNoteUseTime / AutoBlockTime ----

TEST(PunishAutoTime, GetSetRoundTrip) {
    auto m = make_punish_manager();
    // Modern default mirrors legacy Init(): 60 minutes.
    EXPECT_EQ(get_auto_note_use_time(m), 60u);
    set_auto_note_use_time(m, 120u);
    EXPECT_EQ(get_auto_note_use_time(m), 120u);

    EXPECT_EQ(get_auto_block_time(m), 60u);
    set_auto_block_time(m, 180u);
    EXPECT_EQ(get_auto_block_time(m), 180u);
}

TEST(PunishAutoTime, InitResetsTo60) {
    auto m = make_punish_manager();
    set_auto_note_use_time(m, 999u);
    set_auto_block_time(m, 999u);
    punish_manager_init(m);
    EXPECT_EQ(get_auto_note_use_time(m), 60u);
    EXPECT_EQ(get_auto_block_time(m),   60u);
}


