#include "cfrienddialog.hpp"
#include <gtest/gtest.h>
using namespace mxh::ui;
TEST(FriendDialog, AddsSelectsAndWhispers){cFriendDialog d;d.AddFriend({1,"Alice",FriendStatus::Online});FriendEntry got{};d.SetWhisperCallback([&](const auto&f){got=f;});ASSERT_TRUE(d.Select(0));EXPECT_TRUE(d.WhisperSelected());EXPECT_EQ(got.name,"Alice");}
TEST(FriendDialog, PreventsDuplicatesAndUpdatesStatus){cFriendDialog d;d.AddFriend({1,"Alice",{}});d.AddFriend({1,"Duplicate",FriendStatus::Busy});EXPECT_EQ(d.Friends().size(),1u);EXPECT_TRUE(d.UpdateStatus(1,FriendStatus::Busy));EXPECT_EQ(d.Friends()[0].status,FriendStatus::Busy);}
TEST(FriendDialog, RemovesAndRejectsUnknown){cFriendDialog d;d.AddFriend({1,"Alice",{}});EXPECT_TRUE(d.RemoveFriend(1));EXPECT_FALSE(d.RemoveFriend(1));EXPECT_FALSE(d.WhisperSelected());}
