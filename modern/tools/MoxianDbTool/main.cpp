// MoxianDBTool - Command-line tool for SQLite database management.
//
// Subcommands:
//   init    --db <cfg>             Create schema + default data
//   exec    --db <cfg> <sql>       Execute SQL statement
//   query   --db <cfg> <sql>       Run query and print result as TSV
//   schema  --db <cfg>             List tables in database

#include "mxh/db/db_adapter.hpp"
#include "mxh/db/sqlite_adapter.hpp"

#include <cstdio>
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
    init    --db <cfg>             Initialize SQLite database (creates tables)
    exec    --db <cfg> <sql>       Execute a SQL statement (INSERT/UPDATE/DELETE/DDL)
    query   --db <cfg> <sql>       Execute a query and print result as TSV
    schema  --db <cfg>             List tables

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
    id           INTEGER PRIMARY KEY,
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
    auto er = static_cast<mxh::db::SqliteAdapter*>(adapter.get())->exec_multi(moxian_schema_sql());
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
    auto r = adapter->query("SELECT name FROM sqlite_master WHERE type='table' ORDER BY name", rs);
    if (!r) {
        std::cerr << "ERROR: " << r.error_message << "\n";
        return 1;
    }
    for (const auto& row : rs.rows) {
        if (!row.empty()) std::cout << std::get<std::string>(row[0]) << "\n";
    }
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
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 2;
    }

    std::cerr << "ERROR: invalid arguments (run with --help)\n";
    return 1;
}