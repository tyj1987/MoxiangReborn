#include "mxh/server/traffic_log.hpp"
#include <gtest/gtest.h>
using namespace mxh::server;
TEST(TrafficLog, PacketBytesIncludeFortyByteHeaders){TrafficLog t;traffic_add_receive(t,3,10);traffic_add_send(t,3,20);EXPECT_EQ(t.receive_size[3],50u);EXPECT_EQ(t.send_size[3],60u);EXPECT_EQ(t.receive_num[3],1u);}
TEST(TrafficLog, InvalidCategoryDoesNotIndex){TrafficLog t;traffic_add_receive(t,traffic_mp_max,10);EXPECT_EQ(t.receive_num[0],0u);}
TEST(TrafficLog, MoveBucketsMatchLegacy){TrafficLog t;traffic_add_move_receive(t,1999999,1,5);traffic_add_move_receive(t,2000000,2,6);traffic_add_move_send(t,9,3,7);EXPECT_EQ(t.move_receive_size[0][0],5u);EXPECT_EQ(t.move_receive_size[1][1],6u);EXPECT_EQ(t.move_send_size[0][2],7u);}
TEST(TrafficLog, UnknownMoveProtocolUsesFourthBucket){TrafficLog t;traffic_add_move_receive(t,1,99,8);EXPECT_EQ(t.move_receive_size[0][3],8u);}
TEST(TrafficLog, UserLifecycleAndCounts){TrafficLog t;traffic_add_user(t,7,8);EXPECT_TRUE(t.users[7].login);traffic_add_user_packet(t,7,1);traffic_add_user_packet(t,7,0);EXPECT_EQ(t.users[7].valued,1u);EXPECT_EQ(t.users[7].unvalued,1u);traffic_remove_user(t,7,100);EXPECT_FALSE(t.users[7].login);EXPECT_EQ(t.users[7].login_time,100u);}
TEST(TrafficLog, ThresholdReturnsDisconnectAction){TrafficLog t;t.unvalued_limit=2;traffic_add_user(t,7,8);EXPECT_EQ(traffic_add_user_packet(t,7,0),TrafficAction::none);EXPECT_EQ(traffic_add_user_packet(t,7,0),TrafficAction::disconnect);}
TEST(TrafficLog, ClearResetsAllCounters){TrafficLog t;traffic_add_receive(t,1,2);traffic_add_move_send(t,1,1,3);traffic_clear(t);EXPECT_EQ(t.receive_size[1],0u);EXPECT_EQ(t.move_send_size[0][0],0u);}