#include <mxh/db/db_thread.hpp>
#include <mxh/db/db_adapter.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <string>
#include <variant>
#include <vector>

using namespace mxh::db;

namespace {

ConnectionConfig memory_config() {
    ConnectionConfig cfg;
    cfg.backend = "sqlite";
    cfg.path = ":memory:";
    return cfg;
}

}  // namespace

TEST(DbThread, AsyncExecuteQueryAndShutdown) {
    DbThread db(make_adapter("sqlite"), 2);
    ASSERT_TRUE(db.connect(memory_config()));
    ASSERT_EQ(db.backend_name(), "sqlite");

    ASSERT_TRUE(db.execute_async("CREATE TABLE items (id INTEGER, name TEXT)").get());
    std::vector<Bind> insert_params;
    insert_params.push_back(mxh::db::bind(static_cast<std::int64_t>(42)));
    insert_params.push_back(mxh::db::bind(std::string("alpha")));
    ASSERT_TRUE(db.execute_async(
        "INSERT INTO items (id, name) VALUES (?, ?)",
        insert_params).get());

    std::vector<Bind> query_params;
    query_params.push_back(mxh::db::bind(static_cast<std::int64_t>(42)));
    const auto query = db.query_async(
        "SELECT id, name FROM items WHERE id = ?",
        query_params).get();
    ASSERT_TRUE(query.status);
    ASSERT_EQ(query.result.size(), 1u);
    ASSERT_EQ(std::get<std::int64_t>(query.result.at(0, 0)), 42);
    ASSERT_EQ(std::get<std::string>(query.result.at(0, 1)), "alpha");
    EXPECT_TRUE(db.is_running());

    db.shutdown();
    db.shutdown();
    EXPECT_FALSE(db.is_running());
}

TEST(DbThread, AsyncOperationAfterShutdownFailsImmediately) {
    DbThread db(make_adapter("sqlite"), 1);
    db.shutdown();

    const auto result = db.execute_async("SELECT 1").get();
    EXPECT_FALSE(result);
    EXPECT_EQ(result.error, DbError::NotConnected);

    const auto query = db.query_async("SELECT 1").get();
    EXPECT_FALSE(query.status);
    EXPECT_EQ(query.status.error, DbError::NotConnected);
}
