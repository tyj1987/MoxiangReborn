// login_server_db_msg_parser_test.cpp

#include "mxh/server/login_server_db_msg_parser.hpp"
#include <gtest/gtest.h>

namespace {
using mxh::server::LoginServerDBMsgParser;
using mxh::server::LoginAuthResult;
}

TEST(LoginServerDB, EmptyInputsFail) {
    LoginServerDBMsgParser p;
    auto r = p.auth_by_account("", "");
    EXPECT_FALSE(r.ok);
}

TEST(LoginServerDB, EmptyAccountFails) {
    LoginServerDBMsgParser p;
    auto r = p.auth_by_account("", "bar");
    EXPECT_FALSE(r.ok);
}

TEST(LoginServerDB, NonEmptyInputsSucceed) {
    LoginServerDBMsgParser p;
    auto r = p.auth_by_account("alice", "hash_xyz");
    EXPECT_TRUE(r.ok);
    EXPECT_EQ(r.account_name, "alice");
    EXPECT_EQ(r.user_level, 0u);
    EXPECT_GT(r.user_id, 0u);
}

TEST(LoginServerDB, DistinctAccountsDistinctUserIds) {
    LoginServerDBMsgParser p;
    auto a = p.auth_by_account("alice", "h");
    auto b = p.auth_by_account("bob",   "h");
    EXPECT_NE(a.user_id, b.user_id);
}
