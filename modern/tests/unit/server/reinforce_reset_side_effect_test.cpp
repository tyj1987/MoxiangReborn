// Tests for MP_ITEM_SHOPITEM_REINFORCERESET_SYN side-effect dispatcher.

#include <mxh/server/reinforce_reset_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

ReinforceResetValidationInput success_input() {
    ReinforceResetValidationInput in{};
    in.shop_item_is_useable = true;
    in.shop_item_exists = true;
    in.target_item_exists = true;
    in.target_item_info_exists = true;
    in.shop_item_icon_is_reinforce_reset = true;
    in.target_is_equip_kind = true;
    in.target_has_option = true;
    in.discard_returned_true = true;
    return in;
}

TEST(ReinforceResetOutcome, AllGatesPassIsSuccess) {
    EXPECT_EQ(classify_reinforce_reset_outcome(success_input()),
              ReinforceResetOutcome::Success);
}

TEST(ReinforceResetOutcome, NotUseableShopItemIsNotUsable) {
    auto in = success_input();
    in.shop_item_is_useable = false;
    EXPECT_EQ(classify_reinforce_reset_outcome(in),
              ReinforceResetOutcome::NotUsable);
}

TEST(ReinforceResetOutcome, ShopItemMissingIsBadItem) {
    auto in = success_input();
    in.shop_item_exists = false;
    EXPECT_EQ(classify_reinforce_reset_outcome(in),
              ReinforceResetOutcome::BadItem);
}

TEST(ReinforceResetOutcome, TargetItemMissingIsBadItem) {
    auto in = success_input();
    in.target_item_exists = false;
    EXPECT_EQ(classify_reinforce_reset_outcome(in),
              ReinforceResetOutcome::BadItem);
}

TEST(ReinforceResetOutcome, ItemInfoMissingIsItemInfoMissing) {
    auto in = success_input();
    in.target_item_info_exists = false;
    EXPECT_EQ(classify_reinforce_reset_outcome(in),
              ReinforceResetOutcome::ItemInfoMissing);
}

TEST(ReinforceResetOutcome, WrongShopIconIsWrongIcon) {
    auto in = success_input();
    in.shop_item_icon_is_reinforce_reset = false;
    EXPECT_EQ(classify_reinforce_reset_outcome(in),
              ReinforceResetOutcome::WrongIcon);
}

TEST(ReinforceResetOutcome, TargetNotEquipIsNotEquip) {
    auto in = success_input();
    in.target_is_equip_kind = false;
    EXPECT_EQ(classify_reinforce_reset_outcome(in),
              ReinforceResetOutcome::NotEquip);
}

TEST(ReinforceResetOutcome, TargetHasNoOptionIsNoOption) {
    auto in = success_input();
    in.target_has_option = false;
    EXPECT_EQ(classify_reinforce_reset_outcome(in),
              ReinforceResetOutcome::NoOption);
}

TEST(ReinforceResetOutcome, DiscardFailedIsDiscardFailed) {
    auto in = success_input();
    in.discard_returned_true = false;
    EXPECT_EQ(classify_reinforce_reset_outcome(in),
              ReinforceResetOutcome::DiscardFailed);
}

TEST(ReinforceResetOutcome, NotUsableTakesPrecedence) {
    auto in = success_input();
    in.shop_item_is_useable = false;
    in.discard_returned_true = false;
    EXPECT_EQ(classify_reinforce_reset_outcome(in),
              ReinforceResetOutcome::NotUsable);
}

TEST(ReinforceResetNackCode, MapsOutcomesToLegacyCodes) {
    EXPECT_EQ(reinforce_reset_nack_code(
        ReinforceResetOutcome::NotUsable), 1u);
    EXPECT_EQ(reinforce_reset_nack_code(
        ReinforceResetOutcome::BadItem), 2u);
    EXPECT_EQ(reinforce_reset_nack_code(
        ReinforceResetOutcome::ItemInfoMissing), 3u);
    EXPECT_EQ(reinforce_reset_nack_code(
        ReinforceResetOutcome::WrongIcon), 4u);
    EXPECT_EQ(reinforce_reset_nack_code(
        ReinforceResetOutcome::NotEquip), 5u);
    EXPECT_EQ(reinforce_reset_nack_code(
        ReinforceResetOutcome::NoOption), 6u);
    EXPECT_EQ(reinforce_reset_nack_code(
        ReinforceResetOutcome::DiscardFailed), 9u);
    EXPECT_EQ(reinforce_reset_nack_code(
        ReinforceResetOutcome::Success), 0u);
}

TEST(ReinforceResetPlan, SuccessEmitsFullSequence) {
    auto in = success_input();
    auto plan = reinforce_reset_side_effect_plan(in, 100, 50, 6, 999, 7);
    EXPECT_TRUE(plan.send_ack);
    EXPECT_TRUE(plan.send_use_ack);
    EXPECT_TRUE(plan.discard_shop_item);
    EXPECT_TRUE(plan.remove_item_option);
    EXPECT_TRUE(plan.db_item_option_delete);
    EXPECT_TRUE(plan.db_item_update);
    EXPECT_TRUE(plan.log_item_money_use);
    EXPECT_TRUE(plan.log_item_money_reset);
    EXPECT_TRUE(plan.clear_target_durability);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_EQ(plan.effects.size(), 9u);
    EXPECT_EQ(plan.effects[0].kind,
              ReinforceResetSideEffectKind::DiscardShopItem);
    EXPECT_EQ(plan.effects[1].kind,
              ReinforceResetSideEffectKind::LogItemMoneyUse);
    EXPECT_EQ(plan.effects[2].kind,
              ReinforceResetSideEffectKind::RemoveItemOption);
    EXPECT_EQ(plan.effects[2].target_durability, 7u);
    EXPECT_EQ(plan.effects[3].kind,
              ReinforceResetSideEffectKind::CharacterItemOptionDelete);
    EXPECT_EQ(plan.effects[4].kind,
              ReinforceResetSideEffectKind::ItemUpdateToDB);
    EXPECT_EQ(plan.effects[5].kind,
              ReinforceResetSideEffectKind::LogItemMoneyReset);
    EXPECT_EQ(plan.effects[6].kind,
              ReinforceResetSideEffectKind::ClearTargetDurability);
    EXPECT_EQ(plan.effects[7].kind,
              ReinforceResetSideEffectKind::SendUseAckToPlayer);
    EXPECT_EQ(plan.effects[8].kind,
              ReinforceResetSideEffectKind::SendAckToPlayer);
}

TEST(ReinforceResetPlan, NotUsableSendsNackCode1) {
    auto in = success_input();
    in.shop_item_is_useable = false;
    auto plan = reinforce_reset_side_effect_plan(in, 100, 50, 6, 999, 7);
    EXPECT_TRUE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ReinforceResetSideEffectKind::SendNackToPlayer);
    EXPECT_EQ(plan.effects[0].nack_code, 1u);
}

TEST(ReinforceResetPlan, BadItemSendsNackCode2) {
    auto in = success_input();
    in.shop_item_exists = false;
    auto plan = reinforce_reset_side_effect_plan(in, 100, 50, 6, 999, 7);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].nack_code, 2u);
}

TEST(ReinforceResetPlan, ItemInfoMissingSendsNackCode3) {
    auto in = success_input();
    in.target_item_info_exists = false;
    auto plan = reinforce_reset_side_effect_plan(in, 100, 50, 6, 999, 7);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].nack_code, 3u);
}

TEST(ReinforceResetPlan, WrongIconSendsNackCode4) {
    auto in = success_input();
    in.shop_item_icon_is_reinforce_reset = false;
    auto plan = reinforce_reset_side_effect_plan(in, 100, 50, 6, 999, 7);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].nack_code, 4u);
}

TEST(ReinforceResetPlan, NotEquipSendsNackCode5) {
    auto in = success_input();
    in.target_is_equip_kind = false;
    auto plan = reinforce_reset_side_effect_plan(in, 100, 50, 6, 999, 7);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].nack_code, 5u);
}

TEST(ReinforceResetPlan, NoOptionSendsNackCode6) {
    auto in = success_input();
    in.target_has_option = false;
    auto plan = reinforce_reset_side_effect_plan(in, 100, 50, 6, 999, 7);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].nack_code, 6u);
}

TEST(ReinforceResetPlan, DiscardFailedSendsNackCode9) {
    auto in = success_input();
    in.discard_returned_true = false;
    auto plan = reinforce_reset_side_effect_plan(in, 100, 50, 6, 999, 7);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].nack_code, 9u);
}

TEST(ReinforceResetPlan, PlanIsIdempotent) {
    auto in = success_input();
    auto a = reinforce_reset_side_effect_plan(in, 100, 50, 6, 999, 7);
    auto b = reinforce_reset_side_effect_plan(in, 100, 50, 6, 999, 7);
    EXPECT_EQ(a.send_ack, b.send_ack);
    EXPECT_EQ(a.clear_target_durability, b.clear_target_durability);
    EXPECT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].player_id, b.effects[i].player_id);
    }
}
