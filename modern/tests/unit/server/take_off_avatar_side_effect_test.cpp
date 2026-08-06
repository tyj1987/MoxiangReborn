// D4.37 TakeOffAvatarItem side-effect dispatcher tests.

#include <mxh/server/take_off_avatar_side_effect.hpp>

#include <gtest/gtest.h>

#include <cstdint>

using namespace mxh::server;
using namespace mxh::game;

namespace {

AvatarEquipTransition make_transition(bool send_info, bool recalc,
                                      bool calc_stats = true) {
    AvatarEquipTransition t;
    t.status = AvatarEquipStatus::Ok;
    t.send_avatar_info = send_info;
    t.recalculate_avatar_option = recalc;
    t.calc_stats = calc_stats;
    return t;
}

}  // namespace

TEST(TakeOffAvatarSideEffect, AlwaysEmitsBothEffectsOnSuccess) {
    auto t = make_transition(/*send_info=*/true, /*recalc=*/true);
    auto plan = take_off_avatar_side_effect_plan(t, /*dw_item_index=*/10);
    ASSERT_EQ(plan.effects.size(), 2u);
    EXPECT_EQ(plan.effects[0].kind,
              TakeOffAvatarSideEffectKind::BroadcastAvatarInfo);
    EXPECT_EQ(plan.effects[1].kind,
              TakeOffAvatarSideEffectKind::RecomputeAvatarOption);
    EXPECT_EQ(plan.effects[0].w_icon_idx, 10u);
    EXPECT_EQ(plan.effects[1].w_icon_idx, 10u);
}

TEST(TakeOffAvatarSideEffect, BroadcastOnlyWhenSendInfoFlag) {
    auto t = make_transition(/*send_info=*/false, /*recalc=*/true);
    auto plan = take_off_avatar_side_effect_plan(t, /*dw_item_index=*/10);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              TakeOffAvatarSideEffectKind::RecomputeAvatarOption);
}

TEST(TakeOffAvatarSideEffect, RecomputeOnlyWhenRecalcFlag) {
    auto t = make_transition(/*send_info=*/true, /*recalc=*/false);
    auto plan = take_off_avatar_side_effect_plan(t, /*dw_item_index=*/10);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind,
              TakeOffAvatarSideEffectKind::BroadcastAvatarInfo);
}

TEST(TakeOffAvatarSideEffect, EmptyWhenNoFlags) {
    auto t = make_transition(/*send_info=*/false, /*recalc=*/false);
    auto plan = take_off_avatar_side_effect_plan(t, /*dw_item_index=*/0);
    EXPECT_TRUE(plan.effects.empty());
}

TEST(TakeOffAvatarSideEffect, CalcStatsFalsePropagates) {
    auto t = make_transition(/*send_info=*/true, /*recalc=*/true,
                             /*calc_stats=*/false);
    auto plan = take_off_avatar_side_effect_plan(t, /*dw_item_index=*/1);
    ASSERT_EQ(plan.effects.size(), 2u);
    for (const auto& e : plan.effects) {
        EXPECT_FALSE(e.calc_stats);
    }
}
