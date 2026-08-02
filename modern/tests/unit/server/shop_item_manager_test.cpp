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
#include <vector>

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
using mxh::server::SHOP_ITEM_CHECK_INTERVAL_MS;
using mxh::server::SHOP_ITEM_UPDATE_INTERVAL_MS;
using mxh::server::INCANTATION_MEMORY_MOVE_EXTEND;
using mxh::server::INCANTATION_MEMORY_MOVE_EXTEND7;
using mxh::server::INCANTATION_MEMORY_MOVE2;
using mxh::server::INCANTATION_MEMORY_MOVE_EXTEND30;
using mxh::server::MAX_MOVEDATA_PER_PAGE_MODERN;

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


// ---- Tick / expiry logic (D4.13) ----

TEST(ShopItemManagerConstants, CheckAndUpdateIntervalsMatchLegacy) {
    EXPECT_EQ(SHOP_ITEM_CHECK_INTERVAL_MS, 30000u);
    EXPECT_EQ(SHOP_ITEM_UPDATE_INTERVAL_MS, 600000u);
}

TEST(ShopItemManagerTick, ZeroDeltaIsNoOpAndDoesNotRollover) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    EXPECT_FALSE(m.tick(0));
    EXPECT_EQ(m.check_time(), 0u);
    EXPECT_EQ(m.update_time(), 0u);
    EXPECT_FALSE(m.check_due());
}

TEST(ShopItemManagerTick, IncrementsCheckTimeUntilDue) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    m.tick(10000u);
    EXPECT_EQ(m.check_time(), 10000u);
    EXPECT_EQ(m.update_time(), 10000u);
    EXPECT_FALSE(m.check_due());
    m.tick(20000u);
    EXPECT_EQ(m.check_time(), 30000u);
    EXPECT_TRUE(m.check_due());
}

TEST(ShopItemManagerTick, RolloverHappensPastTenMinutes) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    // Use small ticks so check_time stays under 30000 but update_time crosses 600000.
    // 70 ticks of 9000ms each = 630000ms total, crosses rollover on the 67th tick.
    bool saw_rollover = false;
    for (int i = 0; i < 70 && !saw_rollover; ++i) {
        saw_rollover = m.tick(9000u);
    }
    EXPECT_TRUE(saw_rollover);
    EXPECT_EQ(m.update_time(), 0u);  // rollover resets to 0
}

TEST(ShopItemManagerTick, ClearCheckTimeResetsWindow) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    m.tick(30000u);
    ASSERT_TRUE(m.check_due());
    m.clear_check_time();
    EXPECT_EQ(m.check_time(), 0u);
    EXPECT_FALSE(m.check_due());
}

TEST(ShopItemManagerTick, ClearUpdateTimeIsIdempotent) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    m.tick(1234u);
    m.clear_update_time();
    EXPECT_EQ(m.update_time(), 0u);
    m.clear_update_time();
    EXPECT_EQ(m.update_time(), 0u);
}

TEST(ShopItemManagerExpire, EmptyTableProducesEmptyOutput) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    std::vector<std::uint64_t> out;
    m.collect_expired(1000u, out);
    EXPECT_TRUE(out.empty());
    EXPECT_FALSE(m.check_due());
    EXPECT_EQ(m.tick_and_collect_expired(100u, 1000u, out), 0u);
    EXPECT_TRUE(out.empty());
}

TEST(ShopItemManagerExpire, ItemWithFutureDeadlineIsNotExpired) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    UsingShopItemEntry e{};
    e.ItemIdx = 11;
    e.Data = make_with_time(11, 60000);  // 60s
    e.Data.LastCheckTime = 0;
    ASSERT_TRUE(m.add_using_item(e));

    std::vector<std::uint64_t> out;
    m.collect_expired(30000u, out);  // 30s in: not yet expired
    EXPECT_TRUE(out.empty());
}

TEST(ShopItemManagerExpire, ItemAtExactDeadlineIsExpired) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    UsingShopItemEntry e{};
    e.ItemIdx = 12;
    e.Data = make_with_time(12, 60000);
    e.Data.LastCheckTime = 0;
    ASSERT_TRUE(m.add_using_item(e));

    std::vector<std::uint64_t> out;
    m.collect_expired(60000u, out);  // exactly at deadline
    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0], 12u);
}

TEST(ShopItemManagerExpire, PastDeadlineIsExpired) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    UsingShopItemEntry e{};
    e.ItemIdx = 13;
    e.Data = make_with_time(13, 5000);
    e.Data.LastCheckTime = 1000;
    ASSERT_TRUE(m.add_using_item(e));

    std::vector<std::uint64_t> out;
    m.collect_expired(10000u, out);  // deadline 1000+5000=6000, now 10000 -> expired
    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0], 13u);
}

TEST(ShopItemManagerExpire, MixedItemsReportedTogether) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    UsingShopItemEntry a{};
    a.ItemIdx = 20;
    a.Data = make_with_time(20, 1000);
    a.Data.LastCheckTime = 0;
    UsingShopItemEntry b{};
    b.ItemIdx = 21;
    b.Data = make_with_time(21, 60000);
    b.Data.LastCheckTime = 0;
    UsingShopItemEntry c{};
    c.ItemIdx = 22;
    c.Data = make_with_time(22, 5000);
    c.Data.LastCheckTime = 5000;  // deadline 10000
    ASSERT_TRUE(m.add_using_item(a));
    ASSERT_TRUE(m.add_using_item(b));
    ASSERT_TRUE(m.add_using_item(c));

    std::vector<std::uint64_t> out;
    m.collect_expired(10000u, out);
    // 20 (deadline 1000) expired, 21 (deadline 60000) not, 22 (deadline 10000) expired at exact boundary
    ASSERT_EQ(out.size(), 2u);
    EXPECT_EQ(out[0], 20u);
    EXPECT_EQ(out[1], 22u);
}

TEST(ShopItemManagerExpire, TickAndCollectGatesOnCheckDue) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    UsingShopItemEntry e{};
    e.ItemIdx = 30;
    e.Data = make_with_time(30, 1000);
    e.Data.LastCheckTime = 0;
    ASSERT_TRUE(m.add_using_item(e));

    std::vector<std::uint64_t> out;
    // tick by 10000 (< 30000): check_due false -> 0 returned, out cleared
    EXPECT_EQ(m.tick_and_collect_expired(10000u, 10000u, out), 0u);
    EXPECT_TRUE(out.empty());
    EXPECT_FALSE(m.check_due());

    // tick by another 20000 -> total 30000 -> check_due true -> 1 expired
    EXPECT_EQ(m.tick_and_collect_expired(20000u, 10000u, out), 1u);
    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0], 30u);
}

TEST(ShopItemManagerExpire, TickAndCollectPreservesCallerBufferContentsUntilCleared) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    // collect_expired clears its output; tick_and_collect_expired passes through.
    std::vector<std::uint64_t> out{99u, 100u};
    m.collect_expired(0u, out);
    EXPECT_TRUE(out.empty());  // collect_expired clears
}


// ---- D4.14 UsedShopItem data plane (no ITEMMGR/AbilityManager coupling) ----

mxh::game::ItemBase make_item_base(std::uint16_t icon_idx, std::uint32_t db_idx = 12345) {
    mxh::game::ItemBase ib{};
    ib.dwDBIdx = db_idx;
    ib.wIconIdx = icon_idx;
    ib.Position = 0xFFFF;  // "unplaced" sentinel from legacy
    return ib;
}

TEST(ShopItemManagerUsedShopItem, RejectsZeroIconIdx) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    auto ib = make_item_base(0);
    EXPECT_FALSE(m.used_shop_item(ib, 1, PackedTime{0x11111111u}, 60000u, 1000u));
    EXPECT_EQ(m.using_item_count(), 0u);
}

TEST(ShopItemManagerUsedShopItem, FirstInsertSucceedsAndSetsLastCheckTime) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    auto ib = make_item_base(55134);
    ASSERT_TRUE(m.used_shop_item(ib, 1, PackedTime{0x11111111u}, 60000u, 12345u));
    EXPECT_EQ(m.using_item_count(), 1u);
    EXPECT_TRUE(m.has_using_item_by_icon_idx(55134));
    const auto* found = m.find_using_item_by_icon_idx(55134);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->Data.LastCheckTime, 12345u);
    EXPECT_EQ(found->Data.ShopItem.Remaintime, 60000u);
    EXPECT_EQ(found->Data.ShopItem.Param, 1u);
    EXPECT_EQ(found->Data.ShopItem.ItemBase.wIconIdx, 55134);
}

TEST(ShopItemManagerUsedShopItem, DuplicateIconIdxIsRejected) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    auto ib1 = make_item_base(55134, 111);
    auto ib2 = make_item_base(55134, 222);  // same icon, different db_idx
    ASSERT_TRUE(m.used_shop_item(ib1, 1, PackedTime{0x11111111u}, 60000u, 1000u));
    EXPECT_FALSE(m.used_shop_item(ib2, 2, PackedTime{0x22222222u}, 30000u, 2000u));
    EXPECT_EQ(m.using_item_count(), 1u);
    // Existing row must be untouched (still lastCheckTime=1000, not 2000).
    const auto* found = m.find_using_item_by_icon_idx(55134);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->Data.LastCheckTime, 1000u);
    EXPECT_EQ(found->Data.ShopItem.ItemBase.dwDBIdx, 111u);
}

TEST(ShopItemManagerUsedShopItem, DistinctIconIndicesCoexist) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    auto a = make_item_base(55134);
    auto b = make_item_base(55142);  // different icon, allowed alongside
    ASSERT_TRUE(m.used_shop_item(a, 1, PackedTime{0x11111111u}, 60000u, 1000u));
    ASSERT_TRUE(m.used_shop_item(b, 1, PackedTime{0x11111111u}, 30000u, 1000u));
    EXPECT_EQ(m.using_item_count(), 2u);
    EXPECT_TRUE(m.has_using_item_by_icon_idx(55134));
    EXPECT_TRUE(m.has_using_item_by_icon_idx(55142));
}

TEST(ShopItemManagerUsedShopItem, HasByIconIdxIsFalseForUnknownIcon) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    EXPECT_FALSE(m.has_using_item_by_icon_idx(99999));
    EXPECT_EQ(m.find_using_item_by_icon_idx(99999), nullptr);
}

TEST(ShopItemManagerUsedShopItem, HasByIconIdxTrueAfterInsert) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    auto ib = make_item_base(7777);
    EXPECT_FALSE(m.has_using_item_by_icon_idx(7777));
    ASSERT_TRUE(m.used_shop_item(ib, 2, PackedTime{0x33333333u}, 5000u, 500u));
    EXPECT_TRUE(m.has_using_item_by_icon_idx(7777));
}

TEST(ShopItemManagerUsedShopItem, ActivePredicateBeforeDeadline) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    auto ib = make_item_base(55134);
    ASSERT_TRUE(m.used_shop_item(ib, 1, PackedTime{0x11111111u}, 10000u, 1000u));
    // deadline = LastCheckTime + Remaintime = 1000 + 10000 = 11000
    EXPECT_TRUE(m.is_using_item_active(55134, 5000u));   // well before
    EXPECT_TRUE(m.is_using_item_active(55134, 10999u));  // just before
    EXPECT_FALSE(m.is_using_item_active(55134, 11000u)); // exactly at deadline (strict >)
    EXPECT_FALSE(m.is_using_item_active(55134, 99999u)); // well past
}

TEST(ShopItemManagerUsedShopItem, ActivePredicateFalseForUnknownIcon) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    EXPECT_FALSE(m.is_using_item_active(12345, 0u));
}

TEST(ShopItemManagerUsedShopItem, DeleteThenReinsertReplacesAfterDrop) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    auto ib = make_item_base(55134);
    ASSERT_TRUE(m.used_shop_item(ib, 1, PackedTime{0x11111111u}, 60000u, 1000u));
    EXPECT_TRUE(m.has_using_item_by_icon_idx(55134));
    EXPECT_TRUE(m.delete_using_item(55134));
    EXPECT_FALSE(m.has_using_item_by_icon_idx(55134));
    // After delete, legacy allows re-insert with a new lastCheckTime.
    ASSERT_TRUE(m.used_shop_item(ib, 1, PackedTime{0x22222222u}, 30000u, 5000u));
    EXPECT_EQ(m.using_item_count(), 1u);
    const auto* found = m.find_using_item_by_icon_idx(55134);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->Data.LastCheckTime, 5000u);
}

TEST(ShopItemManagerUsedShopItem, ReleaseClearsUsedShopItems) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    auto a = make_item_base(55134);
    auto b = make_item_base(55142);
    ASSERT_TRUE(m.used_shop_item(a, 1, PackedTime{0u}, 60000u, 100u));
    ASSERT_TRUE(m.used_shop_item(b, 1, PackedTime{0u}, 30000u, 100u));
    EXPECT_EQ(m.using_item_count(), 2u);
    m.release();
    EXPECT_EQ(m.using_item_count(), 0u);
    EXPECT_FALSE(m.has_using_item_by_icon_idx(55134));
    EXPECT_FALSE(m.has_using_item_by_icon_idx(55142));
    EXPECT_FALSE(m.is_using_item_active(55134, 0u));
}

TEST(ShopItemManagerUsedShopItem, LegacyKeyAlwaysEqualsIconIdx) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    auto ib = make_item_base(7777, /*db_idx=*/0xCAFEBABEu);
    ASSERT_TRUE(m.used_shop_item(ib, 1, PackedTime{0x55555555u}, 60000u, 1000u));
    const auto* found = m.find_using_item(7777u);  // legacy key = wIconIdx
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->ItemIdx, 7777u);
    EXPECT_EQ(found->Data.ShopItem.ItemBase.wIconIdx, 7777);
}


// ---- D4.15 SEND_SHOPITEM_USEDINFO wire serialization ----

TEST(ShopItemManagerWire, SendShopItemUsedInfoStructLayout) {
    // 8-byte MsgHeader + 2-byte ItemCount + 100 * 34-byte ShopItemBase = 3410 bytes.
    EXPECT_EQ(sizeof(mxh::game::SendShopItemUsedInfo), 3410u);
    EXPECT_EQ(mxh::game::SEND_SHOPITEM_USEDINFO_MAX_BYTES, 3410u);
}

TEST(ShopItemManagerWire, SizeHelperMatchesLegacyGetSize) {
    EXPECT_EQ(mxh::game::send_shopitem_usedinfo_size(0), 10u);   // 8 + 2 + 0
    EXPECT_EQ(mxh::game::send_shopitem_usedinfo_size(1), 44u);   // 8 + 2 + 34
    EXPECT_EQ(mxh::game::send_shopitem_usedinfo_size(50), 1710u);
    EXPECT_EQ(mxh::game::send_shopitem_usedinfo_size(100), 3410u);
    EXPECT_EQ(mxh::game::send_shopitem_usedinfo_size(101), 0u);  // over legacy max
}

TEST(ShopItemManagerWire, EmptyManagerSerializesToHeaderPlusZeroCount) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    auto bytes = m.serialize_using_items(0x55, 0x66);
    ASSERT_EQ(bytes.size(), 10u);  // 8 (header) + 2 (ItemCount)
    EXPECT_EQ(bytes[2], 0x55);  // category
    EXPECT_EQ(bytes[3], 0x66);  // protocol
    std::uint16_t count = 0;
    std::memcpy(&count, bytes.data() + 8, sizeof(std::uint16_t));
    EXPECT_EQ(count, 0u);
}

TEST(ShopItemManagerWire, SingleItemMatchesLegacyGetSize) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    auto ib = make_item_base(55134);
    ASSERT_TRUE(m.used_shop_item(ib, 1, PackedTime{0x11111111u}, 60000u, 1000u));
    auto bytes = m.serialize_using_items(0x05, 0x07);
    EXPECT_EQ(bytes.size(), 44u);  // 8 + 2 + 1*34
    EXPECT_EQ(bytes[2], 0x05);
    EXPECT_EQ(bytes[3], 0x07);
    std::uint16_t count = 0;
    std::memcpy(&count, bytes.data() + 8, sizeof(std::uint16_t));
    EXPECT_EQ(count, 1u);
    // First Item starts at offset 10.
    mxh::game::ShopItemBase first{};
    std::memcpy(&first, bytes.data() + 10, sizeof(first));
    EXPECT_EQ(first.ItemBase.wIconIdx, 55134);
    EXPECT_EQ(first.Param, 1u);
    EXPECT_EQ(first.Remaintime, 60000u);
}

TEST(ShopItemManagerWire, MultipleItemsAreContiguousAndTrimmed) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    for (uint16_t icon : {uint16_t{55134}, uint16_t{55142}, uint16_t{55200}}) {
        auto ib = make_item_base(icon);
        ASSERT_TRUE(m.used_shop_item(ib, 1, PackedTime{0x11111111u}, 60000u, 1000u));
    }
    auto bytes = m.serialize_using_items(0, 0);
    EXPECT_EQ(bytes.size(), 8u + 2u + 3u * 34u);  // = 112
    std::uint16_t count = 0;
    std::memcpy(&count, bytes.data() + 8, sizeof(std::uint16_t));
    EXPECT_EQ(count, 3u);
}

TEST(ShopItemManagerWire, HeaderlessHelperZerosCategoryAndProtocol) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    auto ib = make_item_base(55134);
    ASSERT_TRUE(m.used_shop_item(ib, 1, PackedTime{0x11111111u}, 60000u, 1000u));
    auto bytes = m.serialize_using_items_headerless();
    EXPECT_EQ(bytes[2], 0);
    EXPECT_EQ(bytes[3], 0);
}

TEST(ShopItemManagerWire, SerializeAfterReleaseIsEmptyFrame) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    auto ib = make_item_base(55134);
    ASSERT_TRUE(m.used_shop_item(ib, 1, PackedTime{0x11111111u}, 60000u, 1000u));
    m.release();
    auto bytes = m.serialize_using_items(0, 0);
    EXPECT_EQ(bytes.size(), 10u);
    std::uint16_t count = 0xFFFFu;
    std::memcpy(&count, bytes.data() + 8, sizeof(std::uint16_t));
    EXPECT_EQ(count, 0u);
}


// ---- D4.16 SEND_SHOPITEM_INFO + SEND_MOVEDATA_INFO wire ----

TEST(ShopItemManagerWire, SendShopItemInfoStructLayout) {
    // 8-byte MsgHeader + 2-byte ItemCount + 150 * 22-byte ItemBase = 3310 bytes.
    EXPECT_EQ(sizeof(mxh::game::SendShopItemInfo), 3310u);
    EXPECT_EQ(mxh::game::SEND_SHOPITEM_INFO_MAX_BYTES, 3310u);
    EXPECT_EQ(mxh::game::SLOT_SHOPITEM_NUM_MODERN, 150u);
}

TEST(ShopItemManagerWire, SendShopItemInfoSizeHelperMatchesLegacyGetSize) {
    EXPECT_EQ(mxh::game::send_shopitem_info_size(0),    10u);    // 8 + 2
    EXPECT_EQ(mxh::game::send_shopitem_info_size(1),    32u);    // 8 + 2 + 22
    EXPECT_EQ(mxh::game::send_shopitem_info_size(150),  3310u);  // full
    EXPECT_EQ(mxh::game::send_shopitem_info_size(151),  0u);     // over legacy max
}

TEST(ShopItemManagerWire, SendMoveDataInfoStructLayout) {
    // 8 MsgHeader + 1 bInited + 2 Count + 20 * 31 MoveData = 631 bytes.
    EXPECT_EQ(sizeof(mxh::game::SendMoveDataInfo), 631u);
    EXPECT_EQ(mxh::game::SEND_MOVEDATA_INFO_MAX_BYTES, 631u);
    EXPECT_EQ(mxh::game::MOVEPOINT_TOTAL_MODERN, 20u);
    EXPECT_EQ(mxh::game::MAX_MOVEDATA_PERPAGE_MODERN, 10u);
    EXPECT_EQ(mxh::game::MAX_MOVEPOINT_PAGE_MODERN, 2u);
}

TEST(ShopItemManagerWire, SendMoveDataInfoSizeHelperMatchesLegacyGetSize) {
    EXPECT_EQ(mxh::game::send_movedata_info_size(0,  false), 11u);  // 8 + 1 + 2
    EXPECT_EQ(mxh::game::send_movedata_info_size(0,  true),  11u);
    EXPECT_EQ(mxh::game::send_movedata_info_size(1,  true),  42u);  // 11 + 31
    EXPECT_EQ(mxh::game::send_movedata_info_size(20, true),  631u); // full
    EXPECT_EQ(mxh::game::send_movedata_info_size(21, true),  0u);   // over legacy max
}

TEST(ShopItemManagerWire, MovePointsEmptySerializesToHeaderPlusZeroCount) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    auto bytes = m.serialize_move_points(0x05, 0x06, true);
    ASSERT_EQ(bytes.size(), 11u);  // 8 + 1 + 2
    EXPECT_EQ(bytes[2], 0x05);   // category
    EXPECT_EQ(bytes[3], 0x06);   // protocol
    EXPECT_EQ(bytes[8], 1u);     // bInited
    std::uint16_t count = 0xFFFFu;
    std::memcpy(&count, bytes.data() + 9, sizeof(std::uint16_t));
    EXPECT_EQ(count, 0u);
}

TEST(ShopItemManagerWire, MovePointsSingleRowMatchesLegacyGetSize) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    MovePointEntry e{};
    e.DBIdx = 42;
    e.Data = make_move_data(42, 12);
    ASSERT_TRUE(m.add_move_point(e));
    auto bytes = m.serialize_move_points(0x05, 0x06, false);
    EXPECT_EQ(bytes.size(), 42u);  // 11 + 1*31
    EXPECT_EQ(bytes[8], 0u);       // bInited=false
    std::uint16_t count = 0;
    std::memcpy(&count, bytes.data() + 9, sizeof(std::uint16_t));
    EXPECT_EQ(count, 1u);
    // First Data entry starts at offset 11.
    std::uint32_t first_db = 0;
    std::memcpy(&first_db, bytes.data() + 11, sizeof(std::uint32_t));
    EXPECT_EQ(first_db, 42u);
}

TEST(ShopItemManagerWire, MovePointsMultipleRowsAreContiguous) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    for (uint32_t db : {1u, 2u, 3u, 4u}) {
        MovePointEntry e{};
        e.DBIdx = db;
        e.Data = make_move_data(db, 1);
        ASSERT_TRUE(m.add_move_point(e));
    }
    auto bytes = m.serialize_move_points(0, 0, true);
    EXPECT_EQ(bytes.size(), 11u + 4u * 31u);  // = 135
    std::uint16_t count = 0;
    std::memcpy(&count, bytes.data() + 9, sizeof(std::uint16_t));
    EXPECT_EQ(count, 4u);
}

TEST(ShopItemManagerWire, MovePointsHeaderlessHelperZerosCategoryAndProtocol) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    MovePointEntry e{};
    e.DBIdx = 1;
    e.Data = make_move_data(1, 1);
    ASSERT_TRUE(m.add_move_point(e));
    auto bytes = m.serialize_move_points_headerless(true);
    EXPECT_EQ(bytes[2], 0);
    EXPECT_EQ(bytes[3], 0);
    EXPECT_EQ(bytes[8], 1u);  // bInited preserved
}

TEST(ShopItemManagerWire, MovePointsSerializeAfterReleaseIsEmptyFrame) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    MovePointEntry e{};
    e.DBIdx = 5;
    e.Data = make_move_data(5, 1);
    ASSERT_TRUE(m.add_move_point(e));
    m.release();
    auto bytes = m.serialize_move_points(0, 0, false);
    EXPECT_EQ(bytes.size(), 11u);
    std::uint16_t count = 0xFFFFu;
    std::memcpy(&count, bytes.data() + 9, sizeof(std::uint16_t));
    EXPECT_EQ(count, 0u);
    EXPECT_EQ(bytes[8], 0u);  // bInited=false after release
}

TEST(ShopItemManagerWire, UsingItemsAndMovePointsSerializersAreIndependent) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    auto ib = make_item_base(55134);
    ASSERT_TRUE(m.used_shop_item(ib, 1, PackedTime{0x11111111u}, 60000u, 1000u));
    MovePointEntry e{};
    e.DBIdx = 7;
    e.Data = make_move_data(7, 1);
    ASSERT_TRUE(m.add_move_point(e));
    auto using_bytes = m.serialize_using_items(0x10, 0x11);
    auto move_bytes = m.serialize_move_points(0x10, 0x12, true);
    EXPECT_EQ(using_bytes.size(), 44u);    // 8 + 2 + 34
    EXPECT_EQ(move_bytes.size(), 42u);     // 8 + 1 + 2 + 31
    EXPECT_EQ(using_bytes[2], 0x10);
    EXPECT_EQ(using_bytes[3], 0x11);
    EXPECT_EQ(move_bytes[2], 0x10);
    EXPECT_EQ(move_bytes[3], 0x12);
}


// ---- D4.17 AddMovePoint ValidCount gate ----

TEST(ShopItemManagerCapacity, ConstantsMatchLegacy) {
    EXPECT_EQ(INCANTATION_MEMORY_MOVE_EXTEND,    55365);
    EXPECT_EQ(INCANTATION_MEMORY_MOVE_EXTEND7,   55390);
    EXPECT_EQ(INCANTATION_MEMORY_MOVE2,          55371);
    EXPECT_EQ(INCANTATION_MEMORY_MOVE_EXTEND30,  58010);
    EXPECT_EQ(MAX_MOVEDATA_PER_PAGE_MODERN,      10u);
    EXPECT_EQ(mxh::game::MAX_MOVEPOINT_PAGE_MODERN, 2u);
}

TEST(ShopItemManagerCapacity, IsMemoryMoveExtendIconRecognisesAllFour) {
    EXPECT_TRUE(ShopItemManager::is_memory_move_extend_icon(55365));
    EXPECT_TRUE(ShopItemManager::is_memory_move_extend_icon(55390));
    EXPECT_TRUE(ShopItemManager::is_memory_move_extend_icon(55371));
    EXPECT_TRUE(ShopItemManager::is_memory_move_extend_icon(58010));
    EXPECT_FALSE(ShopItemManager::is_memory_move_extend_icon(55134));  // not extend
    EXPECT_FALSE(ShopItemManager::is_memory_move_extend_icon(0));
    EXPECT_FALSE(ShopItemManager::is_memory_move_extend_icon(65535));
}

TEST(ShopItemManagerCapacity, DefaultCapacityIsTenWithoutIncantations) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    EXPECT_EQ(m.move_point_capacity(), 10u);
}

TEST(ShopItemManagerCapacity, EachIncantationDoublesCapacityToTwenty) {
    for (uint16_t icon : {uint16_t{55365}, uint16_t{55390}, uint16_t{55371}, uint16_t{58010}}) {
        ShopItemManager m;
        int s = 0;
        m.init(&s);
        auto ib = make_item_base(icon);
        ASSERT_TRUE(m.used_shop_item(ib, 1, PackedTime{0x11111111u}, 60000u, 1000u));
        EXPECT_EQ(m.move_point_capacity(), 20u);
    }
}

TEST(ShopItemManagerCapacity, NonExtendingItemKeepsCapacityTen) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    auto ib = make_item_base(55134);  // not a memory-move extend
    ASSERT_TRUE(m.used_shop_item(ib, 1, PackedTime{0x11111111u}, 60000u, 1000u));
    EXPECT_EQ(m.move_point_capacity(), 10u);
}

TEST(ShopItemManagerCapacity, AddMovePointAcceptsUpToTenByDefault) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    for (uint32_t i = 1; i <= 10; ++i) {
        MovePointEntry e{};
        e.DBIdx = i;
        e.Data = make_move_data(i, 1);
        ASSERT_TRUE(m.add_move_point(e)) << "DBIdx " << i << " should be accepted";
    }
    EXPECT_EQ(m.move_point_count(), 10u);
}

TEST(ShopItemManagerCapacity, AddMovePointRejectsEleventhWithoutIncantation) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    for (uint32_t i = 1; i <= 10; ++i) {
        MovePointEntry e{};
        e.DBIdx = i;
        e.Data = make_move_data(i, 1);
        ASSERT_TRUE(m.add_move_point(e));
    }
    MovePointEntry extra{};
    extra.DBIdx = 11;
    extra.Data = make_move_data(11, 1);
    EXPECT_FALSE(m.add_move_point(extra));
    EXPECT_EQ(m.move_point_count(), 10u);
}

TEST(ShopItemManagerCapacity, AddMovePointAcceptsEleventhWithIncantation) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    auto ib = make_item_base(55365);  // INCANTATION_MEMORY_MOVE_EXTEND
    ASSERT_TRUE(m.used_shop_item(ib, 1, PackedTime{0x11111111u}, 60000u, 1000u));
    EXPECT_EQ(m.move_point_capacity(), 20u);
    for (uint32_t i = 1; i <= 11; ++i) {
        MovePointEntry e{};
        e.DBIdx = i;
        e.Data = make_move_data(i, 1);
        ASSERT_TRUE(m.add_move_point(e)) << "DBIdx " << i;
    }
    EXPECT_EQ(m.move_point_count(), 11u);
}

TEST(ShopItemManagerCapacity, AddMovePointAcceptsUpToTwentyWithIncantation) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    auto ib = make_item_base(58010);  // INCANTATION_MEMORY_MOVE_EXTEND30
    ASSERT_TRUE(m.used_shop_item(ib, 1, PackedTime{0x11111111u}, 60000u, 1000u));
    for (uint32_t i = 1; i <= 20; ++i) {
        MovePointEntry e{};
        e.DBIdx = i;
        e.Data = make_move_data(i, 1);
        ASSERT_TRUE(m.add_move_point(e)) << "DBIdx " << i;
    }
    EXPECT_EQ(m.move_point_count(), 20u);

    MovePointEntry extra{};
    extra.DBIdx = 21;
    extra.Data = make_move_data(21, 1);
    EXPECT_FALSE(m.add_move_point(extra));
    EXPECT_EQ(m.move_point_count(), 20u);
}

TEST(ShopItemManagerCapacity, ReleasingIncantationShrinksCapacityBackToTen) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    auto ib = make_item_base(55365);
    ASSERT_TRUE(m.used_shop_item(ib, 1, PackedTime{0x11111111u}, 60000u, 1000u));
    EXPECT_EQ(m.move_point_capacity(), 20u);
    ASSERT_TRUE(m.delete_using_item(55365));
    EXPECT_EQ(m.move_point_capacity(), 10u);
}

TEST(ShopItemManagerCapacity, InitResetsCapacityGate) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    auto ib = make_item_base(55365);
    ASSERT_TRUE(m.used_shop_item(ib, 1, PackedTime{0x11111111u}, 60000u, 1000u));
    EXPECT_EQ(m.move_point_capacity(), 20u);
    m.init(&s);
    EXPECT_EQ(m.move_point_capacity(), 10u);
}


// ---- D4.18 data-plane query/mutate helpers ----

TEST(ShopItemManagerHelpers, FindUsingItemByIconIdxMutableReturnsNullForUnknown) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    EXPECT_EQ(m.find_using_item_by_icon_idx_mutable(99999), nullptr);
}

TEST(ShopItemManagerHelpers, FindUsingItemByIconIdxMutableAllowsMutation) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    auto ib = make_item_base(55134);
    ASSERT_TRUE(m.used_shop_item(ib, 1, PackedTime{0x11111111u}, 60000u, 1000u));
    auto* entry = m.find_using_item_by_icon_idx_mutable(55134);
    ASSERT_NE(entry, nullptr);
    entry->Data.LastCheckTime = 55555u;  // mutate
    EXPECT_EQ(m.find_using_item_by_icon_idx(55134)->Data.LastCheckTime, 55555u);
}

TEST(ShopItemManagerHelpers, AddShopItemUseRejectsZeroIconIdx) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    mxh::game::ShopItemBase si{};
    si.ItemBase.wIconIdx = 0;
    si.Param = 1;
    si.Remaintime = 60000;
    EXPECT_FALSE(m.add_shop_item_use(si, 1000u));
    EXPECT_EQ(m.using_item_count(), 0u);
}

TEST(ShopItemManagerHelpers, AddShopItemUseRejectsDuplicateIconIdx) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    auto ib = make_item_base(55134);
    ASSERT_TRUE(m.used_shop_item(ib, 1, PackedTime{0x11111111u}, 60000u, 1000u));

    mxh::game::ShopItemBase si{};
    si.ItemBase.wIconIdx = 55134;
    si.ItemBase.dwDBIdx = 999;  // different db_idx
    si.Param = 2;
    si.Remaintime = 30000;
    si.BeginTime = PackedTime{0x22222222u};
    EXPECT_FALSE(m.add_shop_item_use(si, 2000u));
    // Original row preserved
    const auto* found = m.find_using_item_by_icon_idx(55134);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->Data.ShopItem.ItemBase.dwDBIdx, 12345u);  // original db
    EXPECT_EQ(found->Data.LastCheckTime, 1000u);                // original lct
}

TEST(ShopItemManagerHelpers, AddShopItemUsePreservesAllShopItemBaseFields) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    mxh::game::ShopItemBase si{};
    si.ItemBase.dwDBIdx = 0xABCD1234u;
    si.ItemBase.wIconIdx = 55142;
    si.ItemBase.Position = 7;
    si.ItemBase.Durability = 999;
    si.ItemBase.RareIdx = 1;
    si.ItemBase.QuickPosition = 0xFFFF;
    si.ItemBase.ItemParam = 5;
    si.Param = 2;
    si.BeginTime = PackedTime{0xABCDEF01u};
    si.Remaintime = 123456789u;
    ASSERT_TRUE(m.add_shop_item_use(si, 7777u));
    const auto* found = m.find_using_item_by_icon_idx(55142);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->Data.ShopItem.ItemBase.dwDBIdx,       0xABCD1234u);
    EXPECT_EQ(found->Data.ShopItem.ItemBase.wIconIdx,      55142);
    EXPECT_EQ(found->Data.ShopItem.ItemBase.Position,      7);
    EXPECT_EQ(found->Data.ShopItem.ItemBase.Durability,    999);
    EXPECT_EQ(found->Data.ShopItem.ItemBase.RareIdx,       1);
    EXPECT_EQ(found->Data.ShopItem.ItemBase.QuickPosition, 0xFFFF);
    EXPECT_EQ(found->Data.ShopItem.ItemBase.ItemParam,     5);
    EXPECT_EQ(found->Data.ShopItem.Param,                  2);
    EXPECT_EQ(found->Data.ShopItem.BeginTime.value,        0xABCDEF01u);
    EXPECT_EQ(found->Data.ShopItem.Remaintime,             123456789u);
    EXPECT_EQ(found->Data.LastCheckTime,                   7777u);
}

TEST(ShopItemManagerHelpers, RenameMovePointRejectsUnknownDbIdx) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    EXPECT_FALSE(m.rename_move_point(99999, "SungTong"));
}

TEST(ShopItemManagerHelpers, RenameMovePointCopiesAndTerminatesAt20) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    MovePointEntry e{};
    e.DBIdx = 1;
    e.Data = make_move_data(1, 1);
    ASSERT_TRUE(m.add_move_point(e));

    ASSERT_TRUE(m.rename_move_point(1, "ABCDE"));
    const auto* found = m.find_move_point(1);
    ASSERT_NE(found, nullptr);
    EXPECT_STREQ(found->Data.Name.data(), "ABCDE");
}

TEST(ShopItemManagerHelpers, RenameMovePointTruncatesAtMaxNameMinusOne) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    MovePointEntry e{};
    e.DBIdx = 2;
    e.Data = make_move_data(2, 1);
    ASSERT_TRUE(m.add_move_point(e));

    // 50 'X' should truncate to 20 chars + NUL (MAX_SAVED_MOVE_NAME=21)
    std::string long_name(50, 'X');
    ASSERT_TRUE(m.rename_move_point(2, long_name));
    const auto* found = m.find_move_point(2);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(std::strlen(found->Data.Name.data()), 20u);
    EXPECT_EQ(found->Data.Name[20], 0);  // NUL terminator
    for (size_t i = 0; i < 20; ++i) EXPECT_EQ(found->Data.Name[i], 'X');
}

TEST(ShopItemManagerHelpers, RenameMovePointZerosOldContent) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    MovePointEntry e{};
    e.DBIdx = 3;
    e.Data = make_move_data(3, 1);
    ASSERT_TRUE(m.add_move_point(e));
    ASSERT_TRUE(m.rename_move_point(3, "OldLongName"));
    ASSERT_TRUE(m.rename_move_point(3, "X"));
    const auto* found = m.find_move_point(3);
    ASSERT_NE(found, nullptr);
    EXPECT_STREQ(found->Data.Name.data(), "X");
    // Bytes past the new name should be 0 (memset fill before copy)
    EXPECT_EQ(found->Data.Name[2], 0);
    EXPECT_EQ(found->Data.Name[5], 0);
}

TEST(ShopItemManagerHelpers, ReleaseMovePointsClearsOnlyMovePoints) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    auto ib = make_item_base(55134);
    ASSERT_TRUE(m.used_shop_item(ib, 1, PackedTime{0x11111111u}, 60000u, 1000u));
    MovePointEntry e{};
    e.DBIdx = 1;
    e.Data = make_move_data(1, 1);
    ASSERT_TRUE(m.add_move_point(e));
    m.bump_dup_charm();
    ASSERT_EQ(m.using_item_count(), 1u);
    ASSERT_EQ(m.move_point_count(), 1u);
    ASSERT_EQ(m.dup_charm(), 1u);

    m.release_move_points();
    EXPECT_EQ(m.move_point_count(), 0u);
    EXPECT_EQ(m.using_item_count(), 1u);  // untouched
    EXPECT_EQ(m.dup_charm(), 1u);        // untouched
}

TEST(ShopItemManagerHelpers, GetSavedMPNumAliasesMovePointCount) {
    ShopItemManager m;
    int s = 0;
    m.init(&s);
    EXPECT_EQ(m.get_saved_mp_num(), 0u);
    for (uint32_t i = 1; i <= 5; ++i) {
        MovePointEntry e{};
        e.DBIdx = i;
        e.Data = make_move_data(i, 1);
        ASSERT_TRUE(m.add_move_point(e));
    }
    EXPECT_EQ(m.get_saved_mp_num(), 5u);
    EXPECT_EQ(m.get_saved_mp_num(), m.move_point_count());
}

TEST(ShopItemManagerConstants, DupNoneIsZero) {
    EXPECT_EQ(SHOP_ITEM_DUP_NONE, 0u);
}
