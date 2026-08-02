
#include "mxh/server/agent_murimnet.hpp"
#include <gtest/gtest.h>
using namespace mxh::server;
TEST(MurimNetUser, ChangeToMurimNetNoPortSendsNack){MurimNetUserRequest r;r.protocol=murimnet_changetomurimnet_syn;auto a=classify_murimnet_user(r);EXPECT_EQ(a.kind,MurimNetActionKind::send_nack);EXPECT_EQ(a.protocol,murimnet_changetomurimnet_nack);}
TEST(MurimNetUser, ChangeToMurimNetWithPortForwards){MurimNetUserRequest r;r.protocol=murimnet_changetomurimnet_syn;r.lookup.port=1234;r.lookup.connection_index=99;auto a=classify_murimnet_user(r);EXPECT_EQ(a.kind,MurimNetActionKind::forward_to_map);EXPECT_EQ(a.protocol,murimnet_changetomurimnet_syn);}
TEST(MurimNetUser, ConnectAndReconnectForward){MurimNetUserRequest r;r.protocol=murimnet_connect_syn;EXPECT_EQ(classify_murimnet_user(r).kind,MurimNetActionKind::forward_to_map);r.protocol=murimnet_reconnect_syn;EXPECT_EQ(classify_murimnet_user(r).kind,MurimNetActionKind::forward_to_map);}
TEST(MurimNetServer, AckNoPortSendsNack){MurimNetServerRequest r;r.protocol=murimnet_changetomurimnet_ack;auto a=classify_murimnet_server(r);EXPECT_EQ(a.kind,MurimNetActionKind::send_nack);EXPECT_EQ(a.protocol,murimnet_changetomurimnet_nack);}
TEST(MurimNetServer, ReturnAckWithPortSendsAck){MurimNetServerRequest r;r.protocol=murimnet_returntomurimnet_ack;r.lookup.port=9000;auto a=classify_murimnet_server(r);EXPECT_EQ(a.kind,MurimNetActionKind::send_ack);EXPECT_EQ(a.protocol,murimnet_returntomurimnet_ack);}
// ---- D5.1 extension: forward user-side runtime commands instead of dropping.
// Legacy MNNetworkMsgParser forwards pr_teamchange_syn (21), chnl_modechange
// (32), chat_all (48), and notifytomn_player_logout (58) to the MapServer's
// MurimNetRuntime. Previously these fell into the default drop_unknown branch
// of classify_murimnet_user, so the runtime never saw them.

TEST(MurimNetUser, PrTeamChangeSynIsForwarded) {
    MurimNetUserRequest r;
    r.protocol = murimnet_pr_teamchange_syn;
    r.character_id = 555;
    r.lookup.port = 4011;
    auto a = classify_murimnet_user(r);
    EXPECT_EQ(a.kind, MurimNetActionKind::forward_to_map);
    EXPECT_EQ(a.protocol, murimnet_pr_teamchange_syn);
    EXPECT_EQ(a.character_id, 555u);
}

TEST(MurimNetUser, ChnlModeChangeIsForwarded) {
    MurimNetUserRequest r;
    r.protocol = murimnet_chnl_modechange;
    r.character_id = 556;
    auto a = classify_murimnet_user(r);
    EXPECT_EQ(a.kind, MurimNetActionKind::forward_to_map);
    EXPECT_EQ(a.protocol, murimnet_chnl_modechange);
}

TEST(MurimNetUser, ChatAllIsForwarded) {
    MurimNetUserRequest r;
    r.protocol = murimnet_chat_all;
    r.character_id = 557;
    auto a = classify_murimnet_user(r);
    EXPECT_EQ(a.kind, MurimNetActionKind::forward_to_map);
    EXPECT_EQ(a.protocol, murimnet_chat_all);
}

TEST(MurimNetUser, NotifyToMnPlayerLogoutIsForwarded) {
    MurimNetUserRequest r;
    r.protocol = murimnet_notifytomn_player_logout;
    r.character_id = 558;
    auto a = classify_murimnet_user(r);
    EXPECT_EQ(a.kind, MurimNetActionKind::forward_to_map);
    EXPECT_EQ(a.protocol, murimnet_notifytomn_player_logout);
}

// Server-side: Pr_TeamChange_Ack (22), Pr_TeamChange_Nack (23), Pr_Start_Ack
// (26), Pr_Start_Nack (27), Chat_All, NotifyToMn_GameEnd (59) are forwarded.
TEST(MurimNetServer, PrStartAckIsForwarded) {
    MurimNetServerRequest r;
    r.protocol = murimnet_pr_start_ack;
    r.character_id = 700;
    auto a = classify_murimnet_server(r);
    EXPECT_EQ(a.kind, MurimNetActionKind::forward_to_map);
    EXPECT_EQ(a.protocol, murimnet_pr_start_ack);
}

TEST(MurimNetServer, PrStartNackIsForwarded) {
    MurimNetServerRequest r;
    r.protocol = static_cast<std::uint8_t>(MurimNetProtocol::Pr_Start_Nack);
    r.character_id = 701;
    auto a = classify_murimnet_server(r);
    EXPECT_EQ(a.kind, MurimNetActionKind::forward_to_map);
    EXPECT_EQ(a.protocol, static_cast<std::uint8_t>(MurimNetProtocol::Pr_Start_Nack));
}

TEST(MurimNetServer, NotifyToMnGameEndIsForwarded) {
    MurimNetServerRequest r;
    r.protocol = murimnet_notifytomn_gameend;
    r.character_id = 702;
    auto a = classify_murimnet_server(r);
    EXPECT_EQ(a.kind, MurimNetActionKind::forward_to_map);
    EXPECT_EQ(a.protocol, murimnet_notifytomn_gameend);
}


TEST(MurimNetServer, PrStartForwards){MurimNetServerRequest r;r.protocol=murimnet_pr_start;EXPECT_EQ(classify_murimnet_server(r).kind,MurimNetActionKind::forward_to_map);}

TEST(MurimNetUser, UnknownProtocolIsDropped){MurimNetUserRequest r;r.protocol=0xff;auto a=classify_murimnet_user(r);EXPECT_EQ(a.kind,MurimNetActionKind::drop_unknown);EXPECT_EQ(a.protocol,0xff);}
TEST(MurimNetServer, UnknownProtocolIsDropped){MurimNetServerRequest r;r.protocol=0xfe;auto a=classify_murimnet_server(r);EXPECT_EQ(a.kind,MurimNetActionKind::drop_unknown);EXPECT_EQ(a.protocol,0xfe);}
