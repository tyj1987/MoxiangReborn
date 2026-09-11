// Tests for the modern_player_position schema + the persist
// SQL used by MapHandler::persist_player_position. Kept in db tests
// because they only exercise SQL, not the network/handler layer.

#include "mxh/db/db_adapter.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>

#if defined(_WIN32)
#include <process.h>
#define MXH_TEST_GETPID() ::_getpid()
#else
#include <unistd.h>
#define MXH_TEST_GETPID() ::getpid()
#endif

using namespace mxh::db;
namespace {

std::atomic<int> counter{0};
std::unique_ptr<IDbAdapter> open_test_db() {
    const auto path = (std::filesystem::temp_directory_path() /
                       ("mxh_pos_test_" + std::to_string(MXH_TEST_GETPID()) +
                        "_" + std::to_string(counter.fetch_add(1)) + ".db"))
                        .string();
    std::error_code ec;
    std::filesystem::remove(path, ec);
    ConnectionConfig cfg;
    cfg.backend = "sqlite";
    cfg.path = path;
    auto db = make_adapter("sqlite");
    EXPECT_TRUE(db->connect(cfg).ok());
    EXPECT_TRUE(db->execute(
        "CREATE TABLE modern_player_position ("
        "player_id INTEGER PRIMARY KEY, map_num INTEGER NOT NULL, "
        "pos_x INTEGER NOT NULL, pos_z INTEGER NOT NULL, "
        "updated_at TEXT NOT NULL)").ok());
    return db;
}

bool persist_position(IDbAdapter& db, std::uint32_t player_id,
                      std::uint16_t map_num, std::uint16_t x, std::uint16_t z) {
    const std::vector<mxh::db::Bind> params{
        mxh::db::bind(static_cast<std::int64_t>(player_id)),
        mxh::db::bind(static_cast<std::int64_t>(map_num)),
        mxh::db::bind(static_cast<std::int64_t>(x)),
        mxh::db::bind(static_cast<std::int64_t>(z))};
    return db.execute(
        "INSERT INTO modern_player_position "
        "(player_id, map_num, pos_x, pos_z, updated_at) "
        "VALUES (?, ?, ?, ?, strftime('%Y-%m-%dT%H:%M:%fZ', 'now')) "
        "ON CONFLICT (player_id) DO UPDATE SET "
        "map_num = excluded.map_num, pos_x = excluded.pos_x, "
        "pos_z = excluded.pos_z, updated_at = excluded.updated_at",
        params).ok();
}

struct Position { std::int64_t map_num; std::int64_t x; std::int64_t z; };
std::optional<Position> read_position(IDbAdapter& db, std::uint32_t player_id) {
    ResultSet rs;
    if (!db.query(
            "SELECT map_num, pos_x, pos_z FROM modern_player_position WHERE player_id = ?",
            {mxh::db::bind(static_cast<std::int64_t>(player_id))}, rs).ok()) {
        return std::nullopt;
    }
    if (rs.empty()) return std::nullopt;
    const auto& row = rs.rows[0];
    return Position{std::get<std::int64_t>(row[0]),
                    std::get<std::int64_t>(row[1]),
                    std::get<std::int64_t>(row[2])};
}
}  // namespace

TEST(PlayerPositionPersistence, InsertCreatesRow) {
    auto db = open_test_db();
    ASSERT_TRUE(persist_position(*db, 42u, 10, 100, 200));
    auto p = read_position(*db, 42u);
    ASSERT_TRUE(p.has_value());
    EXPECT_EQ(p->map_num, 10);
    EXPECT_EQ(p->x, 100);
    EXPECT_EQ(p->z, 200);
}

TEST(PlayerPositionPersistence, SecondWriteUpsertsSameRow) {
    auto db = open_test_db();
    ASSERT_TRUE(persist_position(*db, 7u, 1, 10, 10));
    ASSERT_TRUE(persist_position(*db, 7u, 2, 100, 200));
    ResultSet rs;
    ASSERT_TRUE(db->query("SELECT COUNT(*) FROM modern_player_position", {}, rs).ok());
    EXPECT_EQ(std::get<std::int64_t>(rs.rows[0][0]), 1);
    auto p = read_position(*db, 7u);
    ASSERT_TRUE(p.has_value());
    EXPECT_EQ(p->map_num, 2);
    EXPECT_EQ(p->x, 100);
    EXPECT_EQ(p->z, 200);
}

TEST(PlayerPositionPersistence, IndependentPlayersDoNotCollide) {
    auto db = open_test_db();
    ASSERT_TRUE(persist_position(*db, 1u, 0, 0, 0));
    ASSERT_TRUE(persist_position(*db, 2u, 1, 1, 1));
    ASSERT_TRUE(persist_position(*db, 3u, 2, 2, 2));
    EXPECT_EQ(read_position(*db, 1u)->map_num, 0);
    EXPECT_EQ(read_position(*db, 2u)->map_num, 1);
    EXPECT_EQ(read_position(*db, 3u)->map_num, 2);
}

TEST(PlayerPositionPersistence, GracefulShutdownSweepWritesAllConnectedRows) {
    auto db = open_test_db();
    // Simulate the bounded checkpoint loop used by
    // MapHandler::persist_all_connected_player_positions().
    const std::uint32_t ids[] = {101u, 202u, 303u, 404u};
    for (const auto id : ids) ASSERT_TRUE(persist_position(*db, id, 5, 10, 20));
    ResultSet rs;
    ASSERT_TRUE(db->query("SELECT COUNT(*) FROM modern_player_position", {}, rs).ok());
    EXPECT_EQ(std::get<std::int64_t>(rs.rows[0][0]), 4);
    for (const auto id : ids) {
        auto p = read_position(*db, id);
        ASSERT_TRUE(p.has_value());
        EXPECT_EQ(p->map_num, 5);
    }
}