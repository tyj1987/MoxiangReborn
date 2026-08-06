// D4.48 ItemUse (MP_ITEM_USE_SYN) side-effect dispatcher tests.

#include <mxh/server/item_use_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(ItemUseOutcome, ZeroRtIsSuccess) {
    EXPECT_EQ(classify_item_use_outcome(0),
              ItemUseOutcome::Success);
}

TEST(ItemUseOutcome, NonZeroRtIsFailure) {
    EXPECT_EQ(classify_item_use_outcome(1),
              ItemUseOutcome::Failure);
    EXPECT_EQ(classify_item_use_outcome(99),
              ItemUseOutcome::Failure);
}

TEST(ItemUsePlan, SuccessEmitsSingleAck) {
    auto plan = item_use_side_effect_plan(
        /*use_rt=*/0, /*target_pos=*/5, /*item_idx=*/100);
    EXPECT_TRUE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemUseSideEffectKind::BroadcastUseAck);
    EXPECT_EQ(plan.effects[0].target_pos, 5u);
    EXPECT_EQ(plan.effects[0].item_idx, 100u);
}

TEST(ItemUsePlan, FailureEmitsSingleNackWithRt) {
    auto plan = item_use_side_effect_plan(
        /*use_rt=*/3, /*target_pos=*/5, /*item_idx=*/100);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_TRUE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemUseSideEffectKind::BroadcastUseNack);
    EXPECT_EQ(plan.effects[0].original_rt, 3);
}

TEST(ItemUsePlan, PlanIsIdempotent) {
    auto a = item_use_side_effect_plan(0, 1, 2);
    auto b = item_use_side_effect_plan(0, 1, 2);
    EXPECT_EQ(a.send_ack, b.send_ack);
    EXPECT_EQ(a.send_nack, b.send_nack);
    ASSERT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].target_pos, b.effects[i].target_pos);
    }
}
