// D4.109 -- AgentSiegeWarProfit side-effect plan unit tests.
//
// 1:1 lock the legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_SIEGEWARPROFITUserMsgParser + MP_SIEGEWARPROFITServerMsgParser.
//

#include <gtest/gtest.h>

#include "mxh/server/agent_siegewarprofit.hpp"
#include "mxh/server/agent_siegewarprofit_side_effect_plan.hpp"

using namespace mxh::server;

// USER

TEST(SiegeWarProfitUserPlan, ForwardToMapEmitsRawForward) {
    SiegeWarProfitUserAction a{};
    a.kind = SiegeWarProfitUserActionKind::forward_to_map;
    const auto plan = siegewarprofit_user_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, SiegeWarProfitUserSideEffectKind::ForwardRawToMap);
    EXPECT_TRUE(siegewarprofit_user_effect_targets_map(plan.effects[0]));
}

TEST(SiegeWarProfitUserClassifierPlan, UserAlwaysForwardsToMap) {
    const auto action = classify_siegewarprofit_user();
    EXPECT_EQ(action.kind, SiegeWarProfitUserActionKind::forward_to_map);
    const auto plan = siegewarprofit_user_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, SiegeWarProfitUserSideEffectKind::ForwardRawToMap);
}

// SERVER

TEST(SiegeWarProfitServerPlan, BroadcastToOtherMapsEmitsBroadcastEffect) {
    SiegeWarProfitServerAction a{};
    a.kind = SiegeWarProfitServerActionKind::broadcast_to_other_maps;
    a.protocol = siegewarprofit_change_texrate_notify_to_map;
    const auto plan = siegewarprofit_server_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, SiegeWarProfitServerSideEffectKind::BroadcastToOtherMaps);
    EXPECT_EQ(plan.effects[0].protocol, siegewarprofit_change_texrate_notify_to_map);
    EXPECT_TRUE(siegewarprofit_server_effect_targets_map(plan.effects[0]));
    EXPECT_FALSE(siegewarprofit_server_effect_targets_client(plan.effects[0]));
}

TEST(SiegeWarProfitServerPlan, ForwardToClientEmitsClientForwardEffect) {
    SiegeWarProfitServerAction a{};
    a.kind = SiegeWarProfitServerActionKind::forward_to_client;
    a.protocol = 99u;
    const auto plan = siegewarprofit_server_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, SiegeWarProfitServerSideEffectKind::ForwardRawToClient);
    EXPECT_EQ(plan.effects[0].protocol, 99u);
    EXPECT_TRUE(siegewarprofit_server_effect_targets_client(plan.effects[0]));
    EXPECT_FALSE(siegewarprofit_server_effect_targets_map(plan.effects[0]));
}

TEST(SiegeWarProfitServerClassifierPlan, ChangeTexRateEmitsBroadcastPlan) {
    SiegeWarProfitRequest req{};
    req.protocol = siegewarprofit_change_texrate_notify_to_map;
    const auto action = classify_siegewarprofit_server(req);
    EXPECT_EQ(action.kind, SiegeWarProfitServerActionKind::broadcast_to_other_maps);
    EXPECT_EQ(action.protocol, siegewarprofit_change_texrate_notify_to_map);
    const auto plan = siegewarprofit_server_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, SiegeWarProfitServerSideEffectKind::BroadcastToOtherMaps);
}

TEST(SiegeWarProfitServerClassifierPlan, ChangeGuildEmitsBroadcastPlan) {
    SiegeWarProfitRequest req{};
    req.protocol = siegewarprofit_change_guild_notify_to_map;
    const auto action = classify_siegewarprofit_server(req);
    EXPECT_EQ(action.kind, SiegeWarProfitServerActionKind::broadcast_to_other_maps);
    EXPECT_EQ(action.protocol, siegewarprofit_change_guild_notify_to_map);
    const auto plan = siegewarprofit_server_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, SiegeWarProfitServerSideEffectKind::BroadcastToOtherMaps);
}

TEST(SiegeWarProfitServerClassifierPlan, UnknownServerProtocolEmitsForwardToClient) {
    SiegeWarProfitRequest req{};
    req.protocol = 99u;
    const auto action = classify_siegewarprofit_server(req);
    EXPECT_EQ(action.kind, SiegeWarProfitServerActionKind::forward_to_client);
    EXPECT_EQ(action.protocol, 99u);
    const auto plan = siegewarprofit_server_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, SiegeWarProfitServerSideEffectKind::ForwardRawToClient);
}

TEST(SiegeWarProfitClassifierPlan, BothBroadcastProtocolsRouteToBroadcast) {
    for (auto proto : {siegewarprofit_change_texrate_notify_to_map, siegewarprofit_change_guild_notify_to_map}) {
        SiegeWarProfitRequest req{};
        req.protocol = proto;
        const auto action = classify_siegewarprofit_server(req);
        EXPECT_EQ(action.kind, SiegeWarProfitServerActionKind::broadcast_to_other_maps);
        const auto plan = siegewarprofit_server_side_effect_plan(action);
        EXPECT_EQ(plan.effects[0].kind, SiegeWarProfitServerSideEffectKind::BroadcastToOtherMaps);
    }
}