// D4.38 PutSkinSelectItem success-path side-effect dispatcher tests.
// D4.39 PutSkinSelectItem NACK-path side-effect dispatcher tests.

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

TEST(SkinSelectNack, FailResultEmitsSingleDword3Step) {
    auto plan = skin_select_nack_side_effect_plan(
        SkinSelectResult::Fail, 0u, 50u);
    EXPECT_TRUE(plan.send_nack);
    ASSERT_EQ(plan.steps.size(), 1u);
    EXPECT_EQ(plan.steps[0].result_code, 1u);
    EXPECT_EQ(plan.steps[0].skin_delay_remaining, 0u);
    EXPECT_EQ(plan.steps[0].dw_limit_level, 50u);
}

TEST(SkinSelectNack, DelayFailCarriesRemainingDelay) {
    auto plan = skin_select_nack_side_effect_plan(
        SkinSelectResult::DelayFail, 12345u, 60u);
    EXPECT_TRUE(plan.send_nack);
    ASSERT_EQ(plan.steps.size(), 1u);
    EXPECT_EQ(plan.steps[0].result_code, 2u);
    EXPECT_EQ(plan.steps[0].skin_delay_remaining, 12345u);
    EXPECT_EQ(plan.steps[0].dw_limit_level, 60u);
}

TEST(SkinSelectNack, LevelFailCarriesLimitLevel) {
    auto plan = skin_select_nack_side_effect_plan(
        SkinSelectResult::LevelFail, 0u, 99u);
    EXPECT_TRUE(plan.send_nack);
    ASSERT_EQ(plan.steps.size(), 1u);
    EXPECT_EQ(plan.steps[0].result_code, 3u);
    EXPECT_EQ(plan.steps[0].skin_delay_remaining, 0u);
    EXPECT_EQ(plan.steps[0].dw_limit_level, 99u);
}

TEST(SkinSelectNack, SuccessResultProducesEmptyPlan) {
    auto plan = skin_select_nack_side_effect_plan(
        SkinSelectResult::Success, 0u, 50u);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_TRUE(plan.steps.empty());
}

TEST(SkinSelectNack, PlanIsIdempotent) {
    auto a = skin_select_nack_side_effect_plan(
        SkinSelectResult::LevelFail, 100u, 50u);
    auto b = skin_select_nack_side_effect_plan(
        SkinSelectResult::LevelFail, 100u, 50u);
    EXPECT_EQ(a.send_nack, b.send_nack);
    ASSERT_EQ(a.steps.size(), b.steps.size());
    EXPECT_EQ(a.steps[0].result_code, b.steps[0].result_code);
    EXPECT_EQ(a.steps[0].skin_delay_remaining,
              b.steps[0].skin_delay_remaining);
    EXPECT_EQ(a.steps[0].dw_limit_level, b.steps[0].dw_limit_level);
}
