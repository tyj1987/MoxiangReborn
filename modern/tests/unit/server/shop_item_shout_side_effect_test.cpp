// D4.73 ShopItemShout (MP_ITEM_SHOPITEM_SHOUT_SYN) side-effect
// dispatcher tests.

#include <mxh/server/shop_item_shout_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

ShopItemShoutValidationInput ok() {
    ShopItemShoutValidationInput in{};
    in.player_found = true;
    in.usable_shop_item = true;
    in.is_once_variant = false;
    in.discard_rt = 0;
    return in;
}

TEST(ShopItemShoutOutcome, UsableIsBroadcast) {
    auto in = ok();
    EXPECT_EQ(classify_shop_item_shout_outcome(in),
              ShopItemShoutOutcome::Broadcast);
}

TEST(ShopItemShoutOutcome, NotUsableIsNotUsable) {
    auto in = ok();
    in.usable_shop_item = false;
    EXPECT_EQ(classify_shop_item_shout_outcome(in),
              ShopItemShoutOutcome::NotUsable);
}

TEST(ShopItemShoutOutcome, OnceVariantDiscardFailIsDiscardFail) {
    auto in = ok();
    in.is_once_variant = true;
    in.discard_rt = 1;
    EXPECT_EQ(classify_shop_item_shout_outcome(in),
              ShopItemShoutOutcome::DiscardFail);
}

TEST(ShopItemShoutOutcome, NonOnceVariantIgnoresDiscard) {
    auto in = ok();
    in.is_once_variant = false;
    in.discard_rt = 1;
    EXPECT_EQ(classify_shop_item_shout_outcome(in),
              ShopItemShoutOutcome::Broadcast);
}

TEST(ShopItemShoutOutcome, NoPlayerTakesPrecedence) {
    auto in = ok();
    in.player_found = false;
    EXPECT_EQ(classify_shop_item_shout_outcome(in),
              ShopItemShoutOutcome::NoPlayer);
}

TEST(ShopItemShoutPlan, BroadcastNonOnceEmitsForward) {
    auto in = ok();
    auto plan = shop_item_shout_side_effect_plan(
        in, /*item_idx=*/100, /*item_pos=*/5, /*character_idx=*/42);
    EXPECT_TRUE(plan.forward_shout);
    EXPECT_FALSE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ShopItemShoutSideEffectKind::ForwardShoutAck);
    EXPECT_EQ(plan.effects[0].character_idx, 42u);
}

TEST(ShopItemShoutPlan, BroadcastOnceEmitsDiscardAndUseAckAndForward) {
    auto in = ok();
    in.is_once_variant = true;
    auto plan = shop_item_shout_side_effect_plan(in, 100, 5, 42);
    EXPECT_TRUE(plan.forward_shout);
    EXPECT_FALSE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 3u);
    EXPECT_EQ(plan.effects[0].kind,
              ShopItemShoutSideEffectKind::DiscardShoutItem);
    EXPECT_EQ(plan.effects[1].kind,
              ShopItemShoutSideEffectKind::BroadcastUseAck);
    EXPECT_EQ(plan.effects[2].kind,
              ShopItemShoutSideEffectKind::ForwardShoutAck);
}

TEST(ShopItemShoutPlan, NotUsableEmitsNack) {
    auto in = ok();
    in.usable_shop_item = false;
    auto plan = shop_item_shout_side_effect_plan(in, 1, 2, 3);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_EQ(plan.effects[0].kind,
              ShopItemShoutSideEffectKind::BroadcastShoutNack);
}

TEST(ShopItemShoutPlan, DiscardFailEmitsNack) {
    auto in = ok();
    in.is_once_variant = true;
    in.discard_rt = 1;
    auto plan = shop_item_shout_side_effect_plan(in, 1, 2, 3);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_EQ(plan.effects[0].kind,
              ShopItemShoutSideEffectKind::BroadcastShoutNack);
    EXPECT_TRUE(plan.effects[0].is_once_variant);
}

TEST(ShopItemShoutPlan, NoPlayerEmitsEmptyPlan) {
    auto in = ok();
    in.player_found = false;
    auto plan = shop_item_shout_side_effect_plan(in, 1, 2, 3);
    EXPECT_FALSE(plan.forward_shout);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(ShopItemShoutPlan, PlanIsIdempotent) {
    auto in = ok();
    auto a = shop_item_shout_side_effect_plan(in, 1, 2, 3);
    auto b = shop_item_shout_side_effect_plan(in, 1, 2, 3);
    EXPECT_EQ(a.forward_shout, b.forward_shout);
    EXPECT_EQ(a.send_nack, b.send_nack);
    ASSERT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].item_idx, b.effects[i].item_idx);
        EXPECT_EQ(a.effects[i].item_pos, b.effects[i].item_pos);
    }
}
