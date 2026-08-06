// D4.56 ReviveOther (MP_ITEM_SHOPITEM_REVIVEOTHER_SYN) side-effect
// dispatcher tests.

#include <mxh/server/revive_other_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

ReviveOtherValidationInput all_ok() {
    ReviveOtherValidationInput in{};
    in.target_found = true;
    in.target_is_dead = true;
    in.siege_war_active = false;
    in.observer_team = false;
    in.incantation_limit_level = false;
    in.is_useable_shop_item = true;
    return in;
}

TEST(ReviveOtherOutcome, AllGatesPassIsSuccess) {
    auto in = all_ok();
    EXPECT_EQ(classify_revive_other_outcome(in),
              ReviveOtherOutcome::Success);
}

TEST(ReviveOtherOutcome, TargetNotFoundIsNotDead) {
    auto in = all_ok();
    in.target_found = false;
    EXPECT_EQ(classify_revive_other_outcome(in),
              ReviveOtherOutcome::NotDead);
}

TEST(ReviveOtherOutcome, TargetAliveIsNotDead) {
    auto in = all_ok();
    in.target_is_dead = false;
    EXPECT_EQ(classify_revive_other_outcome(in),
              ReviveOtherOutcome::NotDead);
}

TEST(ReviveOtherOutcome, SiegeWarObserverIncantationIsNotReady) {
    auto in = all_ok();
    in.siege_war_active = true;
    in.observer_team = true;
    in.incantation_limit_level = true;
    EXPECT_EQ(classify_revive_other_outcome(in),
              ReviveOtherOutcome::NotReady);
}

TEST(ReviveOtherOutcome, NotUsableShopItemIsNotUsable) {
    auto in = all_ok();
    in.is_useable_shop_item = false;
    EXPECT_EQ(classify_revive_other_outcome(in),
              ReviveOtherOutcome::NotUsable);
}

TEST(ReviveOtherPlan, SuccessEmitsThreeSteps) {
    auto in = all_ok();
    auto plan = revive_other_side_effect_plan(
        in, /*target_id=*/100, /*item_idx=*/200, /*item_pos=*/5);
    EXPECT_TRUE(plan.forward_syn);
    EXPECT_FALSE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 3u);
    EXPECT_EQ(plan.effects[0].kind,
              ReviveOtherSideEffectKind::ForwardReviveOtherSyn);
    EXPECT_EQ(plan.effects[1].kind,
              ReviveOtherSideEffectKind::SetReviveData);
    EXPECT_EQ(plan.effects[2].kind,
              ReviveOtherSideEffectKind::SetReviveTime);
    EXPECT_EQ(plan.effects[2].revive_time_ms,
              LEGACY_REVIVETIME_60SEC_MS);
}

TEST(ReviveOtherPlan, NotDeadEmitsNack2) {
    auto in = all_ok();
    in.target_is_dead = false;
    auto plan = revive_other_side_effect_plan(in, 100, 200, 5);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_EQ(plan.nack_code, LEGACY_ESHOPITEM_REVIVE_NOTDEAD);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].nack_code, LEGACY_ESHOPITEM_REVIVE_NOTDEAD);
}

TEST(ReviveOtherPlan, NotReadyEmitsNack7) {
    auto in = all_ok();
    in.siege_war_active = true;
    in.observer_team = true;
    in.incantation_limit_level = true;
    auto plan = revive_other_side_effect_plan(in, 100, 200, 5);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_EQ(plan.nack_code, LEGACY_ESHOPITEM_REVIVE_NOTREADY);
}

TEST(ReviveOtherPlan, NotUsableEmitsNack3) {
    auto in = all_ok();
    in.is_useable_shop_item = false;
    auto plan = revive_other_side_effect_plan(in, 100, 200, 5);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_EQ(plan.nack_code, LEGACY_ESHOPITEM_REVIVE_NOTUSE);
}

TEST(ReviveOtherPlan, PlanIsIdempotent) {
    auto in = all_ok();
    auto a = revive_other_side_effect_plan(in, 1, 2, 3);
    auto b = revive_other_side_effect_plan(in, 1, 2, 3);
    EXPECT_EQ(a.forward_syn, b.forward_syn);
    EXPECT_EQ(a.send_nack, b.send_nack);
    ASSERT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
    }
}
