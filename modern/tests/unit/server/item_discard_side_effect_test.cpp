// D4.46 ItemDiscard (MP_ITEM_DISCARD_SYN) data-plane + side-effect
// dispatcher tests.

#include <mxh/server/item_discard_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(ItemDiscardOutcome, ZeroRtIsSuccess) {
    EXPECT_EQ(classify_item_discard_outcome(0, false),
              ItemDiscardOutcome::Success);
}

TEST(ItemDiscardOutcome, NonZeroRtIsFailure) {
    EXPECT_EQ(classify_item_discard_outcome(1, false),
              ItemDiscardOutcome::Failure);
    EXPECT_EQ(classify_item_discard_outcome(99, false),
              ItemDiscardOutcome::Failure);
}

TEST(ItemDiscardOutcome, LootedOverridesRt) {
    EXPECT_EQ(classify_item_discard_outcome(0, true),
              ItemDiscardOutcome::LootedPlayer);
    EXPECT_EQ(classify_item_discard_outcome(7, true),
              ItemDiscardOutcome::LootedPlayer);
}

TEST(ItemDiscardPlan, SuccessEmitsAckThenLog) {
    auto plan = item_discard_side_effect_plan(
        /*discard_rt=*/0, /*is_looted=*/false,
        /*target_pos=*/10, /*item_idx=*/100, /*item_num=*/1);
    EXPECT_TRUE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_FALSE(plan.send_error_nack);
    ASSERT_EQ(plan.effects.size(), 2u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemDiscardSideEffectKind::BroadcastDiscardAck);
    EXPECT_EQ(plan.effects[0].target_pos, 10u);
    EXPECT_EQ(plan.effects[0].item_idx, 100u);
    EXPECT_EQ(plan.effects[1].kind,
              ItemDiscardSideEffectKind::LogDiscardedItem);
    EXPECT_EQ(plan.effects[1].log_code,
              LEGACY_ELOG_ITEM_DISCARD);
}

TEST(ItemDiscardPlan, FailureEmitsDiscardNack) {
    auto plan = item_discard_side_effect_plan(
        /*discard_rt=*/3, /*is_looted=*/false,
        /*target_pos=*/10, /*item_idx=*/100, /*item_num=*/2);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_FALSE(plan.send_error_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemDiscardSideEffectKind::BroadcastDiscardNack);
    EXPECT_EQ(plan.effects[0].original_rt, 3);
    EXPECT_EQ(plan.effects[0].ecode, 3);
}

TEST(ItemDiscardPlan, LootedEmitsErrorNackWithDiscardECode) {
    auto plan = item_discard_side_effect_plan(
        /*discard_rt=*/0, /*is_looted=*/true,
        /*target_pos=*/10, /*item_idx=*/100, /*item_num=*/1);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_TRUE(plan.send_error_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemDiscardSideEffectKind::BroadcastErrorNack);
    EXPECT_EQ(plan.effects[0].original_rt,
              LEGACY_DISCARD_RT_LOOTED);
    EXPECT_EQ(plan.effects[0].ecode,
              LEGACY_EITEMUSE_DISCARD);
}

TEST(ItemDiscardPlan, PlanIsIdempotent) {
    auto a = item_discard_side_effect_plan(0, false, 1, 2, 3);
    auto b = item_discard_side_effect_plan(0, false, 1, 2, 3);
    EXPECT_EQ(a.send_ack, b.send_ack);
    EXPECT_EQ(a.send_nack, b.send_nack);
    EXPECT_EQ(a.send_error_nack, b.send_error_nack);
    ASSERT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].target_pos, b.effects[i].target_pos);
    }
}
