#include "mxh/server/agent_siegewar.hpp"
#include <gtest/gtest.h>
using namespace mxh::server;
TEST(SiegeWarUser, CheatAlwaysFansOut){SiegeWarUserRequest r;r.protocol=siegewar_cheat;r.user_found=false;EXPECT_EQ(classify_siegewar_user(r).kind,SiegeWarUserActionKind::cheat_fanout_to_map_servers);}
TEST(SiegeWarUser, NoUserDropsAllNonCheat){SiegeWarUserRequest r;r.protocol=siegewar_movein_syn;r.user_found=false;EXPECT_EQ(classify_siegewar_user(r).kind,SiegeWarUserActionKind::drop_no_user);}
TEST(SiegeWarUser, MoveInSynSendsToUserMap){SiegeWarUserRequest r;r.protocol=siegewar_movein_syn;r.user_found=true;auto a=classify_siegewar_user(r);EXPECT_EQ(a.kind,SiegeWarUserActionKind::movein_to_user_map);EXPECT_EQ(a.protocol,siegewar_movein_syn);}
TEST(SiegeWarUser, BattleJoinTargetMapFoundForwards){SiegeWarUserRequest r;r.protocol=siegewar_battlejoin_syn;r.user_found=true;r.target_map_found=true;EXPECT_EQ(classify_siegewar_user(r).kind,SiegeWarUserActionKind::battlejoin_to_target_map_or_nack);}
TEST(SiegeWarUser, BattleJoinNoTargetMapNacks){SiegeWarUserRequest r;r.protocol=siegewar_battlejoin_syn;r.user_found=true;r.target_map_found=false;auto a=classify_siegewar_user(r);EXPECT_EQ(a.kind,SiegeWarUserActionKind::battlejoin_to_target_map_or_nack);EXPECT_EQ(a.protocol,siegewar_battlejoin_nack);}
TEST(SiegeWarUser, ObserverJoinTargetFoundForwards){SiegeWarUserRequest r;r.protocol=siegewar_observerjoin_syn;r.user_found=true;r.target_map_found=true;EXPECT_EQ(classify_siegewar_user(r).kind,SiegeWarUserActionKind::battlejoin_to_target_map_or_nack);}
TEST(SiegeWarUser, LeaveSynSendsToUserMap){SiegeWarUserRequest r;r.protocol=siegewar_leave_syn;auto a=classify_siegewar_user(r);EXPECT_EQ(a.kind,SiegeWarUserActionKind::leave_syn_to_user_map);EXPECT_EQ(a.protocol,siegewar_leave_syn);}
TEST(SiegeWarUser, DefaultForwardsToMap){SiegeWarUserRequest r;r.protocol=99;r.user_found=true;EXPECT_EQ(classify_siegewar_user(r).kind,SiegeWarUserActionKind::default_forward_to_map);}