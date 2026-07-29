// agent_battle_test.cpp - Phase 6.3 AgentBattle 1:1 port.
#include "mxh/server/agent_battle.hpp"
#include <gtest/gtest.h>
using namespace mxh::server;
TEST(AgentBattleTest, CategoryByteIs31){ EXPECT_EQ(battle_category,31u); }
TEST(AgentBattleTest, BattleInfoByteIs0){ EXPECT_EQ(battle_info,0u); }
TEST(AgentBattleTest, BattleChatTeamSynIs1){ EXPECT_EQ(battle_chat_team_syn,1u); }
TEST(AgentBattleTest, BattleStartNotifyIs7){ EXPECT_EQ(battle_start_notify,7u); }
TEST(AgentBattleTest, BattleVictoryNotifyIs13){ EXPECT_EQ(battle_victory_notify,13u); }
TEST(AgentBattleTest, BattleDestroyNotifyIs15){ EXPECT_EQ(battle_destroy_notify,15u); }
TEST(AgentBattleTest, BattleResultIs16){ EXPECT_EQ(battle_result,16u); }
TEST(AgentBattleTest, BattleVimuStartIs21){ EXPECT_EQ(battle_vimu_start,21u); }
TEST(AgentBattleTest, ClassifyForwardsToMap){ BattleRequest r{battle_info,42u}; auto a=classify_battle(r); EXPECT_EQ(a.kind,BattleActionKind::forward_to_map); EXPECT_EQ(a.protocol,battle_info); EXPECT_EQ(a.object_id,42u); }
TEST(AgentBattleTest, ClassifyResultAlsoForwards){ BattleRequest r{battle_result,100u}; auto a=classify_battle(r); EXPECT_EQ(a.kind,BattleActionKind::forward_to_map); EXPECT_EQ(a.protocol,battle_result); EXPECT_EQ(a.object_id,100u); }
