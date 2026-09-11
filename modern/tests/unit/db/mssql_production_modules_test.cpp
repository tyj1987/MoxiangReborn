// Modern-schema MSSQL round-trip for the M4 production modules:
//   - PooledDbAdapter (multi-slot acquire/release/transaction)
//   - record_login_audit + count_login_audit_failures
//   - evaluate_lockout escalation
//
// Same env-var gating as the legacy mssql_real_e2e tests:
//   set MXH_MSSQL_PROD_E2E=backend=mssql_odbc;host=(local);database=mxh_test;...

#include "mxh/db/db_adapter.hpp"
#include "mxh/db/pooled_db_adapter.hpp"
#include "mxh/server/account_lockout.hpp"
#include "mxh/server/login_audit.hpp"

#include <gtest/gtest.h>
#include <cstdlib>
#include <string>

using namespace mxh::db;
using namespace mxh::server;
namespace {

bool env_ready() {
    const char* raw = std::getenv("MXH_MSSQL_PROD_E2E");
    return raw && *raw;
}

void apply_modern_schema(IDbAdapter& db) {
    // The kMssqlSchema constexpr lives in modern/src/schema_migration.cpp.
    // For a unit test we inline the two tables this test exercises; the
    // schema_migration constexpr itself is covered by the SQLite branch
    // in schema_migration_test.cpp.
    ASSERT_TRUE(db.execute(
        "IF OBJECT_ID(N'dbo.modern_player_state', N'U') IS NULL "
        "CREATE TABLE dbo.modern_player_state (player_id BIGINT NOT NULL PRIMARY KEY, "
        "money BIGINT NOT NULL DEFAULT 0)").ok());
    ASSERT_TRUE(db.execute(
        "IF OBJECT_ID(N'dbo.modern_login_audit', N'U') IS NULL "
        "CREATE TABLE dbo.modern_login_audit ("
        "audit_id BIGINT IDENTITY(1,1) NOT NULL PRIMARY KEY, "
        "account_id NVARCHAR(50) NOT NULL DEFAULT N'', "
        "remote_addr NVARCHAR(64) NOT NULL DEFAULT N'', "
        "outcome NVARCHAR(32) NOT NULL, "
        "detail NVARCHAR(256) NOT NULL DEFAULT N'', "
        "created_at DATETIME2 NOT NULL DEFAULT SYSUTCDATETIME())").ok());
    ASSERT_TRUE(db.execute(
        "IF OBJECT_ID(N'dbo.modern_account_status', N'U') IS NULL "
        "CREATE TABLE dbo.modern_account_status ("
        "account_id NVARCHAR(50) NOT NULL PRIMARY KEY, "
        "login_blocked INT NOT NULL DEFAULT 0, "
        "reason NVARCHAR(256) NOT NULL DEFAULT N'', "
        "updated_at DATETIME2 NOT NULL DEFAULT SYSUTCDATETIME())").ok());
    ASSERT_TRUE(db.execute(
        "IF OBJECT_ID(N'dbo.modern_gm_audit', N'U') IS NULL "
        "CREATE TABLE dbo.modern_gm_audit ("
        "audit_id BIGINT IDENTITY(1,1) NOT NULL PRIMARY KEY, "
        "actor NVARCHAR(64) NOT NULL, "
        "target_account NVARCHAR(50) NOT NULL, "
        "action NVARCHAR(32) NOT NULL, "
        "reason NVARCHAR(256) NOT NULL DEFAULT N'', "
        "created_at DATETIME2 NOT NULL DEFAULT SYSUTCDATETIME())").ok());
}

}  // namespace

TEST(MssqlProdE2E, PooledDbAdapterMultiSlotRoundTrip) {
    if (!env_ready()) GTEST_SKIP() << "set MXH_MSSQL_PROD_E2E=backend=mssql_odbc;host=(local);database=mxh_test";
    auto cfg = ConnectionConfig::from_kv_string(std::getenv("MXH_MSSQL_PROD_E2E"));
    cfg.busy_timeout_ms = 5000;
    auto single = make_adapter(cfg.backend);
    ASSERT_TRUE(single->connect(cfg).ok());
    apply_modern_schema(*single);

    PooledDbAdapter::Config pcfg;
    pcfg.backend = cfg.backend;
    pcfg.connection = cfg;
    pcfg.pool_size = 3;
    auto pool = PooledDbAdapter::create(pcfg);
    ASSERT_TRUE(pool->is_connected());

    ResultSet rs;
    ASSERT_TRUE(pool->query("SELECT 1 + 1 AS two", {}, rs).ok());
    EXPECT_EQ(std::get<std::int64_t>(rs.rows[0][0]), 2);

    // Two leases acquired concurrently, one per slot, then released.
    auto a = pool->acquire_lease();
    auto b = pool->acquire_lease();
    ASSERT_TRUE(a);
    ASSERT_TRUE(b);
    const std::size_t a_slot = a.slot();
    const std::size_t b_slot = b.slot();
    EXPECT_NE(a_slot, b_slot);
    EXPECT_EQ(pool->stats().in_use, 2u);
    a.release();
    EXPECT_EQ(pool->stats().in_use, 1u);
    b.release();
    EXPECT_EQ(pool->stats().in_use, 0u);
    pool->disconnect();
}

TEST(MssqlProdE2E, LoginAuditAndLockoutEscalateAgainstMssql) {
    if (!env_ready()) GTEST_SKIP() << "set MXH_MSSQL_PROD_E2E=backend=mssql_odbc;host=(local);database=mxh_test";
    auto cfg = ConnectionConfig::from_kv_string(std::getenv("MXH_MSSQL_PROD_E2E"));
    cfg.busy_timeout_ms = 5000;
    auto db = make_adapter(cfg.backend);
    ASSERT_TRUE(db->connect(cfg).ok());
    apply_modern_schema(*db);
    // Make the lockout test isolated: clean any previous escalation row
    // for the synthetic account.
    const std::vector<Bind> delete_status_args{
        Bind{Value{std::string("mssql_lockout")}}};
    ASSERT_TRUE(db->execute(
        "DELETE FROM dbo.modern_account_status WHERE account_id = ?",
        std::span<const Bind>(delete_status_args)).ok());

    for (int i = 0; i < 5; ++i) {
        record_login_audit(*db, "mssql_lockout", "10.0.0.42",
                           kLoginOutcomeBadPassword);
    }
    EXPECT_EQ(count_login_audit_failures(*db, "mssql_lockout", 60), 5);

    const auto reason = evaluate_lockout(*db, "mssql_lockout",
                                          /*threshold=*/5, /*window=*/60);
    EXPECT_EQ(reason, "blocked");

    // modern_account_status row reflects the block; modern_login_audit has
    // an additional "blocked" row from the policy itself.
    ResultSet st;
    const std::vector<Bind> select_status_args{
        Bind{Value{std::string("mssql_lockout")}}};
    ASSERT_TRUE(db->query(
        "SELECT login_blocked FROM dbo.modern_account_status WHERE account_id = ?",
        std::span<const Bind>(select_status_args), st).ok());
    ASSERT_FALSE(st.empty());
    EXPECT_EQ(std::get<std::int64_t>(st.rows[0][0]), 1);
    EXPECT_GE(count_login_audit_failures(*db, "mssql_lockout", 60), 6);

    // A second call is a no-op (already_blocked).
    EXPECT_EQ(evaluate_lockout(*db, "mssql_lockout", 5, 60), "already_blocked");

    // Clean up so the test is repeatable.
    {
        const std::vector<Bind> cleanup_args{Bind{Value{std::string("mssql_lockout")}}};
        ASSERT_TRUE(db->execute(
            "DELETE FROM dbo.modern_account_status WHERE account_id = ?",
            std::span<const Bind>(cleanup_args)).ok());
        ASSERT_TRUE(db->execute(
            "DELETE FROM dbo.modern_login_audit WHERE account_id = ?",
            std::span<const Bind>(cleanup_args)).ok());
    }
    db->disconnect();
}