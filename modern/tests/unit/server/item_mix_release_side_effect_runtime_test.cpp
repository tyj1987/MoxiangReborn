// item_mix_release_side_effect_runtime_test.cpp
//
// Verifies apply_item_mix_release_side_effects() (the runtime
// orchestrator for the CItemManager::MP_ITEM_MIX_RELEASEITEM
// side-effect chain) walks the data-plane plan and dispatches the
// ClearSlotLock entry when the player + slot resolve, and stays a
// no-op otherwise (pure side-effect, no network response).

#include <mxh/server/item_mix_release_side_effect.hpp>
#include <mxh/server/item_mix_release_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>

namespace {

using mxh::server::ItemMixReleaseSideEffectKind;
using mxh::server::ItemMixReleaseSideEffectSink;
using mxh::server::apply_item_mix_release_side_effects;
using mxh::server::item_mix_release_side_effect_plan;

class RecordingSink final : public ItemMixReleaseSideEffectSink {
public:
    std::string last_call;
    std::uint16_t last_slot_pos = 0;
    std::size_t clear_count = 0;

    void clear_slot_lock(std::uint16_t slot_pos) override {
        last_call = "clear";
        last_slot_pos = slot_pos;
        ++clear_count;
    }
};

}  // namespace

TEST(ApplyItemMixReleaseSideEffects, ResolvedEmitsClearSlotLock) {
    mxh::server::ItemMixReleaseValidationInput in;
    in.player_found = true;
    in.slot_resolved = true;
    auto plan = item_mix_release_side_effect_plan(in, /*slot_pos=*/7);
    EXPECT_TRUE(plan.release_lock);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemMixReleaseSideEffectKind::ClearSlotLock);
    EXPECT_EQ(plan.effects[0].slot_pos, 7u);

    RecordingSink sink;
    auto out = apply_item_mix_release_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.locks_cleared, 1u);
    EXPECT_TRUE(out.release_flag_consumed);
    EXPECT_EQ(sink.last_call, "clear");
    EXPECT_EQ(sink.last_slot_pos, 7u);
    EXPECT_EQ(sink.clear_count, 1u);
}

TEST(ApplyItemMixReleaseSideEffects, NoPlayerIsNoOp) {
    // Legacy: FindUser null -> handler returns immediately.
    mxh::server::ItemMixReleaseValidationInput in;
    in.player_found = false;
    in.slot_resolved = true;
    auto plan = item_mix_release_side_effect_plan(in, 7);
    EXPECT_FALSE(plan.release_lock);
    EXPECT_TRUE(plan.effects.empty());

    RecordingSink sink;
    auto out = apply_item_mix_release_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.locks_cleared, 0u);
    EXPECT_FALSE(out.release_flag_consumed);
    EXPECT_EQ(sink.last_call, "");
    EXPECT_EQ(sink.clear_count, 0u);
}

TEST(ApplyItemMixReleaseSideEffects, NoSlotIsNoOp) {
    // Legacy: GetSlot null -> handler returns immediately.
    mxh::server::ItemMixReleaseValidationInput in;
    in.player_found = true;
    in.slot_resolved = false;
    auto plan = item_mix_release_side_effect_plan(in, 7);
    EXPECT_FALSE(plan.release_lock);
    EXPECT_TRUE(plan.effects.empty());

    RecordingSink sink;
    auto out = apply_item_mix_release_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.locks_cleared, 0u);
    EXPECT_FALSE(out.release_flag_consumed);
    EXPECT_EQ(sink.last_call, "");
    EXPECT_EQ(sink.clear_count, 0u);
}

TEST(ApplyItemMixReleaseSideEffects, EmptyPlanIsNoOp) {
    mxh::server::ItemMixReleaseSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_item_mix_release_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.locks_cleared, 0u);
    EXPECT_FALSE(out.release_flag_consumed);
    EXPECT_EQ(sink.last_call, "");
    EXPECT_EQ(sink.clear_count, 0u);
}

TEST(ApplyItemMixReleaseSideEffects, NoPlayerOverridesResolvedSlot) {
    // classify_item_mix_release_outcome: NoPlayer wins over NoSlot.
    mxh::server::ItemMixReleaseValidationInput in;
    in.player_found = false;
    in.slot_resolved = false;
    auto plan = item_mix_release_side_effect_plan(in, 7);
    EXPECT_TRUE(plan.effects.empty());

    RecordingSink sink;
    (void)apply_item_mix_release_side_effects(plan, sink);
    EXPECT_EQ(sink.clear_count, 0u);
}

TEST(ApplyItemMixReleaseSideEffects, ZeroSlotPosStillDispatches) {
    mxh::server::ItemMixReleaseValidationInput in;
    in.player_found = true;
    in.slot_resolved = true;
    auto plan = item_mix_release_side_effect_plan(in, 0);
    EXPECT_TRUE(plan.release_lock);

    RecordingSink sink;
    (void)apply_item_mix_release_side_effects(plan, sink);
    EXPECT_EQ(sink.last_call, "clear");
    EXPECT_EQ(sink.last_slot_pos, 0u);
    EXPECT_EQ(sink.clear_count, 1u);
}
