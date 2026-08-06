// D4.47 ItemSell (MP_ITEM_SELL_SYN) side-effect dispatcher tests.

#include <mxh/server/item_sell_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(ItemSellOutcome, ZeroRtWithValidNpcIsSuccess) {
    EXPECT_EQ(classify_item_sell_outcome(0, true),
              ItemSellOutcome::Success);
}

TEST(ItemSellOutcome, NonZeroRtIsFailure) {
    EXPECT_EQ(classify_item_sell_outcome(101, true),
              ItemSellOutcome::Failure);
    EXPECT_EQ(classify_item_sell_outcome(102, true),
              ItemSellOutcome::Failure);
}

TEST(ItemSellOutcome, NpcGateFailureOverridesRt) {
    EXPECT_EQ(classify_item_sell_outcome(0, false),
              ItemSellOutcome::NpcGateFailure);
    EXPECT_EQ(classify_item_sell_outcome(7, false),
              ItemSellOutcome::NpcGateFailure);
}

TEST(ItemSellPlan, SuccessEmitsSingleAck) {
    auto plan = item_sell_side_effect_plan(
        /*sell_rt=*/0, /*npc_gate_ok=*/true,
        /*target_pos=*/10, /*item_idx=*/100, /*item_num=*/1,
        /*dealer_idx=*/50);
    EXPECT_TRUE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemSellSideEffectKind::BroadcastSellAck);
    EXPECT_EQ(plan.effects[0].target_pos, 10u);
    EXPECT_EQ(plan.effects[0].item_idx, 100u);
    EXPECT_EQ(plan.effects[0].dealer_idx, 50u);
}

TEST(ItemSellPlan, FailureEmitsSingleNackWithRt) {
    auto plan = item_sell_side_effect_plan(
        /*sell_rt=*/LEGACY_NOT_EXIST, /*npc_gate_ok=*/true,
        /*target_pos=*/10, /*item_idx=*/100, /*item_num=*/1,
        /*dealer_idx=*/50);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_TRUE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemSellSideEffectKind::BroadcastSellNack);
    EXPECT_EQ(plan.effects[0].ecode, LEGACY_NOT_EXIST);
}

TEST(ItemSellPlan, NpcGateFailureEmitsNackWithNotExistCode) {
    auto plan = item_sell_side_effect_plan(
        /*sell_rt=*/0, /*npc_gate_ok=*/false,
        /*target_pos=*/10, /*item_idx=*/100, /*item_num=*/1,
        /*dealer_idx=*/50);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_TRUE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemSellSideEffectKind::BroadcastSellNack);
    EXPECT_EQ(plan.effects[0].ecode, LEGACY_NOT_EXIST);
    EXPECT_EQ(plan.effects[0].original_rt, -1);  // sentinel: no SellItem call
}

TEST(ItemSellPlan, PlanIsIdempotent) {
    auto a = item_sell_side_effect_plan(0, true, 1, 2, 3, 4);
    auto b = item_sell_side_effect_plan(0, true, 1, 2, 3, 4);
    EXPECT_EQ(a.send_ack, b.send_ack);
    EXPECT_EQ(a.send_nack, b.send_nack);
    ASSERT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].target_pos, b.effects[i].target_pos);
    }
}
