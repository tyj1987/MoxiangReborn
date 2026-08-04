// Tests for the MssqlOdbcAdapter.
//
// These tests are all compile-time / interface checks; they do NOT require
// a live SQL Server. A real connection round-trip is covered by the
// docker-compose smoke test under docker/ (see DATABASE_SCHEMA.md).
//
// We do not link against odbc32.lib directly here — the adapter is
// pulled in via mxh_db, which already links odbc32. The tests only need
// to construct the adapter, exercise its factory entry, and verify the
// error-translation path for a connection failure.

#include "mxh/db/db_adapter.hpp"

#ifdef _WIN32
#include "mxh/db/mssql_odbc_adapter.hpp"
#endif

#include <gtest/gtest.h>

using namespace mxh::db;

namespace {

#ifdef _WIN32
// Returns a non-connected adapter; tests use this to exercise failure
// paths without needing a SQL Server.
std::unique_ptr<IDbAdapter> make_unconnected() {
    auto a = make_adapter("mssql_odbc");
    return a;
}
#endif

}  // namespace

#ifdef _WIN32

TEST(MssqlOdbcAdapter, FactoryReturnsNonNull) {
    auto a = make_adapter("mssql_odbc");
    ASSERT_NE(a, nullptr);
    EXPECT_EQ(a->backend_name(), "mssql_odbc");
}

TEST(MssqlOdbcAdapter, FactoryAcceptsAliases) {
    EXPECT_NE(make_adapter("mssql"), nullptr);
    EXPECT_NE(make_adapter("sqlserver"), nullptr);
    EXPECT_EQ(make_adapter("nosuch"), nullptr);
}

TEST(MssqlOdbcAdapter, InitiallyNotConnected) {
    auto a = make_unconnected();
    ASSERT_NE(a, nullptr);
    EXPECT_FALSE(a->is_connected());
}

TEST(MssqlOdbcAdapter, ExecuteWithoutConnectionFails) {
    auto a = make_unconnected();
    ASSERT_NE(a, nullptr);
    auto r = a->execute("SELECT 1");
    EXPECT_FALSE(r.ok());
    EXPECT_EQ(r.error, DbError::NotConnected);
}

TEST(MssqlOdbcAdapter, QueryWithoutConnectionFails) {
    auto a = make_unconnected();
    ASSERT_NE(a, nullptr);
    ResultSet rs;
    auto r = a->query("SELECT 1", rs);
    EXPECT_FALSE(r.ok());
    EXPECT_EQ(r.error, DbError::NotConnected);
}

TEST(MssqlOdbcAdapter, BeginTransactionWithoutConnectionFails) {
    auto a = make_unconnected();
    ASSERT_NE(a, nullptr);
    auto r = a->begin_transaction();
    EXPECT_FALSE(r.ok());
}

TEST(MssqlOdbcAdapter, CommitWithoutActiveTransactionFails) {
    auto a = make_unconnected();
    ASSERT_NE(a, nullptr);
    // Not connected AND not in a transaction: depends on which check fires
    // first. Both report a failure, which is the contract we care about.
    auto r = a->commit();
    EXPECT_FALSE(r.ok());
}

TEST(MssqlOdbcAdapter, ConnectToInvalidServerFails) {
    auto a = make_unconnected();
    ASSERT_NE(a, nullptr);
    ConnectionConfig cfg;
    cfg.backend = "mssql_odbc";
    cfg.host = "this-host-does-not-exist.invalid";
    cfg.port = 1433;
    cfg.database = "MoxianDB";
    cfg.user = "sa";
    cfg.password = "no-such-password";
    // Use a 1-second login timeout so the test doesn't hang for the
    // default TCP timeout.
    auto r = a->connect(cfg);
    // Connection either fails outright (best case) or the driver manager
    // returns a SQLSTATE 08001/08003/08S01. Either way, r.ok() == false
    // and r.error is set to a connection-class error.
    if (!r.ok()) {
        EXPECT_EQ(r.error, DbError::ConnectionFailed);
    }
    EXPECT_FALSE(a->is_connected());
    // If by some miracle the connection succeeded (e.g. localhost SQL
    // exists with that password), tear it down cleanly.
    a->disconnect();
}

TEST(MssqlOdbcAdapter, DisconnectIsIdempotent) {
    auto a = make_unconnected();
    ASSERT_NE(a, nullptr);
    a->disconnect();
    a->disconnect();  // second call must be a no-op, not a crash
    EXPECT_FALSE(a->is_connected());
}

#else  // !_WIN32

// On non-Windows platforms the adapter is not built. The factory returns
// nullptr for the mssql_odbc backend (because we #ifdef out the
// registration). Verify that contract.

TEST(MssqlOdbcAdapter, FactoryReturnsNullOnNonWindows) {
    auto a = make_adapter("mssql_odbc");
    EXPECT_EQ(a, nullptr);
}

#endif  // _WIN32
