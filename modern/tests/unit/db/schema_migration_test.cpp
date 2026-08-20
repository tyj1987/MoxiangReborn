#include "mxh/db/schema_migration.hpp"
#include "mxh/db/sqlite_adapter.hpp"

#include <gtest/gtest.h>

namespace {

void connect_memory_db(mxh::db::SqliteAdapter& db) {
    mxh::db::ConnectionConfig config;
    config.backend = "sqlite";
    config.path = ":memory:";
    EXPECT_TRUE(db.connect(config).ok());
}

TEST(SchemaMigration, CreatesVersionedSharedSchemaWithoutAccounts) {
    mxh::db::SqliteAdapter db;
    connect_memory_db(db);
    ASSERT_TRUE(mxh::db::migrate_modern_schema(db).ok());
    EXPECT_EQ(mxh::db::modern_schema_version(db), mxh::db::kModernSchemaVersion);

    mxh::db::ResultSet accounts;
    ASSERT_TRUE(db.query("SELECT id FROM chr_log_info", {}, accounts).ok());
    EXPECT_TRUE(accounts.empty());

    mxh::db::ResultSet columns;
    ASSERT_TRUE(db.query("PRAGMA table_info(character_info)", {}, columns).ok());
    EXPECT_EQ(columns.rows.size(), 14u);
}

TEST(SchemaMigration, IsIdempotent) {
    mxh::db::SqliteAdapter db;
    connect_memory_db(db);
    ASSERT_TRUE(mxh::db::migrate_modern_schema(db).ok());
    ASSERT_TRUE(mxh::db::migrate_modern_schema(db).ok());

    mxh::db::ResultSet versions;
    ASSERT_TRUE(db.query("SELECT version FROM modern_schema_version", {}, versions).ok());
    ASSERT_EQ(versions.rows.size(), 1u);
    EXPECT_EQ(std::get<std::int64_t>(versions.rows[0][0]), mxh::db::kModernSchemaVersion);
}

TEST(SchemaMigration, UpgradesLegacyCharacterTableInPlace) {
    mxh::db::SqliteAdapter db;
    connect_memory_db(db);
    ASSERT_TRUE(db.execute(
        "CREATE TABLE character_info (charname TEXT PRIMARY KEY, chrid INTEGER UNIQUE, userid TEXT, character_data BLOB)", {}).ok());
    ASSERT_TRUE(mxh::db::migrate_modern_schema(db).ok());

    mxh::db::ResultSet columns;
    ASSERT_TRUE(db.query("PRAGMA table_info(character_info)", {}, columns).ok());
    EXPECT_EQ(columns.rows.size(), 14u);
}

}  // namespace
