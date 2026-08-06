// D4.36 PutOnAvatarItem side-effect dispatcher tests.

#include <mxh/server/put_on_avatar_side_effect.hpp>

#include <gtest/gtest.h>

#include <cstdint>

using namespace mxh::server;
using namespace mxh::game;

namespace {

AvatarEquipTransition make_transition(bool send_info, bool recalc) {
    AvatarEquipTransition t;
    t.status = AvatarEquipStatus::Ok;
    t.send_avatar_info = send_info;
    t.recalculate_avatar_option = recalc;
    t.calc_stats = true;
    return t;
}

}  // namespace

TEST(PutOnAvatarSideEffect, BroadcastOnlyWhenItemPosNonZero) {
    auto t = make_transition(/*send_info=*/true, /*recalc=*/false);
    auto plan = put_on_avatar_side_effect_plan(t, /*dw_item_index=*/10);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              PutOnAvatarSideEffectKind::BroadcastAvatarInfo);
    EXPECT_EQ(plan.effects[0].w_icon_idx, 10u);
}

TEST(PutOnAvatarSideEffect, RecomputeOnlyWhenFlagSet) {
    auto t = make_transition(/*send_info=*/false, /*recalc=*/true);
    auto plan = put_on_avatar_side_effect_plan(t, /*dw_item_index=*/10);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              PutOnAvatarSideEffectKind::RecomputeAvatarOption);
}

TEST(PutOnAvatarSideEffect, BothEffectsWhenBothFlagsSet) {
    auto t = make_transition(/*send_info=*/true, /*recalc=*/true);
    auto plan = put_on_avatar_side_effect_plan(t, /*dw_item_index=*/42);
    ASSERT_EQ(plan.effects.size(), 2u);
    EXPECT_EQ(plan.effects[0].kind,
              PutOnAvatarSideEffectKind::BroadcastAvatarInfo);
    EXPECT_EQ(plan.effects[1].kind,
              PutOnAvatarSideEffectKind::RecomputeAvatarOption);
    EXPECT_EQ(plan.effects[1].w_icon_idx, 42u);
}

TEST(PutOnAvatarSideEffect, EmptyWhenNoFlagsSet) {
    auto t = make_transition(/*send_info=*/false, /*recalc=*/false);
    auto plan = put_on_avatar_side_effect_plan(t, /*dw_item_index=*/0);
    EXPECT_TRUE(plan.effects.empty());
}

TEST(PutOnAvatarSideEffect, CalcStatsPropagates) {
    auto t = make_transition(/*send_info=*/true, /*recalc=*/true);
    t.calc_stats = false;
    auto plan = put_on_avatar_side_effect_plan(t, /*dw_item_index=*/1);
    ASSERT_EQ(plan.effects.size(), 2u);
    for (const auto& e : plan.effects) {
        EXPECT_FALSE(e.calc_stats);
    }
}
