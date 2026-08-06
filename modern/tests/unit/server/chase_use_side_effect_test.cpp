// D4.71 ChaseUse (MP_ITEM_SHOPITEM_CHASEUSE_SYN) side-effect
// dispatcher tests.

#include <mxh/server/chase_use_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

ChaseUseValidationInput ok() {
    ChaseUseValidationInput in{};
    in.player_found = true;
    in.has_using_item = true;
    return in;
}

TEST(ChaseUseOutcome, HasUsingItemIsAck) {
    auto in = ok();
    EXPECT_EQ(classify_chase_use_outcome(in),
              ChaseUseOutcome::Ack);
}

TEST(ChaseUseOutcome, NoUsingItemIsNack) {
    auto in = ok();
    in.has_using_item = false;
    EXPECT_EQ(classify_chase_use_outcome(in),
              ChaseUseOutcome::Nack);
}

TEST(ChaseUseOutcome, NoPlayerTakesPrecedence) {
    auto in = ok();
    in.player_found = false;
    EXPECT_EQ(classify_chase_use_outcome(in),
              ChaseUseOutcome::NoPlayer);
}

TEST(ChaseUsePlan, AckEmitsChaseUseAck) {
    auto in = ok();
    auto plan = chase_use_side_effect_plan(
        in, /*item_idx=*/100, /*item_pos=*/5);
    EXPECT_TRUE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ChaseUseSideEffectKind::BroadcastChaseUseAck);
    EXPECT_EQ(plan.effects[0].item_idx, 100u);
    EXPECT_EQ(plan.effects[0].item_pos, 5u);
}

TEST(ChaseUsePlan, NackEmitsChaseUseNack) {
    auto in = ok();
    in.has_using_item = false;
    auto plan = chase_use_side_effect_plan(in, 1, 2);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_EQ(plan.effects[0].kind,
              ChaseUseSideEffectKind::BroadcastChaseUseNack);
}

TEST(ChaseUsePlan, NoPlayerEmitsEmptyPlan) {
    auto in = ok();
    in.player_found = false;
    auto plan = chase_use_side_effect_plan(in, 1, 2);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(ChaseUsePlan, PlanIsIdempotent) {
    auto in = ok();
    auto a = chase_use_side_effect_plan(in, 1, 2);
    auto b = chase_use_side_effect_plan(in, 1, 2);
    EXPECT_EQ(a.send_ack, b.send_ack);
    EXPECT_EQ(a.send_nack, b.send_nack);
    ASSERT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].item_idx, b.effects[i].item_idx);
    }
}
