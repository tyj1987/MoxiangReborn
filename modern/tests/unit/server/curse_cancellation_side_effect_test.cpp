// Tests for MP_ITEMEXT_SHOPITEM_CURSE_CANCELLATION_SYN side-effect.

#include <mxh/server/curse_cancellation_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

CurseCancellationValidationInput success_input() {
    CurseCancellationValidationInput in{};
    in.item_exists_at_target = true;
    in.unique_item_info_exists = true;
    in.unique_item_is_cursed = true;
    in.discard_shop_returned_true = true;
    in.obtain_space_available = true;
    return in;
}

TEST(CurseCancellationOutcome, AllGatesPassIsFullCancel) {
    EXPECT_EQ(classify_curse_cancellation_outcome(success_input()),
              CurseCancellationOutcome::FullCancel);
}

TEST(CurseCancellationOutcome, ItemMissingIsItemNotExist) {
    auto in = success_input();
    in.item_exists_at_target = false;
    EXPECT_EQ(classify_curse_cancellation_outcome(in),
              CurseCancellationOutcome::ItemNotExist);
}

TEST(CurseCancellationOutcome, InfoMissingIsUniqueItemInvalid) {
    auto in = success_input();
    in.unique_item_info_exists = false;
    EXPECT_EQ(classify_curse_cancellation_outcome(in),
              CurseCancellationOutcome::UniqueItemInvalid);
}

TEST(CurseCancellationOutcome, NotCursedIsUniqueItemInvalid) {
    auto in = success_input();
    in.unique_item_is_cursed = false;
    EXPECT_EQ(classify_curse_cancellation_outcome(in),
              CurseCancellationOutcome::UniqueItemInvalid);
}

TEST(CurseCancellationOutcome, DiscardShopFailedIsDiscardShopFailed) {
    auto in = success_input();
    in.discard_shop_returned_true = false;
    EXPECT_EQ(classify_curse_cancellation_outcome(in),
              CurseCancellationOutcome::DiscardShopFailed);
}

TEST(CurseCancellationOutcome, NoSpaceIsNoSpaceForRestore) {
    auto in = success_input();
    in.obtain_space_available = false;
    EXPECT_EQ(classify_curse_cancellation_outcome(in),
              CurseCancellationOutcome::NoSpaceForRestore);
}

TEST(CurseCancellationOutcome, ItemNotExistTakesPrecedence) {
    auto in = success_input();
    in.item_exists_at_target = false;
    in.obtain_space_available = false;
    EXPECT_EQ(classify_curse_cancellation_outcome(in),
              CurseCancellationOutcome::ItemNotExist);
}

TEST(CurseCancellationNackCode, MapsOutcomesToLegacyCodes) {
    EXPECT_EQ(curse_cancellation_nack_code(
        CurseCancellationOutcome::ItemNotExist), 1u);
    EXPECT_EQ(curse_cancellation_nack_code(
        CurseCancellationOutcome::UniqueItemInvalid), 2u);
    EXPECT_EQ(curse_cancellation_nack_code(
        CurseCancellationOutcome::DiscardShopFailed), 3u);
    EXPECT_EQ(curse_cancellation_nack_code(
        CurseCancellationOutcome::FullCancel), 0u);
    EXPECT_EQ(curse_cancellation_nack_code(
        CurseCancellationOutcome::NoSpaceForRestore), 0u);
}

TEST(CurseCancellationPlan, FullCancelEmitsAllSevenEffects) {
    auto in = success_input();
    auto plan = curse_cancellation_side_effect_plan(in, 100, 50, 6, 42);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_TRUE(plan.send_use_ack);
    EXPECT_TRUE(plan.discard_shop);
    EXPECT_TRUE(plan.log_use);
    EXPECT_TRUE(plan.discard_cursed);
    EXPECT_TRUE(plan.send_delete_ack);
    EXPECT_TRUE(plan.log_discard);
    EXPECT_TRUE(plan.obtain_ex);
    EXPECT_EQ(plan.effects.size(), 7u);
    EXPECT_EQ(plan.effects[0].kind,
              CurseCancellationSideEffectKind::DiscardShopItem);
    EXPECT_EQ(plan.effects[1].kind,
              CurseCancellationSideEffectKind::LogItemMoneyUse);
    EXPECT_EQ(plan.effects[2].kind,
              CurseCancellationSideEffectKind::SendUseAckToPlayer);
    EXPECT_EQ(plan.effects[3].kind,
              CurseCancellationSideEffectKind::DiscardCursedItem);
    EXPECT_EQ(plan.effects[4].kind,
              CurseCancellationSideEffectKind::SendDeleteItemAck);
    EXPECT_EQ(plan.effects[5].kind,
              CurseCancellationSideEffectKind::LogItemMoneyDiscard);
    EXPECT_EQ(plan.effects[6].kind,
              CurseCancellationSideEffectKind::ObtainItemEx);
    EXPECT_EQ(plan.effects[6].curse_cancellation_count, 42u);
}

TEST(CurseCancellationPlan, NoSpaceOmitsObtainEx) {
    auto in = success_input();
    in.obtain_space_available = false;
    auto plan = curse_cancellation_side_effect_plan(in, 100, 50, 6, 42);
    EXPECT_TRUE(plan.discard_shop);
    EXPECT_TRUE(plan.send_delete_ack);
    EXPECT_FALSE(plan.obtain_ex);
    EXPECT_EQ(plan.effects.size(), 6u);
    EXPECT_EQ(plan.effects[0].kind,
              CurseCancellationSideEffectKind::DiscardShopItem);
    EXPECT_EQ(plan.effects[5].kind,
              CurseCancellationSideEffectKind::LogItemMoneyDiscard);
}

TEST(CurseCancellationPlan, ItemNotExistSendsNackCode1) {
    auto in = success_input();
    in.item_exists_at_target = false;
    auto plan = curse_cancellation_side_effect_plan(in, 100, 50, 6, 42);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_EQ(plan.nack_code, 1u);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              CurseCancellationSideEffectKind::SendNackToPlayer);
    EXPECT_EQ(plan.effects[0].nack_code, 1u);
}

TEST(CurseCancellationPlan, UniqueItemInvalidSendsNackCode2) {
    auto in = success_input();
    in.unique_item_is_cursed = false;
    auto plan = curse_cancellation_side_effect_plan(in, 100, 50, 6, 42);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_EQ(plan.nack_code, 2u);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].nack_code, 2u);
}

TEST(CurseCancellationPlan, DiscardShopFailedSendsNackCode3) {
    auto in = success_input();
    in.discard_shop_returned_true = false;
    auto plan = curse_cancellation_side_effect_plan(in, 100, 50, 6, 42);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_EQ(plan.nack_code, 3u);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].nack_code, 3u);
}

TEST(CurseCancellationPlan, PlanIsIdempotent) {
    auto in = success_input();
    auto a = curse_cancellation_side_effect_plan(in, 100, 50, 6, 42);
    auto b = curse_cancellation_side_effect_plan(in, 100, 50, 6, 42);
    EXPECT_EQ(a.send_use_ack, b.send_use_ack);
    EXPECT_EQ(a.obtain_ex, b.obtain_ex);
    EXPECT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].player_id, b.effects[i].player_id);
    }
}
