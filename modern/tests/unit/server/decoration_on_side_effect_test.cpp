// Tests for MP_ITEMEXT_SHOPITEM_DECORATION_ON side-effect.

#include <mxh/server/decoration_on_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

DecorationOnValidationInput found() {
    DecorationOnValidationInput in{};
    in.player_found = true;
    return in;
}

TEST(DecorationOnOutcome, PlayerFoundIsBroadcast) {
    EXPECT_EQ(classify_decoration_on_outcome(found()),
              DecorationOnOutcome::Broadcast);
}

TEST(DecorationOnOutcome, PlayerMissingIsNoPlayer) {
    DecorationOnValidationInput in{};
    EXPECT_EQ(classify_decoration_on_outcome(in),
              DecorationOnOutcome::NoPlayer);
}

TEST(DecorationOnPlan, BroadcastEmitsEffect) {
    auto in = found();
    auto plan = decoration_on_side_effect_plan(in, 100, 50, 6);
    EXPECT_TRUE(plan.broadcast);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              DecorationOnSideEffectKind::BroadcastToOthers);
    EXPECT_EQ(plan.effects[0].player_id, 100u);
    EXPECT_EQ(plan.effects[0].data1, 50u);
    EXPECT_EQ(plan.effects[0].data2, 6u);
}

TEST(DecorationOnPlan, NoPlayerEmitsEmptyPlan) {
    DecorationOnValidationInput in{};
    auto plan = decoration_on_side_effect_plan(in, 100, 50, 6);
    EXPECT_FALSE(plan.broadcast);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(DecorationOnPlan, PlanIsIdempotent) {
    auto in = found();
    auto a = decoration_on_side_effect_plan(in, 100, 50, 6);
    auto b = decoration_on_side_effect_plan(in, 100, 50, 6);
    EXPECT_EQ(a.broadcast, b.broadcast);
    EXPECT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].player_id, b.effects[i].player_id);
        EXPECT_EQ(a.effects[i].data1, b.effects[i].data1);
        EXPECT_EQ(a.effects[i].data2, b.effects[i].data2);
    }
}
