// D4.72 ShopItemSeal (MP_ITEM_SHOPITEM_SEAL_SYN) side-effect
// dispatcher tests.

#include <mxh/server/shop_item_seal_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

ShopItemSealValidationInput ok() {
    ShopItemSealValidationInput in{};
    in.player_found = true;
    in.seal_item_usable = true;
    in.target_item_usable = true;
    in.seal_item_resolved = true;
    in.target_item_resolved = true;
    in.target_item_info_resolved = true;
    in.seal_is_item_seal_kind = true;
    in.target_kind_ok = true;
    in.target_sell_price_forever = true;
    in.target_already_sealed = false;
    in.discard_rt = 0;
    return in;
}

TEST(ShopItemSealOutcome, AllGatesPassIsSuccess) {
    auto in = ok();
    EXPECT_EQ(classify_shop_item_seal_outcome(in),
              ShopItemSealOutcome::Success);
}

TEST(ShopItemSealOutcome, NoPlayerIsNoPlayer) {
    auto in = ok();
    in.player_found = false;
    EXPECT_EQ(classify_shop_item_seal_outcome(in),
              ShopItemSealOutcome::NoPlayer);
}

TEST(ShopItemSealOutcome, NotUsableSeal) {
    auto in = ok();
    in.seal_item_usable = false;
    EXPECT_EQ(classify_shop_item_seal_outcome(in),
              ShopItemSealOutcome::NotUsableSeal);
}

TEST(ShopItemSealOutcome, NotUsableTarget) {
    auto in = ok();
    in.target_item_usable = false;
    EXPECT_EQ(classify_shop_item_seal_outcome(in),
              ShopItemSealOutcome::NotUsableTarget);
}

TEST(ShopItemSealOutcome, NotFoundIfSealOrTargetUnresolved) {
    auto in = ok();
    in.seal_item_resolved = false;
    EXPECT_EQ(classify_shop_item_seal_outcome(in),
              ShopItemSealOutcome::NotFound);

    in.seal_item_resolved = true;
    in.target_item_resolved = false;
    EXPECT_EQ(classify_shop_item_seal_outcome(in),
              ShopItemSealOutcome::NotFound);

    in.target_item_resolved = true;
    in.target_item_info_resolved = false;
    EXPECT_EQ(classify_shop_item_seal_outcome(in),
              ShopItemSealOutcome::NotFound);
}

TEST(ShopItemSealOutcome, WrongSealItem) {
    auto in = ok();
    in.seal_is_item_seal_kind = false;
    EXPECT_EQ(classify_shop_item_seal_outcome(in),
              ShopItemSealOutcome::WrongSealItem);
}

TEST(ShopItemSealOutcome, WrongKind) {
    auto in = ok();
    in.target_kind_ok = false;
    EXPECT_EQ(classify_shop_item_seal_outcome(in),
              ShopItemSealOutcome::WrongKind);
}

TEST(ShopItemSealOutcome, NotForever) {
    auto in = ok();
    in.target_sell_price_forever = false;
    EXPECT_EQ(classify_shop_item_seal_outcome(in),
              ShopItemSealOutcome::NotForever);
}

TEST(ShopItemSealOutcome, AlreadySealed) {
    auto in = ok();
    in.target_already_sealed = true;
    EXPECT_EQ(classify_shop_item_seal_outcome(in),
              ShopItemSealOutcome::AlreadySealed);
}

TEST(ShopItemSealOutcome, DiscardFail) {
    auto in = ok();
    in.discard_rt = 1;
    EXPECT_EQ(classify_shop_item_seal_outcome(in),
              ShopItemSealOutcome::DiscardFail);
}

TEST(ShopItemSealPlan, SuccessEmitsEightSteps) {
    auto in = ok();
    auto plan = shop_item_seal_side_effect_plan(
        in, /*target_db_idx=*/12345,
        /*seal_item_idx=*/100, /*seal_item_pos=*/5);
    EXPECT_TRUE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_EQ(plan.nack_code, 0u);
    ASSERT_EQ(plan.effects.size(), 8u);
    EXPECT_EQ(plan.effects[0].kind,
              ShopItemSealSideEffectKind::LogShopItemUse);
    EXPECT_EQ(plan.effects[1].kind,
              ShopItemSealSideEffectKind::SetItemParamSeal);
    EXPECT_EQ(plan.effects[1].target_db_idx, 12345u);
    EXPECT_EQ(plan.effects[1].target_item_param,
              LEGACY_ITEM_PARAM_SEAL);
    EXPECT_EQ(plan.effects[7].kind,
              ShopItemSealSideEffectKind::BroadcastSealAck);
}

TEST(ShopItemSealPlan, NotUsableSealEmitsNackCode1) {
    auto in = ok();
    in.seal_item_usable = false;
    auto plan = shop_item_seal_side_effect_plan(in, 1, 1, 1);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_EQ(plan.nack_code, LEGACY_SEAL_NACK_NOT_USABLE_SEAL);
    EXPECT_EQ(plan.effects[0].nack_code,
              LEGACY_SEAL_NACK_NOT_USABLE_SEAL);
}

TEST(ShopItemSealPlan, NotUsableTargetEmitsNackCode2) {
    auto in = ok();
    in.target_item_usable = false;
    auto plan = shop_item_seal_side_effect_plan(in, 1, 1, 1);
    EXPECT_EQ(plan.nack_code, LEGACY_SEAL_NACK_NOT_USABLE_TARGET);
}

TEST(ShopItemSealPlan, NotFoundEmitsNackCode3) {
    auto in = ok();
    in.seal_item_resolved = false;
    auto plan = shop_item_seal_side_effect_plan(in, 1, 1, 1);
    EXPECT_EQ(plan.nack_code, LEGACY_SEAL_NACK_NOT_FOUND);
}

TEST(ShopItemSealPlan, WrongSealItemEmitsNackCode4) {
    auto in = ok();
    in.seal_is_item_seal_kind = false;
    auto plan = shop_item_seal_side_effect_plan(in, 1, 1, 1);
    EXPECT_EQ(plan.nack_code, LEGACY_SEAL_NACK_WRONG_SEAL_ITEM);
}

TEST(ShopItemSealPlan, WrongKindEmitsNackCode5) {
    auto in = ok();
    in.target_kind_ok = false;
    auto plan = shop_item_seal_side_effect_plan(in, 1, 1, 1);
    EXPECT_EQ(plan.nack_code, LEGACY_SEAL_NACK_WRONG_KIND);
}

TEST(ShopItemSealPlan, NotForeverEmitsNackCode6) {
    auto in = ok();
    in.target_sell_price_forever = false;
    auto plan = shop_item_seal_side_effect_plan(in, 1, 1, 1);
    EXPECT_EQ(plan.nack_code, LEGACY_SEAL_NACK_NOT_FOREVER);
}

TEST(ShopItemSealPlan, AlreadySealedEmitsNackCode7) {
    auto in = ok();
    in.target_already_sealed = true;
    auto plan = shop_item_seal_side_effect_plan(in, 1, 1, 1);
    EXPECT_EQ(plan.nack_code, LEGACY_SEAL_NACK_ALREADY_SEALED);
}

TEST(ShopItemSealPlan, DiscardFailEmitsNackCode9) {
    auto in = ok();
    in.discard_rt = 1;
    auto plan = shop_item_seal_side_effect_plan(in, 1, 1, 1);
    EXPECT_EQ(plan.nack_code, LEGACY_SEAL_NACK_DISCARD_FAIL);
}

TEST(ShopItemSealPlan, NoPlayerEmitsEmptyPlan) {
    auto in = ok();
    in.player_found = false;
    auto plan = shop_item_seal_side_effect_plan(in, 1, 1, 1);
    EXPECT_FALSE(plan.send_ack);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(ShopItemSealPlan, PlanIsIdempotent) {
    auto in = ok();
    auto a = shop_item_seal_side_effect_plan(in, 1, 2, 3);
    auto b = shop_item_seal_side_effect_plan(in, 1, 2, 3);
    EXPECT_EQ(a.send_ack, b.send_ack);
    EXPECT_EQ(a.send_nack, b.send_nack);
    EXPECT_EQ(a.nack_code, b.nack_code);
    ASSERT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
    }
}
