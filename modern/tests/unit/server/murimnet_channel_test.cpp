#include "mxh/server/murimnet_channel.hpp"
#include <gtest/gtest.h>
using namespace mxh::server;
static MnChannelCreateInfo ci(){MnChannelCreateInfo i;i.max_players=2;i.title="test";return i;}
TEST(MurimNetChannel, DefaultAndCapacity){MurimNetChannelManager m;auto d=ci();d.channel_index=1;ASSERT_TRUE(m.init(2,d));ASSERT_NE(m.default_channel(),nullptr);MnRoomPlayer a{1},b{2},c{3};EXPECT_TRUE(m.enter_default(a));EXPECT_TRUE(m.enter_default(b));EXPECT_FALSE(m.enter_default(c));EXPECT_EQ(a.location_index,1u);}
TEST(MurimNetChannel, CreateDeleteAndLookup){MurimNetChannelManager m;ASSERT_TRUE(m.init(3));auto* c=m.create_channel(ci());ASSERT_NE(c,nullptr);EXPECT_EQ(m.get_channel(c->channel_index()),c);EXPECT_TRUE(m.delete_channel(c->channel_index()));EXPECT_EQ(m.get_channel(c->channel_index()),nullptr);}
TEST(MurimNetChannel, PlayerOutClearsLocation){MurimNetChannelManager m;ASSERT_TRUE(m.init(2));MnRoomPlayer p{7};ASSERT_TRUE(m.enter_default(p));EXPECT_TRUE(m.exit(p));EXPECT_EQ(p.location_index,0u);EXPECT_FALSE(m.exit(p));}
TEST(MurimNetChannel, ReleaseClearsPlayers){MurimNetChannelManager m;ASSERT_TRUE(m.init(2));MnRoomPlayer p{7};ASSERT_TRUE(m.enter_default(p));m.release();EXPECT_EQ(p.location_index,0u);EXPECT_EQ(m.channel_count(),0u);}
