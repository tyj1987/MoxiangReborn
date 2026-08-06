// D4.38 PutSkinSelectItem success-path side-effect dispatcher tests.

#include <mxh/server/skin_select_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(SkinSelectSideEffect, SuccessPlanEmitsThreeSteps) {
    auto plan = skin_select_success_side_effect_plan();
    EXPECT_TRUE(plan.send_broadcast);
    ASSERT_EQ(plan.effects.size(), 3u);
    EXPECT_EQ(plan.effects[0].kind, SkinSelectSideEffectKind::StartSkinDelay);
    EXPECT_EQ(plan.effects[1].kind,
              SkinSelectSideEffectKind::CharacterSkinInfoUpdate);
    EXPECT_EQ(plan.effects[2].kind,
              SkinSelectSideEffectKind::BroadcastSkinInfo);
}

TEST(SkinSelectSideEffect, PlanIsIdempotent) {
    auto a = skin_select_success_side_effect_plan();
    auto b = skin_select_success_side_effect_plan();
    EXPECT_EQ(a.send_broadcast, b.send_broadcast);
    ASSERT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
    }
}
