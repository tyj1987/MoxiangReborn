// Tests for MP_ITEM_SHOPITEM_RARECREATE_SYN side-effect dispatcher.

#include <mxh/server/rare_create_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

RareCreateValidationInput success_input() {
    RareCreateValidationInput in{};
    in.shop_item_is_useable = true;
    in.shop_item_exists = true;
    in.target_item_exists = true;
    in.shop_item_info_exists = true;
    in.target_item_info_exists = true;
    in.shop_item_icon_is_create_50_70_90_99 = true;
    in.target_is_equip_kind = true;
    in.target_durability_zero = true;
    in.target_option_idx_zero = true;
    in.target_w_icon_idx_suffix_zero = true;
    in.level_in_range = true;
    in.is_rare_item_able = true;
    in.get_rare_returned_true = true;
    in.discard_returned_true = true;
    return in;
}

TEST(IsRareCreateSundriesIcon, AllFourValuesAreTrue) {
    EXPECT_TRUE(is_rare_create_sundries_icon(LEGACY_ESUNDRIES_RARE_CREATE_50));
    EXPECT_TRUE(is_rare_create_sundries_icon(LEGACY_ESUNDRIES_RARE_CREATE_70));
    EXPECT_TRUE(is_rare_create_sundries_icon(LEGACY_ESUNDRIES_RARE_CREATE_90));
    EXPECT_TRUE(is_rare_create_sundries_icon(LEGACY_ESUNDRIES_RARE_CREATE_99));
}

TEST(IsRareCreateSundriesIcon, OtherValuesAreFalse) {
    EXPECT_FALSE(is_rare_create_sundries_icon(0));
    EXPECT_FALSE(is_rare_create_sundries_icon(49));
    EXPECT_FALSE(is_rare_create_sundries_icon(54));
    EXPECT_FALSE(is_rare_create_sundries_icon(100));
}

TEST(RareCreateOutcome, AllGatesPassIsSuccess) {
    EXPECT_EQ(classify_rare_create_outcome(success_input()),
              RareCreateOutcome::Success);
}

TEST(RareCreateOutcome, NotUseableIsCode1) {
    auto in = success_input();
    in.shop_item_is_useable = false;
    EXPECT_EQ(classify_rare_create_outcome(in),
              RareCreateOutcome::NotUsable);
}

TEST(RareCreateOutcome, ShopItemMissingIsBadItem) {
    auto in = success_input();
    in.shop_item_exists = false;
    EXPECT_EQ(classify_rare_create_outcome(in),
              RareCreateOutcome::BadItem);
}

TEST(RareCreateOutcome, TargetItemMissingIsBadItem) {
    auto in = success_input();
    in.target_item_exists = false;
    EXPECT_EQ(classify_rare_create_outcome(in),
              RareCreateOutcome::BadItem);
}

TEST(RareCreateOutcome, ShopItemInfoMissingIsItemInfoMissing) {
    auto in = success_input();
    in.shop_item_info_exists = false;
    EXPECT_EQ(classify_rare_create_outcome(in),
              RareCreateOutcome::ItemInfoMissing);
}

TEST(RareCreateOutcome, TargetItemInfoMissingIsItemInfoMissing) {
    auto in = success_input();
    in.target_item_info_exists = false;
    EXPECT_EQ(classify_rare_create_outcome(in),
              RareCreateOutcome::ItemInfoMissing);
}

TEST(RareCreateOutcome, WrongIconIsWrongIcon) {
    auto in = success_input();
    in.shop_item_icon_is_create_50_70_90_99 = false;
    EXPECT_EQ(classify_rare_create_outcome(in),
              RareCreateOutcome::WrongIcon);
}

TEST(RareCreateOutcome, NotEquipIsNotEquip) {
    auto in = success_input();
    in.target_is_equip_kind = false;
    EXPECT_EQ(classify_rare_create_outcome(in),
              RareCreateOutcome::NotEquip);
}

TEST(RareCreateOutcome, NonZeroDurabilityIsAlreadyRare) {
    auto in = success_input();
    in.target_durability_zero = false;
    EXPECT_EQ(classify_rare_create_outcome(in),
              RareCreateOutcome::AlreadyRare);
}

TEST(RareCreateOutcome, NonZeroOptionIdxIsAlreadyRare) {
    auto in = success_input();
    in.target_option_idx_zero = false;
    EXPECT_EQ(classify_rare_create_outcome(in),
              RareCreateOutcome::AlreadyRare);
}

TEST(RareCreateOutcome, SuffixNonZeroIsWrongIconSuffix) {
    auto in = success_input();
    in.target_w_icon_idx_suffix_zero = false;
    EXPECT_EQ(classify_rare_create_outcome(in),
              RareCreateOutcome::WrongIconSuffix);
}

TEST(RareCreateOutcome, LevelOutOfRangeIsLevelOutOfRange) {
    auto in = success_input();
    in.level_in_range = false;
    EXPECT_EQ(classify_rare_create_outcome(in),
              RareCreateOutcome::LevelOutOfRange);
}

TEST(RareCreateOutcome, NotRareAbleIsNotRareAble) {
    auto in = success_input();
    in.is_rare_item_able = false;
    EXPECT_EQ(classify_rare_create_outcome(in),
              RareCreateOutcome::NotRareAble);
}

TEST(RareCreateOutcome, GetRareFailedIsGetRareFailed) {
    auto in = success_input();
    in.get_rare_returned_true = false;
    EXPECT_EQ(classify_rare_create_outcome(in),
              RareCreateOutcome::GetRareFailed);
}

TEST(RareCreateOutcome, DiscardFailedIsDiscardFailed) {
    auto in = success_input();
    in.discard_returned_true = false;
    EXPECT_EQ(classify_rare_create_outcome(in),
              RareCreateOutcome::DiscardFailed);
}

TEST(RareCreateOutcome, NotUsableTakesPrecedenceOverOthers) {
    auto in = success_input();
    in.shop_item_is_useable = false;
    in.discard_returned_true = false;
    EXPECT_EQ(classify_rare_create_outcome(in),
              RareCreateOutcome::NotUsable);
}

TEST(RareCreateNackCode, MapsOutcomesToLegacyCodes) {
    EXPECT_EQ(rare_create_nack_code(
        RareCreateOutcome::NotUsable), 1u);
    EXPECT_EQ(rare_create_nack_code(
        RareCreateOutcome::BadItem), 2u);
    EXPECT_EQ(rare_create_nack_code(
        RareCreateOutcome::ItemInfoMissing), 3u);
    EXPECT_EQ(rare_create_nack_code(
        RareCreateOutcome::WrongIcon), 4u);
    EXPECT_EQ(rare_create_nack_code(
        RareCreateOutcome::NotEquip), 5u);
    EXPECT_EQ(rare_create_nack_code(
        RareCreateOutcome::AlreadyRare), 6u);
    EXPECT_EQ(rare_create_nack_code(
        RareCreateOutcome::WrongIconSuffix), 7u);
    EXPECT_EQ(rare_create_nack_code(
        RareCreateOutcome::LevelOutOfRange), 8u);
    EXPECT_EQ(rare_create_nack_code(
        RareCreateOutcome::NotRareAble), 9u);
    EXPECT_EQ(rare_create_nack_code(
        RareCreateOutcome::GetRareFailed), 10u);
    EXPECT_EQ(rare_create_nack_code(
        RareCreateOutcome::DiscardFailed), 11u);
    EXPECT_EQ(rare_create_nack_code(
        RareCreateOutcome::Success), 0u);
}

TEST(RareCreatePlan, SuccessEmitsGenerateThenDiscardThenDbThenLogThenUseAck) {
    auto in = success_input();
    auto plan = rare_create_side_effect_plan(in, 100, 50, 6, 1234, 7, 999);
    EXPECT_TRUE(plan.send_use_ack);
    EXPECT_TRUE(plan.generate_rare_option);
    EXPECT_TRUE(plan.discard_shop_item);
    EXPECT_TRUE(plan.db_rare_insert);
    EXPECT_TRUE(plan.log_item_money);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_EQ(plan.effects.size(), 5u);
    EXPECT_EQ(plan.effects[0].kind,
              RareCreateSideEffectKind::GenerateRareOption);
    EXPECT_EQ(plan.effects[0].target_w_icon_idx, 1234u);
    EXPECT_EQ(plan.effects[1].kind,
              RareCreateSideEffectKind::DiscardShopItem);
    EXPECT_EQ(plan.effects[1].shop_item_idx, 50u);
    EXPECT_EQ(plan.effects[2].kind,
              RareCreateSideEffectKind::ShopItemRareInsertToDB);
    EXPECT_EQ(plan.effects[2].target_db_idx, 999u);
    EXPECT_EQ(plan.effects[2].target_position, 7u);
    EXPECT_EQ(plan.effects[3].kind,
              RareCreateSideEffectKind::LogItemMoney);
    EXPECT_EQ(plan.effects[4].kind,
              RareCreateSideEffectKind::SendUseAckToPlayer);
    EXPECT_EQ(plan.effects[4].shop_item_pos, 6u);
}

TEST(RareCreatePlan, NotUsableSendsNackCode1) {
    auto in = success_input();
    in.shop_item_is_useable = false;
    auto plan = rare_create_side_effect_plan(in, 100, 50, 6, 1234, 7, 999);
    EXPECT_TRUE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              RareCreateSideEffectKind::SendNackToPlayer);
    EXPECT_EQ(plan.effects[0].nack_code, 1u);
}

TEST(RareCreatePlan, AlreadyRareSendsNackCode6) {
    auto in = success_input();
    in.target_durability_zero = false;
    auto plan = rare_create_side_effect_plan(in, 100, 50, 6, 1234, 7, 999);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].nack_code, 6u);
}

TEST(RareCreatePlan, GetRareFailedSendsNackCode10) {
    auto in = success_input();
    in.get_rare_returned_true = false;
    auto plan = rare_create_side_effect_plan(in, 100, 50, 6, 1234, 7, 999);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].nack_code, 10u);
}

TEST(RareCreatePlan, DiscardFailedSendsNackCode11) {
    auto in = success_input();
    in.discard_returned_true = false;
    auto plan = rare_create_side_effect_plan(in, 100, 50, 6, 1234, 7, 999);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].nack_code, 11u);
}

TEST(RareCreatePlan, PlanIsIdempotent) {
    auto in = success_input();
    auto a = rare_create_side_effect_plan(in, 100, 50, 6, 1234, 7, 999);
    auto b = rare_create_side_effect_plan(in, 100, 50, 6, 1234, 7, 999);
    EXPECT_EQ(a.send_use_ack, b.send_use_ack);
    EXPECT_EQ(a.db_rare_insert, b.db_rare_insert);
    EXPECT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].player_id, b.effects[i].player_id);
    }
}
