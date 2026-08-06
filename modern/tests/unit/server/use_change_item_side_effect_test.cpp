// D4.60 UseChangeItem (MP_ITEM_USE_CHANGEITEM_SYN) side-effect
// dispatcher tests.

#include <mxh/server/use_change_item_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(UseChangeItemOutcome, NonZeroRtIsSuccess) {
    EXPECT_EQ(classify_use_change_item_outcome(1, true),
              UseChangeItemOutcome::Success);
    EXPECT_EQ(classify_use_change_item_outcome(99, true),
              UseChangeItemOutcome::Success);
}

TEST(UseChangeItemOutcome, ZeroRtIsNotUsed) {
    EXPECT_EQ(classify_use_change_item_outcome(0, true),
              UseChangeItemOutcome::NotUsed);
}

TEST(UseChangeItemOutcome, NoPlayerTakesPrecedence) {
    EXPECT_EQ(classify_use_change_item_outcome(0, false),
              UseChangeItemOutcome::NoPlayer);
    EXPECT_EQ(classify_use_change_item_outcome(1, false),
              UseChangeItemOutcome::NoPlayer);
}

TEST(UseChangeItemPlan, SuccessIsSilent) {
    auto plan = use_change_item_side_effect_plan(
        /*use_rt=*/1, /*player_found=*/true,
        /*target_pos=*/5, /*item_idx=*/100);
    EXPECT_TRUE(plan.silent_success);
    EXPECT_FALSE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              UseChangeItemSideEffectKind::SilentSuccess);
}

TEST(UseChangeItemPlan, NotUsedEmitsNackWithZeroEcode) {
    auto plan = use_change_item_side_effect_plan(
        /*use_rt=*/0, /*player_found=*/true,
        /*target_pos=*/5, /*item_idx=*/100);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_FALSE(plan.silent_success);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              UseChangeItemSideEffectKind::BroadcastUseNack);
    EXPECT_EQ(plan.effects[0].error_code, 0);
    EXPECT_EQ(plan.error_code, 0);
}

TEST(UseChangeItemPlan, NoPlayerEmitsEmptyPlan) {
    auto plan = use_change_item_side_effect_plan(1, false, 1, 1);
    EXPECT_FALSE(plan.silent_success);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(UseChangeItemPlan, SuccessPreservesTargetPosAndItemIdx) {
    auto plan = use_change_item_side_effect_plan(2, true, 42, 1234);
    EXPECT_EQ(plan.effects[0].target_pos, 42u);
    EXPECT_EQ(plan.effects[0].item_idx, 1234u);
}

TEST(UseChangeItemPlan, PlanIsIdempotent) {
    auto a = use_change_item_side_effect_plan(1, true, 7, 8);
    auto b = use_change_item_side_effect_plan(1, true, 7, 8);
    EXPECT_EQ(a.silent_success, b.silent_success);
    EXPECT_EQ(a.send_nack, b.send_nack);
    ASSERT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].target_pos, b.effects[i].target_pos);
        EXPECT_EQ(a.effects[i].item_idx, b.effects[i].item_idx);
    }
}
