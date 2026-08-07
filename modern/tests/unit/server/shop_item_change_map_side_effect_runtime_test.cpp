// shop_item_change_map_side_effect_runtime_test.cpp
//
// Verifies apply_shop_item_change_map_side_effects() (the runtime
// orchestrator for the CItemManager::MP_ITEM_SHOPITEM_CHANGEMAP_SYN
// side-effect chain) walks the data-plane plan and dispatches the
// SilentConsume entry when the player exists, and stays a no-op
// otherwise (no-op receipt, no network response).

#include <mxh/server/shop_item_change_map_side_effect.hpp>
#include <mxh/server/shop_item_change_map_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>

namespace {

using mxh::server::ShopItemChangeMapSideEffectKind;
using mxh::server::ShopItemChangeMapSideEffectSink;
using mxh::server::apply_shop_item_change_map_side_effects;
using mxh::server::shop_item_change_map_side_effect_plan;

class RecordingSink final : public ShopItemChangeMapSideEffectSink {
public:
    std::string last_call;
    std::uint32_t last_object_id = 0;
    std::size_t consume_count = 0;

    void silent_consume(std::uint32_t object_id) override {
        last_call = "consume";
        last_object_id = object_id;
        ++consume_count;
    }
};

}  // namespace

TEST(ApplyShopItemChangeMapSideEffects, PlayerFoundEmitsSilentConsume) {
    mxh::server::ShopItemChangeMapValidationInput in;
    in.player_found = true;
    auto plan = shop_item_change_map_side_effect_plan(
        in, /*object_id=*/0x00010203u);
    EXPECT_TRUE(plan.consume);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ShopItemChangeMapSideEffectKind::SilentConsume);
    EXPECT_EQ(plan.effects[0].object_id, 0x00010203u);

    RecordingSink sink;
    auto out = apply_shop_item_change_map_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.consumes, 1u);
    EXPECT_TRUE(out.consume_flag_consumed);
    EXPECT_EQ(sink.last_call, "consume");
    EXPECT_EQ(sink.last_object_id, 0x00010203u);
    EXPECT_EQ(sink.consume_count, 1u);
}

TEST(ApplyShopItemChangeMapSideEffects, NoPlayerIsNoOp) {
    // Legacy: FindUser null -> early return, no network response.
    mxh::server::ShopItemChangeMapValidationInput in;
    in.player_found = false;
    auto plan = shop_item_change_map_side_effect_plan(in, 7);
    EXPECT_FALSE(plan.consume);
    EXPECT_TRUE(plan.effects.empty());

    RecordingSink sink;
    auto out = apply_shop_item_change_map_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.consumes, 0u);
    EXPECT_FALSE(out.consume_flag_consumed);
    EXPECT_EQ(sink.last_call, "");
    EXPECT_EQ(sink.consume_count, 0u);
}

TEST(ApplyShopItemChangeMapSideEffects, EmptyPlanIsNoOp) {
    mxh::server::ShopItemChangeMapSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_shop_item_change_map_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.consumes, 0u);
    EXPECT_FALSE(out.consume_flag_consumed);
    EXPECT_EQ(sink.last_call, "");
    EXPECT_EQ(sink.consume_count, 0u);
}

TEST(ApplyShopItemChangeMapSideEffects, MaxObjectIdStillDispatches) {
    mxh::server::ShopItemChangeMapValidationInput in;
    in.player_found = true;
    auto plan = shop_item_change_map_side_effect_plan(
        in, 0xFFFFFFFFu);
    EXPECT_TRUE(plan.consume);

    RecordingSink sink;
    (void)apply_shop_item_change_map_side_effects(plan, sink);
    EXPECT_EQ(sink.last_call, "consume");
    EXPECT_EQ(sink.last_object_id, 0xFFFFFFFFu);
    EXPECT_EQ(sink.consume_count, 1u);
}

TEST(ApplyShopItemChangeMapSideEffects, ZeroObjectIdStillDispatches) {
    mxh::server::ShopItemChangeMapValidationInput in;
    in.player_found = true;
    auto plan = shop_item_change_map_side_effect_plan(in, 0);
    EXPECT_TRUE(plan.consume);

    RecordingSink sink;
    (void)apply_shop_item_change_map_side_effects(plan, sink);
    EXPECT_EQ(sink.last_call, "consume");
    EXPECT_EQ(sink.last_object_id, 0u);
    EXPECT_EQ(sink.consume_count, 1u);
}
