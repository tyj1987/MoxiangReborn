// Tests for IDbAdapter interface basics.

#include "mxh/db/db_adapter.hpp"

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