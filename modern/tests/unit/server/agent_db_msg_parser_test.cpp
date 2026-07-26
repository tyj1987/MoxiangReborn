#include "mxh/server/agent_db_msg_parser.hpp"
#include <gtest/gtest.h>
using namespace mxh::server;
TEST(AgentDbDispatcher, KeepsLegacyQueryBounds){AgentDbDispatcher d;EXPECT_TRUE(d.has_slot(0));EXPECT_TRUE(d.has_slot(max_query-1));EXPECT_FALSE(d.has_slot(max_query));}
TEST(AgentDbDispatcher, RejectsOutOfRangeQuery){AgentDbDispatcher d;AgentDbResult r;r.query_id=static_cast<std::uint16_t>(max_query);EXPECT_FALSE(d.dispatch(r));}
TEST(AgentDbDispatcher, EmptyLegacySlotIsAcceptedNoOp){AgentDbDispatcher d;AgentDbResult r;r.query_id=1;EXPECT_TRUE(d.dispatch(r));}
TEST(AgentDbDispatcher, InvokesRegisteredHandler){AgentDbDispatcher d;int called=0;d.register_handler(1,[&](const AgentDbResult&r){called++;EXPECT_EQ(r.result,7);EXPECT_EQ(r.connection_index,8u);});AgentDbResult r{1,8,7,{}};EXPECT_TRUE(d.dispatch(r));EXPECT_EQ(called,1);}
TEST(AgentDbDispatcher, DuplicateRegistrationReplaces){AgentDbDispatcher d;d.register_handler(1,[](const AgentDbResult&){});d.register_handler(1,[](const AgentDbResult&){});EXPECT_EQ(d.handler_count(),1u);}
TEST(AgentDbDispatcher, NullHandlerDoesNotRegister){AgentDbDispatcher d;d.register_handler(1,{});EXPECT_EQ(d.handler_count(),0u);}