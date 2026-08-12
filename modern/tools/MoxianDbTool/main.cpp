// MoxianDBTool - Command-line tool for SQLite database management.
//
// Subcommands:
//   init    --db <cfg>             Create schema + default data
//   exec    --db <cfg> <sql>       Execute SQL statement
//   query   --db <cfg> <sql>       Run query and print result as TSV
//   schema  --db <cfg>             List tables in database

#include "mxh/db/db_adapter.hpp"
#include "mxh/db/sqlite_adapter.hpp"
#include "mxh/server/account_service.hpp"
#include "mxh/server/account_moderation.hpp"

#include <cstdio>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <string_view>

namespace {

void print_usage() {
    std::cout << R"(Moxian DB Tool

USAGE:
    mxh_db_tool <command> [options]

COMMANDS:
    init    --db <cfg>             Initialize schema (SQLite only; for MSSQL use scripts/db/*.sql)
    exec    --db <cfg> <sql>       Execute a SQL statement (INSERT/UPDATE/DELETE/DDL)
    query   --db <cfg> <sql>       Execute a query and print result as TSV
    schema  --db <cfg>             List tables
    register --db <cfg> <account>  Create account; reads password from stdin
    ban      --db <cfg> <account> <actor> <reason>  Block login and audit
    unban    --db <cfg> <account> <actor> <reason>  Restore login and audit

EXAMPLE:
    mxh_db_tool init --db "sqlite;path=./moxian.db"
    mxh_db_tool query --db "sqlite;path=./moxian.db" "SELECT * FROM CharacterInfo LIMIT 10"

OPTIONS:
    -h, --help                      Show this help
)";
}

// Initialize schema for Moxian's three databases.
// Schema derived from [Server]Map\MapDBMsgParser.cpp, [Server]Agent\AgentDBMsgParser.cpp,
// and standard MHCMEMBER/MHGAME/MHLOG layout.
std::string moxian_schema_sql() {
    return R"SQL(
-- ============================================================================
-- MHCMEMBER (account/character basic info)
-- ============================================================================
CREATE TABLE IF NOT EXISTS chr_log_info (
    id           TEXT PRIMARY KEY,
    pw           TEXT NOT NULL,
    userlevel    INTEGER NOT NULL DEFAULT 0,
    registerdate TEXT,
    lastlogindate TEXT,
    lastloginip  TEXT,
    usepoint     INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS character_info (
    charname     TEXT PRIMARY KEY,
    chrid        INTEGER NOT NULL UNIQUE,
    userid       TEXT NOT NULL,
    character_data BLOB
);

CREATE INDEX IF NOT EXISTS idx_character_info_userid ON character_info(userid);

-- ============================================================================
-- MHGAME (game world data)
-- ============================================================================
CREATE TABLE IF NOT EXISTS item_info (
    itemid       INTEGER PRIMARY KEY,
    owner_chr    TEXT,
    item_idx     INTEGER,
    position     INTEGER,
    durability   INTEGER,
    seal_info    BLOB
);

CREATE TABLE IF NOT EXISTS munpa_info (
    munpaid      INTEGER PRIMARY KEY,
    munpaname    TEXT NOT NULL,
    master_idx   INTEGER,
    member_data  BLOB
);

CREATE TABLE IF NOT EXISTS note_list (
    noteid       INTEGER PRIMARY KEY,
    sender       TEXT,
    receiver     TEXT,
    message      TEXT,
    senddate     TEXT
);

-- ============================================================================
-- MHLOG (logs)
-- ============================================================================
CREATE TABLE IF NOT EXISTS log_money (
    logid    INTEGER PRIMARY KEY AUTOINCREMENT,
    chrname  TEXT,
    amount   INTEGER,
    reason   TEXT,
    logtime  TEXT
);

CREATE TABLE IF NOT EXISTS log_item (
    logid    INTEGER PRIMARY KEY AUTOINCREMENT,
    chrname  TEXT,
    itemid   INTEGER,
    action   TEXT,
    logtime  TEXT
);

CREATE TABLE IF NOT EXISTS log_chat (
    logid    INTEGER PRIMARY KEY AUTOINCREMENT,
    chrname  TEXT,
    channel  TEXT,
    message  TEXT,
    logtime  TEXT
);

-- ============================================================================
-- M3 D-stage: modern player state (upsert target for BuySyn money persistence)
-- ============================================================================
CREATE TABLE IF NOT EXISTS modern_player_state (
    player_id  INTEGER PRIMARY KEY,
    money      INTEGER NOT NULL DEFAULT 0,
    level      INTEGER NOT NULL DEFAULT 1,
    exp        INTEGER NOT NULL DEFAULT 0,
    updated_at TEXT    NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_modern_player_state_updated_at
    ON modern_player_state(updated_at);

CREATE TABLE IF NOT EXISTS modern_player_item (
    player_id     INTEGER NOT NULL,
    container     INTEGER NOT NULL,
    slot          INTEGER NOT NULL,
    db_idx        INTEGER NOT NULL,
    item_idx      INTEGER NOT NULL,
    durability    INTEGER NOT NULL,
    rare_idx      INTEGER NOT NULL,
    quick_position INTEGER NOT NULL,
    item_param    INTEGER NOT NULL,
    PRIMARY KEY (player_id, container, slot),
    UNIQUE (player_id, db_idx)
);

CREATE INDEX IF NOT EXISTS idx_modern_player_item_player
    ON modern_player_item(player_id);

CREATE TABLE IF NOT EXISTS modern_player_quest_log (
    player_id        INTEGER NOT NULL,
    quest_id         INTEGER NOT NULL,
    state            INTEGER NOT NULL DEFAULT 0,
    accepted_time_ms INTEGER NOT NULL DEFAULT 0,
    updated_at       TEXT    NOT NULL,
    PRIMARY KEY (player_id, quest_id)
);

CREATE INDEX IF NOT EXISTS idx_modern_player_quest_log_player
    ON modern_player_quest_log(player_id);

CREATE TABLE IF NOT EXISTS modern_player_quest_sub (
    player_id    INTEGER NOT NULL,
    quest_id     INTEGER NOT NULL,
    sub_index    INTEGER NOT NULL,
    kind         INTEGER NOT NULL,
    target_id    INTEGER NOT NULL,
    count        INTEGER NOT NULL DEFAULT 0,
    target_count INTEGER NOT NULL,
    PRIMARY KEY (player_id, quest_id, sub_index)
);

CREATE INDEX IF NOT EXISTS idx_modern_player_quest_sub_player
    ON modern_player_quest_sub(player_id);

CREATE TABLE IF NOT EXISTS modern_account_status (
    account_id TEXT PRIMARY KEY,
    login_blocked INTEGER NOT NULL DEFAULT 0,
    reason TEXT NOT NULL DEFAULT '',
    updated_at TEXT NOT NULL
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
    created_at TEXT NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_modern_gm_audit_target
    ON modern_gm_audit(target_account, created_at);

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

CREATE INDEX IF NOT EXISTS idx_modern_live_event_window
    ON modern_live_event(enabled, starts_at, ends_at);

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

CREATE INDEX IF NOT EXISTS idx_modern_item_grant_pending
    ON modern_item_grant(character_id, status, grant_id);
)SQL";
}

int cmd_init(const std::string& cfg_str) {
    auto cfg = mxh::db::ConnectionConfig::from_kv_string(cfg_str);
    auto adapter = mxh::db::make_adapter(cfg.backend);
    if (!adapter) {
        std::cerr << "ERROR: unknown backend: " << cfg.backend << "\n";
        return 1;
    }
    auto r = adapter->connect(cfg);
    if (!r) {
        std::cerr << "ERROR connect: " << r.error_message << "\n";
        return 1;
    }
    // The bundled schema uses SQLite-only DDL (INSERT OR IGNORE, AUTOINCREMENT,
    // sqlite_master). For non-SQLite backends the schema must be created
    // out-of-band (e.g. by restoring the MSSQL .bak and applying the migration
    // scripts in scripts/db/).
    auto* sqlite = dynamic_cast<mxh::db::SqliteAdapter*>(adapter.get());
    if (!sqlite) {
        std::cerr << "ERROR init schema: backend '" << adapter->backend_name()
                  << " is not SQLite. The bundled moxian_schema_sql() contains "
                  << "SQLite-only DDL (INSERT OR IGNORE, AUTOINCREMENT). "
                  << "Restore the MSSQL .bak and apply scripts/db/*.sql instead.\n";
        return 1;
    }
    auto er = sqlite->exec_multi(moxian_schema_sql());
    if (!er) {
        std::cerr << "ERROR init schema: " << er.error_message << "\n";
        return 1;
    }
    std::cout << "Schema initialized: " << cfg.path << "\n";
    return 0;
}

int cmd_exec(const std::string& cfg_str, const std::string& sql) {
    auto cfg = mxh::db::ConnectionConfig::from_kv_string(cfg_str);
    auto adapter = mxh::db::make_adapter(cfg.backend);
    if (!adapter) { std::cerr << "unknown backend\n"; return 1; }
    auto cr = adapter->connect(cfg);
    if (!cr) { std::cerr << "connect: " << cr.error_message << "\n"; return 1; }
    auto r = adapter->execute(sql);
    if (!r) {
        std::cerr << "ERROR: " << r.error_message << "\n";
        return 1;
    }
    std::cout << "OK. rows_affected=" << r.rows_affected
              << " last_insert_id=" << r.last_insert_id << "\n";
    return 0;
}

int cmd_query(const std::string& cfg_str, const std::string& sql) {
    auto cfg = mxh::db::ConnectionConfig::from_kv_string(cfg_str);
    auto adapter = mxh::db::make_adapter(cfg.backend);
    if (!adapter) { std::cerr << "unknown backend\n"; return 1; }
    auto cr = adapter->connect(cfg);
    if (!cr) { std::cerr << "connect: " << cr.error_message << "\n"; return 1; }
    mxh::db::ResultSet rs;
    auto r = adapter->query(sql, rs);
    if (!r) {
        std::cerr << "ERROR: " << r.error_message << "\n";
        return 1;
    }
    // Print header (tab-separated).
    for (std::size_t i = 0; i < rs.columns.size(); ++i) {
        if (i > 0) std::cout << '\t';
        std::cout << rs.columns[i];
    }
    std::cout << '\n';
    for (const auto& row : rs.rows) {
        for (std::size_t i = 0; i < row.size(); ++i) {
            if (i > 0) std::cout << '\t';
            std::visit([](auto&& v) {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, std::monostate>) std::cout << "NULL";
                else if constexpr (std::is_same_v<T, std::int64_t>) std::cout << v;
                else if constexpr (std::is_same_v<T, double>) std::cout << v;
                else if constexpr (std::is_same_v<T, std::string>) std::cout << v;
                else if constexpr (std::is_same_v<T, std::vector<std::uint8_t>>) {
                    std::cout << "<" << v.size() << " bytes>";
                }
            }, row[i]);
        }
        std::cout << '\n';
    }
    std::cout << "(" << rs.rows.size() << " rows)\n";
    return 0;
}

int cmd_schema(const std::string& cfg_str) {
    auto cfg = mxh::db::ConnectionConfig::from_kv_string(cfg_str);
    auto adapter = mxh::db::make_adapter(cfg.backend);
    if (!adapter) { std::cerr << "unknown backend\n"; return 1; }
    auto cr = adapter->connect(cfg);
    if (!cr) { std::cerr << "connect: " << cr.error_message << "\n"; return 1; }
    mxh::db::ResultSet rs;
    // sqlite_master is SQLite-only; MSSQL uses INFORMATION_SCHEMA.TABLES.
    const char* kSchemaSql =
        (adapter->backend_name() == "sqlite")
            ? "SELECT name FROM sqlite_master WHERE type='table' ORDER BY name"
            : "SELECT TABLE_NAME AS name FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_TYPE='BASE TABLE' ORDER BY TABLE_NAME";
    auto r = adapter->query(kSchemaSql, rs);
    if (!r) {
        std::cerr << "ERROR: " << r.error_message << "\n";
        return 1;
    }
    for (const auto& row : rs.rows) {
        if (!row.empty()) std::cout << std::get<std::string>(row[0]) << "\n";
    }
    return 0;
}

int cmd_register(const std::string& cfg_str, const std::string& account) {
    auto cfg = mxh::db::ConnectionConfig::from_kv_string(cfg_str);
    auto adapter = mxh::db::make_adapter(cfg.backend);
    if (!adapter) { std::cerr << "ERROR: unknown backend\n"; return 1; }
    const auto connected = adapter->connect(cfg);
    if (!connected) { std::cerr << "ERROR connect: " << connected.error_message << "\n"; return 1; }
    std::string password;
    if (!std::getline(std::cin, password)) {
        std::cerr << "ERROR: password must be provided on stdin\n";
        return 1;
    }
    const auto result = mxh::server::create_account(*adapter, account, password);
    std::fill(password.begin(), password.end(), '\0');
    if (!result.ok()) {
        std::cerr << "ERROR: " << result.message << "\n";
        return 1;
    }
    std::cout << "Account created: " << account << "\n";
    return 0;
}

int cmd_moderate(const std::string& cfg_str, const std::string& account,
                 const std::string& actor, const std::string& reason, bool blocked) {
    auto cfg = mxh::db::ConnectionConfig::from_kv_string(cfg_str);
    auto adapter = mxh::db::make_adapter(cfg.backend);
    if (!adapter) { std::cerr << "ERROR: unknown backend\n"; return 1; }
    const auto connected = adapter->connect(cfg);
    if (!connected) { std::cerr << "ERROR connect: " << connected.error_message << "\n"; return 1; }
    mxh::db::ResultSet account_rows;
    const std::vector<mxh::db::Bind> args{mxh::db::bind(account)};
    const auto found = adapter->query("SELECT 1 FROM chr_log_info WHERE id = ?", args, account_rows);
    if (!found.ok() || account_rows.empty()) { std::cerr << "ERROR: account not found\n"; return 1; }
    const auto result = mxh::server::set_account_login_blocked(
        *adapter, account, blocked, actor, reason);
    if (!result.ok()) { std::cerr << "ERROR: " << result.error_message << "\n"; return 1; }
    std::cout << (blocked ? "Account banned: " : "Account unbanned: ") << account << "\n";
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) { print_usage(); return 1; }
    std::string_view cmd = argv[1];
    if (cmd == "-h" || cmd == "--help") { print_usage(); return 0; }

    // Parse --db and positional args.
    std::string cfg_str;
    std::vector<std::string> positional;
    for (int i = 2; i < argc; ++i) {
        std::string_view a = argv[i];
        if (a == "--db") {
            if (i + 1 < argc) cfg_str = argv[++i];
        } else if (a == "-h" || a == "--help") {
            print_usage(); return 0;
        } else {
            positional.emplace_back(a);
        }
    }

    try {
        if (cfg_str.empty()) {
            std::cerr << "ERROR: --db is required\n";
            return 1;
        }
        if (cmd == "init" && positional.empty()) {
            return cmd_init(cfg_str);
        }
        if (cmd == "exec" && positional.size() == 1) {
            return cmd_exec(cfg_str, positional[0]);
        }
        if (cmd == "query" && positional.size() == 1) {
            return cmd_query(cfg_str, positional[0]);
        }
        if (cmd == "schema" && positional.empty()) {
            return cmd_schema(cfg_str);
        }
        if (cmd == "register" && positional.size() == 1) {
            return cmd_register(cfg_str, positional[0]);
        }
        if (cmd == "ban" && positional.size() == 3) {
            return cmd_moderate(cfg_str, positional[0], positional[1], positional[2], true);
        }
        if (cmd == "unban" && positional.size() == 3) {
            return cmd_moderate(cfg_str, positional[0], positional[1], positional[2], false);
        }
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 2;
    }

    std::cerr << "ERROR: invalid arguments (run with --help)\n";
    return 1;
}
