#include "mxh/server/agent_bobusang_user.hpp"
#include <gtest/gtest.h>
using namespace mxh::server;
TEST(BobusangUser, NoUserDrops){BobusangUserRequest r;r.user_found=false;EXPECT_EQ(classify_bobusang_user(r).kind,BobusangUserActionKind::drop_no_user);}
TEST(BobusangUser, NonGmForwardsToMap){BobusangUserRequest r;r.user_found=true;r.is_gm=false;EXPECT_EQ(classify_bobusang_user(r).kind,BobusangUserActionKind::forward_to_map);}
TEST(BobusangUser, GmWithMasterPowerForwards){BobusangUserRequest r;r.user_found=true;r.is_gm=true;r.gm_master_or_below=true;EXPECT_EQ(classify_bobusang_user(r).kind,BobusangUserActionKind::forward_to_map);}
TEST(BobusangUser, GmAboveMasterDrops){BobusangUserRequest r;r.user_found=true;r.is_gm=true;r.gm_master_or_below=false;EXPECT_EQ(classify_bobusang_user(r).kind,BobusangUserActionKind::drop_wrong_gm_power);}