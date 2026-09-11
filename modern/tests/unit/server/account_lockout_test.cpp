// Tests for the auto-lockout escalation policy. Pure DB; SQLite in-memory.
// The policy composes modern_login_audit (count) and modern_account_status
// (block write + modern_gm_audit), so the tests must declare the relevant
// schema fragments.

#include "mxh/db/db_adapter.hpp"
#include "mxh/server/account_lockout.hpp"
#include "mxh/server/account_moderation.hpp"
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
                       ("mxh_lockout_test_" +
                        std::to_string(MXH_TEST_GETPID()) + "_" +
                        std::to_string(counter.fetch_add(1)) + ".db"))
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
    EXPECT_TRUE(db->execute(
        "CREATE TABLE modern_account_status ("
        "account_id TEXT PRIMARY KEY, "
        "login_blocked INTEGER NOT NULL DEFAULT 0, "
        "reason TEXT NOT NULL DEFAULT '', "
        "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP)").ok());
    EXPECT_TRUE(db->execute(
        "CREATE TABLE modern_gm_audit ("
        "audit_id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "actor TEXT NOT NULL, "
        "target_account TEXT NOT NULL, "
        "action TEXT NOT NULL, "
        "reason TEXT NOT NULL DEFAULT '', "
        "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP)").ok());
    return db;
}

}  // namespace

TEST(AccountLockout, DisabledByZeroThreshold) {
    auto db = open_test_db();
    for (int i = 0; i < 50; ++i)
        record_login_audit(*db, "alice", "1.1.1.1", kLoginOutcomeBadPassword);
    EXPECT_EQ(evaluate_lockout(*db, "alice", /*threshold=*/0, /*window=*/60),
              "below_threshold");
}

TEST(AccountLockout, BelowThresholdReturnsBelowThreshold) {
    auto db = open_test_db();
    for (int i = 0; i < 3; ++i)
        record_login_audit(*db, "alice", "1.1.1.1", kLoginOutcomeBadPassword);
    EXPECT_EQ(evaluate_lockout(*db, "alice", /*threshold=*/5, /*window=*/60),
              "below_threshold");
}

TEST(AccountLockout, HitsThresholdBlocksAndAudits) {
    auto db = open_test_db();
    for (int i = 0; i < 5; ++i)
        record_login_audit(*db, "alice", "1.1.1.1", kLoginOutcomeBadPassword);
    EXPECT_EQ(evaluate_lockout(*db, "alice", /*threshold=*/5, /*window=*/60),
              "blocked");
    EXPECT_TRUE(is_account_login_blocked(*db, "alice"));
    // The lockout itself emits an audit row with outcome=blocked.
    EXPECT_GE(count_login_audit_failures(*db, "alice", 60), 5);
}

TEST(AccountLockout, AlreadyBlockedIsNotDoubleLogged) {
    auto db = open_test_db();
    for (int i = 0; i < 5; ++i)
        record_login_audit(*db, "alice", "1.1.1.1", kLoginOutcomeBadPassword);
    ASSERT_EQ(evaluate_lockout(*db, "alice", 5, 60), "blocked");
    const auto failures_before =
        count_login_audit_failures(*db, "alice", 60);
    // A second evaluation on the same account must NOT emit another
    // blocked row. The call returns already_blocked and is a no-op.
    EXPECT_EQ(evaluate_lockout(*db, "alice", 5, 60), "already_blocked");
    EXPECT_EQ(count_login_audit_failures(*db, "alice", 60), failures_before);
}

TEST(AccountLockout, ThrottledAndBadPasswordAggregate) {
    auto db = open_test_db();
    record_login_audit(*db, "bob", "1.1.1.2", kLoginOutcomeBadPassword);
    record_login_audit(*db, "bob", "1.1.1.2", kLoginOutcomeBadPassword);
    record_login_audit(*db, "bob", "1.1.1.2", kLoginOutcomeThrottled);
    record_login_audit(*db, "bob", "1.1.1.2", kLoginOutcomeBadPassword);
    record_login_audit(*db, "bob", "1.1.1.2", kLoginOutcomeBadPassword);
    // 5 failures mix bad_password + throttled; policy fires.
    EXPECT_EQ(evaluate_lockout(*db, "bob", 5, 60), "blocked");
}

TEST(AccountLockout, AcceptedOutcomesDoNotCount) {
    auto db = open_test_db();
    for (int i = 0; i < 5; ++i)
        record_login_audit(*db, "carol", "1.1.1.3", kLoginOutcomeAccepted);
    EXPECT_EQ(evaluate_lockout(*db, "carol", 5, 60), "below_threshold");
}

TEST(AccountLockout, EmptyAccountIsNoOp) {
    auto db = open_test_db();
    EXPECT_EQ(evaluate_lockout(*db, "", 1, 60), "below_threshold");
}