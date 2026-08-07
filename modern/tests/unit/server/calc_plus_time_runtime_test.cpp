// calc_plus_time_runtime_test.cpp
//
// Verifies apply_calc_plus_time() (the runtime orchestrator for the
// legacy CShopItemManager::CalcPlusTime body) wires the per-row data
// plane decisions to side effects on a real ShopItemManager.
//
// Locks the 4 dwType branches across CHARM / non-CHARM item kinds,
// the ItemInfo lookup miss path, and the LastCheckTime mutation that
// the legacy pShopItem->LastCheckTime = gCurTime produces.
//
// Each test populates the using-items table with exactly one row so
// unordered_map iteration order cannot perturb the counters (the
// legacy code is a per-row loop, so per-row tests are 1:1 with the
// legacy semantics).

#include <mxh/server/calc_plus_time.hpp>
#include <mxh/server/calc_plus_time_runtime.hpp>
#include <mxh/server/calc_shop_item_option.hpp>
#include <mxh/server/shop_item_manager.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace {

using mxh::server::CalcPlusTimeEnv;
using mxh::server::CalcPlusTimeType;
using mxh::server::CalcShopItemOptionInfo;
using mxh::server::LEGACY_SHOP_ITEM_CHARM;
using mxh::server::PlustimeCalcCallback;
using mxh::server::PlustimeItemInfoProvider;
using mxh::server::ShopItemManager;
using mxh::server::UsingShopItemEntry;
using mxh::server::apply_calc_plus_time;

class PlustimeEnv final : public CalcPlusTimeEnv {
public:
    bool active_for[1024] = {};
    std::uint32_t now_ms = 0;

    bool event_rate_active(std::uint16_t rate_id) const noexcept override {
        if (rate_id >= 1024) return false;
        return active_for[rate_id];
    }
    std::uint32_t current_time_ms() const noexcept override { return now_ms; }
};

class InfoProvider final : public PlustimeItemInfoProvider {
public:
    std::unordered_map<std::uint16_t, CalcShopItemOptionInfo> infos;

    bool lookup(std::uint16_t icon_idx,
                CalcShopItemOptionInfo& out_info) const override {
        auto it = infos.find(icon_idx);
        if (it == infos.end()) return false;
        out_info = it->second;
        return true;
    }
};

class CalcCallback final : public PlustimeCalcCallback {
public:
    struct Call {
        std::uint32_t item_idx;
        bool b_add;
    };
    std::vector<Call> calls;

    void on_calc(std::uint32_t item_idx, bool b_add) override {
        calls.push_back({item_idx, b_add});
    }
};

UsingShopItemEntry insert_charm_row(ShopItemManager& mgr,
                                    std::uint16_t icon_idx,
                                    std::uint32_t remaintime) {
    UsingShopItemEntry entry{};
    entry.ItemIdx = static_cast<std::uint64_t>(icon_idx);
    entry.Data.ShopItem.ItemBase.wIconIdx = icon_idx;
    entry.Data.ShopItem.Remaintime = remaintime;
    const bool ok = mgr.add_using_item(entry);
    EXPECT_TRUE(ok);
    return entry;
}

CalcShopItemOptionInfo make_charm_info(std::uint16_t mae,
                                       std::uint32_t item_idx = 99999u) {
    CalcShopItemOptionInfo info{};
    info.ItemKind = LEGACY_SHOP_ITEM_CHARM;
    info.ItemIdx = item_idx;
    info.ItemType = 10;
    info.MeleeAttackMin = mae;
    return info;
}

}  // namespace

// ----- DefaultSentinel (legacy case 0) -----

TEST(ApplyCalcPlusTime, DefaultSentinelWithActiveRateCallsCalc) {
    ShopItemManager mgr;
    mgr.init(nullptr);
    insert_charm_row(mgr, /*icon=*/100, /*remain=*/60000);
    PlustimeEnv env;
    env.active_for[10] = true;
    InfoProvider info;
    info.infos[100] = make_charm_info(/*mae=*/10);
    CalcCallback cb;
    auto out = apply_calc_plus_time(
        mgr, /*dw_event_idx=*/10, CalcPlusTimeType::DefaultSentinel,
        env, info, cb);
    EXPECT_EQ(out.rows_visited, 1u);
    EXPECT_EQ(out.rows_skipped_no_item, 0u);
    EXPECT_EQ(out.calc_invocations, 1u);
    EXPECT_EQ(out.last_check_updates, 0u);
    EXPECT_FALSE(out.stopped_early);
    ASSERT_EQ(cb.calls.size(), 1u);
    EXPECT_EQ(cb.calls[0].item_idx, 99999u);
    EXPECT_FALSE(cb.calls[0].b_add);
}

TEST(ApplyCalcPlusTime, DefaultSentinelWithInactiveRateSkipsCalc) {
    ShopItemManager mgr;
    mgr.init(nullptr);
    insert_charm_row(mgr, /*icon=*/100, /*remain=*/60000);
    PlustimeEnv env;
    env.active_for[10] = false;
    InfoProvider info;
    info.infos[100] = make_charm_info(/*mae=*/10);
    CalcCallback cb;
    auto out = apply_calc_plus_time(
        mgr, /*dw_event_idx=*/10, CalcPlusTimeType::DefaultSentinel,
        env, info, cb);
    EXPECT_EQ(out.calc_invocations, 0u);
    EXPECT_EQ(out.last_check_updates, 0u);
    EXPECT_FALSE(out.stopped_early);
    EXPECT_TRUE(cb.calls.empty());
}

// ----- PlustimeOn -----

TEST(ApplyCalcPlusTime, PlustimeOnMatchingEventCallsCalcAndStops) {
    ShopItemManager mgr;
    mgr.init(nullptr);
    insert_charm_row(mgr, /*icon=*/100, /*remain=*/60000);
    PlustimeEnv env;
    InfoProvider info;
    info.infos[100] = make_charm_info(/*mae=*/10);
    CalcCallback cb;
    auto out = apply_calc_plus_time(
        mgr, /*dw_event_idx=*/10, CalcPlusTimeType::PlustimeOn,
        env, info, cb);
    EXPECT_EQ(out.rows_visited, 1u);
    EXPECT_EQ(out.calc_invocations, 1u);
    EXPECT_EQ(out.last_check_updates, 0u);
    EXPECT_TRUE(out.stopped_early);
    ASSERT_EQ(cb.calls.size(), 1u);
    EXPECT_FALSE(cb.calls[0].b_add);
}

TEST(ApplyCalcPlusTime, PlustimeOnNonMatchingEventSkipsCalc) {
    ShopItemManager mgr;
    mgr.init(nullptr);
    insert_charm_row(mgr, /*icon=*/100, /*remain=*/60000);
    PlustimeEnv env;
    InfoProvider info;
    info.infos[100] = make_charm_info(/*mae=*/10);
    CalcCallback cb;
    auto out = apply_calc_plus_time(
        mgr, /*dw_event_idx=*/999, CalcPlusTimeType::PlustimeOn,
        env, info, cb);
    EXPECT_EQ(out.calc_invocations, 0u);
    EXPECT_FALSE(out.stopped_early);
    EXPECT_TRUE(cb.calls.empty());
}

// ----- PlustimeOff -----

TEST(ApplyCalcPlusTime, PlustimeOffMatchingEventUpdatesAndStops) {
    ShopItemManager mgr;
    mgr.init(nullptr);
    insert_charm_row(mgr, /*icon=*/100, /*remain=*/60000);
    PlustimeEnv env;
    env.now_ms = 12345;
    InfoProvider info;
    info.infos[100] = make_charm_info(/*mae=*/10);
    CalcCallback cb;
    auto out = apply_calc_plus_time(
        mgr, /*dw_event_idx=*/10, CalcPlusTimeType::PlustimeOff,
        env, info, cb);
    EXPECT_EQ(out.calc_invocations, 1u);
    EXPECT_EQ(out.last_check_updates, 1u);
    EXPECT_TRUE(out.stopped_early);
    ASSERT_EQ(cb.calls.size(), 1u);
    EXPECT_TRUE(cb.calls[0].b_add);
    const auto* entry = mgr.find_using_item(100);
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->Data.LastCheckTime, 12345u);
}

TEST(ApplyCalcPlusTime, PlustimeOffNonMatchingEventNoUpdateNoCalc) {
    ShopItemManager mgr;
    mgr.init(nullptr);
    insert_charm_row(mgr, /*icon=*/100, /*remain=*/60000);
    PlustimeEnv env;
    env.now_ms = 99999;
    InfoProvider info;
    info.infos[100] = make_charm_info(/*mae=*/10);
    CalcCallback cb;
    auto out = apply_calc_plus_time(
        mgr, /*dw_event_idx=*/999, CalcPlusTimeType::PlustimeOff,
        env, info, cb);
    EXPECT_EQ(out.calc_invocations, 0u);
    EXPECT_EQ(out.last_check_updates, 0u);
    EXPECT_FALSE(out.stopped_early);
    EXPECT_TRUE(cb.calls.empty());
    const auto* entry = mgr.find_using_item(100);
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->Data.LastCheckTime, 0u);
}

// ----- PlustimeAllOff -----

TEST(ApplyCalcPlusTime, PlustimeAllOffWithActiveRateUpdatesButContinues) {
    ShopItemManager mgr;
    mgr.init(nullptr);
    insert_charm_row(mgr, /*icon=*/100, /*remain=*/60000);
    PlustimeEnv env;
    env.now_ms = 7777;
    env.active_for[10] = true;
    InfoProvider info;
    info.infos[100] = make_charm_info(/*mae=*/10);
    CalcCallback cb;
    auto out = apply_calc_plus_time(
        mgr, /*dw_event_idx=*/10, CalcPlusTimeType::PlustimeAllOff,
        env, info, cb);
    EXPECT_EQ(out.calc_invocations, 1u);
    EXPECT_EQ(out.last_check_updates, 1u);
    EXPECT_FALSE(out.stopped_early);
    ASSERT_EQ(cb.calls.size(), 1u);
    EXPECT_TRUE(cb.calls[0].b_add);
    const auto* entry = mgr.find_using_item(100);
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->Data.LastCheckTime, 7777u);
}

TEST(ApplyCalcPlusTime, PlustimeAllOffWithInactiveRateNoUpdateNoCalc) {
    ShopItemManager mgr;
    mgr.init(nullptr);
    insert_charm_row(mgr, /*icon=*/100, /*remain=*/60000);
    PlustimeEnv env;
    env.now_ms = 7777;
    env.active_for[10] = false;
    InfoProvider info;
    info.infos[100] = make_charm_info(/*mae=*/10);
    CalcCallback cb;
    auto out = apply_calc_plus_time(
        mgr, /*dw_event_idx=*/10, CalcPlusTimeType::PlustimeAllOff,
        env, info, cb);
    EXPECT_EQ(out.calc_invocations, 0u);
    EXPECT_EQ(out.last_check_updates, 0u);
    EXPECT_FALSE(out.stopped_early);
    EXPECT_TRUE(cb.calls.empty());
}

// ----- non-CHARM items -----

TEST(ApplyCalcPlusTime, NonCharmItemIsSkipped) {
    ShopItemManager mgr;
    mgr.init(nullptr);
    insert_charm_row(mgr, /*icon=*/100, /*remain=*/60000);
    PlustimeEnv env;
    env.active_for[10] = true;
    InfoProvider info;
    auto info_charm = make_charm_info(/*mae=*/10);
    info_charm.ItemKind = 257;  // LEGACY_SHOP_ITEM_PREMIUM, not CHARM
    info.infos[100] = info_charm;
    CalcCallback cb;
    auto out = apply_calc_plus_time(
        mgr, /*dw_event_idx=*/10, CalcPlusTimeType::DefaultSentinel,
        env, info, cb);
    EXPECT_EQ(out.rows_visited, 1u);
    EXPECT_EQ(out.calc_invocations, 0u);
    EXPECT_EQ(out.last_check_updates, 0u);
    EXPECT_FALSE(out.stopped_early);
    EXPECT_TRUE(cb.calls.empty());
}

// ----- ItemInfo lookup miss -----

TEST(ApplyCalcPlusTime, ItemInfoLookupMissSkipsRow) {
    ShopItemManager mgr;
    mgr.init(nullptr);
    insert_charm_row(mgr, /*icon=*/100, /*remain=*/60000);
    PlustimeEnv env;
    env.active_for[10] = true;
    InfoProvider info;
    CalcCallback cb;
    auto out = apply_calc_plus_time(
        mgr, /*dw_event_idx=*/10, CalcPlusTimeType::DefaultSentinel,
        env, info, cb);
    EXPECT_EQ(out.rows_visited, 1u);
    EXPECT_EQ(out.rows_skipped_no_item, 1u);
    EXPECT_EQ(out.calc_invocations, 0u);
    EXPECT_EQ(out.last_check_updates, 0u);
    EXPECT_FALSE(out.stopped_early);
    EXPECT_TRUE(cb.calls.empty());
}

// ----- edge: empty table -----

TEST(ApplyCalcPlusTime, EmptyTableIsNoOp) {
    ShopItemManager mgr;
    mgr.init(nullptr);
    PlustimeEnv env;
    env.active_for[10] = true;
    InfoProvider info;
    CalcCallback cb;
    auto out = apply_calc_plus_time(
        mgr, /*dw_event_idx=*/10, CalcPlusTimeType::DefaultSentinel,
        env, info, cb);
    EXPECT_EQ(out.rows_visited, 0u);
    EXPECT_EQ(out.calc_invocations, 0u);
    EXPECT_EQ(out.last_check_updates, 0u);
    EXPECT_FALSE(out.stopped_early);
    EXPECT_TRUE(cb.calls.empty());
}

// ----- edge: remaintime == 0 -----

TEST(ApplyCalcPlusTime, ZeroRemaintimeDoesNotCalcForOnOffAllOff) {
    ShopItemManager mgr;
    mgr.init(nullptr);
    insert_charm_row(mgr, /*icon=*/100, /*remain=*/0);
    PlustimeEnv env;
    env.active_for[10] = true;
    env.now_ms = 5000;
    InfoProvider info;
    info.infos[100] = make_charm_info(/*mae=*/10);
    CalcCallback cb;

    auto out_on = apply_calc_plus_time(
        mgr, /*dw_event_idx=*/10, CalcPlusTimeType::PlustimeOn,
        env, info, cb);
    EXPECT_EQ(out_on.calc_invocations, 0u);
    EXPECT_FALSE(out_on.stopped_early);
    EXPECT_TRUE(cb.calls.empty());

    auto out_off = apply_calc_plus_time(
        mgr, /*dw_event_idx=*/10, CalcPlusTimeType::PlustimeOff,
        env, info, cb);
    EXPECT_EQ(out_off.calc_invocations, 0u);
    EXPECT_EQ(out_off.last_check_updates, 0u);
    EXPECT_FALSE(out_off.stopped_early);
    const auto* entry = mgr.find_using_item(100);
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->Data.LastCheckTime, 0u);

    auto out_all = apply_calc_plus_time(
        mgr, /*dw_event_idx=*/10, CalcPlusTimeType::PlustimeAllOff,
        env, info, cb);
    EXPECT_EQ(out_all.calc_invocations, 0u);
    EXPECT_EQ(out_all.last_check_updates, 0u);
    EXPECT_FALSE(out_all.stopped_early);
}

TEST(ApplyCalcPlusTime, PlustimeOnStopsAfterFirstMatch) {
    ShopItemManager mgr;
    mgr.init(nullptr);
    insert_charm_row(mgr, /*icon=*/100, /*remain=*/60000);
    PlustimeEnv env;
    InfoProvider info;
    info.infos[100] = make_charm_info(/*mae=*/10);
    CalcCallback cb;
    auto out = apply_calc_plus_time(
        mgr, /*dw_event_idx=*/10, CalcPlusTimeType::PlustimeOn,
        env, info, cb);
    EXPECT_EQ(out.calc_invocations, 1u);
    EXPECT_TRUE(out.stopped_early);
    EXPECT_EQ(cb.calls.size(), 1u);
    EXPECT_EQ(out.rows_visited, 1u);
}
