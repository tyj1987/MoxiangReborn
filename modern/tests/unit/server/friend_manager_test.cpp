// friend_manager_test.cpp - Phase D5 FriendManager 1:1 port tests.

#include "mxh/server/friend_manager.hpp"
#include <gtest/gtest.h>

namespace {
using mxh::server::friend_user_logout;
using mxh::server::friend_is_user_logged_out;
using mxh::server::friend_clear_logged_out;
using mxh::server::friend_manager_singleton;
}

TEST(FriendManager, LogoutRecordsUserId) {
    auto& s = friend_manager_singleton();
    s.logged_out_users.clear();

    friend_user_logout(1001u);
    EXPECT_TRUE(friend_is_user_logged_out(1001u));
    EXPECT_FALSE(friend_is_user_logged_out(2002u));
}

TEST(FriendManager, ClearRemovesRecord) {
    auto& s = friend_manager_singleton();
    s.logged_out_users.clear();
    friend_user_logout(1001u);
    friend_clear_logged_out(1001u);
    EXPECT_FALSE(friend_is_user_logged_out(1001u));
}

TEST(FriendManager, IdempotentLogout) {
    auto& s = friend_manager_singleton();
    s.logged_out_users.clear();
    friend_user_logout(1001u);
    friend_user_logout(1001u);
    friend_user_logout(1001u);
    EXPECT_EQ(s.logged_out_users.size(), 1u);
}
