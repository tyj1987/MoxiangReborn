#include <gtest/gtest.h>

#include "../../../tools/MoxianGMTool/gm_security.hpp"
#include "../../../tools/MoxianGMTool/gm_repository.hpp"
#include "../../../tools/MoxianGMTool/gm_item_catalog.hpp"
#include "mxh/db/db_adapter.hpp"

TEST(GmSecurity, ConstantTimeEqualityHandlesLengthAndContent) {
    EXPECT_TRUE(mxh::gm::constant_time_equal("same-token", "same-token"));
    EXPECT_FALSE(mxh::gm::constant_time_equal("same-token", "same-tokem"));
    EXPECT_FALSE(mxh::gm::constant_time_equal("short", "shorter"));
}

TEST(GmSecurity, RejectsMissingOrMalformedAuthorization) {
    const std::string token(32, 'a');
    EXPECT_FALSE(mxh::gm::authorize_bearer({}, token));
    EXPECT_FALSE(mxh::gm::authorize_bearer({{"Authorization", token}}, token));
    EXPECT_FALSE(mxh::gm::authorize_bearer({{"Authorization", "Basic " + token}}, token));
    EXPECT_FALSE(mxh::gm::authorize_bearer({{"Authorization", "Bearer wrong"}}, token));
}

TEST(GmSecurity, AcceptsBearerWithCaseInsensitiveHeaderName) {
    const std::string token = "0123456789abcdef0123456789abcdef";
    EXPECT_TRUE(mxh::gm::authorize_bearer({{"authorization", "Bearer " + token}}, token));
    EXPECT_TRUE(mxh::gm::authorize_bearer({{"AUTHORIZATION", "Bearer " + token}}, token));
}

TEST(GmSecurity, EmptyConfiguredTokenNeverAuthorizes) {
    EXPECT_FALSE(mxh::gm::authorize_bearer({{"Authorization", "Bearer "}}, ""));
}

TEST(GmRepository, ListsAuthoritativeCharacterStateAndBan) {
    auto db = mxh::db::make_adapter("sqlite");
    mxh::db::ConnectionConfig cfg;
    cfg.path = ":memory:";
    ASSERT_TRUE(db->connect(cfg).ok());
    ASSERT_TRUE(db->execute("CREATE TABLE character_info (chrid INTEGER PRIMARY KEY, charname TEXT, userid TEXT)").ok());
    ASSERT_TRUE(db->execute("CREATE TABLE modern_player_state (player_id INTEGER PRIMARY KEY, money INTEGER, level INTEGER)").ok());
    ASSERT_TRUE(db->execute("CREATE TABLE modern_account_status (account_id TEXT PRIMARY KEY, login_blocked INTEGER)").ok());
    ASSERT_TRUE(db->execute("CREATE TABLE modern_account_identity (account_id TEXT PRIMARY KEY, user_idx INTEGER UNIQUE)").ok());
    ASSERT_TRUE(db->execute("CREATE TABLE modern_gm_audit (audit_id INTEGER, actor TEXT, target_account TEXT, action TEXT, reason TEXT, created_at TEXT)").ok());
    ASSERT_TRUE(db->execute("INSERT INTO modern_account_identity VALUES ('player01', 99)").ok());
    ASSERT_TRUE(db->execute("INSERT INTO character_info VALUES (42, 'RealHero', '99')").ok());
    ASSERT_TRUE(db->execute("INSERT INTO modern_player_state VALUES (42, 9001, 27)").ok());
    ASSERT_TRUE(db->execute("INSERT INTO modern_account_status VALUES ('player01', 1)").ok());
    mxh::gm::Repository repository(*db);
    std::vector<mxh::gm::PlayerRecord> players;
    ASSERT_TRUE(repository.list_players(players).ok());
    ASSERT_EQ(players.size(), 1u);
    EXPECT_EQ(players[0].character_id, 42);
    EXPECT_EQ(players[0].character_name, "RealHero");
    EXPECT_EQ(players[0].account_id, "player01");
    EXPECT_EQ(players[0].level, 27);
    EXPECT_EQ(players[0].money, 9001);
    EXPECT_TRUE(players[0].login_blocked);
}

TEST(GmRepository, FindsAccountByCharacterId) {
    auto db = mxh::db::make_adapter("sqlite");
    mxh::db::ConnectionConfig cfg;
    cfg.path = ":memory:";
    ASSERT_TRUE(db->connect(cfg).ok());
    ASSERT_TRUE(db->execute("CREATE TABLE character_info (chrid INTEGER PRIMARY KEY, userid TEXT)").ok());
    ASSERT_TRUE(db->execute("CREATE TABLE modern_account_identity (account_id TEXT PRIMARY KEY, user_idx INTEGER UNIQUE)").ok());
    ASSERT_TRUE(db->execute("INSERT INTO modern_account_identity VALUES ('owner', 12)").ok());
    ASSERT_TRUE(db->execute("INSERT INTO character_info VALUES (7, '12')").ok());
    mxh::gm::Repository repository(*db);
    std::string account;
    ASSERT_TRUE(repository.find_account_for_character(7, account).ok());
    EXPECT_EQ(account, "owner");
    ASSERT_TRUE(repository.find_account_for_character(8, account).ok());
    EXPECT_TRUE(account.empty());
}

TEST(GmRepository, ListsAuthoritativeChatNewestFirst) {
    auto db = mxh::db::make_adapter("sqlite");
    mxh::db::ConnectionConfig cfg;
    cfg.path = ":memory:";
    ASSERT_TRUE(db->connect(cfg).ok());
    ASSERT_TRUE(db->execute("CREATE TABLE log_chat (logid INTEGER PRIMARY KEY, chrname TEXT, channel TEXT, message TEXT, logtime TEXT)").ok());
    ASSERT_TRUE(db->execute("INSERT INTO log_chat VALUES (1, 'Hero', 'all', 'first', '2026-08-12 01:00:00')").ok());
    ASSERT_TRUE(db->execute("INSERT INTO log_chat VALUES (2, 'Hero', 'all', 'second', '2026-08-12 01:01:00')").ok());
    mxh::gm::Repository repository(*db);
    mxh::db::ResultSet rows;
    ASSERT_TRUE(repository.list_chat(rows).ok());
    ASSERT_EQ(rows.rows.size(), 2u);
    EXPECT_EQ(std::get<std::int64_t>(rows.rows[0][0]), 2);
    EXPECT_EQ(std::get<std::string>(rows.rows[0][3]), "second");
}

TEST(GmItemCatalog, LoadsAuthoritativeDeployedItemList) {
    mxh::gm::ItemCatalog catalog;
    std::string error;
    const std::string path = std::string(MXH_SOURCE_DIR) + "/../deploy/server/Distribute/Resource/ItemList.bin";
    ASSERT_TRUE(catalog.load(path, error)) << error;
    EXPECT_GT(catalog.items().size(), 100u);
    EXPECT_NE(catalog.items().front().ItemIdx, 0u);
    EXPECT_FALSE(mxh::gm::item_name_utf8(catalog.items().front()).empty());
}
