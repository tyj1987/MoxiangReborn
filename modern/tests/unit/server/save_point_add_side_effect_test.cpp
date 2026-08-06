// D4.41 SavePointAdd (MP_ITEM_SHOPITEM_SAVEPOINT_ADD_SYN) side-effect
// dispatcher tests.

#include <mxh/server/save_point_add_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(SavePointAddMaxNum, BaseCapacityIs10) {
    EXPECT_EQ(save_point_add_max_num(false), 10u);
}

TEST(SavePointAddMaxNum, ExtendedCapacityIs20) {
    EXPECT_EQ(save_point_add_max_num(true), 20u);
}

TEST(SavePointAddMaxNum, LegacyConstantsMatch) {
    EXPECT_EQ(MAX_MOVEDATA_PERPAGE_LEGACY, 10u);
    EXPECT_EQ(MAX_MOVEPOINT_PAGE_LEGACY, 2u);
    EXPECT_EQ(MAX_SAVED_MOVE_BASE, 10u);
    EXPECT_EQ(MAX_SAVED_MOVE_EXTENDED, 20u);
}

TEST(SavePointAddSuccess, PlanEmitsBroadcastThenDbInsert) {
    mxh::game::ShopItemBase shop_item{};
    shop_item.ItemBase.wIconIdx = 55365;
    shop_item.Remaintime = 60000;

    std::array<char, 21u> name{};
    const char* name_lit = "TestSave";
    for (std::size_t i = 0; i < 8; ++i) {
        name[i] = name_lit[i];
    }

    auto plan = save_point_add_success_side_effect_plan(
        shop_item, /*pos=*/42, /*idx=*/55365, name,
        /*map_num=*/17, /*point=*/12345u);
    EXPECT_TRUE(plan.send_use_ack);
    ASSERT_EQ(plan.effects.size(), 2u);
    EXPECT_EQ(plan.effects[0].kind,
              SavePointAddSideEffectKind::BroadcastUseAck);
    EXPECT_EQ(plan.effects[0].shop_item_base.ItemBase.wIconIdx, 55365u);
    EXPECT_EQ(plan.effects[0].shop_item_pos, 42u);
    EXPECT_EQ(plan.effects[0].shop_item_idx, 55365u);
    EXPECT_EQ(plan.effects[1].kind,
              SavePointAddSideEffectKind::InsertSavedMovePoint);
    EXPECT_EQ(plan.effects[1].map_num, 17u);
    EXPECT_EQ(plan.effects[1].point_value, 12345u);
}

TEST(SavePointAddSuccess, PlanIsIdempotent) {
    mxh::game::ShopItemBase shop_item{};
    std::array<char, 21u> name{};
    auto a = save_point_add_success_side_effect_plan(
        shop_item, 1, 2, name, 3, 4);
    auto b = save_point_add_success_side_effect_plan(
        shop_item, 1, 2, name, 3, 4);
    EXPECT_EQ(a.send_use_ack, b.send_use_ack);
    ASSERT_EQ(a.effects.size(), b.effects.size());
    EXPECT_EQ(a.effects[0].kind, b.effects[0].kind);
    EXPECT_EQ(a.effects[1].kind, b.effects[1].kind);
}

TEST(SavePointAddNack, PlanEmitsSingleDwordWithECode) {
    auto plan = save_point_add_nack_side_effect_plan(/*e_code=*/5u);
    EXPECT_TRUE(plan.send_use_nack);
    ASSERT_EQ(plan.steps.size(), 1u);
    EXPECT_EQ(plan.steps[0].kind,
              SavePointAddNackKind::BroadcastUseNack);
    EXPECT_EQ(plan.steps[0].e_code, 5u);
}

TEST(SavePointAddNack, ZeroECodeMirrorsValidsavenumGate) {
    // Legacy goto SAVEPOINT_ADD_FAILED uses rt (initialized to 0 from
    // UseShopItem's BYTE return). Confirm zero is a valid payload.
    auto plan = save_point_add_nack_side_effect_plan(
        LEGACY_ITEM_USE_SUCCESS);
    EXPECT_TRUE(plan.send_use_nack);
    EXPECT_EQ(plan.steps[0].e_code, 0u);
}

TEST(SavePointAddNack, PlanIsIdempotent) {
    auto a = save_point_add_nack_side_effect_plan(7u);
    auto b = save_point_add_nack_side_effect_plan(7u);
    EXPECT_EQ(a.send_use_nack, b.send_use_nack);
    ASSERT_EQ(a.steps.size(), b.steps.size());
    EXPECT_EQ(a.steps[0].e_code, b.steps[0].e_code);
}


