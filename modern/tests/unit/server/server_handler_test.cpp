// server_handler_test.cpp - Phase 10.17 server handler smoke tests
//
// Covers modern/include/mxh/server/server.hpp ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â the 3 high-level
// game-server handlers (LoginHandler, AgentHandler, MapHandler).
// These are the bridge between the net layer and the game logic.
//
// Scope: this is a smoke test. It verifies the handlers can be
// constructed with a mock IDbAdapter, that on_connect / on_disconnect
// don't crash, and that the few state-setter / state-getter
// methods exposed on the public API surface work as documented.
//
// What is NOT tested here (covered by other test files):
//   - The actual on_message protocol dispatch ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â covered by
//     integration tests that wire a real TcpServer to a handler
//     and feed it bytes (see modern/tools/MoxianLoginServer 5/5
//     smoke). A unit test for the byte-level protocol would need
//     to re-derive the message framing, which would duplicate the
//     integration test surface for no extra value.
//   - HSEL encryption integration with the handlers ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â covered
//     by hsel_stream_test.cpp + aes_gcm_test.cpp.

#include "mxh/game/hero_total_layout.hpp"
#include "mxh/server/server.hpp"
#include "mxh/server/ai_system.hpp"
#include "client/CInGameState.hpp"
#include "mxh/render/EntityScene.hpp"
#include "mxh/game/item_manager.hpp"
#include "mxh/game/item_list_parser.hpp"
#include "mxh/compat/mh_file_ex.hpp"
#include "mxh/server/dealitem_parser.hpp"
#include "mxh/server/quest_script_loader.hpp"
#include "cstdint"
#include "filesystem"
#include "fstream"
#include "sstream"
#include "vector"
#include "mxh/db/db_adapter.hpp"
#include "mxh/db/sqlite_adapter.hpp"
#include "mxh/db/schema_migration.hpp"
#include "mxh/net/net.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <array>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
#include <thread>
#include <unordered_map>
#include <unordered_set>

namespace mxh::server::test {

bool seed_fixture_base_attributes(mxh::db::IDbAdapter& db, std::uint32_t character) {
    for (const char* statement : {
             "ALTER TABLE character_info ADD COLUMN base_gengol INTEGER NULL",
             "ALTER TABLE character_info ADD COLUMN base_minchub INTEGER NULL",
             "ALTER TABLE character_info ADD COLUMN base_cheryuk INTEGER NULL",
             "ALTER TABLE character_info ADD COLUMN base_simmek INTEGER NULL"}) {
        if (!db.execute(statement).ok()) return false;
    }
    if (!db.execute("UPDATE character_info SET base_gengol=12,base_minchub=12,"
                    "base_cheryuk=12,base_simmek=12 WHERE chrid=?",
                    {mxh::db::bind(static_cast<std::int64_t>(character))}).ok()) return false;
    return db.execute("CREATE TABLE IF NOT EXISTS modern_player_item ("
                      "player_id INTEGER NOT NULL,container INTEGER NOT NULL,slot INTEGER NOT NULL,"
                      "db_idx INTEGER NOT NULL,item_idx INTEGER NOT NULL,durability INTEGER NOT NULL,"
                      "rare_idx INTEGER NOT NULL,quick_position INTEGER NOT NULL,item_param INTEGER NOT NULL,"
                      "PRIMARY KEY(player_id,container,slot),UNIQUE(player_id,db_idx))").ok();
}

bool create_fixture_character(mxh::db::IDbAdapter& db, std::uint32_t character) {
    if (!db.execute("CREATE TABLE character_info ("
                    "chrid INTEGER PRIMARY KEY,charname TEXT,sex_type INTEGER,face_type INTEGER,"
                    "hair_type INTEGER,height REAL,width REAL,level INTEGER,map_num INTEGER,"
                    "base_gengol INTEGER NULL,base_minchub INTEGER NULL,"
                    "base_cheryuk INTEGER NULL,base_simmek INTEGER NULL)").ok()) return false;
    if (!db.execute("INSERT INTO character_info VALUES(?,'FixtureHero',0,1,1,1,1,1,7,12,12,12,12)",
                    {mxh::db::bind(static_cast<std::int64_t>(character))}).ok()) return false;
    return db.execute("CREATE TABLE IF NOT EXISTS modern_player_item ("
                      "player_id INTEGER NOT NULL,container INTEGER NOT NULL,slot INTEGER NOT NULL,"
                      "db_idx INTEGER NOT NULL,item_idx INTEGER NOT NULL,durability INTEGER NOT NULL,"
                      "rare_idx INTEGER NOT NULL,quick_position INTEGER NOT NULL,item_param INTEGER NOT NULL,"
                      "PRIMARY KEY(player_id,container,slot),UNIQUE(player_id,db_idx))").ok();
}

// Keep SQLite integration fixtures aligned with the production GameIn
// contract.  Older tests created only the original five-column progression
// row; MapHandler now deliberately refuses to backfill missing progression or
// vitals, so those fixtures must provide the authoritative rows explicitly.
bool seed_fixture_runtime_state(mxh::db::IDbAdapter& db, std::uint32_t player_id,
                                 std::uint32_t level = 1, std::uint32_t exp = 0,
                                 std::uint32_t money = 0) {
    if (!db.execute(
            "CREATE TABLE IF NOT EXISTS modern_player_state ("
            "player_id INTEGER PRIMARY KEY,money INTEGER NOT NULL DEFAULT 0,"
            "level INTEGER NOT NULL DEFAULT 1,exp INTEGER NOT NULL DEFAULT 0,"
            "max_level INTEGER NULL,exp_flag INTEGER NULL,level_up_point INTEGER NULL,"
            "hp_mp_flag INTEGER NULL,party_id INTEGER NOT NULL DEFAULT 0,"
            "guild_id INTEGER NOT NULL DEFAULT 0,updated_at TEXT NOT NULL)").ok()) return false;
    for (const char* statement : {
             "ALTER TABLE modern_player_state ADD COLUMN max_level INTEGER NULL",
             "ALTER TABLE modern_player_state ADD COLUMN exp_flag INTEGER NULL",
             "ALTER TABLE modern_player_state ADD COLUMN level_up_point INTEGER NULL",
             "ALTER TABLE modern_player_state ADD COLUMN hp_mp_flag INTEGER NULL",
             "ALTER TABLE modern_player_state ADD COLUMN party_id INTEGER NOT NULL DEFAULT 0",
             "ALTER TABLE modern_player_state ADD COLUMN guild_id INTEGER NOT NULL DEFAULT 0"}) {
        (void)db.execute(statement);
    }
    const auto args = std::vector<mxh::db::Bind>{
        mxh::db::bind(static_cast<std::int64_t>(player_id)),
        mxh::db::bind(static_cast<std::int64_t>(money)),
        mxh::db::bind(static_cast<std::int64_t>(level)),
        mxh::db::bind(static_cast<std::int64_t>(exp))};
    if (!db.execute(
            "INSERT OR IGNORE INTO modern_player_state "
            "(player_id,money,level,exp,max_level,exp_flag,level_up_point,hp_mp_flag,updated_at) "
            "VALUES(?,?,?,?,?,?,?, ?,CURRENT_TIMESTAMP)",
            {args[0], args[1], args[2], args[3], args[2], mxh::db::bind(16),
             mxh::db::bind(0), mxh::db::bind(0)}).ok()) return false;
    if (!db.execute(
            "UPDATE modern_player_state SET max_level=COALESCE(max_level,?),"
            "exp_flag=COALESCE(exp_flag,16),level_up_point=COALESCE(level_up_point,0),"
            "hp_mp_flag=COALESCE(hp_mp_flag,0) WHERE player_id=?",
            {args[2], args[0]}).ok()) return false;
    if (!db.execute(
            "CREATE TABLE IF NOT EXISTS modern_player_vitals ("
            "player_id INTEGER PRIMARY KEY,hp INTEGER,shield INTEGER,mp INTEGER,"
            "dead INTEGER NOT NULL,initialized INTEGER NOT NULL,revision INTEGER NOT NULL,"
            "updated_at TEXT NOT NULL)").ok()) return false;
    return db.execute(
        "INSERT OR IGNORE INTO modern_player_vitals "
        "(player_id,hp,shield,mp,dead,initialized,revision,updated_at) "
        "VALUES(?,125,0,125,0,1,1,CURRENT_TIMESTAMP)", {args[0]}).ok();
}

// ===========================================================================
// Mock IDbAdapter ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â minimum viable stub for handler construction.
//
// All methods return a default-constructed DbResult (success).
// The DB-write counters are public so tests can verify that a
// handler did NOT call the DB on a no-DB code path.
class MockDbAdapter final : public mxh::db::IDbAdapter {
public:
    // The generic handler fixture models CharacterMake's pending vitals row.
    // Tests that need a loaded state explicitly seed a ready row instead.
    using VitalsRow = std::array<std::int64_t, 6>;  // hp,shield,mp,dead,initialized,revision
    using ProgressRow = std::array<std::int64_t, 7>;  // level,exp,money,max,flag,points,hpmp
    void seed_ready_vitals(std::uint32_t player_id, std::uint32_t hp,
                           std::uint32_t shield, std::uint32_t mp,
                           std::int64_t revision = 1) {
        vitals_rows[player_id] = VitalsRow{hp, shield, mp, hp == 0 ? 1 : 0, 1, revision};
        progress_rows[player_id] = ProgressRow{1, 0, 0, 1, 16, 0, 0};
    }
    mxh::db::DbResult connect(const mxh::db::ConnectionConfig&) override {
        connected = true;
        return {};
    }
    void disconnect() override { ++disconnect_count; connected = false; }
    bool is_connected() const noexcept override { return connected; }
    mxh::db::DbResult execute(std::string_view sql, std::span<const mxh::db::Bind> args) override {
        if (!connected) return {mxh::db::DbError::NotConnected, "retired mock adapter"};
        ++exec_count;
        if (sql.find("UPDATE modern_player_vitals") != std::string_view::npos) {
            const auto value = [&](std::size_t index) {
                return std::get<std::int64_t>(args[index].value);
            };
            if (sql.find("initialized=0") != std::string_view::npos) {
                auto& row = staged_vitals[value(3)];
                if (row[4] != 0 || row[5] != 0) return {mxh::db::DbError::Ok, {}, 0};
                row = VitalsRow{value(0), value(1), value(2), 0, 1, 1};
                return {mxh::db::DbError::Ok, {}, 1};
            }
            const auto id = value(4);
            const auto it = staged_vitals.find(id);
            if (it == staged_vitals.end() || it->second[5] != value(5) ||
                it->second[0] != value(6) || it->second[1] != value(7) ||
                it->second[2] != value(8) || it->second[3] != value(9)) {
                return {mxh::db::DbError::Ok, {}, 0};
            }
            it->second = VitalsRow{value(0), value(1), value(2), value(3), 1, value(5) + 1};
            return {mxh::db::DbError::Ok, {}, 1};
        }
        if (sql.find("UPDATE modern_player_state SET exp_flag=16") != std::string_view::npos) {
            const auto id = std::get<std::int64_t>(args[0].value);
            const auto it = staged_progress.find(id);
            if (it == staged_progress.end() || it->second != ProgressRow{1,0,0,1,0,0,0})
                return {mxh::db::DbError::Ok, {}, 0};
            it->second[4] = 16;
            return {mxh::db::DbError::Ok, {}, 1};
        }
        if (sql.find("UPDATE modern_player_state SET level=") != std::string_view::npos) {
            if (fail_progress_update) return {mxh::db::DbError::ConstraintViolation,
                                              "simulated progress failure"};
            const auto value = [&](std::size_t index) {
                return std::get<std::int64_t>(args[index].value);
            };
            const auto it = staged_progress.find(value(6));
            if (it == staged_progress.end() || it->second[0] != value(7) ||
                it->second[1] != value(8) || it->second[2] != value(9) ||
                it->second[3] != value(10) || it->second[4] != value(11) ||
                it->second[5] != value(12) || it->second[6] != value(13))
                return {mxh::db::DbError::Ok, {}, 0};
            it->second[0] = value(0);
            it->second[1] = value(1);
            it->second[3] = value(2);
            it->second[4] = value(3);
            it->second[5] = value(4);
            it->second[6] = value(5);
            return {mxh::db::DbError::Ok, {}, 1};
        }
        if (sql.find("UPDATE character_info SET level=") != std::string_view::npos) {
            if (fail_character_level_update)
                return {mxh::db::DbError::ConstraintViolation,
                        "simulated character level failure"};
            return {mxh::db::DbError::Ok, {}, 1};
        }
        if (sql.find("INSERT INTO modern_player_item") != std::string_view::npos) {
            ++item_insert_count;
            if (pause_item_insert) std::this_thread::sleep_for(std::chrono::milliseconds(15));
            if (args.size() > 3 && std::holds_alternative<std::int64_t>(args[3].value))
                last_item_db_idx = std::get<std::int64_t>(args[3].value);
            if (fail_item_insert)
                return {mxh::db::DbError::ConstraintViolation, "simulated item write failure"};
        }
        return {};
    }
    // R-2: if userlevel_to_return != UINT8_MAX, treat the next query
    // against "chr_log_info" as returning one row with that value in
    // column [0]. UINT8_MAX means "do not inject" (default) so other
    // tests that do not care about userlevel keep the original empty
    // result semantics.
    std::atomic<std::uint8_t> userlevel_to_return{0xFFu};
    mxh::db::DbResult query(std::string_view sql, std::span<const mxh::db::Bind> args,
                           mxh::db::ResultSet& out) override {
        if (!connected) return {mxh::db::DbError::NotConnected, "retired mock adapter"};
        ++query_count;
        out = mxh::db::ResultSet{};
        if (sql.find("FROM modern_player_vitals") != std::string_view::npos) {
            const auto id = std::get<std::int64_t>(args[0].value);
            if (missing_vitals_ids.contains(id)) return {};
            auto [it, inserted] = vitals_rows.try_emplace(id, VitalsRow{});
            (void)inserted;
            const auto& row = it->second;
            if (row[4] == 0) {
                out.rows.push_back({std::monostate{}, std::monostate{},
                                    std::monostate{}, std::int64_t{0},
                                    std::int64_t{0}, std::int64_t{0}});
            } else {
                out.rows.push_back({row[0],row[1],row[2],row[3],row[4],row[5]});
            }
            return {};
        }
        if (sql.find("modern_player_item") != std::string_view::npos &&
            sql.find("MAX") != std::string_view::npos) {
            out.columns = {"max_db_idx"};
            out.rows.push_back({std::int64_t{699999}});
            return {};
        }
        if (sql.find("FROM modern_player_state") != std::string_view::npos) {
            const auto id = std::get<std::int64_t>(args[0].value);
            if (!missing_progress_ids.contains(id)) {
                auto [it, inserted] = progress_rows.try_emplace(id,
                    ProgressRow{1, 0, 0, 1, 0, 0, 0});
                (void)inserted;
                if (partial_progress_ids.contains(id)) {
                    out.rows.push_back({it->second[0], it->second[1], it->second[2],
                        it->second[3], std::monostate{}, it->second[5], it->second[6]});
                } else {
                    out.rows.push_back({it->second[0], it->second[1], it->second[2],
                        it->second[3], it->second[4], it->second[5], it->second[6]});
                }
            }
            return {};
        }
        if (sql.find("FROM modern_player_quest_log") != std::string_view::npos &&
            preloaded_quest_id) {
            out.rows.push_back({std::int64_t{*preloaded_quest_id},
                std::int64_t{1}, std::int64_t{0}});
            return {};
        }
        if (sql.find("base_gengol,base_minchub,base_cheryuk,base_simmek") != std::string_view::npos &&
            !return_unknown_attributes) {
            out.rows.push_back({std::int64_t{12}, std::int64_t{12},
                                std::int64_t{12}, std::int64_t{12}});
        }
        std::uint8_t lvl = userlevel_to_return.load();
        if (lvl != 0xFFu && sql.find("chr_log_info") != std::string_view::npos) {
            mxh::db::ResultSet injected;
            mxh::db::Row row;
            row.push_back(static_cast<std::int64_t>(lvl));
            injected.rows.push_back(std::move(row));
            out = std::move(injected);
        }
        return {};
    }

    mxh::db::DbResult begin_transaction() override {
        if (!connected) return {mxh::db::DbError::NotConnected, "retired mock adapter"};
        if (++active_transactions > 1) transaction_overlap = true;
        staged_vitals = vitals_rows;
        staged_progress = progress_rows;
        return {};
    }
    mxh::db::DbResult commit() override {
        ++commit_count;
        if (commit_count == fail_commit_on) {
            return {mxh::db::DbError::IoError, "simulated lost commit acknowledgement"};
        }
        vitals_rows = staged_vitals;
        progress_rows = staged_progress;
        --active_transactions;
        return {};
    }
    mxh::db::DbResult rollback() override { ++rollbacks; --active_transactions; return {}; }
    std::string backend_name() const noexcept override { return "mock"; }

    // Counters for tests that want to verify "handler did NOT call
    // the DB on a no-DB code path". Public so the gtest bodies can
    // read them directly.
    std::atomic<int> exec_count{0};
    std::atomic<int> query_count{0};
    bool fail_item_insert = false;
    int rollbacks = 0;
    int item_insert_count = 0;
    std::int64_t last_item_db_idx = 0;
    bool pause_item_insert = false;
    bool return_unknown_attributes = false;
    bool fail_progress_update = false;
    bool fail_character_level_update = false;
    std::atomic<int> active_transactions{0};
    std::atomic<bool> transaction_overlap{false};
    int commit_count = 0;
    int fail_commit_on = -1;
    int disconnect_count = 0;
    bool connected = true;
    std::unordered_map<std::int64_t, VitalsRow> vitals_rows;
    std::unordered_map<std::int64_t, VitalsRow> staged_vitals;
    std::unordered_map<std::int64_t, ProgressRow> progress_rows;
    std::unordered_map<std::int64_t, ProgressRow> staged_progress;
    std::unordered_set<std::int64_t> missing_vitals_ids;
    std::unordered_set<std::int64_t> missing_progress_ids;
    std::unordered_set<std::int64_t> partial_progress_ids;
    std::optional<std::uint32_t> preloaded_quest_id;
};

class CharacterCreateDb final : public mxh::db::IDbAdapter {
public:
    mxh::db::DbResult connect(const mxh::db::ConnectionConfig&) override { return {}; }
    void disconnect() override { connected = false; transaction_active = false; ++disconnects; }
    bool is_connected() const noexcept override { return connected; }
    mxh::db::DbResult query(std::string_view sql, std::span<const mxh::db::Bind>,
                            mxh::db::ResultSet& out) override {
        {
            std::lock_guard<std::mutex> lock(pause_mu);
            if (transaction_active && transaction_owner != std::this_thread::get_id())
                ++foreign_reads_during_transaction;
        }
        out = {};
        if (sql.find("modern_player_item") != std::string_view::npos &&
            sql.find("MAX") != std::string_view::npos) {
            out.columns = {"max_db_idx"};
            out.rows.push_back({std::int64_t{699999}});
        }
        return {};
    }
    mxh::db::DbResult execute(std::string_view sql,
                               std::span<const mxh::db::Bind> params) override {
        if (sql.find("INSERT INTO character_info") != std::string_view::npos) {
            ++character_inserts;
            if (params.size() > 10 &&
                std::holds_alternative<std::int64_t>(params[10].value)) {
                created_map_num = std::get<std::int64_t>(params[10].value);
            }
            if (pause_character_insert) {
                std::unique_lock<std::mutex> lock(pause_mu);
                character_insert_waiting = true;
                pause_cv.notify_all();
                pause_cv.wait(lock, [&] { return release_character_insert; });
            }
            return {mxh::db::DbError::Ok, {}, 1};
        } else if (sql.find("INSERT INTO modern_character_equipment") !=
                   std::string_view::npos) {
            ++equipment_inserts;
            if (sql.find("updated_at") == std::string_view::npos ||
                sql.find("CURRENT_TIMESTAMP") == std::string_view::npos ||
                params.size() != 3) {
                return {mxh::db::DbError::ConstraintViolation,
                        "updated_at is required"};
            }
            if (fail_equipment) {
                return {mxh::db::DbError::ConstraintViolation,
                        "simulated equipment failure"};
            }
            return {mxh::db::DbError::Ok, {}, 1};
        } else if (sql.find("INSERT INTO modern_player_vitals") !=
                   std::string_view::npos) {
            ++pending_vitals_inserts;
            if (fail_pending_vitals) {
                return {mxh::db::DbError::ConstraintViolation,
                        "simulated pending vitals failure"};
            }
            return {mxh::db::DbError::Ok, {}, 1};
        } else if (sql.find("INSERT INTO modern_player_item") !=
                   std::string_view::npos) {
            return {mxh::db::DbError::Ok, {}, 1};
        } else if (sql.find("INSERT INTO modern_player_state") !=
                   std::string_view::npos) {
            ++progress_inserts;
            if (fail_progress) {
                return {mxh::db::DbError::ConstraintViolation,
                        "simulated progress failure"};
            }
            return {mxh::db::DbError::Ok, {}, 1};
        }
        return {};
    }
    mxh::db::DbResult begin_transaction() override {
        ++begins;
        std::lock_guard<std::mutex> lock(pause_mu);
        if (transaction_active) return {mxh::db::DbError::ConstraintViolation,
                                        "another transaction already owns adapter"};
        transaction_active = true;
        transaction_owner = std::this_thread::get_id();
        return {};
    }
    mxh::db::DbResult commit() override {
        ++commits;
        if (fail_commit) return {mxh::db::DbError::IoError,
                                 "simulated unknown commit"};
        transaction_active = false;
        return {};
    }
    mxh::db::DbResult rollback() override {
        ++rollbacks;
        if (fail_rollback) return {mxh::db::DbError::IoError,
                                   "simulated rollback failure"};
        transaction_active = false;
        return {};
    }
    std::string backend_name() const noexcept override { return "test"; }
    bool fail_equipment = false;
    bool fail_pending_vitals = false;
    bool fail_progress = false;
    bool fail_commit = false;
    bool fail_rollback = false;
    bool connected = true;
    int disconnects = 0;
    int character_inserts = 0;
    int equipment_inserts = 0;
    int progress_inserts = 0;
    int pending_vitals_inserts = 0;
    std::optional<std::int64_t> created_map_num;
    std::atomic<int> begins{0};
    std::atomic<int> commits{0};
    std::atomic<int> rollbacks{0};
    std::mutex pause_mu;
    std::condition_variable pause_cv;
    bool pause_character_insert = false;
    bool character_insert_waiting = false;
    bool release_character_insert = false;
    std::atomic<bool> transaction_active{false};
    std::thread::id transaction_owner{};
    std::atomic<int> foreign_reads_during_transaction{0};
};

// Helper: a ReplyFn-compatible spy that counts calls.
struct ReplySpy {
    std::atomic<int> call_count{0};
    mxh::net::ConnectionId last_id{};
    mxh::net::Message last_message{};
    std::vector<mxh::net::Message> messages;
};

inline mxh::server::ReplyFn make_reply_spy(ReplySpy& spy) {
    return [&spy](mxh::net::ConnectionId id, const mxh::net::Message& message) {
        ++spy.call_count;
        spy.last_id = id;
        spy.last_message = message;
        spy.messages.push_back(message);
    };
}

mxh::net::Message character_make_with_default_clothes() {
    mxh::net::Message msg;
    msg.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    msg.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::CharacterMakeSyn);
    msg.payload.resize(59, 0);
    const char name[] = "MoxSuit";
    std::memcpy(msg.payload.data(), name, sizeof(name));
    const std::uint32_t user_id = 42;
    std::memcpy(msg.payload.data() + 17, &user_id, sizeof(user_id));
    const std::uint16_t cloth = 23000;
    const std::uint16_t boot = 27000;
    std::memcpy(msg.payload.data() + 30 + 2 * 2, &cloth, sizeof(cloth));
    std::memcpy(msg.payload.data() + 30 + 3 * 2, &boot, sizeof(boot));
    msg.payload[25] = 17;
    return msg;
}

class RegisteredMapSender final : public mxh::net::ITcpSender {
public:
    [[nodiscard]] mxh::net::NetError send(const mxh::net::Message&) override {
        return mxh::net::NetError::Ok;
    }
    [[nodiscard]] bool is_connected() const noexcept override { return true; }
};

void register_birth_map(mxh::server::AgentHandler& handler,
                        RegisteredMapSender& sender,
                        std::uint16_t map_num = 12) {
    handler.set_map_server_for_map(
        map_num, &sender, mxh::net::make_connection_id(17000u + map_num));
}

TEST(AgentCharacterCreateTest, PersistsDefaultClothesBeforeAcknowledging) {
    CharacterCreateDb db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply), true);
    RegisteredMapSender birth;
    register_birth_map(handler, birth);
    handler.on_message(mxh::net::make_connection_id(9001),
                       character_make_with_default_clothes());
    EXPECT_EQ(db.begins, 1);
    EXPECT_EQ(db.character_inserts, 1);
    EXPECT_EQ(db.progress_inserts, 1);
    EXPECT_EQ(db.equipment_inserts, 2);
    EXPECT_EQ(db.pending_vitals_inserts, 1);
    EXPECT_EQ(db.commits, 1);
    EXPECT_EQ(db.rollbacks, 0);
    ASSERT_FALSE(reply.messages.empty());
    EXPECT_NE(reply.last_message.header.protocol, static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::CharacterMakeNack));
}

TEST(AgentCharacterCreateTest, EquipmentFailureRollsBackAndRejectsCreation) {
    CharacterCreateDb db;
    db.fail_equipment = true;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply), true);
    RegisteredMapSender birth;
    register_birth_map(handler, birth);
    handler.on_message(mxh::net::make_connection_id(9002),
                       character_make_with_default_clothes());
    EXPECT_EQ(db.begins, 1);
    EXPECT_EQ(db.character_inserts, 1);
    EXPECT_EQ(db.progress_inserts, 1);
    EXPECT_EQ(db.equipment_inserts, 1);
    EXPECT_EQ(db.pending_vitals_inserts, 0);
    EXPECT_EQ(db.commits, 0);
    EXPECT_EQ(db.rollbacks, 1);
    ASSERT_EQ(reply.messages.size(), 1u);
    EXPECT_EQ(reply.last_message.header.protocol, static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::CharacterMakeNack));
}

TEST(AgentCharacterCreateTest, PendingVitalsFailureRollsBackCharacterAndClothes) {
    CharacterCreateDb db;
    db.fail_pending_vitals = true;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply), true);
    RegisteredMapSender birth;
    register_birth_map(handler, birth);
    handler.on_message(mxh::net::make_connection_id(9003),
                       character_make_with_default_clothes());
    EXPECT_EQ(db.begins, 1);
    EXPECT_EQ(db.character_inserts, 1);
    EXPECT_EQ(db.progress_inserts, 1);
    EXPECT_EQ(db.equipment_inserts, 2);
    EXPECT_EQ(db.pending_vitals_inserts, 1);
    EXPECT_EQ(db.commits, 0);
    EXPECT_EQ(db.rollbacks, 1);
    ASSERT_EQ(reply.messages.size(), 1u);
    EXPECT_EQ(reply.last_message.header.protocol, static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::CharacterMakeNack));
}

TEST(AgentCharacterCreateTest, ProgressFailureRollsBackCharacterBeforeClothes) {
    CharacterCreateDb db;
    db.fail_progress = true;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply), true);
    RegisteredMapSender birth;
    register_birth_map(handler, birth);
    handler.on_message(mxh::net::make_connection_id(9006),
                       character_make_with_default_clothes());
    EXPECT_EQ(db.begins, 1);
    EXPECT_EQ(db.character_inserts, 1);
    EXPECT_EQ(db.progress_inserts, 1);
    EXPECT_EQ(db.equipment_inserts, 0);
    EXPECT_EQ(db.pending_vitals_inserts, 0);
    EXPECT_EQ(db.commits, 0);
    EXPECT_EQ(db.rollbacks, 1);
    ASSERT_EQ(reply.messages.size(), 1u);
    EXPECT_EQ(reply.last_message.header.protocol, static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::CharacterMakeNack));
}

TEST(AgentCharacterCreateTest, UnknownCommitRetiresAdapterWithoutFalseNack) {
    CharacterCreateDb db;
    db.fail_commit = true;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply), true);
    RegisteredMapSender birth;
    register_birth_map(handler, birth);
    handler.on_message(mxh::net::make_connection_id(9004),
                       character_make_with_default_clothes());
    EXPECT_EQ(db.begins, 1);
    EXPECT_EQ(db.commits, 1);
    EXPECT_EQ(db.disconnects, 1);
    EXPECT_FALSE(db.is_connected());
    EXPECT_TRUE(reply.messages.empty()) << "lost commit reply cannot be called a Nack";
}

TEST(AgentCharacterCreateTest, FailedRollbackRetiresAdapterWithoutFalseNack) {
    CharacterCreateDb db;
    db.fail_pending_vitals = true;
    db.fail_rollback = true;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply), true);
    RegisteredMapSender birth;
    register_birth_map(handler, birth);
    handler.on_message(mxh::net::make_connection_id(9005),
                       character_make_with_default_clothes());
    EXPECT_EQ(db.rollbacks, 1);
    EXPECT_EQ(db.disconnects, 1);
    EXPECT_TRUE(reply.messages.empty());
}

TEST(AgentCharacterCreateTest, SharedAdapterSerializesCreationFriendTxnAndSessionRead) {
    CharacterCreateDb db;
    db.pause_character_insert = true;
    std::mutex reply_mu;
    std::vector<mxh::net::Message> replies;
    mxh::server::AgentHandler handler(db,
        [&](mxh::net::ConnectionId, const mxh::net::Message& msg) {
            std::lock_guard<std::mutex> lock(reply_mu);
            replies.push_back(msg);
        }, true);
    RegisteredMapSender birth;
    register_birth_map(handler, birth);
    const auto friend_connection = mxh::net::make_connection_id(9101);
    handler.register_session(friend_connection, 1u, 123u, 10u, 0u);
    std::thread creator([&] {
        handler.on_message(mxh::net::make_connection_id(9102),
                           character_make_with_default_clothes());
    });
    {
        std::unique_lock<std::mutex> lock(db.pause_mu);
        const bool entered = db.pause_cv.wait_for(lock, std::chrono::seconds(2),
            [&] { return db.character_insert_waiting; });
        if (!entered) {
            db.release_character_insert = true;
            lock.unlock();
            db.pause_cv.notify_all();
            creator.join();
            FAIL() << "CharacterMake did not reach its in-transaction pause";
        }
    }

    mxh::net::Message friend_del;
    friend_del.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Friend);
    friend_del.header.protocol = 8u; // FriendDelSyn, separate client connection
    friend_del.header.object_id = 456u;
    std::promise<void> friend_started, friend_done, read_started, read_done;
    auto friend_started_future = friend_started.get_future();
    auto friend_done_future = friend_done.get_future();
    auto read_started_future = read_started.get_future();
    auto read_done_future = read_done.get_future();
    std::thread friend_thread([&] {
        friend_started.set_value();
        handler.on_message(friend_connection, friend_del);
        friend_done.set_value();
    });
    std::thread reader([&] {
        read_started.set_value();
        handler.register_session(mxh::net::make_connection_id(9103),
                                 2u, 789u, 10u, 0u);
        read_done.set_value();
    });
    friend_started_future.wait();
    read_started_future.wait();
    // Both calls have been launched while the first transaction is paused.
    // A second begin/rollback or query on the same adapter must wait for the
    // entire creation handler to finish, not merely for one SQL statement.
    const auto friend_wait = friend_done_future.wait_for(std::chrono::milliseconds(50));
    const auto read_wait = read_done_future.wait_for(std::chrono::milliseconds(50));
    const int begins_while_paused = db.begins;
    const int rollbacks_while_paused = db.rollbacks;
    const int foreign_reads_while_paused = db.foreign_reads_during_transaction;
    {
        std::lock_guard<std::mutex> lock(db.pause_mu);
        db.release_character_insert = true;
    }
    db.pause_cv.notify_all();
    creator.join();
    friend_thread.join();
    reader.join();
    EXPECT_EQ(friend_wait, std::future_status::timeout);
    EXPECT_EQ(read_wait, std::future_status::timeout);
    EXPECT_EQ(begins_while_paused, 1);
    EXPECT_EQ(rollbacks_while_paused, 0);
    EXPECT_EQ(foreign_reads_while_paused, 0);
    EXPECT_EQ(db.begins, 2);
    EXPECT_EQ(db.commits, 2);
    EXPECT_EQ(db.rollbacks, 0);
    EXPECT_EQ(db.character_inserts, 1);
    EXPECT_EQ(db.progress_inserts, 1);
    EXPECT_EQ(db.pending_vitals_inserts, 1);
    EXPECT_TRUE(db.is_connected());
    EXPECT_EQ(std::count_if(replies.begin(), replies.end(), [](const auto& msg) {
        return msg.header.category == static_cast<std::uint8_t>(
                   mxh::proto::Category::UserConn) &&
               msg.header.protocol == static_cast<std::uint8_t>(
                   mxh::proto::UserConnProtocol::CharacterListAck);
    }), 1);
    EXPECT_EQ(std::count_if(replies.begin(), replies.end(), [](const auto& msg) {
        return msg.header.category == static_cast<std::uint8_t>(
                   mxh::proto::Category::UserConn) &&
               msg.header.protocol == static_cast<std::uint8_t>(
                   mxh::proto::UserConnProtocol::CharacterMakeNack);
    }), 0);
}

// ===========================================================================
// MockTcpSender ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â Phase 12.1 P2-13: capture outgoing messages from
// AgentHandler without spinning up a real socket. Tests verify that
// GameOutSyn / GameInSyn forwarding actually fires by reading sent_msgs_.
// ===========================================================================
class MockTcpSender final : public mxh::net::ITcpSender {
public:
    [[nodiscard]] mxh::net::NetError send(const mxh::net::Message& msg) override {
        ++send_count;
        last_message = msg;
        if (!connected || fail_send) return mxh::net::NetError::SendFailed;
        sent_msgs.push_back(msg);
        if (on_send) on_send(msg);
        return mxh::net::NetError::Ok;
    }
    [[nodiscard]] bool is_connected() const noexcept override { return connected; }

    // Tests can flip connected ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¾ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ false to simulate the map server going
    // down between set_map_server() and on_disconnect().
    void set_connected(bool c) noexcept { connected = c; }
    void set_fail_send(bool f) noexcept { fail_send = f; }

    std::atomic<int> send_count{0};
    std::vector<mxh::net::Message> sent_msgs;
    mxh::net::Message last_message{};
    std::function<void(const mxh::net::Message&)> on_send;
private:
    bool connected = true;
    bool fail_send = false;
};

TEST(AgentCharacterCreateTest, PersistsCanonicalBirthMapWhenRouteIsRegistered) {
    CharacterCreateDb db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply), true,
                                      false, {}, /*default_map_num=*/10,
                                      /*configured_start_area=*/17);
    MockTcpSender map17;
    handler.set_map_server_for_map(17, &map17, mxh::net::make_connection_id(17017));

    auto request = character_make_with_default_clothes();
    ASSERT_GE(request.payload.size(), 26u);
    request.payload[25] = 17;  // canonical StartArea from LoginPoint.bin
    handler.on_message(mxh::net::make_connection_id(9017), request);

    ASSERT_TRUE(db.created_map_num.has_value());
    EXPECT_EQ(*db.created_map_num, 17);
    ASSERT_FALSE(reply.messages.empty());
    EXPECT_NE(reply.last_message.header.protocol,
              static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::CharacterMakeNack));
}

class MapHandlerForwardingSender final : public mxh::net::ITcpSender {
public:
    MapHandlerForwardingSender(mxh::server::MapHandler& target,
                               mxh::net::ConnectionId connection)
        : target_(target), connection_(connection) {}

    [[nodiscard]] mxh::net::NetError send(
        const mxh::net::Message& msg) override {
        if (!connected) return mxh::net::NetError::SendFailed;
        ++send_count;
        target_.on_message(connection_, msg);
        return mxh::net::NetError::Ok;
    }
    [[nodiscard]] bool is_connected() const noexcept override {
        return connected;
    }

    bool connected = true;
    std::atomic<int> send_count{0};

private:
    mxh::server::MapHandler& target_;
    mxh::net::ConnectionId connection_;
};

// ===========================================================================
// LoginHandler
// ===========================================================================

TEST(LoginHandlerTest, DefaultConstructionDoesNotCrash) {
    MockDbAdapter db;
    mxh::server::LoginHandler handler(db, "127.0.0.1", 7000,
                                       mxh::server::ReplyFn{});
    SUCCEED();
}

TEST(LoginHandlerTest, LegacyFlagIsOptional) {
    // The 4-arg form (without use_legacy_framing) and the 5-arg form
    // both work. use_legacy_framing defaults to false.
    MockDbAdapter db;
    mxh::server::LoginHandler h1(db, "127.0.0.1", 7000, mxh::server::ReplyFn{});
    mxh::server::LoginHandler h2(db, "127.0.0.1", 7000, mxh::server::ReplyFn{}, true);
    mxh::server::LoginHandler h3(db, "127.0.0.1", 7000, mxh::server::ReplyFn{}, false);
    SUCCEED();
}

TEST(LoginHandlerTest, OnConnectReturnsTrue) {
    // The default on_connect returns true. The login handler does not
    // override it, so we get the base-class true. In non-legacy mode
    // on_connect is just a logging hook (no reply_, no DB query).
    MockDbAdapter db;
    mxh::server::LoginHandler handler(db, "127.0.0.1", 7000,
                                       mxh::server::ReplyFn{});
    EXPECT_TRUE(handler.on_connect({}, "192.168.1.1:12345"));
}

TEST(LoginHandlerTest, OnDisconnectDoesNotCrash) {
    // The default on_disconnect is a no-op. Verify it does not call
    // reply_ or the DB.
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::LoginHandler handler(db, "127.0.0.1", 7000,
                                       make_reply_spy(reply));
    handler.on_disconnect({}, mxh::net::NetError::Disconnected);
    EXPECT_EQ(reply.call_count.load(), 0);
    EXPECT_EQ(db.exec_count.load(), 0);
    EXPECT_EQ(db.query_count.load(), 0);
}

// ===========================================================================
// AgentHandler
// ===========================================================================

TEST(AgentHandlerTest, DefaultConstructionDoesNotCrash) {
    MockDbAdapter db;
    mxh::server::AgentHandler handler(db, mxh::server::ReplyFn{});
    SUCCEED();
}

TEST(AgentHandlerTest, LegacyFlagIsOptional) {
    MockDbAdapter db;
    mxh::server::AgentHandler h1(db, mxh::server::ReplyFn{});
    mxh::server::AgentHandler h2(db, mxh::server::ReplyFn{}, true);
    SUCCEED();
}

TEST(AgentHandlerTest, GetMapConnectionDefaultsToInvalid) {
    // Before set_map_server() is called, the map connection should
    // be the invalid (zero) ConnectionId.
    MockDbAdapter db;
    mxh::server::AgentHandler handler(db, mxh::server::ReplyFn{});
    auto map_id = handler.get_map_connection();
    EXPECT_FALSE(map_id.valid());
    EXPECT_EQ(map_id.value, 0u);
}

TEST(AgentHandlerTest, SetMapServerStoresConnection) {
    // After set_map_server(client, conn), get_map_connection() must
    // return that conn. The TcpClient pointer is stored as-is (no
    // copy), so we pass nullptr to avoid any real network setup.
    MockDbAdapter db;
    mxh::server::AgentHandler handler(db, mxh::server::ReplyFn{});

    auto map_conn = mxh::net::make_connection_id(42);
    handler.set_map_server(nullptr, map_conn);

    auto retrieved = handler.get_map_connection();
    EXPECT_TRUE(retrieved.valid());
    EXPECT_EQ(retrieved.value, 42u);
}

TEST(AgentHandlerTest, SetMapServerOverwritesPrevious) {
    // Calling set_map_server twice replaces the previous connection.
    MockDbAdapter db;
    mxh::server::AgentHandler handler(db, mxh::server::ReplyFn{});

    handler.set_map_server(nullptr, mxh::net::make_connection_id(100));
    EXPECT_EQ(handler.get_map_connection().value, 100u);

    handler.set_map_server(nullptr, mxh::net::make_connection_id(200));
    EXPECT_EQ(handler.get_map_connection().value, 200u);
}

TEST(AgentHandlerTest, OnConnectNonLegacyDoesNotCallReply) {
    // In non-legacy mode, on_connect just logs and returns true.
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    EXPECT_TRUE(handler.on_connect({}, "10.0.0.1:9999"));
    EXPECT_EQ(reply.call_count.load(), 0);
    EXPECT_EQ(db.query_count.load(), 0);
}

TEST(AgentHandlerTest, OnDisconnectDoesNotCrash) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    handler.on_disconnect({}, mxh::net::NetError::Disconnected);
    EXPECT_EQ(reply.call_count.load(), 0);
    EXPECT_EQ(db.exec_count.load(), 0);
    EXPECT_EQ(db.query_count.load(), 0);
}

TEST(AgentHandlerTest, OnDisconnectWithoutMapServerDoesNotCrash) {
    // Phase 12.1: GameOutSyn forwarding only fires when a TcpClient*
    // has been set via set_map_server(). With no map server attached,
    // on_disconnect must still clean up local state and not deref
    // any null TcpClient.
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    handler.on_disconnect(mxh::net::make_connection_id(99),
                          mxh::net::NetError::Disconnected);
    EXPECT_EQ(reply.call_count.load(), 0);
    // No DB calls either ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â on_disconnect doesn't persist anything.
    EXPECT_EQ(db.exec_count.load(), 0);
    EXPECT_EQ(db.query_count.load(), 0);
}

TEST(AgentHandlerTest, OnDisconnectWithMapServerNullptrDoesNotCrash) {
    // Phase 12.1: even if set_map_server() was called with a
    // non-null ConnectionId but a null TcpClient* (e.g. test setup
    // or a transient race where the TcpClient was destroyed but the
    // connection id is still set), on_disconnect must not deref the
    // null pointer. It should log a "no MapServer connection" debug
    // line and continue cleanly.
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    handler.set_map_server(/*client=*/nullptr,
                           mxh::net::make_connection_id(7));
    handler.on_disconnect(mxh::net::make_connection_id(100),
                          mxh::net::NetError::Disconnected);
    // No reply was sent (no client, no map_client_ connected).
    EXPECT_EQ(reply.call_count.load(), 0);
    // TcpClient::send must NOT have been called (we'd crash otherwise
    // since the pointer is null and the code checks for it before
    // calling send).
    SUCCEED();
}

// ---------------------------------------------------------------------------
// Phase 12.1 P2-13: ITcpSender injection ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â verify GameOutSyn forwarding
// actually fires when a connected MockTcpSender is wired in. Before this
// refactor, AgentHandler held a concrete TcpClient* and there was no way
// to test "did the agent actually call send()?" without a live map server.
//
// Note: full "GameOutSyn sent" verification requires populating the
// private conn_user_ids_ / conn_char_ids_ / conn_map_nums_ maps, which
// has no public setter (the maps are populated by the legacy character
// select path). The tests below cover the early-return / null /
// disconnected paths; the "full send" path is covered by
// `test_map_integration.py` in the integration test (Phase 9).
// ---------------------------------------------------------------------------

TEST(AgentHandlerTest, OnDisconnectWithMockSenderNoSessionDoesNotSend) {
    // Mock sender is wired in but no user/char/map_num session has been
    // registered on the handler. on_disconnect must clean up state
    // without sending anything (the early-return path: removed_char_id
    // == 0 means we have nothing to tell the map server).
    MockDbAdapter db;
    ReplySpy reply;
    MockTcpSender mock;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    handler.set_map_server(&mock, mxh::net::make_connection_id(42));
    handler.on_disconnect(mxh::net::make_connection_id(100),
                          mxh::net::NetError::Disconnected);
    // Mock sender.send was never reached because the early-return fires
    // when there is no recorded session to forward.
    EXPECT_EQ(mock.send_count.load(), 0);
    EXPECT_TRUE(mock.sent_msgs.empty());
    EXPECT_EQ(reply.call_count.load(), 0);
}

TEST(AgentHandlerTest, OnDisconnectWithMockSenderDisconnectedSenderNoSend) {
    // Mock sender is wired in but the sender reports !is_connected().
    // on_disconnect must skip the send entirely (the code checks
    // is_connected() before calling send). Even if a session were
    // registered, no message should be delivered.
    MockDbAdapter db;
    ReplySpy reply;
    MockTcpSender mock;
    mock.set_connected(false);
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    handler.set_map_server(&mock, mxh::net::make_connection_id(42));
    handler.on_disconnect(mxh::net::make_connection_id(100),
                          mxh::net::NetError::Disconnected);
    EXPECT_EQ(mock.send_count.load(), 0);
    EXPECT_TRUE(mock.sent_msgs.empty());
}

TEST(AgentHandlerTest, ForwardFromMapWithMockSenderNoRoute) {
    // forward_from_map with a char_id that has no registered client
    // must not crash and not call reply (just a no-op debug log).
    MockDbAdapter db;
    ReplySpy reply;
    MockTcpSender mock;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    handler.set_map_server(&mock, mxh::net::make_connection_id(42));
    mxh::net::Message msg;
    msg.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Move);
    msg.header.object_id = 9999;  // char_id with no registered client
    handler.forward_from_map(mxh::net::make_connection_id(42), msg);
    EXPECT_EQ(reply.call_count.load(), 0);
}

TEST(AgentHandlerTest, ForwardFromMapRoutesPartyAndGuildToOffMapSession) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    handler.register_session(mxh::net::make_connection_id(100), 1u, 123u, 10u);
    handler.register_session(mxh::net::make_connection_id(101), 2u, 456u, 12u);

    for (const auto category : {mxh::proto::Category::Party,
                                mxh::proto::Category::Guild}) {
        mxh::net::Message msg;
        msg.header.category = static_cast<std::uint8_t>(category);
        msg.header.protocol = 0; // Info
        msg.header.object_id = 456u;
        msg.payload = {0x2a, 0x00, 0x00, 0x00, 0x01};
        handler.forward_from_map(mxh::net::make_connection_id(77), msg);
        ASSERT_EQ(reply.last_id.value, 101u);
        EXPECT_EQ(reply.last_message.header.category,
                  static_cast<std::uint8_t>(category));
        EXPECT_EQ(reply.last_message.header.object_id, 456u);
        EXPECT_EQ(reply.last_message.payload.size(), 5u);
    }
    EXPECT_EQ(reply.call_count.load(), 2);
}

TEST(AgentHandlerTest, RejectsFriendRequestForOfflineTarget) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    handler.register_session(mxh::net::make_connection_id(100), 1u, 123u, 10u);

    mxh::net::Message request;
    request.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Friend);
    request.header.protocol = 0; // FriendAddSyn
    request.header.object_id = 456u;
    handler.on_message(mxh::net::make_connection_id(100), request);

    ASSERT_EQ(reply.call_count.load(), 1);
    EXPECT_EQ(reply.last_message.header.category,
              static_cast<std::uint8_t>(mxh::proto::Category::Friend));
    EXPECT_EQ(reply.last_message.header.protocol, 2u);
    EXPECT_EQ(reply.last_message.header.object_id, 456u);
}

TEST(AgentHandlerTest, CompletesOnlineFriendInviteHandshake) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    handler.register_session(mxh::net::make_connection_id(100), 1u, 123u, 10u);
    handler.register_session(mxh::net::make_connection_id(101), 2u, 456u, 10u);

    mxh::net::Message add;
    add.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Friend);
    add.header.protocol = 0;
    add.header.object_id = 456u;
    handler.on_message(mxh::net::make_connection_id(100), add);
    ASSERT_EQ(reply.messages.size(), 2u);
    EXPECT_EQ(reply.messages[0].header.protocol, 3u);
    EXPECT_EQ(reply.messages[0].header.object_id, 123u);
    EXPECT_EQ(reply.messages[1].header.protocol, 1u);

    mxh::net::Message accept;
    accept.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Friend);
    accept.header.protocol = 4;
    accept.header.object_id = 123u;
    handler.on_message(mxh::net::make_connection_id(101), accept);
    ASSERT_EQ(reply.messages.size(), 4u);
    EXPECT_EQ(reply.messages[2].header.protocol, 5u);
    EXPECT_EQ(reply.messages[2].header.object_id, 456u);
    EXPECT_EQ(reply.messages[3].header.protocol, 5u);
    EXPECT_EQ(reply.messages[3].header.object_id, 123u);
}

TEST(AgentHandlerTest, DisconnectRemovesOffMapSocialRoute) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    handler.register_session(mxh::net::make_connection_id(101), 2u, 456u, 12u);
    MockTcpSender map_sender;
    handler.set_map_server(&map_sender, mxh::net::make_connection_id(77));
    handler.on_disconnect(mxh::net::make_connection_id(101), mxh::net::NetError::Disconnected);

    mxh::net::Message msg;
    msg.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Party);
    msg.header.protocol = static_cast<std::uint8_t>(mxh::proto::PartyProtocol::Info);
    msg.header.object_id = 456u;
    msg.payload = {0x2a, 0x00, 0x00, 0x00, 0x01};
    handler.forward_from_map(mxh::net::make_connection_id(77), msg);
    EXPECT_EQ(reply.call_count.load(), 0);
}

TEST(AgentHandlerTest, SetMapServerAcceptsITcpSender) {
    // Phase 12.1 P2-13: set_map_server now takes ITcpSender* (was
    // TcpClient*). A MockTcpSender must be accepted without conversion
    // and the handler must remember it. The test verifies the
    // signature change is real by passing a non-TcpClient through.
    MockDbAdapter db;
    ReplySpy reply;
    MockTcpSender mock;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    // Compile-time check: this line is only valid if set_map_server
    // takes ITcpSender* (or something more permissive). If the type
    // regresses to TcpClient*, the implicit upcast fails and the
    // test won't compile.
    handler.set_map_server(&mock, mxh::net::make_connection_id(7));
    EXPECT_EQ(handler.get_map_connection().value, 7u);
}

// ---------------------------------------------------------------------------
// Phase 12.1 P2-13 follow-up: register_session() + complete GameOutSyn
// forwarding test. The previous "no session" tests covered the early-return
// path; register_session lets us populate conn_user_ids_/conn_char_ids_/
// conn_map_nums_/char_to_client_ directly so the on_disconnect handler
// fires the real forward-GameOutSyn path and the mock sender captures it.
// ---------------------------------------------------------------------------

TEST(AgentHandlerTest, RegisterSessionStoresUserCharMap) {
    // register_session is the production-meaningful entry point that
    // populates the four session maps without going through the binary
    // protocol. It is also what tests use to drive the on_disconnect
    // forwarding path. Verify all three state slots are reachable.
    MockDbAdapter db;
    mxh::server::AgentHandler handler(db, mxh::server::ReplyFn{});
    handler.register_session(
        mxh::net::make_connection_id(100),
        /*user_id=*/3001, /*char_id=*/450035712, /*map_num=*/12);
    // on_disconnect should now be able to look up the session for conn=100.
    // We verify by triggering a real forward: a connected MockTcpSender
    // must see a GameOutSyn message after on_disconnect.
    MockTcpSender mock;
    handler.set_map_server(&mock, mxh::net::make_connection_id(42));
    handler.on_disconnect(mxh::net::make_connection_id(100),
                          mxh::net::NetError::Disconnected);
    EXPECT_EQ(mock.send_count.load(), 1);
    ASSERT_EQ(mock.sent_msgs.size(), 1u);
    const auto& fwd = mock.sent_msgs[0];
    EXPECT_EQ(static_cast<int>(fwd.header.category),
              static_cast<int>(mxh::proto::Category::UserConn));
    // GameOutSyn protocol id is 33 (per AgentHandler.cpp:244).
    EXPECT_EQ(fwd.header.protocol, 31);  // GameOutSyn = 31 per protocol.hpp
    EXPECT_EQ(fwd.header.object_id, 450035712u);
    // payload = wMapNum(2B) + bIsExiting=1(1B) + padding(5B) = 8B
    ASSERT_EQ(fwd.payload.size(), 8u);
    const std::uint16_t map_in_payload =
        static_cast<std::uint16_t>(fwd.payload[0]) |
        (static_cast<std::uint16_t>(fwd.payload[1]) << 8);
    EXPECT_EQ(map_in_payload, 12u);
    EXPECT_EQ(fwd.payload[2], 1);  // bIsExiting
}

TEST(AgentHandlerTest, RegisterSessionOverridesPriorSession) {
    // Calling register_session twice for the same conn_id replaces the
    // previous entry. The new char_id wins (so the forwarded message
    // uses the new char_id as the object_id).
    MockDbAdapter db;
    mxh::server::AgentHandler handler(db, mxh::server::ReplyFn{});
    handler.register_session(mxh::net::make_connection_id(100),
                              3001, 450035712, 12);
    handler.register_session(mxh::net::make_connection_id(100),
                              3001, 99999, 7);
    MockTcpSender mock;
    handler.set_map_server(&mock, mxh::net::make_connection_id(42));
    handler.on_disconnect(mxh::net::make_connection_id(100),
                          mxh::net::NetError::Disconnected);
    EXPECT_EQ(mock.send_count.load(), 1);
    EXPECT_EQ(mock.sent_msgs[0].header.object_id, 99999u);
    const std::uint16_t map_in_payload =
        static_cast<std::uint16_t>(mock.sent_msgs[0].payload[0]) |
        (static_cast<std::uint16_t>(mock.sent_msgs[0].payload[1]) << 8);
    EXPECT_EQ(map_in_payload, 7u);
}

TEST(AgentHandlerTest, RegisterSessionIsNoOpForUnknownConn) {
    // If we disconnect a conn_id that was never registered, no
    // forwarding happens. Same as the pre-P2-13 OnDisconnectWith-
    // MockSenderNoSessionDoesNotSend test, but exercises the path
    // AFTER a different conn was registered (proving the maps are
    // keyed by conn_id correctly).
    MockDbAdapter db;
    mxh::server::AgentHandler handler(db, mxh::server::ReplyFn{});
    handler.register_session(mxh::net::make_connection_id(100),
                              3001, 450035712, 12);
    MockTcpSender mock;
    handler.set_map_server(&mock, mxh::net::make_connection_id(42));
    handler.on_disconnect(mxh::net::make_connection_id(999),  // never registered
                          mxh::net::NetError::Disconnected);
    EXPECT_EQ(mock.send_count.load(), 0);
    EXPECT_TRUE(mock.sent_msgs.empty());
}

TEST(AgentHandlerTest, GameInUsesMapSpecificServerRoute) {
    MockDbAdapter db;
    mxh::server::AgentHandler handler(db, mxh::server::ReplyFn{}, true,
                                      false, {}, /*default_map_num=*/12);
    const auto connection = mxh::net::make_connection_id(1100);
    handler.register_session(connection, 3001u, 450035712u, 10u);

    MockTcpSender default_map;
    MockTcpSender target_map;
    handler.set_map_server(&default_map, mxh::net::make_connection_id(12));
    handler.set_map_server_for_map(10u, &target_map,
                                   mxh::net::make_connection_id(10));

    mxh::net::Message game_in;
    game_in.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::GameInSyn);
    game_in.header.object_id = 450035712u;
    game_in.payload.resize(8, 0);
    handler.on_message(connection, game_in);

    ASSERT_EQ(target_map.sent_msgs.size(), 1u);
    EXPECT_TRUE(default_map.sent_msgs.empty());
    EXPECT_EQ(target_map.sent_msgs.front().header.protocol,
              static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn));
    EXPECT_EQ(target_map.sent_msgs.front().header.object_id, 450035712u);
}

TEST(AgentHandlerTest, ProductionGameInRejectsWhenMapServerIsUnavailable) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply), true,
                                      false, {}, /*default_map_num=*/12);
    handler.set_allow_dev_gamein_fallback(false);
    const auto connection = mxh::net::make_connection_id(1104);
    handler.register_session(connection, 3001u, 450035716u, 12u);

    mxh::net::Message game_in;
    game_in.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::GameInSyn);
    game_in.header.object_id = 450035716u;
    handler.on_message(connection, game_in);

    ASSERT_EQ(reply.messages.size(), 1u);
    EXPECT_EQ(reply.messages.front().header.protocol,
              static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInNack));
    EXPECT_TRUE(reply.messages.front().payload.empty());
}

TEST(AgentHandlerTest, GameInSendFailureReturnsNackInsteadOfHanging) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply), true,
                                      false, {}, /*default_map_num=*/12);
    const auto connection = mxh::net::make_connection_id(1105);
    handler.register_session(connection, 3001u, 450035717u, 12u);
    MockTcpSender map;
    map.set_fail_send(true);
    handler.set_map_server(&map, mxh::net::make_connection_id(12));

    mxh::net::Message game_in;
    game_in.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::GameInSyn);
    game_in.header.object_id = 450035717u;
    handler.on_message(connection, game_in);

    ASSERT_EQ(reply.messages.size(), 1u);
    EXPECT_EQ(reply.messages.front().header.protocol,
              static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInNack));
}

TEST(AgentHandlerTest, ChangeMapUsesTargetRouteAndClosesCurrentRoute) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply), true,
                                      false, {}, /*default_map_num=*/12);
    const auto connection = mxh::net::make_connection_id(1101);
    handler.register_session(connection, 3001u, 450035713u, 7u);

    MockTcpSender current_map;
    MockTcpSender target_map;
    handler.set_map_server(&current_map, mxh::net::make_connection_id(7));
    handler.set_map_server_for_map(10u, &target_map,
                                   mxh::net::make_connection_id(10));

    mxh::net::Message change;
    change.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    change.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::ChangeMapSyn);
    change.header.object_id = 450035713u;
    change.payload.resize(4, 0);
    const std::uint16_t target_num = 10u;
    std::memcpy(change.payload.data(), &target_num, sizeof(target_num));
    handler.on_message(connection, change);

    ASSERT_EQ(current_map.sent_msgs.size(), 1u);
    EXPECT_EQ(current_map.sent_msgs.front().header.protocol,
              static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameOutSyn));
    ASSERT_EQ(target_map.sent_msgs.size(), 1u);
    EXPECT_EQ(target_map.sent_msgs.front().header.protocol,
              static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn));
    EXPECT_EQ(target_map.sent_msgs.front().header.object_id, 450035713u);
    ASSERT_EQ(reply.messages.size(), 1u);
    EXPECT_EQ(reply.messages.front().header.protocol,
              static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::ChangeMapAck));
}

TEST(CharacterAddTargetRoute, ChangeMapPublishesRouteBeforeSynchronousMapReply) {
    MockDbAdapter db;
    const auto moving_connection = mxh::net::make_connection_id(1101);
    const auto resident_connection = mxh::net::make_connection_id(1202);
    const auto map_connection = mxh::net::make_connection_id(10);
    std::vector<std::pair<std::uint64_t, mxh::net::Message>> delivered;
    mxh::server::AgentHandler agent(db,
        [&](mxh::net::ConnectionId id, const mxh::net::Message& msg) {
            delivered.emplace_back(id.value, msg);
        }, true, false, {}, 7u);
    agent.register_session(moving_connection, 1u, 101u, 7u);
    agent.register_session(resident_connection, 2u, 202u, 10u);
    MockTcpSender old_map, target_map;
    agent.set_map_server(&old_map, mxh::net::make_connection_id(7));
    agent.set_map_server_for_map(10u, &target_map, map_connection);
    const auto add = [](std::uint32_t recipient, std::uint32_t source) {
        mxh::net::Message msg;
        msg.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
        msg.header.protocol = static_cast<std::uint8_t>(
            mxh::proto::UserConnProtocol::CharacterAdd);
        msg.header.object_id = recipient;
        msg.payload.resize(288u);
        const std::uint16_t map = 10u;
        std::memcpy(msg.payload.data(), &source, 4);
        std::memcpy(msg.payload.data() + 4, &source, 4);
        std::memcpy(msg.payload.data() + 77, &map, 2);
        std::memcpy(msg.payload.data() + 79, &map, 2);
        return msg;
    };
    target_map.on_send = [&](const mxh::net::Message& sent) {
        if (sent.header.protocol != static_cast<std::uint8_t>(
                mxh::proto::UserConnProtocol::GameInSyn)) return;
        // A Map response can run before ITcpSender::send returns. Both
        // appearance directions must already see the destination map.
        agent.forward_from_map(map_connection, add(202u, 101u), 10u, &target_map);
        agent.forward_from_map(map_connection, add(101u, 202u), 10u, &target_map);
    };
    mxh::net::Message change;
    change.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    change.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::ChangeMapSyn);
    change.header.object_id = 101u;
    change.payload.resize(4u);
    const std::uint16_t target_num = 10u;
    std::memcpy(change.payload.data(), &target_num, 2);
    agent.on_message(moving_connection, change);

    ASSERT_EQ(delivered.size(), 3u);
    EXPECT_EQ(delivered[0].first, resident_connection.value);
    EXPECT_EQ(delivered[0].second.header.object_id, 101u);
    EXPECT_EQ(delivered[1].first, moving_connection.value);
    EXPECT_EQ(delivered[1].second.header.object_id, 202u);
    EXPECT_EQ(delivered[2].first, moving_connection.value);
    EXPECT_EQ(delivered[2].second.header.protocol,
              static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::ChangeMapAck));
}

TEST(CharacterAddTargetRoute, FailedChangeMapSendRestoresPriorRoute) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler agent(db, make_reply_spy(reply), true,
                                    false, {}, 7u);
    const auto client = mxh::net::make_connection_id(1101);
    agent.register_session(client, 1u, 101u, 7u);
    MockTcpSender old_map, target_map;
    target_map.set_fail_send(true);
    agent.set_map_server(&old_map, mxh::net::make_connection_id(7));
    agent.set_map_server_for_map(10u, &target_map,
                                  mxh::net::make_connection_id(10));
    mxh::net::Message change;
    change.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    change.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::ChangeMapSyn);
    change.header.object_id = 101u;
    change.payload.resize(2u);
    const std::uint16_t target_num = 10u;
    std::memcpy(change.payload.data(), &target_num, 2);
    agent.on_message(client, change);
    ASSERT_EQ(reply.messages.size(), 1u);
    EXPECT_EQ(reply.messages.front().header.protocol,
              static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::ChangeMapNack));
    mxh::net::Message out;
    out.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    out.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::GameOutSyn);
    agent.on_message(client, out);
    ASSERT_EQ(old_map.sent_msgs.size(), 2u);
    EXPECT_EQ(old_map.sent_msgs.back().header.object_id, 101u);
    EXPECT_EQ(target_map.send_count.load(), 1);
}

TEST(CharacterAddTargetRoute, DisconnectDuringTargetSendDoesNotRestoreSession) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler agent(db, make_reply_spy(reply), true,
                                    false, {}, 7u);
    const auto client = mxh::net::make_connection_id(1101);
    agent.register_session(client, 1u, 101u, 7u);
    MockTcpSender old_map, target_map;
    agent.set_map_server(&old_map, mxh::net::make_connection_id(7));
    agent.set_map_server_for_map(10u, &target_map,
                                  mxh::net::make_connection_id(10));
    target_map.on_send = [&](const mxh::net::Message& sent) {
        if (sent.header.protocol == static_cast<std::uint8_t>(
                mxh::proto::UserConnProtocol::GameInSyn)) {
            agent.on_disconnect(client, mxh::net::NetError::Disconnected);
        }
    };
    mxh::net::Message change;
    change.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    change.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::ChangeMapSyn);
    change.header.object_id = 101u;
    change.payload.resize(2u);
    const std::uint16_t target_num = 10u;
    std::memcpy(change.payload.data(), &target_num, 2);
    agent.on_message(client, change);
    EXPECT_TRUE(reply.messages.empty()) << "disconnected client must not receive ChangeMapAck";
    mxh::net::Message out;
    out.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    out.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::GameOutSyn);
    agent.on_message(client, out);
    // Without a session map, the default route is 7, never target 10.
    ASSERT_EQ(old_map.sent_msgs.size(), 2u);
    EXPECT_EQ(target_map.send_count.load(), 2);
    EXPECT_EQ(old_map.sent_msgs.back().header.object_id, 0u);
}

TEST(AgentHandlerTest, ChangeMapWithoutTargetRouteReturnsNackAndKeepsCurrentMap) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply), true,
                                      false, {}, /*default_map_num=*/12);
    const auto connection = mxh::net::make_connection_id(1102);
    handler.register_session(connection, 3001u, 450035714u, 7u);

    MockTcpSender current_map;
    handler.set_map_server(&current_map, mxh::net::make_connection_id(7));

    mxh::net::Message change;
    change.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    change.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::ChangeMapSyn);
    change.header.object_id = 450035714u;
    change.payload.resize(4, 0);
    const std::uint16_t unavailable_map = 99u;
    std::memcpy(change.payload.data(), &unavailable_map,
                sizeof(unavailable_map));
    handler.on_message(connection, change);

    EXPECT_TRUE(current_map.sent_msgs.empty());
    ASSERT_EQ(reply.messages.size(), 1u);
    EXPECT_EQ(reply.messages.front().header.protocol,
              static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::ChangeMapNack));
}

TEST(AgentHandlerTest, ChangeMapBootstrapsTargetMapHandlerAndRelaysGameInAck) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply), true,
                                      false, {}, /*default_map_num=*/7);
    const auto client_connection = mxh::net::make_connection_id(1103);
    const auto target_connection = mxh::net::make_connection_id(1010);
    constexpr std::uint32_t char_id = 450035715u;
    constexpr std::uint16_t target_map = 10u;
    handler.register_session(client_connection, 3001u, char_id, 7u);

    MockTcpSender current_map;
    handler.set_map_server(&current_map, mxh::net::make_connection_id(7));
    mxh::server::MapHandler target_handler(
        db, target_map,
        [&](mxh::net::ConnectionId id, const mxh::net::Message& msg) {
            handler.forward_from_map(id, msg);
        });
    MapHandlerForwardingSender target_map_sender(target_handler,
                                                 target_connection);
    handler.set_map_server_for_map(target_map, &target_map_sender,
                                   target_connection);

    mxh::net::Message change;
    change.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    change.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::ChangeMapSyn);
    change.header.object_id = char_id;
    change.payload.resize(4, 0);
    std::memcpy(change.payload.data(), &target_map, sizeof(target_map));
    handler.on_message(client_connection, change);

    const auto snapshot = target_handler.player_runtime_snapshot(char_id);
    ASSERT_TRUE(snapshot.has_value());
    EXPECT_EQ(snapshot->lifecycle, mxh::server::PlayerLifecycle::Active);
    EXPECT_EQ(snapshot->map_num, target_map);
    EXPECT_EQ(target_map_sender.send_count.load(), 1);
    ASSERT_FALSE(current_map.sent_msgs.empty());
    EXPECT_EQ(current_map.sent_msgs.front().header.protocol,
              static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameOutSyn));

    bool saw_game_in_ack = false;
    bool saw_change_map_ack = false;
    for (const auto& message : reply.messages) {
        saw_game_in_ack |= message.header.protocol == static_cast<std::uint8_t>(
            mxh::proto::UserConnProtocol::GameInAck);
        saw_change_map_ack |= message.header.protocol == static_cast<std::uint8_t>(
            mxh::proto::UserConnProtocol::ChangeMapAck);
    }
    EXPECT_TRUE(saw_game_in_ack);
    EXPECT_TRUE(saw_change_map_ack);
}

TEST(AgentHandlerTest, DisconnectSynAcknowledgesAndClearsMapRoute) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply), true);
    const auto connection = mxh::net::make_connection_id(1001);
    handler.register_session(connection, 3001, 450035712, 12, 5);
    MockTcpSender map;
    handler.set_map_server(&map, mxh::net::make_connection_id(42));

    mxh::net::Message disconnect;
    disconnect.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    disconnect.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::DisconnectSyn);
    handler.on_message(connection, disconnect);

    ASSERT_EQ(reply.call_count.load(), 1);
    EXPECT_EQ(reply.last_message.header.protocol, static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::DisconnectAck));
    EXPECT_TRUE(reply.last_message.payload.empty());
    ASSERT_EQ(map.sent_msgs.size(), 1u);
    EXPECT_EQ(map.sent_msgs[0].header.protocol, static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::GameOutSyn));
    EXPECT_EQ(map.sent_msgs[0].header.object_id, 450035712u);
    EXPECT_EQ(handler.user_level(connection), 0u);

    handler.on_disconnect(connection, mxh::net::NetError::Disconnected);
    EXPECT_EQ(map.sent_msgs.size(), 1u);
}

TEST(AgentHandlerTest, DisconnectSynWithoutCharacterDoesNotTouchMap) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply), true);
    const auto connection = mxh::net::make_connection_id(1002);
    handler.register_session(connection, 3002, 0, 0);
    MockTcpSender map;
    handler.set_map_server(&map, mxh::net::make_connection_id(42));

    mxh::net::Message disconnect;
    disconnect.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    disconnect.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::DisconnectSyn);
    handler.on_message(connection, disconnect);

    ASSERT_EQ(reply.call_count.load(), 1);
    EXPECT_EQ(reply.last_message.header.protocol, static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::DisconnectAck));
    EXPECT_TRUE(map.sent_msgs.empty());
}

// ===========================================================================

// ===========================================================================
// R-2: HackShield routing tests
//
// Cover the data-plane: cat==HackShield messages route through the
// HackShieldManager state machine. Tests verify:
//   - non-superuser (UserLevel < 5) -> no reply, no state change
//   - superuser + GuidAck -> sends Req (160B)
//   - superuser + Ack -> no reply, state cleared
//   - superuser + Disconnect -> disconnect pending flag set
//   - on_disconnect cleans up hackshield_disconnect_pending_ entries
//   - register_session stores user_level + initializes HackShieldUserState
//   - inspection helpers return expected values
// ===========================================================================

TEST(AgentHandlerHackShieldTest, RegisterSessionStoresUserLevel) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    auto cid = mxh::net::make_connection_id(101);
    handler.register_session(cid, /*user_id=*/777, /*char_id=*/9001,
                              /*map_num=*/12, /*user_level=*/5);
    EXPECT_EQ(handler.user_level(cid), 5u);
}

TEST(AgentHandlerHackShieldTest, DefaultUserLevelIsZero) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    auto cid = mxh::net::make_connection_id(102);
    handler.register_session(cid, 1, 2, 3);  // no user_level -> default 0
    EXPECT_EQ(handler.user_level(cid), 0u);
    EXPECT_FALSE(handler.has_hackshield_state(cid));
    EXPECT_FALSE(handler.is_hackshield_disconnect_pending(cid));
}

TEST(AgentHandlerHackShieldTest, GuidReqNonSuperuserIsNoOp) {
    // UserLevel < HACKSHIELD_SUPERUSER_LEVEL (5) -> send_guid_req
    // returns hackshield_none(), so no reply should fire.
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    auto cid = mxh::net::make_connection_id(200);
    handler.register_session(cid, 1, 2, 3, /*user_level=*/2);

    mxh::net::Message msg;
    msg.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::HackShield);
    msg.header.protocol = static_cast<std::uint8_t>(
        mxh::server::HackShieldProtocol::GuidReq);
    handler.on_message(cid, msg);

    EXPECT_EQ(reply.call_count.load(), 0);
    EXPECT_FALSE(handler.has_hackshield_state(cid));
}

TEST(AgentHandlerHackShieldTest, GuidAckSuperuserSendsReq) {
    // superuser sends GuidAck (proto=1) -> state machine clears
    // m_bHSCheck to 0, then re-issues Req (proto=2, 160B).
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    auto cid = mxh::net::make_connection_id(201);
    handler.register_session(cid, 1, 2, 3, /*user_level=*/5);

    mxh::net::Message msg;
    msg.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::HackShield);
    msg.header.protocol = static_cast<std::uint8_t>(
        mxh::server::HackShieldProtocol::GuidAck);
    msg.payload.assign(mxh::server::HACKSHIELD_GUID_ACK_SIZE, 0xAB);
    handler.on_message(cid, msg);

    ASSERT_EQ(reply.call_count.load(), 1);
    EXPECT_EQ(reply.last_message.header.category,
              static_cast<std::uint8_t>(mxh::proto::Category::HackShield));
    EXPECT_EQ(reply.last_message.header.protocol,
              static_cast<std::uint8_t>(mxh::server::HackShieldProtocol::Req));
    EXPECT_EQ(reply.last_message.payload.size(),
              mxh::server::HACKSHIELD_REQ_SIZE);
    EXPECT_TRUE(handler.has_hackshield_state(cid));
    EXPECT_FALSE(handler.is_hackshield_disconnect_pending(cid));
}

TEST(AgentHandlerHackShieldTest, AckSuperuserClearsState) {
    // superuser sends Ack (proto=3) after a successful analysis ->
    // state machine clears m_bHSCheck to 0, no reply.
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    auto cid = mxh::net::make_connection_id(202);
    handler.register_session(cid, 1, 2, 3, /*user_level=*/5);

    mxh::net::Message msg;
    msg.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::HackShield);
    msg.header.protocol = static_cast<std::uint8_t>(
        mxh::server::HackShieldProtocol::Ack);
    msg.payload.assign(mxh::server::HACKSHIELD_ACK_SIZE, 0xCD);
    handler.on_message(cid, msg);

    EXPECT_EQ(reply.call_count.load(), 0);
    EXPECT_TRUE(handler.has_hackshield_state(cid));
    EXPECT_FALSE(handler.is_hackshield_disconnect_pending(cid));
}

TEST(AgentHandlerHackShieldTest, ClientDisconnectProtocolSetsPendingFlag) {
    // proto=4 (Disconnect) from client -> queue drop session.
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    auto cid = mxh::net::make_connection_id(203);
    handler.register_session(cid, 1, 2, 3, /*user_level=*/5);

    mxh::net::Message msg;
    msg.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::HackShield);
    msg.header.protocol = static_cast<std::uint8_t>(
        mxh::server::HackShieldProtocol::Disconnect);
    handler.on_message(cid, msg);

    EXPECT_EQ(reply.call_count.load(), 0);
    EXPECT_TRUE(handler.is_hackshield_disconnect_pending(cid));
}

TEST(AgentHandlerHackShieldTest, ReqSecondCallQueuesDisconnect) {
    // First Req from server: state machine sets m_bHSCheck=1 and
    // returns Send. Second Req with m_bHSCheck==1 returns Disconnect.
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    auto cid = mxh::net::make_connection_id(204);
    handler.register_session(cid, 1, 2, 3, /*user_level=*/5);

    // 1st Req: state was 0, sets to 1, returns Send.
    mxh::net::Message req1;
    req1.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::HackShield);
    req1.header.protocol = static_cast<std::uint8_t>(
        mxh::server::HackShieldProtocol::Req);
    handler.on_message(cid, req1);
    ASSERT_EQ(reply.call_count.load(), 1);
    EXPECT_EQ(reply.last_message.header.protocol,
              static_cast<std::uint8_t>(mxh::server::HackShieldProtocol::Req));
    EXPECT_EQ(reply.last_message.payload.size(),
              mxh::server::HACKSHIELD_REQ_SIZE);
    EXPECT_FALSE(handler.is_hackshield_disconnect_pending(cid));

    // 2nd Req: state was 1, returns Disconnect (no reply).
    mxh::net::Message req2;
    req2.header.category = req1.header.category;
    req2.header.protocol = req1.header.protocol;
    handler.on_message(cid, req2);
    EXPECT_EQ(reply.call_count.load(), 1);  // no second reply
    EXPECT_TRUE(handler.is_hackshield_disconnect_pending(cid));
}

TEST(AgentHandlerHackShieldTest, OnDisconnectClearsPendingFlag) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    auto cid = mxh::net::make_connection_id(205);
    handler.register_session(cid, 1, 2, 3, /*user_level=*/5);

    // Trigger disconnect pending via client Disconnect protocol.
    mxh::net::Message msg;
    msg.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::HackShield);
    msg.header.protocol = static_cast<std::uint8_t>(
        mxh::server::HackShieldProtocol::Disconnect);
    handler.on_message(cid, msg);
    ASSERT_TRUE(handler.is_hackshield_disconnect_pending(cid));

    // on_disconnect clears the pending flag and the HackShieldUserState.
    handler.on_disconnect(cid, mxh::net::NetError::Disconnected);
    EXPECT_FALSE(handler.is_hackshield_disconnect_pending(cid));
    EXPECT_FALSE(handler.has_hackshield_state(cid));
    EXPECT_EQ(handler.user_level(cid), 0u);  // user_level erased too
}

TEST(AgentHandlerHackShieldTest, NonHackShieldCatIsNotRouted) {
    // cat==Chat (6) -- make sure handle_hackshield is NOT called
    // for non-HackShield categories; existing on_message should
    // log "unhandled category" and not touch the HackShield map.
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    auto cid = mxh::net::make_connection_id(206);
    handler.register_session(cid, 1, 2, 3, /*user_level=*/5);

    mxh::net::Message msg;
    msg.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::Chat);
    msg.header.protocol = 0;
    handler.on_message(cid, msg);

    EXPECT_FALSE(handler.has_hackshield_state(cid));
    EXPECT_FALSE(handler.is_hackshield_disconnect_pending(cid));
}

TEST(AgentHandlerHackShieldTest, ConnIdMismatchDoesNotLeakState) {
    // Verify the per-connection map is keyed by id.value, not by
    // some other identifier; two different conn ids with the same
    // user_level should not interfere.
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));

    auto cid_a = mxh::net::make_connection_id(300);
    auto cid_b = mxh::net::make_connection_id(301);

    handler.register_session(cid_a, 1, 2, 3, /*user_level=*/5);
    handler.register_session(cid_b, 4, 5, 6, /*user_level=*/2);

    // Disconnect cid_a only.
    handler.on_disconnect(cid_a, mxh::net::NetError::Disconnected);
    EXPECT_EQ(handler.user_level(cid_a), 0u);
    EXPECT_EQ(handler.user_level(cid_b), 2u);
    EXPECT_FALSE(handler.has_hackshield_state(cid_a));
    EXPECT_FALSE(handler.has_hackshield_state(cid_b));
}



// ===========================================================================
// R-2.1: handle_legacy_character_list auto-populates user_level from DB
//
// Before R-2.1 the AgentHandler required a manual register_session call
// to set user_level before the HackShield gate would let any cat=67
// message through. After R-2.1, handle_legacy_character_list queries
// chr_log_info.userlevel automatically and stores it. This makes
// end-to-end HackShield routing work without any test scaffolding:
// 1) Client sends CharacterListSyn with user_id
// 2) Agent queries chr_log_info.userlevel
// 3) Agent stores the level in conn_user_levels_
// 4) Subsequent HackShield messages see the right threshold
// ===========================================================================

TEST(AgentHandlerHackShieldTest, CharacterListPopulatesUserLevelFromDb) {
    MockDbAdapter db;
    db.userlevel_to_return.store(5u);  // superuser
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply), true);
    auto cid = mxh::net::make_connection_id(700);

    // Drive CharacterListSyn with user_id=42 in the payload.
    mxh::net::Message msg;
    msg.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    msg.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::CharacterListSyn);
    msg.payload.resize(8, 0);
    std::uint32_t user_id = 42u;
    std::memcpy(msg.payload.data(), &user_id, 4);
    handler.on_message(cid, msg);

    EXPECT_EQ(handler.user_level(cid), 5u);

    // Now send a HackShield GuidAck -- the superuser gate should
    // fire and reply with a Req packet (160B).
    mxh::net::Message hs;
    hs.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::HackShield);
    hs.header.protocol = static_cast<std::uint8_t>(
        mxh::server::HackShieldProtocol::GuidAck);
    hs.payload.assign(mxh::server::HACKSHIELD_GUID_ACK_SIZE, 0xEF);
    handler.on_message(cid, hs);

    ASSERT_GE(reply.call_count.load(), 1);
    bool found_req = false;
    for (const auto& m : reply.messages) {
        if (m.header.category ==
                static_cast<std::uint8_t>(mxh::proto::Category::HackShield)
            && m.header.protocol ==
                static_cast<std::uint8_t>(mxh::server::HackShieldProtocol::Req)) {
            EXPECT_EQ(m.payload.size(), mxh::server::HACKSHIELD_REQ_SIZE);
            found_req = true;
            break;
        }
    }
    EXPECT_TRUE(found_req) << "superuser GuidAck should trigger Req reply";
}

TEST(AgentHandlerHackShieldTest, CharacterListNonSuperuserGatesHackShield) {
    MockDbAdapter db;
    db.userlevel_to_return.store(2u);  // NOT a superuser
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply), true);
    auto cid = mxh::net::make_connection_id(701);

    mxh::net::Message msg;
    msg.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    msg.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::CharacterListSyn);
    msg.payload.resize(8, 0);
    std::uint32_t user_id = 99u;
    std::memcpy(msg.payload.data(), &user_id, 4);
    handler.on_message(cid, msg);

    EXPECT_EQ(handler.user_level(cid), 2u);

    // GuidAck from non-superuser -- no reply, no state.
    mxh::net::Message hs;
    hs.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::HackShield);
    hs.header.protocol = static_cast<std::uint8_t>(
        mxh::server::HackShieldProtocol::GuidAck);
    handler.on_message(cid, hs);

    EXPECT_FALSE(handler.has_hackshield_state(cid));
}

TEST(AgentHandlerHackShieldTest, CharacterListMissingUserLevelDefaultsZero) {
    // userlevel_to_return left at default 0xFFu -- chr_log_info query
    // returns empty ResultSet (MockDbAdapter default) -- so
    // conn_user_levels_ is 0 (same as no register_session call).
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply), true);
    auto cid = mxh::net::make_connection_id(702);

    mxh::net::Message msg;
    msg.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    msg.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::CharacterListSyn);
    msg.payload.resize(8, 0);
    std::uint32_t user_id = 7u;
    std::memcpy(msg.payload.data(), &user_id, 4);
    handler.on_message(cid, msg);

    EXPECT_EQ(handler.user_level(cid), 0u);
    EXPECT_FALSE(handler.has_hackshield_state(cid));
}


// MapHandler
// ===========================================================================
// R-2.2: AgentHandler::tick_hackshield() server-side periodic recheck
//
// Walks every connection with active HackShield state and runs
// send_hackshield_req() through the same state machine the
// client-driven Req handler does. Legacy CHackShieldManager
// called this every ~30s. State machine decisions:
//   m_bHSCheck == 0 (idle)       -> send Req, -> 1 (waiting)
//   m_bHSCheck == 1 (waiting)    -> Disconnect (timeout)
//   m_bHSCheck == 2 (just-grace) -> 1 (waiting) without send
// ===========================================================================

TEST(AgentHandlerHackShieldTest, TickHackshieldOnEmptyHandlerIsZero) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    EXPECT_EQ(handler.tick_hackshield(), 0u);
    EXPECT_EQ(reply.call_count.load(), 0);
}

TEST(AgentHandlerHackShieldTest, TickHackshieldGraceSuperuserNoOpThenDisconnect) {
    // Send a GuidReq to push state machine into m_bHSCheck=2 (grace).
    // First tick: grace -> waiting (no send). Second tick: waiting ->
    // Disconnect (timeout exceeded).
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    auto cid = mxh::net::make_connection_id(800);
    handler.register_session(cid, 1, 2, 3, /*user_level=*/5);
    mxh::net::Message req;
    req.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::HackShield);
    req.header.protocol = static_cast<std::uint8_t>(
        mxh::server::HackShieldProtocol::GuidReq);
    handler.on_message(cid, req);
    EXPECT_TRUE(handler.has_hackshield_state(cid));
    reply.call_count.store(0);
    EXPECT_EQ(handler.tick_hackshield(), 0u);
    EXPECT_EQ(reply.call_count.load(), 0);
    EXPECT_GE(handler.tick_hackshield(), 1u);
    EXPECT_TRUE(handler.is_hackshield_disconnect_pending(cid));
}

TEST(AgentHandlerHackShieldTest, TickHackshieldNonSuperuserIsNoOp) {
    // Non-superuser: state should never have been created by the
    // client-driven handler (UserLevel < 5 guard), so tick
    // finds no entries to walk.
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    auto cid = mxh::net::make_connection_id(801);
    handler.register_session(cid, 1, 2, 3, /*user_level=*/2);
    mxh::net::Message req;
    req.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::HackShield);
    req.header.protocol = static_cast<std::uint8_t>(
        mxh::server::HackShieldProtocol::GuidReq);
    handler.on_message(cid, req);
    EXPECT_FALSE(handler.has_hackshield_state(cid));
    EXPECT_EQ(handler.tick_hackshield(), 0u);
    EXPECT_EQ(reply.call_count.load(), 0);
}

TEST(AgentHandlerHackShieldTest, TickHackshieldMixedSuperusersHandlesEach) {
    // Two superusers + one non-superuser. Two ticks produce 2
    // disconnect actions on the superusers only.
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    auto cid_super_a = mxh::net::make_connection_id(810);
    auto cid_super_b = mxh::net::make_connection_id(811);
    auto cid_normal = mxh::net::make_connection_id(812);
    handler.register_session(cid_super_a, 1, 2, 3, /*user_level=*/5);
    handler.register_session(cid_super_b, 4, 5, 6, /*user_level=*/5);
    handler.register_session(cid_normal, 7, 8, 9, /*user_level=*/3);
    auto send_guid = [&](mxh::net::ConnectionId id) {
        mxh::net::Message req;
        req.header.category = static_cast<std::uint8_t>(
            mxh::proto::Category::HackShield);
        req.header.protocol = static_cast<std::uint8_t>(
            mxh::server::HackShieldProtocol::GuidReq);
        handler.on_message(id, req);
    };
    send_guid(cid_super_a);
    send_guid(cid_super_b);
    send_guid(cid_normal);
    EXPECT_EQ(handler.tick_hackshield(), 0u);
    EXPECT_EQ(handler.tick_hackshield(), 2u);
    EXPECT_TRUE(handler.is_hackshield_disconnect_pending(cid_super_a));
    EXPECT_TRUE(handler.is_hackshield_disconnect_pending(cid_super_b));
    EXPECT_FALSE(handler.is_hackshield_disconnect_pending(cid_normal));
}

TEST(AgentHandlerCharacterRemoveTest, DeletesOnlyOwnedCharacterAndDependentState) {
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg{};
    cfg.backend = "sqlite";
    cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());
    ASSERT_TRUE(db.exec_multi(
        "CREATE TABLE character_info (chrid INTEGER PRIMARY KEY,userid TEXT,charname TEXT);"
        "CREATE TABLE modern_player_state (player_id INTEGER);"
        "CREATE TABLE modern_player_item (player_id INTEGER);"
        "CREATE TABLE modern_player_quest_log (player_id INTEGER);"
        "CREATE TABLE modern_player_quest_sub (player_id INTEGER);"
        "CREATE TABLE modern_player_vitals (player_id INTEGER);"
        "CREATE TABLE modern_item_grant (character_id INTEGER);"
        "INSERT INTO character_info VALUES(1001,'42','OwnedHero');"
        "INSERT INTO character_info VALUES(1002,'99','OtherHero');"
        "INSERT INTO modern_player_state VALUES(1001);"
        "INSERT INTO modern_player_item VALUES(1001);"
        "INSERT INTO modern_player_quest_log VALUES(1001);"
        "INSERT INTO modern_player_quest_sub VALUES(1001);"
        "INSERT INTO modern_player_vitals VALUES(1001);"
        "INSERT INTO modern_item_grant VALUES(1001);").ok());

    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply), true);
    const auto connection = mxh::net::make_connection_id(501);
    handler.register_session(connection, 42, 0, 0);

    mxh::net::Message remove;
    remove.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    remove.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::CharacterRemoveSyn);
    remove.payload.resize(sizeof(std::uint32_t));
    const std::uint32_t character_id = 1001;
    std::memcpy(remove.payload.data(), &character_id, sizeof(character_id));
    handler.on_message(connection, remove);

    ASSERT_EQ(reply.call_count.load(), 1);
    EXPECT_EQ(reply.last_message.header.protocol, static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::CharacterRemoveAck));
    const std::vector<mxh::db::Bind> no_args;
    mxh::db::ResultSet rows;
    ASSERT_TRUE(db.query("SELECT chrid FROM character_info ORDER BY chrid", no_args, rows).ok());
    ASSERT_EQ(rows.rows.size(), 1u);
    EXPECT_EQ(std::get<std::int64_t>(rows.rows[0][0]), 1002);
    for (const auto table : {"modern_player_state", "modern_player_item",
                             "modern_player_quest_log", "modern_player_quest_sub",
                             "modern_player_vitals",
                             "modern_item_grant"}) {
        rows = {};
        ASSERT_TRUE(db.query("SELECT * FROM " + std::string(table), no_args, rows).ok());
        EXPECT_TRUE(rows.empty()) << table;
    }
}

TEST(AgentHandlerCharacterRemoveTest, RejectsCharacterOwnedByAnotherUser) {
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg{};
    cfg.backend = "sqlite";
    cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());
    const std::vector<mxh::db::Bind> no_args;
    ASSERT_TRUE(db.execute(
        "CREATE TABLE character_info (chrid INTEGER PRIMARY KEY,userid TEXT,charname TEXT)", no_args).ok());
    ASSERT_TRUE(db.execute(
        "INSERT INTO character_info VALUES(1002,'99','OtherHero')", no_args).ok());

    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply), true);
    const auto connection = mxh::net::make_connection_id(502);
    handler.register_session(connection, 42, 0, 0);
    mxh::net::Message remove;
    remove.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    remove.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::CharacterRemoveSyn);
    remove.payload.resize(sizeof(std::uint32_t));
    const std::uint32_t character_id = 1002;
    std::memcpy(remove.payload.data(), &character_id, sizeof(character_id));
    handler.on_message(connection, remove);

    ASSERT_EQ(reply.call_count.load(), 1);
    EXPECT_EQ(reply.last_message.header.protocol, static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::CharacterRemoveNack));
    mxh::db::ResultSet rows;
    ASSERT_TRUE(db.query("SELECT chrid FROM character_info", no_args, rows).ok());
    EXPECT_EQ(rows.rows.size(), 1u);
}


// ===========================================================================

TEST(MapHandlerTest, ConstructionWithMapNumDoesNotCrash) {
    // MapHandler takes (db, map_num, reply, use_legacy_framing=true).
    // The header notes "legacy framing is mandatory for MapServer" so
    // the default is true; we pass false anyway to exercise the
    // non-default path.
    MockDbAdapter db;
    mxh::server::MapHandler h1(db, /*map_num=*/7, mxh::server::ReplyFn{});
    mxh::server::MapHandler h2(db, /*map_num=*/7, mxh::server::ReplyFn{}, true);
    SUCCEED();
}

TEST(MapHandlerTest, PartyCreateAndBreakupMutateAuthoritativeMapState) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 10, make_reply_spy(reply));
    const auto connection = mxh::net::make_connection_id(77);

    mxh::net::Message game_in;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    game_in.header.object_id = 123u;
    handler.on_message(connection, game_in);

    mxh::net::Message create;
    create.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Party);
    create.header.protocol = static_cast<std::uint8_t>(mxh::proto::PartyProtocol::CreateSyn);
    create.header.object_id = 123u;
    create.payload = {0};
    handler.on_message(connection, create);
    ASSERT_GE(reply.messages.size(), 2u); // GameInAck + PartyCreateAck
    const auto& created = reply.messages.back();
    EXPECT_EQ(created.header.protocol,
              static_cast<std::uint8_t>(mxh::proto::PartyProtocol::CreateAck));
    ASSERT_EQ(created.payload.size(), 28u);
    std::uint32_t party_id = 0;
    std::memcpy(&party_id, created.payload.data(), sizeof(party_id));
    EXPECT_EQ(party_id, 1u);
    EXPECT_EQ(created.payload[4], 1u);

    mxh::net::Message second_game_in = game_in;
    second_game_in.header.object_id = 456u;
    const auto second_connection = mxh::net::make_connection_id(78);
    handler.on_message(second_connection, second_game_in);

    mxh::net::Message invite = create;
    invite.header.protocol = static_cast<std::uint8_t>(mxh::proto::PartyProtocol::AddSyn);
    invite.payload.resize(8);
    std::memcpy(invite.payload.data(), &party_id, sizeof(party_id));
    const std::uint32_t target_id = 456u;
    std::memcpy(invite.payload.data() + 4, &target_id, sizeof(target_id));
    handler.on_message(connection, invite);
    bool saw_invite = false;
    for (const auto& message : reply.messages) {
        saw_invite |= message.header.protocol ==
                      static_cast<std::uint8_t>(mxh::proto::PartyProtocol::AddInvite) &&
                      message.header.object_id == target_id;
    }
    EXPECT_TRUE(saw_invite);

    mxh::net::Message accept;
    accept.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Party);
    accept.header.protocol = static_cast<std::uint8_t>(mxh::proto::PartyProtocol::InviteAcceptSyn);
    accept.header.object_id = target_id;
    accept.payload.resize(sizeof(party_id));
    std::memcpy(accept.payload.data(), &party_id, sizeof(party_id));
    handler.on_message(second_connection, accept);
    bool saw_join_ack = false;
    for (const auto& message : reply.messages) {
        if (message.header.protocol == static_cast<std::uint8_t>(
                mxh::proto::PartyProtocol::InviteAcceptAck) &&
            message.header.object_id == target_id) {
            saw_join_ack = true;
            ASSERT_GE(message.payload.size(), 5u);
            EXPECT_EQ(message.payload[4], 2u);
        }
    }
    EXPECT_TRUE(saw_join_ack);

    handler.on_message(connection, create);
    EXPECT_EQ(reply.messages.back().header.protocol,
              static_cast<std::uint8_t>(mxh::proto::PartyProtocol::CreateNack));

    mxh::net::Message breakup;
    breakup.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Party);
    breakup.header.protocol = static_cast<std::uint8_t>(mxh::proto::PartyProtocol::BreakupSyn);
    breakup.header.object_id = 123u;
    breakup.payload.resize(sizeof(party_id));
    std::memcpy(breakup.payload.data(), &party_id, sizeof(party_id));
    handler.on_message(connection, breakup);
    EXPECT_EQ(reply.messages.back().header.protocol,
              static_cast<std::uint8_t>(mxh::proto::PartyProtocol::BreakupAck));

    mxh::net::Message guild_create;
    guild_create.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Guild);
    guild_create.header.protocol = static_cast<std::uint8_t>(mxh::proto::GuildProtocol::CreateSyn);
    guild_create.header.object_id = 123u;
    guild_create.payload = {'K', 'n', 'i', 'g', 'h', 't', 's'};
    handler.on_message(connection, guild_create);
    ASSERT_EQ(reply.messages.back().header.protocol,
              static_cast<std::uint8_t>(mxh::proto::GuildProtocol::CreateAck));
    ASSERT_GE(reply.messages.back().payload.size(), 5u);
    std::uint32_t guild_id = 0;
    std::memcpy(&guild_id, reply.messages.back().payload.data(), sizeof(guild_id));
    EXPECT_EQ(reply.messages.back().payload[4], 1u);

    mxh::net::Message guild_invite = guild_create;
    guild_invite.header.protocol = static_cast<std::uint8_t>(mxh::proto::GuildProtocol::AddMemberSyn);
    guild_invite.payload.resize(8);
    std::memcpy(guild_invite.payload.data(), &guild_id, sizeof(guild_id));
    std::memcpy(guild_invite.payload.data() + 4, &target_id, sizeof(target_id));
    handler.on_message(connection, guild_invite);
    bool saw_guild_invite = false;
    for (const auto& message : reply.messages) {
        saw_guild_invite |= message.header.protocol == static_cast<std::uint8_t>(
                                mxh::proto::GuildProtocol::AddMemberInvite) &&
                            message.header.object_id == target_id;
    }
    EXPECT_TRUE(saw_guild_invite);

    mxh::net::Message guild_accept;
    guild_accept.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Guild);
    guild_accept.header.protocol = static_cast<std::uint8_t>(mxh::proto::GuildProtocol::InviteAccept);
    guild_accept.header.object_id = target_id;
    guild_accept.payload.resize(sizeof(guild_id));
    std::memcpy(guild_accept.payload.data(), &guild_id, sizeof(guild_id));
    handler.on_message(second_connection, guild_accept);
    bool saw_guild_join = false;
    for (const auto& message : reply.messages) {
        if (message.header.protocol == static_cast<std::uint8_t>(
                mxh::proto::GuildProtocol::InviteAccept) &&
            message.header.object_id == target_id) {
            saw_guild_join = true;
            ASSERT_GE(message.payload.size(), 5u);
            EXPECT_EQ(message.payload[4], 2u);
        }
    }
    EXPECT_TRUE(saw_guild_join);

    handler.on_message(connection, guild_create);
    EXPECT_EQ(reply.messages.back().header.protocol,
              static_cast<std::uint8_t>(mxh::proto::GuildProtocol::CreateNack));

    mxh::net::Message guild_breakup = guild_create;
    guild_breakup.header.protocol = static_cast<std::uint8_t>(mxh::proto::GuildProtocol::BreakupSyn);
    guild_breakup.payload.resize(sizeof(guild_id));
    std::memcpy(guild_breakup.payload.data(), &guild_id, sizeof(guild_id));
    handler.on_message(connection, guild_breakup);
    EXPECT_EQ(reply.messages.back().header.protocol,
              static_cast<std::uint8_t>(mxh::proto::GuildProtocol::BreakupAck));
}

TEST(MapHandlerTest, AllChatPersistsSanitizedAuditRecord) {
    auto db = mxh::db::make_adapter("sqlite");
    mxh::db::ConnectionConfig cfg;
    cfg.path = ":memory:";
    ASSERT_TRUE(db->connect(cfg).ok());
    ASSERT_TRUE(db->execute("CREATE TABLE log_chat (logid INTEGER PRIMARY KEY AUTOINCREMENT, chrname TEXT, channel TEXT, message TEXT, logtime TEXT)").ok());
    ReplySpy reply;
    mxh::server::MapHandler handler(*db, 7, make_reply_spy(reply));
    mxh::net::Message msg;
    msg.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Chat);
    msg.header.protocol = static_cast<std::uint8_t>(mxh::proto::ChatProtocol::All);
    msg.header.object_id = 42;
    msg.payload = {'h', 'i', 1, '!', 0, 'x'};
    handler.on_message(mxh::net::make_connection_id(9), msg);
    mxh::db::ResultSet rows;
    ASSERT_TRUE(db->query("SELECT chrname, channel, message FROM log_chat", rows).ok());
    ASSERT_EQ(rows.rows.size(), 1u);
    EXPECT_EQ(std::get<std::string>(rows.rows[0][0]), "#42");
    EXPECT_EQ(std::get<std::string>(rows.rows[0][1]), "all");
    EXPECT_EQ(std::get<std::string>(rows.rows[0][2]), "hi!");
}

TEST(MapHandlerTest, OnConnectNonLegacyReturnsTrue) {
    // MapHandler.on_connect accepts all clients (no version
    // negotiation, no auth key ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â those happen on Distribute/Agent).
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, /*map_num=*/7, make_reply_spy(reply));
    EXPECT_TRUE(handler.on_connect({}, "10.0.0.1:1234"));
}

TEST(MapHandlerTest, RuntimeSnapshotIsCreatedOnGameIn) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(mxh::net::make_connection_id(55), game_in);
    ASSERT_EQ(handler.player_runtime_count(), 1u);
    const auto snapshot = handler.player_runtime_snapshot(123u);
    ASSERT_TRUE(snapshot.has_value());
    EXPECT_EQ(snapshot->player_id, 123u);
    EXPECT_EQ(snapshot->map_num, 7u);
    EXPECT_FLOAT_EQ(snapshot->pos_x, 25000.0f);
    EXPECT_FLOAT_EQ(snapshot->pos_z, 25000.0f);
    EXPECT_EQ(snapshot->lifecycle, mxh::server::PlayerLifecycle::Active);
    EXPECT_EQ(snapshot->current_hp, 125u);
    EXPECT_EQ(snapshot->max_hp, 125u);
    EXPECT_TRUE(snapshot->physical_stats_ready);
    EXPECT_EQ(snapshot->physical_attack_min, 16u);
    EXPECT_EQ(snapshot->physical_attack_max, 16u);
    EXPECT_EQ(snapshot->physical_defence, 8u);
}

TEST(MapHandlerTest, UnknownBaseAttributesRejectGameInBeforeCreatingActor) {
    MockDbAdapter db;
    db.return_unknown_attributes = true;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(mxh::net::make_connection_id(55), game_in);
    EXPECT_EQ(handler.player_runtime_count(), 0u);
    ASSERT_EQ(reply.messages.size(), 1u);
    EXPECT_EQ(reply.messages.front().header.protocol,
              static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInNack));
}

TEST(MapHandlerTest, TargetMapGameInUsesTargetMapAndEmitsAck) {
    MockDbAdapter db;
    ReplySpy reply;
    constexpr std::uint32_t char_id = 450035713u;
    constexpr std::uint16_t target_map = 10u;
    const auto map_connection = mxh::net::make_connection_id(2010);
    mxh::server::MapHandler target(db, target_map, make_reply_spy(reply));

    mxh::net::Message game_in;
    game_in.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::GameInSyn);
    game_in.header.object_id = char_id;
    game_in.payload.resize(16, 0);
    std::uint32_t user_id = 3001u;
    std::memcpy(game_in.payload.data(), &user_id, sizeof(user_id));

    target.on_message(map_connection, game_in);

    const auto snapshot = target.player_runtime_snapshot(char_id);
    ASSERT_TRUE(snapshot.has_value());
    EXPECT_EQ(snapshot->lifecycle, mxh::server::PlayerLifecycle::Active);
    EXPECT_EQ(snapshot->map_num, target_map);
    EXPECT_EQ(snapshot->player_id, char_id);

    const auto ack = std::find_if(reply.messages.begin(), reply.messages.end(),
        [](const mxh::net::Message& message) {
            return message.header.protocol == static_cast<std::uint8_t>(
                mxh::proto::UserConnProtocol::GameInAck);
        });
    ASSERT_NE(ack, reply.messages.end());
    EXPECT_EQ(ack->header.object_id, char_id);
}

TEST(MapHandlerTest, CharacterAddCarriesVisiblePlayerVitals) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));

    const auto enter = [&](std::uint32_t playerId, std::uint64_t connectionId) {
        mxh::net::Message gameIn;
        gameIn.header.object_id = playerId;
        gameIn.header.category = static_cast<std::uint8_t>(
            mxh::proto::Category::UserConn);
        gameIn.header.protocol = static_cast<std::uint8_t>(
            mxh::proto::UserConnProtocol::GameInSyn);
        handler.on_message(mxh::net::make_connection_id(connectionId), gameIn);
    };
    enter(101u, 51u);
    enter(202u, 52u);

    const auto message = std::find_if(
        reply.messages.begin(), reply.messages.end(),
        [](const mxh::net::Message& candidate) {
            return candidate.header.protocol == static_cast<std::uint8_t>(
                       mxh::proto::UserConnProtocol::CharacterAdd) &&
                   candidate.header.object_id == 101u &&
                   candidate.payload.size() == 288u &&
                   candidate.payload[0] == 202u;
        });
    ASSERT_NE(message, reply.messages.end());
    ASSERT_EQ(message->payload.size(), 288u);
    std::uint32_t currentLife = 0;
    std::uint32_t maxLife = 0;
    std::memcpy(&currentLife, message->payload.data() + 35,
                sizeof(currentLife));
    std::memcpy(&maxLife, message->payload.data() + 39,
                sizeof(maxLife));
    EXPECT_EQ(currentLife, 125u);
    EXPECT_EQ(maxLife, 125u);
}

TEST(CharacterAddTargetRoute, TwoWayAndThirdPlayerUseOneDeliveryPerPair) {
    MockDbAdapter db;
    std::vector<std::pair<std::uint64_t, mxh::net::Message>> delivered;
    mxh::server::AgentHandler agent(db,
        [&](mxh::net::ConnectionId id, const mxh::net::Message& msg) {
            delivered.emplace_back(id.value, msg);
        });
    MockTcpSender map_link;
    const auto map_connection = mxh::net::make_connection_id(77);
    agent.set_map_server_for_map(7u, &map_link, map_connection);
    for (std::uint32_t player : {101u, 202u, 303u}) {
        agent.register_session(mxh::net::make_connection_id(1000u + player),
                               player, player, 7u);
    }
    std::vector<mxh::net::Message> map_emitted;
    mxh::server::MapHandler map(db, 7u,
        [&](mxh::net::ConnectionId id, const mxh::net::Message& msg) {
            if (msg.header.protocol == static_cast<std::uint8_t>(
                    mxh::proto::UserConnProtocol::CharacterAdd)) {
                map_emitted.push_back(msg);
            }
            agent.forward_from_map(id, msg, 7u, &map_link);
        });
    const auto enter = [&](std::uint32_t player) {
        mxh::net::Message game_in;
        game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
        game_in.header.protocol = static_cast<std::uint8_t>(
            mxh::proto::UserConnProtocol::GameInSyn);
        game_in.header.object_id = player;
        map.on_message(map_connection, game_in);
    };
    enter(101u);
    enter(202u);
    enter(303u);

    std::vector<std::pair<std::uint32_t, std::uint32_t>> pairs;
    for (const auto& [connection, msg] : delivered) {
        if (msg.header.protocol != static_cast<std::uint8_t>(
                mxh::proto::UserConnProtocol::CharacterAdd)) continue;
        ASSERT_GE(connection, 1000u);
        const auto recipient = static_cast<std::uint32_t>(connection - 1000u);
        pairs.emplace_back(recipient, msg.header.object_id);
        ASSERT_EQ(msg.payload.size(), 288u);
        std::uint32_t payload_source = 0;
        std::memcpy(&payload_source, msg.payload.data(), 4);
        EXPECT_EQ(payload_source, msg.header.object_id);
    }
    std::sort(pairs.begin(), pairs.end());
    EXPECT_EQ(pairs, (std::vector<std::pair<std::uint32_t, std::uint32_t>>{
        {101u, 202u}, {101u, 303u}, {202u, 101u},
        {202u, 303u}, {303u, 101u}, {303u, 202u}}));
    ASSERT_EQ(map_emitted.size(), 6u);
    // On the Map-Agent hop, the header identifies the recipient while the
    // payload identifies the source. No new client wire bytes were added.
    EXPECT_EQ(map_emitted.front().header.object_id, 101u);
    EXPECT_EQ(map_emitted.front().payload[0], 202u);
}

TEST(CharacterAddTargetRoute, MapSendsOnlyOnRecipientsOwnLink) {
    MockDbAdapter db;
    std::vector<std::pair<std::uint64_t, mxh::net::Message>> sent;
    mxh::server::MapHandler map(db, 7u,
        [&](mxh::net::ConnectionId id, const mxh::net::Message& msg) {
            if (msg.header.protocol == static_cast<std::uint8_t>(
                    mxh::proto::UserConnProtocol::CharacterAdd)) {
                sent.emplace_back(id.value, msg);
            }
        });
    const auto enter = [&](std::uint32_t player, std::uint64_t connection) {
        mxh::net::Message game_in;
        game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
        game_in.header.protocol = static_cast<std::uint8_t>(
            mxh::proto::UserConnProtocol::GameInSyn);
        game_in.header.object_id = player;
        map.on_message(mxh::net::make_connection_id(connection), game_in);
    };
    enter(101u, 51u);
    enter(202u, 52u);
    ASSERT_EQ(sent.size(), 2u);
    EXPECT_EQ(sent[0].first, 51u);
    EXPECT_EQ(sent[0].second.header.object_id, 101u);
    EXPECT_EQ(sent[0].second.payload[0], 202u);
    EXPECT_EQ(sent[1].first, 52u);
    EXPECT_EQ(sent[1].second.header.object_id, 202u);
    EXPECT_EQ(sent[1].second.payload[0], 101u);
}

TEST(CharacterAddTargetRoute, RejectsCrossMapMalformedAndStaleMapLink) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler agent(db, make_reply_spy(reply));
    MockTcpSender old_link, new_link;
    const auto old_connection = mxh::net::make_connection_id(77);
    const auto new_connection = mxh::net::make_connection_id(88);
    agent.set_map_server_for_map(7u, &old_link, old_connection);
    agent.register_session(mxh::net::make_connection_id(1101), 1u, 101u, 7u);
    agent.register_session(mxh::net::make_connection_id(1202), 2u, 202u, 7u);
    agent.register_session(mxh::net::make_connection_id(1404), 4u, 404u, 8u);

    mxh::net::Message add;
    add.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    add.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::CharacterAdd);
    add.header.object_id = 101u;
    add.payload.resize(288u);
    const std::uint32_t source = 202u;
    const std::uint16_t map_num = 7u;
    std::memcpy(add.payload.data(), &source, 4);
    std::memcpy(add.payload.data() + 4, &source, 4);
    std::memcpy(add.payload.data() + 77, &map_num, 2);
    std::memcpy(add.payload.data() + 79, &map_num, 2);
    const auto rejected = [&](const mxh::net::Message& candidate,
                              mxh::net::ConnectionId connection,
                              mxh::net::ITcpSender* link) {
        const auto before = reply.call_count.load();
        agent.forward_from_map(connection, candidate, 7u, link);
        EXPECT_EQ(reply.call_count.load(), before);
    };
    auto wrong = add;
    wrong.header.object_id = 404u;
    rejected(wrong, old_connection, &old_link);
    wrong = add;
    wrong.payload.pop_back();
    rejected(wrong, old_connection, &old_link);
    wrong = add;
    wrong.payload[4] = 0;
    rejected(wrong, old_connection, &old_link);
    wrong = add;
    wrong.payload[79] = 8;
    rejected(wrong, old_connection, &old_link);
    const auto before_wrong_ingress = reply.call_count.load();
    agent.forward_from_map(old_connection, add, 8u, &old_link);
    EXPECT_EQ(reply.call_count.load(), before_wrong_ingress);
    agent.set_map_server_for_map(7u, &new_link, new_connection);
    rejected(add, old_connection, &old_link);

    agent.forward_from_map(new_connection, add, 7u, &new_link);
    ASSERT_EQ(reply.call_count.load(), 1);
    EXPECT_EQ(reply.last_id.value, 1101u);
    EXPECT_EQ(reply.last_message.header.object_id, source);
    EXPECT_EQ(reply.last_message.payload, add.payload);
    // An appearance refresh targeting the source itself remains valid.
    add.header.object_id = source;
    agent.forward_from_map(new_connection, add, 7u, &new_link);
    ASSERT_EQ(reply.call_count.load(), 2);
    EXPECT_EQ(reply.last_id.value, 1202u);
}

TEST(MapHandlerTest, GameInClaimsValidPendingGmGrantExactlyOnce) {
    auto db = mxh::db::make_adapter("sqlite"); mxh::db::ConnectionConfig cfg; cfg.path = ":memory:";
    ASSERT_TRUE(db->connect(cfg).ok());
    ASSERT_TRUE(db->execute("CREATE TABLE character_info (chrid INTEGER PRIMARY KEY,charname TEXT,level INTEGER,sex_type INTEGER,face_type INTEGER,hair_type INTEGER,height REAL,width REAL,map_num INTEGER)").ok());
    ASSERT_TRUE(db->execute("INSERT INTO character_info VALUES (123,'GrantHero',1,0,0,0,1,1,7)").ok());
    ASSERT_TRUE(seed_fixture_base_attributes(*db, 123u));
    ASSERT_TRUE(seed_fixture_runtime_state(*db, 123u));
    ASSERT_TRUE(db->execute("CREATE TABLE modern_item_grant (grant_id INTEGER PRIMARY KEY,idempotency_key TEXT UNIQUE,character_id INTEGER,item_id INTEGER,item_count INTEGER,status TEXT,inventory_slot INTEGER,created_by TEXT,reason TEXT,created_at TEXT,claimed_at TEXT)").ok());
    ASSERT_TRUE(db->execute("INSERT INTO modern_item_grant VALUES (1,'once',123,8000,3,'pending',NULL,'gm','test',CURRENT_TIMESTAMP,NULL)").ok());
    ReplySpy reply; mxh::server::MapHandler handler(*db, 7, make_reply_spy(reply));
    const std::string item_path = std::string(MXH_SOURCE_DIR) + "/data/PlayDH/Resource/ItemList.bin";
    handler.load_item_list(item_path);
    mxh::net::Message game_in; game_in.header.object_id = 123;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(mxh::net::make_connection_id(55), game_in);
    const auto snapshot = handler.player_runtime_snapshot(123); ASSERT_TRUE(snapshot); EXPECT_EQ(snapshot->inventory_count, 1u);
    EXPECT_EQ(handler.claim_pending_item_grants_for_test(123), 0u);
    mxh::db::ResultSet rows; ASSERT_TRUE(db->query("SELECT status,inventory_slot FROM modern_item_grant WHERE grant_id=1", rows).ok());
    EXPECT_EQ(std::get<std::string>(rows.rows[0][0]), "claimed");
    EXPECT_EQ(std::get<std::int64_t>(rows.rows[0][1]), 0);
}

TEST(MapHandlerTest, GameInAckEmbedsCurrentItemLayoutWithoutLocalItemPacket) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);

    handler.on_message(mxh::net::make_connection_id(55), game_in);

    ASSERT_FALSE(reply.messages.empty());
    EXPECT_EQ(reply.call_count.load(), static_cast<int>(reply.messages.size()));
    const auto& ack = reply.messages.front();
    EXPECT_EQ(ack.header.category,
              static_cast<std::uint8_t>(mxh::proto::Category::UserConn));
    EXPECT_EQ(ack.header.protocol,
              static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInAck));
    ASSERT_EQ(ack.payload.size(), mxh::game::HERO_TOTAL_EMPTY_PAYLOAD_SIZE);
    std::uint16_t spawn_x = 0, spawn_z = 0;
    std::memcpy(&spawn_x, ack.payload.data() + mxh::game::HERO_TOTAL_MOVE_OFFSET, sizeof(spawn_x));
    std::memcpy(&spawn_z, ack.payload.data() + mxh::game::HERO_TOTAL_MOVE_OFFSET + 2, sizeof(spawn_z));
    EXPECT_EQ(spawn_x, 25000u);
    EXPECT_EQ(spawn_z, 25000u);
    std::uint32_t wire_hp = 0;
    std::uint16_t wire_gengol = 0;
    std::memcpy(&wire_hp, ack.payload.data() + mxh::game::HERO_TOTAL_CHARACTER_OFFSET, sizeof(wire_hp));
    std::memcpy(&wire_gengol, ack.payload.data() + mxh::game::HERO_TOTAL_HERO_OFFSET, sizeof(wire_gengol));
    EXPECT_EQ(wire_hp, 125u);
    EXPECT_EQ(wire_gengol, 12u);
    for (std::size_t offset = mxh::game::HERO_TOTAL_ITEM_OFFSET;
         offset < mxh::game::HERO_TOTAL_OPTION_COUNTS_OFFSET; ++offset) {
        EXPECT_EQ(ack.payload[offset], 0u);
    }
    EXPECT_EQ(ack.payload[mxh::game::HERO_TOTAL_ADDABLE_INFO_OFFSET], 0u);
    EXPECT_EQ(ack.payload[mxh::game::HERO_TOTAL_ADDABLE_INFO_OFFSET + 1], 0u);
    for (const auto& message : reply.messages) {
        EXPECT_NE(message.header.category,
                  static_cast<std::uint8_t>(mxh::proto::Category::Item));
    }
}

TEST(MapHandlerTest, GameInRestoresProgressAndSerializesLegacyHeroOffsets) {
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg{};
    cfg.backend = "sqlite";
    cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());
    ASSERT_TRUE(db.exec_multi(
        "CREATE TABLE character_info (chrid INTEGER PRIMARY KEY,charname TEXT,sex_type INTEGER,"
        "face_type INTEGER,hair_type INTEGER,height REAL,width REAL,level INTEGER,map_num INTEGER);"
        "CREATE TABLE modern_player_state (player_id INTEGER PRIMARY KEY,money INTEGER NOT NULL,"
        "level INTEGER NOT NULL,exp INTEGER NOT NULL,updated_at TEXT NOT NULL);"
        "INSERT INTO character_info VALUES(123,'PersistHero',0,1,1,1.0,1.0,1,7);"
        "INSERT INTO modern_player_state VALUES(123,7654321,12,34567,'now');").ok());
    ASSERT_TRUE(seed_fixture_base_attributes(db, 123u));
    ASSERT_TRUE(seed_fixture_runtime_state(db, 123u, 12u, 34567u, 7654321u));

    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(mxh::net::make_connection_id(55), game_in);

    const auto snapshot = handler.player_runtime_snapshot(123u);
    ASSERT_TRUE(snapshot.has_value());
    EXPECT_EQ(snapshot->level, 12u);
    EXPECT_EQ(snapshot->level_exp, 34567u);
    EXPECT_EQ(handler.player_money_for_test(123u), 7654321u);
    ASSERT_FALSE(reply.messages.empty());
    const auto& ack = reply.messages.front();
    std::uint16_t wire_level = 0;
    std::int64_t wire_exp = 0;
    std::uint32_t wire_money = 0;
    std::memcpy(&wire_level, ack.payload.data() + mxh::game::HERO_TOTAL_CHARACTER_OFFSET + 40, sizeof(wire_level));
    std::memcpy(&wire_exp, ack.payload.data() + mxh::game::HERO_TOTAL_HERO_OFFSET + 22, sizeof(wire_exp));
    std::memcpy(&wire_money, ack.payload.data() + mxh::game::HERO_TOTAL_HERO_OFFSET + 36, sizeof(wire_money));
    EXPECT_EQ(wire_level, 12u);
    EXPECT_EQ(wire_exp, 34567);
    EXPECT_EQ(wire_money, 7654321u);
}

TEST(MapHandlerTest, InventoryAndEquipmentSurviveDisconnectAndRelogin) {
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg{};
    cfg.backend = "sqlite";
    cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());
    ASSERT_TRUE(db.exec_multi(
        "CREATE TABLE character_info (chrid INTEGER PRIMARY KEY,charname TEXT,sex_type INTEGER,"
        "face_type INTEGER,hair_type INTEGER,height REAL,width REAL,level INTEGER,map_num INTEGER);"
        "CREATE TABLE modern_player_item (player_id INTEGER NOT NULL,container INTEGER NOT NULL,slot INTEGER NOT NULL,"
        "db_idx INTEGER NOT NULL,item_idx INTEGER NOT NULL,durability INTEGER NOT NULL,rare_idx INTEGER NOT NULL,"
        "quick_position INTEGER NOT NULL,item_param INTEGER NOT NULL,PRIMARY KEY(player_id,container,slot),"
        "UNIQUE(player_id,db_idx));"
        "INSERT INTO character_info VALUES(123,'ItemHero',0,1,1,1.0,1.0,1,7);"
        "INSERT INTO modern_player_item VALUES(123,0,4,9001,555,87,3,65535,8);"
        "INSERT INTO modern_player_item VALUES(123,1,1,9002,11000,100,0,65535,1);").ok());
    ASSERT_TRUE(seed_fixture_base_attributes(db, 123u));
    ASSERT_TRUE(seed_fixture_runtime_state(db, 123u));

    const auto connection = mxh::net::make_connection_id(55);
    {
        ReplySpy reply;
        mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
        handler.load_item_list(std::string(MXH_SOURCE_DIR) + "/data/PlayDH/Resource/ItemList.bin");
        mxh::net::Message game_in;
        game_in.header.object_id = 123u;
        game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
        game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
        handler.on_message(connection, game_in);
        const auto snapshot = handler.player_runtime_snapshot(123u);
        ASSERT_TRUE(snapshot.has_value());
        EXPECT_EQ(snapshot->inventory_count, 1u);
        ASSERT_FALSE(reply.messages.empty());
        mxh::game::ItemTotalInfo items{};
        std::memcpy(&items, reply.messages.front().payload.data() + mxh::game::HERO_TOTAL_ITEM_OFFSET, sizeof(items));
        EXPECT_EQ(items.Inventory[4].dwDBIdx, 9001u);
        EXPECT_EQ(items.Inventory[4].wIconIdx, 555u);
        EXPECT_EQ(items.WearedItem[1].dwDBIdx, 9002u);
        EXPECT_EQ(items.WearedItem[1].wIconIdx, 11000u);
        handler.on_disconnect(connection, mxh::net::NetError::Disconnected);
    }
    mxh::db::ResultSet rows;
    const std::vector<mxh::db::Bind> no_args;
    ASSERT_TRUE(db.query("SELECT container,slot,item_idx,item_param FROM modern_player_item "
                         "WHERE player_id=123 ORDER BY container,slot", no_args, rows).ok());
    ASSERT_EQ(rows.rows.size(), 2u);
    EXPECT_EQ(std::get<std::int64_t>(rows.rows[0][1]), 4);
    EXPECT_EQ(std::get<std::int64_t>(rows.rows[0][2]), 555);
    EXPECT_EQ(std::get<std::int64_t>(rows.rows[1][1]), 1);
    EXPECT_EQ(std::get<std::int64_t>(rows.rows[1][2]), 11000);
}

TEST(MapHandlerTest, LoadedGrantIdDoesNotAdvanceOrdinaryItemAllocatorPastBoundary) {
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg{};
    cfg.backend = "sqlite";
    cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());
    ASSERT_TRUE(db.exec_multi(
        "CREATE TABLE character_info (chrid INTEGER PRIMARY KEY,charname TEXT,sex_type INTEGER,"
        "face_type INTEGER,hair_type INTEGER,height REAL,width REAL,level INTEGER,map_num INTEGER);"
        "CREATE TABLE modern_player_item (player_id INTEGER NOT NULL,container INTEGER NOT NULL,slot INTEGER NOT NULL,"
        "db_idx INTEGER NOT NULL,item_idx INTEGER NOT NULL,durability INTEGER NOT NULL,rare_idx INTEGER NOT NULL,"
        "quick_position INTEGER NOT NULL,item_param INTEGER NOT NULL,PRIMARY KEY(player_id,container,slot),"
        "UNIQUE(player_id,db_idx));"
        "INSERT INTO character_info VALUES(123,'GrantHero',0,1,1,1.0,1.0,1,7);"
        "INSERT INTO modern_player_item VALUES(123,0,4,2147483649,555,100,0,65535,1);"
        "INSERT INTO modern_player_item VALUES(123,2,3,9003,777,70,2,4,5);"
        "INSERT INTO modern_player_item VALUES(123,5,2,9004,888,80,3,6,7);"
        "INSERT INTO modern_player_item VALUES(999,0,0,2147483646,556,100,0,65535,1);").ok());
    ASSERT_TRUE(seed_fixture_base_attributes(db, 123u));
    ASSERT_TRUE(seed_fixture_runtime_state(db, 123u));

    ReplySpy reply;
    MapHandler handler(db, 7, make_reply_spy(reply));
    const auto connection = mxh::net::make_connection_id(55);
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);
    ASSERT_EQ(handler.player_runtime_snapshot(123u)->inventory_count, 1u);

    const auto first = handler.create_ground_drop_for_test(50000u, 77u, 1u, 25000.0f, 25000.0f);
    ASSERT_TRUE(first.has_value());
    reply.messages.clear();
    handler.on_message(connection, mxh::client::make_pickup_message(123u, first->object_id));
    ASSERT_EQ(reply.messages.size(), 2u);
    EXPECT_EQ(reply.messages[0].header.protocol,
        static_cast<std::uint8_t>(mxh::proto::ItemProtocol::PickupAck));
    mxh::game::ItemTotalInfo total{};
    ASSERT_EQ(reply.messages[1].payload.size(), sizeof(total));
    std::memcpy(&total, reply.messages[1].payload.data(), sizeof(total));
    EXPECT_EQ(total.Inventory[0].dwDBIdx, 2147483647u);
    EXPECT_EQ(total.Inventory[4].dwDBIdx, 2147483649u);

    const auto second = handler.create_ground_drop_for_test(50001u, 78u, 1u, 25000.0f, 25000.0f);
    ASSERT_TRUE(second.has_value());
    reply.messages.clear();
    handler.on_message(connection, mxh::client::make_pickup_message(123u, second->object_id));
    ASSERT_EQ(reply.messages.size(), 1u);
    EXPECT_EQ(reply.messages[0].header.protocol,
        static_cast<std::uint8_t>(mxh::proto::ItemProtocol::PickupNack));
    EXPECT_EQ(handler.player_runtime_snapshot(123u)->inventory_count, 2u);

    mxh::db::ResultSet rows;
    ASSERT_TRUE(db.query("SELECT db_idx FROM modern_player_item WHERE player_id=123 AND container=0 ORDER BY slot", {}, rows).ok());
    ASSERT_EQ(rows.rows.size(), 2u);
    EXPECT_EQ(std::get<std::int64_t>(rows.rows[0][0]), 2147483647);
    EXPECT_EQ(std::get<std::int64_t>(rows.rows[1][0]), 2147483649);
    rows = {};
    ASSERT_TRUE(db.query("SELECT container,slot,db_idx,item_idx,durability,rare_idx,quick_position,item_param "
                         "FROM modern_player_item WHERE player_id=123 AND container IN (2,5) ORDER BY container",
                         {}, rows).ok());
    ASSERT_EQ(rows.rows.size(), 2u);
    const std::array<std::array<std::int64_t, 8>, 2> expected{{
        {{2,3,9003,777,70,2,4,5}}, {{5,2,9004,888,80,3,6,7}}
    }};
    for (std::size_t i = 0; i < expected.size(); ++i)
        for (std::size_t j = 0; j < expected[i].size(); ++j)
            EXPECT_EQ(std::get<std::int64_t>(rows.rows[i][j]), expected[i][j]);
}

TEST(MapHandlerTest, MoveSynSwapsAuthoritativeSlotsAndPersistsDestination) {
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg{}; cfg.backend = "sqlite"; cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());
    ASSERT_TRUE(db.exec_multi(
        "CREATE TABLE modern_player_item (player_id INTEGER NOT NULL,container INTEGER NOT NULL,slot INTEGER NOT NULL,"
        "db_idx INTEGER NOT NULL,item_idx INTEGER NOT NULL,durability INTEGER NOT NULL,rare_idx INTEGER NOT NULL,"
        "quick_position INTEGER NOT NULL,item_param INTEGER NOT NULL,PRIMARY KEY(player_id,container,slot),"
        "UNIQUE(player_id,db_idx));").ok());
    ASSERT_TRUE(create_fixture_character(db, 123u));
    ASSERT_TRUE(seed_fixture_runtime_state(db, 123u));
    ReplySpy reply; mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    mxh::net::Message game_in; game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    const auto connection = mxh::net::make_connection_id(55);
    handler.on_message(connection, game_in);
    ASSERT_TRUE(handler.add_player_item_for_test(123u, mxh::game::make_item(9001u, 555u, 4u)));
    mxh::net::Message move; move.header.object_id = 123u;
    move.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    move.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::MoveSyn);
    move.payload.resize(24u, 0u);
    const std::uint32_t db_idx = 9001u; const std::uint16_t destination = 9u;
    std::memcpy(move.payload.data(), &db_idx, sizeof(db_idx));
    std::memcpy(move.payload.data() + 22u, &destination, sizeof(destination));
    handler.on_message(connection, move);
    mxh::db::ResultSet rows; const std::vector<mxh::db::Bind> no_args;
    ASSERT_TRUE(db.query("SELECT slot,item_idx FROM modern_player_item WHERE player_id=123", no_args, rows).ok());
    ASSERT_EQ(rows.rows.size(), 1u);
    EXPECT_EQ(std::get<std::int64_t>(rows.rows[0][0]), 9);
    EXPECT_EQ(std::get<std::int64_t>(rows.rows[0][1]), 555);
}

TEST(MapHandlerTest, MoveSynMovesInventoryItemIntoEquipmentAndPersistsContainer) {
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg{}; cfg.backend = "sqlite"; cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());
    ASSERT_TRUE(db.exec_multi(
        "CREATE TABLE modern_player_item (player_id INTEGER NOT NULL,container INTEGER NOT NULL,slot INTEGER NOT NULL,"
        "db_idx INTEGER NOT NULL,item_idx INTEGER NOT NULL,durability INTEGER NOT NULL,rare_idx INTEGER NOT NULL,"
        "quick_position INTEGER NOT NULL,item_param INTEGER NOT NULL,PRIMARY KEY(player_id,container,slot),"
        "UNIQUE(player_id,db_idx));").ok());
    ASSERT_TRUE(create_fixture_character(db, 123u));
    ASSERT_TRUE(seed_fixture_runtime_state(db, 123u));
    ReplySpy reply; mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    mxh::net::Message game_in; game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    const auto connection = mxh::net::make_connection_id(55);
    handler.on_message(connection, game_in);
    mxh::game::ItemInfo equip_info{};
    equip_info.ItemIdx = 601u;
    equip_info.ItemKind = 2048u;  // eEQUIP_ITEM
    equip_info.EquipKind = 0u;    // eWearedItem_Hat
    handler.add_item_info_for_test(equip_info);
    ASSERT_TRUE(handler.add_player_item_for_test(123u, mxh::game::make_item(9101u, 601u, 0u)));

    mxh::net::Message move; move.header.object_id = 123u;
    move.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    move.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::MoveSyn);
    move.payload.resize(24u, 0u);
    const std::uint32_t db_idx = 9101u;
    const std::uint16_t destination = mxh::game::TP_WEAREDITEM_START;
    std::memcpy(move.payload.data(), &db_idx, sizeof(db_idx));
    std::memcpy(move.payload.data() + 22u, &destination, sizeof(destination));
    handler.on_message(connection, move);

    mxh::db::ResultSet rows; const std::vector<mxh::db::Bind> no_args;
    ASSERT_TRUE(db.query("SELECT container,slot,item_idx FROM modern_player_item WHERE player_id=123",
                         no_args, rows).ok());
    ASSERT_EQ(rows.rows.size(), 1u);
    EXPECT_EQ(std::get<std::int64_t>(rows.rows[0][0]), 1);
    EXPECT_EQ(std::get<std::int64_t>(rows.rows[0][1]), 0);
    EXPECT_EQ(std::get<std::int64_t>(rows.rows[0][2]), 601);
    ASSERT_FALSE(reply.messages.empty());
    EXPECT_EQ(reply.messages.back().header.protocol,
              static_cast<std::uint8_t>(mxh::proto::ItemProtocol::TotalInfoLocal));
    EXPECT_EQ(reply.messages.back().payload.size(), sizeof(mxh::game::ItemTotalInfo));
}

TEST(MapHandlerTest, MoveSynRejectsKnownNonEquipmentItemIntoEquipment) {
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg{}; cfg.backend = "sqlite"; cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());
    ASSERT_TRUE(db.exec_multi(
        "CREATE TABLE modern_player_item (player_id INTEGER NOT NULL,container INTEGER NOT NULL,slot INTEGER NOT NULL,"
        "db_idx INTEGER NOT NULL,item_idx INTEGER NOT NULL,durability INTEGER NOT NULL,rare_idx INTEGER NOT NULL,"
        "quick_position INTEGER NOT NULL,item_param INTEGER NOT NULL,PRIMARY KEY(player_id,container,slot),"
        "UNIQUE(player_id,db_idx));").ok());
    ASSERT_TRUE(create_fixture_character(db, 123u));
    ASSERT_TRUE(seed_fixture_runtime_state(db, 123u));
    ReplySpy reply; mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    mxh::game::ItemInfo item_info{};
    item_info.ItemIdx = 601u;
    item_info.ItemKind = 0u;
    handler.add_item_info_for_test(item_info);
    mxh::net::Message game_in; game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    const auto connection = mxh::net::make_connection_id(55);
    handler.on_message(connection, game_in);
    ASSERT_TRUE(handler.add_player_item_for_test(123u, mxh::game::make_item(9101u, 601u, 0u)));
    mxh::net::Message move; move.header.object_id = 123u;
    move.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    move.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::MoveSyn);
    move.payload.resize(24u, 0u);
    const std::uint32_t db_idx = 9101u;
    const std::uint16_t destination = mxh::game::TP_WEAREDITEM_START;
    std::memcpy(move.payload.data(), &db_idx, sizeof(db_idx));
    std::memcpy(move.payload.data() + 22u, &destination, sizeof(destination));
    handler.on_message(connection, move);
    ASSERT_FALSE(reply.messages.empty());
    EXPECT_EQ(reply.messages.back().header.protocol,
              static_cast<std::uint8_t>(mxh::proto::ItemProtocol::MoveNack));
    const auto snapshot = handler.player_runtime_snapshot(123u);
    ASSERT_TRUE(snapshot.has_value());
    EXPECT_EQ(snapshot->inventory_count, 1u);
}

TEST(MapHandlerTest, MoveSynRejectsEquipmentInWrongWearSlot) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    mxh::game::ItemInfo item_info{};
    item_info.ItemIdx = 602u;
    item_info.ItemKind = 2048u;  // eEQUIP_ITEM
    item_info.EquipKind = 1u;    // weapon slot, not hat slot 0
    handler.add_item_info_for_test(item_info);
    mxh::net::Message game_in; game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    const auto connection = mxh::net::make_connection_id(55);
    handler.on_message(connection, game_in);
    ASSERT_TRUE(handler.add_player_item_for_test(123u, mxh::game::make_item(9102u, 602u, 0u)));

    mxh::net::Message move; move.header.object_id = 123u;
    move.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    move.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::MoveSyn);
    move.payload.resize(24u, 0u);
    const std::uint32_t db_idx = 9102u;
    const std::uint16_t destination = mxh::game::TP_WEAREDITEM_START; // hat
    std::memcpy(move.payload.data(), &db_idx, sizeof(db_idx));
    std::memcpy(move.payload.data() + 22u, &destination, sizeof(destination));
    handler.on_message(connection, move);
    ASSERT_FALSE(reply.messages.empty());
    EXPECT_EQ(reply.messages.back().header.protocol,
              static_cast<std::uint8_t>(mxh::proto::ItemProtocol::MoveNack));
}

TEST(MapHandlerTest, MoveSynRejectsEquipmentAbovePlayerLevelLimit) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    mxh::game::ItemInfo item_info{};
    item_info.ItemIdx = 603u;
    item_info.ItemKind = 2048u;
    item_info.EquipKind = 0u;
    item_info.LimitLevel = 5u;
    handler.add_item_info_for_test(item_info);
    mxh::net::Message game_in; game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    const auto connection = mxh::net::make_connection_id(55);
    handler.on_message(connection, game_in);
    ASSERT_TRUE(handler.add_player_item_for_test(123u, mxh::game::make_item(9103u, 603u, 0u)));

    mxh::net::Message move; move.header.object_id = 123u;
    move.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    move.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::MoveSyn);
    move.payload.resize(24u, 0u);
    const std::uint32_t db_idx = 9103u;
    const std::uint16_t destination = mxh::game::TP_WEAREDITEM_START;
    std::memcpy(move.payload.data(), &db_idx, sizeof(db_idx));
    std::memcpy(move.payload.data() + 22u, &destination, sizeof(destination));
    handler.on_message(connection, move);
    ASSERT_FALSE(reply.messages.empty());
    EXPECT_EQ(reply.messages.back().header.protocol,
              static_cast<std::uint8_t>(mxh::proto::ItemProtocol::MoveNack));
}

TEST(MapHandlerTest, MoveSynRejectsCrossEquipmentSwapThatWouldMisplaceTarget) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    mxh::game::ItemInfo hat{};
    hat.ItemIdx = 601u; hat.ItemKind = 2048u; hat.EquipKind = 0u;
    mxh::game::ItemInfo weapon{};
    weapon.ItemIdx = 602u; weapon.ItemKind = 2048u; weapon.EquipKind = 1u;
    handler.add_item_info_for_test(hat);
    handler.add_item_info_for_test(weapon);
    mxh::net::Message game_in; game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    const auto connection = mxh::net::make_connection_id(55);
    handler.on_message(connection, game_in);
    ASSERT_TRUE(handler.add_player_item_for_test(123u, mxh::game::make_item(9101u, 601u, 0u)));
    ASSERT_TRUE(handler.add_player_item_for_test(123u, mxh::game::make_item(9102u, 602u, 1u)));

    const auto move_item = [&](std::uint32_t db_idx, std::uint16_t target) {
        mxh::net::Message move; move.header.object_id = 123u;
        move.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
        move.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::MoveSyn);
        move.payload.resize(24u, 0u);
        std::memcpy(move.payload.data(), &db_idx, sizeof(db_idx));
        std::memcpy(move.payload.data() + 22u, &target, sizeof(target));
        handler.on_message(connection, move);
    };
    move_item(9101u, mxh::game::TP_WEAREDITEM_START);      // hat -> slot 0
    move_item(9102u, mxh::game::TP_WEAREDITEM_START + 1u); // weapon -> slot 1
    move_item(9101u, mxh::game::TP_WEAREDITEM_START + 1u); // hat cannot -> slot 1

    ASSERT_FALSE(reply.messages.empty());
    EXPECT_EQ(reply.messages.back().header.protocol,
              static_cast<std::uint8_t>(mxh::proto::ItemProtocol::MoveNack));
}

TEST(MapHandlerTest, MonsterDeathNotifyReachesClientThenPickupSynClaims) {
    MockDbAdapter db;
    std::vector<mxh::net::Message> replies;
    mxh::server::MapHandler handler(db, 7,
        [&](mxh::net::ConnectionId, const mxh::net::Message& message) {
            replies.push_back(message);
        });
    const auto connection = mxh::net::make_connection_id(55);
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);
    // Exercise the legacy ground-drop wire path independently of the
    // revisioned vitals/EXP award gate, which intentionally withholds drops
    // until the obtain-array transaction is available.
    ASSERT_TRUE(handler.clear_vitals_session_for_test(123u));

    mxh::server::DropTable table;
    table.drop_id = 9u;
    table.monster_kind = 77u;
    table.entries.push_back(mxh::server::DropItemEntry{501u, 10000u, 1u, 1u});
    handler.register_drop_table(table);

    mxh::game::MonsterInstance monster;
    monster.object_id = 88002u;
    monster.monster_kind = 77u;
    monster.map_num = 7u;
    monster.max_life = 1u;
    monster.current_life = 1u;
    monster.drop_item_id = 9u;
    monster.drop_item_ratio = 100u;
    monster.pos_x = 25000.0f;
    monster.pos_z = 25000.0f;
    ASSERT_TRUE(handler.add_monster_instance(monster));
    const auto drop = handler.apply_monster_damage(123u, monster.object_id, 1u, 0u);
    ASSERT_TRUE(drop.has_value());
    const auto notify = std::find_if(replies.begin(), replies.end(), [](const auto& message) {
        return message.header.protocol ==
            static_cast<std::uint8_t>(mxh::proto::ItemProtocol::MonsterObtainNotify);
    });
    ASSERT_NE(notify, replies.end());

    mxh::client::CInGameState state;
    state.on_message(mxh::net::make_connection_id(1), *notify);
    ASSERT_EQ(state.ground_drops().size(), 1u);
    EXPECT_EQ(state.ground_drops()[0].item_id, 501u);

    replies.clear();
    handler.on_message(connection, mxh::client::make_pickup_message(123u, drop->object_id));
    const auto ack = std::find_if(replies.begin(), replies.end(), [](const auto& message) {
        return message.header.protocol ==
            static_cast<std::uint8_t>(mxh::proto::ItemProtocol::PickupAck);
    });
    ASSERT_NE(ack, replies.end());
    state.on_message(mxh::net::make_connection_id(1), *ack);
    EXPECT_TRUE(state.ground_drops().empty());
    EXPECT_EQ(handler.player_runtime_snapshot(123u)->inventory_count, 1u);
}

namespace {
// Hoisted bin-file helpers used by SpeechSynShopListFeedsClientThenBuySynDebitsMoney
// (and the late R-8 / dealitem / quest tests further below). Kept here so the
// SpeechSynShopList test can resolve them in source order. The original
// definitions lived further down (line ~1908/2100/2310); they were moved here
// to avoid a forward-declaration-to-anonymous-namespace mismatch (each anonymous
// namespace in a TU has a unique generated name).
std::filesystem::path write_temp_bin(const std::vector<std::uint8_t>& bytes) {
    static std::atomic<std::uint64_t> sequence{0};
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto serial = sequence.fetch_add(1, std::memory_order_relaxed);
    const auto path = std::filesystem::temp_directory_path() /
        ("mxh_map_handler_load_test_" + std::to_string(stamp) + "_" +
         std::to_string(serial) + ".bin");
    std::ofstream ofs(path, std::ios::binary);
    ofs.write(reinterpret_cast<const char*>(bytes.data()),
               static_cast<std::streamsize>(bytes.size()));
    return path;
}
std::vector<std::uint8_t> synthesize_dealitem_bin(const std::string& text) {
    std::vector<std::uint8_t> payload(text.begin(), text.end());
    const auto encrypted = mxh::compat::encrypt_bin_payload(payload, 42);
    mxh::compat::MhFileHeader header{1, 42, static_cast<std::uint32_t>(payload.size())};
    std::vector<std::uint8_t> raw(sizeof(header) + 1 + encrypted.size() + 1);
    std::memcpy(raw.data(), &header, sizeof(header));
    std::copy(encrypted.begin(), encrypted.end(), raw.begin() + sizeof(header) + 1);
    return raw;
}
std::vector<std::uint8_t> synthesize_item_list_bin(const std::string& text) {
    std::vector<std::uint8_t> out;
    const std::uint32_t version = 1;
    const std::uint32_t type = 3;
    const std::uint32_t file_size = static_cast<std::uint32_t>(text.size());
    auto put_u32le = [&out](std::uint32_t v) {
        out.push_back(static_cast<std::uint8_t>(v & 0xFFu));
        out.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFFu));
        out.push_back(static_cast<std::uint8_t>((v >> 16) & 0xFFu));
        out.push_back(static_cast<std::uint8_t>((v >> 24) & 0xFFu));
    };
    put_u32le(version);
    put_u32le(type);
    put_u32le(file_size);
    out.push_back(0);  // crc1 (not validated)

    std::vector<std::uint8_t> payload(text.begin(), text.end());
    std::uint8_t crc = static_cast<std::uint8_t>(type);
    for (std::uint32_t i = 0; i < payload.size(); ++i) {
        crc = static_cast<std::uint8_t>(crc + payload[i]);
        std::int32_t b = static_cast<std::int32_t>(payload[i])
                       + static_cast<std::int32_t>(i & 0xFFu);
        if (type != 0u && (i % type) == 0u) b += static_cast<std::int32_t>(type);
        payload[i] = static_cast<std::uint8_t>(b & 0xFF);
    }
    out.insert(out.end(), payload.begin(), payload.end());
    out.push_back(crc);  // crc2 (not validated by the parser)
    return out;
}
std::string build_test_row_56(std::uint16_t item_idx, std::uint16_t life_recover,
                              std::uint16_t item_kind = 0u) {
    // 56 columns matching ItemList.bin common-row layout (D6.x field order).
    std::vector<std::string> toks(56u);
    toks[0] = std::to_string(item_idx);
    toks[1] = "HpPotion";
    toks[2] = "0";
    toks[3] = "0";
    toks[4] = std::to_string(item_kind);  // ItemKind
    for (std::size_t i = 5; i < 50; ++i) toks[i] = "0";
    for (std::size_t i = 16; i <= 20; ++i) toks[i] = "0.0";
    for (std::size_t i = 38; i <= 42; ++i) toks[i] = "0.0";
    toks[50] = std::to_string(life_recover);  // LifeRecover
    toks[51] = "0.0";
    toks[52] = "0";
    toks[53] = "0.0";
    toks[54] = "0";
    toks[55] = "1";
    std::ostringstream row;
    for (std::size_t i = 0; i < toks.size(); ++i) {
        if (i != 0) row << "\t";
        row << toks[i];
    }
    return row.str();
}
}  // namespace

TEST(MapHandlerTest, SpeechSynShopListFeedsClientThenBuySynDebitsMoney) {
    MockDbAdapter db;
    std::vector<mxh::net::Message> replies;
    mxh::server::MapHandler handler(db, 7,
        [&](mxh::net::ConnectionId, const mxh::net::Message& message) {
            replies.push_back(message);
        });
    const auto connection = mxh::net::make_connection_id(55);
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);
    ASSERT_TRUE(handler.set_player_money_for_test(123u, 20000u));

    const std::string deal_text = "1 map 2 npc 7 10 20 0 1 tab 555 10\n";
    const auto deal_path = write_temp_bin(synthesize_dealitem_bin(deal_text));
    handler.load_dealitem(deal_path.string());
    std::error_code ec_deal; std::filesystem::remove(deal_path, ec_deal);
    std::vector<std::string> tokens(56u, "0");
    tokens[0] = "555"; tokens[1] = "Potion"; tokens[5] = "12345";
    std::string row;
    for (const auto& t : tokens) { row += t; row.push_back('\t'); }
    row.push_back('\r'); row.push_back('\n');
    const auto item_path = write_temp_bin(synthesize_item_list_bin(row));
    handler.load_item_prices(item_path.string());
    std::error_code ec_item; std::filesystem::remove(item_path, ec_item);

    mxh::net::Message talk;
    talk.header.object_id = 123u;
    talk.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Npc);
    talk.header.protocol = static_cast<std::uint8_t>(mxh::proto::NpcProtocol::SpeechSyn);
    talk.payload.resize(4);
    const std::uint32_t npc_id = 7u;
    std::memcpy(talk.payload.data(), &npc_id, sizeof(npc_id));
    replies.clear();
    handler.on_message(connection, talk);
    const auto shop = std::find_if(replies.begin(), replies.end(), [](const auto& message) {
        return message.header.protocol == mxh::proto::kModernShopList;
    });
    ASSERT_NE(shop, replies.end());

    mxh::client::CInGameState state;
    state.Init(nullptr);
    mxh::net::Message ack;
    ack.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    ack.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInAck);
    ack.payload.assign(mxh::game::HERO_TOTAL_EMPTY_PAYLOAD_SIZE, 0);
    const std::uint32_t pid = 123;
    std::memcpy(ack.payload.data(), &pid, 4);
    state.on_message(mxh::net::make_connection_id(1), ack);
    state.on_message(mxh::net::make_connection_id(1), *shop);
    ASSERT_TRUE(state.shop_open());
    ASSERT_FALSE(state.shop_items().empty());
    EXPECT_EQ(state.shop_npc_id(), 7u);

    replies.clear();
    handler.on_message(connection,
                       mxh::client::make_buy_message(123u, state.shop_items()[0].item_id, 1u));
    EXPECT_EQ(handler.player_money_for_test(123u), 7655u);

    // Dealer purchases must obey the same authoritative interaction radius;
    // a client cannot keep buying after moving away from the live NPC.
    ASSERT_TRUE(handler.set_player_position_for_test(123u, 10000.0f, 10000.0f));
    replies.clear();
    handler.on_message(connection,
                       mxh::client::make_buy_message(123u, state.shop_items()[0].item_id, 1u));
    const auto buy_nack = std::find_if(replies.begin(), replies.end(), [](const auto& message) {
        return message.header.protocol ==
            static_cast<std::uint8_t>(mxh::proto::ItemProtocol::BuyNack);
    });
    ASSERT_NE(buy_nack, replies.end());
    EXPECT_EQ(handler.player_money_for_test(123u), 7655u);
}

TEST(MapHandlerTest, PickupSynClaimsNearbyGroundDropOnce) {
    MockDbAdapter db;
    std::vector<mxh::net::Message> replies;
    mxh::server::MapHandler handler(db, 7,
        [&](mxh::net::ConnectionId, const mxh::net::Message& message) {
            replies.push_back(message);
        });
    const auto connection = mxh::net::make_connection_id(55);
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);
    const auto drop = handler.create_ground_drop_for_test(50000u, 77u, 1u, 25000.0f, 25000.0f);
    ASSERT_TRUE(drop.has_value());

    const auto pickup = mxh::client::make_pickup_message(123u, drop->object_id);
    handler.on_message(connection, pickup);
    const auto ack = std::find_if(replies.begin(), replies.end(), [](const auto& message) {
        return message.header.category == static_cast<std::uint8_t>(mxh::proto::Category::Item) &&
            message.header.protocol == static_cast<std::uint8_t>(mxh::proto::ItemProtocol::PickupAck);
    });
    ASSERT_NE(ack, replies.end());
    ASSERT_EQ(ack->payload.size(), 8u);
    const auto total = std::find_if(replies.begin(), replies.end(), [](const auto& message) {
        return message.header.category == static_cast<std::uint8_t>(mxh::proto::Category::Item) &&
               message.header.protocol == static_cast<std::uint8_t>(mxh::proto::ItemProtocol::TotalInfoLocal);
    });
    ASSERT_NE(total, replies.end());
    ASSERT_EQ(total->payload.size(), sizeof(mxh::game::ItemTotalInfo));
    mxh::game::ItemTotalInfo authoritative{};
    std::memcpy(&authoritative, total->payload.data(), sizeof(authoritative));
    EXPECT_NE(authoritative.Inventory[0].dwDBIdx, drop->object_id);
    EXPECT_EQ(db.item_insert_count, 1);
    EXPECT_EQ(db.last_item_db_idx, authoritative.Inventory[0].dwDBIdx);
    EXPECT_EQ(authoritative.Inventory[0].wIconIdx, 77u);
    EXPECT_EQ(handler.player_runtime_snapshot(123u)->inventory_count, 1u);

    replies.clear();
    handler.on_message(connection, pickup);
    const auto nack = std::find_if(replies.begin(), replies.end(), [](const auto& message) {
        return message.header.protocol ==
            static_cast<std::uint8_t>(mxh::proto::ItemProtocol::PickupNack);
    });
    ASSERT_NE(nack, replies.end());
    EXPECT_EQ(replies.size(), 1u) << "rejected duplicate cannot publish a new inventory snapshot";
}

TEST(MapHandlerTest, PickupWriteFailureKeepsGroundDropAndRejectsAck) {
    MockDbAdapter db;
    std::vector<mxh::net::Message> replies;
    MapHandler handler(db, 7, [&](mxh::net::ConnectionId, const mxh::net::Message& msg) {
        replies.push_back(msg);
    });
    const auto connection = mxh::net::make_connection_id(55);
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);
    const auto drop = handler.create_ground_drop_for_test(50000u, 77u, 1u, 25000.0f, 25000.0f);
    ASSERT_TRUE(drop.has_value());
    const auto pickup = mxh::client::make_pickup_message(123u, drop->object_id);

    db.fail_item_insert = true;
    replies.clear();
    handler.on_message(connection, pickup);
    ASSERT_EQ(replies.size(), 1u);
    EXPECT_EQ(replies[0].header.protocol,
        static_cast<std::uint8_t>(mxh::proto::ItemProtocol::PickupNack));
    EXPECT_EQ(handler.player_runtime_snapshot(123u)->inventory_count, 0u);
    EXPECT_EQ(db.rollbacks, 1);

    db.fail_item_insert = false;
    replies.clear();
    handler.on_message(connection, pickup);
    ASSERT_EQ(replies.size(), 2u);
    EXPECT_EQ(replies[0].header.protocol,
        static_cast<std::uint8_t>(mxh::proto::ItemProtocol::PickupAck));
    EXPECT_EQ(replies[1].header.protocol,
        static_cast<std::uint8_t>(mxh::proto::ItemProtocol::TotalInfoLocal));
    EXPECT_EQ(handler.player_runtime_snapshot(123u)->inventory_count, 1u);
}

TEST(MapHandlerTest, ConcurrentPickupCallbacksSerializeItemTransactions) {
    MockDbAdapter db;
    db.pause_item_insert = true;
    std::mutex replies_mu;
    std::vector<mxh::net::Message> replies;
    MapHandler handler(db, 7, [&](mxh::net::ConnectionId, const mxh::net::Message& msg) {
        std::lock_guard<std::mutex> lock(replies_mu);
        replies.push_back(msg);
    });
    const auto connection = mxh::net::make_connection_id(55);
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);
    const auto first = handler.create_ground_drop_for_test(50000u, 77u, 1u, 25000.0f, 25000.0f);
    const auto second = handler.create_ground_drop_for_test(50001u, 78u, 1u, 25000.0f, 25000.0f);
    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());
    std::atomic<bool> go{false};
    const auto pick = [&](std::uint32_t drop_id) {
        while (!go.load(std::memory_order_acquire)) std::this_thread::yield();
        handler.on_message(connection, mxh::client::make_pickup_message(123u, drop_id));
    };
    std::thread one(pick, first->object_id);
    std::thread two(pick, second->object_id);
    go.store(true, std::memory_order_release);
    one.join();
    two.join();
    EXPECT_FALSE(db.transaction_overlap.load());
    EXPECT_EQ(db.active_transactions.load(), 0);
    EXPECT_EQ(handler.player_runtime_snapshot(123u)->inventory_count, 2u);
    const auto acks = std::count_if(replies.begin(), replies.end(), [](const auto& msg) {
        return msg.header.category == static_cast<std::uint8_t>(mxh::proto::Category::Item) &&
            msg.header.protocol == static_cast<std::uint8_t>(mxh::proto::ItemProtocol::PickupAck);
    });
    EXPECT_EQ(acks, 2);
}

TEST(MapHandlerTest, SpeechSynRejectsLiveNpcOutsideInteractionRange) {
    MockDbAdapter db;
    std::vector<mxh::net::Message> replies;
    MapHandler handler(db, 7,
        [&](mxh::net::ConnectionId, const mxh::net::Message& message) {
            replies.push_back(message);
        });
    const auto deal_path = write_temp_bin(
        synthesize_dealitem_bin("7 map 2 npc 7 10 20 0 1 tab 555 10\n"));
    handler.load_dealitem(deal_path.string());
    std::error_code ignored;
    std::filesystem::remove(deal_path, ignored);

    const auto connection = mxh::net::make_connection_id(55);
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);
    ASSERT_TRUE(handler.set_player_position_for_test(123u, 10000.0f, 10000.0f));

    mxh::net::Message talk;
    talk.header.object_id = 123u;
    talk.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Npc);
    talk.header.protocol = static_cast<std::uint8_t>(mxh::proto::NpcProtocol::SpeechSyn);
    talk.payload.resize(4, 0);
    const std::uint32_t npc_id = 7u;
    std::memcpy(talk.payload.data(), &npc_id, sizeof(npc_id));
    replies.clear();
    handler.on_message(connection, talk);
    ASSERT_FALSE(replies.empty());
    EXPECT_EQ(replies.front().header.protocol,
              static_cast<std::uint8_t>(mxh::proto::NpcProtocol::SpeechNack));
    EXPECT_EQ(replies.front().header.category,
              static_cast<std::uint8_t>(mxh::proto::Category::Npc));

    // A live map must also reject ids that are not among its spawned NPCs;
    // otherwise a forged speech packet could advance a quest or resolve a
    // dealer catalog without a real world target.
    replies.clear();
    const std::uint32_t unknown_npc_id = 999u;
    std::memcpy(talk.payload.data(), &unknown_npc_id, sizeof(unknown_npc_id));
    handler.on_message(connection, talk);
    ASSERT_FALSE(replies.empty());
    EXPECT_EQ(replies.front().header.protocol,
              static_cast<std::uint8_t>(mxh::proto::NpcProtocol::SpeechNack));
}

TEST(MapHandlerTest, ExcessiveClientMoveJumpIsCorrected) {
    MockDbAdapter db;
    std::vector<mxh::net::Message> replies;
    MapHandler handler(db, 7,
        [&](mxh::net::ConnectionId, const mxh::net::Message& message) {
            replies.push_back(message);
        });
    const auto connection = mxh::net::make_connection_id(55);
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);
    ASSERT_TRUE(handler.set_player_position_for_test(123u, 1000.0f, 1000.0f));

    mxh::net::Message move;
    move.header.object_id = 123u;
    move.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Move);
    move.header.protocol = static_cast<std::uint8_t>(mxh::proto::MoveProtocol::OneTarget);
    move.payload.resize(4);
    const std::uint16_t x = 20000u;
    const std::uint16_t z = 20000u;
    std::memcpy(move.payload.data(), &x, sizeof(x));
    std::memcpy(move.payload.data() + 2, &z, sizeof(z));
    handler.on_message(connection, move);

    const auto correction = std::find_if(replies.begin(), replies.end(), [](const auto& message) {
        return message.header.protocol ==
            static_cast<std::uint8_t>(mxh::proto::MoveProtocol::Correction);
    });
    ASSERT_NE(correction, replies.end());
    ASSERT_EQ(correction->payload.size(), 4u);
    std::uint16_t corrected_x = 0;
    std::uint16_t corrected_z = 0;
    std::memcpy(&corrected_x, correction->payload.data(), sizeof(corrected_x));
    std::memcpy(&corrected_z, correction->payload.data() + 2, sizeof(corrected_z));
    EXPECT_EQ(corrected_x, 1000u);
    EXPECT_EQ(corrected_z, 1000u);
    const auto snapshot = handler.player_runtime_snapshot(123u);
    ASSERT_TRUE(snapshot.has_value());
    EXPECT_FLOAT_EQ(snapshot->pos_x, 1000.0f);
    EXPECT_FLOAT_EQ(snapshot->pos_z, 1000.0f);
}

TEST(MapHandlerTest, GroundDropCanBeClaimedExactlyOnce) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    const auto connection = mxh::net::make_connection_id(55);
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);
    const auto drop = handler.create_ground_drop_for_test(50000u, 77u, 3u, 10.0f, 20.0f);
    ASSERT_TRUE(drop.has_value());
    ASSERT_TRUE(handler.claim_ground_drop_for_test(123u, drop->object_id));
    EXPECT_FALSE(handler.claim_ground_drop_for_test(123u, drop->object_id));
    const auto snapshot = handler.player_runtime_snapshot(123u);
    ASSERT_TRUE(snapshot.has_value());
    EXPECT_EQ(snapshot->inventory_count, 1u);
}

// Legacy/fallback ground-extension scenario. Use a deliberately non-canonical
// monster kind so the injected test table exercises the compatibility path;
// canonical MonsterList entries are covered by the revisioned scenario below.
TEST(MapHandlerTest, MonsterDeathCreatesNotifiesAndClaimsGroundDrop) {
    MockDbAdapter db;
    db.seed_ready_vitals(123u, 125u, 245u, 125u);
    std::vector<mxh::net::Message> replies;
    mxh::server::MapHandler handler(db, 7,
        [&](mxh::net::ConnectionId, const mxh::net::Message& message) {
            replies.push_back(message);
        });
    handler.load_experience_curve(std::string(MXH_SOURCE_DIR) +
        "/data/PlayDH/Resource/CharacterExpPoint.bin");
    const auto connection = mxh::net::make_connection_id(55);
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);

    mxh::server::DropTable table;
    table.drop_id = 9u;
    // 0 is the explicit wildcard used by the legacy table registry; the
    // monster kind itself remains outside the canonical MonsterList.
    table.monster_kind = 0u;
    table.entries.push_back(mxh::server::DropItemEntry{501u, 10000u, 1u, 1u});
    handler.register_drop_table(table);

    mxh::game::MonsterInstance monster;
    monster.object_id = 88000u;
    monster.monster_kind = 65535u;
    monster.map_num = 7u;
    monster.max_life = 20u;
    monster.current_life = 20u;
    monster.drop_item_id = 9u;
    monster.drop_item_ratio = 100u;
    monster.exp_reward = 1u;
    monster.pos_x = 10.0f;
    monster.pos_z = 20.0f;
    ASSERT_TRUE(handler.add_monster_instance(monster));

    EXPECT_FALSE(handler.apply_monster_damage(123u, monster.object_id, 19u, 0u).has_value());
    const auto drop = handler.apply_monster_damage(123u, monster.object_id, 1u, 0u);
    ASSERT_TRUE(drop.has_value());
    EXPECT_EQ(drop->source_monster_id, monster.object_id);
    EXPECT_EQ(drop->item_id, 501u);
    EXPECT_EQ(drop->count, 1u);
    EXPECT_FLOAT_EQ(drop->pos_x, 10.0f);
    EXPECT_FLOAT_EQ(drop->pos_z, 20.0f);

    const auto notify = std::find_if(replies.begin(), replies.end(), [](const auto& message) {
        return message.header.category == static_cast<std::uint8_t>(mxh::proto::Category::Item) &&
            message.header.protocol == static_cast<std::uint8_t>(mxh::proto::ItemProtocol::MonsterObtainNotify);
    });
    ASSERT_NE(notify, replies.end());
    EXPECT_EQ(notify->header.object_id, 123u);
    ASSERT_EQ(notify->payload.size(), 20u);

    EXPECT_FALSE(handler.apply_monster_damage(123u, monster.object_id, 1u, 0u).has_value());
    ASSERT_TRUE(handler.claim_ground_drop_for_test(123u, drop->object_id));
    EXPECT_FALSE(handler.claim_ground_drop_for_test(123u, drop->object_id));
    ASSERT_TRUE(handler.player_runtime_snapshot(123u).has_value());
    EXPECT_EQ(handler.player_runtime_snapshot(123u)->inventory_count, 1u);
}

TEST(MapHandlerTest, RevisionedKillPublishesGroundObtainAndPersistsPickup) {
    MockDbAdapter db;
    db.seed_ready_vitals(123u, 125u, 245u, 125u);
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7u, make_reply_spy(reply));
    handler.set_allow_dev_monster_fallback(false);
    handler.load_experience_curve(std::string(MXH_SOURCE_DIR) +
        "/data/PlayDH/Resource/CharacterExpPoint.bin");
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(mxh::net::make_connection_id(55), game_in);
    ASSERT_TRUE(handler.player_runtime_snapshot(123u));

    mxh::server::DropTable table;
    table.drop_id = 9u;
    table.monster_kind = 77u;
    table.entries.push_back(mxh::server::DropItemEntry{501u, 10000u, 1u, 1u});
    handler.register_drop_table(table);
    mxh::game::MonsterInstance monster;
    monster.object_id = 88000u;
    monster.monster_kind = 77u;
    monster.map_num = 7u;
    monster.max_life = monster.current_life = 1u;
    monster.exp_reward = 4u;
    monster.drop_item_id = 9u;
    monster.drop_item_ratio = 100u;
    ASSERT_TRUE(handler.add_monster_instance(monster));
    reply.messages.clear();
    const auto writes_before = db.commit_count;
    const auto drop = handler.apply_monster_damage(123u, monster.object_id, 1u, 0u);
    ASSERT_TRUE(drop.has_value());
    EXPECT_EQ(drop->item_id, 501u);
    EXPECT_EQ(db.commit_count, writes_before + 1);
    EXPECT_EQ(db.progress_rows.at(123u)[1], 4);
    EXPECT_EQ(handler.live_monster_count_for_test(), 0u);
    EXPECT_TRUE(std::any_of(reply.messages.begin(), reply.messages.end(), [](const auto& message) {
        return message.header.category == static_cast<std::uint8_t>(mxh::proto::Category::Item) &&
            message.header.protocol == static_cast<std::uint8_t>(
                mxh::proto::ItemProtocol::MonsterObtainNotify);
    }));
    ASSERT_TRUE(handler.claim_ground_drop_for_test(123u, drop->object_id));
    EXPECT_FALSE(handler.claim_ground_drop_for_test(123u, drop->object_id));
}

TEST(MapHandlerTest, MonsterDeathAwardsExperienceAndSendsLegacyNotification) {
    MockDbAdapter db; std::vector<mxh::net::Message> replies;
    db.seed_ready_vitals(123u, 60u, 80u, 20u);
    mxh::server::MapHandler handler(db, 7, [&](mxh::net::ConnectionId, const auto& message) { replies.push_back(message); });
    const std::string exp_path = std::string(MXH_SOURCE_DIR) + "/data/PlayDH/Resource/CharacterExpPoint.bin";
    handler.load_experience_curve(exp_path);
    mxh::net::Message game_in; game_in.header.object_id = 123; game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(mxh::net::make_connection_id(55), game_in);
    mxh::game::MonsterInstance monster; monster.object_id=88001; monster.monster_kind=77; monster.max_life=1; monster.current_life=1; monster.exp_reward=123;
    ASSERT_TRUE(handler.add_monster_instance(monster));
    (void)handler.apply_monster_damage(123, monster.object_id, 1, 99);
    const auto level = std::find_if(replies.begin(), replies.end(), [](const auto& message) {
        return message.header.category == static_cast<std::uint8_t>(mxh::proto::Category::Character) &&
            message.header.protocol == static_cast<std::uint8_t>(mxh::proto::CharacterProtocol::LevelNotify);
    });
    ASSERT_NE(level, replies.end()); ASSERT_EQ(level->payload.size(), 18u);
    std::uint16_t notified_level = 0;
    std::int64_t remaining = 0, maximum = 0;
    std::memcpy(&notified_level, level->payload.data(), sizeof(notified_level));
    std::memcpy(&remaining, level->payload.data() + 2u, sizeof(remaining));
    std::memcpy(&maximum, level->payload.data() + 10u, sizeof(maximum));
    EXPECT_EQ(notified_level, 2u);
    EXPECT_EQ(remaining, 103);
    EXPECT_GT(maximum, 0);
    const auto points = std::find_if(replies.begin(), replies.end(), [](const auto& message) {
        return message.header.category == static_cast<std::uint8_t>(mxh::proto::Category::Character) &&
            message.header.protocol == static_cast<std::uint8_t>(mxh::proto::CharacterProtocol::LevelUpPointNotify);
    });
    ASSERT_NE(points, replies.end()); ASSERT_EQ(points->payload.size(), 4u);
    std::uint32_t notified_points = 0;
    std::memcpy(&notified_points, points->payload.data(), sizeof(notified_points));
    EXPECT_EQ(notified_points, db.progress_rows.at(123)[5]);
    EXPECT_EQ(db.progress_rows.at(123)[0], 2);
    EXPECT_EQ(db.progress_rows.at(123)[1], 103);
    EXPECT_EQ(handler.live_monster_count_for_test(), 5u);
    const auto leveled = handler.player_runtime_snapshot(123u);
    ASSERT_TRUE(leveled.has_value());
    EXPECT_EQ(leveled->level, 2u);
    EXPECT_EQ(leveled->max_hp, 130u);
    EXPECT_EQ(leveled->max_shield, 250u);
    EXPECT_EQ(leveled->max_mp, 130u);
    EXPECT_EQ(leveled->current_hp, leveled->max_hp);
    EXPECT_EQ(leveled->current_shield, leveled->max_shield);
    EXPECT_EQ(leveled->current_mp, leveled->max_mp);
    EXPECT_EQ(db.vitals_rows.at(123u)[0], leveled->max_hp);
    EXPECT_EQ(db.vitals_rows.at(123u)[1], leveled->max_shield);
    EXPECT_EQ(db.vitals_rows.at(123u)[2], leveled->max_mp);
}

TEST(MapHandlerTest, ProgressionGameInSnapshotRestoresNonzeroExperienceAndPoints) {
    MockDbAdapter db;
    db.seed_ready_vitals(123u, 130u, 250u, 130u, 2);
    db.progress_rows.at(123u) = MockDbAdapter::ProgressRow{
        2, 103, 42, 2, 0x1f, 10, 0};
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7u, make_reply_spy(reply));
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(mxh::net::make_connection_id(55), game_in);
    const auto ack = std::find_if(reply.messages.begin(), reply.messages.end(), [](const auto& m) {
        return m.header.category == static_cast<std::uint8_t>(mxh::proto::Category::UserConn) &&
            m.header.protocol == static_cast<std::uint8_t>(
                mxh::proto::UserConnProtocol::GameInAck);
    });
    ASSERT_NE(ack, reply.messages.end());
    const auto decoded = mxh::client::parse_legacy_gamein_ack(ack->payload);
    ASSERT_TRUE(decoded);
    EXPECT_EQ(decoded->level, 2u);
    EXPECT_EQ(decoded->exp, 103u);
    EXPECT_EQ(decoded->level_up_points, 10u);
    EXPECT_EQ(decoded->money, 42u);
    EXPECT_EQ(decoded->life, 130u);
    EXPECT_EQ(decoded->max_shield, 250u);
    EXPECT_EQ(decoded->mp, 130u);
    EXPECT_EQ(db.commit_count, 0) << "ready relog must only read authoritative rows";
}

TEST(MapHandlerTest, NormalExperienceSegmentRefillsAndPersistsOnce) {
    MockDbAdapter db;
    db.seed_ready_vitals(123u, 60u, 80u, 20u);
    std::vector<mxh::net::Message> replies;
    mxh::server::MapHandler handler(db, 7, [&](mxh::net::ConnectionId,
        const auto& message) { replies.push_back(message); });
    handler.load_experience_curve(std::string(MXH_SOURCE_DIR) +
        "/data/PlayDH/Resource/CharacterExpPoint.bin");
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(mxh::net::make_connection_id(55), game_in);
    mxh::game::MonsterInstance monster;
    monster.object_id = 88091u;
    monster.monster_kind = 77u;
    monster.max_life = monster.current_life = 1u;
    monster.exp_reward = 4u;  // canonical Lv1 threshold 20: first 20% interval
    ASSERT_TRUE(handler.add_monster_instance(monster));
    (void)handler.apply_monster_damage(123u, monster.object_id, 1u, 99u);
    ASSERT_EQ(db.progress_rows.at(123u)[0], 1);
    EXPECT_EQ(db.progress_rows.at(123u)[1], 4);
    EXPECT_EQ(db.progress_rows.at(123u)[4], 0x11);
    EXPECT_EQ(db.progress_rows.at(123u)[5], 1);
    EXPECT_EQ(db.progress_rows.at(123u)[6], 1);
    const auto runtime = handler.player_runtime_snapshot(123u);
    ASSERT_TRUE(runtime);
    EXPECT_EQ(runtime->current_hp, runtime->max_hp);
    EXPECT_EQ(runtime->current_shield, runtime->max_shield);
    EXPECT_EQ(runtime->current_mp, runtime->max_mp);
    const auto exp = std::find_if(replies.begin(), replies.end(), [](const auto& message) {
        return message.header.category == static_cast<std::uint8_t>(mxh::proto::Category::Character) &&
            message.header.protocol == static_cast<std::uint8_t>(mxh::proto::CharacterProtocol::ExpPointAck);
    });
    ASSERT_NE(exp, replies.end());
    ASSERT_EQ(exp->payload.size(), 9u);
    std::int64_t absolute = 0;
    std::memcpy(&absolute, exp->payload.data(), sizeof(absolute));
    EXPECT_EQ(absolute, 4);
    EXPECT_EQ(exp->payload[8], 0u);
}

TEST(MapHandlerTest, FailedMonsterExperienceTransactionKeepsMonsterAliveAndNoRewardWire) {
    MockDbAdapter db;
    std::vector<mxh::net::Message> replies;
    mxh::server::MapHandler handler(db, 7,
        [&](mxh::net::ConnectionId, const auto& message) { replies.push_back(message); });
    handler.load_experience_curve(std::string(MXH_SOURCE_DIR) +
        "/data/PlayDH/Resource/CharacterExpPoint.bin");
    mxh::net::Message game_in;
    game_in.header.object_id = 123;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(mxh::net::make_connection_id(55), game_in);
    ASSERT_TRUE(handler.player_runtime_snapshot(123).has_value());
    mxh::game::MonsterInstance monster;
    monster.object_id = 88002;
    monster.monster_kind = 77;
    monster.max_life = monster.current_life = 1;
    monster.exp_reward = 123;
    ASSERT_TRUE(handler.add_monster_instance(monster));
    const auto live_before = handler.live_monster_count_for_test();
    replies.clear();
    db.fail_progress_update = true;
    EXPECT_FALSE(handler.apply_monster_damage(123, monster.object_id, 1, 99).has_value());
    EXPECT_EQ(handler.live_monster_count_for_test(), live_before);
    EXPECT_EQ(db.progress_rows.at(123)[1], 0);
    EXPECT_TRUE(std::none_of(replies.begin(), replies.end(), [](const auto& message) {
        return message.header.category == 3u && message.header.protocol == 13u;
    }));
    EXPECT_EQ(db.rollbacks, 1);
    db.fail_progress_update = false;
    EXPECT_FALSE(handler.apply_monster_damage(123, monster.object_id, 1, 99).has_value());
    EXPECT_EQ(handler.live_monster_count_for_test(), live_before - 1u);
    EXPECT_EQ(db.progress_rows.at(123)[0], 2);
    EXPECT_EQ(db.progress_rows.at(123)[1], 103);
}

TEST(MapHandlerTest, MonsterExperiencePersistsAndLevelRestoresOnRelogin) {
    auto db = mxh::db::make_adapter("sqlite"); mxh::db::ConnectionConfig cfg; cfg.path = ":memory:"; ASSERT_TRUE(db->connect(cfg).ok());
    ASSERT_TRUE(db->execute("CREATE TABLE character_info(chrid INTEGER PRIMARY KEY,charname TEXT,sex_type INTEGER,face_type INTEGER,hair_type INTEGER,height REAL,width REAL,level INTEGER,map_num INTEGER)").ok());
    ASSERT_TRUE(db->execute("INSERT INTO character_info VALUES(123,'Hero',0,1,1,1,1,1,7)").ok());
    ASSERT_TRUE(seed_fixture_base_attributes(*db, 123u));
    ASSERT_TRUE(db->execute("CREATE TABLE modern_player_state(player_id INTEGER PRIMARY KEY,money INTEGER DEFAULT 0,level INTEGER DEFAULT 1,exp INTEGER DEFAULT 0,updated_at TEXT)").ok());
    ASSERT_TRUE(seed_fixture_runtime_state(*db, 123u));
    ReplySpy reply;
    {
        mxh::server::MapHandler handler(*db, 7, make_reply_spy(reply));
        handler.load_experience_curve(std::string(MXH_SOURCE_DIR) + "/data/PlayDH/Resource/CharacterExpPoint.bin");
        mxh::net::Message in; in.header.object_id=123; in.header.category=static_cast<std::uint8_t>(mxh::proto::Category::UserConn); in.header.protocol=static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn); handler.on_message(mxh::net::make_connection_id(55), in);
        mxh::game::MonsterInstance monster; monster.object_id=99001; monster.max_life=1; monster.current_life=1; monster.exp_reward=1000000; ASSERT_TRUE(handler.add_monster_instance(monster));
        (void)handler.apply_monster_damage(123, monster.object_id, 1, 99);
    }
    mxh::db::ResultSet saved; ASSERT_TRUE(db->query("SELECT level,exp FROM modern_player_state WHERE player_id=123", saved).ok()); ASSERT_EQ(saved.rows.size(),1u);
    const auto saved_level=std::get<std::int64_t>(saved.rows[0][0]); EXPECT_GT(saved_level,1);
    mxh::server::MapHandler restored(*db, 7, make_reply_spy(reply)); mxh::net::Message in; in.header.object_id=123; in.header.category=static_cast<std::uint8_t>(mxh::proto::Category::UserConn); in.header.protocol=static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn); restored.on_message(mxh::net::make_connection_id(56), in);
    const auto snapshot=restored.player_runtime_snapshot(123); ASSERT_TRUE(snapshot); EXPECT_EQ(snapshot->level, saved_level);
}

TEST(MapHandlerTest, UseConsumesActorItemAndUpdatesVitals) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    const auto connection = mxh::net::make_connection_id(55);
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);
    ASSERT_TRUE(handler.set_player_vitals_for_test(123u, 1u, 1u));
    ASSERT_TRUE(handler.add_player_item_for_test(123u, mxh::game::make_item(9002u, 1u, 0u)));
    ASSERT_EQ(handler.player_runtime_snapshot(123u)->inventory_count, 1u);
    mxh::net::Message use;
    use.header.object_id = 123u;
    use.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    use.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::UseSyn);
    use.payload.resize(2);
    const std::uint16_t pos = 0u;
    std::memcpy(use.payload.data(), &pos, sizeof(pos));
    handler.on_message(connection, use);
    const auto snapshot = handler.player_runtime_snapshot(123u);
    ASSERT_TRUE(snapshot.has_value());
    EXPECT_EQ(snapshot->inventory_count, 0u);
    EXPECT_GT(snapshot->current_hp, 1u);
    EXPECT_GE(reply.call_count.load(), 2);
}

TEST(MapHandlerTest, DiscardRemovesAuthoritativeInventoryItem) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    const auto connection = mxh::net::make_connection_id(55);
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);
    ASSERT_TRUE(handler.add_player_item_for_test(123u, mxh::game::make_item(9001u, 77u, 0u)));
    ASSERT_EQ(handler.player_runtime_snapshot(123u)->inventory_count, 1u);
    mxh::net::Message discard;
    discard.header.object_id = 123u;
    discard.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    discard.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::DiscardSyn);
    discard.payload.resize(2);
    const std::uint16_t pos = 0u;
    std::memcpy(discard.payload.data(), &pos, sizeof(pos));
    handler.on_message(connection, discard);
    EXPECT_EQ(handler.player_runtime_snapshot(123u)->inventory_count, 0u);
}

TEST(MapHandlerTest, MoveUpdatesAuthoritativePlayerPosition) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    const auto connection = mxh::net::make_connection_id(55);
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);
    ASSERT_TRUE(handler.set_player_position_for_test(123u, 0.0f, 0.0f));

    mxh::net::Message move;
    move.header.object_id = 123u;
    move.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Move);
    move.header.protocol = static_cast<std::uint8_t>(mxh::proto::MoveProtocol::Target);
    move.payload.resize(4);
    const std::uint16_t x = 321u;
    const std::uint16_t z = 654u;
    std::memcpy(move.payload.data(), &x, sizeof(x));
    std::memcpy(move.payload.data() + 2, &z, sizeof(z));
    handler.on_message(connection, move);

    const auto snapshot = handler.player_runtime_snapshot(123u);
    ASSERT_TRUE(snapshot.has_value());
    EXPECT_FLOAT_EQ(snapshot->pos_x, 321.0f);
    EXPECT_FLOAT_EQ(snapshot->pos_z, 654.0f);
}

TEST(MapHandlerTest, StopMovePersistsAndRestoresCharacterPosition) {
    auto db = mxh::db::make_adapter("sqlite");
    mxh::db::ConnectionConfig cfg;
    cfg.path = ":memory:";
    ASSERT_TRUE(db->connect(cfg).ok());
    ASSERT_TRUE(db->execute(
        "CREATE TABLE character_info (chrid INTEGER PRIMARY KEY,charname TEXT,"
        "sex_type INTEGER,face_type INTEGER,hair_type INTEGER,height REAL,width REAL,"
        "level INTEGER,map_num INTEGER,position_x REAL NULL,position_z REAL NULL)").ok());
    ASSERT_TRUE(db->execute(
        "INSERT INTO character_info VALUES(123,'PersistedHero',0,1,1,1,1,1,7,NULL,NULL)").ok());
    ASSERT_TRUE(seed_fixture_base_attributes(*db, 123u));
    ASSERT_TRUE(seed_fixture_runtime_state(*db, 123u));
    ReplySpy reply;
    const auto connection = mxh::net::make_connection_id(55);
    {
        mxh::server::MapHandler handler(*db, 7, make_reply_spy(reply));
        mxh::net::Message game_in;
        game_in.header.object_id = 123u;
        game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
        game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
        handler.on_message(connection, game_in);

        mxh::net::Message stop;
        stop.header.object_id = 123u;
        stop.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Move);
        stop.header.protocol = static_cast<std::uint8_t>(mxh::proto::MoveProtocol::Stop);
        stop.payload.resize(4u);
        const std::uint16_t x = 32100u;
        const std::uint16_t z = 32200u;
        std::memcpy(stop.payload.data(), &x, sizeof(x));
        std::memcpy(stop.payload.data() + 2u, &z, sizeof(z));
        handler.on_message(connection, stop);
    }
    mxh::db::ResultSet saved;
    ASSERT_TRUE(db->query("SELECT position_x,position_z FROM character_info WHERE chrid=123", saved).ok());
    ASSERT_EQ(saved.rows.size(), 1u);
    EXPECT_DOUBLE_EQ(std::get<double>(saved.rows[0][0]), 32100.0);
    EXPECT_DOUBLE_EQ(std::get<double>(saved.rows[0][1]), 32200.0);

    mxh::server::MapHandler restored(*db, 7, make_reply_spy(reply));
    mxh::net::Message relog;
    relog.header.object_id = 123u;
    relog.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    relog.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    restored.on_message(mxh::net::make_connection_id(56), relog);
    const auto snapshot = restored.player_runtime_snapshot(123u);
    ASSERT_TRUE(snapshot.has_value());
    EXPECT_FLOAT_EQ(snapshot->pos_x, 32100.0f);
    EXPECT_FLOAT_EQ(snapshot->pos_z, 32200.0f);
}

TEST(MapHandlerTest, ActorVitalsAreAuthoritativeForStateObservation) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(mxh::net::make_connection_id(55), game_in);
    ASSERT_TRUE(handler.set_player_vitals_for_test(123u, 1u, 1u));
    const auto snapshot = handler.player_runtime_snapshot(123u);
    ASSERT_TRUE(snapshot.has_value());
    EXPECT_EQ(snapshot->current_hp, 1u);
    EXPECT_EQ(snapshot->current_mp, 1u);
    EXPECT_LE(snapshot->current_hp, snapshot->max_hp);
    EXPECT_LE(snapshot->current_mp, snapshot->max_mp);
}
TEST(MapHandlerTest, RuntimeSnapshotRemovedOnDisconnect) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    const auto connection = mxh::net::make_connection_id(55);
    handler.on_message(connection, game_in);
    handler.on_disconnect(connection, mxh::net::NetError::Disconnected);
    EXPECT_EQ(handler.player_runtime_count(), 0u);
    EXPECT_FALSE(handler.player_runtime_snapshot(123u).has_value());
}
TEST(MapHandlerTest, InstalledAiGroupsDriveMonsterSpawns) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    mxh::server::AiGroupList groups;
    mxh::server::AiGroupDefinition group;
    group.group_id = 42u;
    mxh::server::AiSpawnDefinition first;
    first.object_kind = 3u;
    first.monster_kind = 101u;
    first.pos_x = 10.0f;
    first.pos_z = 20.0f;
    mxh::server::AiSpawnDefinition second = first;
    second.monster_kind = 102u;
    second.pos_x = 30.0f;
    group.spawns = {first, second};
    groups.groups.push_back(group);

    EXPECT_EQ(handler.install_ai_groups(groups), 2u);
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(mxh::net::make_connection_id(55), game_in);

    EXPECT_EQ(handler.monster_count_for_test(), groups.spawn_count());
}

TEST(MapHandlerTest, CanonicalMonster10BinSpawnsAllGroups) {
    const auto path = std::filesystem::path(MXH_SOURCE_DIR) /
        "data" / "PlayDH" / "Resource" / "Server" / "Monster_10.bin";
    ASSERT_TRUE(std::filesystem::exists(path)) << path.string();
    auto& ai = mxh::server::AISystem::instance();
    struct RestoreEmptyAi {
        mxh::server::AISystem& ai;
        ~RestoreEmptyAi() { ai.load_ai_group_list(); }
    } restore{ai};
    ASSERT_TRUE(ai.load_ai_group_list(path, "playdh-current"));
    EXPECT_EQ(ai.group_list().spawn_count(), 228u);

    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 10, make_reply_spy(reply));
    handler.set_allow_dev_monster_fallback(false);
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(mxh::net::make_connection_id(55), game_in);
    EXPECT_EQ(handler.monster_count_for_test(), 228u);
    const auto live = handler.live_monster_count_for_test();
    EXPECT_EQ(live, 228u);

    std::size_t monster_adds = 0;
    const mxh::net::Message* first_add = nullptr;
    for (const auto& message : reply.messages) {
        if (message.header.category ==
                static_cast<std::uint8_t>(mxh::proto::Category::UserConn) &&
            message.header.protocol ==
                static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::MonsterAdd)) {
            if (first_add == nullptr) first_add = &message;
            ++monster_adds;
        }
    }
    EXPECT_EQ(monster_adds, live);
    ASSERT_NE(first_add, nullptr);
    ASSERT_GE(first_add->payload.size(), 64u);

    mxh::client::CInGameState state;
    auto client_add = *first_add;
    std::memcpy(&client_add.header.object_id, client_add.payload.data(), 4u);
    state.on_message({}, client_add);
    ASSERT_EQ(state.monsters().size(), 1u);
    EXPECT_EQ(state.monsters().front().object_id, client_add.header.object_id);
    const auto kind = static_cast<std::uint16_t>(
        first_add->payload[43] | (static_cast<std::uint16_t>(first_add->payload[44]) << 8));
    const auto pos_x = static_cast<std::uint16_t>(
        first_add->payload[49] | (static_cast<std::uint16_t>(first_add->payload[50]) << 8));
    const auto pos_z = static_cast<std::uint16_t>(
        first_add->payload[51] | (static_cast<std::uint16_t>(first_add->payload[52]) << 8));
    EXPECT_EQ(state.monsters().front().monster_kind, kind);
    EXPECT_EQ(state.monsters().front().position_x, pos_x);
    EXPECT_EQ(state.monsters().front().position_z, pos_z);

    mxh::gx::EntityScene scene;
    mxh::gx::WorldSnapshot snapshot;
    snapshot.entities.push_back(mxh::gx::SceneEntity{
        state.monsters().front().object_id,
        state.monsters().front().monster_kind,
        static_cast<float>(state.monsters().front().position_x),
        0.0f,
        static_cast<float>(state.monsters().front().position_z),
        mxh::gx::SceneEntityType::Monster});
    scene.synchronize(snapshot);
    EXPECT_GE(scene.placeholderCount() + scene.loadedModelCount(), 1u);
    if (!scene.placeholders().empty()) {
        EXPECT_GT(scene.placeholders().front().radius, 0.0f);
    }
}

TEST(AgentHandlerTest, ForwardFromMapMonsterAddReachesRegisteredClient) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::AgentHandler handler(db, make_reply_spy(reply));
    MockTcpSender map_link;
    handler.set_map_server_for_map(10u, &map_link, mxh::net::make_connection_id(42));
    handler.register_session(
        mxh::net::make_connection_id(7),
        /*user_id=*/3, /*char_id=*/100003, /*map_num=*/10);

    mxh::net::Message msg;
    msg.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    msg.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::MonsterAdd);
    msg.header.object_id = 100003u;
    msg.payload.assign(64, 0);
    const std::uint32_t monster_id = 50000u;
    const std::uint16_t map_num = 10u;
    std::memcpy(msg.payload.data(), &monster_id, 4u);
    std::memcpy(msg.payload.data() + 4u, &monster_id, 4u);
    std::memcpy(msg.payload.data() + 47u, &map_num, 2u);
    msg.payload[43] = 105;
    handler.forward_from_map(mxh::net::make_connection_id(42), msg, 10u, &map_link);

    ASSERT_EQ(reply.call_count.load(), 1);
    EXPECT_EQ(reply.last_id.value, 7u);
    EXPECT_EQ(reply.last_message.header.protocol,
              static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::MonsterAdd));
    EXPECT_EQ(reply.last_message.header.object_id, 50000u);
}

TEST(MonsterMapRoute, SharedAgentGetsOneMovePerCharacterAndNoOtherMapLeak) {
    MockDbAdapter db;
    MockTcpSender map_link;
    const auto map_connection = mxh::net::make_connection_id(77);
    std::vector<std::pair<std::uint64_t, mxh::net::Message>> delivered;
    mxh::server::AgentHandler agent(db,
        [&](mxh::net::ConnectionId id, const mxh::net::Message& msg) {
            delivered.emplace_back(id.value, msg);
        });
    agent.set_map_server_for_map(7u, &map_link, map_connection);
    agent.register_session(mxh::net::make_connection_id(1001), 1u, 101u, 7u);
    agent.register_session(mxh::net::make_connection_id(1002), 2u, 202u, 7u);
    agent.register_session(mxh::net::make_connection_id(1003), 3u, 303u, 8u);
    std::vector<mxh::net::Message> map_messages;
    mxh::server::MapHandler map(db, 7u,
        [&](mxh::net::ConnectionId id, const mxh::net::Message& msg) {
            map_messages.push_back(msg);
            agent.forward_from_map(id, msg, 7u, &map_link);
        });
    map.set_allow_dev_monster_fallback(false);
    for (const auto player : {101u, 202u}) {
        mxh::net::Message game_in;
        game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
        game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
        game_in.header.object_id = player;
        map.on_message(map_connection, game_in);
    }
    map_messages.clear();
    delivered.clear();
    mxh::game::MonsterInstance monster;
    monster.object_id = 50143u;
    monster.monster_kind = 73u;
    monster.map_num = 7u;
    monster.max_life = monster.current_life = 100u;
    monster.pos_x = monster.pos_z = 100.0f;
    monster.spawn_x = monster.spawn_z = 0.0f;
    monster.walk_speed = 100u;
    monster.search_period_ms = 1u;
    monster.ai_state = mxh::game::MonsterAIState::Return;
    ASSERT_TRUE(map.add_monster_instance(monster));
    map.set_monster_ai_clock_for_test([] { return 1000u; });
    map.tick_monster_ai();
    const auto is_move = [](const auto& msg) {
        return msg.header.category == static_cast<std::uint8_t>(mxh::proto::Category::Move) &&
            msg.header.protocol == static_cast<std::uint8_t>(mxh::proto::MoveProtocol::MonsterMoveNotify) &&
            msg.header.object_id == 50143u;
    };
    EXPECT_EQ(std::count_if(map_messages.begin(), map_messages.end(), is_move), 1u);
    std::vector<std::uint64_t> targets;
    for (const auto& [connection, msg] : delivered) if (is_move(msg)) targets.push_back(connection);
    std::sort(targets.begin(), targets.end());
    EXPECT_EQ(targets, (std::vector<std::uint64_t>{1001u, 1002u}));

    agent.register_session(mxh::net::make_connection_id(1004), 4u, 404u, 7u);
    map_messages.clear();
    delivered.clear();
    mxh::net::Message third_game_in;
    third_game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    third_game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    third_game_in.header.object_id = 404u;
    map.on_message(map_connection, third_game_in);
    const auto is_add = [](const auto& msg) {
        return msg.header.category == static_cast<std::uint8_t>(mxh::proto::Category::UserConn) &&
            msg.header.protocol == static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::MonsterAdd);
    };
    EXPECT_EQ(std::count_if(map_messages.begin(), map_messages.end(), is_add), 1u);
    targets.clear();
    for (const auto& [connection, msg] : delivered) if (is_add(msg)) targets.push_back(connection);
    EXPECT_EQ(targets, (std::vector<std::uint64_t>{1004u}));

    map_messages.clear();
    delivered.clear();
    (void)map.apply_monster_damage(101u, 50143u, 1u, 0u);
    const auto is_life = [](const auto& msg) {
        return msg.header.category == static_cast<std::uint8_t>(mxh::proto::Category::Monster) &&
            msg.header.protocol == static_cast<std::uint8_t>(mxh::proto::MonsterProtocol::LifeNotify) &&
            msg.header.object_id == 50143u;
    };
    EXPECT_EQ(std::count_if(map_messages.begin(), map_messages.end(), is_life), 1u);
    targets.clear();
    for (const auto& [connection, msg] : delivered) if (is_life(msg)) targets.push_back(connection);
    std::sort(targets.begin(), targets.end());
    EXPECT_EQ(targets, (std::vector<std::uint64_t>{1001u, 1002u, 1004u}));
}

TEST(MonsterMapRoute, SnapshotAndRemoveUseTargetWithinSharedMapConnection) {
    MockDbAdapter db;
    MockTcpSender map_link, old_link;
    const auto connection = mxh::net::make_connection_id(77);
    std::vector<std::pair<std::uint64_t, mxh::net::Message>> delivered;
    mxh::server::AgentHandler agent(db,
        [&](mxh::net::ConnectionId id, const mxh::net::Message& msg) {
            delivered.emplace_back(id.value, msg);
        });
    agent.set_map_server_for_map(7u, &map_link, connection);
    agent.register_session(mxh::net::make_connection_id(1001), 1u, 101u, 7u);
    agent.register_session(mxh::net::make_connection_id(1002), 2u, 202u, 7u);
    agent.register_session(mxh::net::make_connection_id(1003), 3u, 303u, 8u);
    mxh::net::Message add;
    add.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    add.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::MonsterAdd);
    add.header.object_id = 202u;
    add.payload.resize(64u);
    const std::uint32_t monster = 50143u;
    const std::uint16_t map_num = 7u;
    std::memcpy(add.payload.data(), &monster, 4u);
    std::memcpy(add.payload.data() + 4u, &monster, 4u);
    std::memcpy(add.payload.data() + 47u, &map_num, 2u);
    agent.forward_from_map(connection, add, 7u, &map_link);
    ASSERT_EQ(delivered.size(), 1u);
    EXPECT_EQ(delivered[0].first, 1002u);
    EXPECT_EQ(delivered[0].second.header.object_id, monster);
    EXPECT_EQ(delivered[0].second.payload, add.payload);
    delivered.clear();
    mxh::net::Message npc = add;
    npc.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::NpcAdd);
    npc.header.object_id = 101u;
    const std::uint32_t npc_id = 4100u;
    std::memcpy(npc.payload.data(), &npc_id, 4u);
    std::memcpy(npc.payload.data() + 4u, &npc_id, 4u);
    std::memcpy(npc.payload.data() + 39u, &map_num, 2u);
    agent.forward_from_map(connection, npc, 7u, &map_link);
    ASSERT_EQ(delivered.size(), 1u);
    EXPECT_EQ(delivered[0].first, 1001u);
    EXPECT_EQ(delivered[0].second.header.object_id, npc_id);
    delivered.clear();
    auto invalid = add;
    invalid.payload[47] = 8u;
    agent.forward_from_map(connection, invalid, 7u, &map_link);
    invalid = add;
    invalid.header.object_id = 303u;
    agent.forward_from_map(connection, invalid, 7u, &map_link);
    agent.forward_from_map(mxh::net::make_connection_id(88), add, 7u, &old_link);
    EXPECT_TRUE(delivered.empty());

    mxh::net::Message remove;
    remove.header.category = add.header.category;
    remove.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::ObjectRemove);
    remove.header.object_id = 101u;
    remove.payload.resize(4u);
    std::memcpy(remove.payload.data(), &monster, 4u);
    agent.forward_from_map(connection, remove, 7u, &map_link);
    ASSERT_EQ(delivered.size(), 1u);
    EXPECT_EQ(delivered[0].first, 1001u);
    EXPECT_EQ(delivered[0].second.header.object_id, 0u);
    EXPECT_EQ(delivered[0].second.payload, remove.payload);
}

TEST(MonsterMapRoute, DeathRequiresCurrentMapVictimAndPreservesOriginalWire) {
    MockDbAdapter db;
    MockTcpSender current_link, replacement_link;
    const auto current = mxh::net::make_connection_id(77);
    const auto replacement = mxh::net::make_connection_id(88);
    std::vector<std::pair<std::uint64_t, mxh::net::Message>> delivered;
    mxh::server::AgentHandler agent(db,
        [&](mxh::net::ConnectionId id, const mxh::net::Message& msg) {
            delivered.emplace_back(id.value, msg);
        });
    agent.set_map_server_for_map(7u, &current_link, current);
    agent.register_session(mxh::net::make_connection_id(1001), 1u, 101u, 7u);
    agent.register_session(mxh::net::make_connection_id(1002), 2u, 202u, 7u);
    agent.register_session(mxh::net::make_connection_id(1003), 3u, 303u, 8u);
    mxh::net::Message death;
    death.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    death.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::CharacterDie);
    death.header.object_id = 50143u;
    death.payload.resize(8u);
    const std::uint32_t killer = 50143u, victim = 101u;
    std::memcpy(death.payload.data(), &killer, 4u);
    std::memcpy(death.payload.data() + 4u, &victim, 4u);
    agent.forward_from_map(current, death, 7u, &current_link);
    ASSERT_EQ(delivered.size(), 2u);
    EXPECT_EQ(delivered[0].second.header.object_id, killer);
    EXPECT_EQ(delivered[0].second.payload, death.payload);
    std::vector<std::uint64_t> targets{delivered[0].first, delivered[1].first};
    std::sort(targets.begin(), targets.end());
    EXPECT_EQ(targets, (std::vector<std::uint64_t>{1001u, 1002u}));
    delivered.clear();
    auto wrong_victim = death;
    const std::uint32_t cross_map = 303u;
    std::memcpy(wrong_victim.payload.data() + 4u, &cross_map, 4u);
    agent.forward_from_map(current, wrong_victim, 7u, &current_link);
    auto wrong_killer = death;
    wrong_killer.header.object_id++;
    agent.forward_from_map(current, wrong_killer, 7u, &current_link);
    agent.forward_from_map(replacement, death, 7u, &replacement_link);
    agent.set_map_server_for_map(7u, &replacement_link, replacement);
    agent.forward_from_map(current, death, 7u, &current_link);
    EXPECT_TRUE(delivered.empty());

    // A separate Agent attached to the same Map sees observers but does not
    // own the victim's session. The authoritative Map still sends its death
    // packet once to that Agent's Map connection.
    MockTcpSender second_agent_map;
    std::vector<std::pair<std::uint64_t, mxh::net::Message>> second_delivered;
    mxh::server::AgentHandler second_agent(db,
        [&](mxh::net::ConnectionId id, const mxh::net::Message& msg) {
            second_delivered.emplace_back(id.value, msg);
        });
    const auto second_link_id = mxh::net::make_connection_id(99);
    second_agent.set_map_server_for_map(7u, &second_agent_map, second_link_id);
    second_agent.register_session(mxh::net::make_connection_id(2005), 5u, 505u, 7u);
    second_agent.forward_from_map(second_link_id, death, 7u, &second_agent_map);
    ASSERT_EQ(second_delivered.size(), 1u);
    EXPECT_EQ(second_delivered[0].first, 2005u);
    EXPECT_EQ(second_delivered[0].second.header.object_id, killer);
    EXPECT_EQ(second_delivered[0].second.payload, death.payload);
}

TEST(MapHandlerTest, GameInOnConnectionZeroSendsMonsterAdds) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    mxh::server::AiGroupList groups;
    mxh::server::AiGroupDefinition group;
    group.group_id = 1u;
    mxh::server::AiSpawnDefinition spawn;
    spawn.monster_kind = 105u;
    spawn.pos_x = 100.0f;
    spawn.pos_z = 200.0f;
    group.spawns = {spawn, spawn};
    groups.groups.push_back(group);
    ASSERT_EQ(handler.install_ai_groups(groups), 2u);

    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(mxh::net::make_connection_id(0), game_in);

    std::size_t monster_adds = 0;
    for (const auto& message : reply.messages) {
        if (message.header.category ==
                static_cast<std::uint8_t>(mxh::proto::Category::UserConn) &&
            message.header.protocol ==
                static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::MonsterAdd)) {
            ++monster_adds;
        }
    }
    EXPECT_EQ(monster_adds, 2u);
}

TEST(MapHandlerTest, ProductionModeKeepsValidEmptyRegenEmpty) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 12, make_reply_spy(reply));
    handler.set_allow_dev_monster_fallback(false);

    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(mxh::net::make_connection_id(55), game_in);

    EXPECT_EQ(handler.monster_count_for_test(), 0u);
}

TEST(MapHandlerTest, ProductionModeDisablesHardcodedSkillFallback) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 12, make_reply_spy(reply));
    ASSERT_NE(handler.find_skill(1u), nullptr);

    handler.set_allow_dev_skill_fallback(false);

    EXPECT_EQ(handler.find_skill(1u), nullptr);
}

TEST(MapHandlerTest, OnDisconnectDoesNotCrash) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, /*map_num=*/7, make_reply_spy(reply));
    handler.on_disconnect({}, mxh::net::NetError::Disconnected);
    EXPECT_EQ(reply.call_count.load(), 0);
}

namespace {
// Synthesize a minimal valid ItemList.bin (1:1 with legacy MHFile packed-text format)
// containing a single row.  Used by MapHandler R-8 tests below.
std::vector<std::uint8_t> synthesize_itemlist_bin(const std::string& single_row_text) {
    constexpr std::uint8_t type_byte = 1u;
    std::vector<std::uint8_t> decoded;
    decoded.reserve(single_row_text.size() + 2);
    for (char c : single_row_text) decoded.push_back(static_cast<std::uint8_t>(c));
    decoded.push_back(13);  // CR
    decoded.push_back(10);  // LF
    std::vector<std::uint8_t> encoded;
    encoded.reserve(decoded.size());
    for (std::size_t i = 0; i < decoded.size(); ++i) {
        const int adjusted = static_cast<int>(decoded[i])
            + static_cast<int>(i & 0xFFu)
            + static_cast<int>(type_byte);
        encoded.push_back(static_cast<std::uint8_t>(adjusted & 0xFFu));
    }
    std::uint8_t crc1 = type_byte;
    for (std::uint8_t b : encoded) crc1 = static_cast<std::uint8_t>(crc1 + b);
    const std::uint32_t file_size = static_cast<std::uint32_t>(encoded.size());
    std::vector<std::uint8_t> out;
    out.reserve(12 + 2 + encoded.size());
    auto append_u32 = [&out](std::uint32_t v) {
        for (int i = 0; i < 4; ++i) out.push_back(static_cast<std::uint8_t>((v >> (i * 8)) & 0xFFu));
    };
    append_u32(1u);  // dwVersion
    append_u32(1u);  // dwType
    append_u32(file_size);
    out.push_back(crc1);
    out.insert(out.end(), encoded.begin(), encoded.end());
    out.push_back(0u);  // crc2 - unused by load_item_list
    return out;
}
}  // anonymous namespace

TEST(MapHandlerTest, ItemManagerEmptyByDefault) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    EXPECT_EQ(handler.item_manager_for_test().size(), 0u);
    EXPECT_FALSE(handler.item_manager_for_test().exists(1u));
}

TEST(MapHandlerTest, LoadItemListPopulatesManagerFromBin) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    ASSERT_EQ(handler.item_manager_for_test().size(), 0u);
    const auto row_text = build_test_row_56(1u, 999u);
    const auto bin = synthesize_itemlist_bin(row_text);
    const auto path = write_temp_bin(bin);
    handler.load_item_list(path.string());
    std::error_code ec;
    std::filesystem::remove(path, ec);
    EXPECT_EQ(handler.item_manager_for_test().size(), 1u);
    EXPECT_TRUE(handler.item_manager_for_test().exists(1u));
    mxh::game::ItemInfo info{};
    ASSERT_TRUE(handler.item_manager_for_test().try_get(1u, info));
    EXPECT_EQ(info.ItemIdx, 1u);
    EXPECT_EQ(info.LifeRecover, 999u);
    EXPECT_FLOAT_EQ(info.LifeRecoverRate, 0.0f);
}

TEST(MapHandlerTest, UseAckReadsHpDeltaFromLoadedItemListBin) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    // Step 1: load a 1-row ItemList.bin with ItemIdx=1, LifeRecover=999.
    const auto row_text = build_test_row_56(1u, 999u);
    const auto bin = synthesize_itemlist_bin(row_text);
    const auto path = write_temp_bin(bin);
    handler.load_item_list(path.string());
    std::error_code ec;
    std::filesystem::remove(path, ec);
    // Step 2: enter map, set HP=1 MP=1, add inventory item with wIconIdx=1.
    const auto connection = mxh::net::make_connection_id(55);
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);
    ASSERT_TRUE(handler.set_player_vitals_for_test(123u, 1u, 1u));
    ASSERT_TRUE(handler.add_player_item_for_test(
        123u, mxh::game::make_item(9002u, 1u, 0u)));
    // Step 3: fire UseSyn, capture UseAck reply.
    mxh::net::Message use;
    use.header.object_id = 123u;
    use.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    use.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::UseSyn);
    use.payload.resize(2);
    const std::uint16_t pos = 0u;
    std::memcpy(use.payload.data(), &pos, sizeof(pos));
    handler.on_message(connection, use);
    // UseAck is the last Item-category reply; GameInAck was sent earlier.
    ASSERT_GE(reply.messages.size(), 1u);
    const mxh::net::Message* ack = nullptr;
    for (const auto& m : reply.messages) {
        if (m.header.category == static_cast<std::uint8_t>(mxh::proto::Category::Item)
            && m.header.protocol == static_cast<std::uint8_t>(mxh::proto::ItemProtocol::UseAck)) {
            ack = &m;
            break;
        }
    }
    ASSERT_NE(ack, nullptr);
    ASSERT_EQ(ack->payload.size(), 20u);
    // Layout (LE): u16 pos | u16 wIconIdx | i32 hp_delta | i32 mp_delta
    //              | u32 new_hp | u32 new_mp
    std::int32_t hp_delta = 0;
    std::memcpy(&hp_delta, ack->payload.data() + 4, 4);
    EXPECT_EQ(hp_delta, 999);
}

TEST(MapHandlerTest, UseSynConsumesItemAndAdvancesUseItemQuest) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    const auto connection = mxh::net::make_connection_id(55);
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);

    const auto item_path = write_temp_bin(synthesize_itemlist_bin(build_test_row_56(1u, 999u)));
    handler.load_item_list(item_path.string());
    std::error_code ec;
    std::filesystem::remove(item_path, ec);

    const auto quest_path = write_temp_bin(synthesize_dealitem_bin(
        "$QUEST 88 { $SUBQUEST 0 { #TRIGGER @USEITEM 1 1 *ENDQUEST 0 } }"));
    handler.load_quest_script(quest_path.string());
    std::filesystem::remove(quest_path, ec);

    mxh::net::Message start;
    start.header.object_id = 123u;
    start.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Quest);
    start.header.protocol = static_cast<std::uint8_t>(mxh::proto::QuestProtocol::StartSyn);
    start.payload.resize(2);
    const std::uint16_t quest_id = 88u;
    std::memcpy(start.payload.data(), &quest_id, sizeof(quest_id));
    handler.on_message(connection, start);
    ASSERT_EQ(handler.player_quest_count_for_test(123u), 1u);
    ASSERT_TRUE(handler.add_player_item_for_test(123u, mxh::game::make_item(9002u, 1u, 0u)));

    mxh::net::Message use;
    use.header.object_id = 123u;
    use.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    use.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::UseSyn);
    use.payload.resize(2);
    const std::uint16_t slot = 0u;
    std::memcpy(use.payload.data(), &slot, sizeof(slot));
    handler.on_message(connection, use);

    const auto progress = handler.quest_progress_for_test(123u, quest_id);
    ASSERT_TRUE(progress.has_value());
    ASSERT_EQ(progress->subs.size(), 1u);
    EXPECT_EQ(progress->subs[0].count, 1u);
    EXPECT_EQ(progress->state, mxh::server::QuestState::Complete);
    EXPECT_EQ(handler.player_runtime_snapshot(123u)->inventory_count, 0u);
}


// D1.3 call-site: synthesize a valid SkillList.bin row (1:1 with the 150-token
// legacy SKILLINFO layout).  Returns a tab-separated string.
std::string build_test_skill_row_150(std::uint16_t skill_idx,
                                std::uint16_t weapon_kind,
                                std::uint16_t first_phy) {
    std::vector<std::string> toks(150u, "0");
    toks[0] = std::to_string(skill_idx);
    toks[1] = "TestSkill";
    toks[7] = std::to_string(weapon_kind);
    toks[8] = "230";
    toks[16] = "0.0";
    toks[20] = "1";
    toks[25] = "1";
    toks[29] = "1000";
    toks[69] = "12";
    toks[70] = std::to_string(first_phy);
    for (std::size_t i = 71; i < 82; ++i) toks[i] = "0";
    for (std::size_t seg = 1; seg < 6; ++seg) {
        toks[69 + seg * 13] = "0";
    }
    toks[149] = "10001";
    std::ostringstream row;
    for (std::size_t i = 0; i < toks.size(); ++i) {
        if (i != 0) row << "	";
        row << toks[i];
    }
    return row.str();
}

TEST(MapHandlerTest, SkillManagerEmptyByDefault) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    EXPECT_EQ(handler.skill_manager_for_test().size(), 0u);
    EXPECT_FALSE(handler.skill_manager_for_test().exists(1u));
}

TEST(MapHandlerTest, LoadSkillListPopulatesManagerFromBin) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    ASSERT_EQ(handler.skill_manager_for_test().size(), 0u);
    const auto row_text = build_test_skill_row_150(1u, 2u, 777u);
    const auto bin = synthesize_itemlist_bin(row_text);
    const auto path = write_temp_bin(bin);
    handler.load_skill_list(path.string());
    std::error_code ec;
    std::filesystem::remove(path, ec);
    ASSERT_EQ(handler.skill_manager_for_test().size(), 1u);
    ASSERT_TRUE(handler.skill_manager_for_test().exists(1u));
    mxh::game::SkillInfo info{};
    ASSERT_TRUE(handler.skill_manager_for_test().try_get(1u, info));
    EXPECT_EQ(info.SkillIdx, 1u);
    EXPECT_EQ(info.WeaponKind, 2u);
    EXPECT_FLOAT_EQ(info.UpPhyAttack[0], 777.0f);
    const mxh::game::SkillInfo* found = handler.find_skill(1u);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->SkillIdx, 1u);
    EXPECT_EQ(found->WeaponKind, 2u);
}

TEST(MapHandlerTest, FindSkillReadsFromLoadedSkillListBin) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    const auto row_text = build_test_skill_row_150(42u, 5u, 1234u);
    const auto bin = synthesize_itemlist_bin(row_text);
    const auto path = write_temp_bin(bin);
    handler.load_skill_list(path.string());
    std::error_code ec;
    std::filesystem::remove(path, ec);
    const mxh::game::SkillInfo* s42 = handler.find_skill(42u);
    ASSERT_NE(s42, nullptr);
    EXPECT_EQ(s42->SkillIdx, 42u);
    EXPECT_EQ(s42->WeaponKind, 5u);
    EXPECT_FLOAT_EQ(s42->UpPhyAttack[0], 1234.0f);
    const mxh::game::SkillInfo* s1 = handler.find_skill(1u);
    EXPECT_EQ(s1, nullptr);
}

namespace {
std::vector<std::uint8_t> synthesize_quest_text_bin(const std::string& text) {
    return synthesize_dealitem_bin(text);
}
}  // namespace

TEST(MapHandlerTest, LoadDealitemPopulatesCatalogFromBin) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    ASSERT_EQ(handler.dealitem_catalog_for_test().npcs.size(), 0u);
    const std::string text =
        "1 map 2 npc 7 10 20 0 1 tab 100 3 tab 101 -1\n"
        "1 map 2 npc 7 10 20 0 2 tab 200 0\n";
    const auto bin = synthesize_dealitem_bin(text);
    const auto path = write_temp_bin(bin);
    handler.load_dealitem(path.string());
    std::error_code ec;
    std::filesystem::remove(path, ec);
    EXPECT_EQ(handler.dealitem_catalog_for_test().npcs.size(), 1u);
    const auto* npc = handler.dealitem_catalog_for_test().find_npc(7);
    ASSERT_NE(npc, nullptr);
    ASSERT_EQ(npc->tabs.size(), 2u);
    EXPECT_EQ(npc->tabs[0].size(), 2u);
    EXPECT_EQ(npc->tabs[0][0].item_idx, 100u);
    EXPECT_EQ(npc->tabs[0][1].item_count, std::numeric_limits<std::uint32_t>::max());
    EXPECT_EQ(npc->tabs[1].size(), 1u);
    EXPECT_EQ(npc->tabs[1][0].item_idx, 200u);
}

TEST(MapHandlerTest, LoadQuestScriptPopulatesDefinitionsFromBin) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    ASSERT_EQ(handler.quest_definitions_for_test().quests.size(), 0u);
    const std::string text =
        "$QUEST 1 { $SUBQUEST 1 { #TRIGGER @HUNT 1 10 *ADDCOUNT 1 1 } }";
    const auto raw = synthesize_dealitem_bin(text);
    const auto path = write_temp_bin(raw);
    handler.load_quest_script(path.string());
    std::error_code ec;
    std::filesystem::remove(path, ec);
    ASSERT_EQ(handler.quest_definitions_for_test().quests.size(), 1u);
    const auto* def = handler.quest_definitions_for_test().find_quest(1u);
    ASSERT_NE(def, nullptr);
    ASSERT_EQ(def->subquests.size(), 1u);
    EXPECT_EQ(def->subquests[0].triggers.size(), 1u);
}
TEST(MapHandlerTest, BuySynOkArmDeductsMoneyAndInsertsInventory) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    const auto connection = mxh::net::make_connection_id(55);
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);
    ASSERT_TRUE(handler.set_player_money_for_test(123u, 1000u));
    // load a one-NPC dealitem catalog that sells item 555 for 10g each.
    const std::string deal_text = "1 map 2 npc 7 10 20 0 1 tab 555 10\n";
    const auto deal_bin = synthesize_dealitem_bin(deal_text);
    const auto deal_path = write_temp_bin(deal_bin);
    handler.load_dealitem(deal_path.string());
    std::error_code ec_deal; std::filesystem::remove(deal_path, ec_deal);
    mxh::net::Message buy;
    buy.header.object_id = 123u;
    buy.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    buy.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::BuySyn);
    buy.payload.resize(4);
    const std::uint16_t item = 555u; const std::uint16_t qty = 3u;
    std::memcpy(buy.payload.data(), &item, sizeof(item));
    std::memcpy(buy.payload.data() + 2, &qty, sizeof(qty));
    handler.on_message(connection, buy);
    EXPECT_EQ(handler.player_money_for_test(123u), 1000u); // price field not in dealitem row; total_price=0 for now
    const auto snap = handler.player_runtime_snapshot(123u);
    ASSERT_TRUE(snap.has_value());
    EXPECT_EQ(snap->inventory_count, 3u); // qty=3 -> 3 stack items
}

TEST(MapHandlerTest, SellSynUsesItemListSellPriceAndPublishesState) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    const auto connection = mxh::net::make_connection_id(55);
    const std::string deal_text = "7 map 2 npc 7 25000 25000 0 1 tab 555 10\n";
    const auto deal_path = write_temp_bin(synthesize_dealitem_bin(deal_text));
    handler.load_dealitem(deal_path.string());
    std::error_code ec_deal; std::filesystem::remove(deal_path, ec_deal);
    std::vector<std::string> tokens(56u, "0");
    tokens[0] = "555"; tokens[1] = "Potion"; tokens[5] = "0"; tokens[6] = "100";
    std::string row;
    for (const auto& token : tokens) { row += token; row.push_back('\t'); }
    row.push_back('\r'); row.push_back('\n');
    const auto item_path = write_temp_bin(synthesize_item_list_bin(row));
    handler.load_item_list(item_path.string());
    std::error_code ec_item; std::filesystem::remove(item_path, ec_item);
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);
    ASSERT_TRUE(handler.set_player_money_for_test(123u, 1000u));
    ASSERT_TRUE(handler.set_player_position_for_test(123u, 25000.0f, 25000.0f));

    mxh::net::Message buy;
    buy.header.object_id = 123u;
    buy.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    buy.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::BuySyn);
    buy.payload.resize(4);
    const std::uint16_t item = 555u, qty = 1u;
    std::memcpy(buy.payload.data(), &item, 2);
    std::memcpy(buy.payload.data() + 2, &qty, 2);
    handler.on_message(connection, buy);
    ASSERT_TRUE(handler.player_runtime_snapshot(123u).has_value());
    ASSERT_EQ(handler.player_runtime_snapshot(123u)->inventory_count, 1u);
    reply.messages.clear();

    mxh::net::Message sell;
    sell.header.object_id = 123u;
    sell.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    sell.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::SellSyn);
    sell.payload.resize(8);
    const std::uint16_t pos = 0u, dealer = 7u;
    std::memcpy(sell.payload.data(), &pos, 2);
    std::memcpy(sell.payload.data() + 2, &item, 2);
    std::memcpy(sell.payload.data() + 4, &qty, 2);
    std::memcpy(sell.payload.data() + 6, &dealer, 2);
    handler.on_message(connection, sell);

    ASSERT_GE(reply.messages.size(), 3u);
    EXPECT_EQ(reply.messages[0].header.protocol,
              static_cast<std::uint8_t>(mxh::proto::ItemProtocol::SellAck));
    EXPECT_EQ(reply.messages[1].header.protocol,
              static_cast<std::uint8_t>(mxh::proto::ItemProtocol::Money));
    EXPECT_EQ(reply.messages[2].header.protocol,
              static_cast<std::uint8_t>(mxh::proto::ItemProtocol::TotalInfoLocal));
    EXPECT_EQ(handler.player_money_for_test(123u), 1100u);
    EXPECT_EQ(handler.player_runtime_snapshot(123u)->inventory_count, 0u);
}

TEST(MapHandlerTest, StartSynOkArmAddsQuestToPlayerLog) {
    MockDbAdapter db;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    const auto connection = mxh::net::make_connection_id(55);
    mxh::net::Message game_in;
    game_in.header.object_id = 456u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);
    const std::string quest_text = "$QUEST 99 { $SUBQUEST 1 { #TRIGGER @HUNT 1 10 *ADDCOUNT 1 1 } }";
    const auto qbin = synthesize_dealitem_bin(quest_text);
    const auto qpath = write_temp_bin(qbin);
    handler.load_quest_script(qpath.string());
    std::error_code ec_q; std::filesystem::remove(qpath, ec_q);
    ASSERT_EQ(handler.quest_definitions_for_test().quests.size(), 1u);
    EXPECT_EQ(handler.player_quest_count_for_test(456u), 0u);
    mxh::net::Message start;
    start.header.object_id = 456u;
    start.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Quest);
    start.header.protocol = static_cast<std::uint8_t>(mxh::proto::QuestProtocol::StartSyn);
    start.payload.resize(2);
    const std::uint16_t qid = 99u;
    std::memcpy(start.payload.data(), &qid, sizeof(qid));
    handler.on_message(connection, start);
    EXPECT_EQ(handler.player_quest_count_for_test(456u), 1u);
}


// M3 D-stage: BuySyn money persistence to modern_player_state.
// End-to-end: real SqliteAdapter (in-memory) + inline CREATE TABLE +
// BuySyn Ok arm + SELECT verification.  Validates that the orchestrator
// actually hits the DB (not just a mock) and that the row is readable
// after the wire reply.
TEST(MapHandlerTest, BuySynOkArmPersistsMoneyToSqliteMemory) {
    // 1) Real in-memory SQLite.
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg{};
    cfg.backend = "sqlite";
    cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());

    // 2) Inline CREATE TABLE (same shape as the production schema in
    //    deploy/database/mx_modern_schema_mssql.sql + MoxianDbTool
    //    moxian_schema_sql()).
    auto ct = db.exec_multi(
        "CREATE TABLE modern_player_state ("
        " player_id  INTEGER PRIMARY KEY,"
        " money      INTEGER NOT NULL DEFAULT 0,"
        " level      INTEGER NOT NULL DEFAULT 1,"
        " exp        INTEGER NOT NULL DEFAULT 0,"
        " updated_at TEXT    NOT NULL"
        ");");
    ASSERT_TRUE(ct.ok()) << ct.error_message;
    ASSERT_TRUE(create_fixture_character(db, 123u));
    ASSERT_TRUE(seed_fixture_runtime_state(db, 123u));

    // 3) Build a MapHandler around the live db.  ReplySpy captures
    //    the BuyAck so we can confirm the wire shape did not change.
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    const auto connection = mxh::net::make_connection_id(55);

    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);

    ASSERT_TRUE(handler.set_player_money_for_test(123u, 1000u));

    // 4) Load a one-NPC dealitem catalog and send a BuySyn Ok path.
    const std::string deal_text = "1 map 2 npc 7 10 20 0 1 tab 555 10\n";
    const auto deal_bin = synthesize_dealitem_bin(deal_text);
    const auto deal_path = write_temp_bin(deal_bin);
    handler.load_dealitem(deal_path.string());
    std::error_code ec_deal; std::filesystem::remove(deal_path, ec_deal);

    mxh::net::Message buy;
    buy.header.object_id = 123u;
    buy.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    buy.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::BuySyn);
    buy.payload.resize(4);
    const std::uint16_t item = 555u; const std::uint16_t qty = 3u;
    std::memcpy(buy.payload.data(), &item, sizeof(item));
    std::memcpy(buy.payload.data() + 2, &qty, sizeof(qty));
    handler.on_message(connection, buy);

    // 5) Wire shape: BuyAck followed by an ITEM_TOTALINFO_LOCAL inventory
    //    refresh for the buyer.
    ASSERT_GE(reply.messages.size(), 2u);
    EXPECT_EQ(reply.messages[reply.messages.size() - 2].header.protocol,
              static_cast<std::uint8_t>(mxh::proto::ItemProtocol::BuyAck));
    EXPECT_EQ(reply.last_message.header.protocol,
              static_cast<std::uint8_t>(
                  mxh::proto::ItemProtocol::TotalInfoLocal));
    EXPECT_EQ(reply.last_message.payload.size(), sizeof(mxh::game::ItemTotalInfo));
    bool found_item = false;
    if (reply.last_message.payload.size() == sizeof(mxh::game::ItemTotalInfo)) {
        mxh::game::ItemTotalInfo total{};
        std::memcpy(&total, reply.last_message.payload.data(), sizeof(total));
        for (const auto& slot : total.Inventory) {
            found_item = found_item || slot.wIconIdx == item;
        }
    }
    EXPECT_TRUE(found_item) << "bought item must appear in the refreshed inventory";

    // 6) DB verification: a row for player_id=123 must exist with
    //    money=1000 (dealitem has no price field yet so total_price=0
    //    and the new money equals the old money).
    mxh::db::ResultSet rs;
    std::vector<mxh::db::Bind> qp = { mxh::db::bind(static_cast<std::int64_t>(123)) };
    auto sel = db.query(
        "SELECT money, updated_at FROM modern_player_state WHERE player_id = ?",
        qp, rs);
    ASSERT_TRUE(sel.ok()) << sel.error_message;
    ASSERT_EQ(rs.rows.size(), 1u);
    ASSERT_TRUE(std::holds_alternative<std::int64_t>(rs.rows[0][0]));
    EXPECT_EQ(std::get<std::int64_t>(rs.rows[0][0]), 1000);
}

TEST(MapHandlerTest, ItemPricesFillCatalogAndDeductRealMoney) {
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg{};
    cfg.backend = "sqlite";
    cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());
    ASSERT_TRUE(create_fixture_character(db, 123u));
    ASSERT_TRUE(seed_fixture_runtime_state(db, 123u));
    mxh::db::SqliteAdapter* db_ptr = &db;
    (void)db_ptr;
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    const auto connection = mxh::net::make_connection_id(55);

    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);
    ASSERT_TRUE(handler.set_player_money_for_test(123u, 20000u));

    const std::string deal_text = "1 map 2 npc 7 10 20 0 1 tab 555 10\n";
    const auto deal_bin = synthesize_dealitem_bin(deal_text);
    const auto deal_path = write_temp_bin(deal_bin);
    handler.load_dealitem(deal_path.string());
    std::error_code ec_deal; std::filesystem::remove(deal_path, ec_deal);

    // One ItemList row: ItemIdx=555, BuyPrice=12345 (token 5).
    std::vector<std::string> tokens(56u, "0");
    tokens[0] = "555";
    tokens[1] = "Potion";
    tokens[5] = "12345";
    std::string row;
    for (const auto& t : tokens) { row += t; row.push_back('\t'); }
    row.push_back('\r'); row.push_back('\n');
    const auto item_bin = synthesize_item_list_bin(row);
    const auto item_path = write_temp_bin(item_bin);
    handler.load_item_prices(item_path.string());
    std::error_code ec_item; std::filesystem::remove(item_path, ec_item);

    // Open the shop: the modern ShopList must carry the real price.
    mxh::net::Message talk;
    talk.header.object_id = 123u;
    talk.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Npc);
    talk.header.protocol = static_cast<std::uint8_t>(mxh::proto::NpcProtocol::SpeechSyn);
    talk.payload.resize(4);
    const std::uint32_t npc_id = 7u;
    std::memcpy(talk.payload.data(), &npc_id, sizeof(npc_id));
    reply.messages.clear();
    handler.on_message(connection, talk);
    ASSERT_FALSE(reply.messages.empty());
    const auto& shop = reply.last_message;
    EXPECT_EQ(shop.header.protocol, mxh::proto::kModernShopList);
    ASSERT_GE(shop.payload.size(), 6u);
    std::uint32_t price = 0;
    std::memcpy(&price, shop.payload.data() + 8, sizeof(price));  // first entry price
    EXPECT_EQ(price, 12345u);

    // Buy one: money 20000 -> 7655.
    reply.messages.clear();
    mxh::net::Message buy;
    buy.header.object_id = 123u;
    buy.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    buy.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::BuySyn);
    buy.payload.resize(4);
    const std::uint16_t item = 555u;
    const std::uint16_t qty = 1u;
    std::memcpy(buy.payload.data(), &item, sizeof(item));
    std::memcpy(buy.payload.data() + 2, &qty, sizeof(qty));
    handler.on_message(connection, buy);
    EXPECT_EQ(handler.player_money_for_test(123u), 7655u);
    ASSERT_FALSE(reply.messages.empty());
    EXPECT_EQ(reply.messages[reply.messages.size() - 2].header.protocol,
              static_cast<std::uint8_t>(mxh::proto::ItemProtocol::BuyAck));
}

// M3 D-stage: persist_player_money_for_test must hit the DB on
// every call, even when the wire-level BuySyn does not (no player
// connected).  This pins the helper in isolation.
TEST(MapHandlerTest, PersistPlayerMoneyForTestHitsDb) {
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg{};
    cfg.backend = "sqlite";
    cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());
    ASSERT_TRUE(db.exec_multi(
        "CREATE TABLE modern_player_state ("
        " player_id INTEGER PRIMARY KEY, money INTEGER NOT NULL DEFAULT 0,"
        " level INTEGER NOT NULL DEFAULT 1, exp INTEGER NOT NULL DEFAULT 0,"
        " updated_at TEXT NOT NULL);").ok());

    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    // No GameInSyn; no player in state.  The helper must still write
    // the row (DB persistence is decoupled from in-memory state).
    EXPECT_TRUE(handler.persist_player_money_for_test(777u, 4242u));

    mxh::db::ResultSet rs;
    std::vector<mxh::db::Bind> qp = { mxh::db::bind(static_cast<std::int64_t>(777)) };
    ASSERT_TRUE(db.query(
        "SELECT money FROM modern_player_state WHERE player_id = ?", qp, rs).ok());
    ASSERT_EQ(rs.rows.size(), 1u);
    EXPECT_EQ(std::get<std::int64_t>(rs.rows[0][0]), 4242);
}


// M3 D-stage: StartSyn quest_log persistence to modern_player_quest_log.
// End-to-end: real SqliteAdapter (in-memory) + inline CREATE TABLE +
// StartSyn Ok arm + SELECT verification.  Validates that the
// orchestrator actually hits the DB (not just a mock) and that
// the row is readable after the wire reply.
TEST(MapHandlerTest, StartSynOkArmPersistsQuestLogToSqliteMemory) {
    // 1) Real in-memory SQLite.
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg{};
    cfg.backend = "sqlite";
    cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());

    // 2) Inline CREATE TABLE (same shape as the production schema in
    //    deploy/database/mx_modern_schema_mssql.sql + MoxianDbTool
    //    moxian_schema_sql()).
    auto ct = db.exec_multi(
        "CREATE TABLE modern_player_quest_log ("
        " player_id        INTEGER NOT NULL,"
        " quest_id         INTEGER NOT NULL,"
        " state            INTEGER NOT NULL DEFAULT 0,"
        " accepted_time_ms INTEGER NOT NULL DEFAULT 0,"
        " updated_at       TEXT    NOT NULL,"
        " PRIMARY KEY (player_id, quest_id));");
    ASSERT_TRUE(ct.ok()) << ct.error_message;
    ASSERT_TRUE(create_fixture_character(db, 456u));
    ASSERT_TRUE(seed_fixture_runtime_state(db, 456u));

    // 3) Build a MapHandler around the live db.  ReplySpy captures
    //    the StartAck so we can confirm the wire shape did not change.
    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    const auto connection = mxh::net::make_connection_id(55);

    mxh::net::Message game_in;
    game_in.header.object_id = 456u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);

    // 4) Load a quest script definition so StartSyn Ok arm fires.
    const std::string quest_text = "$QUEST 99 { $SUBQUEST 1 { #TRIGGER @HUNT 1 10 *ADDCOUNT 1 1 } }";
    const auto qbin = synthesize_dealitem_bin(quest_text);
    const auto qpath = write_temp_bin(qbin);
    handler.load_quest_script(qpath.string());
    std::error_code ec_q; std::filesystem::remove(qpath, ec_q);
    ASSERT_EQ(handler.quest_definitions_for_test().quests.size(), 1u);

    // 5) Send StartSyn for quest 99.
    mxh::net::Message start;
    start.header.object_id = 456u;
    start.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Quest);
    start.header.protocol = static_cast<std::uint8_t>(mxh::proto::QuestProtocol::StartSyn);
    start.payload.resize(2);
    const std::uint16_t qid = 99u;
    std::memcpy(start.payload.data(), &qid, sizeof(qid));
    handler.on_message(connection, start);

    // 6) Wire shape: a StartAck must have been sent.
    ASSERT_FALSE(reply.messages.empty());
    EXPECT_EQ(reply.last_message.header.protocol,
              static_cast<std::uint8_t>(mxh::proto::QuestProtocol::StartAck));

    // 7) DB verification: a row for (player_id=456, quest_id=99)
    //    must exist with state != 0 (Accepted/Active).
    mxh::db::ResultSet rs;
    std::vector<mxh::db::Bind> qp = { mxh::db::bind(static_cast<std::int64_t>(456)) };
    auto sel = db.query(
        "SELECT quest_id, state FROM modern_player_quest_log WHERE player_id = ?",
        qp, rs);
    ASSERT_TRUE(sel.ok()) << sel.error_message;
    ASSERT_EQ(rs.rows.size(), 1u);
    EXPECT_EQ(std::get<std::int64_t>(rs.rows[0][0]), 99);
    const auto state_value = std::get<std::int64_t>(rs.rows[0][1]);
    EXPECT_NE(state_value, 0) << "expected state != 0 (None) after StartSyn Ok";
}

TEST(MapHandlerTest, GameInRestoresPersistedQuestSubProgress) {
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg{};
    cfg.backend = "sqlite";
    cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());
    ASSERT_TRUE(db.exec_multi(
        "CREATE TABLE modern_player_quest_log (player_id INTEGER NOT NULL,quest_id INTEGER NOT NULL,"
        "state INTEGER NOT NULL,accepted_time_ms INTEGER NOT NULL,updated_at TEXT NOT NULL,"
        "PRIMARY KEY(player_id,quest_id));"
        "CREATE TABLE modern_player_quest_sub (player_id INTEGER NOT NULL,quest_id INTEGER NOT NULL,"
        "sub_index INTEGER NOT NULL,kind INTEGER NOT NULL,target_id INTEGER NOT NULL,count INTEGER NOT NULL,"
        "target_count INTEGER NOT NULL,PRIMARY KEY(player_id,quest_id,sub_index));"
        "INSERT INTO modern_player_quest_log VALUES(456,99,1,1234,'now');"
        "INSERT INTO modern_player_quest_sub VALUES(456,99,0,1,10,4,10);").ok());
    ASSERT_TRUE(create_fixture_character(db, 456u));
    ASSERT_TRUE(seed_fixture_runtime_state(db, 456u));

    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    const std::string quest_text = "$QUEST 99 { $SUBQUEST 1 { #TRIGGER @HUNT 10 10 *ADDCOUNT 1 1 } }";
    const auto qpath = write_temp_bin(synthesize_dealitem_bin(quest_text));
    handler.load_quest_script(qpath.string());
    std::error_code ec; std::filesystem::remove(qpath, ec);

    mxh::net::Message game_in;
    game_in.header.object_id = 456u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(mxh::net::make_connection_id(55), game_in);

    const auto quest_snapshot = std::find_if(reply.messages.begin(), reply.messages.end(),
        [](const mxh::net::Message& message) {
            return message.header.category == static_cast<std::uint8_t>(mxh::proto::Category::Quest) &&
                   message.header.protocol == static_cast<std::uint8_t>(mxh::proto::QuestProtocol::TotalInfo) &&
                   message.payload.size() >= 8u;
        });
    ASSERT_NE(quest_snapshot, reply.messages.end());
    std::uint32_t snapshot_state = 0;
    std::memcpy(&snapshot_state, quest_snapshot->payload.data() + 4, 4);
    EXPECT_EQ(snapshot_state, static_cast<std::uint32_t>(mxh::server::QuestState::Accepted));

    const auto progress = handler.quest_progress_for_test(456u, 99u);
    ASSERT_TRUE(progress.has_value());
    ASSERT_EQ(progress->subs.size(), 1u);
    EXPECT_EQ(progress->accepted_time_ms, 1234u);
    EXPECT_EQ(progress->state, mxh::server::QuestState::Accepted);
    EXPECT_EQ(progress->subs[0].count, 4u);
    EXPECT_EQ(progress->subs[0].target, 10u);
}

TEST(MapHandlerTest, CompletedQuestRewardPersistsAndCannotBeClaimedTwice) {
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg{}; cfg.backend = "sqlite"; cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());
    ASSERT_TRUE(db.exec_multi(
        "CREATE TABLE character_info(chrid INTEGER PRIMARY KEY,charname TEXT,sex_type INTEGER,face_type INTEGER,"
        "hair_type INTEGER,height REAL,width REAL,level INTEGER,map_num INTEGER);"
        "CREATE TABLE modern_player_state(player_id INTEGER PRIMARY KEY,money INTEGER NOT NULL,level INTEGER NOT NULL,"
        "exp INTEGER NOT NULL,updated_at TEXT NOT NULL);"
        "CREATE TABLE modern_player_quest_log(player_id INTEGER NOT NULL,quest_id INTEGER NOT NULL,state INTEGER NOT NULL,"
        "accepted_time_ms INTEGER NOT NULL,updated_at TEXT NOT NULL,PRIMARY KEY(player_id,quest_id));"
        "CREATE TABLE modern_player_quest_sub(player_id INTEGER NOT NULL,quest_id INTEGER NOT NULL,sub_index INTEGER NOT NULL,"
        "kind INTEGER NOT NULL,target_id INTEGER NOT NULL,count INTEGER NOT NULL,target_count INTEGER NOT NULL,"
        "PRIMARY KEY(player_id,quest_id,sub_index));"
        "CREATE TABLE modern_player_item(player_id INTEGER NOT NULL,container INTEGER NOT NULL,slot INTEGER NOT NULL,"
        "db_idx INTEGER NOT NULL,item_idx INTEGER NOT NULL,durability INTEGER NOT NULL,rare_idx INTEGER NOT NULL,"
        "quick_position INTEGER NOT NULL,item_param INTEGER NOT NULL,PRIMARY KEY(player_id,container,slot),UNIQUE(player_id,db_idx));"
        "INSERT INTO character_info VALUES(123,'QuestHero',0,1,1,1,1,1,7);"
        "INSERT INTO modern_player_state VALUES(123,100,1,0,'now');").ok());
    ASSERT_TRUE(seed_fixture_base_attributes(db, 123u));
    ASSERT_TRUE(seed_fixture_runtime_state(db, 123u));
    ReplySpy reply; mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    handler.load_experience_curve(std::string(MXH_SOURCE_DIR) + "/data/PlayDH/Resource/CharacterExpPoint.bin");
    const std::string quest_text =
        "$QUEST 99 { $SUBQUEST 1 { #TRIGGER @HUNT 77 1 *GIVEMONEY 50 *TAKEEXP 25 *GIVEITEM 8000 2 *ENDQUEST 1 } }";
    const auto qpath = write_temp_bin(synthesize_dealitem_bin(quest_text));
    handler.load_quest_script(qpath.string()); std::error_code ec; std::filesystem::remove(qpath, ec);
    const auto connection = mxh::net::make_connection_id(55);
    mxh::net::Message in; in.header.object_id=123; in.header.category=static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    in.header.protocol=static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn); handler.on_message(connection,in);
    mxh::net::Message start; start.header.object_id=123; start.header.category=static_cast<std::uint8_t>(mxh::proto::Category::Quest);
    start.header.protocol=static_cast<std::uint8_t>(mxh::proto::QuestProtocol::StartSyn); start.payload.resize(2);
    const std::uint16_t qid=99; std::memcpy(start.payload.data(),&qid,sizeof(qid)); handler.on_message(connection,start);
    mxh::game::MonsterInstance monster; monster.object_id=99001; monster.monster_kind=77; monster.max_life=1; monster.current_life=1;
    ASSERT_TRUE(handler.add_monster_instance(monster)); (void)handler.apply_monster_damage(123,monster.object_id,1,99);
    mxh::net::Message end=start; end.header.protocol=static_cast<std::uint8_t>(mxh::proto::QuestProtocol::EndSyn);
    reply.messages.clear(); handler.on_message(connection,end);
    ASSERT_FALSE(reply.messages.empty());
    EXPECT_EQ(reply.last_message.header.protocol,static_cast<std::uint8_t>(mxh::proto::QuestProtocol::EndAck));
    mxh::db::ResultSet state; const std::vector<mxh::db::Bind> none;
    ASSERT_TRUE(db.query("SELECT money,level,exp FROM modern_player_state WHERE player_id=123",none,state).ok());
    ASSERT_EQ(state.rows.size(),1u);
    EXPECT_EQ(std::get<std::int64_t>(state.rows[0][0]),150);
    // Legacy CPlayer::SetPlayerExpPoint subtracts the level threshold on
    // level-up.  Monster (10) + quest (25) crosses level-1's 20-point
    // threshold, so the persisted current-level experience is 15 at level 2.
    EXPECT_EQ(std::get<std::int64_t>(state.rows[0][1]),2);
    EXPECT_EQ(std::get<std::int64_t>(state.rows[0][2]),15);
    mxh::db::ResultSet items; ASSERT_TRUE(db.query("SELECT item_idx,item_param FROM modern_player_item WHERE player_id=123",none,items).ok());
    ASSERT_EQ(items.rows.size(),1u); EXPECT_EQ(std::get<std::int64_t>(items.rows[0][0]),8000); EXPECT_EQ(std::get<std::int64_t>(items.rows[0][1]),2);
    mxh::db::ResultSet quest; ASSERT_TRUE(db.query("SELECT state FROM modern_player_quest_log WHERE player_id=123 AND quest_id=99",none,quest).ok());
    ASSERT_EQ(quest.rows.size(),1u); EXPECT_EQ(std::get<std::int64_t>(quest.rows[0][0]),static_cast<std::int64_t>(mxh::server::QuestState::Rewarded));
    reply.messages.clear(); handler.on_message(connection,end); ASSERT_FALSE(reply.messages.empty());
    EXPECT_EQ(reply.last_message.header.protocol,static_cast<std::uint8_t>(mxh::proto::QuestProtocol::EndNack));
}

// M3 D-stage: persist_quest_log_for_test must hit the DB on every
// call.  This pins the helper in isolation (no wire traffic).
TEST(MapHandlerTest, PersistQuestLogForTestHitsDb) {
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg{};
    cfg.backend = "sqlite";
    cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());
    ASSERT_TRUE(db.exec_multi(
        "CREATE TABLE modern_player_quest_log ("
        " player_id        INTEGER NOT NULL,"
        " quest_id         INTEGER NOT NULL,"
        " state            INTEGER NOT NULL DEFAULT 0,"
        " accepted_time_ms INTEGER NOT NULL DEFAULT 0,"
        " updated_at       TEXT    NOT NULL,"
        " PRIMARY KEY (player_id, quest_id));").ok());
    ASSERT_TRUE(db.exec_multi(
        "CREATE TABLE modern_player_quest_sub (player_id INTEGER NOT NULL,quest_id INTEGER NOT NULL,"
        "sub_index INTEGER NOT NULL,kind INTEGER NOT NULL,target_id INTEGER NOT NULL,count INTEGER NOT NULL,"
        "target_count INTEGER NOT NULL,PRIMARY KEY(player_id,quest_id,sub_index));"
        "CREATE TABLE modern_item_grant (grant_id INTEGER PRIMARY KEY,idempotency_key TEXT UNIQUE,"
        "character_id INTEGER,item_id INTEGER,item_count INTEGER,status TEXT,inventory_slot INTEGER,"
        "created_by TEXT,reason TEXT,created_at TEXT,claimed_at TEXT);").ok());
    ASSERT_TRUE(create_fixture_character(db, 888u));
    ASSERT_TRUE(seed_fixture_runtime_state(db, 888u));

    ReplySpy reply;
    mxh::server::MapHandler handler(db, 7, make_reply_spy(reply));
    const auto connection = mxh::net::make_connection_id(55);
    mxh::net::Message game_in;
    game_in.header.object_id = 888u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);

    // Inject one quest into the player's quest_log via StartSyn Ok.
    const std::string quest_text = "$QUEST 7 { $SUBQUEST 1 { #TRIGGER @HUNT 1 10 *ADDCOUNT 1 1 } }";
    const auto qbin = synthesize_dealitem_bin(quest_text);
    const auto qpath = write_temp_bin(qbin);
    handler.load_quest_script(qpath.string());
    std::error_code ec_q; std::filesystem::remove(qpath, ec_q);
    ASSERT_EQ(handler.quest_definitions_for_test().quests.size(), 1u);
    mxh::net::Message start;
    start.header.object_id = 888u;
    start.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Quest);
    start.header.protocol = static_cast<std::uint8_t>(mxh::proto::QuestProtocol::StartSyn);
    start.payload.resize(2);
    const std::uint16_t qid = 7u;
    std::memcpy(start.payload.data(), &qid, sizeof(qid));
    // Revisioned players deliberately reject wire quest mutations until the
    // quest/vitals write is atomic. Seed the in-memory log through the
    // test-only fixture hook, then exercise the same persistence path.
    ASSERT_TRUE(handler.seed_quest_for_test(888u, 7u));
    ASSERT_EQ(handler.persisted_quest_count_for_test(888u), 1u);

    // Clear the row and verify persist_quest_log_for_test rewrites it.
    ASSERT_TRUE(db.exec_multi("DELETE FROM modern_player_quest_log WHERE player_id = 888").ok());
    mxh::db::ResultSet rs0;
    std::vector<mxh::db::Bind> qp0 = { mxh::db::bind(static_cast<std::int64_t>(888)) };
    ASSERT_TRUE(db.query("SELECT COUNT(*) FROM modern_player_quest_log WHERE player_id = ?", qp0, rs0).ok());
    ASSERT_EQ(rs0.rows.size(), 1u);
    EXPECT_EQ(std::get<std::int64_t>(rs0.rows[0][0]), 0);

    EXPECT_TRUE(handler.persist_quest_log_for_test(888u));

    mxh::db::ResultSet rs;
    std::vector<mxh::db::Bind> qp = { mxh::db::bind(static_cast<std::int64_t>(888)) };
    ASSERT_TRUE(db.query(
        "SELECT quest_id FROM modern_player_quest_log WHERE player_id = ?", qp, rs).ok());
    ASSERT_EQ(rs.rows.size(), 1u);
    EXPECT_EQ(std::get<std::int64_t>(rs.rows[0][0]), 7);
}

// GameOut is the authoritative session boundary: live money must be flushed
// before the player runtime is removed, so a subsequent GameIn can observe it.
TEST(MapHandlerTest, GameOutSynPersistsLiveMoneyBeforeRuntimeRemoval) {
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg{};
    cfg.backend = "sqlite";
    cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());
    ASSERT_TRUE(db.exec_multi(
        "CREATE TABLE modern_player_state ("
        " player_id INTEGER PRIMARY KEY, money INTEGER NOT NULL DEFAULT 0,"
        " level INTEGER NOT NULL DEFAULT 1, exp INTEGER NOT NULL DEFAULT 0,"
        " updated_at TEXT NOT NULL);"
        "CREATE TABLE modern_player_item ("
        " player_id INTEGER NOT NULL, container INTEGER NOT NULL, slot INTEGER NOT NULL,"
        " db_idx INTEGER NOT NULL, item_idx INTEGER NOT NULL, durability INTEGER NOT NULL,"
        " rare_idx INTEGER NOT NULL, quick_position INTEGER NOT NULL, item_param INTEGER NOT NULL);"
    ).ok());
    ASSERT_TRUE(create_fixture_character(db, 777u));
    ASSERT_TRUE(seed_fixture_runtime_state(db, 777u));

    ReplySpy reply;
    mxh::server::MapHandler handler(db, 10, make_reply_spy(reply));
    const auto connection = mxh::net::make_connection_id(99);
    mxh::net::Message game_in;
    game_in.header.object_id = 777u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);
    ASSERT_TRUE(handler.set_player_money_for_test(777u, 4242u));

    mxh::net::Message game_out;
    game_out.header.object_id = 777u;
    game_out.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_out.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameOutSyn);
    handler.on_message(connection, game_out);

    mxh::db::ResultSet rows;
    const std::vector<mxh::db::Bind> args{mxh::db::bind(static_cast<std::int64_t>(777))};
    ASSERT_TRUE(db.query("SELECT money FROM modern_player_state WHERE player_id=?", args, rows).ok());
    ASSERT_EQ(rows.rows.size(), 1u);
    EXPECT_EQ(std::get<std::int64_t>(rows.rows[0][0]), 4242);
    EXPECT_EQ(handler.player_runtime_count(), 0u);
    ASSERT_FALSE(reply.messages.empty());
    EXPECT_EQ(reply.last_message.header.protocol,
              static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameOutAck));
}

namespace {
std::string build_monster_attack_skill_row(std::uint16_t skill_idx) {
    std::vector<std::string> fields(150u, "0");
    fields[0] = std::to_string(skill_idx);
    fields[1] = "MonsterAttackTest";
    fields[7] = "0";
    fields[8] = "230";
    fields[16] = "0.0";
    fields[20] = "1";
    fields[25] = "1";
    fields[29] = "1000";
    fields[69] = "12";
    fields[70] = "10";
    fields[149] = "10001";
    std::ostringstream row;
    for (std::size_t i = 0; i < fields.size(); ++i) {
        if (i != 0) row << '\t';
        row << fields[i];
    }
    return row.str();
}
}  // namespace

TEST(MapHandlerTest, RevisionedSkillSpendsMpBeforeMonsterAttackAndRejectsUnsafeTargets) {
    MockDbAdapter db;
    db.seed_ready_vitals(123u, 125u, 245u, 20u);
    db.seed_ready_vitals(456u, 125u, 245u, 20u);
    ReplySpy reply;
    MapHandler handler(db, 10, make_reply_spy(reply));
    auto row = build_monster_attack_skill_row(42u);
    // NeedNaeRyuk[0] is field 43 of the legacy SkillList row.
    auto fields = std::vector<std::string>{};
    {
        std::istringstream input(row);
        std::string field;
        while (std::getline(input, field, '\t')) fields.push_back(field);
    }
    ASSERT_GT(fields.size(), 43u);
    fields[43] = "5";
    std::ostringstream cost_row;
    for (std::size_t i = 0; i < fields.size(); ++i) {
        if (i) cost_row << '\t';
        cost_row << fields[i];
    }
    const auto path = write_temp_bin(synthesize_item_list_bin(cost_row.str()));
    handler.load_skill_list(path.string());
    std::error_code ec;
    std::filesystem::remove(path, ec);
    ASSERT_NE(handler.find_skill(42u), nullptr);
    ASSERT_EQ(mxh::game::to_simple(*handler.find_skill(42u)).need_nearyuk, 5u);

    const auto source = mxh::net::make_connection_id(55);
    mxh::net::Message game_in;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    game_in.header.object_id = 123u;
    handler.on_message(source, game_in);
    const auto game_in_ack = std::find_if(reply.messages.begin(), reply.messages.end(),
        [](const auto& message) {
            return message.header.object_id == 123u &&
                message.header.category == static_cast<std::uint8_t>(
                    mxh::proto::Category::UserConn) &&
                message.header.protocol == static_cast<std::uint8_t>(
                    mxh::proto::UserConnProtocol::GameInAck);
        });
    ASSERT_NE(game_in_ack, reply.messages.end());
    const auto initial_wire = *game_in_ack;
    game_in.header.object_id = 456u;
    handler.on_message(mxh::net::make_connection_id(56), game_in);
    ASSERT_TRUE(handler.set_player_position_for_test(123u, 0.0f, 0.0f));
    ASSERT_TRUE(handler.set_player_position_for_test(456u, 0.0f, 0.0f));

    mxh::game::MonsterInstance monster;
    monster.object_id = 88042u;
    monster.monster_kind = 73u;
    monster.map_num = 10u;
    monster.level = 1u;
    monster.max_life = monster.current_life = 100000u;
    monster.pos_x = monster.pos_z = 0.0f;
    ASSERT_TRUE(handler.add_monster_instance(monster));
    mxh::net::Message skill;
    skill.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Skill);
    skill.header.protocol = static_cast<std::uint8_t>(mxh::proto::SkillProtocol::StartSyn);
    skill.header.object_id = 123u;
    skill.payload.resize(16u, 0u);
    const std::uint32_t idx = 42u;
    std::memcpy(skill.payload.data(), &idx, 4u);

    // A different Agent connection cannot spend this actor's MP.
    const auto before = db.commit_count;
    std::uint32_t target = monster.object_id;
    std::memcpy(skill.payload.data() + 4u, &target, 4u);
    handler.on_message(mxh::net::make_connection_id(56), skill);
    EXPECT_EQ(db.commit_count, before);
    EXPECT_EQ(db.vitals_rows.at(123u)[2], 20);

    // The present limited slice cannot damage another player without a
    // matching target-vitals transaction; it must reject before MP or wire.
    reply.messages.clear();
    target = 456u;
    std::memcpy(skill.payload.data() + 4u, &target, 4u);
    handler.on_message(source, skill);
    EXPECT_EQ(db.commit_count, before);
    EXPECT_EQ(db.vitals_rows.at(123u)[2], 20);
    ASSERT_FALSE(reply.messages.empty());
    EXPECT_EQ(reply.last_message.header.protocol,
        static_cast<std::uint8_t>(mxh::proto::SkillProtocol::StartNack));

    reply.messages.clear();
    target = monster.object_id;
    std::memcpy(skill.payload.data() + 4u, &target, 4u);
    handler.on_message(source, skill);
    EXPECT_EQ(db.commit_count, before + 1);
    EXPECT_EQ(db.vitals_rows.at(123u)[2], 15);
    EXPECT_EQ(db.vitals_rows.at(123u)[5], 2);
    const auto snapshot = handler.player_runtime_snapshot(123u);
    ASSERT_TRUE(snapshot.has_value());
    EXPECT_EQ(snapshot->current_mp, 15u);
    const auto mp_sync = std::find_if(reply.messages.begin(), reply.messages.end(),
        [](const auto& message) {
            return message.header.category == static_cast<std::uint8_t>(
                       mxh::proto::Category::Character) &&
                message.header.protocol == static_cast<std::uint8_t>(
                       mxh::proto::CharacterProtocol::NaeRyukAck);
        });
    ASSERT_NE(mp_sync, reply.messages.end());
    ASSERT_EQ(mp_sync->payload.size(), 4u);
    std::uint32_t wire_mp = 0;
    std::memcpy(&wire_mp, mp_sync->payload.data(), sizeof(wire_mp));
    EXPECT_EQ(wire_mp, 15u);
    mxh::client::CInGameState client;
    client.Init(nullptr);
    client.on_message(source, initial_wire);
    client.on_message(source, *mp_sync);
    EXPECT_EQ(client.game_info().mp, 15u);
    EXPECT_TRUE(std::any_of(reply.messages.begin(), reply.messages.end(), [](const auto& message) {
        return message.header.category == static_cast<std::uint8_t>(mxh::proto::Category::Skill) &&
               message.header.protocol == static_cast<std::uint8_t>(mxh::proto::SkillProtocol::StartAck);
    }));

    // A lost commit reply is unknown, so the old actor cannot emit another
    // skill acknowledgement or use its unchanged in-memory MP snapshot.
    reply.messages.clear();
    db.fail_commit_on = before + 2;
    handler.on_message(source, skill);
    EXPECT_EQ(db.commit_count, before + 2);
    EXPECT_EQ(db.disconnect_count, 1);
    EXPECT_EQ(db.vitals_rows.at(123u)[2], 15);
    EXPECT_EQ(handler.player_runtime_snapshot(123u)->current_mp, 15u);
    EXPECT_TRUE(reply.messages.empty());
}

TEST(MapHandlerTest, PreloadedQuestAllowsRevisionedSkillAndAdvancesKillProgress) {
    MockDbAdapter db;
    db.seed_ready_vitals(123u, 125u, 245u, 20u);
    db.preloaded_quest_id = 1u;
    ReplySpy reply;
    MapHandler handler(db, 10u, make_reply_spy(reply));
    handler.set_skill_damage_rng_seed_for_test(1u);  // first dodge roll 45, no random miss
    const auto quest_path = write_temp_bin(synthesize_dealitem_bin(
        "$QUEST 1 { $SUBQUEST 1 { #TRIGGER @HUNT 73 1 *ADDCOUNT 1 1 } }"));
    handler.load_quest_script(quest_path.string());
    std::error_code ec;
    std::filesystem::remove(quest_path, ec);
    ASSERT_EQ(handler.quest_definitions_for_test().quests.size(), 1u);
    const auto skill_path = write_temp_bin(synthesize_item_list_bin(
        build_monster_attack_skill_row(42u)));
    handler.load_skill_list(skill_path.string());
    std::filesystem::remove(skill_path, ec);
    ASSERT_NE(handler.find_skill(42u), nullptr);
    handler.load_experience_curve(std::string(MXH_SOURCE_DIR) +
        "/data/PlayDH/Resource/CharacterExpPoint.bin");
    const auto source = mxh::net::make_connection_id(55);
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(source, game_in);
    ASSERT_EQ(handler.player_quest_count_for_test(123u), 1u);
    ASSERT_TRUE(handler.set_player_position_for_test(123u, 0.0f, 0.0f));
    mxh::game::MonsterInstance monster;
    monster.object_id = 88042u;
    monster.monster_kind = 73u;
    monster.map_num = 10u;
    monster.max_life = monster.current_life = 1u;
    monster.pos_x = monster.pos_z = 0.0f;
    ASSERT_TRUE(handler.add_monster_instance(monster));
    mxh::net::Message skill;
    skill.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Skill);
    skill.header.protocol = static_cast<std::uint8_t>(mxh::proto::SkillProtocol::StartSyn);
    skill.header.object_id = 123u;
    skill.payload.resize(16u, 0u);
    const std::uint32_t skill_idx = 42u, target = monster.object_id;
    std::memcpy(skill.payload.data(), &skill_idx, 4u);
    std::memcpy(skill.payload.data() + 4u, &target, 4u);
    reply.messages.clear();
    const auto before = db.commit_count;
    handler.on_message(source, skill);
    EXPECT_GT(db.commit_count, before);
    EXPECT_EQ(db.vitals_rows.at(123u)[2], 125);
    EXPECT_EQ(handler.live_monster_count_for_test(), 5u);
    EXPECT_FALSE(std::any_of(reply.messages.begin(), reply.messages.end(), [](const auto& m) {
        return m.header.category == static_cast<std::uint8_t>(mxh::proto::Category::Skill) &&
            m.header.protocol == static_cast<std::uint8_t>(mxh::proto::SkillProtocol::StartNack);
    }));
    EXPECT_TRUE(std::any_of(reply.messages.begin(), reply.messages.end(), [](const auto& m) {
        return m.header.category == static_cast<std::uint8_t>(mxh::proto::Category::Skill) &&
            m.header.protocol == static_cast<std::uint8_t>(mxh::proto::SkillProtocol::StartAck);
    }));
    const auto progress = handler.quest_progress_for_test(123u, 1u);
    ASSERT_TRUE(progress.has_value());
    ASSERT_FALSE(progress->subs.empty());
    EXPECT_EQ(progress->subs.front().count, 1u);
}

TEST(MapHandlerTest, LethalRewardFailureNacksWithoutSkillAckOrSecondMpSpend) {
    MockDbAdapter db;
    db.seed_ready_vitals(123u, 125u, 245u, 20u);
    ReplySpy reply;
    MapHandler handler(db, 10, make_reply_spy(reply));
    handler.set_skill_damage_rng_seed_for_test(1u);  // first dodge roll 45, no random miss
    auto row = build_monster_attack_skill_row(42u);
    std::vector<std::string> fields;
    std::istringstream input(row);
    for (std::string field; std::getline(input, field, '\t'); ) fields.push_back(field);
    ASSERT_GT(fields.size(), 43u);
    fields[43] = "5";
    std::ostringstream cost_row;
    for (std::size_t i = 0; i < fields.size(); ++i) {
        if (i) cost_row << '\t';
        cost_row << fields[i];
    }
    const auto path = write_temp_bin(synthesize_item_list_bin(cost_row.str()));
    handler.load_skill_list(path.string());
    std::error_code ec;
    std::filesystem::remove(path, ec);
    handler.load_experience_curve(std::string(MXH_SOURCE_DIR) +
        "/data/PlayDH/Resource/CharacterExpPoint.bin");
    mxh::net::Message game_in;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    game_in.header.object_id = 123u;
    const auto source = mxh::net::make_connection_id(55);
    handler.on_message(source, game_in);
    ASSERT_TRUE(handler.player_runtime_snapshot(123u).has_value());
    ASSERT_TRUE(handler.set_player_position_for_test(123u, 0.0f, 0.0f));
    mxh::game::MonsterInstance monster;
    monster.object_id = 88043u;
    monster.monster_kind = 73u;
    monster.map_num = 10u;
    monster.level = 1u;
    monster.max_life = monster.current_life = 1u;
    monster.exp_reward = 123u;
    ASSERT_TRUE(handler.add_monster_instance(monster));
    const auto live_before = handler.live_monster_count_for_test();
    mxh::net::Message skill;
    skill.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Skill);
    skill.header.protocol = static_cast<std::uint8_t>(mxh::proto::SkillProtocol::StartSyn);
    skill.header.object_id = 123u;
    skill.payload.resize(16u, 0u);
    const std::uint32_t skill_id = 42u;
    std::memcpy(skill.payload.data(), &skill_id, 4u);
    std::memcpy(skill.payload.data() + 4u, &monster.object_id, 4u);
    db.fail_progress_update = true;
    reply.messages.clear();
    handler.on_message(source, skill);
    EXPECT_EQ(db.vitals_rows.at(123u)[2], 15);
    EXPECT_EQ(db.progress_rows.at(123u)[1], 0);
    EXPECT_EQ(handler.live_monster_count_for_test(), live_before);
    const auto has_protocol = [&](mxh::proto::Category category, std::uint8_t protocol) {
        return std::any_of(reply.messages.begin(), reply.messages.end(), [&](const auto& message) {
            return message.header.category == static_cast<std::uint8_t>(category) &&
                   message.header.protocol == protocol;
        });
    };
    EXPECT_TRUE(has_protocol(mxh::proto::Category::Skill,
        static_cast<std::uint8_t>(mxh::proto::SkillProtocol::StartNack)));
    EXPECT_FALSE(has_protocol(mxh::proto::Category::Skill,
        static_cast<std::uint8_t>(mxh::proto::SkillProtocol::StartAck)));
    EXPECT_FALSE(has_protocol(mxh::proto::Category::Character, 13u));
    const auto commits = db.commit_count;
    reply.messages.clear();
    handler.on_message(source, skill);
    EXPECT_EQ(db.commit_count, commits);
    EXPECT_EQ(db.vitals_rows.at(123u)[2], 15);
    EXPECT_EQ(handler.live_monster_count_for_test(), live_before);
}

TEST(MapHandlerTest, LethalSkillPublishesFinalRefilledMpToClient) {
    MockDbAdapter db;
    db.seed_ready_vitals(123u, 125u, 245u, 20u);
    ReplySpy reply;
    MapHandler handler(db, 10, make_reply_spy(reply));
    handler.set_skill_damage_rng_seed_for_test(1u);
    std::vector<std::string> fields;
    std::istringstream input(build_monster_attack_skill_row(42u));
    for (std::string field; std::getline(input, field, '\t'); ) fields.push_back(field);
    ASSERT_GT(fields.size(), 43u);
    fields[43] = "5";
    std::ostringstream cost_row;
    for (std::size_t i = 0; i < fields.size(); ++i) {
        if (i) cost_row << '\t';
        cost_row << fields[i];
    }
    const auto path = write_temp_bin(synthesize_item_list_bin(cost_row.str()));
    handler.load_skill_list(path.string());
    std::error_code ec;
    std::filesystem::remove(path, ec);
    handler.load_experience_curve(std::string(MXH_SOURCE_DIR) +
        "/data/PlayDH/Resource/CharacterExpPoint.bin");
    const auto source = mxh::net::make_connection_id(55);
    mxh::net::Message game_in;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    game_in.header.object_id = 123u;
    handler.on_message(source, game_in);
    const auto game_in_ack = std::find_if(reply.messages.begin(), reply.messages.end(),
        [](const auto& message) {
            return message.header.category == static_cast<std::uint8_t>(mxh::proto::Category::UserConn) &&
                message.header.protocol == static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInAck);
        });
    ASSERT_NE(game_in_ack, reply.messages.end());
    const auto initial_wire = *game_in_ack;
    ASSERT_TRUE(handler.set_player_position_for_test(123u, 0.0f, 0.0f));
    mxh::game::MonsterInstance monster;
    monster.object_id = 88043u;
    monster.monster_kind = 73u;
    monster.map_num = 10u;
    monster.level = 1u;
    monster.max_life = monster.current_life = 1u;
    monster.exp_reward = 4u;  // canonical Lv1 first 20% interval refills MP
    const auto initial_monster_count = handler.live_monster_count_for_test();
    ASSERT_TRUE(handler.add_monster_instance(monster));
    mxh::net::Message skill;
    skill.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Skill);
    skill.header.protocol = static_cast<std::uint8_t>(mxh::proto::SkillProtocol::StartSyn);
    skill.header.object_id = 123u;
    skill.payload.resize(16u, 0u);
    const std::uint32_t skill_id = 42u;
    std::memcpy(skill.payload.data(), &skill_id, 4u);
    std::memcpy(skill.payload.data() + 4u, &monster.object_id, 4u);
    reply.messages.clear();
    handler.on_message(source, skill);
    ASSERT_EQ(db.progress_rows.at(123u)[1], 4);
    ASSERT_EQ(db.vitals_rows.at(123u)[2], 125);
    ASSERT_EQ(handler.live_monster_count_for_test(), initial_monster_count);
    std::vector<mxh::net::Message> mp_messages;
    for (const auto& message : reply.messages) {
        if (message.header.category == static_cast<std::uint8_t>(mxh::proto::Category::Character) &&
            message.header.protocol == static_cast<std::uint8_t>(mxh::proto::CharacterProtocol::NaeRyukAck)) {
            mp_messages.push_back(message);
        }
    }
    ASSERT_EQ(mp_messages.size(), 1u) << "spent MP must not overwrite the later authoritative refill";
    ASSERT_EQ(mp_messages.front().payload.size(), 4u);
    std::uint32_t wire_mp = 0;
    std::memcpy(&wire_mp, mp_messages.front().payload.data(), sizeof(wire_mp));
    EXPECT_EQ(wire_mp, 125u);
    mxh::client::CInGameState client;
    client.Init(nullptr);
    client.on_message(source, initial_wire);
    client.on_message(source, mp_messages.front());
    EXPECT_EQ(client.game_info().mp, 125u);
}

TEST(MapHandlerTest, RevisionedVitalsRejectsUncoupledItemAndQuestWritesBeforeMutation) {
    MockDbAdapter db;
    db.seed_ready_vitals(123u, 125u, 245u, 20u);
    ReplySpy reply;
    MapHandler handler(db, 10, make_reply_spy(reply));
    const auto source = mxh::net::make_connection_id(55);
    mxh::net::Message game_in;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    game_in.header.object_id = 123u;
    handler.on_message(source, game_in);
    ASSERT_EQ(handler.player_runtime_count(), 1u);
    const auto writes = db.exec_count.load();
    const auto commits = db.commit_count;

    struct Rejection {
        mxh::proto::Category category;
        std::uint8_t request;
        std::uint8_t nack;
        std::size_t payload_size;
    };
    using I = mxh::proto::ItemProtocol;
    using Q = mxh::proto::QuestProtocol;
    const Rejection rejected[] = {
        {mxh::proto::Category::Item, static_cast<std::uint8_t>(I::DiscardSyn),
            static_cast<std::uint8_t>(I::DiscardNack), 2u},
        {mxh::proto::Category::Item, static_cast<std::uint8_t>(I::SellSyn),
            static_cast<std::uint8_t>(I::SellNack), 8u},
        {mxh::proto::Category::Item, static_cast<std::uint8_t>(I::UseSyn),
            static_cast<std::uint8_t>(I::UseNack), 2u},
        {mxh::proto::Category::Item, static_cast<std::uint8_t>(I::BuySyn),
            static_cast<std::uint8_t>(I::BuyNack), 4u},
        {mxh::proto::Category::Quest, static_cast<std::uint8_t>(Q::StartSyn),
            static_cast<std::uint8_t>(Q::StartNack), 2u},
        {mxh::proto::Category::Quest, static_cast<std::uint8_t>(Q::EndSyn),
            static_cast<std::uint8_t>(Q::EndNack), 2u},
    };
    for (const auto& item : rejected) {
        SCOPED_TRACE(static_cast<int>(item.request));
        mxh::net::Message request;
        request.header.category = static_cast<std::uint8_t>(item.category);
        request.header.protocol = item.request;
        request.header.object_id = 123u;
        request.payload.resize(item.payload_size, 0u);
        reply.messages.clear();
        handler.on_message(source, request);
        ASSERT_EQ(reply.messages.size(), 1u);
        EXPECT_EQ(reply.last_id.value, source.value);
        EXPECT_EQ(reply.last_message.header.category, request.header.category);
        EXPECT_EQ(reply.last_message.header.protocol, item.nack);
        EXPECT_EQ(reply.last_message.payload.size(), item.payload_size);
        EXPECT_EQ(db.exec_count.load(), writes);
        EXPECT_EQ(db.commit_count, commits);
        EXPECT_EQ(db.vitals_rows.at(123u)[0], 125);
        EXPECT_EQ(db.vitals_rows.at(123u)[2], 20);
    }
    mxh::net::Message forged;
    forged.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    forged.header.protocol = static_cast<std::uint8_t>(I::UseSyn);
    forged.header.object_id = 123u;
    forged.payload = {0u, 0u};
    reply.messages.clear();
    handler.on_message(mxh::net::make_connection_id(56), forged);
    EXPECT_TRUE(reply.messages.empty());
    EXPECT_EQ(db.exec_count.load(), writes);
    EXPECT_EQ(db.commit_count, commits);
}

TEST(MapHandlerTest, Map10AiKindsRequireCanonicalMonsterList) {
    MockDbAdapter db;
    MapHandler handler(db, 10, [](mxh::net::ConnectionId, const mxh::net::Message&) {});
    const auto ai_path = std::filesystem::path(MXH_SOURCE_DIR) /
        "data/PlayDH/Resource/Server/Monster_10.bin";
    const auto groups = load_ai_group_list_bin(ai_path, "playdh-current");
    ASSERT_TRUE(groups.has_value());
    EXPECT_FALSE(handler.has_ai_group_monster_templates(*groups));
    handler.load_monster_list((std::filesystem::path(MXH_SOURCE_DIR) /
        "data/PlayDH/Resource/MonsterList.bin").string());
    ASSERT_TRUE(handler.has_loaded_monster_list());
    handler.load_skill_list((std::filesystem::path(MXH_SOURCE_DIR) /
        "data/PlayDH/Resource/SkillList.bin").string());
    ASSERT_TRUE(handler.has_loaded_skill_list());
    for (const auto id : {1444u, 1508u}) {
        const auto* skill = handler.find_skill(id);
        ASSERT_NE(skill, nullptr);
        EXPECT_EQ(skill->ComboNum, 10u);
        EXPECT_FLOAT_EQ(skill->FirstPhyAttack[0], 2.04f);
    }
    EXPECT_TRUE(handler.has_ai_group_monster_templates(*groups));
    EXPECT_EQ(groups->spawn_count(), 228u);
}

TEST(MapHandlerTest, MissingVitalsRowRejectsGameInBeforeRuntimeRegistration) {
    MockDbAdapter db;
    db.missing_vitals_ids.insert(123u);
    ReplySpy reply;
    MapHandler handler(db, 10, make_reply_spy(reply));
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(mxh::net::make_connection_id(55), game_in);
    EXPECT_FALSE(handler.player_runtime_snapshot(123u).has_value());
    ASSERT_FALSE(reply.messages.empty());
    EXPECT_EQ(reply.last_message.header.protocol,
              static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInNack));
    EXPECT_EQ(db.commit_count, 0);
}

TEST(MapHandlerTest, MissingProgressRowRejectsGameInBeforeRuntimeRegistration) {
    MockDbAdapter db;
    db.missing_progress_ids.insert(123u);
    ReplySpy reply;
    MapHandler handler(db, 10, make_reply_spy(reply));
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(mxh::net::make_connection_id(55), game_in);
    EXPECT_FALSE(handler.player_runtime_snapshot(123u).has_value());
    ASSERT_FALSE(reply.messages.empty());
    EXPECT_EQ(reply.last_message.header.protocol,
              static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInNack));
    EXPECT_EQ(db.commit_count, 0);
}

TEST(MapHandlerTest, PartialNullProgressRejectsGameInWithoutBackfill) {
    MockDbAdapter db;
    db.partial_progress_ids.insert(123u);
    ReplySpy reply;
    MapHandler handler(db, 10, make_reply_spy(reply));
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(mxh::net::make_connection_id(55), game_in);
    EXPECT_FALSE(handler.player_runtime_snapshot(123u).has_value());
    ASSERT_FALSE(reply.messages.empty());
    EXPECT_EQ(reply.last_message.header.protocol,
              static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInNack));
    EXPECT_EQ(db.commit_count, 0);
    EXPECT_EQ(db.progress_rows.at(123u)[4], 0);
}

TEST(MapHandlerTest, PersistedZeroHpStaysDeadAcrossFreshGameIn) {
    MockDbAdapter db;
    db.seed_ready_vitals(123u, 0u, 0u, 0u, 2);
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    for (int i = 0; i < 2; ++i) {
        ReplySpy reply;
        MapHandler handler(db, 10, make_reply_spy(reply));
        handler.on_message(mxh::net::make_connection_id(55 + i), game_in);
        const auto snapshot = handler.player_runtime_snapshot(123u);
        ASSERT_TRUE(snapshot);
        EXPECT_EQ(snapshot->current_hp, 0u);
        EXPECT_EQ(snapshot->lifecycle, PlayerLifecycle::Dead);
        EXPECT_EQ(db.vitals_rows.at(123u)[5], 2);
    }
    EXPECT_EQ(db.commit_count, 0);
}

TEST(MapHandlerTest, MonsterAiAttackUsesSkillDelayAndPublishesAuthoritativeDeath) {
    MockDbAdapter db;
    db.seed_ready_vitals(123u, 3u, 100u, 0u);
    ReplySpy reply;
    std::vector<std::pair<std::uint64_t, mxh::net::Message>> routed;
    const auto spy = make_reply_spy(reply);
    MapHandler handler(db, 10, [&](mxh::net::ConnectionId id, const mxh::net::Message& msg) {
        routed.emplace_back(id.value, msg);
        spy(id, msg);
    });
    handler.load_map_kind_info((std::filesystem::path(MXH_SOURCE_DIR) /
        "data/PlayDH/Resource/MapKindInfo.bin").string());
    ASSERT_TRUE(handler.ordinary_map_verified());
    const auto skill_path = write_temp_bin(synthesize_item_list_bin(
        build_monster_attack_skill_row(42u)));
    handler.load_skill_list(skill_path.string());
    std::error_code ec;
    std::filesystem::remove(skill_path, ec);
    ASSERT_NE(handler.find_skill(42u), nullptr);

    const auto connection = mxh::net::make_connection_id(55);
    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(connection, game_in);
    game_in.header.object_id = 456u;
    handler.on_message(connection, game_in);
    game_in.header.object_id = 789u;
    handler.on_message(mxh::net::make_connection_id(56), game_in);
    ASSERT_TRUE(handler.set_player_position_for_test(123u, 0.0f, 0.0f));

    mxh::game::MonsterInstance monster;
    monster.object_id = 88042u;
    monster.monster_kind = 73u;
    monster.map_num = 10u;
    monster.level = 1u;
    monster.max_life = monster.current_life = 100u;
    monster.attack_min = monster.attack_max = 50u;
    monster.attack_count = 1u;
    monster.attack_skills[0] = 42u;
    monster.search_period_ms = 1u;
    monster.ai_state = mxh::game::MonsterAIState::Attack;
    monster.target_object_id = 123u;
    ASSERT_TRUE(handler.add_monster_instance(monster));

    std::uint64_t now = 1000u;
    handler.set_monster_ai_clock_for_test([&] { return now; });
    handler.set_monster_ai_rng_for_test([] { return 0u; });
    reply.messages.clear();
    handler.tick_monster_ai();
    EXPECT_TRUE(reply.messages.empty()) << "legacy delay uses a strict boundary";

    now = 1001u;
    handler.tick_monster_ai();
    const auto has = [&](mxh::proto::Category category, std::uint8_t protocol) {
        return std::find_if(reply.messages.begin(), reply.messages.end(), [&](const auto& message) {
            return message.header.category == static_cast<std::uint8_t>(category) &&
                   message.header.protocol == protocol;
        });
    };
    const auto life = has(mxh::proto::Category::Character,
        static_cast<std::uint8_t>(mxh::proto::CharacterProtocol::LifeAck));
    ASSERT_NE(life, reply.messages.end());
    ASSERT_EQ(life->payload.size(), 4u);
    std::int32_t life_delta = 0;
    std::memcpy(&life_delta, life->payload.data(), 4u);
    EXPECT_EQ(life_delta, -3);
    const auto death = has(mxh::proto::Category::UserConn,
        static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::CharacterDie));
    ASSERT_NE(death, reply.messages.end());
    ASSERT_EQ(death->payload.size(), 8u);
    std::vector<std::uint64_t> death_links;
    for (const auto& [link, message] : routed) {
        if (message.header.category == static_cast<std::uint8_t>(mxh::proto::Category::UserConn) &&
            message.header.protocol == static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::CharacterDie))
            death_links.push_back(link);
    }
    std::sort(death_links.begin(), death_links.end());
    EXPECT_EQ(death_links, (std::vector<std::uint64_t>{55u, 56u}));
    const auto snapshot = handler.player_runtime_snapshot(123u);
    ASSERT_TRUE(snapshot.has_value());
    EXPECT_EQ(snapshot->current_hp, 0u);
    EXPECT_EQ(snapshot->lifecycle, PlayerLifecycle::Dead);
    ASSERT_EQ(db.vitals_rows.at(123u)[0], 0);
    EXPECT_EQ(db.vitals_rows.at(123u)[3], 1);
    EXPECT_EQ(db.vitals_rows.at(123u)[5], 2);

    // Another connected player would receive a leaked dead-player move if
    // the Map merely skipped its position update but still broadcast below.
    mxh::net::Message second_game_in = game_in;
    second_game_in.header.object_id = 456u;
    handler.on_message(mxh::net::make_connection_id(56), second_game_in);
    reply.messages.clear();
    mxh::net::Message dead_move;
    dead_move.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Move);
    dead_move.header.protocol = static_cast<std::uint8_t>(mxh::proto::MoveProtocol::OneTarget);
    dead_move.header.object_id = 123u;
    dead_move.payload.resize(4u);
    const std::uint16_t requested_x = 200u, requested_z = 300u;
    std::memcpy(dead_move.payload.data(), &requested_x, 2);
    std::memcpy(dead_move.payload.data() + 2, &requested_z, 2);
    handler.on_message(connection, dead_move);
    ASSERT_EQ(reply.messages.size(), 1u) << "dead move must not broadcast";
    EXPECT_EQ(reply.last_id.value, connection.value);
    EXPECT_EQ(reply.messages[0].header.protocol,
              static_cast<std::uint8_t>(mxh::proto::MoveProtocol::Correction));
    ASSERT_EQ(reply.messages[0].payload.size(), 4u);
    const auto after_dead_move = handler.player_runtime_snapshot(123u);
    ASSERT_TRUE(after_dead_move.has_value());
    EXPECT_FLOAT_EQ(after_dead_move->pos_x, snapshot->pos_x);
    EXPECT_FLOAT_EQ(after_dead_move->pos_z, snapshot->pos_z);

    reply.messages.clear();
    now = 3000u;
    handler.tick_monster_ai();
    EXPECT_TRUE(reply.messages.empty()) << "dead target cannot take another hit";

    mxh::net::Message skill_syn;
    skill_syn.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Skill);
    skill_syn.header.protocol = static_cast<std::uint8_t>(mxh::proto::SkillProtocol::StartSyn);
    skill_syn.header.object_id = 123u;
    skill_syn.payload.resize(16, 0);
    const std::uint32_t skill_idx = 42u;
    std::memcpy(skill_syn.payload.data(), &skill_idx, 4);
    handler.on_message(connection, skill_syn);
    ASSERT_EQ(reply.messages.size(), 1u);
    EXPECT_EQ(reply.messages[0].header.protocol,
        static_cast<std::uint8_t>(mxh::proto::SkillProtocol::StartNack));

    mxh::net::Message revive;
    revive.header.category = static_cast<std::uint8_t>(mxh::proto::Category::CharRevive);
    revive.header.object_id = 123u;
    revive.header.protocol = 0u; // original PresentSpotSyn, empty MSGBASE
    const auto before_revive_commits = db.commit_count;
    const auto old_shield_before_revive = db.vitals_rows.at(123u)[1];
    reply.messages.clear();
    handler.on_message(connection, revive);
    EXPECT_TRUE(reply.messages.empty()) << "strictly more than five seconds required";
    EXPECT_EQ(db.commit_count, before_revive_commits);

    now = 6002u;
    handler.tick_monster_ai();
    const auto ready = std::find_if(reply.messages.begin(), reply.messages.end(), [](const auto& m) {
        return m.header.category == static_cast<std::uint8_t>(mxh::proto::Category::UserConn) &&
               m.header.protocol == static_cast<std::uint8_t>(
                   mxh::proto::UserConnProtocol::ReadyToRevive);
    });
    ASSERT_NE(ready, reply.messages.end());
    EXPECT_EQ(ready->header.object_id, 123u);
    EXPECT_TRUE(ready->payload.empty());

    reply.messages.clear();
    handler.on_message(mxh::net::make_connection_id(56), revive);
    EXPECT_TRUE(reply.messages.empty()) << "a different Map connection cannot revive this player";
    EXPECT_EQ(db.commit_count, before_revive_commits);
    revive.payload = {1};
    handler.on_message(connection, revive);
    EXPECT_TRUE(reply.messages.empty()) << "original PresentSpotSyn has no payload";
    EXPECT_EQ(db.commit_count, before_revive_commits);
    revive.payload.clear();
    handler.on_message(connection, revive);
    const auto after_revive = handler.player_runtime_snapshot(123u);
    ASSERT_TRUE(after_revive);
    EXPECT_EQ(after_revive->lifecycle, PlayerLifecycle::Active);
    EXPECT_EQ(after_revive->current_hp,
        static_cast<std::uint32_t>(static_cast<double>(after_revive->max_hp) * 0.3));
    EXPECT_EQ(db.vitals_rows.at(123u)[0], after_revive->current_hp);
    EXPECT_EQ(db.vitals_rows.at(123u)[3], 0);
    EXPECT_EQ(db.vitals_rows.at(123u)[5], 3);
    EXPECT_EQ(db.commit_count, before_revive_commits + 1);
    const auto revived = std::find_if(reply.messages.begin(), reply.messages.end(), [](const auto& m) {
        return m.header.category == static_cast<std::uint8_t>(mxh::proto::Category::UserConn) &&
               m.header.protocol == static_cast<std::uint8_t>(
                   mxh::proto::UserConnProtocol::CharacterRevive);
    });
    ASSERT_NE(revived, reply.messages.end());
    EXPECT_EQ(revived->header.object_id, 123u);
    ASSERT_EQ(revived->payload.size(), 8u);
    std::uint32_t mover = 0;
    std::memcpy(&mover, revived->payload.data(), 4u);
    EXPECT_EQ(mover, 123u);
    for (auto protocol : {mxh::proto::CharacterProtocol::LifeAck,
                          mxh::proto::CharacterProtocol::ShieldAck,
                          mxh::proto::CharacterProtocol::NaeRyukAck}) {
        EXPECT_NE(std::find_if(reply.messages.begin(), reply.messages.end(), [&](const auto& m) {
            return m.header.category == static_cast<std::uint8_t>(mxh::proto::Category::Character) &&
                   m.header.protocol == static_cast<std::uint8_t>(protocol) &&
                   m.header.object_id == 123u && m.payload.size() == 4u;
        }), reply.messages.end());
    }
    const auto shield_ack = std::find_if(reply.messages.begin(), reply.messages.end(), [](const auto& m) {
        return m.header.category == static_cast<std::uint8_t>(mxh::proto::Category::Character) &&
               m.header.protocol == static_cast<std::uint8_t>(
                   mxh::proto::CharacterProtocol::ShieldAck) && m.header.object_id == 123u;
    });
    ASSERT_NE(shield_ack, reply.messages.end());
    std::int32_t actual_shield_delta = 0;
    std::memcpy(&actual_shield_delta, shield_ack->payload.data(), 4u);
    EXPECT_GT(old_shield_before_revive, 0);
    EXPECT_EQ(static_cast<std::int64_t>(actual_shield_delta) + old_shield_before_revive,
              db.vitals_rows.at(123u)[1]);
    EXPECT_LT(actual_shield_delta, 0);
    reply.messages.clear();
    handler.on_message(connection, revive);
    EXPECT_EQ(db.commit_count, before_revive_commits + 1)
        << "replayed PresentSpot must not commit twice";
    EXPECT_TRUE(reply.messages.empty());
}

TEST(MapHandlerTest, LaterUnknownMonsterHitSuppressesEarlierQueuedVictimEvents) {
    MockDbAdapter db;
    db.seed_ready_vitals(123u, 30u, 0u, 0u);
    ReplySpy reply;
    MapHandler handler(db, 10, make_reply_spy(reply));
    const auto skill_path = write_temp_bin(synthesize_item_list_bin(
        build_monster_attack_skill_row(42u)));
    handler.load_skill_list(skill_path.string());
    std::error_code ec;
    std::filesystem::remove(skill_path, ec);
    ASSERT_NE(handler.find_skill(42u), nullptr);

    mxh::net::Message game_in;
    game_in.header.object_id = 123u;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message(mxh::net::make_connection_id(55), game_in);
    ASSERT_TRUE(handler.set_player_position_for_test(123u, 0.0f, 0.0f));
    mxh::game::MonsterInstance monster;
    monster.object_id = 88042u;
    monster.monster_kind = 73u;
    monster.map_num = 10u;
    monster.level = 1u;
    monster.max_life = monster.current_life = 100u;
    monster.attack_min = monster.attack_max = 10u;
    monster.attack_count = 1u;
    monster.attack_skills[0] = 42u;
    monster.search_period_ms = 1u;
    monster.ai_state = mxh::game::MonsterAIState::Attack;
    monster.target_object_id = 123u;
    ASSERT_TRUE(handler.add_monster_instance(monster));
    monster.object_id = 88043u;
    ASSERT_TRUE(handler.add_monster_instance(monster));
    handler.set_monster_ai_clock_for_test([] { return 1001u; });
    handler.set_monster_ai_rng_for_test([] { return 0u; });
    db.fail_commit_on = 2;
    reply.messages.clear();
    handler.tick_monster_ai();
    EXPECT_EQ(db.commit_count, 2);
    EXPECT_EQ(db.disconnect_count, 1);
    EXPECT_FALSE(db.is_connected());
    EXPECT_EQ(db.vitals_rows.at(123u)[5], 2); // first hit committed
    const auto snapshot = handler.player_runtime_snapshot(123u);
    ASSERT_TRUE(snapshot);
    EXPECT_LT(snapshot->current_hp, 30u);
    for (const auto& message : reply.messages) {
        EXPECT_FALSE(message.header.category == static_cast<std::uint8_t>(mxh::proto::Category::Character) &&
            message.header.protocol == static_cast<std::uint8_t>(mxh::proto::CharacterProtocol::LifeAck));
        EXPECT_FALSE(message.header.category == static_cast<std::uint8_t>(mxh::proto::Category::UserConn) &&
            message.header.protocol == static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::CharacterDie));
        EXPECT_FALSE(message.header.category == static_cast<std::uint8_t>(mxh::proto::Category::Skill) &&
            message.header.protocol == static_cast<std::uint8_t>(mxh::proto::SkillProtocol::SkillObjectAdd));
    }
    reply.messages.clear();
    const auto writes_before = db.exec_count.load();
    mxh::net::Message move;
    move.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Move);
    move.header.protocol = static_cast<std::uint8_t>(mxh::proto::MoveProtocol::OneTarget);
    move.header.object_id = 123u;
    move.payload.resize(4u, 0);
    handler.on_message(mxh::net::make_connection_id(55), move);
    mxh::net::Message use;
    use.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    use.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::UseSyn);
    use.header.object_id = 123u;
    use.payload.resize(2u, 0);
    handler.on_message(mxh::net::make_connection_id(55), use);
    EXPECT_TRUE(reply.messages.empty());
    EXPECT_EQ(db.exec_count.load(), writes_before);
}

class FaultedItemLoadDb final : public mxh::db::IDbAdapter {
public:
    enum class Fault { None, QueryError, NullDurability } fault = Fault::None;
    mxh::db::SqliteAdapter storage;
    unsigned item_deletes = 0;

    mxh::db::DbResult connect(const mxh::db::ConnectionConfig& cfg) override { return storage.connect(cfg); }
    void disconnect() override { storage.disconnect(); }
    bool is_connected() const noexcept override { return storage.is_connected(); }
    mxh::db::DbResult execute(std::string_view sql, std::span<const mxh::db::Bind> args) override {
        if (sql.find("DELETE FROM modern_player_item") != std::string_view::npos) ++item_deletes;
        return storage.execute(sql, args);
    }
    mxh::db::DbResult query(std::string_view sql, std::span<const mxh::db::Bind> args,
                            mxh::db::ResultSet& out) override {
        const bool item_load = sql.find("FROM modern_player_item WHERE player_id=? ORDER BY container,slot") !=
                               std::string_view::npos;
        if (item_load && fault == Fault::QueryError)
            return {mxh::db::DbError::IoError, "fixture item read failure"};
        auto result = storage.query(sql, args, out);
        if (result.ok() && item_load && fault == Fault::NullDurability && !out.rows.empty())
            out.rows.front()[4] = std::monostate{};
        return result;
    }
    mxh::db::DbResult begin_transaction() override { return storage.begin_transaction(); }
    mxh::db::DbResult commit() override { return storage.commit(); }
    mxh::db::DbResult rollback() override { return storage.rollback(); }
    std::string backend_name() const noexcept override { return "faulted-sqlite"; }
};

TEST(EquipmentPersistence, FailedOrMalformedItemReadKeepsExistingOwnedEquipment) {
    FaultedItemLoadDb db;
    mxh::db::ConnectionConfig cfg; cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());
    ASSERT_TRUE(mxh::db::migrate_modern_schema(db.storage).ok());
    ASSERT_TRUE(db.execute("INSERT INTO character_info(chrid,userid,charname,level,"
                           "base_gengol,base_minchub,base_cheryuk,base_simmek) "
                           "VALUES(123,'1','OwnedHero',1,12,12,12,12)", {}).ok());
    ASSERT_TRUE(db.execute("INSERT INTO modern_player_item(player_id,container,slot,db_idx,item_idx,"
                           "durability,rare_idx,quick_position,item_param) "
                           "VALUES(123,1,1,700001,11000,100,0,65535,1)", {}).ok());
    ReplySpy reply;
    MapHandler handler(db, 10, make_reply_spy(reply));
    handler.load_item_list((std::filesystem::path(MXH_SOURCE_DIR) / "data/PlayDH/Resource/ItemList.bin").string());
    mxh::net::Message game_in;
    game_in.header.object_id = 123;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    const auto connection = mxh::net::make_connection_id(91);
    for (auto fault : {FaultedItemLoadDb::Fault::QueryError,
                       FaultedItemLoadDb::Fault::NullDurability}) {
        db.fault = fault;
        reply.messages.clear();
        handler.on_message(connection, game_in);
        EXPECT_FALSE(handler.player_runtime_snapshot(123));
        ASSERT_EQ(reply.messages.size(), 1u);
        EXPECT_EQ(reply.messages.front().header.protocol,
                  static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInNack));
        EXPECT_EQ(db.item_deletes, 0u);
        mxh::db::ResultSet persisted;
        ASSERT_TRUE(db.storage.query("SELECT container,slot,db_idx,item_idx,durability FROM modern_player_item WHERE player_id=123",
                                     {}, persisted).ok());
        ASSERT_EQ(persisted.rows.size(), 1u);
        EXPECT_EQ(std::get<std::int64_t>(persisted.rows[0][0]), 1);
        EXPECT_EQ(std::get<std::int64_t>(persisted.rows[0][1]), 1);
        EXPECT_EQ(std::get<std::int64_t>(persisted.rows[0][2]), 700001);
        EXPECT_EQ(std::get<std::int64_t>(persisted.rows[0][3]), 11000);
        EXPECT_EQ(std::get<std::int64_t>(persisted.rows[0][4]), 100);
    }
}

TEST(EquipmentPersistence, UnsupportedOwnedEquipmentRejectsAdmissionWithoutMutation) {
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg; cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());
    ASSERT_TRUE(mxh::db::migrate_modern_schema(db).ok());
    ASSERT_TRUE(seed_fixture_runtime_state(db, 777u, 48u));
    ASSERT_TRUE(db.execute("INSERT INTO character_info(chrid,userid,charname,level,"
                           "base_gengol,base_minchub,base_cheryuk,base_simmek) "
                           "VALUES(123,'1','UnsupportedHero',1,12,12,12,12)", {}).ok());
    ASSERT_TRUE(db.execute("INSERT INTO modern_player_item(player_id,container,slot,db_idx,item_idx,"
                           "durability,rare_idx,quick_position,item_param) "
                           "VALUES(123,1,1,700001,555,100,0,65535,1)", {}).ok());
    ReplySpy reply;
    MapHandler handler(db, 10, make_reply_spy(reply));
    handler.load_item_list((std::filesystem::path(MXH_SOURCE_DIR) / "data/PlayDH/Resource/ItemList.bin").string());
    mxh::net::Message game_in;
    game_in.header.object_id = 123;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    handler.on_message({91}, game_in);
    EXPECT_FALSE(handler.player_runtime_snapshot(123));
    ASSERT_EQ(reply.messages.size(), 1u);
    EXPECT_EQ(reply.messages.front().header.protocol,
              static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInNack));
    mxh::db::ResultSet rows;
    ASSERT_TRUE(db.query("SELECT db_idx,item_idx FROM modern_player_item WHERE player_id=123", {}, rows).ok());
    ASSERT_EQ(rows.rows.size(), 1u);
    EXPECT_EQ(std::get<std::int64_t>(rows.rows[0][0]), 700001);
    EXPECT_EQ(std::get<std::int64_t>(rows.rows[0][1]), 555);
}

TEST(EquipmentPersistence, ProtocolCreateEquipUnequipRepeatAndRelogin) {
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg; cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());
    ASSERT_TRUE(mxh::db::migrate_modern_schema(db).ok());
    ReplySpy reply;
    AgentHandler agent(db, make_reply_spy(reply), true, false, {}, 10);
    RegisteredMapSender birth;
    register_birth_map(agent, birth, 10);
    mxh::net::Message create;
    create.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    create.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::CharacterMakeSyn);
    create.payload.assign(59, 0);
    std::memcpy(create.payload.data(), "EquipmentHero", 13);
    const std::uint32_t account = 123;
    const float scale = 1.f;
    std::memcpy(create.payload.data() + 17, &account, 4);
    std::memcpy(create.payload.data() + 51, &scale, 4);
    std::memcpy(create.payload.data() + 55, &scale, 4);
    create.payload[25] = 17;
    agent.on_message({90}, create);
    mxh::db::ResultSet rows;
    ASSERT_TRUE(db.query("SELECT chrid,base_gengol,base_minchub,base_cheryuk,base_simmek FROM character_info", {}, rows).ok());
    ASSERT_EQ(rows.rows.size(), 1u);
    for (unsigned i = 1; i < 5; ++i) EXPECT_EQ(std::get<std::int64_t>(rows.rows[0][i]), 12);
    const auto character = static_cast<std::uint32_t>(std::get<std::int64_t>(rows.rows[0][0]));
    const std::vector<mxh::db::Bind> grant_args{mxh::db::bind(static_cast<std::int64_t>(character))};
    ASSERT_TRUE(db.execute("INSERT INTO modern_item_grant(idempotency_key,character_id,item_id,item_count,status,created_by,reason) "
                           "VALUES('equipment-test',?,11000,1,'pending','unit-test','ordinary starter')", grant_args).ok());
    mxh::net::Message game_in;
    game_in.header = create.header;
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    game_in.header.object_id = character;
    game_in.payload.assign(16, 0);
    std::memcpy(game_in.payload.data(), &account, 4);
    const auto configure = [&](MapHandler& handler) {
        handler.load_item_list((std::filesystem::path(MXH_SOURCE_DIR) / "data/PlayDH/Resource/ItemList.bin").string());
    };
    std::uint32_t database_id = 0;
    const auto move = [&](MapHandler& handler, std::uint16_t destination) {
        mxh::net::Message request;
        request.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
        request.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::MoveSyn);
        request.header.object_id = character;
        request.payload.assign(24, 0);
        std::memcpy(request.payload.data(), &database_id, 4);
        std::memcpy(request.payload.data() + 22, &destination, 2);
        handler.on_message({91}, request);
    };
    {
        MapHandler handler(db, 10, make_reply_spy(reply)); configure(handler);
        handler.on_message({91}, game_in);
        auto snapshot = handler.player_runtime_snapshot(character); ASSERT_TRUE(snapshot);
        EXPECT_EQ(snapshot->max_hp, 125u);
        EXPECT_EQ(snapshot->physical_defence, 8u);
        EXPECT_EQ(snapshot->physical_attack_min, 16u);
        ASSERT_TRUE(db.query("SELECT db_idx FROM modern_player_item", {}, rows).ok());
        ASSERT_EQ(rows.rows.size(), 1u);
        database_id = static_cast<std::uint32_t>(std::get<std::int64_t>(rows.rows[0][0]));
        move(handler, 81);
        snapshot = handler.player_runtime_snapshot(character); ASSERT_TRUE(snapshot);
        EXPECT_EQ(snapshot->physical_attack_min, 29u);
        EXPECT_EQ(snapshot->physical_attack_max, 35u);
        move(handler, 81);
        EXPECT_EQ(reply.last_message.header.protocol, static_cast<std::uint8_t>(mxh::proto::ItemProtocol::MoveNack));
        EXPECT_EQ(handler.player_runtime_snapshot(character)->physical_attack_min, 29u);
        move(handler, 0);
        EXPECT_EQ(handler.player_runtime_snapshot(character)->physical_attack_min, 16u);
        move(handler, 81);
        EXPECT_EQ(handler.player_runtime_snapshot(character)->physical_attack_min, 29u);
        handler.on_disconnect({91}, mxh::net::NetError::Disconnected);
    }
    {
        MapHandler handler(db, 10, make_reply_spy(reply)); configure(handler);
        handler.on_message({91}, game_in);
        auto snapshot = handler.player_runtime_snapshot(character); ASSERT_TRUE(snapshot);
        EXPECT_EQ(snapshot->physical_attack_min, 29u);
        EXPECT_EQ(snapshot->physical_attack_max, 35u);
        move(handler, 0);
        EXPECT_EQ(handler.player_runtime_snapshot(character)->physical_attack_min, 16u);
        handler.on_disconnect({91}, mxh::net::NetError::Disconnected);
    }
    {
        MapHandler handler(db, 10, make_reply_spy(reply)); configure(handler);
        handler.on_message({91}, game_in);
        auto snapshot = handler.player_runtime_snapshot(character); ASSERT_TRUE(snapshot);
        EXPECT_EQ(snapshot->physical_attack_min, 16u);
    }
}

TEST(EquipmentPersistence, UnknownAndPartialLegacyAttributesRejectWithoutBackfill) {
    mxh::db::SqliteAdapter db;
    mxh::db::ConnectionConfig cfg; cfg.path = ":memory:";
    ASSERT_TRUE(db.connect(cfg).ok());
    ASSERT_TRUE(mxh::db::migrate_modern_schema(db).ok());
    ASSERT_TRUE(db.execute("INSERT INTO character_info(chrid,userid,charname,level) "
                           "VALUES(777,'123','OldUnknown',48)", {}).ok());
    ASSERT_TRUE(seed_fixture_runtime_state(db, 777u, 48u));
    ReplySpy reply;
    MapHandler handler(db, 10, make_reply_spy(reply));
    mxh::net::Message game_in;
    game_in.header.object_id = 777;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInSyn);
    game_in.payload.assign(16, 0);
    game_in.payload[0] = 123;
    handler.on_message({91}, game_in);
    EXPECT_FALSE(handler.player_runtime_snapshot(777));
    ASSERT_FALSE(reply.messages.empty());
    EXPECT_EQ(reply.last_message.header.protocol, static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInNack));
    mxh::db::ResultSet rows;
    ASSERT_TRUE(db.query("SELECT base_gengol,base_minchub,base_cheryuk,base_simmek FROM character_info", {}, rows).ok());
    ASSERT_EQ(rows.rows.size(), 1u);
    for (const auto& cell : rows.rows[0]) EXPECT_TRUE(std::holds_alternative<std::monostate>(cell));
    ASSERT_TRUE(db.execute("UPDATE character_info SET base_gengol=31 WHERE chrid=777", {}).ok());
    handler.on_message({91}, game_in);
    EXPECT_FALSE(handler.player_runtime_snapshot(777));
    ASSERT_TRUE(db.execute("UPDATE character_info SET base_minchub=12,base_cheryuk=18,base_simmek=27 WHERE chrid=777", {}).ok());
    handler.on_message({91}, game_in);
    const auto imported = handler.player_runtime_snapshot(777); ASSERT_TRUE(imported);
    EXPECT_EQ(imported->max_hp, 48u * 5u + 18u * 10u);
}

}  // namespace mxh::server::test
