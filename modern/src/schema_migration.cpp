#include "mxh/db/schema_migration.hpp"

#include "mxh/db/sqlite_adapter.hpp"

#include <array>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>

namespace mxh::db {
namespace {

constexpr std::string_view kSqliteSchema = R"SQL(
CREATE TABLE IF NOT EXISTS modern_schema_version (
    version INTEGER PRIMARY KEY,
    applied_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);
CREATE TABLE IF NOT EXISTS chr_log_info (
    id TEXT PRIMARY KEY,
    pw TEXT NOT NULL,
    userlevel INTEGER NOT NULL DEFAULT 0,
    registerdate TEXT,
    lastlogindate TEXT,
    lastloginip TEXT,
    usepoint INTEGER NOT NULL DEFAULT 0
);
CREATE TABLE IF NOT EXISTS character_info (
    charname TEXT PRIMARY KEY,
    chrid INTEGER NOT NULL UNIQUE,
    userid TEXT NOT NULL,
    sex_type INTEGER NOT NULL DEFAULT 0,
    hair_type INTEGER NOT NULL DEFAULT 0,
    face_type INTEGER NOT NULL DEFAULT 0,
    body_type INTEGER NOT NULL DEFAULT 0,
    start_area INTEGER NOT NULL DEFAULT 12,
    height REAL NOT NULL DEFAULT 1.0,
    width REAL NOT NULL DEFAULT 1.0,
    level INTEGER NOT NULL DEFAULT 1,
    map_num INTEGER NOT NULL DEFAULT 12,
    standing_idx INTEGER NOT NULL DEFAULT 0,
    character_data BLOB
);
CREATE INDEX IF NOT EXISTS idx_character_info_userid ON character_info(userid);
CREATE TABLE IF NOT EXISTS modern_player_state (
    player_id INTEGER PRIMARY KEY,
    money INTEGER NOT NULL DEFAULT 0,
    level INTEGER NOT NULL DEFAULT 1,
    exp INTEGER NOT NULL DEFAULT 0,
    updated_at TEXT NOT NULL
);
CREATE INDEX IF NOT EXISTS idx_modern_player_state_updated_at ON modern_player_state(updated_at);
CREATE TABLE IF NOT EXISTS modern_player_item (
    player_id INTEGER NOT NULL,
    container INTEGER NOT NULL,
    slot INTEGER NOT NULL,
    db_idx INTEGER NOT NULL,
    item_idx INTEGER NOT NULL,
    durability INTEGER NOT NULL,
    rare_idx INTEGER NOT NULL,
    quick_position INTEGER NOT NULL,
    item_param INTEGER NOT NULL,
    PRIMARY KEY (player_id, container, slot),
    UNIQUE (player_id, db_idx)
);
CREATE INDEX IF NOT EXISTS idx_modern_player_item_player ON modern_player_item(player_id);
CREATE TABLE IF NOT EXISTS modern_player_quest_log (
    player_id INTEGER NOT NULL,
    quest_id INTEGER NOT NULL,
    state INTEGER NOT NULL DEFAULT 0,
    accepted_time_ms INTEGER NOT NULL DEFAULT 0,
    updated_at TEXT NOT NULL,
    PRIMARY KEY (player_id, quest_id)
);
CREATE INDEX IF NOT EXISTS idx_modern_player_quest_log_player ON modern_player_quest_log(player_id);
CREATE TABLE IF NOT EXISTS modern_player_quest_sub (
    player_id INTEGER NOT NULL,
    quest_id INTEGER NOT NULL,
    sub_index INTEGER NOT NULL,
    kind INTEGER NOT NULL,
    target_id INTEGER NOT NULL,
    count INTEGER NOT NULL DEFAULT 0,
    target_count INTEGER NOT NULL,
    PRIMARY KEY (player_id, quest_id, sub_index)
);
CREATE INDEX IF NOT EXISTS idx_modern_player_quest_sub_player ON modern_player_quest_sub(player_id);
CREATE TABLE IF NOT EXISTS modern_account_status (
    account_id TEXT PRIMARY KEY,
    login_blocked INTEGER NOT NULL DEFAULT 0,
    reason TEXT NOT NULL DEFAULT '',
    updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);
CREATE TABLE IF NOT EXISTS modern_account_identity (
    account_id TEXT PRIMARY KEY,
    user_idx INTEGER NOT NULL UNIQUE
);
CREATE TABLE IF NOT EXISTS modern_gm_audit (
    audit_id INTEGER PRIMARY KEY AUTOINCREMENT,
    actor TEXT NOT NULL,
    target_account TEXT NOT NULL,
    action TEXT NOT NULL,
    reason TEXT NOT NULL DEFAULT '',
    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);
CREATE INDEX IF NOT EXISTS idx_modern_gm_audit_target ON modern_gm_audit(target_account, created_at);
CREATE TABLE IF NOT EXISTS modern_live_event (
    event_id INTEGER PRIMARY KEY AUTOINCREMENT,
    event_type TEXT NOT NULL,
    title TEXT NOT NULL,
    config_json TEXT NOT NULL,
    starts_at TEXT NOT NULL,
    ends_at TEXT NOT NULL,
    enabled INTEGER NOT NULL DEFAULT 1,
    created_by TEXT NOT NULL,
    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);
CREATE INDEX IF NOT EXISTS idx_modern_live_event_window ON modern_live_event(enabled, starts_at, ends_at);
CREATE TABLE IF NOT EXISTS modern_item_grant (
    grant_id INTEGER PRIMARY KEY AUTOINCREMENT,
    idempotency_key TEXT NOT NULL UNIQUE,
    character_id INTEGER NOT NULL,
    item_id INTEGER NOT NULL,
    item_count INTEGER NOT NULL,
    status TEXT NOT NULL DEFAULT 'pending',
    inventory_slot INTEGER,
    created_by TEXT NOT NULL,
    reason TEXT NOT NULL,
    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    claimed_at TEXT
);
CREATE INDEX IF NOT EXISTS idx_modern_item_grant_pending ON modern_item_grant(character_id, status, grant_id);
CREATE TABLE IF NOT EXISTS log_chat (
    logid INTEGER PRIMARY KEY AUTOINCREMENT,
    chrname TEXT NOT NULL,
    channel TEXT NOT NULL,
    message TEXT NOT NULL,
    logtime TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);
INSERT OR IGNORE INTO modern_schema_version(version) VALUES (1);
)SQL";

constexpr std::array<std::string_view, 18> kMssqlSchema = {
    "IF OBJECT_ID(N'dbo.modern_schema_version', N'U') IS NULL CREATE TABLE dbo.modern_schema_version (version INT NOT NULL PRIMARY KEY, applied_at DATETIME2 NOT NULL DEFAULT SYSUTCDATETIME())",
    "IF OBJECT_ID(N'dbo.chr_log_info', N'U') IS NULL CREATE TABLE dbo.chr_log_info (id NVARCHAR(50) NOT NULL PRIMARY KEY, pw NVARCHAR(160) NOT NULL, userlevel INT NOT NULL DEFAULT 0, registerdate NVARCHAR(32) NULL, lastlogindate NVARCHAR(32) NULL, lastloginip NVARCHAR(64) NULL, usepoint BIGINT NOT NULL DEFAULT 0)",
    "IF COL_LENGTH(N'dbo.chr_log_info', N'pw') < 320 ALTER TABLE dbo.chr_log_info ALTER COLUMN pw NVARCHAR(160) NOT NULL",
    "IF OBJECT_ID(N'dbo.character_info', N'U') IS NULL CREATE TABLE dbo.character_info (charname NVARCHAR(50) NOT NULL PRIMARY KEY, chrid BIGINT NOT NULL UNIQUE, userid NVARCHAR(50) NOT NULL, sex_type TINYINT NOT NULL DEFAULT 0, hair_type TINYINT NOT NULL DEFAULT 0, face_type TINYINT NOT NULL DEFAULT 0, body_type TINYINT NOT NULL DEFAULT 0, start_area INT NOT NULL DEFAULT 12, height FLOAT NOT NULL DEFAULT 1.0, width FLOAT NOT NULL DEFAULT 1.0, level INT NOT NULL DEFAULT 1, map_num INT NOT NULL DEFAULT 12, standing_idx INT NOT NULL DEFAULT 0, character_data VARBINARY(MAX) NULL)",
    "IF COL_LENGTH(N'dbo.character_info', N'character_data') IS NULL ALTER TABLE dbo.character_info ADD character_data VARBINARY(MAX) NULL",
    "IF EXISTS (SELECT 1 FROM sys.columns WHERE object_id=OBJECT_ID(N'dbo.character_info') AND name=N'userid' AND TYPE_NAME(user_type_id)<>N'nvarchar') ALTER TABLE dbo.character_info ALTER COLUMN userid NVARCHAR(50) NOT NULL",
    "IF NOT EXISTS (SELECT 1 FROM sys.indexes WHERE name=N'ux_character_info_charname' AND object_id=OBJECT_ID(N'dbo.character_info')) CREATE UNIQUE NONCLUSTERED INDEX ux_character_info_charname ON dbo.character_info(charname)",
    "IF OBJECT_ID(N'dbo.modern_player_state', N'U') IS NULL CREATE TABLE dbo.modern_player_state (player_id BIGINT NOT NULL PRIMARY KEY, money BIGINT NOT NULL DEFAULT 0, level INT NOT NULL DEFAULT 1, exp BIGINT NOT NULL DEFAULT 0, updated_at NVARCHAR(32) NOT NULL)",
    "IF OBJECT_ID(N'dbo.modern_player_item', N'U') IS NULL CREATE TABLE dbo.modern_player_item (player_id BIGINT NOT NULL, container TINYINT NOT NULL, slot INT NOT NULL, db_idx BIGINT NOT NULL, item_idx INT NOT NULL, durability BIGINT NOT NULL, rare_idx BIGINT NOT NULL, quick_position INT NOT NULL, item_param BIGINT NOT NULL, CONSTRAINT pk_modern_player_item PRIMARY KEY (player_id,container,slot), CONSTRAINT uq_modern_player_item_db_idx UNIQUE (player_id,db_idx))",
    "IF OBJECT_ID(N'dbo.modern_player_quest_log', N'U') IS NULL CREATE TABLE dbo.modern_player_quest_log (player_id BIGINT NOT NULL, quest_id BIGINT NOT NULL, state TINYINT NOT NULL DEFAULT 0, accepted_time_ms BIGINT NOT NULL DEFAULT 0, updated_at NVARCHAR(32) NOT NULL, CONSTRAINT pk_modern_player_quest_log PRIMARY KEY (player_id,quest_id))",
    "IF OBJECT_ID(N'dbo.modern_player_quest_sub', N'U') IS NULL CREATE TABLE dbo.modern_player_quest_sub (player_id BIGINT NOT NULL, quest_id BIGINT NOT NULL, sub_index INT NOT NULL, kind TINYINT NOT NULL, target_id BIGINT NOT NULL, count BIGINT NOT NULL DEFAULT 0, target_count BIGINT NOT NULL, CONSTRAINT pk_modern_player_quest_sub PRIMARY KEY (player_id,quest_id,sub_index))",
    "IF OBJECT_ID(N'dbo.modern_account_status', N'U') IS NULL CREATE TABLE dbo.modern_account_status (account_id NVARCHAR(50) NOT NULL PRIMARY KEY, login_blocked INT NOT NULL DEFAULT 0, reason NVARCHAR(256) NOT NULL DEFAULT N'', updated_at DATETIME2 NOT NULL DEFAULT SYSUTCDATETIME())",
    "IF OBJECT_ID(N'dbo.modern_account_identity', N'U') IS NULL CREATE TABLE dbo.modern_account_identity (account_id NVARCHAR(50) NOT NULL PRIMARY KEY, user_idx BIGINT NOT NULL UNIQUE)",
    "IF OBJECT_ID(N'dbo.modern_gm_audit', N'U') IS NULL CREATE TABLE dbo.modern_gm_audit (audit_id BIGINT IDENTITY(1,1) NOT NULL PRIMARY KEY, actor NVARCHAR(64) NOT NULL, target_account NVARCHAR(50) NOT NULL, action NVARCHAR(32) NOT NULL, reason NVARCHAR(256) NOT NULL DEFAULT N'', created_at DATETIME2 NOT NULL DEFAULT SYSUTCDATETIME())",
    "IF OBJECT_ID(N'dbo.modern_live_event', N'U') IS NULL CREATE TABLE dbo.modern_live_event (event_id BIGINT IDENTITY(1,1) NOT NULL PRIMARY KEY, event_type NVARCHAR(32) NOT NULL, title NVARCHAR(128) NOT NULL, config_json NVARCHAR(MAX) NOT NULL, starts_at DATETIME2 NOT NULL, ends_at DATETIME2 NOT NULL, enabled INT NOT NULL DEFAULT 1, created_by NVARCHAR(64) NOT NULL, created_at DATETIME2 NOT NULL DEFAULT SYSUTCDATETIME(), updated_at DATETIME2 NOT NULL DEFAULT SYSUTCDATETIME())",
    "IF OBJECT_ID(N'dbo.modern_item_grant', N'U') IS NULL CREATE TABLE dbo.modern_item_grant (grant_id BIGINT IDENTITY(1,1) NOT NULL PRIMARY KEY, idempotency_key NVARCHAR(128) NOT NULL UNIQUE, character_id BIGINT NOT NULL, item_id INT NOT NULL, item_count BIGINT NOT NULL, status NVARCHAR(16) NOT NULL DEFAULT N'pending', inventory_slot INT NULL, created_by NVARCHAR(64) NOT NULL, reason NVARCHAR(256) NOT NULL, created_at DATETIME2 NOT NULL DEFAULT SYSUTCDATETIME(), claimed_at DATETIME2 NULL)",
    "IF OBJECT_ID(N'dbo.log_chat', N'U') IS NULL CREATE TABLE dbo.log_chat (logid BIGINT IDENTITY(1,1) NOT NULL PRIMARY KEY, chrname NVARCHAR(64) NOT NULL, channel NVARCHAR(32) NOT NULL, message NVARCHAR(512) NOT NULL, logtime DATETIME2 NOT NULL DEFAULT SYSUTCDATETIME())",
    "IF NOT EXISTS (SELECT 1 FROM dbo.modern_schema_version WHERE version=1) INSERT INTO dbo.modern_schema_version(version) VALUES (1)"
};

DbResult not_supported(const std::string& backend) {
    return {DbError::NotImplemented, "schema migration does not support backend '" + backend + "'"};
}

DbResult add_missing_sqlite_character_columns(IDbAdapter& db) {
    ResultSet rows;
    auto result = db.query("PRAGMA table_info(character_info)", rows);
    if (!result.ok()) return result;
    std::unordered_set<std::string> columns;
    for (const auto& row : rows.rows) {
        if (row.size() > 1) {
            if (const auto* name = std::get_if<std::string>(&row[1])) columns.insert(*name);
        }
    }
    constexpr std::array<std::pair<std::string_view, std::string_view>, 10> additions = {{
        {"sex_type", "ALTER TABLE character_info ADD COLUMN sex_type INTEGER NOT NULL DEFAULT 0"},
        {"hair_type", "ALTER TABLE character_info ADD COLUMN hair_type INTEGER NOT NULL DEFAULT 0"},
        {"face_type", "ALTER TABLE character_info ADD COLUMN face_type INTEGER NOT NULL DEFAULT 0"},
        {"body_type", "ALTER TABLE character_info ADD COLUMN body_type INTEGER NOT NULL DEFAULT 0"},
        {"start_area", "ALTER TABLE character_info ADD COLUMN start_area INTEGER NOT NULL DEFAULT 12"},
        {"height", "ALTER TABLE character_info ADD COLUMN height REAL NOT NULL DEFAULT 1.0"},
        {"width", "ALTER TABLE character_info ADD COLUMN width REAL NOT NULL DEFAULT 1.0"},
        {"level", "ALTER TABLE character_info ADD COLUMN level INTEGER NOT NULL DEFAULT 1"},
        {"map_num", "ALTER TABLE character_info ADD COLUMN map_num INTEGER NOT NULL DEFAULT 12"},
        {"standing_idx", "ALTER TABLE character_info ADD COLUMN standing_idx INTEGER NOT NULL DEFAULT 0"}
    }};
    for (const auto& [name, sql] : additions) {
        if (columns.contains(std::string(name))) continue;
        result = db.execute(sql);
        if (!result.ok()) return result;
    }
    return {};
}

}  // namespace

DbResult migrate_modern_schema(IDbAdapter& db) {
    if (!db.is_connected()) return {DbError::NotConnected, "database is not connected"};
    if (db.backend_name() == "sqlite") {
        auto* sqlite = dynamic_cast<SqliteAdapter*>(&db);
        if (sqlite == nullptr) return not_supported(db.backend_name());
        auto result = sqlite->exec_multi(kSqliteSchema);
        if (!result.ok()) return result;
        return add_missing_sqlite_character_columns(db);
    }
    if (db.backend_name() == "mssql_odbc") {
        for (const auto sql : kMssqlSchema) {
            auto result = db.execute(sql);
            if (!result.ok()) return result;
        }
        return {};
    }
    return not_supported(db.backend_name());
}

int modern_schema_version(IDbAdapter& db) {
    ResultSet rows;
    const auto result = db.query("SELECT MAX(version) AS version FROM modern_schema_version", rows);
    if (!result.ok() || rows.empty() || rows.rows[0].empty()) return 0;
    if (const auto* value = std::get_if<std::int64_t>(&rows.rows[0][0])) {
        return static_cast<int>(*value);
    }
    return 0;
}

}  // namespace mxh::db
