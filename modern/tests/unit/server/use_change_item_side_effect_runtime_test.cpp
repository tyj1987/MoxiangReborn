// use_change_item_side_effect_runtime_test.cpp
//
// Verifies apply_use_change_item_side_effects() (the runtime
// orchestrator for the CItemManager::MP_ITEM_USE_CHANGEITEM_SYN
// side-effect chain) walks the data-plane plan and dispatches each
// entry: NACK on "not used" (rt==0) / silent success on item
// transformed (rt!=0) / no-op when the player is missing.

#include <mxh/server/use_change_item_side_effect.hpp>
#include <mxh/server/use_change_item_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>

namespace {

using mxh::server::UseChangeItemSideEffectKind;
using mxh::server::UseChangeItemSideEffectSink;
using mxh::server::apply_use_change_item_side_effects;
using mxh::server::use_change_item_side_effect_plan;

class RecordingSink final : public UseChangeItemSideEffectSink {
public:
    std::string last_call;
    int last_rt = 0;
    int last_error_code = 0;
    std::uint16_t last_target_pos = 0;
    std::uint16_t last_item_idx = 0;
    std::size_t nack_count = 0;
    std::size_t silent_count = 0;

    void broadcast_use_nack(std::uint16_t target_pos,
                            std::uint16_t item_idx,
                            int original_rt,
                            int error_code) override {
        last_call = "nack";
        last_target_pos = target_pos;
        last_item_idx = item_idx;
        last_rt = original_rt;
        last_error_code = error_code;
        ++nack_count;
    }
    void silent_success() override {
        last_call = "silent";
        ++silent_count;
    }
};

}  // namespace

TEST(ApplyUseChangeItemSideEffects, NotUsedRtZeroEmitsNack) {
    // Legacy: rt == 0 ("not use") -> NACK with ECode = rt (= 0).
    auto plan = use_change_item_side_effect_plan(
        /*use_rt=*/0, /*player_found=*/true,
        /*target_pos=*/10, /*item_idx=*/100);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_FALSE(plan.silent_success);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              UseChangeItemSideEffectKind::BroadcastUseNack);
    EXPECT_EQ(plan.effects[0].error_code, 0);

    RecordingSink sink;
    auto out = apply_use_change_item_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.nacks_sent, 1u);
    EXPECT_EQ(out.silent_successes, 0u);
    EXPECT_TRUE(out.nack_flag_consumed);
    EXPECT_FALSE(out.silent_flag_consumed);
    EXPECT_EQ(sink.last_call, "nack");
    EXPECT_EQ(sink.last_target_pos, 10u);
    EXPECT_EQ(sink.last_item_idx, 100u);
    EXPECT_EQ(sink.last_rt, 0);
    EXPECT_EQ(sink.last_error_code, 0);
    EXPECT_EQ(sink.nack_count, 1u);
    EXPECT_EQ(sink.silent_count, 0u);
}

TEST(ApplyUseChangeItemSideEffects, TransformedRtNonZeroIsSilentSuccess) {
    // Legacy: rt != 0 (item transformed) -> NO network response.
    auto plan = use_change_item_side_effect_plan(
        /*use_rt=*/1, /*player_found=*/true,
        /*target_pos=*/10, /*item_idx=*/100);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_TRUE(plan.silent_success);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              UseChangeItemSideEffectKind::SilentSuccess);

    RecordingSink sink;
    auto out = apply_use_change_item_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_EQ(out.silent_successes, 1u);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_TRUE(out.silent_flag_consumed);
    EXPECT_EQ(sink.last_call, "silent");
    EXPECT_EQ(sink.silent_count, 1u);
    EXPECT_EQ(sink.nack_count, 0u);
}

TEST(ApplyUseChangeItemSideEffects, NoPlayerIsNoOp) {
    auto plan = use_change_item_side_effect_plan(
        /*use_rt=*/0, /*player_found=*/false, 10, 100);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_FALSE(plan.silent_success);
    EXPECT_TRUE(plan.effects.empty());

    RecordingSink sink;
    auto out = apply_use_change_item_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_EQ(out.silent_successes, 0u);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_FALSE(out.silent_flag_consumed);
    EXPECT_EQ(sink.last_call, "");
    EXPECT_EQ(sink.nack_count, 0u);
    EXPECT_EQ(sink.silent_count, 0u);
}

TEST(ApplyUseChangeItemSideEffects, EmptyPlanIsNoOp) {
    mxh::server::UseChangeItemSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_use_change_item_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_EQ(out.silent_successes, 0u);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_FALSE(out.silent_flag_consumed);
    EXPECT_EQ(sink.last_call, "");
    EXPECT_EQ(sink.nack_count, 0u);
    EXPECT_EQ(sink.silent_count, 0u);
}

TEST(ApplyUseChangeItemSideEffects, VariousSuccessRtsAllSilent) {
    for (int rt : {1, 2, 99, -1}) {
        auto plan = use_change_item_side_effect_plan(
            rt, /*player_found=*/true, 1, 2);
        EXPECT_TRUE(plan.silent_success);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].kind,
                  UseChangeItemSideEffectKind::SilentSuccess);

        RecordingSink sink;
        (void)apply_use_change_item_side_effects(plan, sink);
        EXPECT_EQ(sink.last_call, "silent");
        EXPECT_EQ(sink.silent_count, 1u);
        EXPECT_EQ(sink.nack_count, 0u);
    }
}

TEST(ApplyUseChangeItemSideEffects, FieldPassthroughOnNack) {
    auto plan = use_change_item_side_effect_plan(
        /*use_rt=*/0, /*player_found=*/true, 21, 22);
    RecordingSink sink;
    (void)apply_use_change_item_side_effects(plan, sink);
    EXPECT_EQ(sink.last_call, "nack");
    EXPECT_EQ(sink.last_target_pos, 21u);
    EXPECT_EQ(sink.last_item_idx, 22u);
    EXPECT_EQ(sink.last_rt, 0);
    EXPECT_EQ(sink.last_error_code, 0);
    EXPECT_EQ(sink.silent_count, 0u);
}
