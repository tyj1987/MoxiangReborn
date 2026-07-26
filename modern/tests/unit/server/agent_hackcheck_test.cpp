#include "mxh/server/agent_hackcheck.hpp"
#include <gtest/gtest.h>
using namespace mxh::server;
TEST(HackCheck, SpeedHackUserMissingDrops){HackCheckRequest r;r.protocol=hackcheck_speedhack;r.user_found=false;EXPECT_EQ(classify_hackcheck(r).kind,HackCheckActionKind::drop_no_user);}
TEST(HackCheck, SpeedHackDetectedWhenUnderThreshold){HackCheckRequest r;r.protocol=hackcheck_speedhack;r.user_found=true;r.server_time=11000;r.client_time=10500;auto a=classify_hackcheck(r);EXPECT_EQ(a.kind,HackCheckActionKind::detect_speedhack_and_ban);EXPECT_EQ(a.protocol,hackcheck_ban_user);EXPECT_EQ(a.data,500u);}
TEST(HackCheck, SpeedHackIgnoredWhenAboveThreshold){HackCheckRequest r;r.protocol=hackcheck_speedhack;r.user_found=true;r.server_time=20000;r.client_time=10000;EXPECT_EQ(classify_hackcheck(r).kind,HackCheckActionKind::ignore);}
TEST(HackCheck, BanUserToAgentAlwaysBans){HackCheckRequest r;r.protocol=hackcheck_ban_user_toagent;r.user_found=true;auto a=classify_hackcheck(r);EXPECT_EQ(a.kind,HackCheckActionKind::ban_user_to_agent_always);EXPECT_EQ(a.protocol,hackcheck_ban_user);}
TEST(HackCheck, BanUserToAgentUserMissingDrops){HackCheckRequest r;r.protocol=hackcheck_ban_user_toagent;r.user_found=false;EXPECT_EQ(classify_hackcheck(r).kind,HackCheckActionKind::drop_no_user);}
TEST(HackCheck, UnknownProtocolIgnored){HackCheckRequest r;r.protocol=99;EXPECT_EQ(classify_hackcheck(r).kind,HackCheckActionKind::ignore);}