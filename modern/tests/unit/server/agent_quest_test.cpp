// agent_quest_test.cpp - Phase 6.3 AgentQuest 1:1 port.
#include "mxh/server/agent_quest.hpp"
#include <gtest/gtest.h>
using namespace mxh::server;
TEST(AgentQuestTest, CategoryByteIs38){ EXPECT_EQ(quest_category,38u); }
TEST(AgentQuestTest, QuestTotalinfoIs0){ EXPECT_EQ(quest_totalinfo,0u); }
TEST(AgentQuestTest, QuestDeleteSynIs6){ EXPECT_EQ(quest_delete_syn,6u); }
TEST(AgentQuestTest, QuestStartSynIs9){ EXPECT_EQ(quest_start_syn,9u); }
TEST(AgentQuestTest, QuestEndAckIs13){ EXPECT_EQ(quest_end_ack,13u); }
TEST(AgentQuestTest, QuestTimeLimitIs25){ EXPECT_EQ(quest_time_limit,25u); }
TEST(AgentQuestTest, QuestFullIs27){ EXPECT_EQ(quest_full,27u); }
TEST(AgentQuestTest, ClassifyForwardsToMap){ QuestRequest r{quest_start_syn,7u,true}; auto a=classify_quest(r); EXPECT_EQ(a.kind,QuestActionKind::forward_to_map); EXPECT_EQ(a.protocol,quest_start_syn); EXPECT_EQ(a.object_id,7u); }
TEST(AgentQuestTest, ClassifyEndForwardsToMap){ QuestRequest r{quest_end_syn,99u,true}; auto a=classify_quest(r); EXPECT_EQ(a.kind,QuestActionKind::forward_to_map); EXPECT_EQ(a.protocol,quest_end_syn); EXPECT_EQ(a.object_id,99u); }
