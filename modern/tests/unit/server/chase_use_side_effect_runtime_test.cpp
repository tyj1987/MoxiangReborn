// chase_use_side_effect_runtime_test.cpp
//
// Verifies apply_chase_use_side_effects() (the runtime orchestrator
// for the CItemManager::MP_ITEM_SHOPITEM_CHASEUSE_SYN side-effect
// chain) walks the data-plane plan and dispatches the single entry:
// ACK when the chase item is equipped / NACK otherwise / no-op when
// the player is missing.

#include <mxh/server/chase_use_side_effect.hpp>
#include <mxh/server/chase_use_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>

namespace {

using mxh::server::ChaseUseSideEffectKind;
using mxh::server::ChaseUseSideEffectSink;
using mxh::server::apply_chase_use_side_effects;
using mxh::server::chase_use_side_effect_plan;

class RecordingSink final : public ChaseUseSideEffectSink {
public:
    std::string last_call;
    std::uint16_t last_item_idx = 0;
    std::uint16_t last_item_pos = 0;
    std::size_t ack_count = 0;
    std::size_t nack_count = 0;

    void broadcast_chase_use_ack(std::uint16_t item_idx,
                                 std::uint16_t item_pos) override {
        last_call = "ack";
        last_item_idx = item_idx;
        last_item_pos = item_pos;
        ++ack_count;
    }
    void broadcast_chase_use_nack(std::uint16_t item_idx,
                                  std::uint16_t item_pos) override {
        last_call = "nack";
        last_item_idx = item_idx;
        last_item_pos = item_pos;
        ++nack_count;
    }
};

}  // namespace

TEST(ApplyChaseUseSideEffects, HasUsingItemEmitsAck) {
    mxh::server::ChaseUseValidationInput in;
    in.player_found = true;
    in.has_using_item = true;
    auto plan = chase_use_side_effect_plan(
        in, /*item_idx=*/100, /*item_pos=*/7);
    EXPECT_TRUE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ChaseUseSideEffectKind::BroadcastChaseUseAck);

    RecordingSink sink;
    auto out = apply_chase_use_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.acks_sent, 1u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_TRUE(out.ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_EQ(sink.last_call, "ack");
    EXPECT_EQ(sink.last_item_idx, 100u);
    EXPECT_EQ(sink.last_item_pos, 7u);
    EXPECT_EQ(sink.ack_count, 1u);
    EXPECT_EQ(sink.nack_count, 0u);
}

TEST(ApplyChaseUseSideEffects, NoUsingItemEmitsNack) {
    mxh::server::ChaseUseValidationInput in;
    in.player_found = true;
    in.has_using_item = false;
    auto plan = chase_use_side_effect_plan(in, 100, 7);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_TRUE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ChaseUseSideEffectKind::BroadcastChaseUseNack);

    RecordingSink sink;
    auto out = apply_chase_use_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.acks_sent, 0u);
    EXPECT_EQ(out.nacks_sent, 1u);
    EXPECT_FALSE(out.ack_flag_consumed);
    EXPECT_TRUE(out.nack_flag_consumed);
    EXPECT_EQ(sink.last_call, "nack");
    EXPECT_EQ(sink.last_item_idx, 100u);
    EXPECT_EQ(sink.last_item_pos, 7u);
    EXPECT_EQ(sink.ack_count, 0u);
    EXPECT_EQ(sink.nack_count, 1u);
}

TEST(ApplyChaseUseSideEffects, NoPlayerIsNoOp) {
    mxh::server::ChaseUseValidationInput in;
    in.player_found = false;
    in.has_using_item = true;
    auto plan = chase_use_side_effect_plan(in, 100, 7);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_TRUE(plan.effects.empty());

    RecordingSink sink;
    auto out = apply_chase_use_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.acks_sent, 0u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_FALSE(out.ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_EQ(sink.last_call, "");
    EXPECT_EQ(sink.ack_count, 0u);
    EXPECT_EQ(sink.nack_count, 0u);
}

TEST(ApplyChaseUseSideEffects, EmptyPlanIsNoOp) {
    mxh::server::ChaseUseSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_chase_use_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.acks_sent, 0u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_FALSE(out.ack_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_EQ(sink.last_call, "");
    EXPECT_EQ(sink.ack_count, 0u);
    EXPECT_EQ(sink.nack_count, 0u);
}

TEST(ApplyChaseUseSideEffects, NoPlayerOverridesUsingItem) {
    mxh::server::ChaseUseValidationInput in;
    in.player_found = false;
    in.has_using_item = false;
    auto plan = chase_use_side_effect_plan(in, 100, 7);
    EXPECT_TRUE(plan.effects.empty());

    RecordingSink sink;
    (void)apply_chase_use_side_effects(plan, sink);
    EXPECT_EQ(sink.ack_count, 0u);
    EXPECT_EQ(sink.nack_count, 0u);
}

TEST(ApplyChaseUseSideEffects, FieldPassthroughOnNack) {
    mxh::server::ChaseUseValidationInput in;
    in.player_found = true;
    in.has_using_item = false;
    auto plan = chase_use_side_effect_plan(in, 21, 22);
    RecordingSink sink;
    (void)apply_chase_use_side_effects(plan, sink);
    EXPECT_EQ(sink.last_call, "nack");
    EXPECT_EQ(sink.last_item_idx, 21u);
    EXPECT_EQ(sink.last_item_pos, 22u);
    EXPECT_EQ(sink.ack_count, 0u);
}
