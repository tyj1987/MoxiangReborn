#include <gtest/gtest.h>

#include "mxh/db/db_adapter.hpp"
#include "mxh/server/account_moderation.hpp"

namespace {
std::unique_ptr<mxh::db::IDbAdapter> make_db() {
    auto db = mxh::db::make_adapter("sqlite");
    mxh::db::ConnectionConfig cfg;
    cfg.path = ":memory:";
    EXPECT_TRUE(db->connect(cfg).ok());
    EXPECT_TRUE(db->execute("CREATE TABLE modern_account_status (account_id TEXT PRIMARY KEY, login_blocked INTEGER NOT NULL, reason TEXT NOT NULL, updated_at TEXT NOT NULL)").ok());
    EXPECT_TRUE(db->execute("CREATE TABLE modern_gm_audit (audit_id INTEGER PRIMARY KEY AUTOINCREMENT, actor TEXT NOT NULL, target_account TEXT NOT NULL, action TEXT NOT NULL, reason TEXT NOT NULL, created_at TEXT NOT NULL)").ok());
    return db;
}

TEST(AccountModeration, MissingMigrationFailsOpenForLegacyDatabase) {
    auto db = mxh::db::make_adapter("sqlite");
    mxh::db::ConnectionConfig cfg;
    cfg.path = ":memory:";
    ASSERT_TRUE(db->connect(cfg).ok());
    EXPECT_FALSE(mxh::server::is_account_login_blocked(*db, "player"));
}

TEST(AccountModeration, BanAndUnbanPersistWithAuditTrail) {
    auto db = make_db();
    ASSERT_TRUE(mxh::server::set_account_login_blocked(*db, "player", true, "gm.alice", "botting").ok());
    EXPECT_TRUE(mxh::server::is_account_login_blocked(*db, "player"));
    ASSERT_TRUE(mxh::server::set_account_login_blocked(*db, "player", false, "gm.bob", "appeal accepted").ok());
    EXPECT_FALSE(mxh::server::is_account_login_blocked(*db, "player"));
    mxh::db::ResultSet rows;
    ASSERT_TRUE(db->query("SELECT actor, action, reason FROM modern_gm_audit ORDER BY audit_id", rows).ok());
    ASSERT_EQ(rows.size(), 2u);
    EXPECT_EQ(std::get<std::string>(rows.rows[0][0]), "gm.alice");
    EXPECT_EQ(std::get<std::string>(rows.rows[0][1]), "ban");
    EXPECT_EQ(std::get<std::string>(rows.rows[1][1]), "unban");
}
} // namespace
