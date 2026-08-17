// M5.3: /api/auth/* route unit tests.
//
// Coverage split:
//   - Pure validation paths (no DB / no BCrypt): run unconditionally
//   - DB-backed paths (SqliteAdapter :memory:): run if mxh_db + mxh_server link
//   - BCrypt-backed paths (PBKDF2 register / verify): GTEST_SKIP with note
//     pointing at the Python integration test
//     (tests/unit/portal/portal_integration_test.py)

#include "portal/auth_routes.hpp"
#include "portal/jwt_token.hpp"

#include "mxh/db/db_adapter.hpp"
#include "mxh/db/sqlite_adapter.hpp"
#include "mxh/server/account_service.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <string>

using namespace mxh::portal;
using mxh::db::IDbAdapter;
using mxh::db::SqliteAdapter;

namespace {

// Minimal schema for the auth tables we touch.
// Mirrors the relevant subset of MoxianDbTool::moxian_schema_sql() so the
// tests are self-contained.
constexpr const char* kAuthSchema = R"SQL(
CREATE TABLE IF NOT EXISTS chr_log_info (
    id           TEXT PRIMARY KEY,
    pw           TEXT NOT NULL,
    userlevel    INTEGER NOT NULL DEFAULT 0,
    registerdate TEXT,
    lastlogindate TEXT,
    lastloginip  TEXT,
    usepoint     INTEGER NOT NULL DEFAULT 0
);
CREATE TABLE IF NOT EXISTS modern_account_identity (
    account_id TEXT PRIMARY KEY,
    user_idx INTEGER NOT NULL UNIQUE
);
CREATE TABLE IF NOT EXISTS modern_account_status (
    account_id TEXT PRIMARY KEY,
    login_blocked INTEGER NOT NULL DEFAULT 0,
    reason TEXT NOT NULL DEFAULT '',
    updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);
)SQL";

// Test fixture: in-memory SQLite with auth schema applied.
class AuthRoutesDbTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_ = std::make_unique<SqliteAdapter>();
        mxh::db::ConnectionConfig cfg;
        cfg.backend = "sqlite";
        cfg.path    = ":memory:";
        ASSERT_TRUE(db_->connect(cfg).ok());
        ASSERT_TRUE(db_->exec_multi(kAuthSchema).ok());
    }
    std::unique_ptr<IDbAdapter> db_;
};

}  // namespace

// ---------------------------------------------------------------------------
// Pure validation paths — no DB / no BCrypt required.
// ---------------------------------------------------------------------------

TEST(AuthRegisterValidation, NonObjectBodyReturns400) {
    auto db = std::make_unique<SqliteAdapter>();
    mxh::db::ConnectionConfig cfg;
    cfg.backend = "sqlite";
    cfg.path = ":memory:";
    ASSERT_TRUE(db->connect(cfg).ok());
    auto r = handle_register(*db, nlohmann::json::array());
    EXPECT_EQ(r.status_code, 400);
    EXPECT_FALSE(r.error.empty());
}

TEST(AuthRegisterValidation, MissingFieldsReturns400) {
    auto db = std::make_unique<SqliteAdapter>();
    mxh::db::ConnectionConfig cfg;
    cfg.backend = "sqlite";
    cfg.path = ":memory:";
    ASSERT_TRUE(db->connect(cfg).ok());
    nlohmann::json body = {{"account", "alice"}};
    auto r = handle_register(*db, body);
    EXPECT_EQ(r.status_code, 400);
    EXPECT_NE(r.error.find("password"), std::string::npos);
}

TEST(AuthRegisterValidation, PasswordMismatchReturns400) {
    auto db = std::make_unique<SqliteAdapter>();
    mxh::db::ConnectionConfig cfg;
    cfg.backend = "sqlite";
    cfg.path = ":memory:";
    ASSERT_TRUE(db->connect(cfg).ok());
    nlohmann::json body = {
        {"account", "alice"}, {"password", "secret123"}, {"confirm", "different"}};
    auto r = handle_register(*db, body);
    EXPECT_EQ(r.status_code, 400);
    EXPECT_NE(r.error.find("match"), std::string::npos);
}

TEST(AuthRegisterValidation, InvalidAccountNameReturns400) {
    auto db = std::make_unique<SqliteAdapter>();
    mxh::db::ConnectionConfig cfg;
    cfg.backend = "sqlite";
    cfg.path = ":memory:";
    ASSERT_TRUE(db->connect(cfg).ok());
    nlohmann::json body = {
        {"account", "ab"},               // too short
        {"password", "secret123"},
        {"confirm",  "secret123"}};
    auto r = handle_register(*db, body);
    EXPECT_EQ(r.status_code, 400);
    EXPECT_FALSE(r.error.empty());
}

TEST(AuthRegisterValidation, WeakPasswordReturns400) {
    auto db = std::make_unique<SqliteAdapter>();
    mxh::db::ConnectionConfig cfg;
    cfg.backend = "sqlite";
    cfg.path = ":memory:";
    ASSERT_TRUE(db->connect(cfg).ok());
    nlohmann::json body = {
        {"account", "alice"},
        {"password", "allletters"},      // no digit
        {"confirm",  "allletters"}};
    auto r = handle_register(*db, body);
    EXPECT_EQ(r.status_code, 400);
}

TEST(AuthLoginValidation, EmptySecretReturns500) {
    auto db = std::make_unique<SqliteAdapter>();
    mxh::db::ConnectionConfig cfg;
    cfg.backend = "sqlite";
    cfg.path = ":memory:";
    ASSERT_TRUE(db->connect(cfg).ok());
    nlohmann::json body = {{"account", "alice"}, {"password", "secret123"}};
    auto r = handle_login(*db, "", body);
    EXPECT_EQ(r.status_code, 500);
    EXPECT_NE(r.error.find("secret"), std::string::npos);
}

TEST(AuthLoginValidation, NonObjectBodyReturns401) {
    auto db = std::make_unique<SqliteAdapter>();
    mxh::db::ConnectionConfig cfg;
    cfg.backend = "sqlite";
    cfg.path = ":memory:";
    ASSERT_TRUE(db->connect(cfg).ok());
    auto r = handle_login(*db, "secret", nlohmann::json::array());
    EXPECT_EQ(r.status_code, 401);
}

TEST(AuthLoginValidation, MissingFieldsReturns401) {
    auto db = std::make_unique<SqliteAdapter>();
    mxh::db::ConnectionConfig cfg;
    cfg.backend = "sqlite";
    cfg.path = ":memory:";
    ASSERT_TRUE(db->connect(cfg).ok());
    nlohmann::json body = {{"account", "alice"}};
    auto r = handle_login(*db, "secret", body);
    EXPECT_EQ(r.status_code, 401);
}

TEST(AuthLogout, StatelessReturns204) {
    auto r = handle_logout();
    EXPECT_EQ(r.status_code, 204);
}

// ---------------------------------------------------------------------------
// DB-backed paths (SqliteAdapter :memory:).
// ---------------------------------------------------------------------------

TEST_F(AuthRoutesDbTest, RegisterDuplicateAccountReturns409) {
    nlohmann::json body = {
        {"account", "alice01"},
        {"password", "secret123"},
        {"confirm",  "secret123"}};
    auto first  = handle_register(*db_, body);
    EXPECT_EQ(first.status_code, 201);
    auto second = handle_register(*db_, body);
    EXPECT_EQ(second.status_code, 409);
    EXPECT_FALSE(second.error.empty());
}

TEST_F(AuthRoutesDbTest, LoginUnknownAccountReturns401) {
    auto r = handle_login(*db_, "secret",
        nlohmann::json{{"account", "ghost"}, {"password", "anything"}});
    EXPECT_EQ(r.status_code, 401);
}

TEST_F(AuthRoutesDbTest, MeUnknownAccountReturns404) {
    auto r = handle_me(*db_, "ghost");
    EXPECT_EQ(r.status_code, 404);
    EXPECT_EQ(r.body.value("error", ""), "account not found");
}

// ---------------------------------------------------------------------------
// BCrypt-required happy paths — skipped in unit tests, covered by integration.
// ---------------------------------------------------------------------------

TEST_F(AuthRoutesDbTest, RegisterValidAccountReturns201WithUserIdx) {
    auto r = handle_register(*db_, nlohmann::json{
        {"account", "alice01"},
        {"password", "secret123"},
        {"confirm",  "secret123"}});
    EXPECT_EQ(r.status_code, 201);
    EXPECT_GT(r.user_idx, 0u);
    EXPECT_EQ(r.account, "alice01");
}

TEST_F(AuthRoutesDbTest, LoginValidCredentialsReturnsTokenAndUserIdx) {
    ASSERT_EQ(handle_register(*db_, nlohmann::json{
        {"account", "alice01"},
        {"password", "secret123"},
        {"confirm",  "secret123"}}).status_code, 201);
    auto r = handle_login(*db_, "test-secret-32bytes-long-key!!!",
        nlohmann::json{{"account", "alice01"}, {"password", "secret123"}});
    EXPECT_EQ(r.status_code, 200);
    EXPECT_FALSE(r.token.empty());
    // Three JWT segments.
    auto first_dot = r.token.find('.');
    EXPECT_NE(first_dot, std::string::npos);
    EXPECT_NE(r.token.find('.', first_dot + 1), std::string::npos);
    EXPECT_GT(r.user_idx, 0u);
}

TEST_F(AuthRoutesDbTest, MeAfterLoginReturnsAccountInfo) {
    ASSERT_EQ(handle_register(*db_, nlohmann::json{
        {"account", "alice01"},
        {"password", "secret123"},
        {"confirm",  "secret123"}}).status_code, 201);
    auto r = handle_me(*db_, "alice01");
    EXPECT_EQ(r.status_code, 200);
    EXPECT_EQ(r.body.value("account", ""), "alice01");
}
