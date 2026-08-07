// item_divide_side_effect_runtime_test.cpp
//
// Verifies apply_item_divide_side_effects() (the runtime orchestrator
// for the CItemManager::MP_ITEM_DIVIDE_SYN side-effect chain) walks
// the data-plane plan and dispatches the entry to its respective
// subsystem (silent success on DivideItem rt==0 / NACK with the fixed
// divide error code on failure).

#include <mxh/server/item_divide_side_effect.hpp>
#include <mxh/server/item_divide_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>

namespace {

using mxh::server::ItemDivideSideEffectKind;
using mxh::server::ItemDivideSideEffectSink;
using mxh::server::LEGACY_EITEMUSE_DIVIDE;
using mxh::server::apply_item_divide_side_effects;
using mxh::server::item_divide_side_effect_plan;

class RecordingSink final : public ItemDivideSideEffectSink {
public:
    std::string last_call;
    int last_rt = 0;
    int last_ecode = 0;
    std::uint16_t last_from_pos = 0;
    std::uint16_t last_to_pos = 0;
    std::uint16_t last_item_idx = 0;
    std::uint16_t last_from_dur = 0;
    std::uint16_t last_to_dur = 0;
    std::size_t nack_count = 0;
    std::size_t silent_count = 0;

    void broadcast_error_nack(std::uint16_t from_pos,
                              std::uint16_t to_pos,
                              std::uint16_t item_idx,
                              std::uint16_t from_dur,
                              std::uint16_t to_dur,
                              int original_rt,
                              int ecode) override {
        last_call = "nack";
        last_from_pos = from_pos;
        last_to_pos = to_pos;
        last_item_idx = item_idx;
        last_from_dur = from_dur;
        last_to_dur = to_dur;
        last_rt = original_rt;
        last_ecode = ecode;
        ++nack_count;
    }
    void silent_success() override {
        last_call = "silent";
        ++silent_count;
    }
};

}  // namespace

TEST(ApplyItemDivideSideEffects, SuccessRtIsSilentNoWire) {
    // Legacy: rt == 0 -> empty body; ObtainItemEx emits its own ACK.
    auto plan = item_divide_side_effect_plan(
        /*divide_rt=*/0, /*from_pos=*/10, /*to_pos=*/20,
        /*item_idx=*/30, /*from_dur=*/40, /*to_dur=*/50);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_TRUE(plan.silent_success);
    EXPECT_TRUE(plan.effects.empty());

    RecordingSink sink;
    auto out = apply_item_divide_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_EQ(out.silent_successes, 1u);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_TRUE(out.silent_flag_consumed);
    EXPECT_EQ(sink.last_call, "silent");
    EXPECT_EQ(sink.silent_count, 1u);
    EXPECT_EQ(sink.nack_count, 0u);
}

TEST(ApplyItemDivideSideEffects, FailureRtEmitsNackWithFixedDivideEcode) {
    // Legacy: any non-zero rt emits NACK with ECode =
    // eItemUseErr_Divide (= 4) and the original rt as aux code.
    auto plan = item_divide_side_effect_plan(
        /*divide_rt=*/7, /*from_pos=*/10, /*to_pos=*/20,
        /*item_idx=*/30, /*from_dur=*/40, /*to_dur=*/50);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_FALSE(plan.silent_success);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemDivideSideEffectKind::BroadcastErrorNack);

    RecordingSink sink;
    auto out = apply_item_divide_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.nacks_sent, 1u);
    EXPECT_EQ(out.silent_successes, 0u);
    EXPECT_TRUE(out.nack_flag_consumed);
    EXPECT_FALSE(out.silent_flag_consumed);
    EXPECT_EQ(sink.last_call, "nack");
    EXPECT_EQ(sink.last_rt, 7);
    EXPECT_EQ(sink.last_ecode, LEGACY_EITEMUSE_DIVIDE);
    EXPECT_EQ(sink.nack_count, 1u);
    EXPECT_EQ(sink.silent_count, 0u);
}

TEST(ApplyItemDivideSideEffects, EmptyPlanIsNoOp) {
    mxh::server::ItemDivideSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_item_divide_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_EQ(out.silent_successes, 0u);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_FALSE(out.silent_flag_consumed);
    EXPECT_EQ(sink.last_call, "");
    EXPECT_EQ(sink.nack_count, 0u);
    EXPECT_EQ(sink.silent_count, 0u);
}

TEST(ApplyItemDivideSideEffects, VariousFailureCodesAllEmitNack) {
    for (int rt : {1, 2, 5, 99, 100, -1}) {
        auto plan = item_divide_side_effect_plan(
            rt, 1, 2, 3, 4, 5);
        EXPECT_TRUE(plan.send_nack);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].kind,
                  ItemDivideSideEffectKind::BroadcastErrorNack);
        EXPECT_EQ(plan.effects[0].original_rt, rt);

        RecordingSink sink;
        (void)apply_item_divide_side_effects(plan, sink);
        EXPECT_EQ(sink.last_call, "nack");
        EXPECT_EQ(sink.last_rt, rt);
        EXPECT_EQ(sink.last_ecode, LEGACY_EITEMUSE_DIVIDE);
    }
}

TEST(ApplyItemDivideSideEffects, NackDoesNotTouchSilentState) {
    auto plan = item_divide_side_effect_plan(3, 1, 2, 3, 4, 5);
    RecordingSink sink;
    (void)apply_item_divide_side_effects(plan, sink);
    EXPECT_EQ(sink.last_call, "nack");
    EXPECT_EQ(sink.last_ecode, LEGACY_EITEMUSE_DIVIDE);
    EXPECT_EQ(sink.nack_count, 1u);
    EXPECT_EQ(sink.silent_count, 0u);
}

TEST(ApplyItemDivideSideEffects, FieldPassthroughOnNack) {
    auto plan = item_divide_side_effect_plan(9, 11, 12, 13, 14, 15);
    RecordingSink sink;
    (void)apply_item_divide_side_effects(plan, sink);
    EXPECT_EQ(sink.last_call, "nack");
    EXPECT_EQ(sink.last_from_pos, 11u);
    EXPECT_EQ(sink.last_to_pos, 12u);
    EXPECT_EQ(sink.last_item_idx, 13u);
    EXPECT_EQ(sink.last_from_dur, 14u);
    EXPECT_EQ(sink.last_to_dur, 15u);
    EXPECT_EQ(sink.last_rt, 9);
    EXPECT_EQ(sink.last_ecode, LEGACY_EITEMUSE_DIVIDE);
}
