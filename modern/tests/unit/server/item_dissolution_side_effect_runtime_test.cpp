// item_dissolution_side_effect_runtime_test.cpp
//
// Verifies apply_item_dissolution_side_effects() (the runtime
// orchestrator for the CItemManager::MP_ITEM_DISSOLUTION_SYN
// side-effect chain) walks the data-plane plan and dispatches the
// single entry to its subsystem: ACK on success / plain NACK on
// failure (both MSGBASE, not MSG_ITEM_ERROR).

#include <mxh/server/item_dissolution_side_effect.hpp>
#include <mxh/server/item_dissolution_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>

namespace {

using mxh::server::ItemDissolutionSideEffectKind;
using mxh::server::ItemDissolutionSideEffectSink;
using mxh::server::apply_item_dissolution_side_effects;
using mxh::server::item_dissolution_side_effect_plan;

class RecordingSink final : public ItemDissolutionSideEffectSink {
public:
    std::string last_call;
    int last_rt = 0;
    std::uint16_t last_item_idx = 0;
    std::uint16_t last_item_pos = 0;
    std::size_t ack_count = 0;
    std::size_t nack_count = 0;

    void broadcast_dissolution_ack(std::uint16_t item_idx,
                                   std::uint16_t item_pos,
                                   int original_rt) override {
        last_call = "ack";
        last_item_idx = item_idx;
        last_item_pos = item_pos;
        last_rt = original_rt;
        ++ack_count;
    }
    void broadcast_dissolution_nack(std::uint16_t item_idx,
                                    std::uint16_t item_pos,
                                    int original_rt) override {
        last_call = "nack";
        last_item_idx = item_idx;
        last_item_pos = item_pos;
        last_rt = original_rt;
        ++nack_count;
    }
};

}  // namespace

TEST(ApplyItemDissolutionSideEffects, SuccessRtEmitsAck) {
    auto plan = item_dissolution_side_effect_plan(
        /*dissolution_rt=*/0, /*item_idx=*/100, /*item_pos=*/7);
    EXPECT_TRUE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemDissolutionSideEffectKind::BroadcastDissolutionAck);

    RecordingSink sink;
    auto out = apply_item_dissolution_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.acks_sent, 1u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_TRUE(out.ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_EQ(sink.last_call, "ack");
    EXPECT_EQ(sink.last_item_idx, 100u);
    EXPECT_EQ(sink.last_item_pos, 7u);
    EXPECT_EQ(sink.last_rt, 0);
    EXPECT_EQ(sink.ack_count, 1u);
    EXPECT_EQ(sink.nack_count, 0u);
}

TEST(ApplyItemDissolutionSideEffects, FailureRtEmitsPlainNack) {
    // Legacy: non-zero rt sends MSGBASE with the NACK protocol (NOT
    // MSG_ITEM_ERROR).
    auto plan = item_dissolution_side_effect_plan(
        /*dissolution_rt=*/5, /*item_idx=*/100, /*item_pos=*/7);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_TRUE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemDissolutionSideEffectKind::BroadcastDissolutionNack);

    RecordingSink sink;
    auto out = apply_item_dissolution_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.acks_sent, 0u);
    EXPECT_EQ(out.nacks_sent, 1u);
    EXPECT_FALSE(out.ack_flag_consumed);
    EXPECT_TRUE(out.nack_flag_consumed);
    EXPECT_EQ(sink.last_call, "nack");
    EXPECT_EQ(sink.last_rt, 5);
    EXPECT_EQ(sink.ack_count, 0u);
    EXPECT_EQ(sink.nack_count, 1u);
}

TEST(ApplyItemDissolutionSideEffects, EmptyPlanIsNoOp) {
    mxh::server::ItemDissolutionSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_item_dissolution_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.acks_sent, 0u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_FALSE(out.ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_EQ(sink.last_call, "");
    EXPECT_EQ(sink.ack_count, 0u);
    EXPECT_EQ(sink.nack_count, 0u);
}

TEST(ApplyItemDissolutionSideEffects, VariousFailureCodesAllEmitNack) {
    for (int rt : {1, 5, 99, 1000, -1}) {
        auto plan = item_dissolution_side_effect_plan(rt, 1, 2);
        EXPECT_TRUE(plan.send_nack);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].kind,
                  ItemDissolutionSideEffectKind::BroadcastDissolutionNack);

        RecordingSink sink;
        (void)apply_item_dissolution_side_effects(plan, sink);
        EXPECT_EQ(sink.last_call, "nack");
        EXPECT_EQ(sink.last_rt, rt);
    }
}

TEST(ApplyItemDissolutionSideEffects, NackDoesNotTouchAckState) {
    auto plan = item_dissolution_side_effect_plan(3, 1, 2);
    RecordingSink sink;
    (void)apply_item_dissolution_side_effects(plan, sink);
    EXPECT_EQ(sink.last_call, "nack");
    EXPECT_EQ(sink.ack_count, 0u);
    EXPECT_EQ(sink.nack_count, 1u);
}

TEST(ApplyItemDissolutionSideEffects, FieldPassthroughOnAck) {
    auto plan = item_dissolution_side_effect_plan(0, 21, 22);
    RecordingSink sink;
    (void)apply_item_dissolution_side_effects(plan, sink);
    EXPECT_EQ(sink.last_call, "ack");
    EXPECT_EQ(sink.last_item_idx, 21u);
    EXPECT_EQ(sink.last_item_pos, 22u);
    EXPECT_EQ(sink.last_rt, 0);
    EXPECT_EQ(sink.nack_count, 0u);
}
