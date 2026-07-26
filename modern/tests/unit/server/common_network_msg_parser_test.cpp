#include "mxh/server/common_network_msg_parser.hpp"
#include <gtest/gtest.h>
using namespace mxh::server;
TEST(CommonParserTables, AllocatesLegacyMpMaxSlots){CommonParserTables t;EXPECT_EQ(t.server.size(),mp_max);EXPECT_EQ(t.user.size(),mp_max);}
TEST(CommonParserTables, EmptySlotIsNoOp){CommonParserTables t;EXPECT_TRUE(t.invoke_server(1,2,{}));}
TEST(CommonParserTables, OutOfRangeRejected){CommonParserTables t;EXPECT_FALSE(t.invoke_user(mp_max,0,{}));}
TEST(CommonParserTables, DispatchesServerAndUserSeparately){CommonParserTables t;int a=0,b=0;t.set_server(3,[&](auto,auto){++a;});t.set_user(3,[&](auto,auto){++b;});t.invoke_server(3,0,{});t.invoke_user(3,0,{});EXPECT_EQ(a,1);EXPECT_EQ(b,1);}
TEST(CommonParserTables, NullParserRejected){CommonParserTables t;EXPECT_FALSE(t.set_server(2,{}));}