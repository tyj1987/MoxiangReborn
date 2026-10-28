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
 auto cr = db->connect(cfg);
 ASSERT_TRUE(cr.ok()) << cr.error_message;
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
 auto cr = db->connect(cfg);
 ASSERT_TRUE(cr.ok()) << cr.error_message;

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
 // The DB has a unique index on charname, so a previous interrupted run
 // could leave a row with the same name but a different chrid.  Delete
 // by both columns to make the test fully idempotent.
 std::vector<Bind> del_params = { mxh::db::bind(chrid) };
 std::vector<Bind> del_by_name_params = { mxh::db::bind(std::string("MSSQLHero")) };
 ASSERT_TRUE(db->execute(
     "DELETE FROM dbo.character_info WHERE chrid = ?",
     del_params).ok());
 ASSERT_TRUE(db->execute(
     "DELETE FROM dbo.character_info WHERE charname = ?",
     del_by_name_params).ok());
 std::vector<Bind> ins_params = {
     mxh::db::bind(chrid), mxh::db::bind(std::string("MSSQLHero")),
     mxh::db::bind(std::int64_t(1)),
     mxh::db::bind(std::int64_t(0)), mxh::db::bind(std::int64_t(1)),
     mxh::db::bind(std::int64_t(2)), mxh::db::bind(std::int64_t(3)),
     mxh::db::bind(std::int64_t(0)), mxh::db::bind(1.0),
     mxh::db::bind(1.0),
     mxh::db::bind(std::int64_t(12)), mxh::db::bind(std::int64_t(0)),
 };
 auto ins_r = db->execute(
     "INSERT INTO character_info "
     "(chrid, charname, userid, sex_type, hair_type, face_type, "
     "body_type, start_area, height, width, level, map_num, standing_idx) "
     "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 1, ?, ?)",
     ins_params);
 ASSERT_TRUE(ins_r.ok()) << ins_r.error_message;
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

// M3 D-stage: BuySyn money persistence to MSSQL modern_player_state.
// End-to-end: real MssqlOdbcAdapter + modern schema + UPSERT verification.
// Mirrors MapHandlerTest.BuySynOkArmPersistsMoneyToSqliteMemory but on
// the real SQL Server. Gated by MXH_MSSQL_E2E.
TEST(MssqlRealE2E, BuySynOkArmPersistsMoneyToMssqlModernPlayerState) {
 const char* raw = std::getenv("MXH_MSSQL_E2E");
 if (!raw || !*raw) GTEST_SKIP()
     << "set MXH_MSSQL_E2E=backend=mssql_odbc;host=...;database=Moxiang";
 auto cfg = ConnectionConfig::from_kv_string(raw);
 cfg.backend = "mssql_odbc";
 auto db = make_adapter(cfg.backend);
 ASSERT_NE(db, nullptr);
 auto cr = db->connect(cfg);
 ASSERT_TRUE(cr.ok()) << cr.error_message;

 // Make sure modern_player_state exists (idempotent — schema deploy
 // already creates it, but the test is safe to run on a fresh server).
 ASSERT_TRUE(db->execute(
     "IF OBJECT_ID(N'dbo.modern_player_state', N'U') IS NULL "
     "CREATE TABLE dbo.modern_player_state ("
     " player_id  BIGINT NOT NULL PRIMARY KEY,"
     " money      BIGINT NOT NULL DEFAULT 0,"
     " level      INT    NOT NULL DEFAULT 1,"
     " exp        BIGINT NOT NULL DEFAULT 0,"
     " updated_at DATETIME2 NOT NULL DEFAULT SYSUTCDATETIME())").ok());

 const std::int64_t pid = 912345;
 std::vector<Bind> del_params = { mxh::db::bind(pid) };
 ASSERT_TRUE(db->execute(
     "DELETE FROM dbo.modern_player_state WHERE player_id = ?",
     del_params).ok());

 // UPSERT path the production orchestrator uses: MERGE so an existing
 // row gets money += delta, otherwise a fresh row is inserted.
 ASSERT_TRUE(db->execute(
     "MERGE dbo.modern_player_state AS t "
     "USING (SELECT CAST(? AS BIGINT) AS player_id, "
     "              CAST(? AS BIGINT) AS delta) AS s "
     "ON t.player_id = s.player_id "
     "WHEN MATCHED THEN "
     "  UPDATE SET money = t.money + s.delta, "
     "             updated_at = SYSUTCDATETIME() "
     "WHEN NOT MATCHED THEN "
     "  INSERT (player_id, money, level, exp, updated_at) "
     "  VALUES (s.player_id, s.delta, 1, 0, SYSUTCDATETIME());",
     std::vector<Bind>{ mxh::db::bind(pid),
                        mxh::db::bind(std::int64_t(1000)) }
 ).ok());

 ResultSet rs;
 std::vector<Bind> qp = { mxh::db::bind(pid) };
 auto sel = db->query(
     "SELECT money, level, exp FROM dbo.modern_player_state WHERE player_id = ?",
     qp, rs);
 ASSERT_TRUE(sel.ok()) << sel.error_message;
 ASSERT_EQ(rs.rows.size(), 1u);
 ASSERT_TRUE(std::holds_alternative<std::int64_t>(rs.rows[0][0]));
 EXPECT_EQ(std::get<std::int64_t>(rs.rows[0][0]), 1000);

 // Second UPSERT proves the MATCHED branch fires (no duplicate row).
 ASSERT_TRUE(db->execute(
     "MERGE dbo.modern_player_state AS t "
     "USING (SELECT CAST(? AS BIGINT) AS player_id, "
     "              CAST(? AS BIGINT) AS delta) AS s "
     "ON t.player_id = s.player_id "
     "WHEN MATCHED THEN "
     "  UPDATE SET money = t.money + s.delta, "
     "             updated_at = SYSUTCDATETIME() "
     "WHEN NOT MATCHED THEN "
     "  INSERT (player_id, money, level, exp, updated_at) "
     "  VALUES (s.player_id, s.delta, 1, 0, SYSUTCDATETIME());",
     std::vector<Bind>{ mxh::db::bind(pid),
                        mxh::db::bind(std::int64_t(250)) }
 ).ok());

 ResultSet rs2;
 std::vector<Bind> qp2 = { mxh::db::bind(pid) };
 auto sel2 = db->query(
     "SELECT money FROM dbo.modern_player_state WHERE player_id = ?",
     qp2, rs2);
 ASSERT_TRUE(sel2.ok()) << sel2.error_message;
 ASSERT_EQ(rs2.rows.size(), 1u);
 EXPECT_EQ(std::get<std::int64_t>(rs2.rows[0][0]), 1250);

 ASSERT_TRUE(db->execute(
     "DELETE FROM dbo.modern_player_state WHERE player_id = ?",
     del_params).ok());
 db->disconnect();
}
}
