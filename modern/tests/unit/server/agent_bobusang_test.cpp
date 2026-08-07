//
// 1:1 port of MP_BOBUSANGUserMsgParser / MP_BOBUSANGServerMsgParser from
// legacy [Server]Agent/AgentNetworkMsgParser.cpp lines 5191-5234.

#include <mxh/server/agent_bobusang.hpp>

#include <gtest/gtest.h>

#include <array>
#include <set>

using namespace mxh::server;

namespace {
constexpr std::uint32_t kObjectId = 0xBABEFACEu;
constexpr std::uint32_t kData = 0xFEEDF00Du;
constexpr std::uint32_t kChannel = 7u;
}

TEST(AgentBobusangClassify, CategoryConstantMatchesProtocolHeader) {
    EXPECT_EQ(bobusang_category, 74u);
}

TEST(AgentBobusangClassify, ProtocolConstantsCoverZeroToTwelve) {
    EXPECT_EQ(bobusang_info_agent_to_map, 0u);
    EXPECT_EQ(bobusang_disappear_agent_to_map, 1u);
    EXPECT_EQ(bobusang_appear_map_to_agent, 2u);
    EXPECT_EQ(bobusang_disappear_map_to_agent, 3u);
    EXPECT_EQ(bobusang_add_guest_syn, 4u);
    EXPECT_EQ(bobusang_add_guest_ack, 5u);
    EXPECT_EQ(bobusang_add_guest_nack, 6u);
    EXPECT_EQ(bobusang_leave_guest_syn, 7u);
    EXPECT_EQ(bobusang_leave_guest_ack, 8u);
    EXPECT_EQ(bobusang_leave_guest_nack, 9u);
    EXPECT_EQ(bobusang_all_dealiteminfo_to_guest, 10u);
    EXPECT_EQ(bobusang_dealiteminfo_to_guest, 11u);
    EXPECT_EQ(bobusang_notify_for_disappearance, 12u);
}

TEST(AgentBobusangClassify, GmPowerSentinelsAreOrdered) {
    EXPECT_LT(bobusang_gm_power_master, bobusang_gm_power_max);
    EXPECT_EQ(bobusang_user_level_gm, 9u);
}

TEST(AgentBobusangUserClassify, UserNotFoundDrops) {
    BobusangUserRequest r;
    r.protocol = bobusang_add_guest_syn;
    r.dw_object_id = kObjectId;
    r.user_found = false;
    auto a = classify_bobusang_user(r);
    EXPECT_EQ(a.kind, BobusangUserActionKind::drop_no_user);
    EXPECT_EQ(a.reply_protocol, bobusang_add_guest_syn);
}

TEST(AgentBobusangUserClassify, NonGmUserForwardsToMap) {
    BobusangUserRequest r;
    r.protocol = bobusang_add_guest_syn;
    r.user_found = true;
    r.user_is_gm = false;
    auto a = classify_bobusang_user(r);
    EXPECT_EQ(a.kind, BobusangUserActionKind::forward_to_map_server);
    EXPECT_TRUE(a.forward_payload);
}

TEST(AgentBobusangUserClassify, GmMasterForwardsToMap) {
    // legacy: GM_POWER > MASTER is blocked. ==MASTER still forwards.
    BobusangUserRequest r;
    r.protocol = bobusang_add_guest_syn;
    r.user_found = true;
    r.user_is_gm = true;
    r.gm_power = bobusang_gm_power_master;  // ==MASTER -> forward
    auto a = classify_bobusang_user(r);
    EXPECT_EQ(a.kind, BobusangUserActionKind::forward_to_map_server);
}

TEST(AgentBobusangUserClassify, GmAboveMasterDrops) {
    BobusangUserRequest r;
    r.protocol = bobusang_add_guest_syn;
    r.user_found = true;
    r.user_is_gm = true;
    r.gm_power = bobusang_gm_power_master + 1u;
    auto a = classify_bobusang_user(r);
    EXPECT_EQ(a.kind, BobusangUserActionKind::drop_gm_overshoot);
}

TEST(AgentBobusangUserClassify, GmMaxDrops) {
    BobusangUserRequest r;
    r.protocol = bobusang_add_guest_syn;
    r.user_found = true;
    r.user_is_gm = true;
    r.gm_power = bobusang_gm_power_max;
    auto a = classify_bobusang_user(r);
    EXPECT_EQ(a.kind, BobusangUserActionKind::drop_gm_overshoot);
}

TEST(AgentBobusangUserClassify, EchoesProtocolOnDrop) {
    BobusangUserRequest r;
    r.protocol = 99u;
    r.user_found = false;
    auto a = classify_bobusang_user(r);
    EXPECT_EQ(a.kind, BobusangUserActionKind::drop_no_user);
    EXPECT_EQ(a.reply_protocol, 99u);
}

TEST(AgentBobusangServerClassify, AppearMapToAgentSetsAppear) {
    BobusangServerRequest r;
    r.protocol = bobusang_appear_map_to_agent;
    r.dword.dw_object_id = kObjectId;
    r.dword.dw_data = kChannel;
    auto a = classify_bobusang_server(r);
    EXPECT_EQ(a.kind, BobusangServerActionKind::set_channel_state);
    EXPECT_EQ(a.reply_protocol, bobusang_appear_map_to_agent);
    EXPECT_EQ(a.channel_id, kChannel);
    EXPECT_EQ(a.state, BobusangChannelState::Appear);
}

TEST(AgentBobusangServerClassify, DisappearMapToAgentSetsDisappear) {
    BobusangServerRequest r;
    r.protocol = bobusang_disappear_map_to_agent;
    r.dword.dw_data = kChannel;
    auto a = classify_bobusang_server(r);
    EXPECT_EQ(a.kind, BobusangServerActionKind::set_channel_state);
    EXPECT_EQ(a.state, BobusangChannelState::Disappear);
    EXPECT_EQ(a.channel_id, kChannel);
}

TEST(AgentBobusangServerClassify, DisappearMapToAgentEchoesObjectId) {
    BobusangServerRequest r;
    r.protocol = bobusang_disappear_map_to_agent;
    r.dword.dw_object_id = kObjectId;
    auto a = classify_bobusang_server(r);
    EXPECT_EQ(a.dw_object_id, kObjectId);
}

TEST(AgentBobusangServerClassify, AllDefaultProtocolsForwardToClient) {
    for (std::uint8_t p : {bobusang_info_agent_to_map, bobusang_disappear_agent_to_map,
                         bobusang_add_guest_syn, bobusang_add_guest_ack,
                         bobusang_add_guest_nack, bobusang_leave_guest_syn,
                         bobusang_leave_guest_ack, bobusang_leave_guest_nack,
                         bobusang_all_dealiteminfo_to_guest,
                         bobusang_dealiteminfo_to_guest,
                         bobusang_notify_for_disappearance}) {
        BobusangServerRequest r;
        r.protocol = p;
        auto a = classify_bobusang_server(r);
        EXPECT_EQ(a.kind, BobusangServerActionKind::forward_to_originating_client);
        EXPECT_EQ(a.reply_protocol, p);
    }
}

TEST(AgentBobusangServerClassify, UnknownProtocolDrops) {
    BobusangServerRequest r;
    r.protocol = 200u;
    auto a = classify_bobusang_server(r);
    EXPECT_EQ(a.kind, BobusangServerActionKind::drop_unknown_protocol);
    EXPECT_EQ(a.reply_protocol, 200u);
    EXPECT_EQ(a.channel_id, 0u);
}

TEST(AgentBobusangClassify, RequestAndActionDefaultsAreStable) {
    BobusangUserRequest ur;
    EXPECT_EQ(ur.protocol, 0u);
    EXPECT_EQ(ur.dw_object_id, 0u);
    EXPECT_TRUE(ur.user_found);
    EXPECT_FALSE(ur.user_is_gm);
    EXPECT_EQ(ur.gm_power, 0u);
    BobusangServerRequest sr;
    EXPECT_EQ(sr.protocol, 0u);
    EXPECT_EQ(sr.dword.dw_data, 0u);
    BobusangUserAction ua;
    EXPECT_EQ(ua.kind, BobusangUserActionKind::drop_no_user);
    EXPECT_EQ(ua.reply_protocol, 0u);
    BobusangServerAction sa;
    EXPECT_EQ(sa.kind, BobusangServerActionKind::drop_unknown_protocol);
    EXPECT_EQ(sa.channel_id, 0u);
    EXPECT_EQ(sa.state, BobusangChannelState::Appear);
}

TEST(AgentBobusangServerClassify, AppearZeroChannelStillSetsState) {
    BobusangServerRequest r;
    r.protocol = bobusang_appear_map_to_agent;
    r.dword.dw_data = 0u;
    auto a = classify_bobusang_server(r);
    EXPECT_EQ(a.kind, BobusangServerActionKind::set_channel_state);
    EXPECT_EQ(a.channel_id, 0u);
    EXPECT_EQ(a.state, BobusangChannelState::Appear);
}
