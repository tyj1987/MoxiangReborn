
// friend_manager_data_plane_test.cpp
//
// Comprehensive data plane tests for mxh::server::friend_manager (D4.133).
// Augments the legacy 3-test friend_manager_test.cpp with deeper coverage of:
//   - Singleton identity invariant.
//   - FriendManagerState wire struct layout + default values.
//   - Logout/clear semantics across multiple users.
//   - Logout idempotence guarantees (no duplicates).
//   - Edge user IDs (max uint32).
//   - Cross-call state preservation.
//
// 1:1 invariants (locked):
//   - friend_manager_singleton() returns the same global instance
//     on every call (Meyers singleton).
//   - friend_user_logout(id) inserts id into the logged_out_users set.
//   - friend_is_user_logged_out(id) returns true iff id was logged out.
//   - friend_clear_logged_out(id) removes id (no-op if absent).
//   - Multiple logout calls for the same id result in exactly 1 entry
//     (set semantics).
//   - FriendManagerState default-constructed has empty logged_out_users.

#pragma once

#include "mxh/server/friend_manager.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <unordered_set>

namespace {

using mxh::server::friend_clear_logged_out;
using mxh::server::friend_is_user_logged_out;
using mxh::server::friend_manager_singleton;
using mxh::server::friend_user_logout;
using mxh::server::FriendManagerState;

}  // namespace


// ===========================================================================
// Singleton identity (1:1 with Meyers singleton)
// ===========================================================================

TEST(FriendManagerDataPlane, SingletonReturnsSameReference) {
    auto& a = friend_manager_singleton();
    auto& b = friend_manager_singleton();
    EXPECT_EQ(&a, &b);
}

TEST(FriendManagerDataPlane, SingletonIsGlobal) {
    auto& s = friend_manager_singleton();
    friend_user_logout(9001u);
    auto& t = friend_manager_singleton();
    EXPECT_TRUE(friend_is_user_logged_out(9001u));
    s.logged_out_users.clear();
    (void)t;
}


// ===========================================================================
// FriendManagerState default values
// ===========================================================================

TEST(FriendManagerDataPlane, FriendManagerStateDefaultIsEmpty) {
    FriendManagerState s{};
    EXPECT_TRUE(s.logged_out_users.empty());
    EXPECT_EQ(s.logged_out_users.size(), 0u);
}

TEST(FriendManagerDataPlane, FriendManagerStateSupportsInsert) {
    FriendManagerState s{};
    s.logged_out_users.insert(100u);
    s.logged_out_users.insert(200u);
    EXPECT_EQ(s.logged_out_users.size(), 2u);
    EXPECT_EQ(s.logged_out_users.count(100u), 1u);
    EXPECT_EQ(s.logged_out_users.count(200u), 1u);
    EXPECT_EQ(s.logged_out_users.count(300u), 0u);
}

TEST(FriendManagerDataPlane, FriendManagerStateIsUnorderedSet) {
    FriendManagerState s{};
    s.logged_out_users.insert(100u);
    s.logged_out_users.insert(50u);
    s.logged_out_users.insert(200u);
    EXPECT_EQ(s.logged_out_users.size(), 3u);
    EXPECT_EQ(s.logged_out_users.count(50u), 1u);
    EXPECT_EQ(s.logged_out_users.count(100u), 1u);
    EXPECT_EQ(s.logged_out_users.count(200u), 1u);
}


// ===========================================================================
// friend_user_logout() - basic semantics
// ===========================================================================

TEST(FriendManagerDataPlane, LogoutMarksUserAsLoggedOut) {
    auto& s = friend_manager_singleton();
    s.logged_out_users.clear();
    friend_user_logout(12345u);
    EXPECT_TRUE(friend_is_user_logged_out(12345u));
    s.logged_out_users.clear();
}

TEST(FriendManagerDataPlane, LogoutMultipleUsers) {
    auto& s = friend_manager_singleton();
    s.logged_out_users.clear();
    friend_user_logout(100u);
    friend_user_logout(200u);
    friend_user_logout(300u);
    EXPECT_TRUE(friend_is_user_logged_out(100u));
    EXPECT_TRUE(friend_is_user_logged_out(200u));
    EXPECT_TRUE(friend_is_user_logged_out(300u));
    EXPECT_EQ(s.logged_out_users.size(), 3u);
    s.logged_out_users.clear();
}

TEST(FriendManagerDataPlane, LogoutIsIdempotent) {
    auto& s = friend_manager_singleton();
    s.logged_out_users.clear();
    friend_user_logout(555u);
    friend_user_logout(555u);
    friend_user_logout(555u);
    EXPECT_EQ(s.logged_out_users.size(), 1u);
    s.logged_out_users.clear();
}

TEST(FriendManagerDataPlane, LogoutAcceptsMaxUint32) {
    auto& s = friend_manager_singleton();
    s.logged_out_users.clear();
    friend_user_logout(0xFFFFFFFFu);
    EXPECT_TRUE(friend_is_user_logged_out(0xFFFFFFFFu));
    EXPECT_EQ(s.logged_out_users.size(), 1u);
    s.logged_out_users.clear();
}

TEST(FriendManagerDataPlane, LogoutAcceptsZeroUserId) {
    auto& s = friend_manager_singleton();
    s.logged_out_users.clear();
    friend_user_logout(0u);
    EXPECT_TRUE(friend_is_user_logged_out(0u));
    s.logged_out_users.clear();
}


// ===========================================================================
// friend_is_user_logged_out() - basic semantics
// ===========================================================================

TEST(FriendManagerDataPlane, IsLoggedOutReturnsFalseForUnknown) {
    auto& s = friend_manager_singleton();
    s.logged_out_users.clear();
    EXPECT_FALSE(friend_is_user_logged_out(77777u));
    EXPECT_FALSE(friend_is_user_logged_out(0u));
    EXPECT_FALSE(friend_is_user_logged_out(0xFFFFFFFFu));
}

TEST(FriendManagerDataPlane, IsLoggedOutIsFalseAfterClear) {
    auto& s = friend_manager_singleton();
    s.logged_out_users.clear();
    friend_user_logout(100u);
    EXPECT_TRUE(friend_is_user_logged_out(100u));
    friend_clear_logged_out(100u);
    EXPECT_FALSE(friend_is_user_logged_out(100u));
    s.logged_out_users.clear();
}


// ===========================================================================
// friend_clear_logged_out() - basic semantics
// ===========================================================================

TEST(FriendManagerDataPlane, ClearRemovesSpecificUser) {
    auto& s = friend_manager_singleton();
    s.logged_out_users.clear();
    friend_user_logout(100u);
    friend_user_logout(200u);
    friend_user_logout(300u);
    friend_clear_logged_out(200u);
    EXPECT_TRUE(friend_is_user_logged_out(100u));
    EXPECT_FALSE(friend_is_user_logged_out(200u));
    EXPECT_TRUE(friend_is_user_logged_out(300u));
    EXPECT_EQ(s.logged_out_users.size(), 2u);
    s.logged_out_users.clear();
}

TEST(FriendManagerDataPlane, ClearUnknownUserIsNoop) {
    auto& s = friend_manager_singleton();
    s.logged_out_users.clear();
    friend_user_logout(100u);
    EXPECT_EQ(s.logged_out_users.size(), 1u);
    friend_clear_logged_out(99999u);
    EXPECT_EQ(s.logged_out_users.size(), 1u);
    EXPECT_TRUE(friend_is_user_logged_out(100u));
    s.logged_out_users.clear();
}

TEST(FriendManagerDataPlane, ClearAllUsers) {
    auto& s = friend_manager_singleton();
    s.logged_out_users.clear();
    friend_user_logout(1u);
    friend_user_logout(2u);
    friend_user_logout(3u);
    friend_user_logout(4u);
    friend_user_logout(5u);
    EXPECT_EQ(s.logged_out_users.size(), 5u);
    s.logged_out_users.clear();
    EXPECT_EQ(s.logged_out_users.size(), 0u);
    EXPECT_FALSE(friend_is_user_logged_out(1u));
    EXPECT_FALSE(friend_is_user_logged_out(5u));
}

TEST(FriendManagerDataPlane, ClearThenLogoutWorks) {
    auto& s = friend_manager_singleton();
    s.logged_out_users.clear();
    friend_user_logout(100u);
    friend_clear_logged_out(100u);
    friend_user_logout(100u);
    EXPECT_TRUE(friend_is_user_logged_out(100u));
    s.logged_out_users.clear();
}



// ===========================================================================
// Logout/clear cycles
// ===========================================================================

TEST(FriendManagerDataPlane, LogoutCheckClearCycle) {
    auto& s = friend_manager_singleton();
    s.logged_out_users.clear();
    EXPECT_FALSE(friend_is_user_logged_out(500u));
    friend_user_logout(500u);
    EXPECT_TRUE(friend_is_user_logged_out(500u));
    friend_clear_logged_out(500u);
    EXPECT_FALSE(friend_is_user_logged_out(500u));
    friend_user_logout(500u);
    EXPECT_TRUE(friend_is_user_logged_out(500u));
    s.logged_out_users.clear();
}

TEST(FriendManagerDataPlane, MultipleCyclesDoNotCorruptState) {
    auto& s = friend_manager_singleton();
    s.logged_out_users.clear();
    for (std::uint32_t i = 0; i < 10; ++i) {
        friend_user_logout(i);
        EXPECT_TRUE(friend_is_user_logged_out(i));
        friend_clear_logged_out(i);
        EXPECT_FALSE(friend_is_user_logged_out(i));
    }
    EXPECT_EQ(s.logged_out_users.size(), 0u);
}


// ===========================================================================
// Cross-user isolation
// ===========================================================================

TEST(FriendManagerDataPlane, LogoutOfOneUserDoesNotAffectOthers) {
    auto& s = friend_manager_singleton();
    s.logged_out_users.clear();
    friend_user_logout(1000u);
    friend_user_logout(2000u);
    friend_user_logout(3000u);
    friend_clear_logged_out(2000u);
    EXPECT_TRUE(friend_is_user_logged_out(1000u));
    EXPECT_FALSE(friend_is_user_logged_out(2000u));
    EXPECT_TRUE(friend_is_user_logged_out(3000u));
    s.logged_out_users.clear();
}

TEST(FriendManagerDataPlane, ManyDistinctUsers) {
    auto& s = friend_manager_singleton();
    s.logged_out_users.clear();
    for (std::uint32_t i = 100; i < 200; ++i) {
        friend_user_logout(i);
    }
    EXPECT_EQ(s.logged_out_users.size(), 100u);
    for (std::uint32_t i = 100; i < 200; ++i) {
        EXPECT_TRUE(friend_is_user_logged_out(i));
    }
    EXPECT_FALSE(friend_is_user_logged_out(99u));
    EXPECT_FALSE(friend_is_user_logged_out(200u));
    s.logged_out_users.clear();
}


// ===========================================================================
// State struct field types
// ===========================================================================

TEST(FriendManagerDataPlane, LoggedOutUsersIsUnorderedSet) {
    FriendManagerState s{};
    static_assert(std::is_same<decltype(s.logged_out_users),
                               std::unordered_set<std::uint32_t>>::value,
                  "logged_out_users must be unordered_set<uint32_t>");
    EXPECT_TRUE(true);
}


// ===========================================================================
// Boundary user IDs
// ===========================================================================

TEST(FriendManagerDataPlane, BoundaryUserIdsAreStored) {
    auto& s = friend_manager_singleton();
    s.logged_out_users.clear();
    friend_user_logout(0u);
    friend_user_logout(1u);
    friend_user_logout(0x7FFFFFFFu);
    friend_user_logout(0x80000000u);
    friend_user_logout(0xFFFFFFFEu);
    friend_user_logout(0xFFFFFFFFu);
    EXPECT_EQ(s.logged_out_users.size(), 6u);
    EXPECT_TRUE(friend_is_user_logged_out(0u));
    EXPECT_TRUE(friend_is_user_logged_out(0xFFFFFFFFu));
    s.logged_out_users.clear();
}


// ===========================================================================
// Mixed scenarios
// ===========================================================================

TEST(FriendManagerDataPlane, LogoutThenLogoutSameUserKeepsSingleEntry) {
    auto& s = friend_manager_singleton();
    s.logged_out_users.clear();
    friend_user_logout(42u);
    std::size_t size_after_first = s.logged_out_users.size();
    friend_user_logout(42u);
    friend_user_logout(42u);
    friend_user_logout(42u);
    EXPECT_EQ(s.logged_out_users.size(), size_after_first);
    EXPECT_EQ(size_after_first, 1u);
    s.logged_out_users.clear();
}

TEST(FriendManagerDataPlane, ClearBetweenLogoutsPreservesState) {
    auto& s = friend_manager_singleton();
    s.logged_out_users.clear();
    friend_user_logout(10u);
    friend_user_logout(20u);
    EXPECT_EQ(s.logged_out_users.size(), 2u);
    friend_clear_logged_out(10u);
    EXPECT_EQ(s.logged_out_users.size(), 1u);
    friend_user_logout(10u);
    EXPECT_EQ(s.logged_out_users.size(), 2u);
    s.logged_out_users.clear();
}

TEST(FriendManagerDataPlane, ClearAllAfterManyLogoutsLeavesEmpty) {
    auto& s = friend_manager_singleton();
    s.logged_out_users.clear();
    for (std::uint32_t i = 0; i < 1000; ++i) {
        friend_user_logout(i);
    }
    EXPECT_EQ(s.logged_out_users.size(), 1000u);
    s.logged_out_users.clear();
    EXPECT_EQ(s.logged_out_users.size(), 0u);
}
