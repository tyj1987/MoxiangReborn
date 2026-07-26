// bobusang_manager_test.cpp - Phase D5 BobusangManager 1:1 port tests.

#include "mxh/server/bobusang_manager.hpp"
#include <gtest/gtest.h>

namespace {
using mxh::server::BobusangManagerState;
using mxh::server::BobusangInfo;
using mxh::server::BobusangTotalInfo;
using mxh::server::DealerItem;
using mxh::server::BOBUSANG_NPCIDX;
using mxh::server::BOBUSANG_wNpcUniqueIdx;
using mxh::server::make_bobusang_manager;
using mxh::server::bobusang_mgr_init;
using mxh::server::bobusang_mgr_release;
using mxh::server::make_new_bobusang_npc;
using mxh::server::remove_bobusang_npc;
using mxh::server::set_bobusang_info;
using mxh::server::is_bobusang_active;
using mxh::server::add_selling_item;
using mxh::server::clear_selling_items;
using mxh::server::get_bobusang_selling_rt;
using mxh::server::get_selling_item;
using mxh::server::add_guest;
using mxh::server::leave_guest;
using mxh::server::get_customer_count;
using mxh::server::buy_item_available;

static BobusangInfo make_info(std::uint32_t map = 7u, std::uint32_t channel = 1u,
                              std::uint32_t start = 100u, std::uint32_t dur = 3600u,
                              std::uint32_t list_idx = 42u) {
    BobusangInfo i;
    i.AppearanceMapNum       = map;
    i.AppearanceChannel      = channel;
    i.AppearanceStartTime    = start;
    i.AppearanceDurationTime = dur;
    i.SellingListIndex       = list_idx;
    return i;
}

static DealerItem make_dealer(std::uint16_t idx, std::uint32_t money, std::uint16_t vol = 1u) {
    DealerItem d;
    d.itemIdx = idx;
    d.money = money;
    d.volume = vol;
    return d;
}
}

// ---- Constants 1:1 ----

TEST(BobusangManagerConstants, NpcIdxMatchesLegacy) {
    EXPECT_EQ(BOBUSANG_NPCIDX, 74u);
}

TEST(BobusangManagerConstants, NpcUniqueIdxMatchesLegacy) {
    EXPECT_EQ(BOBUSANG_wNpcUniqueIdx, 300u);
}

// ---- Init / Release ----

TEST(BobusangManagerInit, InitClearsState) {
    auto s = make_bobusang_manager();
    make_new_bobusang_npc(s, make_info());
    bobusang_mgr_init(s);
    EXPECT_FALSE(is_bobusang_active(s));
}

TEST(BobusangManagerRelease, ReleaseClearsState) {
    auto s = make_bobusang_manager();
    make_new_bobusang_npc(s, make_info());
    bobusang_mgr_release(s);
    EXPECT_FALSE(is_bobusang_active(s));
}

// ---- NPC lifecycle ----

TEST(BobusangManagerMakeNpc, CreatesActiveMerchant) {
    auto s = make_bobusang_manager();
    EXPECT_FALSE(is_bobusang_active(s));
    EXPECT_TRUE(make_new_bobusang_npc(s, make_info()));
    EXPECT_TRUE(is_bobusang_active(s));
    EXPECT_EQ(s.m_pBobusang->AppearanceInfo.AppearanceMapNum, 7u);
}

TEST(BobusangManagerRemoveNpc, RemoveReturnsTrueAndDeactivates) {
    auto s = make_bobusang_manager();
    make_new_bobusang_npc(s, make_info());
    EXPECT_TRUE(remove_bobusang_npc(s));
    EXPECT_FALSE(is_bobusang_active(s));
}

TEST(BobusangManagerRemoveNpc, RemoveOnInactiveReturnsFalse) {
    auto s = make_bobusang_manager();
    EXPECT_FALSE(remove_bobusang_npc(s));
}

TEST(BobusangManagerSetInfo, UpdatesAppearanceInfoInPlace) {
    auto s = make_bobusang_manager();
    make_new_bobusang_npc(s, make_info());
    auto new_info = make_info(8u, 2u);
    EXPECT_TRUE(set_bobusang_info(s, new_info));
    EXPECT_EQ(s.m_pBobusang->AppearanceInfo.AppearanceMapNum, 8u);
    EXPECT_EQ(s.m_pBobusang->AppearanceInfo.AppearanceChannel, 2u);
}

TEST(BobusangManagerSetInfo, OnInactiveReturnsFalse) {
    auto s = make_bobusang_manager();
    EXPECT_FALSE(set_bobusang_info(s, make_info()));
}

// ---- Selling items ----

TEST(BobusangManagerSelling, AddAppends) {
    auto s = make_bobusang_manager();
    make_new_bobusang_npc(s, make_info());
    add_selling_item(s, make_dealer(100u, 500u));
    add_selling_item(s, make_dealer(200u, 800u));
    EXPECT_EQ(s.m_pBobusang->SellingItemList.size(), 2u);
}

TEST(BobusangManagerSelling, ClearRemovesAll) {
    auto s = make_bobusang_manager();
    make_new_bobusang_npc(s, make_info());
    add_selling_item(s, make_dealer(100u, 500u));
    clear_selling_items(s);
    EXPECT_TRUE(s.m_pBobusang->SellingItemList.empty());
}

TEST(BobusangManagerSelling, GetSellingRtClampsAt20) {
    auto s = make_bobusang_manager();
    make_new_bobusang_npc(s, make_info());
    for (int i = 0; i < 25; ++i) add_selling_item(s, make_dealer(static_cast<std::uint16_t>(i), 10u));
    std::array<DealerItem, 30> buf{};
    int n = get_bobusang_selling_rt(s, buf.data(), 30);
    EXPECT_EQ(n, 20);
    EXPECT_EQ(buf[0].itemIdx, 0u);
    EXPECT_EQ(buf[19].itemIdx, 19u);
}

TEST(BobusangManagerSelling, GetSellingRtRespectsAvail) {
    auto s = make_bobusang_manager();
    make_new_bobusang_npc(s, make_info());
    add_selling_item(s, make_dealer(7u, 100u));
    std::array<DealerItem, 10> buf{};
    int n = get_bobusang_selling_rt(s, buf.data(), 10);
    EXPECT_EQ(n, 1);
    EXPECT_EQ(buf[0].itemIdx, 7u);
}

TEST(BobusangManagerSelling, GetSellingRtOnInactiveReturnsZero) {
    auto s = make_bobusang_manager();
    std::array<DealerItem, 10> buf{};
    EXPECT_EQ(get_bobusang_selling_rt(s, buf.data(), 10), 0);
}

TEST(BobusangManagerSelling, GetSellingItemFindsByIdx) {
    auto s = make_bobusang_manager();
    make_new_bobusang_npc(s, make_info());
    add_selling_item(s, make_dealer(100u, 500u));
    add_selling_item(s, make_dealer(200u, 800u));
    auto* it = get_selling_item(s, 200u);
    ASSERT_NE(it, nullptr);
    EXPECT_EQ(it->money, 800u);
}

TEST(BobusangManagerSelling, GetSellingItemMissing) {
    auto s = make_bobusang_manager();
    make_new_bobusang_npc(s, make_info());
    add_selling_item(s, make_dealer(100u, 500u));
    EXPECT_EQ(get_selling_item(s, 999u), nullptr);
}

TEST(BobusangManagerSelling, BuyItemAvailableDetects) {
    auto s = make_bobusang_manager();
    make_new_bobusang_npc(s, make_info());
    add_selling_item(s, make_dealer(100u, 500u));
    EXPECT_TRUE(buy_item_available(s, 100u));
    EXPECT_FALSE(buy_item_available(s, 999u));
}

// ---- Customer list ----

TEST(BobusangManagerCustomer, AddIncreasesCount) {
    auto s = make_bobusang_manager();
    make_new_bobusang_npc(s, make_info());
    add_guest(s, 1u);
    add_guest(s, 2u);
    add_guest(s, 3u);
    EXPECT_EQ(get_customer_count(s), 3);
}

TEST(BobusangManagerCustomer, LeaveRemovesById) {
    auto s = make_bobusang_manager();
    make_new_bobusang_npc(s, make_info());
    add_guest(s, 1u);
    add_guest(s, 2u);
    leave_guest(s, 1u);
    EXPECT_EQ(get_customer_count(s), 1);
    EXPECT_EQ(s.m_pBobusang->pCustomerList[0], 2u);
}

TEST(BobusangManagerCustomer, LeaveUnknownIsNoOp) {
    auto s = make_bobusang_manager();
    make_new_bobusang_npc(s, make_info());
    add_guest(s, 1u);
    leave_guest(s, 999u);
    EXPECT_EQ(get_customer_count(s), 1);
}
