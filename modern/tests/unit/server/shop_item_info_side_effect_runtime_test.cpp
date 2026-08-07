// shop_item_info_side_effect_runtime_test.cpp
//
// Verifies apply_shop_item_info_side_effects() (the runtime
// orchestrator for the CItemManager::MP_ITEM_SHOPITEM_INFO_SYN
// side-effect chain) walks the data-plane plan and dispatches the
// 2-step chain (SetShopItemInit -> DB query) in legacy order, and
// stays a no-op when the player is missing.

#include <mxh/server/shop_item_info_side_effect.hpp>
#include <mxh/server/shop_item_info_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {

using mxh::server::ShopItemInfoSideEffectKind;
using mxh::server::ShopItemInfoSideEffectSink;
using mxh::server::apply_shop_item_info_side_effects;
using mxh::server::shop_item_info_side_effect_plan;

class RecordingSink final : public ShopItemInfoSideEffectSink {
public:
    std::vector<std::string> calls;
    std::uint32_t last_object_id = 0;
    std::uint32_t last_start_db_idx = 0;

    void set_shop_item_init(std::uint32_t object_id) override {
        calls.push_back("init");
        last_object_id = object_id;
    }
    void fire_shop_item_db_query(std::uint32_t object_id,
                                 std::uint32_t start_db_idx) override {
        calls.push_back("db");
        last_object_id = object_id;
        last_start_db_idx = start_db_idx;
    }
};

}  // namespace

TEST(ApplyShopItemInfoSideEffects, PlayerFoundEmitsInitThenDbInOrder) {
    mxh::server::ShopItemInfoValidationInput in;
    in.player_found = true;
    auto plan = shop_item_info_side_effect_plan(
        in, /*object_id=*/0x00010203u);
    EXPECT_TRUE(plan.reset_init);
    EXPECT_TRUE(plan.trigger_db);
    ASSERT_EQ(plan.effects.size(), 2u);
    EXPECT_EQ(plan.effects[0].kind,
              ShopItemInfoSideEffectKind::SetShopItemInit);
    EXPECT_EQ(plan.effects[1].kind,
              ShopItemInfoSideEffectKind::FireShopItemDbQuery);

    RecordingSink sink;
    auto out = apply_shop_item_info_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 2u);
    EXPECT_EQ(out.init_resets, 1u);
    EXPECT_EQ(out.db_queries, 1u);
    EXPECT_TRUE(out.reset_init_flag_consumed);
    EXPECT_TRUE(out.trigger_db_flag_consumed);
    ASSERT_EQ(sink.calls.size(), 2u);
    EXPECT_EQ(sink.calls[0], "init");
    EXPECT_EQ(sink.calls[1], "db");
    EXPECT_EQ(sink.last_object_id, 0x00010203u);
    EXPECT_EQ(sink.last_start_db_idx, 0u);  // legacy arg 0
}

TEST(ApplyShopItemInfoSideEffects, NoPlayerIsNoOp) {
    mxh::server::ShopItemInfoValidationInput in;
    in.player_found = false;
    auto plan = shop_item_info_side_effect_plan(in, 7);
    EXPECT_FALSE(plan.reset_init);
    EXPECT_FALSE(plan.trigger_db);
    EXPECT_TRUE(plan.effects.empty());

    RecordingSink sink;
    auto out = apply_shop_item_info_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.init_resets, 0u);
    EXPECT_EQ(out.db_queries, 0u);
    EXPECT_FALSE(out.reset_init_flag_consumed);
    EXPECT_FALSE(out.trigger_db_flag_consumed);
    EXPECT_TRUE(sink.calls.empty());
}

TEST(ApplyShopItemInfoSideEffects, EmptyPlanIsNoOp) {
    mxh::server::ShopItemInfoSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_shop_item_info_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.init_resets, 0u);
    EXPECT_EQ(out.db_queries, 0u);
    EXPECT_FALSE(out.reset_init_flag_consumed);
    EXPECT_FALSE(out.trigger_db_flag_consumed);
    EXPECT_TRUE(sink.calls.empty());
}

TEST(ApplyShopItemInfoSideEffects, MaxObjectIdStillDispatches) {
    mxh::server::ShopItemInfoValidationInput in;
    in.player_found = true;
    auto plan = shop_item_info_side_effect_plan(in, 0xFFFFFFFFu);
    EXPECT_TRUE(plan.reset_init);
    EXPECT_TRUE(plan.trigger_db);

    RecordingSink sink;
    (void)apply_shop_item_info_side_effects(plan, sink);
    EXPECT_EQ(sink.calls,
              std::vector<std::string>({"init", "db"}));
    EXPECT_EQ(sink.last_object_id, 0xFFFFFFFFu);
    EXPECT_EQ(sink.last_start_db_idx, 0u);
}

TEST(ApplyShopItemInfoSideEffects, ZeroObjectIdStillDispatches) {
    mxh::server::ShopItemInfoValidationInput in;
    in.player_found = true;
    auto plan = shop_item_info_side_effect_plan(in, 0);
    EXPECT_TRUE(plan.reset_init);

    RecordingSink sink;
    (void)apply_shop_item_info_side_effects(plan, sink);
    EXPECT_EQ(sink.calls,
              std::vector<std::string>({"init", "db"}));
    EXPECT_EQ(sink.last_object_id, 0u);
}
