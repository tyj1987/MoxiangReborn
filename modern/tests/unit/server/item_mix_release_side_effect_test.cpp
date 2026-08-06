// D4.57 ItemMixRelease (MP_ITEM_MIX_RELEASEITEM) side-effect
// dispatcher tests.

#include <mxh/server/item_mix_release_side_effect.hpp>

#include <gtest/gtest.h>

using namespace mxh::server;

ItemMixReleaseValidationInput ok() {
    ItemMixReleaseValidationInput in{};
    in.player_found = true;
    in.slot_resolved = true;
    return in;
}

TEST(ItemMixReleaseOutcome, PlayerFoundAndSlotResolvedIsReleased) {
    auto in = ok();
    EXPECT_EQ(classify_item_mix_release_outcome(in),
              ItemMixReleaseOutcome::Released);
}

TEST(ItemMixReleaseOutcome, PlayerNotFoundIsNoPlayer) {
    auto in = ok();
    in.player_found = false;
    EXPECT_EQ(classify_item_mix_release_outcome(in),
              ItemMixReleaseOutcome::NoPlayer);
}

TEST(ItemMixReleaseOutcome, SlotNotResolvedIsNoSlot) {
    auto in = ok();
    in.slot_resolved = false;
    EXPECT_EQ(classify_item_mix_release_outcome(in),
              ItemMixReleaseOutcome::NoSlot);
}

TEST(ItemMixReleasePlan, ReleasedEmitsClearSlotLock) {
    auto in = ok();
    auto plan = item_mix_release_side_effect_plan(in, /*slot_pos=*/42);
    EXPECT_TRUE(plan.release_lock);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              ItemMixReleaseSideEffectKind::ClearSlotLock);
    EXPECT_EQ(plan.effects[0].slot_pos, 42u);
}

TEST(ItemMixReleasePlan, NoPlayerEmitsEmptyPlan) {
    auto in = ok();
    in.player_found = false;
    auto plan = item_mix_release_side_effect_plan(in, 1);
    EXPECT_FALSE(plan.release_lock);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(ItemMixReleasePlan, NoSlotEmitsEmptyPlan) {
    auto in = ok();
    in.slot_resolved = false;
    auto plan = item_mix_release_side_effect_plan(in, 1);
    EXPECT_FALSE(plan.release_lock);
    EXPECT_EQ(plan.effects.size(), 0u);
}

TEST(ItemMixReleasePlan, PlanIsIdempotent) {
    auto in = ok();
    auto a = item_mix_release_side_effect_plan(in, 7);
    auto b = item_mix_release_side_effect_plan(in, 7);
    EXPECT_EQ(a.release_lock, b.release_lock);
    ASSERT_EQ(a.effects.size(), b.effects.size());
    for (std::size_t i = 0; i < a.effects.size(); ++i) {
        EXPECT_EQ(a.effects[i].kind, b.effects[i].kind);
        EXPECT_EQ(a.effects[i].slot_pos, b.effects[i].slot_pos);
    }
}
