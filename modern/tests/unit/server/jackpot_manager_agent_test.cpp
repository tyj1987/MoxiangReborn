#include "mxh/server/jackpot_manager_agent.hpp"
#include <gtest/gtest.h>
using namespace mxh::server;
TEST(JackpotAgent, InitUsesLegacyUpdateLength){JackpotState s;s.total_money=9;jackpot_init(s);EXPECT_EQ(s.total_money,0u);EXPECT_EQ(s.update_length,60000u);EXPECT_FALSE(s.manager);}
TEST(JackpotAgent, ServerZeroIsManager){JackpotState s;jackpot_init(s);jackpot_start(s,0);EXPECT_TRUE(s.manager);}
TEST(JackpotAgent, OtherServersAreNotManager){JackpotState s;jackpot_init(s);jackpot_start(s,2);EXPECT_FALSE(s.manager);}
TEST(JackpotAgent, ProcessIsThrottled){JackpotState s;jackpot_init(s);jackpot_start(s,0);EXPECT_TRUE(jackpot_process(s,59999).empty());auto a=jackpot_process(s,60000);ASSERT_EQ(a.size(),1u);EXPECT_EQ(a[0].kind,JackpotActionKind::load_db);EXPECT_TRUE(jackpot_process(s,60001).empty());}
TEST(JackpotAgent, SetMoneyBroadcastsBothScopes){JackpotState s;auto a=jackpot_set_total_money(s,1234);EXPECT_EQ(s.total_money,1234u);ASSERT_EQ(a.size(),2u);EXPECT_EQ(a[0].kind,JackpotActionKind::notify_agents);EXPECT_EQ(a[1].kind,JackpotActionKind::notify_users);}
TEST(JackpotAgent, CharacterNotificationCarriesObjectId){JackpotState s;s.total_money=77;auto a=jackpot_notify_character(s,88);EXPECT_EQ(a.kind,JackpotActionKind::notify_character);EXPECT_EQ(a.character_id,88u);EXPECT_EQ(a.total_money,77u);}