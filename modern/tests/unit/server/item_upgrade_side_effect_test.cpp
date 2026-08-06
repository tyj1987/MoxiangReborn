// D4.74 ItemUpgrade (MP_ITEM_UPGRADE_SYN) side-effect
// dispatcher tests.

#include <mxh/server/item_upgrade_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(ItemUpgradeOutcome, ZeroRtIsSuccess) {
    EXPECT_EQ(classify_item_upgrade_outcome(0),
              ItemUpgradeOutcome::Success);
}

TEST(ItemUpgradeOutcome, NonZeroRtIsFailure) {
    EXPECT_EQ(classify_item_upgrade_outcome(1),
              ItemUpgradeOutcome::Failure);
    EXPECT_EQ(classify_item_upgrade_outcome(99),
              ItemUpgradeOutcome::Failure);
}

TEST(ItemUpgradePlan, SuccessEmitsUpgradeSuccessAck) {
    auto plan = item_upgrade_side_effect_plan(
        /*upgrade_rt=*/0,
        /*item_idx=*/1, /*item_pos=*/2,
        /*material_item_idx=*/3, /*material_item_pos=*/4);
    EXPECT_TRUE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_EQ(plan.error_code, 0);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemUpgradeSideEffectKind::BroadcastUpgradeSuccessAck);
    EXPECT_EQ(plan.effects[0].item_idx, 1u);
    EXPECT_EQ(plan.effects[0].item_pos, 2u);
    EXPECT_EQ(plan.effects[0].material_item_idx, 3u);
    EXPECT_EQ(plan.effects[0].material_item_pos, 4u);
}

TEST(ItemUpgradePlan, FailureEmitsUpgradeErrorNackWithUpgradeCode) {
    auto plan = item_upgrade_side_effect_plan(
        /*upgrade_rt=*/5,
        /*item_idx=*/1, /*item_pos=*/2,
        /*material_item_idx=*/3, /*material_item_pos=*/4);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_EQ(plan.error_code, LEGACY_EITEMUSE_UPGRADE);
    EXPECT_EQ(plan.effects[0].kind,
              ItemUpgradeSideEffectKind::BroadcastUpgradeErrorNack);
    EXPECT_EQ(plan.effects[0].error_code, LEGACY_EITEMUSE_UPGRADE);
}

TEST(ItemUpgradePlan, AnyNonZeroRtFoldsToUpgradeError) {
    auto plan = item_upgrade_side_effect_plan(99, 1, 2, 3, 4);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_EQ(plan.error_code, LEGACY_EITEMUSE_UPGRADE);
}

TEST(ItemUpgradePlan, PlanIsIdempotent) {
    auto a = item_upgrade_side_effect_plan(0, 1, 2, 3, 4);
    auto b = item_upgrade_side_effect_plan(0, 1, 2, 3, 4);
    EXPECT_EQ(a.send_ack, b.send_ack);
    EXPECT_EQ(a.send_nack, b.send_nack);
    EXPECT_EQ(a.error_code, b.error_code);
    ASSERT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].item_idx, b.effects[i].item_idx);
        EXPECT_EQ(a.effects[i].item_pos, b.effects[i].item_pos);
    }
}
