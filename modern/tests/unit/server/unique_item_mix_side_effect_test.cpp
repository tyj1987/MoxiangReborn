// Tests for MP_ITEMEXT_UNIQUEITEM_MIX_SYN side-effect dispatcher.

#include <mxh/server/unique_item_mix_side_effect.hpp>

#include <gtest/gtest.h>

#include <vector>

using namespace mxh::server;

UniqueItemMixValidationInput success_input() {
    UniqueItemMixValidationInput in{};
    in.basic_item_exists = true;
    in.all_materials_exist = true;
    in.mix_info_exists = true;
    in.enough_material_for_each_kind = true;
    in.inventory_has_space = true;
    return in;
}

std::vector<UniqueItemMixMaterial> two_materials() {
    return {
        {50, 100, 1000, 1},
        {51, 101, 1001, 2},
    };
}

TEST(UniqueItemMixOutcome, AllGatesPassIsMixed) {
    EXPECT_EQ(classify_unique_item_mix_outcome(success_input()),
              UniqueItemMixOutcome::Mixed);
}

TEST(UniqueItemMixOutcome, BasicMissingIsBasicItemMissing) {
    auto in = success_input();
    in.basic_item_exists = false;
    EXPECT_EQ(classify_unique_item_mix_outcome(in),
              UniqueItemMixOutcome::BasicItemMissing);
}

TEST(UniqueItemMixOutcome, MaterialMissingIsMaterialMissing) {
    auto in = success_input();
    in.all_materials_exist = false;
    EXPECT_EQ(classify_unique_item_mix_outcome(in),
              UniqueItemMixOutcome::MaterialMissing);
}

TEST(UniqueItemMixOutcome, MixInfoMissingIsMixInfoMissing) {
    auto in = success_input();
    in.mix_info_exists = false;
    EXPECT_EQ(classify_unique_item_mix_outcome(in),
              UniqueItemMixOutcome::MixInfoMissing);
}

TEST(UniqueItemMixOutcome, NotEnoughMaterialIsNotEnoughMaterial) {
    auto in = success_input();
    in.enough_material_for_each_kind = false;
    EXPECT_EQ(classify_unique_item_mix_outcome(in),
              UniqueItemMixOutcome::NotEnoughMaterial);
}

TEST(UniqueItemMixOutcome, NoSpaceIsNoSpaceForResult) {
    auto in = success_input();
    in.inventory_has_space = false;
    EXPECT_EQ(classify_unique_item_mix_outcome(in),
              UniqueItemMixOutcome::NoSpaceForResult);
}

TEST(UniqueItemMixOutcome, BasicItemTakesPrecedence) {
    auto in = success_input();
    in.basic_item_exists = false;
    in.all_materials_exist = false;
    in.inventory_has_space = false;
    EXPECT_EQ(classify_unique_item_mix_outcome(in),
              UniqueItemMixOutcome::BasicItemMissing);
}

TEST(UniqueItemMixRandomSeed, ClampsRange) {
    EXPECT_EQ(unique_item_mix_random_seed(0), LEGACY_UNIQUE_MIX_RANDOM_MIN);
    EXPECT_EQ(unique_item_mix_random_seed(1), 1u);
    EXPECT_EQ(unique_item_mix_random_seed(50), 50u);
    EXPECT_EQ(unique_item_mix_random_seed(100),
              LEGACY_UNIQUE_MIX_RANDOM_MAX);
    EXPECT_EQ(unique_item_mix_random_seed(101),
              LEGACY_UNIQUE_MIX_RANDOM_MAX);
    EXPECT_EQ(unique_item_mix_random_seed(999),
              LEGACY_UNIQUE_MIX_RANDOM_MAX);
}

TEST(UniqueItemMixPlan, MixedEmitsFullSequence) {
    auto in = success_input();
    auto mats = two_materials();
    auto plan = unique_item_mix_side_effect_plan(
        in, 100, mats, 49, 200, 999, 300, 1);
    EXPECT_TRUE(plan.discard_materials);
    EXPECT_TRUE(plan.discard_basic);
    EXPECT_TRUE(plan.roll_result);
    EXPECT_TRUE(plan.obtain_result);
    // 3 effects per material (2 mats) + 4 final = 10
    EXPECT_EQ(plan.effects.size(), 11u);
    EXPECT_EQ(plan.effects[0].kind,
              UniqueItemMixSideEffectKind::DiscardMaterialItem);
    EXPECT_EQ(plan.effects[0].material.pos, 50u);
    EXPECT_EQ(plan.effects[1].kind,
              UniqueItemMixSideEffectKind::SendMaterialDeleteAck);
    EXPECT_EQ(plan.effects[2].kind,
              UniqueItemMixSideEffectKind::LogMaterialDiscard);
    EXPECT_EQ(plan.effects[3].kind,
              UniqueItemMixSideEffectKind::DiscardMaterialItem);
    EXPECT_EQ(plan.effects[3].material.pos, 51u);
    EXPECT_EQ(plan.effects[4].kind,
              UniqueItemMixSideEffectKind::SendMaterialDeleteAck);
    EXPECT_EQ(plan.effects[5].kind,
              UniqueItemMixSideEffectKind::LogMaterialDiscard);
    EXPECT_EQ(plan.effects[6].kind,
              UniqueItemMixSideEffectKind::DiscardBasicItem);
    EXPECT_EQ(plan.effects[6].basic_pos, 49u);
    EXPECT_EQ(plan.effects[6].basic_w_idx, 200u);
    EXPECT_EQ(plan.effects[6].basic_db_idx, 999u);
    EXPECT_EQ(plan.effects[7].kind,
              UniqueItemMixSideEffectKind::SendBasicDeleteAck);
    EXPECT_EQ(plan.effects[8].kind,
              UniqueItemMixSideEffectKind::LogBasicDiscard);
    EXPECT_EQ(plan.effects[9].kind,
              UniqueItemMixSideEffectKind::RollRandomResultItem);
    EXPECT_EQ(plan.effects[10].kind,
              UniqueItemMixSideEffectKind::ObtainResultItem);
    EXPECT_EQ(plan.effects[10].result_w_idx, 300u);
}

TEST(UniqueItemMixPlan, NoSpaceOmitsObtainResult) {
    auto in = success_input();
    in.inventory_has_space = false;
    auto mats = two_materials();
    auto plan = unique_item_mix_side_effect_plan(
        in, 100, mats, 49, 200, 999, 300, 0);
    EXPECT_TRUE(plan.discard_materials);
    EXPECT_TRUE(plan.discard_basic);
    EXPECT_TRUE(plan.roll_result);
    EXPECT_FALSE(plan.obtain_result);
    EXPECT_EQ(plan.effects.size(), 10u);
    EXPECT_EQ(plan.effects[9].kind,
              UniqueItemMixSideEffectKind::RollRandomResultItem);
}

TEST(UniqueItemMixPlan, BasicItemMissingEmitsEmptyPlan) {
    auto in = success_input();
    in.basic_item_exists = false;
    auto mats = two_materials();
    auto plan = unique_item_mix_side_effect_plan(
        in, 100, mats, 49, 200, 999, 300, 1);
    EXPECT_FALSE(plan.discard_materials);
    EXPECT_FALSE(plan.discard_basic);
    EXPECT_FALSE(plan.roll_result);
    EXPECT_FALSE(plan.obtain_result);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(UniqueItemMixPlan, MaterialMissingEmitsEmptyPlan) {
    auto in = success_input();
    in.all_materials_exist = false;
    auto mats = two_materials();
    auto plan = unique_item_mix_side_effect_plan(
        in, 100, mats, 49, 200, 999, 300, 1);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(UniqueItemMixPlan, MixInfoMissingEmitsEmptyPlan) {
    auto in = success_input();
    in.mix_info_exists = false;
    auto plan = unique_item_mix_side_effect_plan(
        in, 100, two_materials(), 49, 200, 999, 300, 1);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(UniqueItemMixPlan, NotEnoughMaterialEmitsEmptyPlan) {
    auto in = success_input();
    in.enough_material_for_each_kind = false;
    auto plan = unique_item_mix_side_effect_plan(
        in, 100, two_materials(), 49, 200, 999, 300, 1);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(UniqueItemMixPlan, EmptyMaterialsListStillEmitsBasicAndRoll) {
    auto in = success_input();
    std::vector<UniqueItemMixMaterial> empty;
    auto plan = unique_item_mix_side_effect_plan(
        in, 100, empty, 49, 200, 999, 300, 1);
    EXPECT_TRUE(plan.discard_basic);
    EXPECT_TRUE(plan.obtain_result);
    EXPECT_EQ(plan.effects.size(), 5u);
    EXPECT_EQ(plan.effects[0].kind,
              UniqueItemMixSideEffectKind::DiscardBasicItem);
    EXPECT_EQ(plan.effects[1].kind,
              UniqueItemMixSideEffectKind::SendBasicDeleteAck);
    EXPECT_EQ(plan.effects[2].kind,
              UniqueItemMixSideEffectKind::LogBasicDiscard);
    EXPECT_EQ(plan.effects[3].kind,
              UniqueItemMixSideEffectKind::RollRandomResultItem);
    EXPECT_EQ(plan.effects[4].kind,
              UniqueItemMixSideEffectKind::ObtainResultItem);
}

TEST(UniqueItemMixPlan, PlanIsIdempotent) {
    auto in = success_input();
    auto mats = two_materials();
    auto a = unique_item_mix_side_effect_plan(
        in, 100, mats, 49, 200, 999, 300, 1);
    auto b = unique_item_mix_side_effect_plan(
        in, 100, mats, 49, 200, 999, 300, 1);
    EXPECT_EQ(a.discard_materials, b.discard_materials);
    EXPECT_EQ(a.obtain_result, b.obtain_result);
    EXPECT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
    }
}



