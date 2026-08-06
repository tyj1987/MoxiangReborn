// D4.76 ShopItemNameChange (MP_ITEM_SHOPITEM_NCHANGE_SYN) side-effect
// dispatcher tests.

#include <mxh/server/shop_item_name_change_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

ShopItemNameChangeValidationInput ok() {
    ShopItemNameChangeValidationInput in{};
    in.player_found = true;
    in.item_found = true;
    return in;
}

TEST(ShopItemNameChangeOutcome, PlayerAndItemIsTriggered) {
    auto in = ok();
    EXPECT_EQ(classify_shop_item_name_change_outcome(in),
              ShopItemNameChangeOutcome::Triggered);
}

TEST(ShopItemNameChangeOutcome, ItemNotFoundIsNotFound) {
    auto in = ok();
    in.item_found = false;
    EXPECT_EQ(classify_shop_item_name_change_outcome(in),
              ShopItemNameChangeOutcome::NotFound);
}

TEST(ShopItemNameChangeOutcome, NoPlayerTakesPrecedence) {
    auto in = ok();
    in.player_found = false;
    EXPECT_EQ(classify_shop_item_name_change_outcome(in),
              ShopItemNameChangeOutcome::NoPlayer);
}

TEST(ShopItemNameChangePlan, TriggeredEmitsDbCall) {
    auto in = ok();
    auto plan = shop_item_name_change_side_effect_plan(
        in, /*object_id=*/100, /*item_db_idx=*/42);
    EXPECT_TRUE(plan.trigger_db);
    EXPECT_FALSE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ShopItemNameChangeSideEffectKind::FireCharacterChangeNameDb);
    EXPECT_EQ(plan.effects[0].object_id, 100u);
    EXPECT_EQ(plan.effects[0].item_db_idx, 42u);
}

TEST(ShopItemNameChangePlan, NotFoundEmitsNackWithCode6) {
    auto in = ok();
    in.item_found = false;
    auto plan = shop_item_name_change_side_effect_plan(in, 1, 1);
    EXPECT_FALSE(plan.trigger_db);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_EQ(plan.nack_code, LEGACY_NCHANGE_ERR_NOT_FOUND);
    EXPECT_EQ(plan.effects[0].kind,
              ShopItemNameChangeSideEffectKind::BroadcastNchangeNack);
    EXPECT_EQ(plan.effects[0].nack_code, LEGACY_NCHANGE_ERR_NOT_FOUND);
}

TEST(ShopItemNameChangePlan, NoPlayerEmitsEmptyPlan) {
    auto in = ok();
    in.player_found = false;
    auto plan = shop_item_name_change_side_effect_plan(in, 1, 1);
    EXPECT_FALSE(plan.trigger_db);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(ShopItemNameChangePlan, PlanIsIdempotent) {
    auto in = ok();
    auto a = shop_item_name_change_side_effect_plan(in, 1, 2);
    auto b = shop_item_name_change_side_effect_plan(in, 1, 2);
    EXPECT_EQ(a.trigger_db, b.trigger_db);
    EXPECT_EQ(a.send_nack, b.send_nack);
    EXPECT_EQ(a.nack_code, b.nack_code);
    ASSERT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].object_id, b.effects[i].object_id);
        EXPECT_EQ(a.effects[i].item_db_idx, b.effects[i].item_db_idx);
    }
}
