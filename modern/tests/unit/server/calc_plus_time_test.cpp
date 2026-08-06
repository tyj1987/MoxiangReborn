// calc_plus_time_test.cpp - 1:1 data-plane tests for the
// legacy CShopItemManager::CalcPlusTime per-row dispatch from
// [Server]Map/ShopItemManager.cpp. Locks the 4 dwType branches
// across CHARM and non-CHARM item kinds, with full coverage of
// the (Remaintime, event_rate_active, dwEventIdx == MeleeAttackMin)
// predicate gates.

#include <mxh/server/calc_plus_time.hpp>

#include <gtest/gtest.h>

#include <cstdint>

using namespace mxh::server;

namespace {

// Test env that exposes event_rate_active and current_time_ms.
// Default: event_rate returns false (legacy: tables match -> gate
// inactive) and current_time returns a fixed sentinel.
class FixedEnv final : public CalcPlusTimeEnv {
public:
    bool active_for[1024] = {};  // indexed by MeleeAttackMin
    std::uint32_t now_ms = 0;

    bool event_rate_active(std::uint16_t rate_id) const noexcept override {
        if (rate_id >= 1024) return false;
        return active_for[rate_id];
    }
    std::uint32_t current_time_ms() const noexcept override { return now_ms; }
};

CalcPlusTimeRowInput make_row(std::uint16_t item_kind,
                              std::uint16_t melee_attack_min,
                              std::uint32_t remaintime) {
    CalcPlusTimeRowInput r;
    r.item_kind = item_kind;
    r.melee_attack_min = melee_attack_min;
    r.remaintime = remaintime;
    r.w_icon_idx = 1000;  // arbitrary
    return r;
}

}  // namespace

// ----- CHARM gate -----

TEST(CalcPlusTime, NonCharmItemsAlwaysSkipped) {
    FixedEnv env;
    auto row = make_row(/*kind=*/257 /*PREMIUM*/, /*mae=*/10, /*rem=*/60000);
    env.active_for[10] = true;
    for (auto t : {CalcPlusTimeType::DefaultSentinel,
                   CalcPlusTimeType::PlustimeOn,
                   CalcPlusTimeType::PlustimeOff,
                   CalcPlusTimeType::PlustimeAllOff}) {
        auto d = calc_plus_time_row_decision(/*dw_event=*/10, t, row, env);
        EXPECT_FALSE(d.should_calc);
        EXPECT_FALSE(d.update_last_check);
        EXPECT_FALSE(d.stop_iteration);
    }
}

// ----- DefaultSentinel (legacy case 0) -----

TEST(CalcPlusTime, DefaultSentinelRequiresRemaintime) {
    FixedEnv env;
    auto row = make_row(LEGACY_SHOP_ITEM_CHARM, /*mae=*/10, /*rem=*/0);
    env.active_for[10] = true;
    auto d = calc_plus_time_row_decision(0, CalcPlusTimeType::DefaultSentinel, row, env);
    EXPECT_FALSE(d.should_calc);
}

TEST(CalcPlusTime, DefaultSentinelRequiresEventRateActive) {
    FixedEnv env;
    auto row = make_row(LEGACY_SHOP_ITEM_CHARM, /*mae=*/10, /*rem=*/60000);
    env.active_for[10] = false;  // gate inactive
    auto d = calc_plus_time_row_decision(0, CalcPlusTimeType::DefaultSentinel, row, env);
    EXPECT_FALSE(d.should_calc);
}

TEST(CalcPlusTime, DefaultSentinelFiresCalcFalse) {
    FixedEnv env;
    auto row = make_row(LEGACY_SHOP_ITEM_CHARM, /*mae=*/10, /*rem=*/60000);
    env.active_for[10] = true;
    auto d = calc_plus_time_row_decision(0, CalcPlusTimeType::DefaultSentinel, row, env);
    EXPECT_TRUE(d.should_calc);
    EXPECT_FALSE(d.b_add);          // legacy FALSE
    EXPECT_FALSE(d.update_last_check); // legacy does NOT update LastCheckTime in case 0
    EXPECT_FALSE(d.stop_iteration);  // legacy does NOT return in case 0
}

TEST(CalcPlusTime, DefaultSentinelIgnoresDwEventIdx) {
    // The case 0 branch doesn't compare dwEventIdx against MeleeAttackMin.
    FixedEnv env;
    auto row = make_row(LEGACY_SHOP_ITEM_CHARM, /*mae=*/10, /*rem=*/60000);
    env.active_for[10] = true;
    auto d999 = calc_plus_time_row_decision(999, CalcPlusTimeType::DefaultSentinel, row, env);
    auto d10 = calc_plus_time_row_decision(10, CalcPlusTimeType::DefaultSentinel, row, env);
    EXPECT_TRUE(d999.should_calc);
    EXPECT_TRUE(d10.should_calc);
}

// ----- PlustimeOn -----

TEST(CalcPlusTime, PlustimeOnRequiresMatchingEventIdx) {
    FixedEnv env;
    auto row = make_row(LEGACY_SHOP_ITEM_CHARM, /*mae=*/10, /*rem=*/60000);
    env.active_for[10] = true;  // not consulted in PlustimeOn
    auto d = calc_plus_time_row_decision(/*dw_event=*/999, CalcPlusTimeType::PlustimeOn, row, env);
    EXPECT_FALSE(d.should_calc);
}

TEST(CalcPlusTime, PlustimeOnRequiresRemaintime) {
    FixedEnv env;
    auto row = make_row(LEGACY_SHOP_ITEM_CHARM, /*mae=*/10, /*rem=*/0);
    auto d = calc_plus_time_row_decision(10, CalcPlusTimeType::PlustimeOn, row, env);
    EXPECT_FALSE(d.should_calc);
}

TEST(CalcPlusTime, PlustimeOnFiresCalcFalseAndStops) {
    FixedEnv env;
    auto row = make_row(LEGACY_SHOP_ITEM_CHARM, /*mae=*/10, /*rem=*/60000);
    auto d = calc_plus_time_row_decision(10, CalcPlusTimeType::PlustimeOn, row, env);
    EXPECT_TRUE(d.should_calc);
    EXPECT_FALSE(d.b_add);
    EXPECT_FALSE(d.update_last_check);  // PlustimeOn does NOT update LastCheckTime
    EXPECT_TRUE(d.stop_iteration);      // legacy return
}

// ----- PlustimeOff -----

TEST(CalcPlusTime, PlustimeOffRequiresMatchingEventIdx) {
    FixedEnv env;
    auto row = make_row(LEGACY_SHOP_ITEM_CHARM, /*mae=*/10, /*rem=*/60000);
    auto d = calc_plus_time_row_decision(999, CalcPlusTimeType::PlustimeOff, row, env);
    EXPECT_FALSE(d.should_calc);
    EXPECT_FALSE(d.update_last_check);
}

TEST(CalcPlusTime, PlustimeOffRequiresRemaintime) {
    FixedEnv env;
    auto row = make_row(LEGACY_SHOP_ITEM_CHARM, /*mae=*/10, /*rem=*/0);
    auto d = calc_plus_time_row_decision(10, CalcPlusTimeType::PlustimeOff, row, env);
    EXPECT_FALSE(d.should_calc);
    EXPECT_FALSE(d.update_last_check);
}

TEST(CalcPlusTime, PlustimeOffFiresCalcTrueAndUpdatesAndStops) {
    FixedEnv env;
    env.now_ms = 12345;
    auto row = make_row(LEGACY_SHOP_ITEM_CHARM, /*mae=*/10, /*rem=*/60000);
    auto d = calc_plus_time_row_decision(10, CalcPlusTimeType::PlustimeOff, row, env);
    EXPECT_TRUE(d.should_calc);
    EXPECT_TRUE(d.b_add);
    EXPECT_TRUE(d.update_last_check);   // legacy: pShopItem->LastCheckTime = gCurTime
    EXPECT_TRUE(d.stop_iteration);      // legacy return
}

// ----- PlustimeAllOff -----

TEST(CalcPlusTime, PlustimeAllOffRequiresRemaintime) {
    FixedEnv env;
    env.active_for[10] = true;
    auto row = make_row(LEGACY_SHOP_ITEM_CHARM, /*mae=*/10, /*rem=*/0);
    auto d = calc_plus_time_row_decision(0, CalcPlusTimeType::PlustimeAllOff, row, env);
    EXPECT_FALSE(d.should_calc);
    EXPECT_FALSE(d.update_last_check);
}

TEST(CalcPlusTime, PlustimeAllOffRequiresEventRateActive) {
    FixedEnv env;
    auto row = make_row(LEGACY_SHOP_ITEM_CHARM, /*mae=*/10, /*rem=*/60000);
    env.active_for[10] = false;
    auto d = calc_plus_time_row_decision(0, CalcPlusTimeType::PlustimeAllOff, row, env);
    EXPECT_FALSE(d.should_calc);
    EXPECT_FALSE(d.update_last_check);
}

TEST(CalcPlusTime, PlustimeAllOffFiresCalcTrueAndUpdatesAndContinues) {
    FixedEnv env;
    env.now_ms = 7777;
    env.active_for[10] = true;
    auto row = make_row(LEGACY_SHOP_ITEM_CHARM, /*mae=*/10, /*rem=*/60000);
    auto d = calc_plus_time_row_decision(0, CalcPlusTimeType::PlustimeAllOff, row, env);
    EXPECT_TRUE(d.should_calc);
    EXPECT_TRUE(d.b_add);
    EXPECT_TRUE(d.update_last_check);
    EXPECT_FALSE(d.stop_iteration);  // legacy does NOT return in ALLOFF
}

TEST(CalcPlusTime, PlustimeAllOffIgnoresDwEventIdx) {
    // ALLOFF is not gated by dwEventIdx.
    FixedEnv env;
    env.now_ms = 1000;
    env.active_for[10] = true;
    auto row = make_row(LEGACY_SHOP_ITEM_CHARM, /*mae=*/10, /*rem=*/60000);
    auto d = calc_plus_time_row_decision(999, CalcPlusTimeType::PlustimeAllOff, row, env);
    EXPECT_TRUE(d.should_calc);
    EXPECT_TRUE(d.update_last_check);
}

// ----- boundary -----

TEST(CalcPlusTime, EmptyEventRateTableAllFalse) {
    FixedEnv env;  // all zeros
    auto row = make_row(LEGACY_SHOP_ITEM_CHARM, /*mae=*/10, /*rem=*/60000);
    auto d = calc_plus_time_row_decision(0, CalcPlusTimeType::DefaultSentinel, row, env);
    EXPECT_FALSE(d.should_calc);
}

TEST(CalcPlusTime, MaxRemaintimeStillGatedByRate) {
    FixedEnv env;
    auto row = make_row(LEGACY_SHOP_ITEM_CHARM, /*mae=*/10, /*rem=*/0xFFFFFFFFu);
    env.active_for[10] = true;
    auto d = calc_plus_time_row_decision(0, CalcPlusTimeType::DefaultSentinel, row, env);
    EXPECT_TRUE(d.should_calc);
}
