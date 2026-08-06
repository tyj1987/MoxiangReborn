// D4.75 ItemReinforce (MP_ITEM_REINFORCE_SYN) side-effect
// dispatcher tests.

#include <mxh/server/item_reinforce_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(ItemReinforceOutcome, ZeroRtIsSilentSuccess) {
    EXPECT_EQ(classify_item_reinforce_outcome(0),
              ItemReinforceOutcome::SilentSuccess);
}

TEST(ItemReinforceOutcome, Rt99IsFailedAck) {
    EXPECT_EQ(classify_item_reinforce_outcome(99),
              ItemReinforceOutcome::FailedAck);
}

TEST(ItemReinforceOutcome, OtherRtIsFailureNack) {
    EXPECT_EQ(classify_item_reinforce_outcome(1),
              ItemReinforceOutcome::FailureNack);
    EXPECT_EQ(classify_item_reinforce_outcome(50),
              ItemReinforceOutcome::FailureNack);
}

TEST(ItemReinforcePlan, ZeroRtEmitsSilent) {
    auto plan = item_reinforce_side_effect_plan(
        /*rt=*/0,
        /*target_item_idx=*/1, /*target_pos=*/2,
        /*jewel_which=*/3, /*jewel_unit=*/4);
    EXPECT_FALSE(plan.send_failed_ack);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_EQ(plan.error_code, 0);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemReinforceSideEffectKind::SilentSuccess);
}

TEST(ItemReinforcePlan, Rt99EmitsFailedAck) {
    auto plan = item_reinforce_side_effect_plan(
        /*rt=*/99,
        /*target_item_idx=*/1, /*target_pos=*/2,
        /*jewel_which=*/3, /*jewel_unit=*/4);
    EXPECT_TRUE(plan.send_failed_ack);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_EQ(plan.effects[0].kind,
              ItemReinforceSideEffectKind::BroadcastReinforceFailed);
}

TEST(ItemReinforcePlan, OtherRtEmitsNack) {
    auto plan = item_reinforce_side_effect_plan(
        /*rt=*/7,
        /*target_item_idx=*/1, /*target_pos=*/2,
        /*jewel_which=*/3, /*jewel_unit=*/4);
    EXPECT_FALSE(plan.send_failed_ack);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_EQ(plan.error_code, 7);
    EXPECT_EQ(plan.effects[0].kind,
              ItemReinforceSideEffectKind::BroadcastReinforceNack);
    EXPECT_EQ(plan.effects[0].error_code, 7);
}

TEST(ItemReinforcePlan, PreservesTargetAndJewel) {
    auto plan = item_reinforce_side_effect_plan(0, 11, 22, 33, 44);
    EXPECT_EQ(plan.effects[0].target_item_idx, 11u);
    EXPECT_EQ(plan.effects[0].target_pos, 22u);
    EXPECT_EQ(plan.effects[0].jewel_which, 33);
    EXPECT_EQ(plan.effects[0].jewel_unit, 44u);
}

TEST(ItemReinforcePlan, PlanIsIdempotent) {
    auto a = item_reinforce_side_effect_plan(0, 1, 2, 3, 4);
    auto b = item_reinforce_side_effect_plan(0, 1, 2, 3, 4);
    EXPECT_EQ(a.send_failed_ack, b.send_failed_ack);
    EXPECT_EQ(a.send_nack, b.send_nack);
    EXPECT_EQ(a.error_code, b.error_code);
    ASSERT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].target_item_idx, b.effects[i].target_item_idx);
        EXPECT_EQ(a.effects[i].target_pos, b.effects[i].target_pos);
    }
}
