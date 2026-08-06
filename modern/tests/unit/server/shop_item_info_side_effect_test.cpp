// D4.64 ShopItemInfo (MP_ITEM_SHOPITEM_INFO_SYN) side-effect
// dispatcher tests.

#include <mxh/server/shop_item_info_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(ShopItemInfoOutcome, PlayerFoundIsTriggered) {
    ShopItemInfoValidationInput in{};
    in.player_found = true;
    EXPECT_EQ(classify_shop_item_info_outcome(in),
              ShopItemInfoOutcome::Triggered);
}

TEST(ShopItemInfoOutcome, NoPlayerIsNoPlayer) {
    ShopItemInfoValidationInput in{};
    in.player_found = false;
    EXPECT_EQ(classify_shop_item_info_outcome(in),
              ShopItemInfoOutcome::NoPlayer);
}

TEST(ShopItemInfoPlan, TriggeredEmitsTwoSteps) {
    ShopItemInfoValidationInput in{};
    in.player_found = true;
    auto plan = shop_item_info_side_effect_plan(in, /*object_id=*/42);
    EXPECT_TRUE(plan.reset_init);
    EXPECT_TRUE(plan.trigger_db);
    ASSERT_EQ(plan.effects.size(), 2u);
    EXPECT_EQ(plan.effects[0].kind,
              ShopItemInfoSideEffectKind::SetShopItemInit);
    EXPECT_EQ(plan.effects[1].kind,
              ShopItemInfoSideEffectKind::FireShopItemDbQuery);
    EXPECT_EQ(plan.effects[0].object_id, 42u);
    EXPECT_EQ(plan.effects[1].object_id, 42u);
}

TEST(ShopItemInfoPlan, NoPlayerEmitsEmptyPlan) {
    ShopItemInfoValidationInput in{};
    in.player_found = false;
    auto plan = shop_item_info_side_effect_plan(in, 1);
    EXPECT_FALSE(plan.reset_init);
    EXPECT_FALSE(plan.trigger_db);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(ShopItemInfoPlan, PlanIsIdempotent) {
    ShopItemInfoValidationInput in{};
    in.player_found = true;
    auto a = shop_item_info_side_effect_plan(in, 1);
    auto b = shop_item_info_side_effect_plan(in, 1);
    EXPECT_EQ(a.reset_init, b.reset_init);
    EXPECT_EQ(a.trigger_db, b.trigger_db);
    ASSERT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].object_id, b.effects[i].object_id);
    }
}
