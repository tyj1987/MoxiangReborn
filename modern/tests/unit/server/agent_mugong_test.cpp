// agent_mugong_test.cpp - Phase 6.3 AgentMugong 1:1 port.
#include "mxh/server/agent_mugong.hpp"
#include <gtest/gtest.h>
using namespace mxh::server;
TEST(AgentMugongTest, CategoryByteIs9){ EXPECT_EQ(mugong_category,9u); }
TEST(AgentMugongTest, MugongTotalinfoLocalIs0){ EXPECT_EQ(mugong_totalinfo_local,0u); }
TEST(AgentMugongTest, MugongMoveSynIs1){ EXPECT_EQ(mugong_move_syn,1u); }
TEST(AgentMugongTest, MugongAddSynIs7){ EXPECT_EQ(mugong_add_syn,7u); }
TEST(AgentMugongTest, MugongExppointNotifyIs16){ EXPECT_EQ(mugong_exppoint_notify,16u); }
TEST(AgentMugongTest, MugongSungLevelupIs18){ EXPECT_EQ(mugong_sung_levelup,18u); }
TEST(AgentMugongTest, MugongOptionClearAckIs23){ EXPECT_EQ(mugong_option_clear_ack,23u); }
TEST(AgentMugongTest, ClassifyMoveForwardsToMap){ MugongRequest r{mugong_move_syn,5u,0u,0u,0u}; auto a=classify_mugong(r); EXPECT_EQ(a.kind,MugongActionKind::forward_to_map); EXPECT_EQ(a.protocol,mugong_move_syn); EXPECT_EQ(a.object_id,5u); }
TEST(AgentMugongTest, ClassifyOptionLevelGateRejects){ MugongRequest r{mugong_option_syn,5u,1u,5u,10u}; auto a=classify_mugong(r); EXPECT_EQ(a.kind,MugongActionKind::send_nack); EXPECT_EQ(a.protocol,mugong_option_nack); EXPECT_EQ(a.error_code,1u); }
TEST(AgentMugongTest, ClassifyOptionLevelGateAccepts){ MugongRequest r{mugong_option_syn,5u,1u,15u,10u}; auto a=classify_mugong(r); EXPECT_EQ(a.kind,MugongActionKind::forward_to_map); EXPECT_EQ(a.protocol,mugong_option_syn); EXPECT_EQ(a.error_code,0u); }
