// D4.105 -- AgentSurvival side-effect plan unit tests.
//
// 1:1 lock the legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_SURVIVALUserMsgParser (lines 5094-5105) and MP_SURVIVALServerMsgParser
// (lines 5105-5158). Each test pins one branch of the legacy dispatch to its modern
// side-effect plan output so future drift triggers a test failure.
//

#include <gtest/gtest.h>

#include "mxh/server/agent_survival.hpp"
#include "mxh/server/agent_survival_side_effect_plan.hpp"

using namespace mxh::server;

// ---------------------------------------------------------------------
// USER side-effects
// ---------------------------------------------------------------------

TEST(SurvivalUserPlan, LeaveSynSendsToMapWithUniqueConnectIdx) {
    SurvivalUserAction a{};
    a.kind = SurvivalUserActionKind::send_leave_syn_to_map;
    a.protocol = survival_leave_syn;
    a.object_id = 7u;
    a.unique_connect_idx = 100u;
    a.user_level = 5u;
    a.channel = 3u;
    const auto plan = survival_user_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, SurvivalUserSideEffectKind::SendLeaveSynToMap);
    EXPECT_EQ(plan.effects[0].object_id, 7u);
    EXPECT_EQ(plan.effects[0].unique_connect_idx, 100u);
    EXPECT_EQ(plan.effects[0].user_level, 5u);
    EXPECT_EQ(plan.effects[0].channel, 3u);
    EXPECT_TRUE(survival_user_effect_targets_map(plan.effects[0]));
}

TEST(SurvivalUserPlan, GmProtectedForwardEmitsRawForward) {
    SurvivalUserAction a{};
    a.kind = SurvivalUserActionKind::gm_protected_forward_to_map;
    a.protocol = survival_ready_syn;
    a.object_id = 7u;
    const auto plan = survival_user_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_EQ(plan.effects[0].kind, SurvivalUserSideEffectKind::ForwardRawToMap);
    EXPECT_TRUE(survival_user_effect_targets_map(plan.effects[0]));
}

TEST(SurvivalUserPlan, StopSynGmProtectedForward) {
    SurvivalUserAction a{};
    a.kind = SurvivalUserActionKind::gm_protected_forward_to_map;
    a.protocol = survival_stop_syn;
    const auto plan = survival_user_side_effect_plan(a);
    EXPECT_EQ(plan.effects[0].kind, SurvivalUserSideEffectKind::ForwardRawToMap);
}

TEST(SurvivalUserPlan, MapOffSynGmProtectedForward) {
    SurvivalUserAction a{};
    a.kind = SurvivalUserActionKind::gm_protected_forward_to_map;
    a.protocol = survival_mapoff_syn;
    const auto plan = survival_user_side_effect_plan(a);
    EXPECT_EQ(plan.effects[0].kind, SurvivalUserSideEffectKind::ForwardRawToMap);
}

TEST(SurvivalUserPlan, ItemUsingCountSetGmProtectedForward) {
    SurvivalUserAction a{};
    a.kind = SurvivalUserActionKind::gm_protected_forward_to_map;
    a.protocol = survival_itemusingcount_set;
    const auto plan = survival_user_side_effect_plan(a);
    EXPECT_EQ(plan.effects[0].kind, SurvivalUserSideEffectKind::ForwardRawToMap);
}

TEST(SurvivalUserPlan, DefaultForwardEmitsRawForward) {
    SurvivalUserAction a{};
    a.kind = SurvivalUserActionKind::default_forward_to_map;
    a.protocol = survival_info;
    const auto plan = survival_user_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_EQ(plan.effects[0].kind, SurvivalUserSideEffectKind::ForwardRawToMap);
}

TEST(SurvivalUserPlan, NoUserStillDefaultForwards) {
    SurvivalUserAction a{};
    a.kind = SurvivalUserActionKind::default_forward_to_map;
    a.protocol = survival_aliveuser_count;
    const auto plan = survival_user_side_effect_plan(a);
    EXPECT_EQ(plan.effects[0].kind, SurvivalUserSideEffectKind::ForwardRawToMap);
}

// ---------------------------------------------------------------------
// SERVER side-effects
// ---------------------------------------------------------------------

TEST(SurvivalServerPlan, ReturnToMapWithPortUpdatesUserState) {
    SurvivalServerAction a{};
    a.kind = SurvivalServerActionKind::update_user_map_and_forward_to_client;
    a.protocol = survival_returntomap;
    a.object_id = 11u;
    a.target_map = 17u;
    a.update_user_state = true;
    const auto plan = survival_server_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, SurvivalServerSideEffectKind::UpdateUserMapAndForwardToClient);
    EXPECT_EQ(plan.effects[0].target_map, 17u);
    EXPECT_TRUE(plan.effects[0].update_user_state);
    EXPECT_TRUE(survival_server_effect_targets_client(plan.effects[0]));
}

TEST(SurvivalServerPlan, ReturnToMapNoPortForwardsToClient) {
    SurvivalServerAction a{};
    a.kind = SurvivalServerActionKind::default_forward_to_client;
    a.protocol = survival_returntomap;
    a.object_id = 11u;
    const auto plan = survival_server_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_EQ(plan.effects[0].kind, SurvivalServerSideEffectKind::ForwardRawToClient);
    EXPECT_FALSE(plan.effects[0].update_user_state);
}

TEST(SurvivalServerPlan, UnknownProtocolForwardsToClient) {
    SurvivalServerAction a{};
    a.kind = SurvivalServerActionKind::default_forward_to_client;
    a.protocol = 99u;
    const auto plan = survival_server_side_effect_plan(a);
    EXPECT_EQ(plan.effects[0].kind, SurvivalServerSideEffectKind::ForwardRawToClient);
}

// ---------------------------------------------------------------------
// Classifier 1:1 tests
// ---------------------------------------------------------------------

TEST(SurvivalUserClassifierPlan, LeaveSynEmitsSendLeaveSynPlan) {
    SurvivalUserRequest req{};
    req.protocol = survival_leave_syn;
    req.user_found = true;
    req.object_id = 7u;
    req.unique_connect_idx = 100u;
    req.user_level = 5u;
    req.channel = 3u;
    const auto action = classify_survival_user(req);
    EXPECT_EQ(action.kind, SurvivalUserActionKind::send_leave_syn_to_map);
    EXPECT_EQ(action.unique_connect_idx, 100u);
    EXPECT_EQ(action.user_level, 5u);
    EXPECT_EQ(action.channel, 3u);
    const auto plan = survival_user_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, SurvivalUserSideEffectKind::SendLeaveSynToMap);
}

TEST(SurvivalUserClassifierPlan, ReadySynEmitsGmProtectedForward) {
    SurvivalUserRequest req{};
    req.protocol = survival_ready_syn;
    req.user_found = true;
    const auto action = classify_survival_user(req);
    EXPECT_EQ(action.kind, SurvivalUserActionKind::gm_protected_forward_to_map);
    const auto plan = survival_user_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, SurvivalUserSideEffectKind::ForwardRawToMap);
}

TEST(SurvivalUserClassifierPlan, NoUserDefaultsToForwardToMap) {
    SurvivalUserRequest req{};
    req.protocol = survival_leave_syn;
    req.user_found = false;
    const auto action = classify_survival_user(req);
    EXPECT_EQ(action.kind, SurvivalUserActionKind::default_forward_to_map);
    const auto plan = survival_user_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, SurvivalUserSideEffectKind::ForwardRawToMap);
}

TEST(SurvivalUserClassifierPlan, InfoProtocolEmitsDefaultForward) {
    SurvivalUserRequest req{};
    req.protocol = survival_info;
    req.user_found = true;
    const auto action = classify_survival_user(req);
    EXPECT_EQ(action.kind, SurvivalUserActionKind::default_forward_to_map);
    const auto plan = survival_user_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, SurvivalUserSideEffectKind::ForwardRawToMap);
}

TEST(SurvivalUserClassifierPlan, AllGmProtectedProtocolsRouteCorrectly) {
    for (auto proto : {survival_ready_syn, survival_stop_syn, survival_mapoff_syn, survival_itemusingcount_set}) {
        SurvivalUserRequest req{};
        req.protocol = proto;
        req.user_found = true;
        const auto action = classify_survival_user(req);
        EXPECT_EQ(action.kind, SurvivalUserActionKind::gm_protected_forward_to_map);
        const auto plan = survival_user_side_effect_plan(action);
        EXPECT_EQ(plan.effects[0].kind, SurvivalUserSideEffectKind::ForwardRawToMap);
    }
}

TEST(SurvivalServerClassifierPlan, ReturnToMapWithPortEmitsUpdatePlan) {
    SurvivalServerRequest req{};
    req.protocol = survival_returntomap;
    req.user_found = true;
    req.target_map = 17u;
    req.target_map_port_found = true;
    const auto action = classify_survival_server(req);
    EXPECT_EQ(action.kind, SurvivalServerActionKind::update_user_map_and_forward_to_client);
    EXPECT_EQ(action.target_map, 17u);
    EXPECT_TRUE(action.update_user_state);
    const auto plan = survival_server_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, SurvivalServerSideEffectKind::UpdateUserMapAndForwardToClient);
}

TEST(SurvivalServerClassifierPlan, ReturnToMapNoPortEmitsDefaultPlan) {
    SurvivalServerRequest req{};
    req.protocol = survival_returntomap;
    req.user_found = true;
    req.target_map_port_found = false;
    const auto action = classify_survival_server(req);
    EXPECT_EQ(action.kind, SurvivalServerActionKind::default_forward_to_client);
    const auto plan = survival_server_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, SurvivalServerSideEffectKind::ForwardRawToClient);
}

TEST(SurvivalServerClassifierPlan, ReturnToMapNoUserEmitsDefaultPlan) {
    SurvivalServerRequest req{};
    req.protocol = survival_returntomap;
    req.user_found = false;
    req.target_map_port_found = true;
    const auto action = classify_survival_server(req);
    EXPECT_EQ(action.kind, SurvivalServerActionKind::default_forward_to_client);
    const auto plan = survival_server_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, SurvivalServerSideEffectKind::ForwardRawToClient);
}

TEST(SurvivalServerClassifierPlan, UnknownServerProtocolEmitsDefaultPlan) {
    SurvivalServerRequest req{};
    req.protocol = 99u;
    const auto action = classify_survival_server(req);
    EXPECT_EQ(action.kind, SurvivalServerActionKind::default_forward_to_client);
    const auto plan = survival_server_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, SurvivalServerSideEffectKind::ForwardRawToClient);
}