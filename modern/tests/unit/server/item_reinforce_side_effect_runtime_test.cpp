// item_reinforce_side_effect_runtime_test.cpp
//
// Verifies apply_item_reinforce_side_effects() (the runtime
// orchestrator for the CItemManager::MP_ITEM_REINFORCE_SYN
// side-effect chain) walks the data-plane plan and dispatches the
// single entry: silent success on rt==0 / failed-ACK on rt==99 /
// error-NACK with rt otherwise.

#include <mxh/server/item_reinforce_side_effect.hpp>
#include <mxh/server/item_reinforce_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>

namespace {

using mxh::server::ItemReinforceSideEffectKind;
using mxh::server::ItemReinforceSideEffectSink;
using mxh::server::LEGACY_REINFORCE_FAILED_RT;
using mxh::server::apply_item_reinforce_side_effects;
using mxh::server::item_reinforce_side_effect_plan;

class RecordingSink final : public ItemReinforceSideEffectSink {
public:
    std::string last_call;
    int last_rt = 0;
    int last_error_code = 0;
    std::uint16_t last_target_item_idx = 0;
    std::uint16_t last_target_pos = 0;
    int last_jewel_which = 0;
    std::uint16_t last_jewel_unit = 0;
    std::size_t silent_count = 0;
    std::size_t failed_ack_count = 0;
    std::size_t nack_count = 0;

    void silent_success() override {
        last_call = "silent";
        ++silent_count;
    }
    void broadcast_reinforce_failed(
        std::uint16_t target_item_idx, std::uint16_t target_pos,
        int jewel_which, std::uint16_t jewel_unit,
        int original_rt) override {
        last_call = "failed_ack";
        last_target_item_idx = target_item_idx;
        last_target_pos = target_pos;
        last_jewel_which = jewel_which;
        last_jewel_unit = jewel_unit;
        last_rt = original_rt;
        ++failed_ack_count;
    }
    void broadcast_reinforce_nack(
        std::uint16_t target_item_idx, std::uint16_t target_pos,
        int jewel_which, std::uint16_t jewel_unit,
        int original_rt, int error_code) override {
        last_call = "nack";
        last_target_item_idx = target_item_idx;
        last_target_pos = target_pos;
        last_jewel_which = jewel_which;
        last_jewel_unit = jewel_unit;
        last_rt = original_rt;
        last_error_code = error_code;
        ++nack_count;
    }
};

}  // namespace

TEST(ApplyItemReinforceSideEffects, SuccessRtIsSilent) {
    auto plan = item_reinforce_side_effect_plan(
        /*reinforce_rt=*/0, /*target_item_idx=*/100, /*target_pos=*/7,
        /*jewel_which=*/1, /*jewel_unit=*/2);
    EXPECT_FALSE(plan.send_failed_ack);
    EXPECT_FALSE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemReinforceSideEffectKind::SilentSuccess);

    RecordingSink sink;
    auto out = apply_item_reinforce_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.silent_successes, 1u);
    EXPECT_EQ(out.failed_acks, 0u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_FALSE(out.failed_ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_EQ(sink.last_call, "silent");
    EXPECT_EQ(sink.silent_count, 1u);
    EXPECT_EQ(sink.failed_ack_count, 0u);
    EXPECT_EQ(sink.nack_count, 0u);
}

TEST(ApplyItemReinforceSideEffects, FailedRt99EmitsFailedAck) {
    auto plan = item_reinforce_side_effect_plan(
        /*reinforce_rt=*/LEGACY_REINFORCE_FAILED_RT,
        /*target_item_idx=*/100, /*target_pos=*/7,
        /*jewel_which=*/1, /*jewel_unit=*/2);
    EXPECT_TRUE(plan.send_failed_ack);
    EXPECT_FALSE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemReinforceSideEffectKind::BroadcastReinforceFailed);

    RecordingSink sink;
    auto out = apply_item_reinforce_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.failed_acks, 1u);
    EXPECT_TRUE(out.failed_ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_EQ(sink.last_call, "failed_ack");
    EXPECT_EQ(sink.last_target_item_idx, 100u);
    EXPECT_EQ(sink.last_target_pos, 7u);
    EXPECT_EQ(sink.last_jewel_which, 1);
    EXPECT_EQ(sink.last_jewel_unit, 2u);
    EXPECT_EQ(sink.last_rt, LEGACY_REINFORCE_FAILED_RT);
    EXPECT_EQ(sink.failed_ack_count, 1u);
    EXPECT_EQ(sink.silent_count, 0u);
    EXPECT_EQ(sink.nack_count, 0u);
}

TEST(ApplyItemReinforceSideEffects, OtherRtEmitsNackWithRt) {
    auto plan = item_reinforce_side_effect_plan(
        /*reinforce_rt=*/7, /*target_item_idx=*/100, /*target_pos=*/7,
        /*jewel_which=*/1, /*jewel_unit=*/2);
    EXPECT_FALSE(plan.send_failed_ack);
    EXPECT_TRUE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemReinforceSideEffectKind::BroadcastReinforceNack);
    EXPECT_EQ(plan.effects[0].error_code, 7);

    RecordingSink sink;
    auto out = apply_item_reinforce_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.nacks_sent, 1u);
    EXPECT_TRUE(out.nack_flag_consumed);
    EXPECT_FALSE(out.failed_ack_flag_consumed);
    EXPECT_EQ(sink.last_call, "nack");
    EXPECT_EQ(sink.last_rt, 7);
    EXPECT_EQ(sink.last_error_code, 7);
    EXPECT_EQ(sink.nack_count, 1u);
    EXPECT_EQ(sink.silent_count, 0u);
}

TEST(ApplyItemReinforceSideEffects, EmptyPlanIsNoOp) {
    mxh::server::ItemReinforceSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_item_reinforce_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.silent_successes, 0u);
    EXPECT_EQ(out.failed_acks, 0u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_FALSE(out.failed_ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_EQ(sink.last_call, "");
    EXPECT_EQ(sink.silent_count, 0u);
    EXPECT_EQ(sink.failed_ack_count, 0u);
    EXPECT_EQ(sink.nack_count, 0u);
}

TEST(ApplyItemReinforceSideEffects, VariousFailureCodesAllEmitNack) {
    for (int rt : {1, 5, 98, 100, -1}) {
        auto plan = item_reinforce_side_effect_plan(rt, 1, 2, 3, 4);
        EXPECT_TRUE(plan.send_nack);
        ASSERT_EQ(plan.effects.size(), 1u);
        EXPECT_EQ(plan.effects[0].kind,
                  ItemReinforceSideEffectKind::BroadcastReinforceNack);
        EXPECT_EQ(plan.effects[0].error_code, rt);

        RecordingSink sink;
        (void)apply_item_reinforce_side_effects(plan, sink);
        EXPECT_EQ(sink.last_call, "nack");
        EXPECT_EQ(sink.last_rt, rt);
        EXPECT_EQ(sink.last_error_code, rt);
    }
}

TEST(ApplyItemReinforceSideEffects, FieldPassthroughOnFailedAck) {
    auto plan = item_reinforce_side_effect_plan(
        LEGACY_REINFORCE_FAILED_RT, 11, 12, 13, 14);
    RecordingSink sink;
    (void)apply_item_reinforce_side_effects(plan, sink);
    EXPECT_EQ(sink.last_call, "failed_ack");
    EXPECT_EQ(sink.last_target_item_idx, 11u);
    EXPECT_EQ(sink.last_target_pos, 12u);
    EXPECT_EQ(sink.last_jewel_which, 13);
    EXPECT_EQ(sink.last_jewel_unit, 14u);
    EXPECT_EQ(sink.nack_count, 0u);
    EXPECT_EQ(sink.silent_count, 0u);
}
