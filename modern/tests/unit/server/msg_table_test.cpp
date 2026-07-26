// msg_table_test.cpp - Phase 6.3 MsgTable 1:1 port tests.

#include "mxh/server/msg_table.hpp"

#include <gtest/gtest.h>

#include <cstring>

namespace {

using mxh::server::MAX_CHAT_LENGTH;
using mxh::server::MAX_MSGTABLE;
using mxh::server::MAX_NAME_LENGTH;
using mxh::server::MsgChat;
using mxh::server::MsgTable;
using mxh::server::TestMsg;
using mxh::server::add_msg;
using mxh::server::get_msg;
using mxh::server::make_msg_table;
using mxh::server::msg_table_clear;
using mxh::server::msg_table_size;
using mxh::server::remove_msg;

} // namespace

// ---- Constants 1:1 ----

TEST(MsgTableConstants, MaxChatLengthIs127) {
    EXPECT_EQ(MAX_CHAT_LENGTH, 127u);
}

TEST(MsgTableConstants, MaxNameLengthIs16) {
    EXPECT_EQ(MAX_NAME_LENGTH, 16u);
}

TEST(MsgTableConstants, MaxMsgTableIs500) {
    EXPECT_EQ(MAX_MSGTABLE, 500u);
}

// ---- POD 1:1 ----

TEST(MsgTablePOD, MsgChatDefaultsAreZero) {
    MsgChat m;
    EXPECT_EQ(m.Category, 0);
    EXPECT_EQ(m.Protocol, 0);
    EXPECT_EQ(m.dwObjectID, 0u);
    for (char c : m.Name) EXPECT_EQ(c, 0);
    for (char c : m.Msg)  EXPECT_EQ(c, 0);
    EXPECT_EQ(sizeof(m.Name), MAX_NAME_LENGTH + 1u);
    EXPECT_EQ(sizeof(m.Msg),  MAX_CHAT_LENGTH + 1u);
}

TEST(MsgTablePOD, TestMsgDefaultsAreZero) {
    TestMsg m;
    EXPECT_EQ(m.dwObjectID, 0u);
    for (char c : m.Msg) EXPECT_EQ(c, 0);
    EXPECT_EQ(sizeof(m.Msg), MAX_CHAT_LENGTH + 1u);
}

// ---- Lifecycle ----

TEST(MsgTableInit, MakeIsEmpty) {
    auto t = make_msg_table();
    EXPECT_EQ(msg_table_size(t), 0u);
    EXPECT_TRUE(t.m_Table.empty());
}

TEST(MsgTableInit, ClearDropsEverything) {
    auto t = make_msg_table();
    MsgChat m; m.Category = 1; m.Protocol = 2;
    std::strncpy(m.Msg, "hello", MAX_CHAT_LENGTH);
    std::uint32_t key = 0;
    ASSERT_TRUE(add_msg(t, m, key));

    msg_table_clear(t);

    EXPECT_EQ(msg_table_size(t), 0u);
    EXPECT_EQ(get_msg(t, key), nullptr);
}

// ---- AddMsg(MSG_CHAT*) ----

TEST(MsgTableAdd, AddMsgChatReturnsTrueWithKey) {
    auto t = make_msg_table();
    MsgChat m;
    m.Category = 5;
    m.Protocol = 6;
    m.dwObjectID = 0xCAFEu;
    std::strncpy(m.Name, "Alice", MAX_NAME_LENGTH);
    std::strncpy(m.Msg,  "hello world", MAX_CHAT_LENGTH);

    std::uint32_t key = 0;
    EXPECT_TRUE(add_msg(t, m, key));
    EXPECT_NE(key, 0u);
    EXPECT_EQ(msg_table_size(t), 1u);
}

TEST(MsgTableAdd, GetMsgReturnsCopy) {
    auto t = make_msg_table();
    MsgChat m;
    m.Category   = 7;
    m.Protocol   = 8;
    m.dwObjectID = 0xBEEFu;
    std::strncpy(m.Name, "Bob", MAX_NAME_LENGTH);
    std::strncpy(m.Msg,  "test message", MAX_CHAT_LENGTH);

    std::uint32_t key = 0;
    ASSERT_TRUE(add_msg(t, m, key));

    const MsgChat* stored = get_msg(t, key);
    ASSERT_NE(stored, nullptr);
    EXPECT_EQ(stored->Category,   7);
    EXPECT_EQ(stored->Protocol,   8);
    EXPECT_EQ(stored->dwObjectID, 0xBEEFu);
    EXPECT_STREQ(stored->Name, "Bob");
    EXPECT_STREQ(stored->Msg,  "test message");
}

TEST(MsgTableAdd, GetMissingReturnsNull) {
    auto t = make_msg_table();
    EXPECT_EQ(get_msg(t, 999u), nullptr);
}

TEST(MsgTableAdd, KeysAreUnique) {
    auto t = make_msg_table();
    MsgChat m;
    m.Msg[0] = 0;
    std::uint32_t k1 = 0, k2 = 0, k3 = 0;
    EXPECT_TRUE(add_msg(t, m, k1));
    EXPECT_TRUE(add_msg(t, m, k2));
    EXPECT_TRUE(add_msg(t, m, k3));
    EXPECT_NE(k1, k2);
    EXPECT_NE(k2, k3);
    EXPECT_NE(k1, k3);
}

// ---- AddMsg(TESTMSG*) ----

TEST(MsgTableAddTestMsg, TestMsgLeavesNameEmpty) {
    auto t = make_msg_table();
    TestMsg src;
    src.Category = 1;
    src.Protocol = 2;
    src.dwObjectID = 0xDEADu;
    std::strncpy(src.Msg, "anon", MAX_CHAT_LENGTH);

    std::uint32_t key = 0;
    EXPECT_TRUE(add_msg(t, src, key));

    const MsgChat* stored = get_msg(t, key);
    ASSERT_NE(stored, nullptr);
    EXPECT_EQ(stored->Name[0], 0);  // legacy: TESTMSG path leaves Name empty.
    EXPECT_STREQ(stored->Msg, "anon");
    EXPECT_EQ(stored->dwObjectID, 0xDEADu);
}

// ---- RemoveMsg ----

TEST(MsgTableRemove, RemoveExistingReturnsTrue) {
    auto t = make_msg_table();
    MsgChat m;
    std::uint32_t key = 0;
    ASSERT_TRUE(add_msg(t, m, key));
    ASSERT_NE(get_msg(t, key), nullptr);

    EXPECT_TRUE(remove_msg(t, key));
    EXPECT_EQ(get_msg(t, key), nullptr);
    EXPECT_EQ(msg_table_size(t), 0u);
}

TEST(MsgTableRemove, RemoveMissingReturnsFalse) {
    auto t = make_msg_table();
    EXPECT_FALSE(remove_msg(t, 999u));
}

// ---- Capacity ----

TEST(MsgTableCapacity, CapacityRespected) {
    auto t = make_msg_table(/*capacity*/ 3);
    MsgChat m;
    std::uint32_t k;
    EXPECT_TRUE (add_msg(t, m, k));
    EXPECT_TRUE (add_msg(t, m, k));
    EXPECT_TRUE (add_msg(t, m, k));
    EXPECT_FALSE(add_msg(t, m, k));  // 4th insert is rejected.
    EXPECT_EQ(msg_table_size(t), 3u);
}

TEST(MsgTableCapacity, RemoveFreesSlot) {
    auto t = make_msg_table(/*capacity*/ 2);
    MsgChat m;
    std::uint32_t k1 = 0, k2 = 0;
    ASSERT_TRUE(add_msg(t, m, k1));
    ASSERT_TRUE(add_msg(t, m, k2));
    ASSERT_FALSE(add_msg(t, m, k1));  // capacity reached

    ASSERT_TRUE(remove_msg(t, k1));
    EXPECT_TRUE(add_msg(t, m, k1));  // now slot freed, new add succeeds.
}

TEST(MsgTableCapacity, DefaultCapacityIsMaxMsgTable) {
    auto t = make_msg_table();
    EXPECT_EQ(t.m_Capacity, MAX_MSGTABLE);
}

