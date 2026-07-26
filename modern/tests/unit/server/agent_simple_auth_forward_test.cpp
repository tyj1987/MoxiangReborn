#include "mxh/server/agent_simple_auth_forward.hpp"
#include <gtest/gtest.h>
using namespace mxh::server;
TEST(StreetStallUser, NoUserDrops){StreetStallUserRequest r;r.user_found=false;EXPECT_EQ(classify_streetstall_user(r).kind,StreetStallUserActionKind::drop_no_user);}
TEST(StreetStallUser, CharacterMismatchDrops){StreetStallUserRequest r;r.user_found=true;r.character_id=1;r.object_id=2;EXPECT_EQ(classify_streetstall_user(r).kind,StreetStallUserActionKind::drop_object_mismatch);}
TEST(StreetStallUser, MatchForwardsToMap){StreetStallUserRequest r;r.user_found=true;r.character_id=10;r.object_id=10;EXPECT_EQ(classify_streetstall_user(r).kind,StreetStallUserActionKind::forward_to_map);}
TEST(ExchangeUser, NoUserDrops){ExchangeUserRequest r;r.user_found=false;EXPECT_EQ(classify_exchange_user(r).kind,ExchangeUserActionKind::drop_no_user);}
TEST(ExchangeUser, CharacterMismatchDrops){ExchangeUserRequest r;r.user_found=true;r.character_id=3;r.object_id=4;EXPECT_EQ(classify_exchange_user(r).kind,ExchangeUserActionKind::drop_object_mismatch);}
TEST(ExchangeUser, MatchForwardsToMap){ExchangeUserRequest r;r.user_found=true;r.character_id=7;r.object_id=7;EXPECT_EQ(classify_exchange_user(r).kind,ExchangeUserActionKind::forward_to_map);}
TEST(SimpleAuthForward, BothCategoriesAreNonzero){EXPECT_NE(streetstall_category,exchange_category);EXPECT_NE(streetstall_category,0);EXPECT_NE(exchange_category,0);}