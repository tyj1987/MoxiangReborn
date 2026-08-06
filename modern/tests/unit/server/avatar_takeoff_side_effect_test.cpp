// D4.69 AvatarTakeoff (MP_ITEM_SHOPITEM_AVATAR_TAKEOFF) side-effect
// dispatcher tests.

#include <mxh/server/avatar_takeoff_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

AvatarTakeoffValidationInput ok() {
    AvatarTakeoffValidationInput in{};
    in.player_found = true;
    in.usable_shop_item = true;
    in.take_off_ok = true;
    return in;
}

TEST(AvatarTakeoffOutcome, AllGatesPassIsSilentSuccess) {
    auto in = ok();
    EXPECT_EQ(classify_avatar_takeoff_outcome(in),
              AvatarTakeoffOutcome::SilentSuccess);
}

TEST(AvatarTakeoffOutcome, NotUsableIsNotUsable) {
    auto in = ok();
    in.usable_shop_item = false;
    EXPECT_EQ(classify_avatar_takeoff_outcome(in),
              AvatarTakeoffOutcome::NotUsable);
}

TEST(AvatarTakeoffOutcome, TakeOffFailIsTakeOffFailed) {
    auto in = ok();
    in.take_off_ok = false;
    EXPECT_EQ(classify_avatar_takeoff_outcome(in),
              AvatarTakeoffOutcome::TakeOffFailed);
}

TEST(AvatarTakeoffOutcome, PrecedenceIsNoPlayerThenNotUsableThenTakeOffFail) {
    auto in = ok();
    in.player_found = false;
    in.usable_shop_item = false;
    in.take_off_ok = false;
    EXPECT_EQ(classify_avatar_takeoff_outcome(in),
              AvatarTakeoffOutcome::NoPlayer);

    in.player_found = true;
    EXPECT_EQ(classify_avatar_takeoff_outcome(in),
              AvatarTakeoffOutcome::NotUsable);

    in.usable_shop_item = true;
    EXPECT_EQ(classify_avatar_takeoff_outcome(in),
              AvatarTakeoffOutcome::TakeOffFailed);
}

TEST(AvatarTakeoffPlan, SilentSuccessEmitsSilent) {
    auto in = ok();
    auto plan = avatar_takeoff_side_effect_plan(
        in, /*item_idx=*/100, /*item_pos=*/5);
    EXPECT_TRUE(plan.silent_success);
    EXPECT_FALSE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              AvatarTakeoffSideEffectKind::SilentSuccess);
    EXPECT_EQ(plan.effects[0].item_idx, 100u);
    EXPECT_EQ(plan.effects[0].item_pos, 5u);
}

TEST(AvatarTakeoffPlan, NotUsableEmitsNack) {
    auto in = ok();
    in.usable_shop_item = false;
    auto plan = avatar_takeoff_side_effect_plan(in, 1, 2);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_EQ(plan.effects[0].kind,
              AvatarTakeoffSideEffectKind::BroadcastAvatarUseNack);
}

TEST(AvatarTakeoffPlan, TakeOffFailEmitsNack) {
    auto in = ok();
    in.take_off_ok = false;
    auto plan = avatar_takeoff_side_effect_plan(in, 1, 2);
    EXPECT_TRUE(plan.send_nack);
}

TEST(AvatarTakeoffPlan, NoPlayerEmitsEmptyPlan) {
    auto in = ok();
    in.player_found = false;
    auto plan = avatar_takeoff_side_effect_plan(in, 1, 2);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_FALSE(plan.silent_success);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(AvatarTakeoffPlan, PlanIsIdempotent) {
    auto in = ok();
    auto a = avatar_takeoff_side_effect_plan(in, 1, 2);
    auto b = avatar_takeoff_side_effect_plan(in, 1, 2);
    EXPECT_EQ(a.silent_success, b.silent_success);
    EXPECT_EQ(a.send_nack, b.send_nack);
    ASSERT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].item_idx, b.effects[i].item_idx);
        EXPECT_EQ(a.effects[i].item_pos, b.effects[i].item_pos);
    }
}
