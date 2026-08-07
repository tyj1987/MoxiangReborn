// shout_pass_through_side_effect_runtime_test.cpp
//
// Verifies apply_shout_pass_through_side_effects() (the runtime
// orchestrator for the CItemManager::MP_ITEM_SHOPITEM_SHOUT_ACK/_NACK
// pass-through chain) walks the data-plane plan and dispatches the
// forward entry: protocol rewrite for the ACK variant / plain
// passthrough for the NACK variant / empty plan when the player is
// missing.

#include <mxh/server/shout_pass_through_side_effect.hpp>
#include <mxh/server/shout_pass_through_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {

using mxh::server::ShoutPassThroughSideEffectKind;
using mxh::server::ShoutPassThroughSideEffectSink;
using mxh::server::ShoutPassThroughValidationInput;
using mxh::server::ShoutPassThroughVariant;
using mxh::server::apply_shout_pass_through_side_effects;
using mxh::server::shout_pass_through_side_effect_plan;

class RecordingSink final : public ShoutPassThroughSideEffectSink {
public:
    std::vector<std::string> calls;
    std::uint32_t last_player_id = 0;
    ShoutPassThroughVariant last_variant = ShoutPassThroughVariant::SendAck;
    bool last_rewrite = false;
    std::size_t forward_count = 0;

    void forward_to_player(std::uint32_t player_id,
                           ShoutPassThroughVariant variant,
                           bool rewrite_to_sendack) override {
        calls.push_back("forward");
        last_player_id = player_id;
        last_variant = variant;
        last_rewrite = rewrite_to_sendack;
        ++forward_count;
    }
};

}  // namespace

TEST(ApplyShoutPassThroughSideEffects, SendAckEmitsForwardWithRewrite) {
    ShoutPassThroughValidationInput in;
    in.variant = ShoutPassThroughVariant::SendAck;
    in.player_found = true;
    in.player_id = 0x000A000Bu;
    auto plan = shout_pass_through_side_effect_plan(in);
    EXPECT_TRUE(plan.forward);
    EXPECT_TRUE(plan.rewrite_protocol);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ShoutPassThroughSideEffectKind::ForwardToPlayer);
    EXPECT_EQ(plan.effects[0].variant, ShoutPassThroughVariant::SendAck);
    EXPECT_EQ(plan.effects[0].rewrite_to_sendack, true);

    RecordingSink sink;
    auto out = apply_shout_pass_through_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.forwards_sent, 1u);
    EXPECT_TRUE(out.forward_flag_consumed);
    EXPECT_TRUE(out.rewrite_flag_consumed);
    EXPECT_EQ(sink.calls, std::vector<std::string>({"forward"}));
    EXPECT_EQ(sink.last_player_id, 0x000A000Bu);
    EXPECT_EQ(sink.last_variant, ShoutPassThroughVariant::SendAck);
    EXPECT_EQ(sink.last_rewrite, true);
}

TEST(ApplyShoutPassThroughSideEffects, SendNackEmitsForwardWithoutRewrite) {
    ShoutPassThroughValidationInput in;
    in.variant = ShoutPassThroughVariant::SendNack;
    in.player_found = true;
    in.player_id = 42;
    auto plan = shout_pass_through_side_effect_plan(in);
    EXPECT_TRUE(plan.forward);
    EXPECT_FALSE(plan.rewrite_protocol);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].variant, ShoutPassThroughVariant::SendNack);
    EXPECT_EQ(plan.effects[0].rewrite_to_sendack, false);

    RecordingSink sink;
    auto out = apply_shout_pass_through_side_effects(plan, sink);
    EXPECT_EQ(out.forwards_sent, 1u);
    EXPECT_TRUE(out.forward_flag_consumed);
    EXPECT_FALSE(out.rewrite_flag_consumed);
    EXPECT_EQ(sink.last_variant, ShoutPassThroughVariant::SendNack);
    EXPECT_EQ(sink.last_rewrite, false);
}

TEST(ApplyShoutPassThroughSideEffects, NoPlayerEmitsEmptyPlanForBothVariants) {
    for (auto variant : {ShoutPassThroughVariant::SendAck,
                         ShoutPassThroughVariant::SendNack}) {
        ShoutPassThroughValidationInput in;
        in.variant = variant;
        in.player_found = false;
        in.player_id = 1;
        auto plan = shout_pass_through_side_effect_plan(in);
        EXPECT_FALSE(plan.forward);
        EXPECT_FALSE(plan.rewrite_protocol);
        EXPECT_TRUE(plan.effects.empty());

        RecordingSink sink;
        auto out = apply_shout_pass_through_side_effects(plan, sink);
        EXPECT_EQ(out.effects_applied, 0u);
        EXPECT_EQ(out.forwards_sent, 0u);
        EXPECT_TRUE(sink.calls.empty());
    }
}

TEST(ApplyShoutPassThroughSideEffects, PlayerIdBoundaryPassthrough) {
    ShoutPassThroughValidationInput in;
    in.variant = ShoutPassThroughVariant::SendAck;
    in.player_found = true;
    in.player_id = 0xFFFFFFFFu;
    auto plan = shout_pass_through_side_effect_plan(in);
    RecordingSink sink;
    (void)apply_shout_pass_through_side_effects(plan, sink);
    EXPECT_EQ(sink.last_player_id, 0xFFFFFFFFu);
    EXPECT_EQ(sink.last_rewrite, true);
}

TEST(ApplyShoutPassThroughSideEffects, EmptyPlanIsNoOp) {
    mxh::server::ShoutPassThroughSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_shout_pass_through_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.forwards_sent, 0u);
    EXPECT_FALSE(out.forward_flag_consumed);
    EXPECT_FALSE(out.rewrite_flag_consumed);
    EXPECT_TRUE(sink.calls.empty());
}

TEST(ApplyShoutPassThroughSideEffects, ForwardDoesNotTouchRewriteState) {
    ShoutPassThroughValidationInput nack_in;
    nack_in.variant = ShoutPassThroughVariant::SendNack;
    nack_in.player_found = true;
    auto nack_plan = shout_pass_through_side_effect_plan(nack_in);
    RecordingSink nack_sink;
    auto nack_out =
        apply_shout_pass_through_side_effects(nack_plan, nack_sink);
    EXPECT_EQ(nack_out.forwards_sent, 1u);
    EXPECT_FALSE(nack_out.rewrite_flag_consumed);
    EXPECT_EQ(nack_sink.last_rewrite, false);

    ShoutPassThroughValidationInput ack_in;
    ack_in.variant = ShoutPassThroughVariant::SendAck;
    ack_in.player_found = true;
    auto ack_plan = shout_pass_through_side_effect_plan(ack_in);
    RecordingSink ack_sink;
    auto ack_out =
        apply_shout_pass_through_side_effects(ack_plan, ack_sink);
    EXPECT_EQ(ack_out.forwards_sent, 1u);
    EXPECT_TRUE(ack_out.rewrite_flag_consumed);
    EXPECT_EQ(ack_sink.last_rewrite, true);
}
