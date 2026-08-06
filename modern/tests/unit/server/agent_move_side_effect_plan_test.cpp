// D4.115 -- AgentMove side-effect plan unit tests.
//
// 1:1 lock the legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_MOVEMsgParser.
//

#include <gtest/gtest.h>

#include "mxh/server/agent_move.hpp"
#include "mxh/server/agent_move_side_effect_plan.hpp"

using namespace mxh::server;

TEST(MovePlan, DropNoMapEmitsDropEffect) {
    MoveAction a{};
    a.kind = MoveActionKind::drop_no_map;
    a.protocol = move_target;
    a.object_id = 11u;
    const auto plan = move_side_effect_plan(a);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, MoveSideEffectKind::Drop);
    EXPECT_EQ(plan.effects[0].reply_protocol, move_target);
    EXPECT_FALSE(move_effect_targets_map(plan.effects[0]));
}

TEST(MovePlan, ForwardToMapEmitsRawForward) {
    MoveAction a{};
    a.kind = MoveActionKind::forward_to_map;
    a.protocol = move_walkmode;
    a.object_id = 11u;
    const auto plan = move_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, MoveSideEffectKind::ForwardRawToMap);
    EXPECT_TRUE(move_effect_targets_map(plan.effects[0]));
}

TEST(MovePlan, ForwardToMapIfInMapAlsoEmitsRawForward) {
    MoveAction a{};
    a.kind = MoveActionKind::forward_to_map_if_in_map;
    a.protocol = move_kyunggong_syn;
    const auto plan = move_side_effect_plan(a);
    EXPECT_EQ(plan.effects[0].kind, MoveSideEffectKind::ForwardRawToMap);
}

// Classifier 1:1

TEST(MoveClassifierPlan, UserInMapForwardsToMap) {
    MoveRequest req{};
    req.protocol = move_target;
    req.object_id = 11u;
    req.user_in_map = true;
    const auto action = classify_move(req);
    EXPECT_EQ(action.kind, MoveActionKind::forward_to_map);
    const auto plan = move_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, MoveSideEffectKind::ForwardRawToMap);
}

TEST(MoveClassifierPlan, UserNotInMapDrops) {
    MoveRequest req{};
    req.protocol = move_target;
    req.object_id = 11u;
    req.user_in_map = false;
    const auto action = classify_move(req);
    EXPECT_EQ(action.kind, MoveActionKind::drop_no_map);
    const auto plan = move_side_effect_plan(action);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, MoveSideEffectKind::Drop);
}

TEST(MoveClassifierPlan, AllProtocolsWithUserInMapRouteToForward) {
    for (auto proto : {move_init, move_target, move_correction, move_walkmode, move_runmode, move_kyunggong_syn, move_kyunggong_ack, move_kyunggong_nack, move_stop, move_effectmove, move_monstermove_notify, move_forcestopkyunggong, move_warp, move_onetarget, move_pet_onetarget, move_pet_target, move_pet_stop, move_pet_correction, move_pet_warp_syn, move_pet_warp_ack}) {
        MoveRequest req{};
        req.protocol = proto;
        req.user_in_map = true;
        const auto action = classify_move(req);
        EXPECT_EQ(action.kind, MoveActionKind::forward_to_map);
        const auto plan = move_side_effect_plan(action);
        EXPECT_EQ(plan.effects[0].kind, MoveSideEffectKind::ForwardRawToMap);
    }
}

TEST(MoveClassifierPlan, AllProtocolsWithUserNotInMapRouteToDrop) {
    for (auto proto : {move_init, move_target, move_correction, move_walkmode, move_runmode, move_kyunggong_syn, move_stop, move_warp}) {
        MoveRequest req{};
        req.protocol = proto;
        req.user_in_map = false;
        const auto action = classify_move(req);
        EXPECT_EQ(action.kind, MoveActionKind::drop_no_map);
        const auto plan = move_side_effect_plan(action);
        EXPECT_TRUE(plan.drop);
    }
}