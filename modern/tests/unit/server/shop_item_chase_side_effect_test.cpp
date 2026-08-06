// D4.77 ShopItemChase (MP_ITEM_SHOPITEM_CHASE_SYN) side-effect
// dispatcher tests.

#include <mxh/server/shop_item_chase_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

ShopItemChaseValidationInput ok() {
    ShopItemChaseValidationInput in{};
    in.target_found = true;
    in.item_kind = LEGACY_EINCANTATION_TRACKING;
    return in;
}

TEST(ShopItemChaseOutcome, TargetAndTrackingIsResolve) {
    auto in = ok();
    EXPECT_EQ(classify_shop_item_chase_outcome(in),
              ShopItemChaseOutcome::Resolve);
}

TEST(ShopItemChaseOutcome, AllThreeTrackingVariantsResolve) {
    for (auto k : {LEGACY_EINCANTATION_TRACKING,
                   LEGACY_EINCANTATION_TRACKING7,
                   LEGACY_EINCANTATION_TRACKING7_NOTRADE}) {
        ShopItemChaseValidationInput in{};
        in.target_found = true;
        in.item_kind = k;
        EXPECT_EQ(classify_shop_item_chase_outcome(in),
                  ShopItemChaseOutcome::Resolve);
    }
}

TEST(ShopItemChaseOutcome, OtherKindIsNotChase) {
    auto in = ok();
    in.item_kind = 99;
    EXPECT_EQ(classify_shop_item_chase_outcome(in),
              ShopItemChaseOutcome::NotChase);
}

TEST(ShopItemChaseOutcome, NoTargetIsNoTarget) {
    auto in = ok();
    in.target_found = false;
    EXPECT_EQ(classify_shop_item_chase_outcome(in),
              ShopItemChaseOutcome::NoTarget);
}

TEST(ShopItemChaseOutcome, NoTargetTakesPrecedence) {
    auto in = ok();
    in.target_found = false;
    in.item_kind = 99;
    EXPECT_EQ(classify_shop_item_chase_outcome(in),
              ShopItemChaseOutcome::NoTarget);
}

TEST(ShopItemChasePlan, ResolveEmitsAckAndTracking) {
    auto in = ok();
    auto plan = shop_item_chase_side_effect_plan(
        in, /*target_id=*/100, /*requester_char_idx=*/42,
        /*map_num=*/12, /*event_map_num=*/7);
    EXPECT_TRUE(plan.forward_ack);
    EXPECT_TRUE(plan.broadcast_tracking);
    EXPECT_FALSE(plan.forward_nack);
    ASSERT_EQ(plan.effects.size(), 2u);
    EXPECT_EQ(plan.effects[0].kind,
              ShopItemChaseSideEffectKind::ForwardChaseAckToAgent);
    EXPECT_EQ(plan.effects[0].target_id, 100u);
    EXPECT_EQ(plan.effects[0].requester_char_idx, 42u);
    EXPECT_EQ(plan.effects[0].item_kind, LEGACY_EINCANTATION_TRACKING);
    EXPECT_EQ(plan.effects[0].map_num, 12);
    EXPECT_EQ(plan.effects[0].event_map_num, 7);
    EXPECT_EQ(plan.effects[1].kind,
              ShopItemChaseSideEffectKind::BroadcastChaseTracking);
}

TEST(ShopItemChasePlan, NoTargetEmitsNack) {
    auto in = ok();
    in.target_found = false;
    auto plan = shop_item_chase_side_effect_plan(in, 100, 42, 12, 7);
    EXPECT_TRUE(plan.forward_nack);
    EXPECT_FALSE(plan.forward_ack);
    EXPECT_FALSE(plan.broadcast_tracking);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ShopItemChaseSideEffectKind::ForwardChaseNackToAgent);
    EXPECT_EQ(plan.effects[0].requester_char_idx, 42u);
}

TEST(ShopItemChasePlan, NotChaseEmitsEmptyPlan) {
    auto in = ok();
    in.item_kind = 99;
    auto plan = shop_item_chase_side_effect_plan(in, 100, 42, 12, 7);
    EXPECT_FALSE(plan.forward_ack);
    EXPECT_FALSE(plan.forward_nack);
    EXPECT_FALSE(plan.broadcast_tracking);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(ShopItemChasePlan, PlanIsIdempotent) {
    auto in = ok();
    auto a = shop_item_chase_side_effect_plan(in, 1, 2, 3, 4);
    auto b = shop_item_chase_side_effect_plan(in, 1, 2, 3, 4);
    EXPECT_EQ(a.forward_ack, b.forward_ack);
    EXPECT_EQ(a.forward_nack, b.forward_nack);
    EXPECT_EQ(a.broadcast_tracking, b.broadcast_tracking);
    ASSERT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].target_id, b.effects[i].target_id);
        EXPECT_EQ(a.effects[i].requester_char_idx, b.effects[i].requester_char_idx);
    }
}
