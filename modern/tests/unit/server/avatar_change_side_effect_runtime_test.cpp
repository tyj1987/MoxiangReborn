// avatar_change_side_effect_runtime_test.cpp
//
// Verifies apply_avatar_change_side_effects() (the runtime
// orchestrator for the CItemManager::MP_ITEM_SHOPITEM_AVATAR_CHANGE
// side-effect chain) walks the data-plane plan and dispatches the
// 2-step chain (recalc options -> broadcast puton) in legacy order,
// and stays a no-op when the player is missing.

#include <mxh/server/avatar_change_side_effect.hpp>
#include <mxh/server/avatar_change_side_effect_runtime.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {

using mxh::server::AvatarChangeSideEffectKind;
using mxh::server::AvatarChangeSideEffectSink;
using mxh::server::apply_avatar_change_side_effects;
using mxh::server::avatar_change_side_effect_plan;

class RecordingSink final : public AvatarChangeSideEffectSink {
public:
    std::vector<std::string> calls;
    std::uint32_t last_object_id = 0;
    std::uint16_t last_item_pos = 0;

    void recalc_shop_item_option(std::uint32_t object_id,
                                 std::uint16_t item_pos) override {
        calls.push_back("recalc");
        last_object_id = object_id;
        last_item_pos = item_pos;
    }
    void broadcast_avatar_puton(std::uint32_t object_id,
                                std::uint16_t item_pos) override {
        calls.push_back("broadcast");
        last_object_id = object_id;
        last_item_pos = item_pos;
    }
};

}  // namespace

TEST(ApplyAvatarChangeSideEffects, PlayerFoundEmitsRecalcThenBroadcast) {
    mxh::server::AvatarChangeValidationInput in;
    in.player_found = true;
    auto plan = avatar_change_side_effect_plan(
        in, /*object_id=*/0x00010002u, /*item_pos=*/7);
    EXPECT_TRUE(plan.recalc);
    EXPECT_TRUE(plan.broadcast);
    ASSERT_EQ(plan.effects.size(), 2u);
    EXPECT_EQ(plan.effects[0].kind,
              AvatarChangeSideEffectKind::RecalcShopItemOption);
    EXPECT_EQ(plan.effects[1].kind,
              AvatarChangeSideEffectKind::BroadcastAvatarPuton);

    RecordingSink sink;
    auto out = apply_avatar_change_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 2u);
    EXPECT_EQ(out.recalcs, 1u);
    EXPECT_EQ(out.broadcasts, 1u);
    EXPECT_TRUE(out.recalc_flag_consumed);
    EXPECT_TRUE(out.broadcast_flag_consumed);
    ASSERT_EQ(sink.calls.size(), 2u);
    EXPECT_EQ(sink.calls[0], "recalc");
    EXPECT_EQ(sink.calls[1], "broadcast");
    EXPECT_EQ(sink.last_object_id, 0x00010002u);
    EXPECT_EQ(sink.last_item_pos, 7u);
}

TEST(ApplyAvatarChangeSideEffects, NoPlayerIsNoOp) {
    mxh::server::AvatarChangeValidationInput in;
    in.player_found = false;
    auto plan = avatar_change_side_effect_plan(in, 7, 8);
    EXPECT_FALSE(plan.recalc);
    EXPECT_FALSE(plan.broadcast);
    EXPECT_TRUE(plan.effects.empty());

    RecordingSink sink;
    auto out = apply_avatar_change_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.recalcs, 0u);
    EXPECT_EQ(out.broadcasts, 0u);
    EXPECT_FALSE(out.recalc_flag_consumed);
    EXPECT_FALSE(out.broadcast_flag_consumed);
    EXPECT_TRUE(sink.calls.empty());
}

TEST(ApplyAvatarChangeSideEffects, EmptyPlanIsNoOp) {
    mxh::server::AvatarChangeSideEffectPlan plan;
    RecordingSink sink;
    auto out = apply_avatar_change_side_effects(plan, sink);
    EXPECT_EQ(out.effects_applied, 0u);
    EXPECT_EQ(out.recalcs, 0u);
    EXPECT_EQ(out.broadcasts, 0u);
    EXPECT_FALSE(out.recalc_flag_consumed);
    EXPECT_FALSE(out.broadcast_flag_consumed);
    EXPECT_TRUE(sink.calls.empty());
}

TEST(ApplyAvatarChangeSideEffects, MaxObjectIdStillDispatches) {
    mxh::server::AvatarChangeValidationInput in;
    in.player_found = true;
    auto plan = avatar_change_side_effect_plan(
        in, 0xFFFFFFFFu, 65535);
    EXPECT_TRUE(plan.recalc);
    EXPECT_TRUE(plan.broadcast);

    RecordingSink sink;
    (void)apply_avatar_change_side_effects(plan, sink);
    EXPECT_EQ(sink.calls,
              std::vector<std::string>({"recalc", "broadcast"}));
    EXPECT_EQ(sink.last_object_id, 0xFFFFFFFFu);
    EXPECT_EQ(sink.last_item_pos, 65535u);
}

TEST(ApplyAvatarChangeSideEffects, ZeroItemPosStillDispatches) {
    mxh::server::AvatarChangeValidationInput in;
    in.player_found = true;
    auto plan = avatar_change_side_effect_plan(in, 7, 0);
    EXPECT_TRUE(plan.recalc);

    RecordingSink sink;
    (void)apply_avatar_change_side_effects(plan, sink);
    EXPECT_EQ(sink.calls,
              std::vector<std::string>({"recalc", "broadcast"}));
    EXPECT_EQ(sink.last_object_id, 7u);
    EXPECT_EQ(sink.last_item_pos, 0u);
}
