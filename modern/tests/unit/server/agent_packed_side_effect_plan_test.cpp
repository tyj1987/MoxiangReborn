// D4.118 -- AgentPacked side-effect plan unit tests.
//
// 1:1 lock the legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_PACKEDMsgParser (lines 2074-2100).
//

#include <gtest/gtest.h>

#include "mxh/server/agent_packed.hpp"
#include "mxh/server/agent_packed_side_effect_plan.hpp"

using namespace mxh::server;

TEST(PackedPlan, FanoutToUsersEmitsFanoutEffect) {
    PackedAction a{};
    a.kind = PackedActionKind::fanout_to_users;
    a.protocol = packed_normal;
    a.receiver_count = 3u;
    a.data_size = 64u;
    const auto plan = packed_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, PackedSideEffectKind::FanoutToUsers);
    EXPECT_EQ(plan.effects[0].receiver_count, 3u);
    EXPECT_EQ(plan.effects[0].data_size, 64u);
    EXPECT_TRUE(packed_effect_targets_user(plan.effects[0]));
}

TEST(PackedPlan, SendToMapServerByPortEmitsMapEffect) {
    PackedAction a{};
    a.kind = PackedActionKind::send_to_map_server_by_port;
    a.protocol = packed_to_mapserver;
    a.receiver_count = 1u;
    a.data_size = 128u;
    const auto plan = packed_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_EQ(plan.effects[0].kind, PackedSideEffectKind::SendToMapServerByPort);
    EXPECT_EQ(plan.effects[0].data_size, 128u);
    EXPECT_TRUE(packed_effect_targets_map(plan.effects[0]));
    EXPECT_FALSE(packed_effect_targets_user(plan.effects[0]));
}

TEST(PackedPlan, BroadcastToOtherMapsEmitsBroadcastEffect) {
    PackedAction a{};
    a.kind = PackedActionKind::broadcast_to_other_maps;
    a.protocol = packed_to_broad_mapserver;
    a.receiver_count = 5u;
    const auto plan = packed_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_EQ(plan.effects[0].kind, PackedSideEffectKind::BroadcastToOtherMaps);
    EXPECT_EQ(plan.effects[0].receiver_count, 5u);
    EXPECT_TRUE(packed_effect_targets_map(plan.effects[0]));
}

TEST(PackedPlan, UnknownEmitsDrop) {
    PackedAction a{};
    a.kind = PackedActionKind::unknown;
    const auto plan = packed_side_effect_plan(a);
    EXPECT_TRUE(plan.drop);
    EXPECT_EQ(plan.effects[0].kind, PackedSideEffectKind::Drop);
}

// Classifier 1:1

TEST(PackedClassifierPlan, NormalEmitsFanoutPlan) {
    PackedRequest req{};
    req.protocol = packed_normal;
    req.receivers_present = {1u, 2u, 3u};
    req.data_size = 64u;
    const auto action = classify_packed_user(req);
    EXPECT_EQ(action.kind, PackedActionKind::fanout_to_users);
    EXPECT_EQ(action.receiver_count, 3u);
    EXPECT_EQ(action.data_size, 64u);
    const auto plan = packed_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, PackedSideEffectKind::FanoutToUsers);
}

TEST(PackedClassifierPlan, ToMapServerWithPortEmitsMapPlan) {
    PackedRequest req{};
    req.protocol = packed_to_mapserver;
    req.target_map_port_found = true;
    req.receiver_count = 1u;
    const auto action = classify_packed_user(req);
    EXPECT_EQ(action.kind, PackedActionKind::send_to_map_server_by_port);
    const auto plan = packed_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, PackedSideEffectKind::SendToMapServerByPort);
}

TEST(PackedClassifierPlan, ToMapServerNoPortDrops) {
    PackedRequest req{};
    req.protocol = packed_to_mapserver;
    req.target_map_port_found = false;
    const auto action = classify_packed_user(req);
    EXPECT_EQ(action.kind, PackedActionKind::unknown);
    const auto plan = packed_side_effect_plan(action);
    EXPECT_TRUE(plan.drop);
}

TEST(PackedClassifierPlan, BroadMapServerEmitsBroadcastPlan) {
    PackedRequest req{};
    req.protocol = packed_to_broad_mapserver;
    req.receiver_count = 10u;
    const auto action = classify_packed_user(req);
    EXPECT_EQ(action.kind, PackedActionKind::broadcast_to_other_maps);
    EXPECT_EQ(action.receiver_count, 10u);
    const auto plan = packed_side_effect_plan(action);
    EXPECT_EQ(plan.effects[0].kind, PackedSideEffectKind::BroadcastToOtherMaps);
}

TEST(PackedClassifierPlan, UnknownProtocolDrops) {
    PackedRequest req{};
    req.protocol = 99u;
    const auto action = classify_packed_user(req);
    EXPECT_EQ(action.kind, PackedActionKind::unknown);
    const auto plan = packed_side_effect_plan(action);
    EXPECT_TRUE(plan.drop);
}

TEST(PackedClassifierPlan, AllThreeKnownProtocolsRouteCorrectly) {
    struct Pair { std::uint8_t proto; PackedActionKind expected_kind; PackedSideEffectKind expected_effect; };
    Pair cases[] = {{packed_normal, PackedActionKind::fanout_to_users, PackedSideEffectKind::FanoutToUsers}, {packed_to_mapserver, PackedActionKind::send_to_map_server_by_port, PackedSideEffectKind::SendToMapServerByPort}, {packed_to_broad_mapserver, PackedActionKind::broadcast_to_other_maps, PackedSideEffectKind::BroadcastToOtherMaps}};
    for (auto& c : cases) {
        PackedRequest req{};
        req.protocol = c.proto;
        if (c.proto == packed_to_mapserver) req.target_map_port_found = true;
        const auto action = classify_packed_user(req);
        EXPECT_EQ(action.kind, c.expected_kind);
        const auto plan = packed_side_effect_plan(action);
        EXPECT_EQ(plan.effects[0].kind, c.expected_effect);
    }
}