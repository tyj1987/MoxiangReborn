// D4.53 ItemDissolution (MP_ITEM_DISSOLUTION_SYN) side-effect
// dispatcher tests.

#include <mxh/server/item_dissolution_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(ItemDissolutionOutcome, ZeroRtIsSuccess) {
    EXPECT_EQ(classify_item_dissolution_outcome(0),
              ItemDissolutionOutcome::Success);
}

TEST(ItemDissolutionOutcome, NonZeroRtIsFailure) {
    EXPECT_EQ(classify_item_dissolution_outcome(1),
              ItemDissolutionOutcome::Failure);
    EXPECT_EQ(classify_item_dissolution_outcome(99),
              ItemDissolutionOutcome::Failure);
}

TEST(ItemDissolutionPlan, SuccessEmitsSingleAck) {
    auto plan = item_dissolution_side_effect_plan(
        /*dissolution_rt=*/0, /*item_idx=*/100, /*item_pos=*/10);
    EXPECT_TRUE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemDissolutionSideEffectKind::BroadcastDissolutionAck);
    EXPECT_EQ(plan.effects[0].item_idx, 100u);
    EXPECT_EQ(plan.effects[0].item_pos, 10u);
}

TEST(ItemDissolutionPlan, FailureEmitsSingleNack) {
    auto plan = item_dissolution_side_effect_plan(
        /*dissolution_rt=*/1, /*item_idx=*/100, /*item_pos=*/10);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_TRUE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemDissolutionSideEffectKind::BroadcastDissolutionNack);
    EXPECT_EQ(plan.effects[0].original_rt, 1);
}

TEST(ItemDissolutionPlan, PlanIsIdempotent) {
    auto a = item_dissolution_side_effect_plan(0, 1, 2);
    auto b = item_dissolution_side_effect_plan(0, 1, 2);
    EXPECT_EQ(a.send_ack, b.send_ack);
    EXPECT_EQ(a.send_nack, b.send_nack);
    ASSERT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].item_idx, b.effects[i].item_idx);
    }
}
