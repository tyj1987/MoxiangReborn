// shop_item_manager_test.cpp - Phase D4 ShopItemManager data-plane tests.
//
// Locks the 1:1 invariants of legacy [Server]Map/ShopItemManager.cpp:
//   - Init zeroes counters/timers and stores the opaque player pointer
//   - Release clears everything including the player pointer
//   - Five dup counters are independent and reset together by clear_dup_counters
//   - add_using_item rejects ItemIdx==0 and duplicate keys
//   - delete_using_item is idempotent (returns false on missing key)
//   - add_move_point rejects DBIdx==0 and duplicate keys
//   - protect_item_idx and check_time/update_time round-trip
//   - Pool sizing constants mirror legacy (50/10 max/increment, table 50/30)

#include "mxh/game/shop_item_types.hpp"
#include "mxh/server/shop_item_manager.hpp"

#include <gtest/gtest.h>

#include <cstring>

namespace {

using mxh::game::ItemBase;
using mxh::game::MoveData;
using mxh::game::PackedTime;
using mxh::game::ShopItemBase;
using mxh::game::ShopItemWithTime;
using mxh::server::MovePointEntry;
using mxh::server::ShopItemManager;
using mxh::server::SHOP_ITEM_DUP_NONE;
using mxh::server::SHOP_ITEM_MANAGER_MOVE_POOL_INCREMENT;
using mxh::server::SHOP_ITEM_MANAGER_MOVE_POOL_MAX;
using mxh::server::SHOP_ITEM_MANAGER_MOVE_TABLE_CAPACITY;
using mxh::server::SHOP_ITEM_MANAGER_USING_POOL_INCREMENT;
using mxh::server::SHOP_ITEM_MANAGER_USING_POOL_MAX;
using mxh::server::SHOP_ITEM_MANAGER_USING_TABLE_CAPACITY;
using mxh::server::UsingShopItemEntry;

ShopItemWithTime make_with_time(std::uint64_t db_idx, std::uint32_t remain_ms) {
    ShopItemWithTime out{};
    out.ShopItem.ItemBase.dwDBIdx = static_cast<std::uint32_t>(db_idx & 0xFFFFFFFFu);
    out.ShopItem.ItemBase.wIconIdx = 0x1234u;
    out.ShopItem.Param = mxh::game::SHOP_ITEM_PARAM_STORED_TIME;
    out.ShopItem.BeginTime = PackedTime{0x12345678u};
    out.ShopItem.Remaintime = remain_ms;
    out.LastCheckTime = 0;
    return out;
}

MoveData make_move_data(std::uint32_t db_idx, std::uint16_t map_num) {
    MoveData d{};
    d.DBIdx = db_idx;
    std::strncpy(d.Name.data(), "SungTong", d.Name.size() - 1);
    d.MapNum = map_num;
    d.Point = 0x00ABCDEFu;
    return d;
}

} // namespace

TEST(ShopItemManagerConstants, PoolSizingMatchesLegacyFiftyTen) {
    EXPECT_EQ(SHOP_ITEM_MANAGER_USING_POOL_MAX, 50u);
    EXPECT_EQ(SHOP_ITEM_MANAGER_USING_POOL_INCREMENT, 10u);
    EXPECT_EQ(SHOP_ITEM_MANAGER_MOVE_POOL_MAX, 50u);
    EXPECT_EQ(SHOP_ITEM_MANAGER_MOVE_POOL_INCREMENT, 10u);
}

TEST(ShopItemManagerConstants, TableCapacitiesMatchLegacy) {
    EXPECT_EQ(SHOP_ITEM_MANAGER_USING_TABLE_CAPACITY, 50u);
    EXPECT_EQ(SHOP_ITEM_MANAGER_MOVE_TABLE_CAPACITY, 30u);
}

TEST(ShopItemManagerStructs, ShopItemBaseIsThirtyFourBytes) {
    EXPECT_EQ(sizeof(ShopItemBase), 34u);
}

TEST(ShopItemManagerStructs, ShopItemWithTimeIsThirtyEightBytes) {
    EXPECT_EQ(sizeof(ShopItemWithTime), 38u);
}

TEST(ShopItemManagerStructs, MoveDataIsThirtyOneBytesUnderPackOne) {
    EXPECT_EQ(sizeof(MoveData), 31u);
}

TEST(ShopItemManagerStructs, PackedTimeBitfieldsMatchLegacyCalendar) {
    // Each field encoded in 6 bits (year/month in 4 bits) so we can verify
    // them independently. value layout: year(4) | month(4) | day(6) | hour(6)
    // | minute(6) | second(6).
    constexpr std::uint32_t year   = 0x5u;   // 5
    constexpr std::uint32_t month  = 0x7u;   // 7
    constexpr std::uint32_t day    = 0x14u;  // 20
    constexpr std::uint32_t hour   = 0x1Au;  // 26
    constexpr std::uint32_t minute = 0x2Du;  // 45
    constexpr std::uint32_t second = 0x3Bu;  // 59
    std::uint32_t v = (year << 28) | (month << 24) | (day << 18) | (hour << 12) | (minute << 6) | second;
    PackedTime t{v};
    EXPECT_EQ(t.year(),   year);
    EXPECT_EQ(t.month(),  month);
    EXPECT_EQ(t.day(),    day);
    EXPECT_EQ(t.hour(),   hour);
    EXPECT_EQ(t.minute(), minute);
    EXPECT_EQ(t.second(), second);
    EXPECT_EQ(t.value,    v);
}

TEST(ShopItemManagerLifecycle, InitStoresPlayerAndZeroesCounters) {
    ShopItemManager m;
    int sentinel = 0;
    m.init(&sentinel);
    EXPECT_EQ(m.player(), &sentinel);
    EXPECT_EQ(m.dup_incantation(), 0u);
    EXPECT_EQ(m.dup_charm(),       0u);
    EXPECT_EQ(m.dup_herb(),        0u);
    EXPECT_EQ(m.dup_sundries(),    0u);
    EXPECT_EQ(m.dup_pet_equip(),   0u);
    EXPECT_EQ(m.protect_item_idx(), 0u);
    EXPECT_EQ(m.check_time(),       0u);
    EXPECT_EQ(m.update_time(),      0u);
    EXPECT_EQ(m.using_item_count(), 0u);
    EXPECT_EQ(m.move_point_count(), 0u);
}

TEST(ShopItemManagerLifecycle, ReleaseNullsPlayerAndClearsState) {
    ShopItemManager m;
    int sentinel = 0;
    m.init(&sentinel);
    UsingShopItemEntry e{};
    e.ItemIdx = 7;
    e.Data = make_with_time(7, 1000);
    ASSERT_TRUE(m.add_using_item(e));
    MovePointEntry mp{};
    mp.DBIdx = 5;
    mp.Data = make_move_data(5, 12);
    ASSERT_TRUE(m.add_move_point(mp));
    ASSERT_EQ(m.using_item_count(), 1u);
    ASSERT_EQ(m.move_point_count(), 1u);

    m.release();
    EXPECT_EQ(m.player(), nullptr);
    EXPECT_EQ(m.using_item_count(), 0u);
    EXPECT_EQ(m.move_point_count(), 0u);
    EXPECT_EQ(m.dup_incantation(), 0u);
}

TEST(ShopItemManagerLifecycle, ReinitAfterReleaseResetsAllCounters) {
    ShopItemManager m;
    int a = 0;
    m.init(&a);
    m.bump_dup_charm();
    m.bump_dup_herb();
    m.set_protect_item_idx(42u);
    m.set_check_time(1234u);
    m.release();

    int b = 0;
    m.init(&b);
    EXPECT_EQ(m.player(), &b);
    EXPECT_EQ(m.dup_charm(),     0u);
    EXPECT_EQ(m.dup_herb(),      0u);
    EXPECT_EQ(m.protect_item_idx(), 0u);
    EXPECT_EQ(m.check_time(),    0u);
}

TEST(ShopItemManagerDupCounters, EachBumpIncrementsOnlyItsOwnCounter) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);

    EXPECT_EQ(m.bump_dup_incantation(), 1u);
    EXPECT_EQ(m.bump_dup_incantation(), 2u);
    EXPECT_EQ(m.bump_dup_charm(),       1u);
    EXPECT_EQ(m.bump_dup_herb(),        1u);
    EXPECT_EQ(m.bump_dup_sundries(),    1u);
    EXPECT_EQ(m.bump_dup_pet_equip(),   1u);

    EXPECT_EQ(m.dup_incantation(), 2u);
    EXPECT_EQ(m.dup_charm(),       1u);
    EXPECT_EQ(m.dup_herb(),        1u);
    EXPECT_EQ(m.dup_sundries(),    1u);
    EXPECT_EQ(m.dup_pet_equip(),   1u);
}

TEST(ShopItemManagerDupCounters, ClearDupCountersResetsAllFive) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    m.bump_dup_incantation();
    m.bump_dup_charm();
    m.bump_dup_herb();
    m.bump_dup_sundries();
    m.bump_dup_pet_equip();
    m.clear_dup_counters();

    EXPECT_EQ(m.dup_incantation(), 0u);
    EXPECT_EQ(m.dup_charm(),       0u);
    EXPECT_EQ(m.dup_herb(),        0u);
    EXPECT_EQ(m.dup_sundries(),    0u);
    EXPECT_EQ(m.dup_pet_equip(),   0u);
    EXPECT_EQ(m.protect_item_idx(), 0u);  // unaffected
    EXPECT_EQ(m.check_time(),       0u);  // unaffected
}

TEST(ShopItemManagerProtect, SetterRoundTripsIndex) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    m.set_protect_item_idx(0xDEADBEEFu);
    EXPECT_EQ(m.protect_item_idx(), 0xDEADBEEFu);
    m.set_protect_item_idx(0);
    EXPECT_EQ(m.protect_item_idx(), 0u);
}

TEST(ShopItemManagerTimers, CheckAndUpdateTimesRoundTripIndependently) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    m.set_check_time(1000u);
    m.set_update_time(2000u);
    EXPECT_EQ(m.check_time(),  1000u);
    EXPECT_EQ(m.update_time(), 2000u);
    m.set_check_time(5000u);
    EXPECT_EQ(m.check_time(),  5000u);
    EXPECT_EQ(m.update_time(), 2000u);
}

TEST(ShopItemManagerUsingTable, AddRejectsZeroItemIdx) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    UsingShopItemEntry e{};
    e.ItemIdx = 0;
    EXPECT_FALSE(m.add_using_item(e));
    EXPECT_EQ(m.using_item_count(), 0u);
}

TEST(ShopItemManagerUsingTable, AddSucceedsAndFindReturnsPointer) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    UsingShopItemEntry e{};
    e.ItemIdx = 11;
    e.Data = make_with_time(11, 60000);
    ASSERT_TRUE(m.add_using_item(e));
    EXPECT_TRUE(m.has_using_item(11));
    const auto* found = m.find_using_item(11);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->Data.ShopItem.Remaintime, 60000u);
}

TEST(ShopItemManagerUsingTable, AddDuplicateKeyIsRejected) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    UsingShopItemEntry a{};
    a.ItemIdx = 99;
    a.Data = make_with_time(99, 1000);
    UsingShopItemEntry b{};
    b.ItemIdx = 99;
    b.Data = make_with_time(99, 2000);
    ASSERT_TRUE(m.add_using_item(a));
    EXPECT_FALSE(m.add_using_item(b));
    EXPECT_EQ(m.using_item_count(), 1u);
}

TEST(ShopItemManagerUsingTable, DeleteRemovesEntryAndReturnsTrue) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    UsingShopItemEntry e{};
    e.ItemIdx = 5;
    e.Data = make_with_time(5, 0);
    ASSERT_TRUE(m.add_using_item(e));
    EXPECT_TRUE(m.delete_using_item(5));
    EXPECT_FALSE(m.has_using_item(5));
    EXPECT_EQ(m.using_item_count(), 0u);
}

TEST(ShopItemManagerUsingTable, DeleteUnknownReturnsFalse) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    EXPECT_FALSE(m.delete_using_item(12345));
}

TEST(ShopItemManagerMoveTable, AddRejectsZeroDbIdx) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    MovePointEntry e{};
    e.DBIdx = 0;
    EXPECT_FALSE(m.add_move_point(e));
    EXPECT_EQ(m.move_point_count(), 0u);
}

TEST(ShopItemManagerMoveTable, AddAndFindRoundTrip) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    MovePointEntry e{};
    e.DBIdx = 7;
    e.Data = make_move_data(7, 42);
    ASSERT_TRUE(m.add_move_point(e));
    const auto* found = m.find_move_point(7);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->Data.MapNum, 42u);
    EXPECT_STREQ(found->Data.Name.data(), "SungTong");
}

TEST(ShopItemManagerMoveTable, AddDuplicateDbIdxIsRejected) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    MovePointEntry a{};
    a.DBIdx = 12;
    a.Data = make_move_data(12, 1);
    MovePointEntry b{};
    b.DBIdx = 12;
    b.Data = make_move_data(12, 2);
    ASSERT_TRUE(m.add_move_point(a));
    EXPECT_FALSE(m.add_move_point(b));
    EXPECT_EQ(m.move_point_count(), 1u);
}

TEST(ShopItemManagerMoveTable, DeleteRemovesEntryAndReturnsTrue) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    MovePointEntry e{};
    e.DBIdx = 3;
    e.Data = make_move_data(3, 1);
    ASSERT_TRUE(m.add_move_point(e));
    EXPECT_TRUE(m.delete_move_point(3));
    EXPECT_EQ(m.find_move_point(3), nullptr);
}

TEST(ShopItemManagerMoveTable, DeleteUnknownReturnsFalse) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    EXPECT_FALSE(m.delete_move_point(0xC0FFEEu));
}

TEST(ShopItemManagerMix, TwoDistinctTablesAreIndependent) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    UsingShopItemEntry u{};
    u.ItemIdx = 10;
    u.Data = make_with_time(10, 1);
    MovePointEntry mp{};
    mp.DBIdx = 10;  // same numeric value as using table, different table
    mp.Data = make_move_data(10, 1);
    ASSERT_TRUE(m.add_using_item(u));
    ASSERT_TRUE(m.add_move_point(mp));
    EXPECT_TRUE(m.has_using_item(10));
    ASSERT_NE(m.find_move_point(10), nullptr);
    EXPECT_EQ(m.using_item_count(), 1u);
    EXPECT_EQ(m.move_point_count(), 1u);
    EXPECT_TRUE(m.delete_using_item(10));
    EXPECT_FALSE(m.has_using_item(10));
    EXPECT_NE(m.find_move_point(10), nullptr);
    EXPECT_EQ(m.move_point_count(), 1u);
}

TEST(ShopItemManagerConstants, DupNoneIsZero) {
    EXPECT_EQ(SHOP_ITEM_DUP_NONE, 0u);
}
