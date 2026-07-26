#include "mxh/server/agent_siegewar_server.hpp"
#include <gtest/gtest.h>
using namespace mxh::server;
TEST(SiegeWarServer, TaxRateBroadcastsToAffectedMaps){SiegeWarServerRequest r;r.protocol=siegewar_taxrate;r.affected_count=5;auto a=classify_siegewar_server(r);EXPECT_EQ(a.kind,SiegeWarServerActionKind::broadcast_taxrate_to_affected_maps);EXPECT_EQ(a.protocol,siegewar_taxrate);}
TEST(SiegeWarServer, ReturnToMapUserMissingDrops){SiegeWarServerRequest r;r.protocol=siegewar_returntomap;r.user_found=false;EXPECT_EQ(classify_siegewar_server(r).kind,SiegeWarServerActionKind::drop_no_user);}
TEST(SiegeWarServer, ReturnToMapTargetMapFoundUpdatesUser){SiegeWarServerRequest r;r.protocol=siegewar_returntomap;r.user_found=true;r.target_map_found=true;r.target_map=42;auto a=classify_siegewar_server(r);EXPECT_EQ(a.kind,SiegeWarServerActionKind::update_user_map_and_forward_to_client);EXPECT_EQ(a.target_map,42u);}
TEST(SiegeWarServer, ReturnToMapTargetMapMissingStillForwards){SiegeWarServerRequest r;r.protocol=siegewar_returntomap;r.user_found=true;r.target_map_found=false;EXPECT_EQ(classify_siegewar_server(r).kind,SiegeWarServerActionKind::default_forward_to_client);}
TEST(SiegeWarServer, FlagChangeBroadcastsToAllUsers){SiegeWarServerRequest r;r.protocol=siegewar_flagchange;EXPECT_EQ(classify_siegewar_server(r).kind,SiegeWarServerActionKind::broadcast_to_all_users);}
TEST(SiegeWarServer, DefaultForwardsToClient){SiegeWarServerRequest r;r.protocol=99;EXPECT_EQ(classify_siegewar_server(r).kind,SiegeWarServerActionKind::default_forward_to_client);}