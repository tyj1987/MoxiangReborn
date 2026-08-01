// exchange_manager_test.cpp - Phase D5 ExchangeManager 1:1 port tests.

#include "mxh/server/exchange_manager.hpp"
#include <gtest/gtest.h>

namespace {
using mxh::server::ExchangeRoom;
using mxh::server::ExchangeData;
using mxh::server::ExchangeError;
using mxh::server::ExchangeState;
using mxh::server::ExchangeItemSlot;
using mxh::server::MAX_EXCHANGEITEM;
using mxh::server::init_exchange_room;
using mxh::server::exit_exchange_room;
using mxh::server::lock_slot;
using mxh::server::is_slot_locked;
using mxh::server::is_all_locked;
using mxh::server::set_exchange_accept;
using mxh::server::is_all_accepted;
using mxh::server::set_exchange_state;
using mxh::server::add_exchange_item;
using mxh::server::del_exchange_item;
using mxh::server::input_money;
using mxh::server::do_exchange;
using mxh::server::cancel_exchange;
using mxh::server::total_item_count;

// Trivial player pointer surrogates (host code would pass CPlayer*).
static int dummy_player_1 = 1;
static int dummy_player_2 = 2;

static ExchangeRoom make_room() {
    ExchangeRoom r;
    init_exchange_room(r, &dummy_player_1, &dummy_player_2);
    return r;
}

static ExchangeItemSlot make_item(std::uint16_t idx = 1u) {
    ExchangeItemSlot s;
    s.dwDBIdx = 1u;
    s.wItemIdx = idx;
    s.wPosition = 0;
    return s;
}
}

// ---- Constants 1:1 ----

TEST(ExchangeManagerConstants, MaxItemsMatchesLegacy) {
    EXPECT_EQ(MAX_EXCHANGEITEM, 10);
}

// ---- Init / Exit ----

TEST(ExchangeManagerInit, InitZeroesSlotData) {
    auto r = make_room();
    EXPECT_EQ(r.m_ExchangeData[0].pPlayer, &dummy_player_1);
    EXPECT_EQ(r.m_ExchangeData[1].pPlayer, &dummy_player_2);
    EXPECT_EQ(r.m_ExchangeData[0].nAddItemNum, 0);
    EXPECT_EQ(r.m_ExchangeData[0].dwMoney, 0u);
    EXPECT_FALSE(r.m_ExchangeData[0].bLock);
    EXPECT_FALSE(r.m_ExchangeData[0].bExchange);
    EXPECT_EQ(r.m_nExchangeState, static_cast<int>(ExchangeState::Waiting));
}

TEST(ExchangeManagerInit, ExitClearsState) {
    auto r = make_room();
    lock_slot(r, 0, true);
    set_exchange_accept(r, 0);
    set_exchange_state(r, ExchangeState::Doing);
    exit_exchange_room(r);
    EXPECT_EQ(r.m_nExchangeState, static_cast<int>(ExchangeState::Waiting));
    EXPECT_FALSE(r.m_ExchangeData[0].bLock);
    EXPECT_FALSE(r.m_ExchangeData[0].bExchange);
    EXPECT_EQ(r.m_ExchangeData[0].dwMoney, 0u);
}

// ---- Lock state ----

TEST(ExchangeManagerLock, LockOnlyAffectsTargetSlot) {
    auto r = make_room();
    lock_slot(r, 0, true);
    EXPECT_TRUE(is_slot_locked(r, 0));
    EXPECT_FALSE(is_slot_locked(r, 1));
    EXPECT_FALSE(is_all_locked(r));
}

TEST(ExchangeManagerLock, LockBothSlotsIsAllLock) {
    auto r = make_room();
    lock_slot(r, 0, true);
    lock_slot(r, 1, true);
    EXPECT_TRUE(is_all_locked(r));
}

TEST(ExchangeManagerLock, UnlockResetsLock) {
    auto r = make_room();
    lock_slot(r, 0, true);
    lock_slot(r, 0, false);
    EXPECT_FALSE(is_slot_locked(r, 0));
}

TEST(ExchangeManagerLock, LockInvalidIndexIsNoOp) {
    auto r = make_room();
    lock_slot(r, -1, true);
    lock_slot(r, 5, true);
    EXPECT_FALSE(is_slot_locked(r, 0));
    EXPECT_FALSE(is_slot_locked(r, 1));
}

// ---- Accept state ----

TEST(ExchangeManagerAccept, AcceptRequiresBoth) {
    auto r = make_room();
    set_exchange_accept(r, 0);
    EXPECT_FALSE(is_all_accepted(r));
    set_exchange_accept(r, 1);
    EXPECT_TRUE(is_all_accepted(r));
}

TEST(ExchangeManagerAccept, AcceptInvalidIndexIsNoOp) {
    auto r = make_room();
    set_exchange_accept(r, -1);
    set_exchange_accept(r, 5);
    EXPECT_FALSE(is_all_accepted(r));
}

// ---- Add/Del items ----

TEST(ExchangeManagerAddItem, AddsInOrder) {
    auto r = make_room();
    EXPECT_TRUE(add_exchange_item(r, 0, make_item(10)));
    EXPECT_TRUE(add_exchange_item(r, 0, make_item(20)));
    EXPECT_EQ(r.m_ExchangeData[0].nAddItemNum, 2);
    EXPECT_EQ(r.m_ExchangeData[0].ItemInfo[0].wItemIdx, 10);
    EXPECT_EQ(r.m_ExchangeData[0].ItemInfo[1].wItemIdx, 20);
}

TEST(ExchangeManagerAddItem, OverflowRejects) {
    auto r = make_room();
    for (int i = 0; i < MAX_EXCHANGEITEM; ++i) {
        EXPECT_TRUE(add_exchange_item(r, 0, make_item(static_cast<std::uint16_t>(i))));
    }
    EXPECT_FALSE(add_exchange_item(r, 0, make_item(99)));
}

TEST(ExchangeManagerAddItem, InvalidIndexReturnsFalse) {
    auto r = make_room();
    EXPECT_FALSE(add_exchange_item(r, -1, make_item()));
    EXPECT_FALSE(add_exchange_item(r, 5, make_item()));
}

TEST(ExchangeManagerDelItem, RemovesAtPosShiftsDown) {
    auto r = make_room();
    add_exchange_item(r, 0, make_item(10));
    add_exchange_item(r, 0, make_item(20));
    add_exchange_item(r, 0, make_item(30));
    EXPECT_TRUE(del_exchange_item(r, 0, 1));
    EXPECT_EQ(r.m_ExchangeData[0].nAddItemNum, 2);
    EXPECT_EQ(r.m_ExchangeData[0].ItemInfo[0].wItemIdx, 10);
    EXPECT_EQ(r.m_ExchangeData[0].ItemInfo[1].wItemIdx, 30);
}

TEST(ExchangeManagerDelItem, OutOfRangeFails) {
    auto r = make_room();
    add_exchange_item(r, 0, make_item(10));
    EXPECT_FALSE(del_exchange_item(r, 0, 5));
    EXPECT_EQ(r.m_ExchangeData[0].nAddItemNum, 1);
}

TEST(ExchangeManagerTotalItems, SumAcrossSlots) {
    auto r = make_room();
    add_exchange_item(r, 0, make_item(1));
    add_exchange_item(r, 1, make_item(2));
    add_exchange_item(r, 1, make_item(3));
    EXPECT_EQ(total_item_count(r), 3);
}

// ---- Money ----

TEST(ExchangeManagerMoney, FirstInputSucceeds) {
    auto r = make_room();
    EXPECT_EQ(input_money(r, 0, 1000u), 1000u);
    EXPECT_EQ(r.m_ExchangeData[0].dwMoney, 1000u);
}

TEST(ExchangeManagerMoney, RepeatedInputReplacesOffer) {
    auto r = make_room();
    EXPECT_EQ(input_money(r, 0, 1000u), 1000u);
    EXPECT_EQ(input_money(r, 0, 500u), 500u);
    EXPECT_EQ(r.m_ExchangeData[0].dwMoney, 500u);
}

TEST(ExchangeManagerMoney, InputClampsToAvailablePlayerMoney) {
    auto r = make_room();
    EXPECT_EQ(input_money(r, 0, 1000u, 600u), 600u);
    EXPECT_EQ(r.m_ExchangeData[0].dwMoney, 600u);
}

TEST(ExchangeManagerMoney, InvalidIndexReturnsZero) {
    auto r = make_room();
    EXPECT_EQ(input_money(r, -1, 100u), 0u);
    EXPECT_EQ(input_money(r, 5, 100u), 0u);
}

// ---- DoExchange ----

TEST(ExchangeManagerDoExchange, RequiresBothLockedAndAccepted) {
    auto r = make_room();
    EXPECT_EQ(do_exchange(r), ExchangeError::Error);

    lock_slot(r, 0, true);
    EXPECT_EQ(do_exchange(r), ExchangeError::Error);

    lock_slot(r, 1, true);
    EXPECT_EQ(do_exchange(r), ExchangeError::Error);

    set_exchange_accept(r, 0);
    EXPECT_EQ(do_exchange(r), ExchangeError::Error);

    set_exchange_accept(r, 1);
    EXPECT_EQ(do_exchange(r), ExchangeError::OK);
}

// ---- Cancel ----

TEST(ExchangeManagerCancel, ResetsLockAndAccept) {
    auto r = make_room();
    lock_slot(r, 0, true);
    lock_slot(r, 1, true);
    set_exchange_accept(r, 0);
    set_exchange_accept(r, 1);
    set_exchange_state(r, ExchangeState::Doing);

    cancel_exchange(r);
    EXPECT_EQ(r.m_nExchangeState, static_cast<int>(ExchangeState::Waiting));
    EXPECT_FALSE(r.m_ExchangeData[0].bLock);
    EXPECT_FALSE(r.m_ExchangeData[1].bLock);
    EXPECT_FALSE(r.m_ExchangeData[0].bExchange);
    EXPECT_FALSE(r.m_ExchangeData[1].bExchange);
}

// ---- ExchangeState enum 1:1 ----

TEST(ExchangeManagerEnum, WaitingValueIsZero) {
    EXPECT_EQ(static_cast<int>(ExchangeState::Waiting), 0);
    EXPECT_EQ(static_cast<int>(ExchangeState::Doing),   1);
}

TEST(ExchangeManagerErrorEnum, OkValueIsZero) {
    EXPECT_EQ(static_cast<int>(ExchangeError::OK),             0);
    EXPECT_EQ(static_cast<int>(ExchangeError::UserCancel),     1);
    EXPECT_EQ(static_cast<int>(ExchangeError::UserLogout),     2);
    EXPECT_EQ(static_cast<int>(ExchangeError::UserDie),        3);
    EXPECT_EQ(static_cast<int>(ExchangeError::Die),            4);
    EXPECT_EQ(static_cast<int>(ExchangeError::NotEnoughMoney), 5);
    EXPECT_EQ(static_cast<int>(ExchangeError::NotEnoughSpace), 6);
    EXPECT_EQ(static_cast<int>(ExchangeError::MaxMoney),       7);
    EXPECT_EQ(static_cast<int>(ExchangeError::NotMatchItem),   8);
    EXPECT_EQ(static_cast<int>(ExchangeError::Error),          9);
}


TEST(ExchangeManagerLayout, ItemSlotMatchesLegacyItemBasePrefixAndSize) {
    EXPECT_EQ(sizeof(ExchangeItemSlot), 24u);
    EXPECT_EQ(offsetof(ExchangeItemSlot, dwDBIdx), 0u);
    EXPECT_EQ(offsetof(ExchangeItemSlot, wItemIdx), 4u);
    EXPECT_EQ(offsetof(ExchangeItemSlot, wPosition), 6u);
    EXPECT_EQ(offsetof(ExchangeItemSlot, dwDurability), 8u);
    EXPECT_EQ(offsetof(ExchangeItemSlot, dwRareIdx), 12u);
    EXPECT_EQ(offsetof(ExchangeItemSlot, wQuickPosition), 16u);
    EXPECT_EQ(offsetof(ExchangeItemSlot, dwItemParam), 20u);
}

TEST(ExchangeManagerLock, UnlockEitherParticipantResetsBothSidesAndAccepts) {
    auto room = make_room();
    lock_slot(room, 0, true);
    lock_slot(room, 1, true);
    set_exchange_accept(room, 0);
    set_exchange_accept(room, 1);
    lock_slot(room, 0, false);
    EXPECT_FALSE(is_all_locked(room));
    EXPECT_FALSE(is_all_accepted(room));
    EXPECT_FALSE(is_slot_locked(room, 0));
    EXPECT_FALSE(is_slot_locked(room, 1));
}

TEST(ExchangeManagerAddItem, LockedParticipantCannotAdd) {
    auto room = make_room();
    lock_slot(room, 0, true);
    EXPECT_FALSE(add_exchange_item(room, 0, make_item()));
    EXPECT_EQ(total_item_count(room), 0);
}

TEST(ExchangeManagerAddItem, QuickPositionItemIsRejected) {
    auto room = make_room();
    auto item = make_item();
    item.wQuickPosition = 1;
    EXPECT_FALSE(add_exchange_item(room, 0, item));
}

TEST(ExchangeManagerDelItem, LockedParticipantCannotDelete) {
    auto room = make_room();
    ASSERT_TRUE(add_exchange_item(room, 0, make_item()));
    lock_slot(room, 0, true);
    EXPECT_FALSE(del_exchange_item(room, 0, 0));
    EXPECT_EQ(total_item_count(room), 1);
}
