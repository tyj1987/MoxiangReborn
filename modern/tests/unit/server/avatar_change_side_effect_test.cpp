// D4.70 AvatarChange (MP_ITEM_SHOPITEM_AVATAR_CHANGE) side-effect
// dispatcher tests.

#include <mxh/server/avatar_change_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(AvatarChangeOutcome, PlayerFoundIsBroadcast) {
    AvatarChangeValidationInput in{};
    in.player_found = true;
    EXPECT_EQ(classify_avatar_change_outcome(in),
              AvatarChangeOutcome::Broadcast);
}

TEST(AvatarChangeOutcome, NoPlayerIsNoPlayer) {
    AvatarChangeValidationInput in{};
    in.player_found = false;
    EXPECT_EQ(classify_avatar_change_outcome(in),
              AvatarChangeOutcome::NoPlayer);
}

TEST(AvatarChangePlan, BroadcastEmitsTwoSteps) {
    AvatarChangeValidationInput in{};
    in.player_found = true;
    auto plan = avatar_change_side_effect_plan(
        in, /*object_id=*/42, /*item_pos=*/5);
    EXPECT_TRUE(plan.recalc);
    EXPECT_TRUE(plan.broadcast);
    ASSERT_EQ(plan.effects.size(), 2u);
    EXPECT_EQ(plan.effects[0].kind,
              AvatarChangeSideEffectKind::RecalcShopItemOption);
    EXPECT_EQ(plan.effects[1].kind,
              AvatarChangeSideEffectKind::BroadcastAvatarPuton);
    EXPECT_EQ(plan.effects[0].object_id, 42u);
    EXPECT_EQ(plan.effects[1].object_id, 42u);
    EXPECT_EQ(plan.effects[0].item_pos, 5u);
    EXPECT_EQ(plan.effects[1].item_pos, 5u);
}

TEST(AvatarChangePlan, NoPlayerEmitsEmptyPlan) {
    AvatarChangeValidationInput in{};
    in.player_found = false;
    auto plan = avatar_change_side_effect_plan(in, 1, 1);
    EXPECT_FALSE(plan.recalc);
    EXPECT_FALSE(plan.broadcast);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(AvatarChangePlan, PlanIsIdempotent) {
    AvatarChangeValidationInput in{};
    in.player_found = true;
    auto a = avatar_change_side_effect_plan(in, 1, 2);
    auto b = avatar_change_side_effect_plan(in, 1, 2);
    EXPECT_EQ(a.recalc, b.recalc);
    EXPECT_EQ(a.broadcast, b.broadcast);
    ASSERT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].object_id, b.effects[i].object_id);
        EXPECT_EQ(a.effects[i].item_pos, b.effects[i].item_pos);
    }
}
