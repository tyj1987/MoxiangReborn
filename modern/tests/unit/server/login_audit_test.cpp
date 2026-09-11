// Tests for modern_login_audit insert + counter aggregation.

#include "mxh/db/db_adapter.hpp"
#include "mxh/server/login_audit.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

#if defined(_WIN32)
#include <process.h>
#define MXH_TEST_GETPID() ::_getpid()
#else
#include <unistd.h>
#define MXH_TEST_GETPID() ::getpid()
#endif

using namespace mxh::db;
using namespace mxh::server;
namespace {

std::atomic<int> counter{0};
std::unique_ptr<IDbAdapter> open_test_db() {
    const auto path = (std::filesystem::temp_directory_path() /
                       ("mxh_audit_test_" + std::to_string(MXH_TEST_GETPID()) +
                        "_" + std::to_string(counter.fetch_add(1)) + ".db"))
                        .string();
    std::error_code ec;
    std::filesystem::remove(path, ec);
    ConnectionConfig cfg;
    cfg.backend = "sqlite";
    cfg.path = path;
    auto db = make_adapter("sqlite");
    EXPECT_TRUE(db->connect(cfg).ok());
    EXPECT_TRUE(db->execute(
        "CREATE TABLE modern_login_audit ("
        "audit_id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "account_id TEXT NOT NULL DEFAULT '', "
        "remote_addr TEXT NOT NULL DEFAULT '', "
        "outcome TEXT NOT NULL, "
        "detail TEXT NOT NULL DEFAULT '', "
        "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP)").ok());
    return db;
}
}  // namespace

TEST(LoginAudit, RecordAndQueryBackReturnsRow) {
    auto db = open_test_db();
    ASSERT_TRUE(record_login_audit(*db, "alice", "10.0.0.1",
                                   kLoginOutcomeBadPassword));
    ResultSet rs;
    ASSERT_TRUE(db->query(
        "SELECT account_id, remote_addr, outcome FROM modern_login_audit", {}, rs).ok());
    ASSERT_EQ(rs.rows.size(), 1u);
    EXPECT_EQ(std::get<std::string>(rs.rows[0][0]), "alice");
    EXPECT_EQ(std::get<std::string>(rs.rows[0][1]), "10.0.0.1");
    EXPECT_EQ(std::get<std::string>(rs.rows[0][2]), kLoginOutcomeBadPassword);
}

TEST(LoginAudit, EmptyAccountAndAddressAreAccepted) {
    auto db = open_test_db();
    ASSERT_TRUE(record_login_audit(*db, "", "", kLoginOutcomeRateLimited));
    ResultSet rs;
    ASSERT_TRUE(db->query("SELECT COUNT(*) FROM modern_login_audit", {}, rs).ok());
    EXPECT_EQ(std::get<std::int64_t>(rs.rows[0][0]), 1);
}

TEST(LoginAudit, CountFailuresAggregatesOutcomesWithinWindow) {
    auto db = open_test_db();
    record_login_audit(*db, "bob", "10.0.0.2", kLoginOutcomeBadPassword);
    record_login_audit(*db, "bob", "10.0.0.2", kLoginOutcomeBadPassword);
    record_login_audit(*db, "bob", "10.0.0.2", kLoginOutcomeThrottled);
    record_login_audit(*db, "bob", "10.0.0.2", kLoginOutcomeBlocked);
    record_login_audit(*db, "bob", "10.0.0.2", kLoginOutcomeAccepted);  // ignored
    record_login_audit(*db, "alice", "10.0.0.3", kLoginOutcomeBadPassword);  // different account
    EXPECT_EQ(count_login_audit_failures(*db, "bob", 60), 4);
    EXPECT_EQ(count_login_audit_failures(*db, "alice", 60), 1);
    EXPECT_EQ(count_login_audit_failures(*db, "nobody", 60), 0);
}

TEST(LoginAudit, CountFailuresRespectsWindowBoundary) {
    auto db = open_test_db();
    // Insert a row whose created_at is far in the past so the window
    // filter excludes it. The schema above does not allow setting
    // created_at directly, so we cheat by inserting a custom row.
    ASSERT_TRUE(db->execute(
        "INSERT INTO modern_login_audit "
        "(account_id, remote_addr, outcome, detail, created_at) "
        "VALUES ('eve', '10.0.0.4', 'bad_password', '', "
        "datetime('now', '-2 hours'))").ok());
    ASSERT_TRUE(record_login_audit(*db, "eve", "10.0.0.4",
                                   kLoginOutcomeBadPassword));
    // 1-minute window: only the just-inserted audit row counts.
    EXPECT_EQ(count_login_audit_failures(*db, "eve", 1), 1);
    // 24-hour window: both rows count.
    EXPECT_EQ(count_login_audit_failures(*db, "eve", 60 * 24), 2);
}

TEST(LoginAudit, DbFailureDoesNotThrow) {
    // Pass a disconnected adapter; record_login_audit must return false
    // but not throw. This is the production contract — losing an audit
    // row must not crash a login flow.
    auto db = make_adapter("sqlite");
    EXPECT_FALSE(record_login_audit(*db, "x", "y", kLoginOutcomeBadPassword));
}