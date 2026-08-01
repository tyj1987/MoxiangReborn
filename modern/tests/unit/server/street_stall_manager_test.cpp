// street_stall_manager_test.cpp - Phase D5 StreetStallManager 1:1 port tests.

#include "mxh/server/street_stall_manager.hpp"
#include <gtest/gtest.h>

namespace {
using mxh::server::StreetStall;
using mxh::server::StreetStallManagerState;
using mxh::server::StallKind;
using mxh::server::StreetStallDelayState;
using mxh::server::MAX_STREETSTALL_CELLNUM;
using mxh::server::MAX_STREETBUYSTALL_CELLNUM;
using mxh::server::STALL_SEARCH_DELAY_TIME;
using mxh::server::ITEM_VIEW_DELAY_TIME;
using mxh::server::make_street_stall_manager;
using mxh::server::init_street_stall;
using mxh::server::fill_cell;
using mxh::server::empty_cell;
using mxh::server::empty_cell_all;
using mxh::server::change_cell_state;
using mxh::server::set_cell_money;
using mxh::server::set_cell_volume;
using mxh::server::get_stall_kind;
using mxh::server::set_stall_kind;
using mxh::server::add_guest;
using mxh::server::delete_guest;
using mxh::server::delete_guest_all;
using mxh::server::is_stall_full;
using mxh::server::get_cur_regist_item_num;
using mxh::server::get_total_money;
using mxh::server::register_stall;
using mxh::server::find_stall_by_owner;
using mxh::server::delete_stall_by_owner;
using mxh::server::set_search_delay;
using mxh::server::check_delay_elapsed;
}

// ---- Constants 1:1 ----

TEST(StreetStallManagerConstants, MaxStallCellNumMatchesLegacy) {
    EXPECT_EQ(MAX_STREETSTALL_CELLNUM, 25);
}

TEST(StreetStallManagerConstants, MaxBuyStallCellNumMatchesLegacy) {
    EXPECT_EQ(MAX_STREETBUYSTALL_CELLNUM, 5);
}

TEST(StreetStallManagerConstants, SearchDelayTimeMatchesLegacy) {
    EXPECT_EQ(STALL_SEARCH_DELAY_TIME, 3000u);
}

TEST(StreetStallManagerConstants, ItemViewDelayTimeMatchesLegacy) {
    EXPECT_EQ(ITEM_VIEW_DELAY_TIME, 1000u);
}

// ---- Init ----

TEST(StreetStallInit, DefaultClearsAll) {
    StreetStall s;
    init_street_stall(s, /*owner_id*/ 7u);
    EXPECT_TRUE(s.m_pOwner.has_value());
    EXPECT_EQ(*s.m_pOwner, 7u);
    EXPECT_EQ(s.m_nCurRegistItemNum, 0);
    EXPECT_EQ(s.m_nTotalMoney, 0u);
    EXPECT_EQ(s.m_wStallKind, static_cast<std::uint16_t>(StallKind::Null));
    EXPECT_TRUE(s.m_GuestList.empty());
    for (const auto& c : s.m_sArticles) {
        EXPECT_FALSE(c.bFill);
        EXPECT_EQ(c.dwMoney, 0u);
    }
}

// ---- Fill / Empty cells ----

TEST(StreetStallFillCell, AddsAtCurrentSlot) {
    StreetStall s;
    init_street_stall(s, 1u);
    set_stall_kind(s, StallKind::Sell);
    EXPECT_TRUE(fill_cell(s, /*money*/100u, /*vol*/1u));
    EXPECT_EQ(s.m_nCurRegistItemNum, 1);
    EXPECT_EQ(s.m_nTotalMoney, 0u);
    EXPECT_TRUE(s.m_sArticles[0].bFill);
    EXPECT_EQ(s.m_sArticles[0].dwMoney, 100u);
}

TEST(StreetStallFillCell, BuyStallAddsAtAbsolutePosition) {
    StreetStall s;
    init_street_stall(s, 1u);
    set_stall_kind(s, StallKind::Buy);
    EXPECT_TRUE(fill_cell(s, 50u, 2u, false, 4));
    EXPECT_TRUE(s.m_sArticles[4].bFill);
    EXPECT_EQ(s.m_sArticles[4].dwMoney, 50u);
    EXPECT_EQ(s.m_nCurRegistItemNum, 1);
    EXPECT_EQ(s.m_nTotalMoney, 100u);
}

TEST(StreetStallFillCell, OverflowRejects) {
    StreetStall s;
    init_street_stall(s, 1u);
    set_stall_kind(s, StallKind::Sell);
    for (int i = 0; i < MAX_STREETSTALL_CELLNUM; ++i) {
        EXPECT_TRUE(fill_cell(s, 10u, 1u));
    }
    EXPECT_TRUE(is_stall_full(s));
    EXPECT_FALSE(fill_cell(s, 10u, 1u));
}

TEST(StreetStallFillCell, SellStallIgnoresAbsolutePosition) {
    StreetStall s;
    init_street_stall(s, 1u);
    set_stall_kind(s, StallKind::Sell);
    EXPECT_TRUE(fill_cell(s, 10u, 1u, false, MAX_STREETSTALL_CELLNUM));
    EXPECT_TRUE(s.m_sArticles[0].bFill);
}

TEST(StreetStallEmptyCell, RemovesCellAndUpdatesTotals) {
    StreetStall s;
    init_street_stall(s, 1u);
    set_stall_kind(s, StallKind::Sell);
    fill_cell(s, 100u, 1u);
    fill_cell(s, 200u, 1u);
    EXPECT_TRUE(empty_cell(s, /*pos*/0));
    EXPECT_FALSE(s.m_sArticles[0].bFill);
    EXPECT_EQ(s.m_nCurRegistItemNum, 1);
    EXPECT_EQ(s.m_nTotalMoney, 0u);
}

TEST(StreetStallEmptyCell, EmptyRejectsOnUnfilled) {
    StreetStall s;
    init_street_stall(s, 1u);
    set_stall_kind(s, StallKind::Sell);
    EXPECT_FALSE(empty_cell(s, 0));
}

TEST(StreetStallEmptyCell, OutOfRangeRejects) {
    StreetStall s;
    init_street_stall(s, 1u);
    set_stall_kind(s, StallKind::Sell);
    EXPECT_FALSE(empty_cell(s, MAX_STREETSTALL_CELLNUM));
}

TEST(StreetStallEmptyCell, EmptyCellAllResets) {
    StreetStall s;
    init_street_stall(s, 1u);
    set_stall_kind(s, StallKind::Sell);
    fill_cell(s, 100u, 1u);
    fill_cell(s, 200u, 1u);
    empty_cell_all(s);
    EXPECT_EQ(s.m_nCurRegistItemNum, 0);
    EXPECT_EQ(s.m_nTotalMoney, 0u);
    for (const auto& c : s.m_sArticles) EXPECT_FALSE(c.bFill);
}

// ---- Cell state setters ----

TEST(StreetStallCellState, ChangeLockTogglesFlag) {
    StreetStall s;
    init_street_stall(s, 1u);
    set_stall_kind(s, StallKind::Sell);
    fill_cell(s, 100u, 1u);
    change_cell_state(s, 0, true);
    EXPECT_TRUE(s.m_sArticles[0].bLock);
    change_cell_state(s, 0, false);
    EXPECT_FALSE(s.m_sArticles[0].bLock);
}

TEST(StreetStallCellState, ChangeLockOutOfRangeIsNoOp) {
    StreetStall s;
    init_street_stall(s, 1u);
    set_stall_kind(s, StallKind::Sell);
    change_cell_state(s, MAX_STREETSTALL_CELLNUM, true);
}

TEST(StreetStallCellMoney, SetMoneyUpdatesBuyTotalWhenFilled) {
    StreetStall s;
    init_street_stall(s, 1u);
    set_stall_kind(s, StallKind::Buy);
    fill_cell(s, 100u, 1u);
    set_cell_money(s, 0, 300u);
    EXPECT_EQ(s.m_sArticles[0].dwMoney, 300u);
    EXPECT_EQ(s.m_nTotalMoney, 300u);
}

TEST(StreetStallCellMoney, SetMoneyOnUnfilledDoesNotChangeTotal) {
    StreetStall s;
    init_street_stall(s, 1u);
    set_stall_kind(s, StallKind::Sell);
    set_cell_money(s, 0, 300u);
    EXPECT_EQ(s.m_sArticles[0].dwMoney, 0u);
    EXPECT_EQ(s.m_nTotalMoney, 0u);
}

TEST(StreetStallCellVolume, SetVolumeUpdatesCell) {
    StreetStall s;
    init_street_stall(s, 1u);
    set_stall_kind(s, StallKind::Sell);
    fill_cell(s, 100u, 1u);
    set_cell_volume(s, 0, 99u);
    EXPECT_EQ(s.m_sArticles[0].wVolume, 99u);
}

// ---- Stall kind ----

TEST(StreetStallKind, SetAndGet) {
    StreetStall s;
    init_street_stall(s, 1u);
    set_stall_kind(s, StallKind::Sell);
    set_stall_kind(s, StallKind::Sell);
    EXPECT_EQ(get_stall_kind(s), StallKind::Sell);
    set_stall_kind(s, StallKind::Buy);
    EXPECT_EQ(get_stall_kind(s), StallKind::Buy);
}

TEST(StreetStallKind, EnumValues) {
    EXPECT_EQ(static_cast<std::uint16_t>(StallKind::Null), 0);
    EXPECT_EQ(static_cast<std::uint16_t>(StallKind::Sell), 1);
    EXPECT_EQ(static_cast<std::uint16_t>(StallKind::Buy),  2);
}

// ---- Guests ----

TEST(StreetStallGuest, AddAppendsUnique) {
    StreetStall s;
    init_street_stall(s, 1u);
    set_stall_kind(s, StallKind::Sell);
    add_guest(s, 100u);
    add_guest(s, 200u);
    EXPECT_EQ(s.m_GuestList.size(), 2u);
}

TEST(StreetStallGuest, DeleteRemovesById) {
    StreetStall s;
    init_street_stall(s, 1u);
    set_stall_kind(s, StallKind::Sell);
    add_guest(s, 100u);
    add_guest(s, 200u);
    delete_guest(s, 100u);
    EXPECT_EQ(s.m_GuestList.size(), 1u);
    EXPECT_EQ(s.m_GuestList[0], 200u);
}

TEST(StreetStallGuest, DeleteUnknownIsNoOp) {
    StreetStall s;
    init_street_stall(s, 1u);
    set_stall_kind(s, StallKind::Sell);
    add_guest(s, 100u);
    delete_guest(s, 999u);
    EXPECT_EQ(s.m_GuestList.size(), 1u);
}

TEST(StreetStallGuest, DeleteAllClears) {
    StreetStall s;
    init_street_stall(s, 1u);
    set_stall_kind(s, StallKind::Sell);
    add_guest(s, 100u);
    add_guest(s, 200u);
    delete_guest_all(s);
    EXPECT_TRUE(s.m_GuestList.empty());
}

// ---- Manager ----

TEST(StreetStallManagerRegister, PushesAndReturnsIndex) {
    auto m = make_street_stall_manager();
    StreetStall s;
    init_street_stall(s, 100u);
    auto idx = register_stall(m, s);
    EXPECT_EQ(idx, 0u);
    EXPECT_EQ(m.m_StallTable.size(), 1u);
}

TEST(StreetStallManagerFind, FindsByOwner) {
    auto m = make_street_stall_manager();
    StreetStall s1; init_street_stall(s1, 100u);
    StreetStall s2; init_street_stall(s2, 200u);
    register_stall(m, s1);
    register_stall(m, s2);
    auto* found = find_stall_by_owner(m, 200u);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(*found->m_pOwner, 200u);
}

TEST(StreetStallManagerFind, MissingReturnsNull) {
    auto m = make_street_stall_manager();
    StreetStall s1; init_street_stall(s1, 100u);
    register_stall(m, s1);
    EXPECT_EQ(find_stall_by_owner(m, 999u), nullptr);
}

TEST(StreetStallManagerDelete, RemovesByOwner) {
    auto m = make_street_stall_manager();
    StreetStall s1; init_street_stall(s1, 100u);
    StreetStall s2; init_street_stall(s2, 200u);
    register_stall(m, s1);
    register_stall(m, s2);
    EXPECT_TRUE(delete_stall_by_owner(m, 100u));
    EXPECT_EQ(m.m_StallTable.size(), 1u);
    EXPECT_EQ(*m.m_StallTable[0].m_pOwner, 200u);
}

// ---- Delay state ----

TEST(StreetStallDelay, UnsetDelayMeansAvailable) {
    auto m = make_street_stall_manager();
    EXPECT_TRUE(check_delay_elapsed(m, StreetStallDelayState::DelayStallSearch, 1000u, STALL_SEARCH_DELAY_TIME));
}

TEST(StreetStallDelay, WithinWindowReturnsFalse) {
    auto m = make_street_stall_manager();
    set_search_delay(m, StreetStallDelayState::DelayStallSearch, 1000u);
    EXPECT_FALSE(check_delay_elapsed(m, StreetStallDelayState::DelayStallSearch, 1000u + STALL_SEARCH_DELAY_TIME - 1u, STALL_SEARCH_DELAY_TIME));
}

TEST(StreetStallDelay, AfterWindowReturnsTrue) {
    auto m = make_street_stall_manager();
    set_search_delay(m, StreetStallDelayState::DelayStallSearch, 1000u);
    EXPECT_TRUE(check_delay_elapsed(m, StreetStallDelayState::DelayStallSearch, 1000u + STALL_SEARCH_DELAY_TIME, STALL_SEARCH_DELAY_TIME));
}

// ---- Capacity ----

TEST(StreetStallCapacity, IsFullAtMax) {
    StreetStall s;
    init_street_stall(s, 1u);
    set_stall_kind(s, StallKind::Sell);
    EXPECT_FALSE(is_stall_full(s));
    for (int i = 0; i < MAX_STREETSTALL_CELLNUM; ++i) fill_cell(s, 1u, 1u);
    EXPECT_TRUE(is_stall_full(s));
    EXPECT_EQ(get_cur_regist_item_num(s), MAX_STREETSTALL_CELLNUM);
}


TEST(StreetStallLayout, CellMatchesLegacyFortyByteLayout) {
    EXPECT_EQ(sizeof(mxh::server::StallItemBase), 24u);
    EXPECT_EQ(sizeof(mxh::server::StallCellInfo), 40u);
    EXPECT_EQ(offsetof(mxh::server::StallCellInfo, sItemBase), 0u);
    EXPECT_EQ(offsetof(mxh::server::StallCellInfo, dwMoney), 24u);
    EXPECT_EQ(offsetof(mxh::server::StallCellInfo, wVolume), 28u);
    EXPECT_EQ(offsetof(mxh::server::StallCellInfo, bLock), 32u);
    EXPECT_EQ(offsetof(mxh::server::StallCellInfo, bFill), 36u);
}

TEST(StreetStallFillCell, RequiresOwnerAndConfiguredKind) {
    StreetStall stall;
    init_street_stall(stall);
    EXPECT_FALSE(fill_cell(stall, 10u, 1u));
    init_street_stall(stall, 1u);
    EXPECT_FALSE(fill_cell(stall, 10u, 1u));
}

TEST(StreetStallFillCell, BuyReplacementKeepsCountAndRecomputesTotal) {
    StreetStall stall;
    init_street_stall(stall, 1u);
    set_stall_kind(stall, StallKind::Buy);
    ASSERT_TRUE(fill_cell(stall, 10u, 2u, false, 0));
    ASSERT_TRUE(fill_cell(stall, 30u, 3u, false, 0));
    EXPECT_EQ(stall.m_nCurRegistItemNum, 1);
    EXPECT_EQ(stall.m_nTotalMoney, 90u);
}

TEST(StreetStallCellMoney, LockedCellRejectsMoneyAndVolumeChanges) {
    StreetStall stall;
    init_street_stall(stall, 1u);
    set_stall_kind(stall, StallKind::Sell);
    ASSERT_TRUE(fill_cell(stall, 10u, 2u, true));
    set_cell_money(stall, 0, 99u);
    set_cell_volume(stall, 0, 9u);
    EXPECT_EQ(stall.m_sArticles[0].dwMoney, 10u);
    EXPECT_EQ(stall.m_sArticles[0].wVolume, 2u);
}

TEST(StreetStallGuest, DuplicateAndZeroGuestsAreIgnored) {
    StreetStall stall;
    init_street_stall(stall, 1u);
    add_guest(stall, 0u);
    add_guest(stall, 7u);
    add_guest(stall, 7u);
    EXPECT_EQ(stall.m_GuestList.size(), 1u);
    EXPECT_EQ(stall.m_GuestList.front(), 7u);
}
