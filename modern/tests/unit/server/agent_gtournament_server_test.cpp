// agent_gtournament_server_test.cpp - Phase 6.3 AgentGTournament 1:1 port tests.

#include "mxh/server/agent_gtournament_server.hpp"

#include <gtest/gtest.h>

namespace {

using mxh::server::classify_agent_gtournament_user;
using mxh::server::classify_agent_gtournament_server;
using mxh::server::GTournamentActionKind;
using mxh::server::GTournamentUserRequest;
using mxh::server::GTournamentServerRequest;

GTournamentUserRequest base_user() {
    GTournamentUserRequest r{};
    r.protocol = mxh::server::gtournament_movetobattlemap_syn;
    r.gt_map_port = 9060u;
    r.target_map_port = 9017u;
    r.user_level = mxh::server::gtournament_user_level_max_for_event_start;
    return r;
}

TEST(GTournamentCategory, CategoryByteIs60) {
    EXPECT_EQ(mxh::server::gtoournament_category, 60u);
}

TEST(GTournamentConstants, MapNumIs60) {
    EXPECT_EQ(mxh::server::gtournament_map_num, 60u);
}

TEST(GTournamentUserMoveToBattle, UnknownCharDropped) {
    auto r = base_user();
    r.user_known_by_charid = false;
    auto a = classify_agent_gtournament_user(r);
    EXPECT_EQ(a.kind, GTournamentActionKind::drop_no_user_by_charid);
}

TEST(GTournamentUserMoveToBattle, NoPortNacks) {
    auto r = base_user();
    r.gt_map_port = 0u;
    auto a = classify_agent_gtournament_user(r);
    EXPECT_EQ(a.kind, GTournamentActionKind::send_movetobattlemap_nack_to_user);
    EXPECT_EQ(a.reply_protocol, mxh::server::gtournament_movetobattlemap_nack);
}

TEST(GTournamentUserMoveToBattle, PortKnownForwards) {
    auto r = base_user();
    auto a = classify_agent_gtournament_user(r);
    EXPECT_EQ(a.kind, GTournamentActionKind::forward_to_tournament_map_server);
    EXPECT_EQ(a.reply_protocol, mxh::server::gtournament_movetobattlemap_syn);
}

TEST(GTournamentUserStandingInfo, NoPortNacks) {
    auto r = base_user();
    r.protocol = mxh::server::gtournament_standinginfo_syn;
    r.gt_map_port = 0u;
    auto a = classify_agent_gtournament_user(r);
    EXPECT_EQ(a.kind, GTournamentActionKind::send_standinginfo_nack_to_user);
}

TEST(GTournamentUserBattleJoin, NoPortNacks) {
    auto r = base_user();
    r.protocol = mxh::server::gtournament_battlejoin_syn;
    r.gt_map_port = 0u;
    auto a = classify_agent_gtournament_user(r);
    EXPECT_EQ(a.kind, GTournamentActionKind::send_battlejoin_nack_to_user);
}

TEST(GTournamentUserObserverJoin, PortKnownForwards) {
    auto r = base_user();
    r.protocol = mxh::server::gtournament_observerjoin_syn;
    auto a = classify_agent_gtournament_user(r);
    EXPECT_EQ(a.kind, GTournamentActionKind::forward_to_tournament_map_server);
    EXPECT_EQ(a.reply_protocol, mxh::server::gtournament_observerjoin_syn);
}

TEST(GTournamentUserLeave, ForwardsToUserMapServer) {
    auto r = base_user();
    r.protocol = mxh::server::gtournament_leave_syn;
    auto a = classify_agent_gtournament_user(r);
    EXPECT_EQ(a.kind, GTournamentActionKind::forward_to_user_map_server);
    EXPECT_EQ(a.reply_protocol, mxh::server::gtournament_leave_syn);
}

TEST(GTournamentUserCheatTargetMap, ForwardsUserMapServer) {
    auto r = base_user();
    r.protocol = mxh::server::gtournament_cheat;
    r.target_map_port = 8001u;
    auto a = classify_agent_gtournament_user(r);
    EXPECT_EQ(a.kind, GTournamentActionKind::forward_to_user_map_server);
}

TEST(GTournamentUserCheatNoTargetMap, ForwardsGTMap) {
    auto r = base_user();
    r.protocol = mxh::server::gtournament_cheat;
    r.target_map_port = 0u;
    r.gt_map_port = 9060u;
    auto a = classify_agent_gtournament_user(r);
    EXPECT_EQ(a.kind, GTournamentActionKind::forward_to_tournament_map_server);
}

TEST(GTournamentUserCheatNoPortAnywhere, Drops) {
    auto r = base_user();
    r.protocol = mxh::server::gtournament_cheat;
    r.target_map_port = 0u;
    r.gt_map_port = 0u;
    auto a = classify_agent_gtournament_user(r);
    EXPECT_EQ(a.kind, GTournamentActionKind::drop_no_user_by_charid);
}

TEST(GTournamentUserEventStartNonGm, Drops) {
    auto r = base_user();
    r.protocol = mxh::server::gtournament_event_start;
    r.user_level = 99u;
    auto a = classify_agent_gtournament_user(r);
    EXPECT_EQ(a.kind, GTournamentActionKind::drop_event_start_not_gm);
}

TEST(GTournamentUserEventStartGmNoConn, Drops) {
    auto r = base_user();
    r.protocol = mxh::server::gtournament_event_start;
    r.user_known_by_conn = false;
    auto a = classify_agent_gtournament_user(r);
    EXPECT_EQ(a.kind, GTournamentActionKind::drop_no_user_by_conn);
}

TEST(GTournamentUserEventStartGmPort, Forwards) {
    auto r = base_user();
    r.protocol = mxh::server::gtournament_event_start;
    auto a = classify_agent_gtournament_user(r);
    EXPECT_EQ(a.kind, GTournamentActionKind::forward_to_tournament_map_server);
    EXPECT_EQ(a.reply_protocol, mxh::server::gtournament_event_start);
}

TEST(GTournamentUserEventEndGm, Forwards) {
    auto r = base_user();
    r.protocol = mxh::server::gtournament_event_end;
    auto a = classify_agent_gtournament_user(r);
    EXPECT_EQ(a.kind, GTournamentActionKind::forward_to_tournament_map_server);
}

TEST(GTournamentUserUnknown, DefaultsToForward) {
    auto r = base_user();
    r.protocol = 200u;
    auto a = classify_agent_gtournament_user(r);
    EXPECT_EQ(a.kind, GTournamentActionKind::default_to_trans_to_user);
}

TEST(GTournamentServerCheatUnknownTarget, Drops) {
    GTournamentServerRequest r{};
    r.protocol = mxh::server::gtournament_cheat;
    r.target_user_found = false;
    auto a = classify_agent_gtournament_server(r);
    EXPECT_EQ(a.kind, GTournamentActionKind::drop_no_user_by_charid);
}

TEST(GTournamentServerCheatKnownTarget, SendsToUser) {
    GTournamentServerRequest r{};
    r.protocol = mxh::server::gtournament_cheat;
    auto a = classify_agent_gtournament_server(r);
    EXPECT_EQ(a.kind, GTournamentActionKind::send_to_user);
    EXPECT_EQ(a.reply_protocol, mxh::server::gtournament_cheat);
}

TEST(GTournamentServerStandingInfoRegistedKnownTarget, SendsToUser) {
    GTournamentServerRequest r{};
    r.protocol = mxh::server::gtournament_standinginfo_registed;
    auto a = classify_agent_gtournament_server(r);
    EXPECT_EQ(a.kind, GTournamentActionKind::send_to_user);
}

TEST(GTournamentServerReturnToMapKnownPort, SetsMapStateForwards) {
    GTournamentServerRequest r{};
    r.protocol = mxh::server::gtournament_returntomap;
    r.return_port_known = true;
    auto a = classify_agent_gtournament_server(r);
    EXPECT_EQ(a.kind, GTournamentActionKind::set_user_map_state_and_forward);
    EXPECT_TRUE(a.update_map_state);
}

TEST(GTournamentServerReturnToMapUnknownPort, SendsToUser) {
    GTournamentServerRequest r{};
    r.protocol = mxh::server::gtournament_returntomap;
    r.return_port_known = false;
    auto a = classify_agent_gtournament_server(r);
    EXPECT_EQ(a.kind, GTournamentActionKind::send_to_user);
}

TEST(GTournamentServerNotifyWinLoseKnown, SendsToUser) {
    GTournamentServerRequest r{};
    r.protocol = mxh::server::gtournament_notify_winlose;
    auto a = classify_agent_gtournament_server(r);
    EXPECT_EQ(a.kind, GTournamentActionKind::send_to_user);
    EXPECT_EQ(a.reply_protocol, mxh::server::gtournament_notify_winlose);
}

}  // namespace