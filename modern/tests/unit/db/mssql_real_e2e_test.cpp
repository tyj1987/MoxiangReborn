#include "mxh/db/db_adapter.hpp"
#include <gtest/gtest.h>
#include <cstdlib>
using namespace mxh::db;
namespace {
TEST(MssqlRealE2E, LoginCharacterAndLogMoneyRoundTrip) {
 const char* raw = std::getenv("MXH_MSSQL_LEGACY_E2E");
 if (!raw || !*raw) GTEST_SKIP()
     << "set MXH_MSSQL_LEGACY_E2E only after restoring a legacy .bak "
        "containing CharacterInfo and LogMoney";
 auto cfg = ConnectionConfig::from_kv_string(raw);
 cfg.backend = "mssql_odbc";
 auto db = make_adapter(cfg.backend);
 ASSERT_NE(db, nullptr);
 ASSERT_TRUE(db->connect(cfg).ok());
 ResultSet characters;
 auto q = db->query("SELECT TOP 1 * FROM CharacterInfo", characters);
 ASSERT_TRUE(q.ok()) << q.error_message;
 ASSERT_FALSE(characters.empty());
 auto tx = db->begin_transaction(); ASSERT_TRUE(tx.ok());
 auto write = db->execute("INSERT INTO LogMoney (CharacterID, Money, RegDate) SELECT 0, 0, GETDATE() WHERE NOT EXISTS (SELECT 1 FROM LogMoney WHERE 1=0)");
 ASSERT_TRUE(write.ok()) << write.error_message;
 ASSERT_TRUE(db->commit().ok());
 db->disconnect();
}

// Modern-schema roundtrip on a real SQL Server (LocalDB etc.). Gated by
// the same MXH_MSSQL_E2E env; use e.g.
//   backend=mssql_odbc;host=(localdb)\MSSQLLocalDB;database=Moxiang;
// (empty user -> Windows integrated auth).
TEST(MssqlRealE2E, ModernSchemaLoginAndCharacterRoundTrip) {
 const char* raw = std::getenv("MXH_MSSQL_E2E");
 if (!raw || !*raw) GTEST_SKIP() << "set MXH_MSSQL_E2E=backend=mssql_odbc;host=...;database=...";
 auto cfg = ConnectionConfig::from_kv_string(raw);
 cfg.backend = "mssql_odbc";
 auto db = make_adapter(cfg.backend);
 ASSERT_NE(db, nullptr);
 ASSERT_TRUE(db->connect(cfg).ok());

 // Ensure the modern tables exist (idempotent; matches
 // deploy/database/mx_modern_schema_mssql.sql).
 ASSERT_TRUE(db->execute(
     "IF OBJECT_ID(N'dbo.chr_log_info', N'U') IS NULL "
     "CREATE TABLE dbo.chr_log_info ("
     " id NVARCHAR(50) NOT NULL PRIMARY KEY,"
     " pw NVARCHAR(50) NOT NULL,"
     " userlevel INT NOT NULL DEFAULT 0)").ok());
 ASSERT_TRUE(db->execute(
     "IF OBJECT_ID(N'dbo.character_info', N'U') IS NULL "
     "CREATE TABLE dbo.character_info ("
     " chrid BIGINT NOT NULL PRIMARY KEY,"
     " charname NVARCHAR(50) NOT NULL,"
     " userid BIGINT NOT NULL,"
     " sex_type TINYINT NOT NULL DEFAULT 0,"
     " hair_type TINYINT NOT NULL DEFAULT 0,"
     " face_type TINYINT NOT NULL DEFAULT 0,"
     " body_type TINYINT NOT NULL DEFAULT 0,"
     " start_area INT NOT NULL DEFAULT 0,"
     " height FLOAT NOT NULL DEFAULT 1.0,"
     " width FLOAT NOT NULL DEFAULT 1.0,"
     " level INT NOT NULL DEFAULT 1,"
     " map_num INT NOT NULL DEFAULT 0,"
     " standing_idx INT NOT NULL DEFAULT 0)").ok());

 // Seed the test account (LoginHandler's exact query shape).
 ASSERT_TRUE(db->execute(
     "IF NOT EXISTS (SELECT 1 FROM dbo.chr_log_info WHERE id = N'test') "
     "INSERT INTO dbo.chr_log_info (id, pw, userlevel) "
     "VALUES (N'test', N'test', 2)").ok());
 ResultSet login_rs;
 std::vector<Bind> login_params = { mxh::db::bind(std::string("test")) };
 auto lq = db->query(
     "SELECT id, pw, userlevel FROM chr_log_info WHERE id = ?",
     login_params, login_rs);
 ASSERT_TRUE(lq.ok()) << lq.error_message;
 ASSERT_EQ(login_rs.rows.size(), 1u);

 // LoginHandler's login check: pw matches + userlevel read back.
 ASSERT_EQ(login_rs.rows[0].size(), 3u);
 ASSERT_TRUE(std::holds_alternative<std::string>(login_rs.rows[0][1]));
 EXPECT_EQ(std::get<std::string>(login_rs.rows[0][1]), "test");
 ASSERT_TRUE(std::holds_alternative<std::int64_t>(login_rs.rows[0][2]));
 EXPECT_EQ(std::get<std::int64_t>(login_rs.rows[0][2]), 2);

 // Character insert (AgentHandler's exact column set) + readback.
 const std::int64_t chrid = 99001;
 std::vector<Bind> del_params = { mxh::db::bind(chrid) };
 ASSERT_TRUE(db->execute(
     "DELETE FROM dbo.character_info WHERE chrid = ?",
     del_params).ok());
 std::vector<Bind> ins_params = {
     mxh::db::bind(chrid), mxh::db::bind(std::string("MSSQLHero")),
     mxh::db::bind(std::int64_t(1)),
     mxh::db::bind(std::int64_t(0)), mxh::db::bind(std::int64_t(1)),
     mxh::db::bind(std::int64_t(2)), mxh::db::bind(std::int64_t(3)),
     mxh::db::bind(std::int64_t(0)), mxh::db::bind(1.0),
     mxh::db::bind(1.0),
     mxh::db::bind(std::int64_t(12)), mxh::db::bind(std::int64_t(0)),
 };
 ASSERT_TRUE(db->execute(
     "INSERT INTO character_info "
     "(chrid, charname, userid, sex_type, hair_type, face_type, "
     "body_type, start_area, height, width, level, map_num, standing_idx) "
     "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 1, ?, ?)",
     ins_params).ok());
 ResultSet char_rs;
 std::vector<Bind> char_params = { mxh::db::bind(chrid) };
 auto cq = db->query(
     "SELECT chrid, charname, level, map_num FROM character_info WHERE chrid = ?",
     char_params, char_rs);
 ASSERT_TRUE(cq.ok()) << cq.error_message;
 ASSERT_EQ(char_rs.rows.size(), 1u);
 ASSERT_TRUE(std::holds_alternative<std::string>(char_rs.rows[0][1]));
 EXPECT_EQ(std::get<std::string>(char_rs.rows[0][1]), "MSSQLHero");

 ASSERT_TRUE(db->execute(
     "DELETE FROM dbo.character_info WHERE chrid = ?",
     del_params).ok());
 db->disconnect();
}
}
