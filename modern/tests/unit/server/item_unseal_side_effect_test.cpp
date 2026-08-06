// D4.55 ItemUnseal (MP_ITEM_SHOPITEM_UNSEAL_SYN) side-effect
// dispatcher tests.

#include <mxh/server/item_unseal_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(ItemUnsealOutcome, TrueIsSuccess) {
    EXPECT_EQ(classify_item_unseal_outcome(true),
              ItemUnsealOutcome::Success);
}

TEST(ItemUnsealOutcome, FalseIsFailure) {
    EXPECT_EQ(classify_item_unseal_outcome(false),
              ItemUnsealOutcome::Failure);
}

TEST(ItemUnsealPlan, SuccessEmitsSingleAck) {
    auto plan = item_unseal_side_effect_plan(
        /*unseal_ok=*/true, /*dw_data=*/42u);
    EXPECT_TRUE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemUnsealSideEffectKind::BroadcastUnsealAck);
    EXPECT_EQ(plan.effects[0].dw_data, 42u);
    EXPECT_EQ(plan.effects[0].target_pos, 42u);
}

TEST(ItemUnsealPlan, FailureEmitsSingleNack) {
    auto plan = item_unseal_side_effect_plan(
        /*unseal_ok=*/false, /*dw_data=*/7u);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_TRUE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemUnsealSideEffectKind::BroadcastUnsealNack);
    EXPECT_EQ(plan.effects[0].dw_data, 7u);
}

TEST(ItemUnsealPlan, PlanIsIdempotent) {
    auto a = item_unseal_side_effect_plan(true, 1u);
    auto b = item_unseal_side_effect_plan(true, 1u);
    EXPECT_EQ(a.send_ack, b.send_ack);
    EXPECT_EQ(a.send_nack, b.send_nack);
    ASSERT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].dw_data, b.effects[i].dw_data);
    }
}
