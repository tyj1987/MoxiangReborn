#include "mxh/server/agent_packed.hpp"
#include <gtest/gtest.h>
using namespace mxh::server;
TEST(PackedUser, NormalFanoutIncludesOnlyPresentReceivers){PackedRequest r;r.protocol=packed_normal;r.receiver_count=5;r.data_size=100;r.receivers_present={1,2,3};auto a=classify_packed_user(r);EXPECT_EQ(a.kind,PackedActionKind::fanout_to_users);EXPECT_EQ(a.receiver_count,3u);EXPECT_EQ(a.data_size,100);}
TEST(PackedUser, ToMapServerWithPortFoundForwards){PackedRequest r;r.protocol=packed_to_mapserver;r.target_map_num=12;r.target_map_port_found=true;auto a=classify_packed_user(r);EXPECT_EQ(a.kind,PackedActionKind::send_to_map_server_by_port);EXPECT_EQ(a.protocol,packed_to_mapserver);}
TEST(PackedUser, ToMapServerPortMissingReturnsUnknown){PackedRequest r;r.protocol=packed_to_mapserver;r.target_map_port_found=false;EXPECT_EQ(classify_packed_user(r).kind,PackedActionKind::unknown);}
TEST(PackedUser, ToBroadMapServerBroadcasts){PackedRequest r;r.protocol=packed_to_broad_mapserver;auto a=classify_packed_user(r);EXPECT_EQ(a.kind,PackedActionKind::broadcast_to_other_maps);EXPECT_EQ(a.protocol,packed_to_broad_mapserver);}
TEST(PackedUser, UnknownProtocolReturnsUnknown){PackedRequest r;r.protocol=99;EXPECT_EQ(classify_packed_user(r).kind,PackedActionKind::unknown);}