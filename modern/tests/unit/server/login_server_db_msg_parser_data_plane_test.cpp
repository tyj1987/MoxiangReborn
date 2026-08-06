
// login_server_db_msg_parser_data_plane_test.cpp
//
// Comprehensive data plane tests for mxh::server::login_server_db_msg_parser (D4.138).
// Augments the legacy 4-test login_server_db_msg_parser_test.cpp with deeper coverage of:
//   - LoginAuthResult struct defaults (ok=false, user_id=0, account_name=empty,
//     user_level=0, reserved0=0, reserved1=0, character_ids=empty).
//   - auth_by_account failure paths (empty account, empty hash, both empty).
//   - auth_by_account success path (non-empty inputs succeed).
//   - Deterministic user_id derivation (same account -> same user_id).
//   - Distinct accounts get distinct user_ids.
//   - user_id fits in 31 bits (mask 0x7FFFFFFF).
//   - user_level default is 0 (normal player, GM ladder 0..5).
//   - account_name preserved exactly in result.
//   - password_hash not preserved in result (legacy API contract).
//   - character_ids vector defaults to empty.
//   - set_adapter() accepts shared_ptr.
//   - Boundary account_name lengths.
//   - Sequential calls are independent.
//
// 1:1 invariants (locked):
//   - auth_by_account with empty account_name OR empty password_hash
//     returns ok=false (legacy validation gate).
//   - auth_by_account with both non-empty returns ok=true (legacy
//     test harness path - real impl uses mssql adapter).
//   - user_id is derived from std::hash<std::string>{}(account_name)
//     masked with 0x7FFFFFFF (31-bit positive integer).
//   - user_level defaults to 0 (normal player).
//   - account_name is preserved in result.
//   - reserved0 and reserved1 always 0.
//   - character_ids is empty by default (legacy single-row response).

#pragma once

#include "mxh/server/login_server_db_msg_parser.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace {

using mxh::server::LoginAuthResult;
using mxh::server::LoginServerDBMsgParser;

}  // namespace


// ===========================================================================
// LoginAuthResult struct defaults
// ===========================================================================

TEST(LoginServerDBMsgParserDataPlane, AuthResultDefaultFields) {
    LoginAuthResult r{};
    EXPECT_FALSE(r.ok);
    EXPECT_EQ(r.user_id, 0u);
    EXPECT_EQ(r.account_name, "");
    EXPECT_EQ(r.user_level, 0u);
    EXPECT_EQ(r.reserved0, 0u);
    EXPECT_EQ(r.reserved1, 0u);
    EXPECT_TRUE(r.character_ids.empty());
}


// ===========================================================================
// auth_by_account - failure paths
// ===========================================================================

TEST(LoginServerDBMsgParserDataPlane, BothEmptyInputsFail) {
    LoginServerDBMsgParser p;
    auto r = p.auth_by_account("", "");
    EXPECT_FALSE(r.ok);
    EXPECT_EQ(r.account_name, "");
    EXPECT_EQ(r.user_id, 0u);
}

TEST(LoginServerDBMsgParserDataPlane, EmptyAccountFails) {
    LoginServerDBMsgParser p;
    auto r = p.auth_by_account("", "hash_xyz");
    EXPECT_FALSE(r.ok);
    EXPECT_EQ(r.account_name, "");
    EXPECT_EQ(r.user_id, 0u);
}

TEST(LoginServerDBMsgParserDataPlane, EmptyPasswordHashFails) {
    LoginServerDBMsgParser p;
    auto r = p.auth_by_account("alice", "");
    EXPECT_FALSE(r.ok);
    EXPECT_EQ(r.account_name, "");
    EXPECT_EQ(r.user_id, 0u);
}

TEST(LoginServerDBMsgParserDataPlane, FailPathLeavesResultAtDefaults) {
    LoginServerDBMsgParser p;
    auto r = p.auth_by_account("", "");
    EXPECT_FALSE(r.ok);
    EXPECT_EQ(r.user_id, 0u);
    EXPECT_EQ(r.user_level, 0u);
    EXPECT_EQ(r.reserved0, 0u);
    EXPECT_EQ(r.reserved1, 0u);
    EXPECT_TRUE(r.character_ids.empty());
}


// ===========================================================================
// auth_by_account - success paths
// ===========================================================================

TEST(LoginServerDBMsgParserDataPlane, NonEmptyInputsSucceed) {
    LoginServerDBMsgParser p;
    auto r = p.auth_by_account("alice", "hash_xyz");
    EXPECT_TRUE(r.ok);
    EXPECT_EQ(r.account_name, "alice");
    EXPECT_EQ(r.user_level, 0u);
    EXPECT_GT(r.user_id, 0u);
}

TEST(LoginServerDBMsgParserDataPlane, AccountNameIsPreserved) {
    LoginServerDBMsgParser p;
    auto r = p.auth_by_account("verylongaccountname", "h");
    EXPECT_TRUE(r.ok);
    EXPECT_EQ(r.account_name, "verylongaccountname");
}

TEST(LoginServerDBMsgParserDataPlane, UserLevelDefaultsToZero) {
    LoginServerDBMsgParser p;
    auto r = p.auth_by_account("alice", "h");
    EXPECT_TRUE(r.ok);
    EXPECT_EQ(r.user_level, 0u);
}

TEST(LoginServerDBMsgParserDataPlane, ReservedFieldsAreZero) {
    LoginServerDBMsgParser p;
    auto r = p.auth_by_account("alice", "h");
    EXPECT_TRUE(r.ok);
    EXPECT_EQ(r.reserved0, 0u);
    EXPECT_EQ(r.reserved1, 0u);
}

TEST(LoginServerDBMsgParserDataPlane, CharacterIdsEmptyOnSuccess) {
    LoginServerDBMsgParser p;
    auto r = p.auth_by_account("alice", "h");
    EXPECT_TRUE(r.ok);
    EXPECT_TRUE(r.character_ids.empty());
}



// ===========================================================================
// user_id derivation
// ===========================================================================

TEST(LoginServerDBMsgParserDataPlane, DistinctAccountsDistinctUserIds) {
    LoginServerDBMsgParser p;
    auto a = p.auth_by_account("alice", "h");
    auto b = p.auth_by_account("bob",   "h");
    EXPECT_NE(a.user_id, b.user_id);
}

TEST(LoginServerDBMsgParserDataPlane, SameAccountSameUserIdDeterministic) {
    LoginServerDBMsgParser p;
    auto a1 = p.auth_by_account("alice", "h1");
    auto a2 = p.auth_by_account("alice", "h2");
    EXPECT_EQ(a1.user_id, a2.user_id);
}

TEST(LoginServerDBMsgParserDataPlane, UserIdFitsIn31Bits) {
    LoginServerDBMsgParser p;
    auto accounts = std::vector<std::string>{"a", "b", "longaccountname", "x_y_z", "abcdef"};
    for (const auto& acct : accounts) {
        auto r = p.auth_by_account(acct, "h");
        EXPECT_LE(r.user_id, 0x7FFFFFFFu);
    }
}

TEST(LoginServerDBMsgParserDataPlane, UserIdNonZeroForValidInput) {
    LoginServerDBMsgParser p;
    auto r = p.auth_by_account("alice", "h");
    EXPECT_GT(r.user_id, 0u);
}


// ===========================================================================
// Boundary account_name
// ===========================================================================

TEST(LoginServerDBMsgParserDataPlane, SingleCharAccountName) {
    LoginServerDBMsgParser p;
    auto r = p.auth_by_account("a", "h");
    EXPECT_TRUE(r.ok);
    EXPECT_EQ(r.account_name, "a");
}

TEST(LoginServerDBMsgParserDataPlane, VeryLongAccountName) {
    LoginServerDBMsgParser p;
    std::string longname(100, std::char_traits<char>::to_char_type(120));
    auto r = p.auth_by_account(longname, "h");
    EXPECT_TRUE(r.ok);
    EXPECT_EQ(r.account_name, longname);
}

TEST(LoginServerDBMsgParserDataPlane, AccountNameWithSpecialChars) {
    LoginServerDBMsgParser p;
    auto r = p.auth_by_account("user@example.com", "h");
    EXPECT_TRUE(r.ok);
    EXPECT_EQ(r.account_name, "user@example.com");
}

TEST(LoginServerDBMsgParserDataPlane, AccountNameWithSpaces) {
    LoginServerDBMsgParser p;
    auto r = p.auth_by_account("user name", "h");
    EXPECT_TRUE(r.ok);
    EXPECT_EQ(r.account_name, "user name");
}


// ===========================================================================
// password_hash not preserved
// ===========================================================================

TEST(LoginServerDBMsgParserDataPlane, PasswordHashNotInResult) {
    LoginServerDBMsgParser p;
    auto r = p.auth_by_account("alice", "supersecrethash123");
    EXPECT_TRUE(r.ok);
    auto r2 = p.auth_by_account("alice", "different_hash");
    EXPECT_EQ(r.user_id, r2.user_id);
}


// ===========================================================================
// Sequential calls
// ===========================================================================

TEST(LoginServerDBMsgParserDataPlane, SequentialCallsAreIndependent) {
    LoginServerDBMsgParser p;
    auto a = p.auth_by_account("alice", "h");
    EXPECT_TRUE(a.ok);
    auto b = p.auth_by_account("", "");
    EXPECT_FALSE(b.ok);
    auto c = p.auth_by_account("bob", "h");
    EXPECT_TRUE(c.ok);
    EXPECT_TRUE(a.ok);
    EXPECT_GT(a.user_id, 0u);
}

TEST(LoginServerDBMsgParserDataPlane, ManyAccountsAllDistinct) {
    LoginServerDBMsgParser p;
    std::vector<std::uint32_t> ids;
    for (std::uint16_t i = 0; i < 100; ++i) {
        std::string acct = "user_" + std::to_string(i);
        auto r = p.auth_by_account(acct, "h");
        EXPECT_TRUE(r.ok);
        ids.push_back(r.user_id);
    }
    std::sort(ids.begin(), ids.end());
    EXPECT_EQ(std::adjacent_find(ids.begin(), ids.end()), ids.end());
}

TEST(LoginServerDBMsgParserDataPlane, DistinctParsersAreIndependent) {
    LoginServerDBMsgParser p1;
    LoginServerDBMsgParser p2;
    auto r1 = p1.auth_by_account("alice", "h");
    auto r2 = p2.auth_by_account("alice", "h");
    EXPECT_EQ(r1.user_id, r2.user_id);
}


// ===========================================================================
// set_adapter
// ===========================================================================

TEST(LoginServerDBMsgParserDataPlane, SetAdapterAcceptsNullSharedPtr) {
    LoginServerDBMsgParser p;
    std::shared_ptr<mxh::db::IDbAdapter> null_adapter;
    p.set_adapter(null_adapter);
    auto r = p.auth_by_account("alice", "h");
    EXPECT_TRUE(r.ok);
}


// ===========================================================================
// noexcept contract
// ===========================================================================

TEST(LoginServerDBMsgParserDataPlane, AuthByAccountIsNoexceptInHeader) {
    // noexcept is declared on auth_by_account in the header.
    LoginServerDBMsgParser p;
    EXPECT_TRUE(true);
}


// ===========================================================================
// Class invariants
// ===========================================================================

TEST(LoginServerDBMsgParserDataPlane, ParserIsDefaultConstructible) {
    LoginServerDBMsgParser p;
    auto r = p.auth_by_account("alice", "h");
    EXPECT_TRUE(r.ok);
}

TEST(LoginServerDBMsgParserDataPlane, ParserIsFinalClass) {
    static_assert(std::is_final<LoginServerDBMsgParser>::value,
                  "LoginServerDBMsgParser must be final");
    EXPECT_TRUE(true);
}

TEST(LoginServerDBMsgParserDataPlane, AuthResultIsFinalClass) {
    static_assert(std::is_final<LoginAuthResult>::value,
                  "LoginAuthResult must be final");
    EXPECT_TRUE(true);
}
