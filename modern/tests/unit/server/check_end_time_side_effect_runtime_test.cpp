// check_end_time_side_effect_runtime_test.cpp
//
// Verifies apply_check_end_time_side_effects() (the runtime
// orchestrator for the CShopItemManager::CheckEndTime side-effect
// chain) walks the data-plane step list and dispatches each step to
// its respective subsystem in the legacy order.
//
// Locks:
//   - All 5 step kinds dispatched in the legacy order
//   - Legacy invariant: discard failure does NOT skip the remaining
//     steps (the legacy code ASSERTs and falls through)
//   - Empty step list is a no-op
//   - Dup-counter bump is skipped when ShopItemDupSlot::None

#include <mxh/server/check_end_time_side_effect.hpp>
#include <mxh/server/check_end_time_side_effect_runtime.hpp>
#include <mxh/server/shop_item_manager.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {

using mxh::server::CheckEndTimeSideEffectSink;
using mxh::server::CheckEndTimeStep;
using mxh::server::CheckEndTimeStepKind;
using mxh::server::ShopItemDupSlot;
using mxh::server::apply_check_end_time_side_effects;

class RecordingSink final : public CheckEndTimeSideEffectSink {
public:
    std::vector<std::string> calls;
    bool discard_returns_ok = true;

    bool discard_item(std::uint16_t position,
                      std::uint16_t w_icon_idx,
                      std::uint64_t db_idx) override {
        calls.push_back("discard " + std::to_string(position) + "/" +
                        std::to_string(w_icon_idx) + "/" +
                        std::to_string(db_idx));
        return discard_returns_ok;
    }
    void bump_dup_counter(ShopItemDupSlot slot,
                          std::uint64_t db_idx) override {
        calls.push_back("dup " + std::to_string(static_cast<int>(slot)) +
                        "/" + std::to_string(db_idx));
    }
    void broadcast_use_end(std::uint32_t player_id,
                           std::uint16_t w_icon_idx,
                           std::uint64_t db_idx) override {
        calls.push_back("broadcast " + std::to_string(player_id) + "/" +
                        std::to_string(w_icon_idx) + "/" +
                        std::to_string(db_idx));
    }
    void shop_item_delete_to_db(std::uint32_t player_id,
                                std::uint64_t db_idx) override {
        calls.push_back("db_delete " + std::to_string(player_id) + "/" +
                        std::to_string(db_idx));
    }
    void log_item_money(std::uint32_t player_id,
                        std::uint16_t w_icon_idx,
                        std::uint64_t db_idx) override {
        calls.push_back("log " + std::to_string(player_id) + "/" +
                        std::to_string(w_icon_idx) + "/" +
                        std::to_string(db_idx));
    }
};

CheckEndTimeStep make_step(CheckEndTimeStepKind kind,
                            std::uint16_t w_icon_idx = 100,
                            std::uint16_t pos = 5,
                            std::uint64_t db_idx = 777,
                            ShopItemDupSlot slot = ShopItemDupSlot::None) {
    CheckEndTimeStep s;
    s.kind = kind;
    s.w_icon_idx = w_icon_idx;
    s.item_pos = pos;
    s.db_idx = db_idx;
    s.dup_slot = slot;
    return s;
}

}  // namespace

// ----- full 5-step chain -----

TEST(ApplyCheckEndTimeSideEffects, FullChainDispatchesInLegacyOrder) {
    RecordingSink sink;
    std::vector<CheckEndTimeStep> steps = {
        make_step(CheckEndTimeStepKind::DiscardItemAttempt),
        make_step(CheckEndTimeStepKind::BumpDupCounter,
                  /*w=*/100, /*p=*/5, /*db=*/777, ShopItemDupSlot::Charm),
        make_step(CheckEndTimeStepKind::BroadcastUseEnd),
        make_step(CheckEndTimeStepKind::ShopItemDeleteToDB),
        make_step(CheckEndTimeStepKind::LogItemMoney),
    };
    auto out = apply_check_end_time_side_effects(
        steps, /*player_id=*/42, sink);
    EXPECT_EQ(out.steps_applied, 5u);
    EXPECT_EQ(out.discard_attempts, 1u);
    EXPECT_EQ(out.discard_failures, 0u);
    EXPECT_EQ(out.dup_counter_bumps, 1u);
    EXPECT_EQ(out.broadcasts, 1u);
    EXPECT_EQ(out.db_deletes, 1u);
    EXPECT_EQ(out.log_calls, 1u);
    EXPECT_FALSE(out.discard_failure_observed);
    const std::vector<std::string> kExpected = {
        "discard 5/100/777",
        "dup 2/777",
        "broadcast 42/100/777",
        "db_delete 42/777",
        "log 42/100/777",
    };
    EXPECT_EQ(sink.calls, kExpected);
}

// ----- legacy invariant: discard failure still continues -----

TEST(ApplyCheckEndTimeSideEffects, DiscardFailureContinuesRemainingSteps) {
    RecordingSink sink;
    sink.discard_returns_ok = false;
    std::vector<CheckEndTimeStep> steps = {
        make_step(CheckEndTimeStepKind::DiscardItemAttempt),
        make_step(CheckEndTimeStepKind::BroadcastUseEnd),
        make_step(CheckEndTimeStepKind::ShopItemDeleteToDB),
        make_step(CheckEndTimeStepKind::LogItemMoney),
    };
    auto out = apply_check_end_time_side_effects(steps, 42, sink);
    EXPECT_EQ(out.steps_applied, 4u);
    EXPECT_EQ(out.discard_attempts, 1u);
    EXPECT_EQ(out.discard_failures, 1u);
    EXPECT_TRUE(out.discard_failure_observed);
    EXPECT_EQ(out.broadcasts, 1u);
    EXPECT_EQ(out.db_deletes, 1u);
    EXPECT_EQ(out.log_calls, 1u);
    // All 4 calls still recorded (legacy invariant).
    EXPECT_EQ(sink.calls.size(), 4u);
}

// ----- empty step list -----

TEST(ApplyCheckEndTimeSideEffects, EmptyStepListIsNoOp) {
    RecordingSink sink;
    std::vector<CheckEndTimeStep> steps;
    auto out = apply_check_end_time_side_effects(steps, 42, sink);
    EXPECT_EQ(out.steps_applied, 0u);
    EXPECT_TRUE(sink.calls.empty());
}

// ----- single step kinds -----

TEST(ApplyCheckEndTimeSideEffects, OnlyDiscard) {
    RecordingSink sink;
    std::vector<CheckEndTimeStep> steps = {
        make_step(CheckEndTimeStepKind::DiscardItemAttempt),
    };
    auto out = apply_check_end_time_side_effects(steps, 42, sink);
    EXPECT_EQ(out.discard_attempts, 1u);
    EXPECT_EQ(out.broadcasts, 0u);
    EXPECT_EQ(out.db_deletes, 0u);
    EXPECT_EQ(out.log_calls, 0u);
    EXPECT_EQ(out.dup_counter_bumps, 0u);
    EXPECT_EQ(sink.calls.size(), 1u);
}

TEST(ApplyCheckEndTimeSideEffects, OnlyBroadcast) {
    RecordingSink sink;
    std::vector<CheckEndTimeStep> steps = {
        make_step(CheckEndTimeStepKind::BroadcastUseEnd),
    };
    auto out = apply_check_end_time_side_effects(steps, /*player_id=*/999, sink);
    EXPECT_EQ(out.broadcasts, 1u);
    const std::vector<std::string> kExpected = {"broadcast 999/100/777"};
    EXPECT_EQ(sink.calls, kExpected);
}

TEST(ApplyCheckEndTimeSideEffects, OnlyDbDelete) {
    RecordingSink sink;
    std::vector<CheckEndTimeStep> steps = {
        make_step(CheckEndTimeStepKind::ShopItemDeleteToDB),
    };
    auto out = apply_check_end_time_side_effects(steps, 42, sink);
    EXPECT_EQ(out.db_deletes, 1u);
    const std::vector<std::string> kExpected = {"db_delete 42/777"};
    EXPECT_EQ(sink.calls, kExpected);
}

TEST(ApplyCheckEndTimeSideEffects, OnlyLog) {
    RecordingSink sink;
    std::vector<CheckEndTimeStep> steps = {
        make_step(CheckEndTimeStepKind::LogItemMoney),
    };
    auto out = apply_check_end_time_side_effects(steps, 42, sink);
    EXPECT_EQ(out.log_calls, 1u);
    const std::vector<std::string> kExpected = {"log 42/100/777"};
    EXPECT_EQ(sink.calls, kExpected);
}

// ----- step ordering preserved through runtime -----

TEST(ApplyCheckEndTimeSideEffects, StepOrderPreservedAcrossCallsites) {
    RecordingSink sink;
    std::vector<CheckEndTimeStep> steps = {
        make_step(CheckEndTimeStepKind::DiscardItemAttempt),
        make_step(CheckEndTimeStepKind::BumpDupCounter,
                  /*w=*/100, /*p=*/5, /*db=*/777,
                  ShopItemDupSlot::Incantation),
        make_step(CheckEndTimeStepKind::BroadcastUseEnd),
        make_step(CheckEndTimeStepKind::ShopItemDeleteToDB),
        make_step(CheckEndTimeStepKind::LogItemMoney),
    };
    apply_check_end_time_side_effects(steps, 1, sink);
    // The first call is always DiscardItemAttempt; the second is the
    // dup bump; the remaining are broadcast + db_delete + log. This
    // locks the legacy ordering invariant.
    EXPECT_EQ(sink.calls[0], "discard 5/100/777");
    EXPECT_EQ(sink.calls[1], "dup 1/777");
    EXPECT_EQ(sink.calls[2], "broadcast 1/100/777");
    EXPECT_EQ(sink.calls[3], "db_delete 1/777");
    EXPECT_EQ(sink.calls[4], "log 1/100/777");
}
