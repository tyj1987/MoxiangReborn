// D4.52 ItemMix (MP_ITEM_MIX_SYN) side-effect dispatcher tests.

#include <mxh/server/item_mix_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(ItemMixOutcome, ZeroRtIsSuccess) {
    EXPECT_EQ(classify_item_mix_outcome(0),
              ItemMixOutcome::Success);
}

TEST(ItemMixOutcome, BigFailRtIsBigFail) {
    EXPECT_EQ(classify_item_mix_outcome(LEGACY_MIX_RT_BIGFAIL),
              ItemMixOutcome::BigFail);
    EXPECT_EQ(classify_item_mix_outcome(1000),
              ItemMixOutcome::BigFail);
}

TEST(ItemMixOutcome, FailRtIsFail) {
    EXPECT_EQ(classify_item_mix_outcome(LEGACY_MIX_RT_FAIL),
              ItemMixOutcome::Fail);
    EXPECT_EQ(classify_item_mix_outcome(1001),
              ItemMixOutcome::Fail);
}

TEST(ItemMixOutcome, MsgRtRangeIsMsg) {
    EXPECT_EQ(classify_item_mix_outcome(20),
              ItemMixOutcome::Msg);
    EXPECT_EQ(classify_item_mix_outcome(21),
              ItemMixOutcome::Msg);
    EXPECT_EQ(classify_item_mix_outcome(22),
              ItemMixOutcome::Msg);
    EXPECT_EQ(classify_item_mix_outcome(23),
              ItemMixOutcome::Msg);
}

TEST(ItemMixOutcome, OtherRtIsErrorNack) {
    EXPECT_EQ(classify_item_mix_outcome(2),
              ItemMixOutcome::ErrorNack);
    EXPECT_EQ(classify_item_mix_outcome(99),
              ItemMixOutcome::ErrorNack);
    EXPECT_EQ(classify_item_mix_outcome(19),
              ItemMixOutcome::ErrorNack);
    EXPECT_EQ(classify_item_mix_outcome(24),
              ItemMixOutcome::ErrorNack);
}

TEST(ItemMixPlan, SuccessEmitsSingleSuccessAck) {
    auto plan = item_mix_side_effect_plan(
        /*mix_rt=*/0, /*basic_item_idx=*/100, /*basic_item_pos=*/10,
        /*result_index=*/0, /*material_num=*/2,
        /*shop_item_idx=*/0, /*shop_item_pos=*/0);
    EXPECT_TRUE(plan.send_ack);
    EXPECT_FALSE(plan.send_msg);
    EXPECT_FALSE(plan.send_error_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemMixSideEffectKind::BroadcastSuccessAck);
}

TEST(ItemMixPlan, BigFailEmitsBigFailAck) {
    auto plan = item_mix_side_effect_plan(
        /*mix_rt=*/LEGACY_MIX_RT_BIGFAIL, /*basic_item_idx=*/100,
        /*basic_item_pos=*/10, /*result_index=*/0,
        /*material_num=*/2, /*shop_item_idx=*/0,
        /*shop_item_pos=*/0);
    EXPECT_TRUE(plan.send_ack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemMixSideEffectKind::BroadcastBigFailAck);
}

TEST(ItemMixPlan, FailEmitsFailAck) {
    auto plan = item_mix_side_effect_plan(
        /*mix_rt=*/LEGACY_MIX_RT_FAIL, /*basic_item_idx=*/100,
        /*basic_item_pos=*/10, /*result_index=*/0,
        /*material_num=*/2, /*shop_item_idx=*/0,
        /*shop_item_pos=*/0);
    EXPECT_TRUE(plan.send_ack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemMixSideEffectKind::BroadcastFailAck);
}

TEST(ItemMixPlan, MsgRtEmitsDword2Msg) {
    auto plan = item_mix_side_effect_plan(
        /*mix_rt=*/21, /*basic_item_idx=*/100, /*basic_item_pos=*/42,
        /*result_index=*/0, /*material_num=*/2,
        /*shop_item_idx=*/0, /*shop_item_pos=*/0);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_TRUE(plan.send_msg);
    EXPECT_FALSE(plan.send_error_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemMixSideEffectKind::BroadcastMixMsg);
    EXPECT_EQ(plan.effects[0].ecode, 21);
    EXPECT_EQ(plan.effects[0].basic_item_pos, 42u);
}

TEST(ItemMixPlan, DefaultEmitsErrorNack) {
    auto plan = item_mix_side_effect_plan(
        /*mix_rt=*/99, /*basic_item_idx=*/100, /*basic_item_pos=*/10,
        /*result_index=*/0, /*material_num=*/2,
        /*shop_item_idx=*/0, /*shop_item_pos=*/0);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_FALSE(plan.send_msg);
    EXPECT_TRUE(plan.send_error_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemMixSideEffectKind::BroadcastErrorNack);
    EXPECT_EQ(plan.effects[0].ecode, 99);
}

TEST(ItemMixPlan, PlanIsIdempotent) {
    auto a = item_mix_side_effect_plan(0, 1, 2, 3, 4, 5, 6);
    auto b = item_mix_side_effect_plan(0, 1, 2, 3, 4, 5, 6);
    EXPECT_EQ(a.send_ack, b.send_ack);
    EXPECT_EQ(a.send_msg, b.send_msg);
    EXPECT_EQ(a.send_error_nack, b.send_error_nack);
    ASSERT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].ecode, b.effects[i].ecode);
    }
}
