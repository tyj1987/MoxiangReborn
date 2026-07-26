// user_table_test.cpp - Phase 6.3 UserTable 1:1 port tests.
//
// Locks the byte-level shape and lifecycle behavior of the legacy
// [Server]Agent/UserTable.h + UserTable.cpp state. Each test names a
// concrete 1:1 invariant.

#include "mxh/server/user_table.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace {

using mxh::server::CharSelectInfo;
using mxh::server::DisconnectUserCountBroadcast;
using mxh::server::GameOption;
using mxh::server::MAX_CHARACTER_NUM;
using mxh::server::MAX_NAME_LENGTH;
using mxh::server::UserInfo;
using mxh::server::UserTable;
using mxh::server::add_user;
using mxh::server::can_send_to_user;
using mxh::server::disconnect_clear_indexes;
using mxh::server::disconnect_user_count_broadcast;
using mxh::server::eUSERLEVEL_GM;
using mxh::server::find_user;
using mxh::server::get_add_count;
using mxh::server::get_remove_count;
using mxh::server::get_user_count;
using mxh::server::get_user_max_count;
using mxh::server::make_user_table;
using mxh::server::remove_all_user;
using mxh::server::remove_user;
using mxh::server::set_calc_max_count;
using mxh::server::user_table_init;

UserInfo make_user(std::uint32_t conn_idx, std::uint32_t char_id, std::uint32_t user_id) {
    UserInfo u;
    u.dwConnectionIndex = conn_idx;
    u.dwCharacterID     = char_id;
    u.dwUserID          = user_id;
    u.dwUniqueConnectIdx = conn_idx ^ 0xA5A5A5A5u;
    return u;
}

} // namespace

// ---- Constants 1:1 ----

TEST(UserTableConstants, MaxNameLengthIs21WithNul) {
    EXPECT_EQ(MAX_NAME_LENGTH, 20u);
}

TEST(UserTableConstants, MaxCharacterNumIsFour) {
    EXPECT_EQ(MAX_CHARACTER_NUM, 4u);
}

TEST(UserTableConstants, GMUserLevelIsOne) {
    EXPECT_EQ(eUSERLEVEL_GM, 1u);
}

// ---- POD layout 1:1 ----

TEST(UserTablePOD, CharSelectInfoNameArrayHoldsNamePlusNul) {
    CharSelectInfo info;
    EXPECT_EQ(sizeof(info.CharacterName), MAX_NAME_LENGTH + 1u);
    EXPECT_EQ(info.dwCharacterID, 0u);
    EXPECT_EQ(info.Level, 0u);
    EXPECT_EQ(info.MapNum, 0u);
    EXPECT_EQ(info.Gender, 0u);
}

TEST(UserTablePOD, GameOptionDefaultsZero) {
    GameOption opt;
    EXPECT_EQ(opt.bNoFriend, 0u);
    EXPECT_EQ(opt.bNoWhisper, 0u);
}

TEST(UserTablePOD, UserInfoDefaultsAllZero) {
    UserInfo u;
    EXPECT_EQ(u.dwConnectionIndex, 0u);
    EXPECT_EQ(u.dwCharacterID, 0u);
    EXPECT_EQ(u.dwUserID, 0u);
    EXPECT_EQ(u.UserLevel, 0u);
    EXPECT_EQ(u.dwMapServerConnectionIndex, 0u);
    EXPECT_EQ(u.wUserMapNum, 0u);
    EXPECT_EQ(u.DistAuthKey, 0u);
    EXPECT_EQ(u.dwLastChatTime, 0u);
    EXPECT_EQ(u.wChannel, 0u);
    EXPECT_EQ(u.dwUniqueConnectIdx, 0u);
    EXPECT_EQ(u.dwLastConnectionCheckTime, 0u);
    EXPECT_FALSE(u.bConnectionCheckFailed);
    for (const auto& slot : u.SelectInfoArray) {
        EXPECT_EQ(slot.dwCharacterID, 0u);
    }
}

TEST(UserTablePOD, UserInfoHoldsFourCharacterSlots) {
    UserInfo u;
    EXPECT_EQ(sizeof(u.SelectInfoArray) / sizeof(u.SelectInfoArray[0]),
              MAX_CHARACTER_NUM);
}

// ---- Lifecycle ----

TEST(UserTableInit, MakeUserTableIsZero) {
    auto t = make_user_table();
    EXPECT_EQ(get_user_count(t), 0u);
    EXPECT_EQ(get_user_max_count(t), 0u);
    EXPECT_EQ(get_add_count(t), 0u);
    EXPECT_EQ(get_remove_count(t), 0u);
    EXPECT_TRUE(t.m_Table.empty());
}

TEST(UserTableInit, UserTableInitResetsCountersAfterUse) {
    auto t = make_user_table();
    UserInfo u = make_user(1u, 11u, 21u);
    ASSERT_TRUE(add_user(t, 1u, u));
    ASSERT_EQ(get_user_count(t), 1u);
    ASSERT_EQ(get_add_count(t), 1u);

    user_table_init(t);

    EXPECT_EQ(get_user_count(t), 0u);
    EXPECT_EQ(get_user_max_count(t), 0u);
    EXPECT_EQ(get_add_count(t), 0u);
    EXPECT_EQ(get_remove_count(t), 0u);
    EXPECT_TRUE(t.m_Table.empty());
}

TEST(UserTableInit, UserTableInitAcceptsBucketHint) {
    auto t = make_user_table();
    user_table_init(t, 64u);
    EXPECT_TRUE(t.m_Table.empty());
}

// ---- Add / find ----

TEST(UserTableAdd, NewKeyIncrementsCountAndAddCounter) {
    auto t = make_user_table();
    UserInfo u = make_user(1u, 11u, 21u);
    ASSERT_TRUE(add_user(t, 1u, u));
    EXPECT_EQ(get_user_count(t), 1u);
    EXPECT_EQ(get_add_count(t), 1u);
    EXPECT_EQ(get_remove_count(t), 0u);
    EXPECT_EQ(t.m_Table.size(), 1u);
}

TEST(UserTableAdd, FindUserReturnsSlot) {
    auto t = make_user_table();
    UserInfo u = make_user(1u, 11u, 21u);
    ASSERT_TRUE(add_user(t, 1u, u));

    const UserInfo* found = find_user(t, 1u);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->dwCharacterID, 11u);
    EXPECT_EQ(found->dwUserID,      21u);
}

TEST(UserTableAdd, FindUserMissingReturnsNull) {
    auto t = make_user_table();
    EXPECT_EQ(find_user(t, 42u), nullptr);
    const auto& ct = t;
    EXPECT_EQ(find_user(ct, 42u), nullptr);
}

TEST(UserTableAdd, DuplicateKeyRejectedAndDoesNotDoubleCount) {
    auto t = make_user_table();
    UserInfo u = make_user(1u, 11u, 21u);
    ASSERT_TRUE(add_user(t, 1u, u));
    // Legacy AddUser asserts !FindUser; modern returns false on duplicate.
    EXPECT_FALSE(add_user(t, 1u, u));
    EXPECT_EQ(get_user_count(t), 1u);
    EXPECT_EQ(get_add_count(t), 1u);
}

TEST(UserTableAdd, MultipleUsersTracked) {
    auto t = make_user_table();
    for (std::uint32_t i = 0; i < 8u; ++i) {
        ASSERT_TRUE(add_user(t, 100u + i, make_user(100u + i, 200u + i, 300u + i)));
    }
    EXPECT_EQ(get_user_count(t), 8u);
    EXPECT_EQ(get_add_count(t), 8u);
    EXPECT_EQ(t.m_Table.size(), 8u);
}

// ---- Remove ----

TEST(UserTableRemove, RemovesAndReturnsSlot) {
    auto t = make_user_table();
    UserInfo u = make_user(5u, 55u, 65u);
    ASSERT_TRUE(add_user(t, 5u, u));

    auto popped = remove_user(t, 5u);
    ASSERT_TRUE(popped.has_value());
    EXPECT_EQ(popped->dwCharacterID, 55u);
    EXPECT_EQ(get_user_count(t), 0u);
    EXPECT_EQ(get_remove_count(t), 1u);
    EXPECT_EQ(find_user(t, 5u), nullptr);
}

TEST(UserTableRemove, MissingKeyDoesNotChangeCounts) {
    // Modern deliberately diverges from legacy here: legacy RemoveUser
    // unconditionally --m_dwUserCount and ++m_removeCount even when the
    // key is absent, which causes DWORD underflow when the table is
    // empty. Modern saturates the count at 0 and does not bump
    // m_removeCount for a no-op remove, so callers can defensively call
    // remove_user without pre-checking find_user.
    auto t = make_user_table();
    auto popped = remove_user(t, 999u);
    EXPECT_FALSE(popped.has_value());
    EXPECT_EQ(get_user_count(t), 0u);
    EXPECT_EQ(get_remove_count(t), 0u);
}

TEST(UserTableRemove, CountSaturatesAtZero) {
    auto t = make_user_table();
    UserInfo u = make_user(1u, 11u, 21u);
    ASSERT_TRUE(add_user(t, 1u, u));
    ASSERT_TRUE(remove_user(t, 1u).has_value());
    EXPECT_EQ(get_user_count(t), 0u);
    // Removing a missing key must not push count negative.
    EXPECT_FALSE(remove_user(t, 1u).has_value());
    EXPECT_EQ(get_user_count(t), 0u);
}

TEST(UserTableRemove, RemoveAllUserClearsTable) {
    auto t = make_user_table();
    for (std::uint32_t i = 0; i < 5u; ++i) {
        ASSERT_TRUE(add_user(t, i, make_user(i, 100u + i, 200u + i)));
    }
    EXPECT_EQ(get_user_count(t), 5u);

    remove_all_user(t);

    EXPECT_EQ(get_user_count(t), 0u);
    EXPECT_TRUE(t.m_Table.empty());
    // legacy does not touch addCount / removeCount / MaxUserCount here.
    EXPECT_EQ(get_add_count(t), 5u);
    EXPECT_EQ(get_remove_count(t), 0u);
}

TEST(UserTableRemove, AddRemoveAddReusesSameKey) {
    auto t = make_user_table();
    ASSERT_TRUE(add_user(t, 7u, make_user(7u, 70u, 80u)));
    ASSERT_TRUE(remove_user(t, 7u).has_value());
    ASSERT_TRUE(add_user(t, 7u, make_user(7u, 71u, 81u)));

    const UserInfo* found = find_user(t, 7u);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->dwCharacterID, 71u);
    EXPECT_EQ(get_user_count(t), 1u);
    EXPECT_EQ(get_add_count(t), 2u);
    EXPECT_EQ(get_remove_count(t), 1u);
}

// ---- MaxUserCount / SetCalcMaxCount ----

TEST(UserTableMax, SetCalcMaxCountOnlyRaises) {
    auto t = make_user_table();
    set_calc_max_count(t, 10u);
    EXPECT_EQ(get_user_max_count(t), 10u);

    set_calc_max_count(t, 7u);   // lower: should be ignored
    EXPECT_EQ(get_user_max_count(t), 10u);

    set_calc_max_count(t, 25u);  // higher: should win
    EXPECT_EQ(get_user_max_count(t), 25u);
}

TEST(UserTableMax, SetCalcMaxCountStartsAtZero) {
    auto t = make_user_table();
    EXPECT_EQ(get_user_max_count(t), 0u);
    set_calc_max_count(t, 0u);
    EXPECT_EQ(get_user_max_count(t), 0u);
}

// ---- SendToUser pre-check ----

TEST(UserTableSend, CanSendToUserRequiresKeyAndUniqueIdx) {
    auto t = make_user_table();
    UserInfo u = make_user(2u, 22u, 32u);
    u.dwUniqueConnectIdx = 0xCAFEu;
    ASSERT_TRUE(add_user(t, 2u, u));

    EXPECT_TRUE(can_send_to_user(t, 2u, 0xCAFEu));
    EXPECT_FALSE(can_send_to_user(t, 2u, 0xDEADu));
    EXPECT_FALSE(can_send_to_user(t, 999u, 0xCAFEu));
}

TEST(UserTableSend, CanSendToUserRejectsMissingKey) {
    auto t = make_user_table();
    EXPECT_FALSE(can_send_to_user(t, 1u, 0u));
}

// ---- Disconnect helpers ----

TEST(UserTableDisconnect, ClearIndexesRemovesBothWhenPresent) {
    std::unordered_map<std::uint32_t, UserInfo> by_oid;
    std::unordered_map<std::uint32_t, UserInfo> by_uid;
    by_oid.emplace(11u, UserInfo{});
    by_uid.emplace(21u, UserInfo{});

    disconnect_clear_indexes(11u, 21u, by_oid, by_uid);

    EXPECT_TRUE(by_oid.empty());
    EXPECT_TRUE(by_uid.empty());
}

TEST(UserTableDisconnect, ClearIndexesNoOpWhenZeroIds) {
    std::unordered_map<std::uint32_t, UserInfo> by_oid;
    std::unordered_map<std::uint32_t, UserInfo> by_uid;
    by_oid.emplace(11u, UserInfo{});
    by_uid.emplace(21u, UserInfo{});

    disconnect_clear_indexes(0u, 0u, by_oid, by_uid);

    EXPECT_EQ(by_oid.size(), 1u);
    EXPECT_EQ(by_uid.size(), 1u);
}

TEST(UserTableDisconnect, ClearIndexesKeepsUnrelatedEntries) {
    std::unordered_map<std::uint32_t, UserInfo> by_oid;
    std::unordered_map<std::uint32_t, UserInfo> by_uid;
    by_oid.emplace(11u, UserInfo{});
    by_oid.emplace(99u, UserInfo{});
    by_uid.emplace(21u, UserInfo{});
    by_uid.emplace(77u, UserInfo{});

    disconnect_clear_indexes(11u, 21u, by_oid, by_uid);

    EXPECT_EQ(by_oid.size(), 1u);
    EXPECT_EQ(by_oid.count(99u), 1u);
    EXPECT_EQ(by_uid.size(), 1u);
    EXPECT_EQ(by_uid.count(77u), 1u);
}

TEST(UserTableDisconnect, UserCountBroadcastClampsAndEncodes) {
    auto b1 = disconnect_user_count_broadcast(4010u, 100u);
    EXPECT_EQ(b1.port, 4010u);
    EXPECT_EQ(b1.count, 100u);

    auto b2 = disconnect_user_count_broadcast(4011u, 0x10000u);
    EXPECT_EQ(b2.port, 4011u);
    EXPECT_EQ(b2.count, 0xFFFFu);
}

TEST(UserTableDisconnect, UserCountBroadcastZeroUserCount) {
    auto b = disconnect_user_count_broadcast(4010u, 0u);
    EXPECT_EQ(b.port, 4010u);
    EXPECT_EQ(b.count, 0u);
}

