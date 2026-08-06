// D4.65 ShopItemMpInfo (MP_ITEM_SHOPITEM_MPINFO) side-effect
// dispatcher tests.

#include <mxh/server/shop_item_mpinfo_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(ShopItemMpInfoOutcome, PlayerFoundIsTriggered) {
    ShopItemMpInfoValidationInput in{};
    in.player_found = true;
    EXPECT_EQ(classify_shop_item_mpinfo_outcome(in),
              ShopItemMpInfoOutcome::Triggered);
}

TEST(ShopItemMpInfoOutcome, NoPlayerIsNoPlayer) {
    ShopItemMpInfoValidationInput in{};
    in.player_found = false;
    EXPECT_EQ(classify_shop_item_mpinfo_outcome(in),
              ShopItemMpInfoOutcome::NoPlayer);
}

TEST(ShopItemMpInfoPlan, TriggeredEmitsDbQuery) {
    ShopItemMpInfoValidationInput in{};
    in.player_found = true;
    auto plan = shop_item_mpinfo_side_effect_plan(in, /*object_id=*/42);
    EXPECT_TRUE(plan.trigger_db);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ShopItemMpInfoSideEffectKind::FireSavedMovePointDbQuery);
    EXPECT_EQ(plan.effects[0].object_id, 42u);
}

TEST(ShopItemMpInfoPlan, NoPlayerEmitsEmptyPlan) {
    ShopItemMpInfoValidationInput in{};
    in.player_found = false;
    auto plan = shop_item_mpinfo_side_effect_plan(in, 1);
    EXPECT_FALSE(plan.trigger_db);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(ShopItemMpInfoPlan, PlanIsIdempotent) {
    ShopItemMpInfoValidationInput in{};
    in.player_found = true;
    auto a = shop_item_mpinfo_side_effect_plan(in, 1);
    auto b = shop_item_mpinfo_side_effect_plan(in, 1);
    EXPECT_EQ(a.trigger_db, b.trigger_db);
    ASSERT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].object_id, b.effects[i].object_id);
    }
}
