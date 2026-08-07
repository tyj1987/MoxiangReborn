// update_logout_to_db_runtime_test.cpp
//
// Verifies apply_update_logout_to_db() (the runtime orchestrator for
// legacy CShopItemManager::UpdateLogoutToDB) wires the per-row data
// plane decisions to side effects on a real ShopItemManager and
// dispatches SQL through a virtual interface.
//
// Locks the three Action paths (Drop / Skip / Persist) plus the
// clamp-to-zero + clamp-to-30s invariants that the legacy code
// applies to Remaintime.

#include <mxh/server/calc_shop_item_option.hpp>
#include <mxh/server/legacy_shop_item_kind.hpp>
#include <mxh/server/shop_item_manager.hpp>
#include <mxh/server/shop_item_update_sql.hpp>
#include <mxh/server/update_logout_to_db.hpp>
#include <mxh/server/update_logout_to_db_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace {

using mxh::server::CalcShopItemOptionEnv;
using mxh::server::ShopItemManager;
using mxh::server::UpdateLogoutItemInfoProvider;
using mxh::server::UpdateLogoutRowDecision;
using mxh::server::UpdateLogoutSqlDispatch;
using mxh::server::UpdateLogoutToDBInfo;
using mxh::server::UsingShopItemEntry;
using mxh::server::apply_update_logout_to_db;
using mxh::server::build_shop_item_update_time_sql;

class TestEnv final : public CalcShopItemOptionEnv {
public:
    bool active_for[1024] = {};
    bool event_rate_active(std::uint16_t rate_id) const noexcept override {
        if (rate_id >= 1024) return false;
        return active_for[rate_id];
    }
};

class InfoProvider final : public UpdateLogoutItemInfoProvider {
public:
    std::unordered_map<std::uint64_t, UpdateLogoutToDBInfo> infos;

    bool lookup(std::uint64_t item_idx,
                UpdateLogoutToDBInfo& out_info) const override {
        auto it = infos.find(item_idx);
        if (it == infos.end()) return false;
        out_info = it->second;
        return true;
    }
};

class SqlSpy final : public UpdateLogoutSqlDispatch {
public:
    struct Call {
        std::uint32_t character_idx;
        std::uint32_t item_idx;
        std::uint32_t remain;
        std::string sql;
    };
    std::vector<Call> calls;

    void dispatch(std::uint32_t character_idx,
                  std::uint32_t item_idx,
                  std::uint32_t remain) override {
        Call c;
        c.character_idx = character_idx;
        c.item_idx = item_idx;
        c.remain = remain;
        c.sql = build_shop_item_update_time_sql(character_idx, item_idx, remain);
        calls.push_back(c);
    }
};

UsingShopItemEntry insert_playtime_row(ShopItemManager& mgr,
                                       std::uint16_t icon_idx,
                                       std::uint32_t remaintime,
                                       std::uint32_t last_check) {
    UsingShopItemEntry entry{};
    entry.ItemIdx = static_cast<std::uint64_t>(icon_idx);
    entry.Data.ShopItem.ItemBase.wIconIdx = icon_idx;
    entry.Data.ShopItem.Param =
        mxh::server::LEGACY_SHOP_ITEM_USE_PARAM_PLAYTIME;
    entry.Data.ShopItem.Remaintime = remaintime;
    entry.Data.LastCheckTime = last_check;
    const bool ok = mgr.add_using_item(entry);
    EXPECT_TRUE(ok);
    return entry;
}

UpdateLogoutToDBInfo playtime_charm_info(std::uint16_t mae) {
    UpdateLogoutToDBInfo info;
    info.sell_price = mxh::server::LEGACY_SHOP_ITEM_USE_PARAM_PLAYTIME;
    info.item_kind = mxh::server::LEGACY_SHOP_ITEM_CHARM;
    info.melee_attack_min = mae;
    return info;
}

UpdateLogoutToDBInfo playtime_realtime_info() {
    UpdateLogoutToDBInfo info;
    info.sell_price = 1;  // STORED_TIME, not PLAYTIME -> Drop
    return info;
}

}  // namespace

// ----- Drop path: SellPrice != PLAYTIME -----

TEST(ApplyUpdateLogoutToDB, NonPlaytimeRowIsDropped) {
    ShopItemManager mgr;
    mgr.init(nullptr);
    insert_playtime_row(mgr, /*icon=*/100, /*remain=*/60000, /*last_check=*/0);
    TestEnv env;
    InfoProvider info;
    info.infos[100] = playtime_realtime_info();
    SqlSpy sql;

    auto out = apply_update_logout_to_db(
        mgr, /*character_idx=*/42, /*g_cur_time=*/10000, info, env, sql);

    EXPECT_EQ(out.rows_visited, 1u);
    EXPECT_EQ(out.rows_dropped, 1u);
    EXPECT_EQ(out.rows_persisted, 0u);
    EXPECT_EQ(out.rows_skipped, 0u);
    EXPECT_TRUE(sql.calls.empty());
    const auto* entry = mgr.find_using_item(100);
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->Data.ShopItem.Remaintime, 60000u);
    EXPECT_EQ(entry->Data.LastCheckTime, 0u);
}

// ----- Drop path: no info -----

TEST(ApplyUpdateLogoutToDB, MissingInfoRowIsDropped) {
    ShopItemManager mgr;
    mgr.init(nullptr);
    insert_playtime_row(mgr, /*icon=*/100, /*remain=*/60000, /*last_check=*/0);
    TestEnv env;
    InfoProvider info;  // empty
    SqlSpy sql;

    auto out = apply_update_logout_to_db(
        mgr, 42, 10000, info, env, sql);

    EXPECT_EQ(out.rows_dropped, 1u);
    EXPECT_TRUE(sql.calls.empty());
}

// ----- Persist path: normal PLAYTIME row, no plustime -----

TEST(ApplyUpdateLogoutToDB, PlaytimeRowDecrementsByChecktimeAndDispatchesSql) {
    ShopItemManager mgr;
    mgr.init(nullptr);
    insert_playtime_row(mgr, /*icon=*/100, /*remain=*/60000, /*last_check=*/5000);
    TestEnv env;
    InfoProvider info;
    info.infos[100] = playtime_charm_info(/*mae=*/0);  // not plustime
    SqlSpy sql;

    auto out = apply_update_logout_to_db(
        mgr, /*character_idx=*/42, /*g_cur_time=*/15000, info, env, sql);

    EXPECT_EQ(out.rows_visited, 1u);
    EXPECT_EQ(out.rows_persisted, 1u);
    EXPECT_EQ(out.remaintime_updates, 1u);
    EXPECT_EQ(out.last_check_updates, 1u);
    const auto* entry = mgr.find_using_item(100);
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->Data.ShopItem.Remaintime, 50000u);  // 60000 - 10000
    EXPECT_EQ(entry->Data.LastCheckTime, 15000u);
    ASSERT_EQ(sql.calls.size(), 1u);
    EXPECT_EQ(sql.calls[0].character_idx, 42u);
    EXPECT_EQ(sql.calls[0].item_idx, 100u);
    EXPECT_EQ(sql.calls[0].remain, 50000u);
    EXPECT_EQ(sql.calls[0].sql,
              "EXEC dbo.MP_SHOPITEM_Updatetime 42, 100, 50000");
}

TEST(ApplyUpdateLogoutToDB, PlaytimeUnderflowClampsToZero) {
    ShopItemManager mgr;
    mgr.init(nullptr);
    insert_playtime_row(mgr, /*icon=*/100, /*remain=*/5000, /*last_check=*/0);
    TestEnv env;
    InfoProvider info;
    info.infos[100] = playtime_charm_info(/*mae=*/0);
    SqlSpy sql;

    auto out = apply_update_logout_to_db(
        mgr, 42, /*g_cur_time=*/10000, info, env, sql);

    EXPECT_EQ(out.rows_persisted, 1u);
    const auto* entry = mgr.find_using_item(100);
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->Data.ShopItem.Remaintime, 0u);
    ASSERT_EQ(sql.calls.size(), 1u);
    EXPECT_EQ(sql.calls[0].remain, 0u);
}

TEST(ApplyUpdateLogoutToDB, PlaytimeClampsChecktimeTo30Seconds) {
    ShopItemManager mgr;
    mgr.init(nullptr);
    // last_check=0, g_cur_time=120000 -> checktime=120000, clamp to 30000
    insert_playtime_row(mgr, /*icon=*/100, /*remain=*/60000, /*last_check=*/0);
    TestEnv env;
    InfoProvider info;
    info.infos[100] = playtime_charm_info(/*mae=*/0);
    SqlSpy sql;

    auto out = apply_update_logout_to_db(
        mgr, 42, /*g_cur_time=*/120000, info, env, sql);

    EXPECT_EQ(out.rows_persisted, 1u);
    const auto* entry = mgr.find_using_item(100);
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->Data.ShopItem.Remaintime, 30000u);  // 60000 - 30000
}

// ----- Plustime path: env.event_rate_active false -> Skip -----

TEST(ApplyUpdateLogoutToDB, PlustimeRowWithInactiveRateSkips) {
    ShopItemManager mgr;
    mgr.init(nullptr);
    insert_playtime_row(mgr, /*icon=*/100, /*remain=*/60000, /*last_check=*/0);
    TestEnv env;
    env.active_for[10] = false;
    InfoProvider info;
    info.infos[100] = playtime_charm_info(/*mae=*/10);  // plustime
    SqlSpy sql;

    auto out = apply_update_logout_to_db(
        mgr, 42, /*g_cur_time=*/10000, info, env, sql);

    EXPECT_EQ(out.rows_skipped, 1u);
    EXPECT_EQ(out.rows_persisted, 0u);
    EXPECT_EQ(out.last_check_updates, 1u);
    EXPECT_EQ(out.remaintime_updates, 0u);
    const auto* entry = mgr.find_using_item(100);
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->Data.ShopItem.Remaintime, 60000u);  // unchanged
    EXPECT_EQ(entry->Data.LastCheckTime, 10000u);  // stamped
    EXPECT_TRUE(sql.calls.empty());
}

// ----- Plustime path: env.event_rate_active true -> Persist (rate file gate passes) -----

TEST(ApplyUpdateLogoutToDB, PlustimeRowWithActiveRatePersists) {
    ShopItemManager mgr;
    mgr.init(nullptr);
    insert_playtime_row(mgr, /*icon=*/100, /*remain=*/60000, /*last_check=*/0);
    TestEnv env;
    env.active_for[10] = true;
    InfoProvider info;
    info.infos[100] = playtime_charm_info(/*mae=*/10);
    SqlSpy sql;

    auto out = apply_update_logout_to_db(
        mgr, 42, /*g_cur_time=*/10000, info, env, sql);

    EXPECT_EQ(out.rows_persisted, 1u);
    EXPECT_EQ(out.rows_skipped, 0u);
    const auto* entry = mgr.find_using_item(100);
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->Data.ShopItem.Remaintime, 50000u);  // 60000 - 10000
    EXPECT_EQ(sql.calls.size(), 1u);
}

// ----- Edge: empty table -----

TEST(ApplyUpdateLogoutToDB, EmptyTableIsNoOp) {
    ShopItemManager mgr;
    mgr.init(nullptr);
    TestEnv env;
    InfoProvider info;
    SqlSpy sql;

    auto out = apply_update_logout_to_db(
        mgr, 42, 10000, info, env, sql);

    EXPECT_EQ(out.rows_visited, 0u);
    EXPECT_TRUE(sql.calls.empty());
}
