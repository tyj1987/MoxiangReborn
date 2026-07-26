#include "mxh/server/agent_skill.hpp"
#include <gtest/gtest.h>
using namespace mxh::server;
TEST(AgentSkill, UserNonPremierForwards){SkillDelayManager m;auto a=process_agent_skill_user(m,7,99,0);EXPECT_EQ(a.kind,AgentSkillActionKind::forward_to_map);}
TEST(AgentSkill, UserFirstPremierForwards){SkillDelayManager m;add_premier_skill(m,100,10000);auto a=process_agent_skill_user(m,7,100,0);EXPECT_EQ(a.kind,AgentSkillActionKind::forward_to_map);}
TEST(AgentSkill, UserCooldownSendsNack){SkillDelayManager m;add_premier_skill(m,100,10000);process_agent_skill_user(m,7,100,0);auto a=process_agent_skill_user(m,7,100,4000);EXPECT_EQ(a.kind,AgentSkillActionKind::send_start_nack);EXPECT_EQ(a.protocol,2);EXPECT_EQ(a.character_id,7u);}
TEST(AgentSkill, ServerPathForcesAndForwards){SkillDelayManager m;add_premier_skill(m,100,10000);process_agent_skill_server(m,7,100,0);auto a=process_agent_skill_server(m,7,100,0);EXPECT_EQ(a.kind,AgentSkillActionKind::forward_to_map);}
TEST(AgentSkill, OtherProtocolsForward){auto a=process_agent_skill_other(7,100);EXPECT_EQ(a.kind,AgentSkillActionKind::forward_to_map);EXPECT_EQ(a.protocol,0);}