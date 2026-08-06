// update_logout_to_db_test.cpp - 1:1 data-plane tests for the legacy
// CShopItemManager::UpdateLogoutToDB() from [Server]Map/ShopItemManager.cpp:1195.
// Locks the per-row Remaintime decrement semantics, the env.event_rate_active
// plustime gate, and the 30000 ms checktime cap.

#include <mxh/game/shop_item_types.hpp>
#include <mxh/server/update_logout_to_db.hpp>

#include <gtest/gtest.h>

#include <cstdint>

using mxh::server::CalcShopItemOptionEnv;
using mxh::server::UpdateLogoutRowDecision;
using mxh::server::UpdateLogoutToDBInfo;
using mxh::server::update_logout_to_db_decision;

namespace {

class TestEnv final : public CalcShopItemOptionEnv {
public:
    bool event_rate_active(std::uint16_t rate_id) const noexcept override {
        (void)rate_id;
        return rate_active;
    }
    bool rate_active = true;
};

UpdateLogoutToDBInfo PlayTimeCharm() {
    UpdateLogoutToDBInfo info;
    info.sell_price = mxh::server::LEGACY_SHOP_ITEM_USE_PARAM_PLAYTIME;
    info.item_kind  = mxh::server::LEGACY_SHOP_ITEM_CHARM;
    return info;
}

UpdateLogoutToDBInfo PlayTimeCharmPlusTime(std::uint16_t melee_attack_min) {
    UpdateLogoutToDBInfo info = PlayTimeCharm();
    info.melee_attack_min = melee_attack_min;
    return info;
}

UpdateLogoutToDBInfo PlayTimeNonCharm() {
    UpdateLogoutToDBInfo info;
    info.sell_price = mxh::server::LEGACY_SHOP_ITEM_USE_PARAM_PLAYTIME;
    info.item_kind  = mxh::server::LEGACY_SHOP_ITEM_HERB;
    return info;
}

UpdateLogoutToDBInfo RealTimeItem() {
    UpdateLogoutToDBInfo info;
    info.sell_price = mxh::game::SHOP_ITEM_PARAM_STORED_TIME;
    info.item_kind  = mxh::server::LEGACY_SHOP_ITEM_CHARM;
    return info;
}

}  // namespace

// Non-PLAYTIME items are dropped: no DB write, no Remaintime change.
TEST(UpdateLogoutToDB, RealTimeItemIsDropped) {
    auto info = RealTimeItem();
    TestEnv env;
    auto dec = update_logout_to_db_decision(60'000, 0, 60'000, info, env);
    EXPECT_EQ(dec.action, UpdateLogoutRowDecision::Action::Drop);
    EXPECT_EQ(dec.new_remaintime, 60'000u);
}

// Default PLAYTIME path: Remaintime decrements by elapsed time.
TEST(UpdateLogoutToDB, PlayTimeDecrementsByElapsedTime) {
    auto info = PlayTimeNonCharm();
    TestEnv env;
    auto dec = update_logout_to_db_decision(60'000, 0, 30'000, info, env);
    EXPECT_EQ(dec.action, UpdateLogoutRowDecision::Action::Persist);
    EXPECT_EQ(dec.new_remaintime, 30'000u);
    EXPECT_EQ(dec.new_last_check, 30'000u);
}

// Remaintime 0 -> clamp to 0 (no underflow).
TEST(UpdateLogoutToDB, RemaintimeUnderflowClampsToZero) {
    auto info = PlayTimeNonCharm();
    TestEnv env;
    auto dec = update_logout_to_db_decision(5'000, 0, 30'000, info, env);
    EXPECT_EQ(dec.action, UpdateLogoutRowDecision::Action::Persist);
    EXPECT_EQ(dec.new_remaintime, 0u);
}

// Plustime gate: env_rate ACTIVE -> falls through to normal decrement.
TEST(UpdateLogoutToDB, PlustimeActiveDecrementsNormally) {
    auto info = PlayTimeCharmPlusTime(7);
    TestEnv env;
    env.rate_active = true;
    auto dec = update_logout_to_db_decision(60'000, 0, 30'000, info, env);
    EXPECT_EQ(dec.action, UpdateLogoutRowDecision::Action::Persist);
    EXPECT_EQ(dec.new_remaintime, 30'000u);
}

// Plustime gate: env_rate INACTIVE -> Skip (stamp LastCheckTime, keep Remaintime).
TEST(UpdateLogoutToDB, PlustimeInactiveSkipsRemaintimeUpdate) {
    auto info = PlayTimeCharmPlusTime(7);
    TestEnv env;
    env.rate_active = false;
    auto dec = update_logout_to_db_decision(60'000, 0, 30'000, info, env);
    EXPECT_EQ(dec.action, UpdateLogoutRowDecision::Action::Skip);
    EXPECT_EQ(dec.new_remaintime, 60'000u);
    EXPECT_EQ(dec.new_last_check, 30'000u);
}

// Plustime gate with Remaintime == 0 -> Drop (no DB write needed).
TEST(UpdateLogoutToDB, PlustimeRemaintimeZeroAlwaysDrops) {
    auto info = PlayTimeCharmPlusTime(7);
    TestEnv env;
    env.rate_active = false;
    auto dec = update_logout_to_db_decision(0, 0, 30'000, info, env);
    EXPECT_EQ(dec.action, UpdateLogoutRowDecision::Action::Drop);
    EXPECT_EQ(dec.new_remaintime, 0u);
}

// Plustime gate with MeleeAttackMin == 0 -> falls through to normal decrement
// (the plustime path is gated on MeleeAttackMin != 0).
TEST(UpdateLogoutToDB, PlustimeDisabledWhenMeleeAttackMinZero) {
    auto info = PlayTimeCharmPlusTime(0);  // MeleeAttackMin = 0
    TestEnv env;
    env.rate_active = false;
    auto dec = update_logout_to_db_decision(60'000, 0, 30'000, info, env);
    EXPECT_EQ(dec.action, UpdateLogoutRowDecision::Action::Persist);
    EXPECT_EQ(dec.new_remaintime, 30'000u);
}

// 30000 ms checktime cap.
TEST(UpdateLogoutToDB, CheckTimeCapClampsToThirtySeconds) {
    auto info = PlayTimeNonCharm();
    TestEnv env;
    // Started at 0, current is 90000 ms (very long), cap to 30000.
    auto dec = update_logout_to_db_decision(60'000, 0, 90'000, info, env);
    EXPECT_EQ(dec.action, UpdateLogoutRowDecision::Action::Persist);
    EXPECT_EQ(dec.new_remaintime, 30'000u);
}

// Legacy behavior: if LastCheckTime > gCurTime (clock skew), checktime=0.
TEST(UpdateLogoutToDB, ClockSkewYieldsZeroChecktime) {
    auto info = PlayTimeNonCharm();
    TestEnv env;
    auto dec = update_logout_to_db_decision(60'000, 90'000, 30'000, info, env);
    EXPECT_EQ(dec.action, UpdateLogoutRowDecision::Action::Persist);
    EXPECT_EQ(dec.new_remaintime, 60'000u);
}

// Plustime + Remaintime 0 + env_rate ACTIVE -> Drop (legacy `else if` branch).
TEST(UpdateLogoutToDB, PlustimeRemaintimeZeroWithActiveRateDrops) {
    auto info = PlayTimeCharmPlusTime(7);
    TestEnv env;
    env.rate_active = true;
    auto dec = update_logout_to_db_decision(0, 0, 30'000, info, env);
    EXPECT_EQ(dec.action, UpdateLogoutRowDecision::Action::Drop);
    EXPECT_EQ(dec.new_remaintime, 0u);
}

// Integration: PLAYTIME + Charm + MeleeAttackMin + env_rate inactive + valid
// Remaintime -> Skip path.
TEST(UpdateLogoutToDB, IntegrationCharmPlustimeInactiveSkip) {
    auto info = PlayTimeCharmPlusTime(7);
    TestEnv env;
    env.rate_active = false;
    auto dec = update_logout_to_db_decision(120'000, 0, 30'000, info, env);
    EXPECT_EQ(dec.action, UpdateLogoutRowDecision::Action::Skip);
    EXPECT_EQ(dec.new_remaintime, 120'000u);
    EXPECT_EQ(dec.new_last_check, 30'000u);
}

// Integration: PLAYTIME + Charm + MeleeAttackMin + env_rate active + valid
// Remaintime -> Persist (decrement).
TEST(UpdateLogoutToDB, IntegrationCharmPlustimeActivePersist) {
    auto info = PlayTimeCharmPlusTime(7);
    TestEnv env;
    env.rate_active = true;
    auto dec = update_logout_to_db_decision(120'000, 0, 30'000, info, env);
    EXPECT_EQ(dec.action, UpdateLogoutRowDecision::Action::Persist);
    EXPECT_EQ(dec.new_remaintime, 90'000u);
}
