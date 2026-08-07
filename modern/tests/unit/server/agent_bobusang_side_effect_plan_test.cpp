// D4.168 -- 1:1 lock the legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_BOBUSANGUserMsgParser (lines 5191-5210) and MP_BOBUSANGServerMsgParser
// (lines 5212-5234). Each test pins one branch of the legacy dispatch to its
// modern side-effect plan output so future drift triggers a test failure.

#include <gtest/gtest.h>

#include "mxh/server/agent_bobusang.hpp"
#include "mxh/server/agent_bobusang_side_effect_plan.hpp"

using namespace mxh::server;

namespace {
constexpr std::uint32_t kObjectId = 0xBABEFACEu;
constexpr std::uint32_t kChannel = 7u;
}

TEST(BobusangPlan, UserNotFoundEmitsDrop) {
    BobusangUserRequest r;
    r.protocol = bobusang_add_guest_syn;
    r.dw_object_id = kObjectId;
    r.user_found = false;
    auto a = classify_bobusang_user(r);
    const auto plan = bobusang_user_side_effect_plan(a);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, BobusangSideEffectKind::Drop);
    EXPECT_EQ(plan.effects[0].reply_protocol, bobusang_add_guest_syn);
    EXPECT_EQ(plan.effects[0].dw_object_id, kObjectId);
    EXPECT_FALSE(plan.effects[0].forward_payload);
}

TEST(BobusangPlan, UserGmOvershootEmitsDrop) {
    BobusangUserRequest r;
    r.protocol = bobusang_add_guest_syn;
    r.user_found = true;
    r.user_is_gm = true;
    r.gm_power = bobusang_gm_power_master + 1u;
    auto a = classify_bobusang_user(r);
    EXPECT_EQ(a.kind, BobusangUserActionKind::drop_gm_overshoot);
    const auto plan = bobusang_user_side_effect_plan(a);
    EXPECT_TRUE(plan.drop);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, BobusangSideEffectKind::Drop);
    EXPECT_FALSE(plan.effects[0].forward_payload);
}

TEST(BobusangPlan, UserNonGmForwardsToMapServer) {
    BobusangUserRequest r;
    r.protocol = bobusang_add_guest_syn;
    r.dw_object_id = kObjectId;
    r.user_found = true;
    r.user_is_gm = false;
    auto a = classify_bobusang_user(r);
    const auto plan = bobusang_user_side_effect_plan(a);
    EXPECT_FALSE(plan.drop);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_TRUE(plan.forward_to_map_server);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, BobusangSideEffectKind::ForwardToMapServer);
    EXPECT_EQ(plan.effects[0].reply_protocol, bobusang_add_guest_syn);
    EXPECT_EQ(plan.effects[0].dw_object_id, kObjectId);
    EXPECT_TRUE(plan.effects[0].forward_payload);
}

TEST(BobusangPlan, ServerAppearMapToAgentSetsChannelStateAppear) {
    BobusangServerRequest r;
    r.protocol = bobusang_appear_map_to_agent;
    r.dword.dw_object_id = kObjectId;
    r.dword.dw_data = kChannel;
    auto a = classify_bobusang_server(r);
    EXPECT_EQ(a.kind, BobusangServerActionKind::set_channel_state);
    const auto plan = bobusang_server_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_FALSE(plan.drop);
    EXPECT_TRUE(plan.set_channel_state);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, BobusangSideEffectKind::SetChannelState);
    EXPECT_EQ(plan.effects[0].channel_id, kChannel);
    EXPECT_EQ(plan.effects[0].state, BobusangChannelState::Appear);
    EXPECT_EQ(plan.effects[0].reply_protocol, bobusang_appear_map_to_agent);
    EXPECT_EQ(plan.effects[0].dw_object_id, kObjectId);
}

TEST(BobusangPlan, ServerDisappearMapToAgentSetsChannelStateDisappear) {
    BobusangServerRequest r;
    r.protocol = bobusang_disappear_map_to_agent;
    r.dword.dw_data = kChannel;
    auto a = classify_bobusang_server(r);
    const auto plan = bobusang_server_side_effect_plan(a);
    EXPECT_TRUE(plan.set_channel_state);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].state, BobusangChannelState::Disappear);
    EXPECT_EQ(plan.effects[0].channel_id, kChannel);
}

TEST(BobusangPlan, ServerInfoAgentToMapForwardsToOriginatingClient) {
    BobusangServerRequest r;
    r.protocol = bobusang_info_agent_to_map;
    r.dword.dw_object_id = kObjectId;
    auto a = classify_bobusang_server(r);
    EXPECT_EQ(a.kind, BobusangServerActionKind::forward_to_originating_client);
    const auto plan = bobusang_server_side_effect_plan(a);
    EXPECT_TRUE(plan.dispatched);
    EXPECT_TRUE(plan.forward_to_originating_client);
    EXPECT_FALSE(plan.set_channel_state);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, BobusangSideEffectKind::ForwardToOriginatingClient);
    EXPECT_EQ(plan.effects[0].reply_protocol, bobusang_info_agent_to_map);
    EXPECT_EQ(plan.effects[0].dw_object_id, kObjectId);
}

TEST(BobusangPlan, ServerGuestLeaveAckForwardsToOriginatingClient) {
    BobusangServerRequest r;
    r.protocol = bobusang_leave_guest_ack;
    auto a = classify_bobusang_server(r);
    EXPECT_EQ(a.kind, BobusangServerActionKind::forward_to_originating_client);
    const auto plan = bobusang_server_side_effect_plan(a);
    EXPECT_TRUE(plan.forward_to_originating_client);
    EXPECT_EQ(plan.effects[0].reply_protocol, bobusang_leave_guest_ack);
}

TEST(BobusangPlan, ServerDealItemInfoForwardsToOriginatingClient) {
    BobusangServerRequest r;
    r.protocol = bobusang_dealiteminfo_to_guest;
    auto a = classify_bobusang_server(r);
    const auto plan = bobusang_server_side_effect_plan(a);
    EXPECT_TRUE(plan.forward_to_originating_client);
    EXPECT_EQ(plan.effects[0].reply_protocol, bobusang_dealiteminfo_to_guest);
}

TEST(BobusangPlan, ServerNotifyForDisappearanceForwardsToOriginatingClient) {
    BobusangServerRequest r;
    r.protocol = bobusang_notify_for_disappearance;
    auto a = classify_bobusang_server(r);
    const auto plan = bobusang_server_side_effect_plan(a);
    EXPECT_TRUE(plan.forward_to_originating_client);
    EXPECT_EQ(plan.effects[0].reply_protocol, bobusang_notify_for_disappearance);
}

TEST(BobusangPlan, ServerUnknownProtocolEmitsDrop) {
    BobusangServerRequest r;
    r.protocol = 200u;
    auto a = classify_bobusang_server(r);
    EXPECT_EQ(a.kind, BobusangServerActionKind::drop_unknown_protocol);
    const auto plan = bobusang_server_side_effect_plan(a);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.dispatched);
    ASSERT_EQ(plan.effects.size(), 1u);
    EXPECT_EQ(plan.effects[0].kind, BobusangSideEffectKind::Drop);
    EXPECT_EQ(plan.effects[0].reply_protocol, 200u);
    EXPECT_FALSE(plan.effects[0].forward_payload);
}

TEST(BobusangPlan, PlanDefaultsAreConservative) {
    BobusangSideEffectPlan plan;
    EXPECT_FALSE(plan.dispatched);
    EXPECT_TRUE(plan.drop);
    EXPECT_FALSE(plan.forward_to_map_server);
    EXPECT_FALSE(plan.set_channel_state);
    EXPECT_FALSE(plan.forward_to_originating_client);
    EXPECT_TRUE(plan.effects.empty());
}

