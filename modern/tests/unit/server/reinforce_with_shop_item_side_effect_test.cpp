// Tests for MP_ITEM_REINFORCE_WITHSHOPITEM_SYN side-effect dispatcher.

#include <mxh/server/reinforce_with_shop_item_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(ReinforceWithShopItemOutcome, RtZeroIsSuccess) {
    ReinforceWithShopItemValidationInput in{};
    in.rt = LEGACY_REINFORCE_EI_TRUE;
    EXPECT_EQ(classify_reinforce_with_shop_item_outcome(in),
              ReinforceWithShopItemOutcome::Success);
}

TEST(ReinforceWithShopItemOutcome, Rt99IsFailedAck) {
    ReinforceWithShopItemValidationInput in{};
    in.rt = LEGACY_REINFORCE_FAILED_RT;
    EXPECT_EQ(classify_reinforce_with_shop_item_outcome(in),
              ReinforceWithShopItemOutcome::FailedAck);
}

TEST(ReinforceWithShopItemOutcome, OtherRtIsNack) {
    for (int rt : {1, 2, 3, 50, 100, 200, -1, -99}) {
        ReinforceWithShopItemValidationInput in{};
        in.rt = rt;
        EXPECT_EQ(classify_reinforce_with_shop_item_outcome(in),
                  ReinforceWithShopItemOutcome::Nack);
    }
}

TEST(ReinforceWithShopItemPlan, SuccessSendsNoMessage) {
    ReinforceWithShopItemValidationInput in{};
    in.rt = LEGACY_REINFORCE_EI_TRUE;
    auto plan = reinforce_with_shop_item_side_effect_plan(in, 100);
    EXPECT_FALSE(plan.send_failed_ack);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(ReinforceWithShopItemPlan, Rt99SendsFailedAck) {
    ReinforceWithShopItemValidationInput in{};
    in.rt = LEGACY_REINFORCE_FAILED_RT;
    auto plan = reinforce_with_shop_item_side_effect_plan(in, 100);
    EXPECT_TRUE(plan.send_failed_ack);
    EXPECT_FALSE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ReinforceWithShopItemSideEffectKind::SendFailedAckToPlayer);
    EXPECT_EQ(plan.effects[0].player_id, 100u);
}

TEST(ReinforceWithShopItemPlan, OtherRtSendsNackWithCode) {
    ReinforceWithShopItemValidationInput in{};
    in.rt = 42;
    auto plan = reinforce_with_shop_item_side_effect_plan(in, 100);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_FALSE(plan.send_failed_ack);
    EXPECT_EQ(plan.nack_error_code, 42);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ReinforceWithShopItemSideEffectKind::SendNackToPlayer);
    EXPECT_EQ(plan.effects[0].player_id, 100u);
    EXPECT_EQ(plan.effects[0].nack_error_code, 42);
}

TEST(ReinforceWithShopItemPlan, NegativeRtSendsNackWithNegativeCode) {
    ReinforceWithShopItemValidationInput in{};
    in.rt = -5;
    auto plan = reinforce_with_shop_item_side_effect_plan(in, 100);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_EQ(plan.nack_error_code, -5);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].nack_error_code, -5);
}

TEST(ReinforceWithShopItemPlan, PlanIsIdempotent) {
    ReinforceWithShopItemValidationInput in{};
    in.rt = LEGACY_REINFORCE_FAILED_RT;
    auto a = reinforce_with_shop_item_side_effect_plan(in, 100);
    auto b = reinforce_with_shop_item_side_effect_plan(in, 100);
    EXPECT_EQ(a.send_failed_ack, b.send_failed_ack);
    EXPECT_EQ(a.send_nack, b.send_nack);
    EXPECT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].player_id, b.effects[i].player_id);
    }
}
