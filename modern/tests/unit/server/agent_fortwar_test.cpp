#include "mxh/server/agent_fortwar.hpp"
#include <gtest/gtest.h>
using namespace mxh::server;
TEST(FortWar, StartBefore10MinBroadcastsToAllUsers){FortWarRequest r;r.protocol=fortwar_start_before10min;auto a=classify_fortwar(r);EXPECT_EQ(a.kind,FortWarActionKind::broadcast_to_all_users);EXPECT_EQ(a.protocol,fortwar_start_before10min);}
TEST(FortWar, StartBroadcastsToAllUsers){FortWarRequest r;r.protocol=fortwar_start;EXPECT_EQ(classify_fortwar(r).kind,FortWarActionKind::broadcast_to_all_users);}
TEST(FortWar, EndBroadcastsToAllUsers){FortWarRequest r;r.protocol=fortwar_end;EXPECT_EQ(classify_fortwar(r).kind,FortWarActionKind::broadcast_to_all_users);}
TEST(FortWar, StartBefore10MinToMapBroadcastsToOtherMaps){FortWarRequest r;r.protocol=fortwar_start_before10min_to_map;EXPECT_EQ(classify_fortwar(r).kind,FortWarActionKind::broadcast_to_other_maps);}
TEST(FortWar, StartToMapBroadcastsToOtherMaps){FortWarRequest r;r.protocol=fortwar_start_to_map;EXPECT_EQ(classify_fortwar(r).kind,FortWarActionKind::broadcast_to_other_maps);}
TEST(FortWar, InfoUserFoundForwardsToUser){FortWarRequest r;r.protocol=fortwar_info;r.user_object_found=true;EXPECT_EQ(classify_fortwar(r).kind,FortWarActionKind::forward_to_user_if_found);}
TEST(FortWar, InfoUserMissingDrops){FortWarRequest r;r.protocol=fortwar_info;r.user_object_found=false;EXPECT_EQ(classify_fortwar(r).kind,FortWarActionKind::drop_no_user);}
TEST(FortWar, IngUserFoundForwardsToUser){FortWarRequest r;r.protocol=fortwar_ing;r.user_object_found=true;EXPECT_EQ(classify_fortwar(r).kind,FortWarActionKind::forward_to_user_if_found);}