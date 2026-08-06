// Tests for MP_ITEM_SHOPITEM_REVIVEOTHER_ACK side-effect dispatcher.

#include <mxh/server/revive_other_ack_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

ReviveOtherAckValidationInput success_input() {
    ReviveOtherAckValidationInput in{};
    in.resurrector_state_is_die = true;
    in.item_is_useable = true;
    in.item_info_exists = true;
    in.item_kind_is_incantation = true;
    in.item_limit_level_nonzero = true;
    in.item_in_using_list = false;
    in.item_sell_price_zero = true;
    in.discard_returned_true = true;
    in.resurrector_is_able = true;
    return in;
}

TEST(ReviveOtherAckOutcome, NotDeadTakesPrecedence) {
    auto in = success_input();
    in.resurrector_state_is_die = false;
    EXPECT_EQ(classify_revive_other_ack_outcome(in),
              ReviveOtherAckOutcome::NotDead);
}

TEST(ReviveOtherAckOutcome, NotUsableWhenIsUseAbleShopItemFails) {
    auto in = success_input();
    in.item_is_useable = false;
    EXPECT_EQ(classify_revive_other_ack_outcome(in),
              ReviveOtherAckOutcome::NotUsable);
}

TEST(ReviveOtherAckOutcome, BadItemInfoWhenItemInfoMissing) {
    auto in = success_input();
    in.item_info_exists = false;
    EXPECT_EQ(classify_revive_other_ack_outcome(in),
              ReviveOtherAckOutcome::BadItemInfo);
}

TEST(ReviveOtherAckOutcome, BadItemInfoWhenNotIncantation) {
    auto in = success_input();
    in.item_kind_is_incantation = false;
    EXPECT_EQ(classify_revive_other_ack_outcome(in),
              ReviveOtherAckOutcome::BadItemInfo);
}

TEST(ReviveOtherAckOutcome, BadItemInfoWhenLimitLevelZero) {
    auto in = success_input();
    in.item_limit_level_nonzero = false;
    EXPECT_EQ(classify_revive_other_ack_outcome(in),
              ReviveOtherAckOutcome::BadItemInfo);
}

TEST(ReviveOtherAckOutcome, FailWhenPlayerNotAbleToRevive) {
    auto in = success_input();
    in.resurrector_is_able = false;
    EXPECT_EQ(classify_revive_other_ack_outcome(in),
              ReviveOtherAckOutcome::Fail);
}

TEST(ReviveOtherAckOutcome, AlreadyUsedWhenItemInUsingList) {
    auto in = success_input();
    in.item_in_using_list = true;
    EXPECT_EQ(classify_revive_other_ack_outcome(in),
              ReviveOtherAckOutcome::AlreadyUsed);
}

TEST(ReviveOtherAckOutcome, FailWhenSellPriceNonZero) {
    auto in = success_input();
    in.item_sell_price_zero = false;
    EXPECT_EQ(classify_revive_other_ack_outcome(in),
              ReviveOtherAckOutcome::Fail);
}

TEST(ReviveOtherAckOutcome, FailWhenDiscardFailed) {
    auto in = success_input();
    in.discard_returned_true = false;
    EXPECT_EQ(classify_revive_other_ack_outcome(in),
              ReviveOtherAckOutcome::Fail);
}

TEST(ReviveOtherAckOutcome, SuccessWhenAllChecksPass) {
    EXPECT_EQ(classify_revive_other_ack_outcome(success_input()),
              ReviveOtherAckOutcome::Success);
}

TEST(ReviveOtherAckPlan, NotDeadEmitsTwoNacksAndClearsReviveData) {
    auto in = success_input();
    in.resurrector_state_is_die = false;
    auto plan = revive_other_ack_side_effect_plan(in, 100, 200, 50, 6);
    EXPECT_TRUE(plan.send_not_dead_nack);
    EXPECT_FALSE(plan.send_revive_ack);
    EXPECT_TRUE(plan.clear_revive_data);
    ASSERT_EQ(plan.effects.size(), 2u);
    EXPECT_EQ(plan.effects[0].kind,
              ReviveOtherAckSideEffectKind::SendNotDeadNackToTarget);
    EXPECT_EQ(plan.effects[0].target_id, 100u);
    EXPECT_EQ(plan.effects[0].nack_code, LEGACY_ESHOPITEM_REVIVE_NOTDEAD);
    EXPECT_EQ(plan.effects[1].kind,
              ReviveOtherAckSideEffectKind::SendNotDeadNackToResurrector);
    EXPECT_EQ(plan.effects[1].resurrector_id, 200u);
    EXPECT_EQ(plan.effects[1].nack_code, LEGACY_ESHOPITEM_REVIVE_NOTDEAD);
}

TEST(ReviveOtherAckPlan, NotUsableSendsNotUseToTargetAndFailToResurrector) {
    auto in = success_input();
    in.item_is_useable = false;
    auto plan = revive_other_ack_side_effect_plan(in, 100, 200, 50, 6);
    EXPECT_TRUE(plan.send_not_usable_nack);
    EXPECT_FALSE(plan.send_revive_ack);
    ASSERT_EQ(plan.effects.size(), 2u);
    EXPECT_EQ(plan.effects[0].kind,
              ReviveOtherAckSideEffectKind::SendNotUsableNackToTarget);
    EXPECT_EQ(plan.effects[0].nack_code, LEGACY_ESHOPITEM_REVIVE_NOTUSE);
    EXPECT_EQ(plan.effects[1].kind,
              ReviveOtherAckSideEffectKind::SendNotUsableNackToResurrector);
    EXPECT_EQ(plan.effects[1].nack_code, LEGACY_ESHOPITEM_REVIVE_FAIL);
}

TEST(ReviveOtherAckPlan, BadItemInfoSendsFailedNackPair) {
    auto in = success_input();
    in.item_info_exists = false;
    auto plan = revive_other_ack_side_effect_plan(in, 100, 200, 50, 6);
    EXPECT_TRUE(plan.send_failed_nack);
    ASSERT_EQ(plan.effects.size(), 2u);
    EXPECT_EQ(plan.effects[0].kind,
              ReviveOtherAckSideEffectKind::SendFailedNackToTarget);
    EXPECT_EQ(plan.effects[0].nack_code, LEGACY_ESHOPITEM_REVIVE_FAIL);
    EXPECT_EQ(plan.effects[1].kind,
              ReviveOtherAckSideEffectKind::SendFailedNackToResurrector);
    EXPECT_EQ(plan.effects[1].nack_code, LEGACY_ESHOPITEM_REVIVE_FAIL);
}

TEST(ReviveOtherAckPlan, FailSendsFailedNackPair) {
    auto in = success_input();
    in.discard_returned_true = false;
    auto plan = revive_other_ack_side_effect_plan(in, 100, 200, 50, 6);
    EXPECT_TRUE(plan.send_failed_nack);
    ASSERT_EQ(plan.effects.size(), 2u);
    EXPECT_EQ(plan.effects[0].kind,
              ReviveOtherAckSideEffectKind::SendFailedNackToTarget);
    EXPECT_EQ(plan.effects[1].kind,
              ReviveOtherAckSideEffectKind::SendFailedNackToResurrector);
}

TEST(ReviveOtherAckPlan, SuccessSendsAckPairPlusUseAckAndDiscard) {
    auto in = success_input();
    auto plan = revive_other_ack_side_effect_plan(in, 100, 200, 50, 6);
    EXPECT_TRUE(plan.send_revive_ack);
    EXPECT_TRUE(plan.send_use_ack_to_target);
    EXPECT_TRUE(plan.revive_shop_item);
    EXPECT_TRUE(plan.discard_shop_item);
    EXPECT_FALSE(plan.send_failed_nack);
    EXPECT_EQ(plan.effects.size(), 5u);
    EXPECT_EQ(plan.effects[0].kind,
              ReviveOtherAckSideEffectKind::DiscardShopItemFromTarget);
    EXPECT_EQ(plan.effects[0].target_id, 100u);
    EXPECT_EQ(plan.effects[0].shop_item_idx, 50u);
    EXPECT_EQ(plan.effects[0].shop_item_pos, 6u);
    EXPECT_EQ(plan.effects[1].kind,
              ReviveOtherAckSideEffectKind::ReviveShopItemOnResurrector);
    EXPECT_EQ(plan.effects[1].resurrector_id, 200u);
    EXPECT_EQ(plan.effects[2].kind,
              ReviveOtherAckSideEffectKind::SendUseAckToTarget);
    EXPECT_EQ(plan.effects[2].target_id, 100u);
    EXPECT_EQ(plan.effects[2].shop_item_idx, 50u);
    EXPECT_EQ(plan.effects[2].shop_item_pos, 6u);
    EXPECT_EQ(plan.effects[3].kind,
              ReviveOtherAckSideEffectKind::SendReviveAckToTarget);
    EXPECT_EQ(plan.effects[3].target_id, 100u);
    EXPECT_EQ(plan.effects[3].resurrector_id, 200u);
    EXPECT_EQ(plan.effects[4].kind,
              ReviveOtherAckSideEffectKind::SendReviveAckToResurrector);
    EXPECT_EQ(plan.effects[4].target_id, 100u);
    EXPECT_EQ(plan.effects[4].resurrector_id, 200u);
}

TEST(ReviveOtherAckPlan, AlreadyUsedSkipsDiscardButStillAcks) {
    auto in = success_input();
    in.item_in_using_list = true;
    auto plan = revive_other_ack_side_effect_plan(in, 100, 200, 50, 6);
    EXPECT_TRUE(plan.send_revive_ack);
    EXPECT_TRUE(plan.send_use_ack_to_target);
    EXPECT_FALSE(plan.discard_shop_item);
    EXPECT_EQ(plan.effects.size(), 4u);
    EXPECT_EQ(plan.effects[0].kind,
              ReviveOtherAckSideEffectKind::ReviveShopItemOnResurrector);
    EXPECT_EQ(plan.effects[1].kind,
              ReviveOtherAckSideEffectKind::SendUseAckToTarget);
    EXPECT_EQ(plan.effects[2].kind,
              ReviveOtherAckSideEffectKind::SendReviveAckToTarget);
    EXPECT_EQ(plan.effects[3].kind,
              ReviveOtherAckSideEffectKind::SendReviveAckToResurrector);
}

TEST(ReviveOtherAckPlan, PlanIsIdempotent) {
    auto in = success_input();
    auto a = revive_other_ack_side_effect_plan(in, 100, 200, 50, 6);
    auto b = revive_other_ack_side_effect_plan(in, 100, 200, 50, 6);
    EXPECT_EQ(a.send_revive_ack, b.send_revive_ack);
    EXPECT_EQ(a.clear_revive_data, b.clear_revive_data);
    EXPECT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].target_id, b.effects[i].target_id);
        EXPECT_EQ(a.effects[i].resurrector_id, b.effects[i].resurrector_id);
        EXPECT_EQ(a.effects[i].nack_code, b.effects[i].nack_code);
    }
}
