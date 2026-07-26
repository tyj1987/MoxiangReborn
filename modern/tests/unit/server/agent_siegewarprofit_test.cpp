#include "mxh/server/agent_siegewarprofit.hpp"
#include <gtest/gtest.h>
using namespace mxh::server;
TEST(SiegeWarProfitUser, AlwaysForwardsToMap){EXPECT_EQ(classify_siegewarprofit_user().kind,SiegeWarProfitUserActionKind::forward_to_map);}
TEST(SiegeWarProfitServer, TexRateNotifyBroadcasts){SiegeWarProfitRequest r;r.protocol=siegewarprofit_change_texrate_notify_to_map;auto a=classify_siegewarprofit_server(r);EXPECT_EQ(a.kind,SiegeWarProfitServerActionKind::broadcast_to_other_maps);EXPECT_EQ(a.protocol,siegewarprofit_change_texrate_notify_to_map);}
TEST(SiegeWarProfitServer, GuildNotifyBroadcasts){SiegeWarProfitRequest r;r.protocol=siegewarprofit_change_guild_notify_to_map;EXPECT_EQ(classify_siegewarprofit_server(r).kind,SiegeWarProfitServerActionKind::broadcast_to_other_maps);}
TEST(SiegeWarProfitServer, UnknownForwardsToClient){SiegeWarProfitRequest r;r.protocol=99;EXPECT_EQ(classify_siegewarprofit_server(r).kind,SiegeWarProfitServerActionKind::forward_to_client);}