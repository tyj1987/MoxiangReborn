// D4.49 ItemDivide (MP_ITEM_DIVIDE_SYN) side-effect dispatcher tests.

#include <mxh/server/item_divide_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(ItemDivideOutcome, ZeroRtIsSuccess) {
    EXPECT_EQ(classify_item_divide_outcome(0),
              ItemDivideOutcome::Success);
}

TEST(ItemDivideOutcome, NonZeroRtIsFailure) {
    EXPECT_EQ(classify_item_divide_outcome(1),
              ItemDivideOutcome::Failure);
    EXPECT_EQ(classify_item_divide_outcome(99),
              ItemDivideOutcome::Failure);
}

TEST(ItemDividePlan, SuccessProducesSilentPlan) {
    auto plan = item_divide_side_effect_plan(
        /*divide_rt=*/0, /*from_pos=*/10, /*to_pos=*/11,
        /*item_idx=*/100, /*from_dur=*/5, /*to_dur=*/5);
    EXPECT_TRUE(plan.silent_success);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_TRUE(plan.effects.empty());
}

TEST(ItemDividePlan, FailureEmitsSingleNackWithECode) {
    auto plan = item_divide_side_effect_plan(
        /*divide_rt=*/2, /*from_pos=*/10, /*to_pos=*/11,
        /*item_idx=*/100, /*from_dur=*/5, /*to_dur=*/5);
    EXPECT_FALSE(plan.silent_success);
    EXPECT_TRUE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemDivideSideEffectKind::BroadcastErrorNack);
    EXPECT_EQ(plan.effects[0].from_pos, 10u);
    EXPECT_EQ(plan.effects[0].to_pos, 11u);
    EXPECT_EQ(plan.effects[0].item_idx, 100u);
    EXPECT_EQ(plan.effects[0].from_dur, 5u);
    EXPECT_EQ(plan.effects[0].to_dur, 5u);
    EXPECT_EQ(plan.effects[0].original_rt, 2);
}

TEST(ItemDividePlan, PlanIsIdempotent) {
    auto a = item_divide_side_effect_plan(3, 1, 2, 3, 4, 5);
    auto b = item_divide_side_effect_plan(3, 1, 2, 3, 4, 5);
    EXPECT_EQ(a.send_nack, b.send_nack);
    EXPECT_EQ(a.silent_success, b.silent_success);
    ASSERT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].from_pos, b.effects[i].from_pos);
        EXPECT_EQ(a.effects[i].to_pos, b.effects[i].to_pos);
        EXPECT_EQ(a.effects[i].original_rt, b.effects[i].original_rt);
    }
}

TEST(ItemDivideConstants, LegacyEnumValues) {
    EXPECT_EQ(LEGACY_EITEMUSE_DIVIDE, 4);
    EXPECT_EQ(LEGACY_MP_ITEM_ERROR_NACK_DIVIDE, 99u);
}
