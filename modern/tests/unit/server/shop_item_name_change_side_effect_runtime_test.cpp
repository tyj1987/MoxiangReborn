// shop_item_name_change_side_effect_runtime_test.cpp
//
// Verifies apply_shop_item_name_change_side_effects() (the runtime
// orchestrator for the CItemManager::MP_ITEM_SHOPITEM_NCHANGE_SYN
// side-effect chain) walks the data-plane plan and dispatches the
// single entry: DB call when the name-change item is found / NACK
// with dwData=6 when not found / no-op when the player is missing.

#include <mxh/server/shop_item_name_change_side_effect.hpp>
#include <mxh/server/shop_item_name_change_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>

namespace {

using mxh::server::ShopItemNameChangeSideEffectKind;
using mxh::server::ShopItemNameChangeSideEffectSink;
using mxh::server::LEGACY_NCHANGE_ERR_NOT_FOUND;
using mxh::server::apply_shop_item_name_change_side_effects;
using mxh::server::shop_item_name_change_side_effect_plan;

class RecordingSink final : public ShopItemNameChangeSideEffectSink {
public:
    std::string last_call;
    std::uint32_t last_object_id = 0;
    std::uint32_t last_item_db_idx = 0;
    std::uint32_t last_nack_code = 0;
    std::size_t db_count = 0;
    std::size_t nack_count = 0;

    void fire_character_change_name_db(
        std::uint32_t object_id, std::uint32_t item_db_idx) override {
        last_call = "db";
        last_object_id = object_id;
        last_item_db_idx = item_db_idx;
        ++db_count;
    }
    void broadcast_nchange_nack(std::uint32_t object_id,
                                std::uint32_t item_db_idx,
                                std::uint32_t nack_code) override {
        last_call = "nack";
        last_object_id = object_id;
        last_item_db_idx = item_db_idx;
        last_nack_code = nack_code;
        ++nack_count;
    }
};

}  // namespace

TEST(ApplyShopItemNameChangeSideEffects, ItemFoundEmitsDbCall) {
    mxh::server::ShopItemNameChangeValidationInput in;
    in.player_found = true;
    in.item_found = true;
    auto plan = shop_item_name_change_side_effect_plan(
        in, /*object_id=*/0x00010002u, /*item_db_idx=*/0x00000003u);
    EXPECT_TRUE(plan.trigger_db);
    EXPECT_FALSE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ShopItemNameChangeSideEffectKind::FireCharacterChangeNameDb);
    EXPECT_EQ(plan.effects[0].object_id, 0x00010002u);
    EXPECT_EQ(plan.effects[0].item_db_idx, 0x00000003u);

    RecordingSink sink;
    auto out = apply_shop_item_name_change_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.db_fires, 1u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_TRUE(out.db_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_EQ(sink.last_call, "db");
    EXPECT_EQ(sink.last_object_id, 0x00010002u);
    EXPECT_EQ(sink.last_item_db_idx, 0x00000003u);
    EXPECT_EQ(sink.db_count, 1u);
    EXPECT_EQ(sink.nack_count, 0u);
}

TEST(ApplyShopItemNameChangeSideEffects, ItemNotFoundEmitsNackCode6) {
    mxh::server::ShopItemNameChangeValidationInput in;
    in.player_found = true;
    in.item_found = false;
    auto plan = shop_item_name_change_side_effect_plan(in, 7, 8);
    EXPECT_FALSE(plan.trigger_db);
    EXPECT_TRUE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ShopItemNameChangeSideEffectKind::BroadcastNchangeNack);
    EXPECT_EQ(plan.effects[0].nack_code, LEGACY_NCHANGE_ERR_NOT_FOUND);

    RecordingSink sink;
    auto out = apply_shop_item_name_change_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.nacks_sent, 1u);
    EXPECT_TRUE(out.nack_flag_consumed);
    EXPECT_FALSE(out.db_flag_consumed);
    EXPECT_EQ(sink.last_call, "nack");
    EXPECT_EQ(sink.last_object_id, 7u);
    EXPECT_EQ(sink.last_item_db_idx, 8u);
    EXPECT_EQ(sink.last_nack_code, LEGACY_NCHANGE_ERR_NOT_FOUND);
    EXPECT_EQ(sink.nack_count, 1u);
    EXPECT_EQ(sink.db_count, 0u);
}

TEST(ApplyShopItemNameChangeSideEffects, NoPlayerIsNoOp) {
    mxh::server::ShopItemNameChangeValidationInput in;
    in.player_found = false;
    in.item_found = true;
    auto plan = shop_item_name_change_side_effect_plan(in, 7, 8);
    EXPECT_FALSE(plan.trigger_db);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_TRUE(plan.effects.empty());

    RecordingSink sink;
    auto out = apply_shop_item_name_change_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.db_fires, 0u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_FALSE(out.db_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_EQ(sink.last_call, "");
    EXPECT_EQ(sink.db_count, 0u);
    EXPECT_EQ(sink.nack_count, 0u);
}

TEST(ApplyShopItemNameChangeSideEffects, EmptyPlanIsNoOp) {
    mxh::server::ShopItemNameChangeSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_shop_item_name_change_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.db_fires, 0u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_FALSE(out.db_flag_consumed);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_EQ(sink.last_call, "");
    EXPECT_EQ(sink.db_count, 0u);
    EXPECT_EQ(sink.nack_count, 0u);
}

TEST(ApplyShopItemNameChangeSideEffects, NoPlayerOverridesNotFound) {
    mxh::server::ShopItemNameChangeValidationInput in;
    in.player_found = false;
    in.item_found = false;
    auto plan = shop_item_name_change_side_effect_plan(in, 7, 8);
    EXPECT_TRUE(plan.effects.empty());

    RecordingSink sink;
    (void)apply_shop_item_name_change_side_effects(plan, sink);
    EXPECT_EQ(sink.db_count, 0u);
    EXPECT_EQ(sink.nack_count, 0u);
}

TEST(ApplyShopItemNameChangeSideEffects, MaxIdsStillDispatches) {
    mxh::server::ShopItemNameChangeValidationInput in;
    in.player_found = true;
    in.item_found = true;
    auto plan = shop_item_name_change_side_effect_plan(
        in, 0xFFFFFFFFu, 0xFFFFFFFEu);
    EXPECT_TRUE(plan.trigger_db);

    RecordingSink sink;
    (void)apply_shop_item_name_change_side_effects(plan, sink);
    EXPECT_EQ(sink.last_call, "db");
    EXPECT_EQ(sink.last_object_id, 0xFFFFFFFFu);
    EXPECT_EQ(sink.last_item_db_idx, 0xFFFFFFFEu);
    EXPECT_EQ(sink.db_count, 1u);
}
