#include "mxh/server/agent_chat.hpp"
#include <gtest/gtest.h>
using namespace mxh::server;
TEST(AgentChat, BroadcastProtocolsClassify){EXPECT_EQ(classify_chat(chat_all,true).route,ChatRoute::broadcast);EXPECT_EQ(classify_chat(chat_smallshout,true).route,ChatRoute::broadcast);EXPECT_EQ(classify_chat(chat_monster_speech,false).route,ChatRoute::broadcast);}
TEST(AgentChat, GmSmallShoutRequiresGm){EXPECT_EQ(classify_chat(chat_gm_smallshout,true,false).route,ChatRoute::rejected);EXPECT_EQ(classify_chat(chat_gm_smallshout,true,true).route,ChatRoute::broadcast);}
TEST(AgentChat, WhisperRequiresTarget){auto a=classify_chat(chat_whisper_syn,true);EXPECT_EQ(a.route,ChatRoute::whisper);EXPECT_TRUE(a.requires_target);EXPECT_FALSE(a.gm_only);EXPECT_TRUE(classify_chat(chat_whisper_gm_syn,true).gm_only);}
TEST(AgentChat, GroupRoutes){EXPECT_EQ(classify_chat(chat_party,true).route,ChatRoute::party);EXPECT_EQ(classify_chat(chat_guild,true).route,ChatRoute::guild);EXPECT_EQ(classify_chat(chat_guild_union,true).route,ChatRoute::guild_union);}
TEST(AgentChat, ServerShoutAndUserFastchat){EXPECT_EQ(classify_chat(chat_shout_send_server,false).route,ChatRoute::shout_server);EXPECT_EQ(classify_chat(chat_fastchat,true).route,ChatRoute::fast_chat);EXPECT_EQ(classify_chat(chat_fastchat,false).route,ChatRoute::rejected);}
TEST(AgentChat, UnknownProtocolRejected){EXPECT_EQ(classify_chat(255,true).route,ChatRoute::rejected);}