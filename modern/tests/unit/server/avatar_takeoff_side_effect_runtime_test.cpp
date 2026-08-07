// avatar_takeoff_side_effect_runtime_test.cpp
//
// Verifies apply_avatar_takeoff_side_effects() (the runtime
// orchestrator for the CItemManager::MP_ITEM_SHOPITEM_AVATAR_TAKEOFF
// side-effect chain) walks the data-plane plan and dispatches each
// entry: NACK on not-usable / take-off-failed / silent success on
// successful take-off / no-op when the player is missing.

#include <mxh/server/avatar_takeoff_side_effect.hpp>
#include <mxh/server/avatar_takeoff_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>

namespace {

using mxh::server::AvatarTakeoffSideEffectKind;
using mxh::server::AvatarTakeoffSideEffectSink;
using mxh::server::apply_avatar_takeoff_side_effects;
using mxh::server::avatar_takeoff_side_effect_plan;

class RecordingSink final : public AvatarTakeoffSideEffectSink {
public:
    std::string last_call;
    std::uint16_t last_item_idx = 0;
    std::uint16_t last_item_pos = 0;
    std::size_t nack_count = 0;
    std::size_t silent_count = 0;

    void broadcast_avatar_use_nack(std::uint16_t item_idx,
                                   std::uint16_t item_pos) override {
        last_call = "nack";
        last_item_idx = item_idx;
        last_item_pos = item_pos;
        ++nack_count;
    }
    void silent_success() override {
        last_call = "silent";
        ++silent_count;
    }
};

}  // namespace

TEST(ApplyAvatarTakeoffSideEffects, SuccessIsSilent) {
    mxh::server::AvatarTakeoffValidationInput in;
    in.player_found = true;
    in.usable_shop_item = true;
    in.take_off_ok = true;
    auto plan = avatar_takeoff_side_effect_plan(
        in, /*item_idx=*/100, /*item_pos=*/7);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_TRUE(plan.silent_success);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              AvatarTakeoffSideEffectKind::SilentSuccess);

    RecordingSink sink;
    auto out = apply_avatar_takeoff_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_EQ(out.silent_successes, 1u);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_TRUE(out.silent_flag_consumed);
    EXPECT_EQ(sink.last_call, "silent");
    EXPECT_EQ(sink.silent_count, 1u);
    EXPECT_EQ(sink.nack_count, 0u);
}

TEST(ApplyAvatarTakeoffSideEffects, NotUsableEmitsNack) {
    mxh::server::AvatarTakeoffValidationInput in;
    in.player_found = true;
    in.usable_shop_item = false;
    in.take_off_ok = true;
    auto plan = avatar_takeoff_side_effect_plan(in, 100, 7);
    EXPECT_TRUE(plan.send_nack);
    EXPECT_FALSE(plan.silent_success);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              AvatarTakeoffSideEffectKind::BroadcastAvatarUseNack);

    RecordingSink sink;
    auto out = apply_avatar_takeoff_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 1u);
    EXPECT_EQ(out.nacks_sent, 1u);
    EXPECT_EQ(out.silent_successes, 0u);
    EXPECT_TRUE(out.nack_flag_consumed);
    EXPECT_FALSE(out.silent_flag_consumed);
    EXPECT_EQ(sink.last_call, "nack");
    EXPECT_EQ(sink.last_item_idx, 100u);
    EXPECT_EQ(sink.last_item_pos, 7u);
    EXPECT_EQ(sink.nack_count, 1u);
    EXPECT_EQ(sink.silent_count, 0u);
}

TEST(ApplyAvatarTakeoffSideEffects, TakeOffFailedEmitsNack) {
    mxh::server::AvatarTakeoffValidationInput in;
    in.player_found = true;
    in.usable_shop_item = true;
    in.take_off_ok = false;
    auto plan = avatar_takeoff_side_effect_plan(in, 100, 7);
    EXPECT_TRUE(plan.send_nack);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              AvatarTakeoffSideEffectKind::BroadcastAvatarUseNack);

    RecordingSink sink;
    (void)apply_avatar_takeoff_side_effects(plan, sink);
    EXPECT_EQ(sink.last_call, "nack");
    EXPECT_EQ(sink.nack_count, 1u);
    EXPECT_EQ(sink.silent_count, 0u);
}

TEST(ApplyAvatarTakeoffSideEffects, NoPlayerIsNoOp) {
    mxh::server::AvatarTakeoffValidationInput in;
    in.player_found = false;
    in.usable_shop_item = true;
    in.take_off_ok = true;
    auto plan = avatar_takeoff_side_effect_plan(in, 100, 7);
    EXPECT_FALSE(plan.send_nack);
    EXPECT_FALSE(plan.silent_success);
    EXPECT_TRUE(plan.effects.empty());

    RecordingSink sink;
    auto out = apply_avatar_takeoff_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_EQ(out.silent_successes, 0u);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_FALSE(out.silent_flag_consumed);
    EXPECT_EQ(sink.last_call, "");
    EXPECT_EQ(sink.nack_count, 0u);
    EXPECT_EQ(sink.silent_count, 0u);
}

TEST(ApplyAvatarTakeoffSideEffects, EmptyPlanIsNoOp) {
    mxh::server::AvatarTakeoffSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_avatar_takeoff_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.nacks_sent, 0u);
    EXPECT_EQ(out.silent_successes, 0u);
    EXPECT_FALSE(out.nack_flag_consumed);
    EXPECT_FALSE(out.silent_flag_consumed);
    EXPECT_EQ(sink.last_call, "");
    EXPECT_EQ(sink.nack_count, 0u);
    EXPECT_EQ(sink.silent_count, 0u);
}

TEST(ApplyAvatarTakeoffSideEffects, NoPlayerOverridesOtherGates) {
    mxh::server::AvatarTakeoffValidationInput in;
    in.player_found = false;
    in.usable_shop_item = false;
    in.take_off_ok = false;
    auto plan = avatar_takeoff_side_effect_plan(in, 100, 7);
    EXPECT_TRUE(plan.effects.empty());

    RecordingSink sink;
    (void)apply_avatar_takeoff_side_effects(plan, sink);
    EXPECT_EQ(sink.nack_count, 0u);
    EXPECT_EQ(sink.silent_count, 0u);
}

TEST(ApplyAvatarTakeoffSideEffects, FieldPassthroughOnNack) {
    mxh::server::AvatarTakeoffValidationInput in;
    in.player_found = true;
    in.usable_shop_item = false;
    in.take_off_ok = false;
    auto plan = avatar_takeoff_side_effect_plan(in, 21, 22);
    RecordingSink sink;
    (void)apply_avatar_takeoff_side_effects(plan, sink);
    EXPECT_EQ(sink.last_call, "nack");
    EXPECT_EQ(sink.last_item_idx, 21u);
    EXPECT_EQ(sink.last_item_pos, 22u);
    EXPECT_EQ(sink.silent_count, 0u);
}
