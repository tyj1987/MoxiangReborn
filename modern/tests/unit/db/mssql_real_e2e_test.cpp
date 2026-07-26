#include "mxh/db/db_adapter.hpp"
#include <gtest/gtest.h>
#include <cstdlib>
using namespace mxh::db;
namespace {
TEST(MssqlRealE2E, LoginCharacterAndLogMoneyRoundTrip) {
 const char* raw = std::getenv("MXH_MSSQL_E2E");
 if (!raw || !*raw) GTEST_SKIP() << "set MXH_MSSQL_E2E=backend=mssql_odbc;host=...;database=Moxiang;user=...;password=...";
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
}
