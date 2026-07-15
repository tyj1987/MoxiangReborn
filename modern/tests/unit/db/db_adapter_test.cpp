// Tests for IDbAdapter interface basics.

#include "mxh/db/db_adapter.hpp"
#include "mxh/db/sqlite_adapter.hpp"

#ifdef _WIN32
#  include "mxh/db/mssql_odbc_adapter.hpp"
#endif

#include <gtest/gtest.h>

using namespace mxh::db;

TEST(DbAdapter, ConnectionConfigRoundTrip) {
    auto cfg = ConnectionConfig::from_kv_string(
        "backend=mssql;host=localhost;port=1433;database=mh;user=sa;password=secret;");
    EXPECT_EQ(cfg.backend, "mssql");
    EXPECT_EQ(cfg.host, "localhost");
    EXPECT_EQ(cfg.port, 1433);
    EXPECT_EQ(cfg.database, "mh");
    EXPECT_EQ(cfg.user, "sa");
    EXPECT_EQ(cfg.password, "secret");

    auto str = cfg.to_kv_string();
    EXPECT_NE(str.find("host=localhost"), std::string::npos);
}

TEST(DbAdapter, MakeAdapterReturnsNonNullForSqlite) {
    auto a = make_adapter("sqlite");
    ASSERT_NE(a, nullptr);
    EXPECT_EQ(a->backend_name(), "sqlite");
}

TEST(DbAdapter, MakeAdapterReturnsNullForUnknown) {
    auto a = make_adapter("nosuch");
    EXPECT_EQ(a, nullptr);
}

TEST(DbAdapter, EmptyBackendDefaultsToSqlite) {
    auto a = make_adapter("");
    ASSERT_NE(a, nullptr);
    EXPECT_EQ(a->backend_name(), "sqlite");
}

// Phase 10.23: additional factory contract checks on top of
// the original Phase 2 tests above. These pin the type
// identity of the returned adapter and the case-sensitivity
// of the backend name so a future "fix" that maps "SQLite"
// or returns the wrong concrete class is caught here.

TEST(DbAdapter, SqliteFactoryReturnsSqliteAdapterConcreteType) {
    // "sqlite" must return exactly a SqliteAdapter, not just
    // any IDbAdapter. If a future change routes the
    // "sqlite" string to the MSSQL backend (e.g. a copy-paste
    // error in the factory) the dynamic_cast returns null
    // and this test fails loud — rather than the new code
    // silently opening a SQLite-shaped MSSQL connection.
    auto a = make_adapter("sqlite");
    ASSERT_NE(a, nullptr);
    auto* sqlite = dynamic_cast<SqliteAdapter*>(a.get());
    EXPECT_NE(sqlite, nullptr)
        << "make_adapter(\"sqlite\") must return a SqliteAdapter";
}

TEST(DbAdapter, FactoryBackendNameLookupIsCaseSensitive) {
    // The factory uses a literal "sqlite" / "mssql_odbc"
    // string compare, not a tolower() compare. "SQLite"
    // (capital S) is unknown and must return null. A typo
    // like "SqLite" is treated as a typo, not silently
    // corrected. This is a deliberate contract — a future
    // "convenience" change to case-insensitive matching
    // would mask config typos and is caught here.
    EXPECT_EQ(make_adapter("SQLite"), nullptr);
    EXPECT_EQ(make_adapter("SQLITE"), nullptr);
    EXPECT_EQ(make_adapter("SqLite"), nullptr);
    EXPECT_EQ(make_adapter("MSSQL"),  nullptr);
    EXPECT_EQ(make_adapter("MsSql_Odbc"), nullptr);
}

TEST(DbAdapter, FactoryReturnedAdapterIsNotConnected) {
    // The factory must not auto-connect — connecting is the
    // caller's job (and may need credentials, file paths,
    // or ODBC DSN setup that the factory cannot provide).
    // If a future "convenience" change makes the factory
    // auto-connect (e.g. to ":memory:" for SQLite), this
    // test fails and the change is forced to be deliberate.
    auto a = make_adapter("sqlite");
    ASSERT_NE(a, nullptr);
    EXPECT_FALSE(a->is_connected());
}

#ifdef _WIN32
TEST(DbAdapter, MssqlFactoryReturnsMssqlAdapterConcreteType) {
    // "mssql_odbc" must return exactly a MssqlOdbcAdapter
    // — same rationale as the Sqlite concrete-type test
    // above. Pinning the concrete class protects against
    // accidental wiring changes in the factory.
    auto a = make_adapter("mssql_odbc");
    ASSERT_NE(a, nullptr);
    auto* mssql = dynamic_cast<MssqlOdbcAdapter*>(a.get());
    EXPECT_NE(mssql, nullptr)
        << "make_adapter(\"mssql_odbc\") must return a MssqlOdbcAdapter";
}

TEST(DbAdapter, MssqlFactoryAliasesRouteToMssqlAdapter) {
    // "mssql" and "sqlserver" are documented aliases for
    // "mssql_odbc" — config files in the wild use all
    // three. The factory must route all three to the same
    // concrete class.
    for (const char* alias : {"mssql", "sqlserver"}) {
        auto a = make_adapter(alias);
        ASSERT_NE(a, nullptr) << "alias '" << alias << "' must resolve";
        auto* mssql = dynamic_cast<MssqlOdbcAdapter*>(a.get());
        EXPECT_NE(mssql, nullptr) << "alias '" << alias << "' must be MssqlOdbcAdapter";
    }
}
#endif  // _WIN32