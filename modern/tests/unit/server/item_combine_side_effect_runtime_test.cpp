// item_combine_side_effect_runtime_test.cpp
//
// Verifies apply_item_combine_side_effects() (the runtime
// orchestrator for the CItemManager::MP_ITEM_COMBINE_SYN side-effect
// chain) walks the data-plane plan and dispatches the single entry to
// its respective subsystem (ACK on CombineItem success / NACK with
// the fixed combine error code on failure).

#include <mxh/server/item_combine_side_effect.hpp>
#include <mxh/server/item_combine_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>

namespace {

using mxh::server::ItemCombineSideEffectKind;
using mxh::server::ItemCombineSideEffectSink;
using mxh::server::LEGACY_EITEMUSE_COMBINE;
using mxh::server::apply_item_combine_side_effects;
using mxh::server::item_combine_side_effect_plan;

class RecordingSink final : public ItemCombineSideEffectSink {
public:
    std::string last_call;
    int last_rt = 0;
    int last_ecode = 0;
    std::uint16_t last_from_pos = 0;
    std::uint16_t last_to_pos = 0;
    std::uint16_t last_item_idx = 0;
    std::uint16_t last_from_dur = 0;
    std::uint16_t last_to_dur = 0;
    std::size_t ack_count = 0;
    std::size_t nack_count = 0;

    void broadcast_combine_ack(std::uint16_t from_pos,
                               std::uint16_t to_pos,
                               std::uint16_t item_idx,
                               std::uint16_t from_dur,
                               std::uint16_t to_dur,
                               int original_rt) override {
        last_call = "ack";
        last_from_pos = from_pos;
        last_to_pos = to_pos;
        last_item_idx = item_idx;
        last_from_dur = from_dur;
        last_to_dur = to_dur;
        last_rt = original_rt;
        ++ack_count;
    }
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
};

}  // namespace

TEST(ApplyItemCombineSideEffects, SuccessRtEmitsAck) {
    auto plan = item_combine_side_effect_plan(
        /*combine_rt=*/0, /*from_pos=*/10, /*to_pos=*/20,
        /*item_idx=*/30, /*from_dur=*/40, /*to_dur=*/50);
    EXPECT_TRUE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemCombineSideEffectKind::BroadcastCombineAck);

    RecordingSink sink;
    auto out = apply_item_combine_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.acks_sent, 1u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_TRUE(out.ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_EQ(sink.last_call, "ack");
    EXPECT_EQ(sink.last_from_pos, 10u);
    EXPECT_EQ(sink.last_to_pos, 20u);
    EXPECT_EQ(sink.last_item_idx, 30u);
    EXPECT_EQ(sink.last_from_dur, 40u);
    EXPECT_EQ(sink.last_to_dur, 50u);
    EXPECT_EQ(sink.last_rt, 0);
    EXPECT_EQ(sink.ack_count, 1u);
    EXPECT_EQ(sink.nack_count, 0u);
}

TEST(ApplyItemCombineSideEffects, FailureRtEmitsNackWithFixedCombineEcode) {
    // Legacy: any non-zero rt emits NACK with ECode =
    // eItemUseErr_Combine (= 3) and the original rt as aux code.
    auto plan = item_combine_side_effect_plan(
        /*combine_rt=*/7, /*from_pos=*/10, /*to_pos=*/20,
        /*item_idx=*/30, /*from_dur=*/40, /*to_dur=*/50);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_TRUE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemCombineSideEffectKind::BroadcastErrorNack);

    RecordingSink sink;
    auto out = apply_item_combine_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.acks_sent, 0u);
    EXPECT_EQ(out.nacks_sent, 1u);
    EXPECT_FALSE(out.ack_flag_consumed);
    EXPECT_TRUE(out.nack_flag_consumed);
    EXPECT_EQ(sink.last_call, "nack");
    EXPECT_EQ(sink.last_rt, 7);
    EXPECT_EQ(sink.last_ecode, LEGACY_EITEMUSE_COMBINE);
    EXPECT_EQ(sink.ack_count, 0u);
    EXPECT_EQ(sink.nack_count, 1u);
}

TEST(ApplyItemCombineSideEffects, EmptyPlanIsNoOp) {
    mxh::server::ItemCombineSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_item_combine_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.acks_sent, 0u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_FALSE(out.ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_EQ(sink.last_call, "");
    EXPECT_EQ(sink.ack_count, 0u);
    EXPECT_EQ(sink.nack_count, 0u);
}

TEST(ApplyItemCombineSideEffects, VariousFailureCodesAllEmitNack) {
    for (int rt : {1, 3, 99, 1000, -1}) {
        auto plan = item_combine_side_effect_plan(
            rt, 1, 2, 3, 4, 5);
        EXPECT_TRUE(plan.send_nack);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].kind,
                  ItemCombineSideEffectKind::BroadcastErrorNack);
        EXPECT_EQ(plan.effects[0].original_rt, rt);

        RecordingSink sink;
        (void)apply_item_combine_side_effects(plan, sink);
        EXPECT_EQ(sink.last_call, "nack");
        EXPECT_EQ(sink.last_rt, rt);
        EXPECT_EQ(sink.last_ecode, LEGACY_EITEMUSE_COMBINE);
    }
}

TEST(ApplyItemCombineSideEffects, NackDoesNotTouchAckState) {
    auto plan = item_combine_side_effect_plan(5, 1, 2, 3, 4, 5);
    RecordingSink sink;
    (void)apply_item_combine_side_effects(plan, sink);
    EXPECT_EQ(sink.last_call, "nack");
    EXPECT_EQ(sink.last_ecode, LEGACY_EITEMUSE_COMBINE);
    EXPECT_EQ(sink.ack_count, 0u);
    EXPECT_EQ(sink.nack_count, 1u);
}

TEST(ApplyItemCombineSideEffects, FieldPassthroughOnAck) {
    auto plan = item_combine_side_effect_plan(0, 11, 12, 13, 14, 15);
    RecordingSink sink;
    (void)apply_item_combine_side_effects(plan, sink);
    EXPECT_EQ(sink.last_call, "ack");
    EXPECT_EQ(sink.last_from_pos, 11u);
    EXPECT_EQ(sink.last_to_pos, 12u);
    EXPECT_EQ(sink.last_item_idx, 13u);
    EXPECT_EQ(sink.last_from_dur, 14u);
    EXPECT_EQ(sink.last_to_dur, 15u);
    EXPECT_EQ(sink.last_rt, 0);
    EXPECT_EQ(sink.nack_count, 0u);
}
