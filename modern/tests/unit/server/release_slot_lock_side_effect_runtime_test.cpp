// release_slot_lock_side_effect_runtime_test.cpp
//
// Verifies apply_release_slot_lock_side_effects() (the runtime
// orchestrator for the MP_ITEMEXT_*_RELEASE slot-unlock chain) walks
// the data-plane plan and dispatches the unlock entry / empty plan
// when the player or slot is missing.

#include <mxh/server/release_slot_lock_side_effect.hpp>
#include <mxh/server/release_slot_lock_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {

using mxh::server::ReleaseSlotLockSideEffectKind;
using mxh::server::ReleaseSlotLockSideEffectSink;
using mxh::server::ReleaseSlotLockValidationInput;
using mxh::server::apply_release_slot_lock_side_effects;
using mxh::server::release_slot_lock_side_effect_plan;

class RecordingSink final : public ReleaseSlotLockSideEffectSink {
public:
    std::vector<std::string> calls;
    std::uint32_t last_player_id = 0;
    std::uint16_t last_slot_pos = 0;
    std::size_t unlock_count = 0;

    void set_slot_unlock(std::uint32_t player_id,
                         std::uint16_t slot_pos) override {
        calls.push_back("unlock");
        last_player_id = player_id;
        last_slot_pos = slot_pos;
        ++unlock_count;
    }
};

}  // namespace

TEST(ApplyReleaseSlotLockSideEffects, UnlockedEmitsSetUnlock) {
    ReleaseSlotLockValidationInput in;
    in.player_found = true;
    in.slot_exists = true;
    auto plan = release_slot_lock_side_effect_plan(
        in, /*player_id=*/0x00160017u, /*slot_pos=*/22);
    EXPECT_TRUE(plan.set_unlock);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ReleaseSlotLockSideEffectKind::SetSlotUnlock);
    EXPECT_EQ(plan.effects[0].slot_pos, 22u);

    RecordingSink sink;
    auto out = apply_release_slot_lock_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.unlocks, 1u);
    EXPECT_TRUE(out.unlock_flag_consumed);
    EXPECT_EQ(sink.calls, std::vector<std::string>({"unlock"}));
    EXPECT_EQ(sink.last_player_id, 0x00160017u);
    EXPECT_EQ(sink.last_slot_pos, 22u);
}

TEST(ApplyReleaseSlotLockSideEffects, NoPlayerEmitsEmptyPlan) {
    ReleaseSlotLockValidationInput in;
    in.player_found = false;
    in.slot_exists = true;
    auto plan = release_slot_lock_side_effect_plan(in, 7, 3);
    EXPECT_FALSE(plan.set_unlock);
    EXPECT_TRUE(plan.effects.empty());

    RecordingSink sink;
    auto out = apply_release_slot_lock_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.unlocks, 0u);
    EXPECT_TRUE(sink.calls.empty());
}

TEST(ApplyReleaseSlotLockSideEffects, NoSlotEmitsEmptyPlan) {
    ReleaseSlotLockValidationInput in;
    in.player_found = true;
    in.slot_exists = false;
    auto plan = release_slot_lock_side_effect_plan(in, 7, 3);
    EXPECT_FALSE(plan.set_unlock);
    EXPECT_TRUE(plan.effects.empty());

    RecordingSink sink;
    auto out = apply_release_slot_lock_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.unlocks, 0u);
    EXPECT_TRUE(sink.calls.empty());
}

TEST(ApplyReleaseSlotLockSideEffects, EmptyPlanIsNoOp) {
    mxh::server::ReleaseSlotLockSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_release_slot_lock_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.unlocks, 0u);
    EXPECT_FALSE(out.unlock_flag_consumed);
    EXPECT_TRUE(sink.calls.empty());
}

TEST(ApplyReleaseSlotLockSideEffects, BoundaryPlayerAndSlotPos) {
    ReleaseSlotLockValidationInput in;
    in.player_found = true;
    in.slot_exists = true;
    auto plan = release_slot_lock_side_effect_plan(
        in, /*player_id=*/0xFFFFFFFFu, /*slot_pos=*/0xFFFFu);
    RecordingSink sink;
    (void)apply_release_slot_lock_side_effects(plan, sink);
    EXPECT_EQ(sink.last_player_id, 0xFFFFFFFFu);
    EXPECT_EQ(sink.last_slot_pos, 0xFFFFu);
}
