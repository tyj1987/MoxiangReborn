// D4.50 ItemCombine (MP_ITEM_COMBINE_SYN) side-effect dispatcher tests.

#include <mxh/server/item_combine_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(ItemCombineOutcome, ZeroRtIsSuccess) {
    EXPECT_EQ(classify_item_combine_outcome(0),
              ItemCombineOutcome::Success);
}

TEST(ItemCombineOutcome, NonZeroRtIsFailure) {
    EXPECT_EQ(classify_item_combine_outcome(2),
              ItemCombineOutcome::Failure);
    EXPECT_EQ(classify_item_combine_outcome(99),
              ItemCombineOutcome::Failure);
}

TEST(ItemCombinePlan, SuccessEmitsSingleAck) {
    auto plan = item_combine_side_effect_plan(
        /*combine_rt=*/0, /*from_pos=*/10, /*to_pos=*/11,
        /*item_idx=*/100, /*from_dur=*/3, /*to_dur=*/5);
    EXPECT_TRUE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemCombineSideEffectKind::BroadcastCombineAck);
    EXPECT_EQ(plan.effects[0].from_pos, 10u);
    EXPECT_EQ(plan.effects[0].to_pos, 11u);
    EXPECT_EQ(plan.effects[0].item_idx, 100u);
    EXPECT_EQ(plan.effects[0].from_dur, 3u);
    EXPECT_EQ(plan.effects[0].to_dur, 5u);
}

TEST(ItemCombinePlan, FailureEmitsSingleNack) {
    auto plan = item_combine_side_effect_plan(
        /*combine_rt=*/2, /*from_pos=*/10, /*to_pos=*/11,
        /*item_idx=*/100, /*from_dur=*/3, /*to_dur=*/5);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_TRUE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemCombineSideEffectKind::BroadcastErrorNack);
    EXPECT_EQ(plan.effects[0].original_rt, 2);
}

TEST(ItemCombinePlan, PlanIsIdempotent) {
    auto a = item_combine_side_effect_plan(0, 1, 2, 3, 4, 5);
    auto b = item_combine_side_effect_plan(0, 1, 2, 3, 4, 5);
    EXPECT_EQ(a.send_ack, b.send_ack);
    EXPECT_EQ(a.send_nack, b.send_nack);
    ASSERT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].from_pos, b.effects[i].from_pos);
        EXPECT_EQ(a.effects[i].to_pos, b.effects[i].to_pos);
    }
}
