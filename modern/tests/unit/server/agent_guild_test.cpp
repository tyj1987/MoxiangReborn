
#include "mxh/server/agent_guild.hpp"
#include <gtest/gtest.h>
using namespace mxh::server;
TEST(AgentGuild, RejectInvalidGuildName){GuildUserRequest r;r.object_id=7;r.usable_name=false;r.has_invalid_char=true;auto a=classify_guild_user(r);EXPECT_EQ(a.kind,GuildActionKind::send_nack);EXPECT_EQ(a.protocol,guild_create_nack);EXPECT_EQ(a.error_code,guild_err_create_name);}
TEST(AgentGuild, AcceptGuildName){GuildUserRequest r;r.object_id=7;r.usable_name=true;auto a=classify_guild_user(r);EXPECT_EQ(a.kind,GuildActionKind::forward);EXPECT_EQ(a.protocol,guild_create_syn);}
TEST(AgentGuild, RejectInvalidNickname){GuildUserRequest r;r.object_id=7;r.usable_name=false;r.has_quote_space=true;r.is_nickname_path=true;auto a=classify_guild_user(r);EXPECT_EQ(a.kind,GuildActionKind::send_nack);EXPECT_EQ(a.protocol,guild_givenickname_nack);EXPECT_EQ(a.error_code,guild_err_nick_filter);}
TEST(AgentGuild, AcceptNickname){GuildUserRequest r;r.object_id=7;r.usable_name=true;r.is_nickname_path=true;auto a=classify_guild_user(r);EXPECT_EQ(a.kind,GuildActionKind::forward);EXPECT_EQ(a.protocol,guild_givenickname_syn);}
TEST(AgentGuild, ServerDefaultForwards){EXPECT_EQ(classify_guild_server_default(10).kind,GuildActionKind::forward);}
