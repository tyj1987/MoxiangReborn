// agent_powerup_test.cpp - Phase 6.3 AgentPowerUp 1:1 port tests.

#include "mxh/server/agent_powerup.hpp"

#include <gtest/gtest.h>

namespace {

using mxh::server::classify_powerup;
using mxh::server::classify_powerup_self_init;
using mxh::server::classify_powerup_server_kind_dispatch;
using mxh::server::classify_powerup_registmap_ack;
using mxh::server::PowerUpActionKind;
using mxh::server::PowerUpRequest;

PowerUpRequest base() {
    PowerUpRequest r{};
    r.protocol = mxh::server::powerup_booting_notify;
    r.server_num = 0u;
    r.self_port = 9001u;
    r.target_port = 8000u;
    return r;
}

TEST(PowerUpCategory, CategoryByteIsTwo) {
    EXPECT_EQ(mxh::server::powerup_category, 2u);
}

TEST(PowerUpProtocol, AllFiveSubProtocolsMap) {
    EXPECT_EQ(mxh::server::powerup_booting_notify, 0u);
    EXPECT_EQ(mxh::server::powerup_bootlist_syn, 1u);
    EXPECT_EQ(mxh::server::powerup_bootlist_ack, 2u);
    EXPECT_EQ(mxh::server::powerup_connect_syn, 3u);
    EXPECT_EQ(mxh::server::powerup_connect_ack, 4u);
}

TEST(PowerUpBooting, ForwardsToBootManager) {
    auto a = classify_powerup(base());
    EXPECT_EQ(a.kind, PowerUpActionKind::forward_to_boot_manager);
    EXPECT_TRUE(a.forward_payload);
}

TEST(PowerUpBootListSyn, ForwardsToBootManager) {
    auto r = base();
    r.protocol = mxh::server::powerup_bootlist_syn;
    auto a = classify_powerup(r);
    EXPECT_EQ(a.kind, PowerUpActionKind::forward_to_boot_manager);
}

TEST(PowerUpBootListAck, ForwardsToBootManager) {
    auto r = base();
    r.protocol = mxh::server::powerup_bootlist_ack;
    auto a = classify_powerup(r);
    EXPECT_EQ(a.kind, PowerUpActionKind::forward_to_boot_manager);
}

TEST(PowerUpConnectSyn, MsUnreachableFailsServerStart) {
    auto r = base();
    r.protocol = mxh::server::powerup_connect_syn;
    r.ms_reachable = false;
    auto a = classify_powerup(r);
    EXPECT_EQ(a.kind, PowerUpActionKind::drop_unreachable_ms);
}

TEST(PowerUpConnectSyn, MsReachableForwards) {
    auto r = base();
    r.protocol = mxh::server::powerup_connect_syn;
    auto a = classify_powerup(r);
    EXPECT_EQ(a.kind, PowerUpActionKind::forward_to_boot_manager);
}

TEST(PowerUpConnectAck, ForwardsToBootManager) {
    auto r = base();
    r.protocol = mxh::server::powerup_connect_ack;
    auto a = classify_powerup(r);
    EXPECT_EQ(a.kind, PowerUpActionKind::forward_to_boot_manager);
}

TEST(PowerUpUnknownProto, DefaultsToForward) {
    auto r = base();
    r.protocol = 99u;
    auto a = classify_powerup(r);
    EXPECT_EQ(a.kind, PowerUpActionKind::forward_to_boot_manager);
}

TEST(PowerUpSelfInit, AgentKindAddsSelfToBootList) {
    auto r = base();
    r.server_num = 0u;
    auto a = classify_powerup_self_init(r);
    EXPECT_EQ(a.kind, PowerUpActionKind::add_self_boot_list);
    EXPECT_EQ(a.target_port, 9001u);
}

TEST(PowerUpSelfInit, MsUnreachableReturnsDrop) {
    auto r = base();
    r.ms_reachable = false;
    auto a = classify_powerup_self_init(r);
    EXPECT_EQ(a.kind, PowerUpActionKind::drop_unreachable_ms);
}

TEST(PowerUpServerKindDispatch, AgentReturnsRegistAck) {
    auto r = base();
    r.is_agent_kind = true;
    auto a = classify_powerup_server_kind_dispatch(r);
    EXPECT_EQ(a.kind, PowerUpActionKind::send_registmap_ack_to_ms);
    EXPECT_EQ(a.reply_protocol, 1u); // MP_SERVER_REGISTMAP_ACK
    EXPECT_EQ(a.target_port, 9001u);
}

TEST(PowerUpServerKindDispatch, MonitorReturnsRegistAck) {
    auto r = base();
    r.is_monitor_kind = true;
    auto a = classify_powerup_server_kind_dispatch(r);
    EXPECT_EQ(a.kind, PowerUpActionKind::send_registmap_ack_to_ms);
}

TEST(PowerUpServerKindDispatch, DistributeSendsUserCount) {
    auto r = base();
    r.is_distribute_kind = true;
    r.agent_user_count = 4242u;
    auto a = classify_powerup_server_kind_dispatch(r);
    EXPECT_EQ(a.kind, PowerUpActionKind::send_user_count_to_distribute);
    EXPECT_EQ(a.user_count, 4242u);
    EXPECT_EQ(a.reply_protocol, 6u);
}

TEST(PowerUpServerKindDispatch, MapSendsRegistSyn) {
    auto r = base();
    r.is_map_kind = true;
    r.target_port = 8017u;
    auto a = classify_powerup_server_kind_dispatch(r);
    EXPECT_EQ(a.kind, PowerUpActionKind::send_registmap_syn_to_map);
    EXPECT_EQ(a.target_port, 8017u);
}

TEST(PowerUpServerKindDispatch, UnknownServerKindDrops) {
    auto r = base();
    auto a = classify_powerup_server_kind_dispatch(r);
    EXPECT_EQ(a.kind, PowerUpActionKind::drop_unknown_server_kind);
}

TEST(PowerUpServerKindDispatch, TargetNotFoundDrops) {
    auto r = base();
    r.is_agent_kind = true;
    r.target_server_found = false;
    auto a = classify_powerup_server_kind_dispatch(r);
    EXPECT_EQ(a.kind, PowerUpActionKind::drop_unknown_server_kind);
}

TEST(PowerUpRegistMapAck, MapNumZeroNoop) {
    auto r = base();
    /* r.map_num */
    auto a = classify_powerup_registmap_ack(r, 0u);
    EXPECT_EQ(a.kind, PowerUpActionKind::forward_to_boot_manager);
}

TEST(PowerUpRegistMapAck, MapNumNonZeroInRangeTriggersSetMapRegist) {
    auto r = base();
    r.self_port = 8500u; // in MAPSERVER_PORT..MAXSERVER_PORT
    /* map_num 7 */
    auto a = classify_powerup_registmap_ack(r, 7u);
    EXPECT_EQ(a.kind, PowerUpActionKind::map_user_unregist_login);
    EXPECT_EQ(a.map_num, 7u);
}

TEST(PowerUpRegistMapAck, MapNumNonZeroBelowRangeTriggersSetMapRegist) {
    auto r = base();
    r.self_port = 7000u;
    /* map_num 5 */
    auto a = classify_powerup_registmap_ack(r, 5u);
    EXPECT_EQ(a.kind, PowerUpActionKind::set_map_regist);
    EXPECT_EQ(a.map_num, 5u);
}

TEST(PowerUpMaxAgent, CapsBelowThreshold) {
    EXPECT_GE(mxh::server::powerup_max_agent_servers, 50u);
}

}  // namespace