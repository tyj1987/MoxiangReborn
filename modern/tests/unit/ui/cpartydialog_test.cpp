#include "cpartydialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(PartyDialog, FirstMemberBecomesLeaderAndCanTransfer){cPartyDialog d;d.SetSelfId(1);d.AddMember({1,"Me",false});d.AddMember({2,"Bob",false});ASSERT_EQ(d.Leader()->id,1u);EXPECT_TRUE(d.TransferLeader(2));EXPECT_EQ(d.Leader()->id,2u);}
TEST(PartyDialog, LeaderCannotBeRemovedAndMembersCanBeRemoved){cPartyDialog d;d.SetSelfId(1);d.AddMember({1,"Me",false});d.AddMember({2,"Bob",false});EXPECT_FALSE(d.RemoveMember(1));EXPECT_TRUE(d.RemoveMember(2));}
TEST(PartyDialog, InvitesThroughCallback){cPartyDialog d;std::string_view got;d.SetInviteCallback([&](std::string_view n){got=n;return true;});EXPECT_TRUE(d.Invite("Alice"));EXPECT_EQ(got,"Alice");EXPECT_FALSE(d.Invite(""));}
