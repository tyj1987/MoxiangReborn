// D4.170 -- 1:1 lock the legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_STREETSTALLUserMsgParser (lines 5083-5092). Each test pins one branch of the
// legacy dispatch to its modern side-effect plan output so future drift triggers a
// test failure.

#include <gtest/gtest.h>

#include "mxh/server/agent_streetstall.hpp"
#include "mxh/server/agent_streetstall_side_effect_plan.hpp"

using namespace mxh::server;

namespace {
constexpr std::uint32_t kObjectId = 0xAABBCCDDu;
constexpr std::uint32_t kCharId = 0x11223344u;
}

TEST(StreetStallPlan, UserNotFoundEmitsDrop) {
    StreetStallUserRequest r;
    r.protocol = streetstall_open_syn;
    r.dw_object_id = kObjectId;
    r.user_found = false;
    auto a = classify_streetstall_user(r);
    EXPECT_EQ(a.kind, StreetStallUserActionKind::drop_no_user);
    const auto plan = streetstall_user_side_effect_plan(a);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
    EXPECT_FALSE(plan.forward_to_map_server);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, StreetStallSideEffectKind::Drop);
    EXPECT_EQ(plan.effects[0].reply_protocol, streetstall_open_syn);
    EXPECT_EQ(plan.effects[0].dw_object_id, kObjectId);
    EXPECT_FALSE(plan.effects[0].forward_payload);
}

TEST(StreetStallPlan, UserObjectIdMismatchEmitsDrop) {
    StreetStallUserRequest r;
    r.protocol = streetstall_buyitem_syn;
    r.dw_object_id = kObjectId;
    r.dw_user_character_id = kCharId;
    r.user_found = true;
    auto a = classify_streetstall_user(r);
    EXPECT_EQ(a.kind, StreetStallUserActionKind::drop_object_id_mismatch);
    const auto plan = streetstall_user_side_effect_plan(a);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, StreetStallSideEffectKind::Drop);
    EXPECT_EQ(plan.effects[0].reply_protocol, streetstall_buyitem_syn);
    EXPECT_FALSE(plan.effects[0].forward_payload);
}

TEST(StreetStallPlan, UserObjectIdMatchForwardsToMapServer) {
    StreetStallUserRequest r;
    r.protocol = streetstall_buyitem_syn;
    r.dw_object_id = kCharId;
    r.dw_user_character_id = kCharId;
    r.user_found = true;
    auto a = classify_streetstall_user(r);
    EXPECT_EQ(a.kind, StreetStallUserActionKind::forward_to_map_server);
    const auto plan = streetstall_user_side_effect_plan(a);
    EXPECT_FALSE(plan.drop);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_TRUE(plan.forward_to_map_server);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, StreetStallSideEffectKind::ForwardToMapServer);
    EXPECT_EQ(plan.effects[0].reply_protocol, streetstall_buyitem_syn);
    EXPECT_EQ(plan.effects[0].dw_object_id, kCharId);
    EXPECT_TRUE(plan.effects[0].forward_payload);
}

TEST(StreetStallPlan, UserStartProtocolForwardsToMapServer) {
    StreetStallUserRequest r;
    r.protocol = streetstall_start;
    r.dw_object_id = kObjectId;
    r.dw_user_character_id = kObjectId;
    r.user_found = true;
    auto a = classify_streetstall_user(r);
    EXPECT_EQ(a.kind, StreetStallUserActionKind::forward_to_map_server);
    const auto plan = streetstall_user_side_effect_plan(a);
    EXPECT_TRUE(plan.forward_to_map_server);
    EXPECT_EQ(plan.effects[0].reply_protocol, streetstall_start);
}

TEST(StreetStallPlan, UserEndProtocolForwardsToMapServer) {
    StreetStallUserRequest r;
    r.protocol = streetstall_end;
    r.dw_object_id = kObjectId;
    r.dw_user_character_id = kObjectId;
    r.user_found = true;
    auto a = classify_streetstall_user(r);
    EXPECT_EQ(a.kind, StreetStallUserActionKind::forward_to_map_server);
    const auto plan = streetstall_user_side_effect_plan(a);
    EXPECT_TRUE(plan.forward_to_map_server);
    EXPECT_EQ(plan.effects[0].reply_protocol, streetstall_end);
}

TEST(StreetStallPlan, PlanDefaultsAreConservative) {
    StreetStallSideEffectPlan plan;
    EXPECT_FALSE(plan.dispatched);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.forward_to_map_server);
    EXPECT_TRUE(plan.effects.empty());
}

