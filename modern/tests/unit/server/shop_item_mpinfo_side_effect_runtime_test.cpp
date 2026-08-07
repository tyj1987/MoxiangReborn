// shop_item_mpinfo_side_effect_runtime_test.cpp
//
// Verifies apply_shop_item_mpinfo_side_effects() (the runtime
// orchestrator for the CItemManager::MP_ITEM_SHOPITEM_MPINFO
// side-effect chain) walks the data-plane plan and dispatches the
// SavedMovePointInfo DB query when the player exists, and stays a
// no-op otherwise (no ACK/NACK; data arrives later via DB callback).

#include <mxh/server/shop_item_mpinfo_side_effect.hpp>
#include <mxh/server/shop_item_mpinfo_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>

namespace {

using mxh::server::ShopItemMpInfoSideEffectKind;
using mxh::server::ShopItemMpInfoSideEffectSink;
using mxh::server::apply_shop_item_mpinfo_side_effects;
using mxh::server::shop_item_mpinfo_side_effect_plan;

class RecordingSink final : public ShopItemMpInfoSideEffectSink {
public:
    std::string last_call;
    std::uint32_t last_object_id = 0;
    std::size_t db_count = 0;

    void fire_saved_move_point_db_query(
        std::uint32_t object_id) override {
        last_call = "db";
        last_object_id = object_id;
        ++db_count;
    }
};

}  // namespace

TEST(ApplyShopItemMpInfoSideEffects, PlayerFoundEmitsDbQuery) {
    mxh::server::ShopItemMpInfoValidationInput in;
    in.player_found = true;
    auto plan = shop_item_mpinfo_side_effect_plan(
        in, /*object_id=*/0x00010203u);
    EXPECT_TRUE(plan.trigger_db);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ShopItemMpInfoSideEffectKind::FireSavedMovePointDbQuery);
    EXPECT_EQ(plan.effects[0].object_id, 0x00010203u);

    RecordingSink sink;
    auto out = apply_shop_item_mpinfo_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.db_queries, 1u);
    EXPECT_TRUE(out.trigger_db_flag_consumed);
    EXPECT_EQ(sink.last_call, "db");
    EXPECT_EQ(sink.last_object_id, 0x00010203u);
    EXPECT_EQ(sink.db_count, 1u);
}

TEST(ApplyShopItemMpInfoSideEffects, NoPlayerIsNoOp) {
    mxh::server::ShopItemMpInfoValidationInput in;
    in.player_found = false;
    auto plan = shop_item_mpinfo_side_effect_plan(in, 7);
    EXPECT_FALSE(plan.trigger_db);
    EXPECT_TRUE(plan.effects.empty());

    RecordingSink sink;
    auto out = apply_shop_item_mpinfo_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.db_queries, 0u);
    EXPECT_FALSE(out.trigger_db_flag_consumed);
    EXPECT_EQ(sink.last_call, "");
    EXPECT_EQ(sink.db_count, 0u);
}

TEST(ApplyShopItemMpInfoSideEffects, EmptyPlanIsNoOp) {
    mxh::server::ShopItemMpInfoSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_shop_item_mpinfo_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.db_queries, 0u);
    EXPECT_FALSE(out.trigger_db_flag_consumed);
    EXPECT_EQ(sink.last_call, "");
    EXPECT_EQ(sink.db_count, 0u);
}

TEST(ApplyShopItemMpInfoSideEffects, MaxObjectIdStillDispatches) {
    mxh::server::ShopItemMpInfoValidationInput in;
    in.player_found = true;
    auto plan = shop_item_mpinfo_side_effect_plan(in, 0xFFFFFFFFu);
    EXPECT_TRUE(plan.trigger_db);

    RecordingSink sink;
    (void)apply_shop_item_mpinfo_side_effects(plan, sink);
    EXPECT_EQ(sink.last_call, "db");
    EXPECT_EQ(sink.last_object_id, 0xFFFFFFFFu);
    EXPECT_EQ(sink.db_count, 1u);
}

TEST(ApplyShopItemMpInfoSideEffects, ZeroObjectIdStillDispatches) {
    mxh::server::ShopItemMpInfoValidationInput in;
    in.player_found = true;
    auto plan = shop_item_mpinfo_side_effect_plan(in, 0);
    EXPECT_TRUE(plan.trigger_db);

    RecordingSink sink;
    (void)apply_shop_item_mpinfo_side_effects(plan, sink);
    EXPECT_EQ(sink.last_call, "db");
    EXPECT_EQ(sink.last_object_id, 0u);
    EXPECT_EQ(sink.db_count, 1u);
}
