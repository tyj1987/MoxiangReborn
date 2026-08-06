// D4.63 PetInvenInfo (MP_ITEM_PETINVEN_INFO_SYN) side-effect
// dispatcher tests.

#include <mxh/server/pet_inven_info_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

PetInvenInfoValidationInput ok() {
    PetInvenInfoValidationInput in{};
    in.player_found = true;
    in.pet_summoned = true;
    return in;
}

TEST(PetInvenInfoOutcome, AllGatesPassIsTriggered) {
    auto in = ok();
    EXPECT_EQ(classify_pet_inven_info_outcome(in),
              PetInvenInfoOutcome::Triggered);
}

TEST(PetInvenInfoOutcome, NoPlayerIsNoPlayer) {
    auto in = ok();
    in.player_found = false;
    EXPECT_EQ(classify_pet_inven_info_outcome(in),
              PetInvenInfoOutcome::NoPlayer);
}

TEST(PetInvenInfoOutcome, NoPetIsNoPetActive) {
    auto in = ok();
    in.pet_summoned = false;
    EXPECT_EQ(classify_pet_inven_info_outcome(in),
              PetInvenInfoOutcome::NoPetActive);
}

TEST(PetInvenInfoPlan, TriggeredEmitsDbQuery) {
    auto in = ok();
    auto plan = pet_inven_info_side_effect_plan(
        in, /*object_id=*/100, /*user_id=*/200);
    EXPECT_TRUE(plan.trigger_db);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              PetInvenInfoSideEffectKind::FirePetInvenDbQuery);
    EXPECT_EQ(plan.effects[0].object_id, 100u);
    EXPECT_EQ(plan.effects[0].user_id, 200u);
    EXPECT_EQ(plan.effects[0].start_pos, LEGACY_TP_PETINVEN_START);
    EXPECT_EQ(plan.effects[0].end_pos, LEGACY_TP_PETINVEN_END);
}

TEST(PetInvenInfoPlan, NoPlayerEmitsEmptyPlan) {
    auto in = ok();
    in.player_found = false;
    auto plan = pet_inven_info_side_effect_plan(in, 1, 1);
    EXPECT_FALSE(plan.trigger_db);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(PetInvenInfoPlan, NoPetEmitsEmptyPlan) {
    auto in = ok();
    in.pet_summoned = false;
    auto plan = pet_inven_info_side_effect_plan(in, 1, 1);
    EXPECT_FALSE(plan.trigger_db);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(PetInvenInfoPlan, PlanIsIdempotent) {
    auto in = ok();
    auto a = pet_inven_info_side_effect_plan(in, 1, 2);
    auto b = pet_inven_info_side_effect_plan(in, 1, 2);
    EXPECT_EQ(a.trigger_db, b.trigger_db);
    ASSERT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].object_id, b.effects[i].object_id);
        EXPECT_EQ(a.effects[i].user_id, b.effects[i].user_id);
        EXPECT_EQ(a.effects[i].start_pos, b.effects[i].start_pos);
        EXPECT_EQ(a.effects[i].end_pos, b.effects[i].end_pos);
    }
}
