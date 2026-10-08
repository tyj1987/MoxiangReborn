#include "mxh/db/schema_migration.hpp"
#include "mxh/db/sqlite_adapter.hpp"

#include <gtest/gtest.h>
#include <algorithm>
#include <vector>

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
    EXPECT_EQ(columns.rows.size(), 18u);
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
    EXPECT_EQ(columns.rows.size(), 18u);
}

TEST(SchemaMigration, LegacyAttributesStayNullWithoutDefaultsOrBackfill) {
    mxh::db::SqliteAdapter db;
    connect_memory_db(db);
    ASSERT_TRUE(db.execute(
        "CREATE TABLE character_info (charname TEXT PRIMARY KEY, chrid INTEGER UNIQUE, userid TEXT, character_data BLOB)", {}).ok());
    ASSERT_TRUE(db.execute(
        "INSERT INTO character_info(charname,chrid,userid) VALUES('OldHero',7,'old')", {}).ok());
    ASSERT_TRUE(mxh::db::migrate_modern_schema(db).ok());
    ASSERT_TRUE(mxh::db::migrate_modern_schema(db).ok());

    mxh::db::ResultSet row;
    ASSERT_TRUE(db.query("SELECT base_gengol,base_minchub,base_cheryuk,base_simmek "
                         "FROM character_info WHERE chrid=7", {}, row).ok());
    ASSERT_EQ(row.rows.size(), 1u);
    for (const auto& value : row.rows.front()) EXPECT_TRUE(std::holds_alternative<std::monostate>(value));

    mxh::db::ResultSet columns;
    ASSERT_TRUE(db.query("PRAGMA table_info(character_info)", {}, columns).ok());
    unsigned seen = 0;
    for (const auto& col : columns.rows) {
        const auto* name = std::get_if<std::string>(&col[1]);
        if (name && name->rfind("base_", 0) == 0) {
            ++seen;
            EXPECT_EQ(std::get<std::int64_t>(col[3]), 0);
            EXPECT_TRUE(std::holds_alternative<std::monostate>(col[4]));
        }
    }
    EXPECT_EQ(seen, 4u);
    EXPECT_FALSE(db.execute("UPDATE character_info SET base_gengol=65536 WHERE chrid=7", {}).ok());
}

TEST(SchemaMigration, UpgradesLegacyPlayerStateWithSocialColumns) {
    mxh::db::SqliteAdapter db;
    connect_memory_db(db);
    ASSERT_TRUE(db.execute(
        "CREATE TABLE modern_player_state (player_id INTEGER PRIMARY KEY, money INTEGER NOT NULL DEFAULT 0, level INTEGER NOT NULL DEFAULT 1, exp INTEGER NOT NULL DEFAULT 0, updated_at TEXT NOT NULL)", {}).ok());
    ASSERT_TRUE(mxh::db::migrate_modern_schema(db).ok());

    mxh::db::ResultSet columns;
    ASSERT_TRUE(db.query("PRAGMA table_info(modern_player_state)", {}, columns).ok());
    ASSERT_EQ(columns.rows.size(), 7u);
    std::vector<std::string> names;
    for (const auto& row : columns.rows) names.push_back(std::get<std::string>(row[1]));
    EXPECT_NE(std::find(names.begin(), names.end(), "party_id"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "guild_id"), names.end());
}

}  // namespace
