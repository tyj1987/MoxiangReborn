// agent_user_test.cpp - Phase 6.3 AgentUser 1:1 port.
#include "mxh/server/agent_user.hpp"
#include <gtest/gtest.h>
#include <cstring>
using namespace mxh::server;
TEST(AgentUserTest, InsertEmptyRecord){ AgentUserRecord r{}; EXPECT_TRUE(insert_agent_user(r,0x12345678u,100u)); EXPECT_TRUE(r.in_use); EXPECT_EQ(r.info.dwAuthKey,0x12345678u); EXPECT_EQ(r.info.dwObjectID,100u); }
TEST(AgentUserTest, InsertDuplicateKeyRejects){ AgentUserRecord r{}; ASSERT_TRUE(insert_agent_user(r,1u,1u)); EXPECT_FALSE(insert_agent_user(r,2u,2u)); EXPECT_EQ(r.info.dwAuthKey,1u); EXPECT_EQ(r.info.dwObjectID,1u); }
TEST(AgentUserTest, RemoveClearsRecord){ AgentUserRecord r{}; ASSERT_TRUE(insert_agent_user(r,1u,2u)); EXPECT_TRUE(remove_agent_user(r)); EXPECT_FALSE(r.in_use); EXPECT_EQ(r.info.dwAuthKey,0u); EXPECT_EQ(r.info.dwObjectID,0u); }
TEST(AgentUserTest, RemoveEmptyRecordReturnsFalse){ AgentUserRecord r{}; EXPECT_FALSE(remove_agent_user(r)); }
TEST(AgentUserTest, AssignMapChannelSetsNonZero){ AgentUserRecord r{}; ASSERT_TRUE(insert_agent_user(r,1u,1u)); EXPECT_TRUE(assign_agent_user_map(r,7u)); EXPECT_EQ(r.info.dwMapChannel,7u); }
TEST(AgentUserTest, AssignMapChannelZeroRejects){ AgentUserRecord r{}; ASSERT_TRUE(insert_agent_user(r,1u,1u)); EXPECT_FALSE(assign_agent_user_map(r,0u)); EXPECT_EQ(r.info.dwMapChannel,0u); }
TEST(AgentUserTest, ToggleForceMoveFlips){ AgentUserRecord r{}; ASSERT_TRUE(insert_agent_user(r,1u,1u)); EXPECT_TRUE(toggle_agent_user_force_move(r)); EXPECT_EQ(r.info.bForceMove,1u); EXPECT_FALSE(toggle_agent_user_force_move(r)); EXPECT_EQ(r.info.bForceMove,0u); }
TEST(AgentUserTest, UserInfoNameBufferIs17Bytes){ AgentUserInfo info{}; const char* nm="hello"; std::strncpy(info.name,nm,5); EXPECT_EQ(std::strlen(info.name),5u); EXPECT_EQ(sizeof(info.name),17u); }