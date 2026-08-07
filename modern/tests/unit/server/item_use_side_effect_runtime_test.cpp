// item_use_side_effect_runtime_test.cpp
//
// Verifies apply_item_use_side_effects() (the runtime orchestrator
// for the CItemManager::MP_ITEM_USE_SYN side-effect chain) walks the
// data-plane plan and dispatches the single entry to its respective
// subsystem (ACK on success / NACK on failure).

#include <mxh/server/item_use_side_effect.hpp>
#include <mxh/server/item_use_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>

namespace {

using mxh::server::ItemUseSideEffectKind;
using mxh::server::ItemUseSideEffectSink;
using mxh::server::apply_item_use_side_effects;
using mxh::server::item_use_side_effect_plan;

class RecordingSink final : public ItemUseSideEffectSink {
public:
    std::string last_call;
    int last_rt = 0;
    std::uint16_t last_target_pos = 0;
    std::uint16_t last_item_idx = 0;

    void broadcast_use_ack(std::uint16_t target_pos,
                           std::uint16_t item_idx,
                           int original_rt) override {
        last_call = "ack";
        last_target_pos = target_pos;
        last_item_idx = item_idx;
        last_rt = original_rt;
    }
    void broadcast_use_nack(std::uint16_t target_pos,
                            std::uint16_t item_idx,
                            int original_rt) override {
        last_call = "nack";
        last_target_pos = target_pos;
        last_item_idx = item_idx;
        last_rt = original_rt;
    }
};

}  // namespace

TEST(ApplyItemUseSideEffects, SuccessRtEmitsAck) {
    auto plan = item_use_side_effect_plan(
        /*use_rt=*/0, /*target_pos=*/5, /*item_idx=*/100);
    EXPECT_TRUE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemUseSideEffectKind::BroadcastUseAck);
    EXPECT_EQ(plan.effects[0].target_pos, 5u);
    EXPECT_EQ(plan.effects[0].item_idx, 100u);
    EXPECT_EQ(plan.effects[0].original_rt, 0);

    RecordingSink sink;
    auto out = apply_item_use_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.acks_sent, 1u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_TRUE(out.ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_EQ(sink.last_call, "ack");
    EXPECT_EQ(sink.last_target_pos, 5u);
    EXPECT_EQ(sink.last_item_idx, 100u);
    EXPECT_EQ(sink.last_rt, 0);
}

TEST(ApplyItemUseSideEffects, NonZeroRtEmitsNack) {
    auto plan = item_use_side_effect_plan(
        /*use_rt=*/3, /*target_pos=*/10, /*item_idx=*/200);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_TRUE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemUseSideEffectKind::BroadcastUseNack);
    EXPECT_EQ(plan.effects[0].original_rt, 3);

    RecordingSink sink;
    auto out = apply_item_use_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.acks_sent, 0u);
    EXPECT_EQ(out.nacks_sent, 1u);
    EXPECT_FALSE(out.ack_flag_consumed);
    EXPECT_TRUE(out.nack_flag_consumed);
    EXPECT_EQ(sink.last_call, "nack");
    EXPECT_EQ(sink.last_rt, 3);
}

TEST(ApplyItemUseSideEffects, EmptyPlanIsNoOp) {
    mxh::server::ItemUseSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_item_use_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.acks_sent, 0u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_FALSE(out.ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_EQ(sink.last_call, "");
}

TEST(ApplyItemUseSideEffects, VariousFailureCodesAllEmitNack) {
    // Legacy: any non-zero use_rt emits NACK with the same rt.
    for (int rt : {1, 2, 5, 99, -1, 100}) {
        auto plan = item_use_side_effect_plan(rt, 1, 1);
        EXPECT_TRUE(plan.send_nack);
        EXPECT_EQ(plan.effects[0].original_rt, rt);
    }
}
