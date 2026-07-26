
#include "mxh/server/agent_murimnet.hpp"
#include <gtest/gtest.h>
using namespace mxh::server;
TEST(MurimNetUser, ChangeToMurimNetNoPortSendsNack){MurimNetUserRequest r;r.protocol=murimnet_changetomurimnet_syn;auto a=classify_murimnet_user(r);EXPECT_EQ(a.kind,MurimNetActionKind::send_nack);EXPECT_EQ(a.protocol,murimnet_changetomurimnet_nack);}
TEST(MurimNetUser, ChangeToMurimNetWithPortForwards){MurimNetUserRequest r;r.protocol=murimnet_changetomurimnet_syn;r.lookup.port=1234;r.lookup.connection_index=99;auto a=classify_murimnet_user(r);EXPECT_EQ(a.kind,MurimNetActionKind::forward_to_map);EXPECT_EQ(a.protocol,murimnet_changetomurimnet_syn);}
TEST(MurimNetUser, ConnectAndReconnectForward){MurimNetUserRequest r;r.protocol=murimnet_connect_syn;EXPECT_EQ(classify_murimnet_user(r).kind,MurimNetActionKind::forward_to_map);r.protocol=murimnet_reconnect_syn;EXPECT_EQ(classify_murimnet_user(r).kind,MurimNetActionKind::forward_to_map);}
TEST(MurimNetServer, AckNoPortSendsNack){MurimNetServerRequest r;r.protocol=murimnet_changetomurimnet_ack;auto a=classify_murimnet_server(r);EXPECT_EQ(a.kind,MurimNetActionKind::send_nack);EXPECT_EQ(a.protocol,murimnet_changetomurimnet_nack);}
TEST(MurimNetServer, ReturnAckWithPortSendsAck){MurimNetServerRequest r;r.protocol=murimnet_returntomurimnet_ack;r.lookup.port=9000;auto a=classify_murimnet_server(r);EXPECT_EQ(a.kind,MurimNetActionKind::send_ack);EXPECT_EQ(a.protocol,murimnet_returntomurimnet_ack);}
TEST(MurimNetServer, PrStartForwards){MurimNetServerRequest r;r.protocol=murimnet_pr_start;EXPECT_EQ(classify_murimnet_server(r).kind,MurimNetActionKind::forward_to_map);}
