// pet_inven_info_side_effect_runtime_test.cpp
//
// Verifies apply_pet_inven_info_side_effects() (the runtime
// orchestrator for the CItemManager::MP_ITEM_PETINVEN_INFO_SYN
// side-effect chain) walks the data-plane plan and dispatches the
// PetInvenItemOptionInfo DB query when the player has a pet summoned,
// and stays a no-op otherwise (no ACK/NACK).

#include <mxh/server/pet_inven_info_side_effect.hpp>
#include <mxh/server/pet_inven_info_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>

namespace {

using mxh::server::PetInvenInfoSideEffectKind;
using mxh::server::PetInvenInfoSideEffectSink;
using mxh::server::apply_pet_inven_info_side_effects;
using mxh::server::pet_inven_info_side_effect_plan;

class RecordingSink final : public PetInvenInfoSideEffectSink {
public:
    std::string last_call;
    std::uint32_t last_object_id = 0;
    std::uint32_t last_user_id = 0;
    std::uint16_t last_start_pos = 0;
    std::uint16_t last_end_pos = 0;
    std::size_t db_count = 0;

    void fire_pet_inven_db_query(std::uint32_t object_id,
                                 std::uint32_t user_id,
                                 std::uint16_t start_pos,
                                 std::uint16_t end_pos) override {
        last_call = "db";
        last_object_id = object_id;
        last_user_id = user_id;
        last_start_pos = start_pos;
        last_end_pos = end_pos;
        ++db_count;
    }
};

}  // namespace

TEST(ApplyPetInvenInfoSideEffects, PetSummonedEmitsDbQuery) {
    mxh::server::PetInvenInfoValidationInput in;
    in.player_found = true;
    in.pet_summoned = true;
    auto plan = pet_inven_info_side_effect_plan(
        in, /*object_id=*/0x00010002u, /*user_id=*/0x00000003u);
    EXPECT_TRUE(plan.trigger_db);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              PetInvenInfoSideEffectKind::FirePetInvenDbQuery);
    EXPECT_EQ(plan.effects[0].object_id, 0x00010002u);
    EXPECT_EQ(plan.effects[0].start_pos,
              mxh::server::LEGACY_TP_PETINVEN_START);
    EXPECT_EQ(plan.effects[0].end_pos,
              mxh::server::LEGACY_TP_PETINVEN_END);

    RecordingSink sink;
    auto out = apply_pet_inven_info_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.db_queries, 1u);
    EXPECT_TRUE(out.trigger_db_flag_consumed);
    EXPECT_EQ(sink.last_call, "db");
    EXPECT_EQ(sink.last_object_id, 0x00010002u);
    EXPECT_EQ(sink.last_user_id, 0x00000003u);
    EXPECT_EQ(sink.last_start_pos,
              mxh::server::LEGACY_TP_PETINVEN_START);
    EXPECT_EQ(sink.last_end_pos,
              mxh::server::LEGACY_TP_PETINVEN_END);
    EXPECT_EQ(sink.db_count, 1u);
}

TEST(ApplyPetInvenInfoSideEffects, NoPlayerIsNoOp) {
    mxh::server::PetInvenInfoValidationInput in;
    in.player_found = false;
    in.pet_summoned = true;
    auto plan = pet_inven_info_side_effect_plan(in, 7, 8);
    EXPECT_FALSE(plan.trigger_db);
    EXPECT_TRUE(plan.effects.empty());

    RecordingSink sink;
    auto out = apply_pet_inven_info_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.db_queries, 0u);
    EXPECT_FALSE(out.trigger_db_flag_consumed);
    EXPECT_EQ(sink.last_call, "");
    EXPECT_EQ(sink.db_count, 0u);
}

TEST(ApplyPetInvenInfoSideEffects, NoPetActiveIsNoOp) {
    // Legacy: GetCurSummonPet() null -> silent drop.
    mxh::server::PetInvenInfoValidationInput in;
    in.player_found = true;
    in.pet_summoned = false;
    auto plan = pet_inven_info_side_effect_plan(in, 7, 8);
    EXPECT_FALSE(plan.trigger_db);
    EXPECT_TRUE(plan.effects.empty());

    RecordingSink sink;
    auto out = apply_pet_inven_info_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.db_queries, 0u);
    EXPECT_FALSE(out.trigger_db_flag_consumed);
    EXPECT_EQ(sink.last_call, "");
    EXPECT_EQ(sink.db_count, 0u);
}

TEST(ApplyPetInvenInfoSideEffects, EmptyPlanIsNoOp) {
    mxh::server::PetInvenInfoSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_pet_inven_info_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.db_queries, 0u);
    EXPECT_FALSE(out.trigger_db_flag_consumed);
    EXPECT_EQ(sink.last_call, "");
    EXPECT_EQ(sink.db_count, 0u);
}

TEST(ApplyPetInvenInfoSideEffects, NoPlayerOverridesNoPet) {
    // classify: NoPlayer wins over NoPetActive.
    mxh::server::PetInvenInfoValidationInput in;
    in.player_found = false;
    in.pet_summoned = false;
    auto plan = pet_inven_info_side_effect_plan(in, 7, 8);
    EXPECT_TRUE(plan.effects.empty());

    RecordingSink sink;
    (void)apply_pet_inven_info_side_effects(plan, sink);
    EXPECT_EQ(sink.db_count, 0u);
}

TEST(ApplyPetInvenInfoSideEffects, MaxIdsStillDispatches) {
    mxh::server::PetInvenInfoValidationInput in;
    in.player_found = true;
    in.pet_summoned = true;
    auto plan = pet_inven_info_side_effect_plan(
        in, 0xFFFFFFFFu, 0xFFFFFFFEu);
    EXPECT_TRUE(plan.trigger_db);

    RecordingSink sink;
    (void)apply_pet_inven_info_side_effects(plan, sink);
    EXPECT_EQ(sink.last_call, "db");
    EXPECT_EQ(sink.last_object_id, 0xFFFFFFFFu);
    EXPECT_EQ(sink.last_user_id, 0xFFFFFFFEu);
    EXPECT_EQ(sink.db_count, 1u);
}
