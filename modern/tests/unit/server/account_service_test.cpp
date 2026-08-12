#include "mxh/server/account_service.hpp"
#include "mxh/db/sqlite_adapter.hpp"

#include <gtest/gtest.h>

using namespace mxh::server;

TEST(AccountValidation, EnforcesLegacyWireLengthAndSafeAlphabet) {
    EXPECT_TRUE(valid_account_name("player_01"));
    EXPECT_FALSE(valid_account_name("ab"));
    EXPECT_FALSE(valid_account_name("name-with-dash"));
    EXPECT_FALSE(valid_account_name("12345678901234567"));
}

TEST(AccountValidation, PasswordRequiresLetterDigitAndPrintableAscii) {
    EXPECT_TRUE(valid_account_password("Sword123"));
    EXPECT_FALSE(valid_account_password("short1"));
    EXPECT_FALSE(valid_account_password("allletters"));
    EXPECT_FALSE(valid_account_password("12345678"));
}

TEST(AccountPassword, Pbkdf2RoundTripAndLegacyCompatibility) {
    const auto encoded = hash_account_password("Sword123");
    ASSERT_FALSE(encoded.empty());
    EXPECT_EQ(encoded.find("pbkdf2-sha256$210000$"), 0u);
    EXPECT_TRUE(verify_account_password("Sword123", encoded));
    EXPECT_FALSE(verify_account_password("Sword124", encoded));
    EXPECT_TRUE(verify_account_password("legacy", "legacy"));
}

TEST(AccountService, CreatesHashedAccountAndRejectsDuplicate) {
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg;
    cfg.backend = "sqlite";
    cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());
    ASSERT_TRUE(db.execute(
        "CREATE TABLE chr_log_info (id TEXT PRIMARY KEY, pw TEXT NOT NULL, "
        "userlevel INTEGER NOT NULL DEFAULT 0, registerdate TEXT, usepoint INTEGER NOT NULL DEFAULT 0)", {}).ok());
    const auto created = create_account(db, "player_01", "Sword123");
    ASSERT_TRUE(created.ok()) << created.message;
    mxh::db::ResultSet rows;
    const std::vector<mxh::db::Bind> args{mxh::db::bind(std::string("player_01"))};
    ASSERT_TRUE(db.query("SELECT pw FROM chr_log_info WHERE id = ?", args, rows).ok());
    ASSERT_EQ(rows.size(), 1u);
    const auto& stored = std::get<std::string>(rows.rows[0][0]);
    EXPECT_NE(stored, "Sword123");
    EXPECT_TRUE(verify_account_password("Sword123", stored));
    EXPECT_EQ(create_account(db, "player_01", "Sword123").status,
              AccountCreateStatus::AlreadyExists);
}
