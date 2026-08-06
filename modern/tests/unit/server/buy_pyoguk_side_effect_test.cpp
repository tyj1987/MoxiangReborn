// D4.45 BuyPyogukSyn (MP_PYOGUK_BUY_SYN) data-plane + side-effect
// dispatcher tests.

#include <mxh/server/buy_pyoguk_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(BuyPyogukOutcome, UnderTabCapWithEnoughMoneyIsSuccess) {
    EXPECT_EQ(classify_buy_pyoguk_outcome(0, 1000, 100),
              BuyPyogukOutcome::Success);
    EXPECT_EQ(classify_buy_pyoguk_outcome(4, 1000, 100),
              BuyPyogukOutcome::Success);
}

TEST(BuyPyogukOutcome, AtTabCapIsFailure) {
    EXPECT_EQ(classify_buy_pyoguk_outcome(5, 1000, 100),
              BuyPyogukOutcome::Failure);
    EXPECT_EQ(classify_buy_pyoguk_outcome(6, 1000, 100),
              BuyPyogukOutcome::Failure);
}

TEST(BuyPyogukOutcome, MoneyBelowBuyPriceIsFailure) {
    EXPECT_EQ(classify_buy_pyoguk_outcome(0, 99, 100),
              BuyPyogukOutcome::Failure);
    EXPECT_EQ(classify_buy_pyoguk_outcome(0, 0, 0),
              BuyPyogukOutcome::Success);  // edge: free
}

TEST(BuyPyogukPlan, SuccessEmitsFiveSteps) {
    auto plan = buy_pyoguk_side_effect_plan(
        /*current_pyoguk_num=*/2, /*inven_money=*/1000,
        /*buy_price=*/100, /*new_max_money=*/50000);
    EXPECT_TRUE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 5u);
    EXPECT_EQ(plan.effects[0].kind,
              BuyPyogukSideEffectKind::SubtractBuyPrice);
    EXPECT_EQ(plan.effects[0].buy_price, 100u);
    EXPECT_EQ(plan.effects[1].kind,
              BuyPyogukSideEffectKind::IncrementPyogukNum);
    EXPECT_EQ(plan.effects[1].new_pyoguk_num, 3u);
    EXPECT_EQ(plan.effects[2].kind,
              BuyPyogukSideEffectKind::UpdateMaxPurseMoney);
    EXPECT_EQ(plan.effects[2].new_max_money, 50000u);
    EXPECT_EQ(plan.effects[3].kind,
              BuyPyogukSideEffectKind::InsertPyogukBuyDB);
    EXPECT_EQ(plan.effects[4].kind,
              BuyPyogukSideEffectKind::BroadcastBuyAck);
    EXPECT_EQ(plan.effects[4].new_pyoguk_num, 3u);
}

TEST(BuyPyogukPlan, MoneyShortageEmitsSingleNack) {
    auto plan = buy_pyoguk_side_effect_plan(
        /*current_pyoguk_num=*/1, /*inven_money=*/50,
        /*buy_price=*/100, /*new_max_money=*/50000);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_TRUE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              BuyPyogukSideEffectKind::BroadcastBuyNack);
}

TEST(BuyPyogukPlan, TabCapEmitsSingleNack) {
    auto plan = buy_pyoguk_side_effect_plan(
        /*current_pyoguk_num=*/5, /*inven_money=*/10000,
        /*buy_price=*/100, /*new_max_money=*/50000);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_TRUE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
}

TEST(BuyPyogukPlan, PlanIsIdempotent) {
    auto a = buy_pyoguk_side_effect_plan(0, 1000, 100, 50000);
    auto b = buy_pyoguk_side_effect_plan(0, 1000, 100, 50000);
    EXPECT_EQ(a.send_ack, b.send_ack);
    EXPECT_EQ(a.send_nack, b.send_nack);
    ASSERT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].new_pyoguk_num, b.effects[i].new_pyoguk_num);
    }
}

TEST(BuyPyogukConstants, LegacyConstants) {
    EXPECT_EQ(LEGACY_TAB_PYOGUK_NUM, 5u);
    EXPECT_EQ(LEGACY_GIVEN_PYOGUK_SLOT, 3u);
    EXPECT_EQ(LEGACY_EMONEYLOG_LOSE_PYOGUK_BUY, 23u);
}
