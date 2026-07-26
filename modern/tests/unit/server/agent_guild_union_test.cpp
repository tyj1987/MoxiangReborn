#include "mxh/server/agent_guild_union.hpp"
#include <gtest/gtest.h>
using namespace mxh::server;
TEST(GuildUnionUser, NoUserDrops){GuildUnionRequest r;r.protocol=guild_union_create_syn;r.user_found=false;EXPECT_EQ(classify_guild_union_user(r).kind,GuildUnionActionKind::drop_no_user);}
TEST(GuildUnionUser, CreateSynInvalidCharSendsNack){GuildUnionRequest r;r.protocol=guild_union_create_syn;r.has_invalid_char=true;auto a=classify_guild_union_user(r);EXPECT_EQ(a.kind,GuildUnionActionKind::send_create_nack_to_user);EXPECT_EQ(a.protocol,guild_union_create_nack);EXPECT_EQ(a.error_code,guild_union_err_not_valid_name);}
TEST(GuildUnionUser, CreateSynUnusableNameSendsNack){GuildUnionRequest r;r.protocol=guild_union_create_syn;r.name_usable=false;EXPECT_EQ(classify_guild_union_user(r).kind,GuildUnionActionKind::send_create_nack_to_user);}
TEST(GuildUnionUser, CreateSynValidForwardsToMap){GuildUnionRequest r;r.protocol=guild_union_create_syn;r.name_usable=true;r.has_invalid_char=false;auto a=classify_guild_union_user(r);EXPECT_EQ(a.kind,GuildUnionActionKind::forward_to_map);EXPECT_EQ(a.protocol,guild_union_create_syn);}
TEST(GuildUnionUser, NonCreateProtocolForwardsToMap){GuildUnionRequest r;r.protocol=99;EXPECT_EQ(classify_guild_union_user(r).kind,GuildUnionActionKind::forward_to_map);}