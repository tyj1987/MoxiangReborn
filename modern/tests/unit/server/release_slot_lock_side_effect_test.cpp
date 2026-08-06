// Tests for the generic Release slot-lock side-effect dispatcher.

#include <mxh/server/release_slot_lock_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

ReleaseSlotLockValidationInput success_input() {
    ReleaseSlotLockValidationInput in{};
    in.player_found = true;
    in.slot_exists = true;
    return in;
}

TEST(ReleaseSlotLockOutcome, BothFoundIsUnlocked) {
    EXPECT_EQ(classify_release_slot_lock_outcome(success_input()),
              ReleaseSlotLockOutcome::Unlocked);
}

TEST(ReleaseSlotLockOutcome, PlayerMissingIsNoPlayer) {
    auto in = success_input();
    in.player_found = false;
    EXPECT_EQ(classify_release_slot_lock_outcome(in),
              ReleaseSlotLockOutcome::NoPlayer);
}

TEST(ReleaseSlotLockOutcome, SlotMissingIsNoSlot) {
    auto in = success_input();
    in.slot_exists = false;
    EXPECT_EQ(classify_release_slot_lock_outcome(in),
              ReleaseSlotLockOutcome::NoSlot);
}

TEST(ReleaseSlotLockPlan, UnlockedEmitsSetUnlock) {
    auto in = success_input();
    auto plan = release_slot_lock_side_effect_plan(in, 100, 25);
    EXPECT_TRUE(plan.set_unlock);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ReleaseSlotLockSideEffectKind::SetSlotUnlock);
    EXPECT_EQ(plan.effects[0].player_id, 100u);
    EXPECT_EQ(plan.effects[0].slot_pos, 25u);
}

TEST(ReleaseSlotLockPlan, NoPlayerEmitsEmptyPlan) {
    auto in = success_input();
    in.player_found = false;
    auto plan = release_slot_lock_side_effect_plan(in, 100, 25);
    EXPECT_FALSE(plan.set_unlock);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(ReleaseSlotLockPlan, NoSlotEmitsEmptyPlan) {
    auto in = success_input();
    in.slot_exists = false;
    auto plan = release_slot_lock_side_effect_plan(in, 100, 25);
    EXPECT_FALSE(plan.set_unlock);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(ReleaseSlotLockPlan, PlanIsIdempotent) {
    auto in = success_input();
    auto a = release_slot_lock_side_effect_plan(in, 100, 25);
    auto b = release_slot_lock_side_effect_plan(in, 100, 25);
    EXPECT_EQ(a.set_unlock, b.set_unlock);
    EXPECT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].player_id, b.effects[i].player_id);
    }
}
