// D4.51 ItemBuy (MP_ITEM_BUY_SYN) side-effect dispatcher tests.

#include <mxh/server/item_buy_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(ItemBuyOutcome, AllGatesPassZeroRtIsSuccess) {
    EXPECT_EQ(classify_item_buy_outcome(0, true, true),
              ItemBuyOutcome::Success);
}

TEST(ItemBuyOutcome, NpcGateFailureOverridesOtherGates) {
    EXPECT_EQ(classify_item_buy_outcome(0, false, true),
              ItemBuyOutcome::NpcGateFailure);
    EXPECT_EQ(classify_item_buy_outcome(7, false, true),
              ItemBuyOutcome::NpcGateFailure);
    EXPECT_EQ(classify_item_buy_outcome(0, false, false),
              ItemBuyOutcome::NpcGateFailure);
}

TEST(ItemBuyOutcome, DemandFailureOverridesBuyRt) {
    EXPECT_EQ(classify_item_buy_outcome(0, true, false),
              ItemBuyOutcome::DemandFailure);
    EXPECT_EQ(classify_item_buy_outcome(7, true, false),
              ItemBuyOutcome::DemandFailure);
}

TEST(ItemBuyOutcome, AllGatesPassNonZeroRtIsBuyFailure) {
    EXPECT_EQ(classify_item_buy_outcome(101, true, true),
              ItemBuyOutcome::BuyFailure);
}

TEST(ItemBuyPlan, SuccessProducesSilentPlan) {
    auto plan = item_buy_side_effect_plan(
        /*buy_rt=*/0, /*npc_gate_ok=*/true, /*demand_ok=*/true,
        /*buy_item_idx=*/100, /*buy_item_num=*/1,
        /*dealer_idx=*/50);
    EXPECT_TRUE(plan.silent_success);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_TRUE(plan.effects.empty());
}

TEST(ItemBuyPlan, NpcGateFailureEmitsNackWithNotExist) {
    auto plan = item_buy_side_effect_plan(
        /*buy_rt=*/0, /*npc_gate_ok=*/false, /*demand_ok=*/true,
        /*buy_item_idx=*/100, /*buy_item_num=*/1,
        /*dealer_idx=*/50);
    EXPECT_FALSE(plan.silent_success);
    EXPECT_TRUE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemBuySideEffectKind::BroadcastNackNpcGate);
    EXPECT_EQ(plan.effects[0].ecode, LEGACY_NOT_EXIST_BUY);
    EXPECT_EQ(plan.effects[0].original_rt, -1);
}

TEST(ItemBuyPlan, DemandFailureEmitsNackWithNoDemandItem) {
    auto plan = item_buy_side_effect_plan(
        /*buy_rt=*/0, /*npc_gate_ok=*/true, /*demand_ok=*/false,
        /*buy_item_idx=*/100, /*buy_item_num=*/1,
        /*dealer_idx=*/50);
    EXPECT_TRUE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemBuySideEffectKind::BroadcastNackDemand);
    EXPECT_EQ(plan.effects[0].ecode, LEGACY_NO_DEMANDITEM_BUY);
}

TEST(ItemBuyPlan, BuyFailureEmitsNackWithRt) {
    auto plan = item_buy_side_effect_plan(
        /*buy_rt=*/LEGACY_NOT_EXIST_BUY, /*npc_gate_ok=*/true,
        /*demand_ok=*/true, /*buy_item_idx=*/100,
        /*buy_item_num=*/1, /*dealer_idx=*/50);
    EXPECT_TRUE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemBuySideEffectKind::BroadcastNackBuyFail);
    EXPECT_EQ(plan.effects[0].ecode, LEGACY_NOT_EXIST_BUY);
    EXPECT_EQ(plan.effects[0].original_rt, LEGACY_NOT_EXIST_BUY);
}

TEST(ItemBuyPlan, PlanIsIdempotent) {
    auto a = item_buy_side_effect_plan(
        7, true, true, 1, 2, 3);
    auto b = item_buy_side_effect_plan(
        7, true, true, 1, 2, 3);
    EXPECT_EQ(a.send_nack, b.send_nack);
    EXPECT_EQ(a.silent_success, b.silent_success);
    ASSERT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].ecode, b.effects[i].ecode);
    }
}
