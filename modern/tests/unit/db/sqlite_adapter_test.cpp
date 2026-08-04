// Tests for SQLite adapter: in-memory database CRUD + transactions.

#include "mxh/db/db_adapter.hpp"
#include "mxh/db/sqlite_adapter.hpp"

#include <gtest/gtest.h>

#include <cstdio>
#include <filesystem>

using namespace mxh::db;
namespace mx = mxh::db;

namespace {

// Returns a fresh in-memory adapter; closed on test teardown.
std::unique_ptr<IDbAdapter> make_in_memory() {
    ConnectionConfig cfg;
    cfg.backend = "sqlite";
    cfg.path = ":memory:";
    auto a = make_adapter("sqlite");
    EXPECT_TRUE(a->connect(cfg).ok());
    return a;
}

}  // namespace

TEST(SqliteAdapter, CreateTableAndInsert) {
    auto db = make_in_memory();
    ASSERT_NE(db, nullptr);

    EXPECT_TRUE(db->execute(
        "CREATE TABLE t (id INTEGER PRIMARY KEY, name TEXT, age INTEGER)").ok());

    std::vector<Bind> p1{mx::bind(std::string("alice")), mx::bind(int(30))};
    auto r = db->execute(
        "INSERT INTO t (name, age) VALUES (?, ?)", p1);
    ASSERT_TRUE(r.ok()) << r.error_message;
    EXPECT_EQ(r.rows_affected, 1);
    EXPECT_EQ(r.last_insert_id, 1);

    std::vector<Bind> p2{mx::bind(std::string("bob")), mx::bind(int(25))};
    r = db->execute("INSERT INTO t (name, age) VALUES (?, ?)", p2);
    EXPECT_EQ(r.last_insert_id, 2);
}

TEST(SqliteAdapter, SelectWithBindParameters) {
    auto db = make_in_memory();
    db->execute("CREATE TABLE t (id INTEGER, name TEXT)");
    db->execute("INSERT INTO t VALUES (1, 'alice')");
    db->execute("INSERT INTO t VALUES (2, 'bob')");
    db->execute("INSERT INTO t VALUES (3, 'carol')");

    std::vector<Bind> params{mx::bind(int(1))};
    ResultSet rs;
    auto r = db->query("SELECT id, name FROM t WHERE id > ?", params, rs);
    ASSERT_TRUE(r.ok()) << r.error_message;
    EXPECT_EQ(rs.columns.size(), 2u);
    EXPECT_EQ(rs.columns[0], "id");
    EXPECT_EQ(rs.columns[1], "name");
    ASSERT_EQ(rs.rows.size(), 2u);
    EXPECT_EQ(std::get<std::int64_t>(rs.rows[0][0]), 2);
    EXPECT_EQ(std::get<std::string>(rs.rows[0][1]), "bob");
    EXPECT_EQ(std::get<std::int64_t>(rs.rows[1][0]), 3);
    EXPECT_EQ(std::get<std::string>(rs.rows[1][1]), "carol");
}

TEST(SqliteAdapter, Transactions) {
    auto db = make_in_memory();
    db->execute("CREATE TABLE t (id INTEGER, val INTEGER)");

    EXPECT_TRUE(db->begin_transaction().ok());
    db->execute("INSERT INTO t VALUES (1, 100)");
    db->execute("INSERT INTO t VALUES (2, 200)");
    EXPECT_TRUE(db->commit().ok());

    ResultSet rs;
    db->query("SELECT COUNT(*) FROM t", rs);
    EXPECT_EQ(std::get<std::int64_t>(rs.rows[0][0]), 2);

    // Rollback test.
    EXPECT_TRUE(db->begin_transaction().ok());
    db->execute("INSERT INTO t VALUES (3, 300)");
    db->execute("INSERT INTO t VALUES (4, 400)");
    EXPECT_TRUE(db->rollback().ok());

    rs.rows.clear();
    db->query("SELECT COUNT(*) FROM t", rs);
    EXPECT_EQ(std::get<std::int64_t>(rs.rows[0][0]), 2);
}

TEST(SqliteAdapter, NullAndBlobBindings) {
    auto db = make_in_memory();
    db->execute("CREATE TABLE t (data BLOB, name TEXT)");

    std::vector<std::uint8_t> blob = {0x01, 0x02, 0x03, 0xFF};
    std::vector<Bind> params{mx::bind(blob), mx::bind_null()};
    db->execute("INSERT INTO t VALUES (?, ?)", params);

    ResultSet rs;
    db->query("SELECT data, name FROM t", rs);
    ASSERT_EQ(rs.rows.size(), 1u);
    auto& b = std::get<std::vector<std::uint8_t>>(rs.rows[0][0]);
    EXPECT_EQ(b.size(), 4u);
    EXPECT_EQ(b[3], 0xFF);
    EXPECT_TRUE(std::holds_alternative<std::monostate>(rs.rows[0][1]));
}

TEST(SqliteAdapter, FileBasedPersistence) {
    auto path = std::filesystem::temp_directory_path() / "mxh_db_test.db";
    std::filesystem::remove(path);

    {
        ConnectionConfig cfg;
        cfg.backend = "sqlite";
        cfg.path = path.string();
        auto db = make_adapter("sqlite");
        ASSERT_TRUE(db->connect(cfg).ok());
        db->execute("CREATE TABLE t (id INTEGER PRIMARY KEY, name TEXT)");
        db->execute("INSERT INTO t (name) VALUES ('persisted')");
    }

    {
        // Reopen.
        ConnectionConfig cfg;
        cfg.backend = "sqlite";
        cfg.path = path.string();
        auto db = make_adapter("sqlite");
        ASSERT_TRUE(db->connect(cfg).ok());
        ResultSet rs;
        db->query("SELECT name FROM t", rs);
        EXPECT_EQ(rs.rows.size(), 1u);
        EXPECT_EQ(std::get<std::string>(rs.rows[0][0]), "persisted");
    }

    std::filesystem::remove(path);
}

TEST(SqliteAdapter, ExecMultiWithInsertOrIgnore) {
    // B5.3 lock: exec_multi() must handle SQLite-only DDL such as
    // INSERT OR IGNORE without crashing. The bundled moxian_schema_sql()
    // in tools/MoxianDbTool/main.cpp uses this pattern to seed default
    // accounts; this test pins the contract.
    auto db = make_in_memory();
    ASSERT_NE(db, nullptr);
    
    const char* schema = R"SQL(
CREATE TABLE IF NOT EXISTS t (id TEXT PRIMARY KEY, n INTEGER);
INSERT OR IGNORE INTO t (id, n) VALUES ('a', 1);
INSERT OR IGNORE INTO t (id, n) VALUES ('b', 2);
)SQL";
    auto* sqlite = dynamic_cast<SqliteAdapter*>(db.get());
    ASSERT_NE(sqlite, nullptr);
    auto er = sqlite->exec_multi(schema);
    ASSERT_TRUE(er.ok()) << er.error_message;
    
    // Re-run; OR IGNORE means the inserts are no-ops the second time,
    // and the table still has exactly 2 rows.
    er = sqlite->exec_multi(schema);
    ASSERT_TRUE(er.ok()) << er.error_message;
    
    ResultSet rs;
    ASSERT_TRUE(db->query("SELECT COUNT(*) FROM t", rs).ok());
    EXPECT_EQ(std::get<std::int64_t>(rs.rows[0][0]), 2);
    
    ResultSet rs2;
    ASSERT_TRUE(db->query("SELECT id, n FROM t ORDER BY id", rs2).ok());
    ASSERT_EQ(rs2.rows.size(), 2u);
    EXPECT_EQ(std::get<std::string>(rs2.rows[0][0]), "a");
    EXPECT_EQ(std::get<std::int64_t>(rs2.rows[0][1]), 1);
    EXPECT_EQ(std::get<std::string>(rs2.rows[1][0]), "b");
    EXPECT_EQ(std::get<std::int64_t>(rs2.rows[1][1]), 2);
}
