// D4.43 ItemMove (MP_ITEM_MOVE_SYN) data-plane + side-effect dispatcher
// tests.

#include <mxh/server/item_move_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(ItemMoveOutcome, ZeroRtIsSuccess) {
    EXPECT_EQ(classify_item_move_outcome(LEGACY_EI_TRUE),
              ItemMoveOutcome::Success);
    EXPECT_EQ(classify_item_move_outcome(0),
              ItemMoveOutcome::Success);
}

TEST(ItemMoveOutcome, NinetyNineIsSilent) {
    EXPECT_EQ(classify_item_move_outcome(LEGACY_MOVE_RT_SILENT),
              ItemMoveOutcome::Silent);
    EXPECT_EQ(classify_item_move_outcome(99),
              ItemMoveOutcome::Silent);
}

TEST(ItemMoveOutcome, NonZeroNonNinetyNineIsFailure) {
    EXPECT_EQ(classify_item_move_outcome(LEGACY_EI_LOCKED),
              ItemMoveOutcome::Failure);
    EXPECT_EQ(classify_item_move_outcome(LEGACY_EI_NOSPACE),
              ItemMoveOutcome::Failure);
    EXPECT_EQ(classify_item_move_outcome(1),
              ItemMoveOutcome::Failure);
    EXPECT_EQ(classify_item_move_outcome(98),
              ItemMoveOutcome::Failure);
}

TEST(ItemMovePlan, SuccessEmitsSingleAckWithPayload) {
    auto plan = item_move_side_effect_plan(
        /*move_rt=*/0, /*from_item_idx=*/100, /*from_pos=*/5,
        /*to_item_idx=*/100, /*to_pos=*/6);
    EXPECT_TRUE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_FALSE(plan.silent);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemMoveSideEffectKind::BroadcastMoveAck);
    EXPECT_EQ(plan.effects[0].from_item_idx, 100u);
    EXPECT_EQ(plan.effects[0].from_pos, 5u);
    EXPECT_EQ(plan.effects[0].to_item_idx, 100u);
    EXPECT_EQ(plan.effects[0].to_pos, 6u);
    EXPECT_EQ(plan.effects[0].original_rt, 0);
}

TEST(ItemMovePlan, FailureEmitsSingleNackWithRt) {
    auto plan = item_move_side_effect_plan(
        /*move_rt=*/LEGACY_EI_NOSPACE, /*from_item_idx=*/100,
        /*from_pos=*/5, /*to_item_idx=*/101, /*to_pos=*/6);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_FALSE(plan.silent);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemMoveSideEffectKind::BroadcastMoveNack);
    EXPECT_EQ(plan.effects[0].original_rt, LEGACY_EI_NOSPACE);
}

TEST(ItemMovePlan, SilentRtNinetyNineProducesEmptyPlan) {
    auto plan = item_move_side_effect_plan(
        /*move_rt=*/99, /*from_item_idx=*/100, /*from_pos=*/5,
        /*to_item_idx=*/101, /*to_pos=*/6);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_TRUE(plan.silent);
    EXPECT_TRUE(plan.effects.empty());
}

TEST(ItemMovePlan, PlanIsIdempotent) {
    auto a = item_move_side_effect_plan(0, 1, 2, 3, 4);
    auto b = item_move_side_effect_plan(0, 1, 2, 3, 4);
    EXPECT_EQ(a.send_ack, b.send_ack);
    EXPECT_EQ(a.send_nack, b.send_nack);
    EXPECT_EQ(a.silent, b.silent);
    ASSERT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].from_pos, b.effects[i].from_pos);
        EXPECT_EQ(a.effects[i].to_pos, b.effects[i].to_pos);
    }
}

TEST(ItemMoveConstants, LegacyEnumValues) {
    EXPECT_EQ(LEGACY_EI_TRUE, 0);
    EXPECT_EQ(LEGACY_EI_OUTOFPOS, 1);
    EXPECT_EQ(LEGACY_EI_NOTEQUALDATA, 2);
    EXPECT_EQ(LEGACY_EI_EXISTED, 3);
    EXPECT_EQ(LEGACY_EI_NOTEXIST, 4);
    EXPECT_EQ(LEGACY_EI_LOCKED, 5);
    EXPECT_EQ(LEGACY_EI_PASSWD, 6);
    EXPECT_EQ(LEGACY_EI_NOTENOUGHMONEY, 7);
    EXPECT_EQ(LEGACY_EI_NOSPACE, 8);
    EXPECT_EQ(LEGACY_EI_MAXMONEY, 9);
    EXPECT_EQ(LEGACY_MOVE_RT_SILENT, 99);
    EXPECT_EQ(LEGACY_EITEMUSE_SUCCESS, 0);
    EXPECT_EQ(LEGACY_EITEMUSE_MOVE, 2);
}
