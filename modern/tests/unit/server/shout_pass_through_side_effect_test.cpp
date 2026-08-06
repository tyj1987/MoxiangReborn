// Tests for MP_ITEM_SHOPITEM_SHOUT_ACK/_NACK pass-through dispatcher.

#include <mxh/server/shout_pass_through_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

ShoutPassThroughValidationInput ack_input(bool found) {
    ShoutPassThroughValidationInput in{};
    in.variant = ShoutPassThroughVariant::SendAck;
    in.player_found = found;
    in.player_id = 100;
    return in;
}

ShoutPassThroughValidationInput nack_input(bool found) {
    ShoutPassThroughValidationInput in{};
    in.variant = ShoutPassThroughVariant::SendNack;
    in.player_found = found;
    in.player_id = 200;
    return in;
}

TEST(ShoutPassThroughOutcome, AckForwardedWhenPlayerFound) {
    EXPECT_EQ(classify_shout_pass_through_outcome(ack_input(true)),
              ShoutPassThroughOutcome::Forwarded);
}

TEST(ShoutPassThroughOutcome, AckNoPlayerWhenNotFound) {
    EXPECT_EQ(classify_shout_pass_through_outcome(ack_input(false)),
              ShoutPassThroughOutcome::NoPlayer);
}

TEST(ShoutPassThroughOutcome, NackForwardedWhenPlayerFound) {
    EXPECT_EQ(classify_shout_pass_through_outcome(nack_input(true)),
              ShoutPassThroughOutcome::Forwarded);
}

TEST(ShoutPassThroughOutcome, NackNoPlayerWhenNotFound) {
    EXPECT_EQ(classify_shout_pass_through_outcome(nack_input(false)),
              ShoutPassThroughOutcome::NoPlayer);
}

TEST(ShoutPassThroughPlan, AckForwardedRewritesToSendack) {
    auto in = ack_input(true);
    auto plan = shout_pass_through_side_effect_plan(in);
    EXPECT_TRUE(plan.forward);
    EXPECT_TRUE(plan.rewrite_protocol);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ShoutPassThroughSideEffectKind::ForwardToPlayer);
    EXPECT_EQ(plan.effects[0].variant,
              ShoutPassThroughVariant::SendAck);
    EXPECT_EQ(plan.effects[0].player_id, 100u);
    EXPECT_TRUE(plan.effects[0].rewrite_to_sendack);
}

TEST(ShoutPassThroughPlan, NackForwardedDoesNotRewrite) {
    auto in = nack_input(true);
    auto plan = shout_pass_through_side_effect_plan(in);
    EXPECT_TRUE(plan.forward);
    EXPECT_FALSE(plan.rewrite_protocol);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].variant,
              ShoutPassThroughVariant::SendNack);
    EXPECT_EQ(plan.effects[0].player_id, 200u);
    EXPECT_FALSE(plan.effects[0].rewrite_to_sendack);
}

TEST(ShoutPassThroughPlan, AckNoPlayerEmitsEmptyPlan) {
    auto in = ack_input(false);
    auto plan = shout_pass_through_side_effect_plan(in);
    EXPECT_FALSE(plan.forward);
    EXPECT_FALSE(plan.rewrite_protocol);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(ShoutPassThroughPlan, NackNoPlayerEmitsEmptyPlan) {
    auto in = nack_input(false);
    auto plan = shout_pass_through_side_effect_plan(in);
    EXPECT_FALSE(plan.forward);
    EXPECT_FALSE(plan.rewrite_protocol);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(ShoutPassThroughPlan, PlanIsIdempotent) {
    auto in = ack_input(true);
    auto a = shout_pass_through_side_effect_plan(in);
    auto b = shout_pass_through_side_effect_plan(in);
    EXPECT_EQ(a.forward, b.forward);
    EXPECT_EQ(a.rewrite_protocol, b.rewrite_protocol);
    EXPECT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].variant, b.effects[i].variant);
        EXPECT_EQ(a.effects[i].player_id, b.effects[i].player_id);
    }
}
