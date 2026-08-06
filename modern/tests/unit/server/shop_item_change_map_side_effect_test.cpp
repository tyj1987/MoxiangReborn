// D4.59 ShopItemChangeMap (MP_ITEM_SHOPITEM_CHANGEMAP_SYN) side-effect
// dispatcher tests.

#include <mxh/server/shop_item_change_map_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

ShopItemChangeMapValidationInput ok() {
    ShopItemChangeMapValidationInput in{};
    in.player_found = true;
    return in;
}

TEST(ShopItemChangeMapOutcome, PlayerFoundIsConsumed) {
    auto in = ok();
    EXPECT_EQ(classify_shop_item_change_map_outcome(in),
              ShopItemChangeMapOutcome::Consumed);
}

TEST(ShopItemChangeMapOutcome, PlayerNotFoundIsNoPlayer) {
    auto in = ok();
    in.player_found = false;
    EXPECT_EQ(classify_shop_item_change_map_outcome(in),
              ShopItemChangeMapOutcome::NoPlayer);
}

TEST(ShopItemChangeMapPlan, ConsumedEmitsSilentConsume) {
    auto in = ok();
    auto plan = shop_item_change_map_side_effect_plan(
        in, /*object_id=*/12345);
    EXPECT_TRUE(plan.consume);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ShopItemChangeMapSideEffectKind::SilentConsume);
    EXPECT_EQ(plan.effects[0].object_id, 12345u);
}

TEST(ShopItemChangeMapPlan, NoPlayerEmitsEmptyPlan) {
    auto in = ok();
    in.player_found = false;
    auto plan = shop_item_change_map_side_effect_plan(in, 1);
    EXPECT_FALSE(plan.consume);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(ShopItemChangeMapPlan, PlanIsIdempotent) {
    auto in = ok();
    auto a = shop_item_change_map_side_effect_plan(in, 42);
    auto b = shop_item_change_map_side_effect_plan(in, 42);
    EXPECT_EQ(a.consume, b.consume);
    ASSERT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].object_id, b.effects[i].object_id);
    }
}
