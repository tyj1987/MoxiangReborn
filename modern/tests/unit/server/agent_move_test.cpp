// agent_move_test.cpp - Phase 6.3 AgentMove 1:1 port.
#include "mxh/server/agent_move.hpp"
#include <gtest/gtest.h>
using namespace mxh::server;
TEST(AgentMoveTest, CategoryByteIs8){ EXPECT_EQ(move_category,8u); }
TEST(AgentMoveTest, MoveInitIs0){ EXPECT_EQ(move_init,0u); }
TEST(AgentMoveTest, MoveTargetIs1){ EXPECT_EQ(move_target,1u); }
TEST(AgentMoveTest, MoveWalkmodeIs3){ EXPECT_EQ(move_walkmode,3u); }
TEST(AgentMoveTest, MoveKyunggongAckIs6){ EXPECT_EQ(move_kyunggong_ack,6u); }
TEST(AgentMoveTest, MoveStopIs8){ EXPECT_EQ(move_stop,8u); }
TEST(AgentMoveTest, MoveWarpIs12){ EXPECT_EQ(move_warp,12u); }
TEST(AgentMoveTest, MovePetWarpAckIs19){ EXPECT_EQ(move_pet_warp_ack,19u); }
TEST(AgentMoveTest, ClassifyForwardsIfUserInMap){ MoveRequest r{move_target,5u,true}; auto a=classify_move(r); EXPECT_EQ(a.kind,MoveActionKind::forward_to_map); EXPECT_EQ(a.protocol,move_target); EXPECT_EQ(a.object_id,5u); }
TEST(AgentMoveTest, ClassifyDropsWhenUserNotInMap){ MoveRequest r{move_target,5u,false}; auto a=classify_move(r); EXPECT_EQ(a.kind,MoveActionKind::drop_no_map); EXPECT_EQ(a.object_id,5u); }
TEST(AgentMoveTest, ClassifyWarpForwards){ MoveRequest r{move_warp,5u,true}; auto a=classify_move(r); EXPECT_EQ(a.kind,MoveActionKind::forward_to_map); EXPECT_EQ(a.protocol,move_warp); }
