#include "mxh/server/agent_itemlimit.hpp"
#include <gtest/gtest.h>
using namespace mxh::server;
TEST(ItemLimit, AddCountToMapBroadcastsToOtherMaps){ItemLimitRequest r;r.protocol=itemlimit_addcount_to_map;auto a=classify_itemlimit(r);EXPECT_EQ(a.kind,ItemLimitActionKind::broadcast_to_other_maps);EXPECT_EQ(a.protocol,itemlimit_addcount_to_map);}
TEST(ItemLimit, FullToClientForwardsToClient){ItemLimitRequest r;r.protocol=itemlimit_full_to_client;EXPECT_EQ(classify_itemlimit(r).kind,ItemLimitActionKind::forward_to_client);}
TEST(ItemLimit, UnknownProtocolForwardsToClient){ItemLimitRequest r;r.protocol=99;EXPECT_EQ(classify_itemlimit(r).kind,ItemLimitActionKind::forward_to_client);}