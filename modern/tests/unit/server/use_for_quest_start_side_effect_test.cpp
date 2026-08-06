// D4.58 UseForQuestStart (MP_ITEM_USE_FOR_QUESTSTART_SYN) side-effect
// dispatcher tests.

#include <mxh/server/use_for_quest_start_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(UseForQuestStartOutcome, ZeroRtIsSuccess) {
    EXPECT_EQ(classify_use_for_quest_start_outcome(0),
              UseForQuestStartOutcome::Success);
}

TEST(UseForQuestStartOutcome, NonZeroRtIsFailure) {
    EXPECT_EQ(classify_use_for_quest_start_outcome(1),
              UseForQuestStartOutcome::Failure);
    EXPECT_EQ(classify_use_for_quest_start_outcome(7),
              UseForQuestStartOutcome::Failure);
    EXPECT_EQ(classify_use_for_quest_start_outcome(99),
              UseForQuestStartOutcome::Failure);
}

TEST(UseForQuestStartPlan, SuccessEmitsUseAck) {
    auto plan = use_for_quest_start_side_effect_plan(
        /*use_rt=*/0, /*target_pos=*/5, /*item_idx=*/100);
    EXPECT_TRUE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              UseForQuestStartSideEffectKind::BroadcastUseAck);
    EXPECT_EQ(plan.effects[0].target_pos, 5u);
    EXPECT_EQ(plan.effects[0].item_idx, 100u);
    EXPECT_EQ(plan.effects[0].error_code, 0);
}

TEST(UseForQuestStartPlan, FailureEmitsUseNackWithQuestError) {
    auto plan = use_for_quest_start_side_effect_plan(
        /*use_rt=*/1, /*target_pos=*/5, /*item_idx=*/100);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_TRUE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              UseForQuestStartSideEffectKind::BroadcastUseNack);
    EXPECT_EQ(plan.effects[0].error_code, LEGACY_EITEMUSE_QUEST);
    EXPECT_EQ(plan.error_code, LEGACY_EITEMUSE_QUEST);
}

TEST(UseForQuestStartPlan, AnyNonZeroRtFoldsToQuestError) {
    auto plan = use_for_quest_start_side_effect_plan(99, 1, 1);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_EQ(plan.error_code, LEGACY_EITEMUSE_QUEST);
}

TEST(UseForQuestStartPlan, AckPreservesTargetPosAndItemIdx) {
    auto plan = use_for_quest_start_side_effect_plan(0, 42, 1234);
    EXPECT_EQ(plan.effects[0].target_pos, 42u);
    EXPECT_EQ(plan.effects[0].item_idx, 1234u);
}

TEST(UseForQuestStartPlan, PlanIsIdempotent) {
    auto a = use_for_quest_start_side_effect_plan(0, 7, 8);
    auto b = use_for_quest_start_side_effect_plan(0, 7, 8);
    EXPECT_EQ(a.send_ack, b.send_ack);
    EXPECT_EQ(a.send_nack, b.send_nack);
    EXPECT_EQ(a.error_code, b.error_code);
    ASSERT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].target_pos, b.effects[i].target_pos);
        EXPECT_EQ(a.effects[i].item_idx, b.effects[i].item_idx);
    }
}
