// Real-resource integration test: read MonsterList.bin content into SQLite.

#include "mxh/compat/mh_file_ex.hpp"
#include "mxh/compat/pack_file.hpp"
#include "mxh/db/db_adapter.hpp"
#include "mxh/db/sqlite_adapter.hpp"

#include <gtest/gtest.h>

#include <windows.h>

#include <filesystem>

using namespace mxh;
using namespace mxh::db;
using namespace mxh::compat;

namespace {

// Hardcoded project paths (workspace is fixed at D:\Moxian).
const std::filesystem::path kPlaydhBin = LR"(D:\Moxian\墨香【源码配套资源】\PlayDH\Resource\MonsterList.bin)";
const std::filesystem::path kPlaydhPak = LR"(D:\Moxian\墨香【源码配套资源】\PlayDH\Effect.pak)";

}  // namespace

TEST(RealResource, MonsterListIntoSqlite) {
    if (!std::filesystem::exists(kPlaydhBin)) GTEST_SKIP() << "MonsterList.bin not found";

    // 1. Read .bin via modern compat layer.
    auto file = read_mh_bin(kPlaydhBin);
    ASSERT_TRUE(file.ok()) << "Failed to read MonsterList.bin";
    ASSERT_GT(file.value.data.size(), 100u);

    // 2. Set up an in-memory DB with a monsters table.
    auto db = make_adapter("sqlite");
    ConnectionConfig cfg;
    cfg.path = ":memory:";
    ASSERT_TRUE(db->connect(cfg).ok());

    db->execute(R"(CREATE TABLE monster_bin (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        raw BLOB NOT NULL,
        byte_count INTEGER NOT NULL
    ))");

    // Decoded bytes contain EUC-KR; store as BLOB to preserve bytes.
    std::vector<mxh::db::Bind> params{
        mxh::db::bind(file.value.data),
        mxh::db::bind(static_cast<std::int64_t>(file.value.data.size()))
    };
    auto er = db->execute("INSERT INTO monster_bin (raw, byte_count) VALUES (?, ?)", params);
    ASSERT_TRUE(er.ok()) << er.error_message;

    // 3. Verify.
    ResultSet rs;
    ASSERT_TRUE(db->query("SELECT byte_count, length(raw) FROM monster_bin", rs).ok());
    ASSERT_EQ(rs.rows.size(), 1u);
    EXPECT_EQ(std::get<std::int64_t>(rs.rows[0][0]), file.value.data.size());
    EXPECT_EQ(std::get<std::int64_t>(rs.rows[0][1]), file.value.data.size());
}

TEST(RealResource, PakFileListingToDb) {
    if (!std::filesystem::exists(kPlaydhPak)) GTEST_SKIP() << "Effect.pak not found";

    auto pack = compat::PackFile::open(kPlaydhPak);
    ASSERT_NE(pack, nullptr);

    auto db = make_adapter("sqlite");
    ConnectionConfig cfg;
    cfg.path = ":memory:";
    ASSERT_TRUE(db->connect(cfg).ok());

    db->execute(R"(CREATE TABLE pak_entries (
        name TEXT PRIMARY KEY,
        size INTEGER NOT NULL,
        offset INTEGER NOT NULL
    ))");

    ASSERT_TRUE(db->begin_transaction().ok());
    int inserted = 0;
    for (const auto& e : pack->entries()) {
        std::vector<Bind> params{
            mxh::db::bind(e.name),
            mxh::db::bind(static_cast<std::int64_t>(e.size)),
            mxh::db::bind(static_cast<std::int64_t>(e.entry_offset))
        };
        auto r = db->execute(
            "INSERT INTO pak_entries (name, size, offset) VALUES (?, ?, ?)",
            params);
        if (r.ok()) inserted++;
    }
    ASSERT_TRUE(db->commit().ok());

    ResultSet rs;
    db->query("SELECT COUNT(*) FROM pak_entries", rs);
    EXPECT_EQ(std::get<std::int64_t>(rs.rows[0][0]), inserted);
    EXPECT_GT(inserted, 1000) << "Effect.pak should have many entries";
}