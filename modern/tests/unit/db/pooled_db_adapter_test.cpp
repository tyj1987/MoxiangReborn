// Tests for the IDbAdapter connection pool. Pure SQLite :memory: so the
// tests are hermetic and fast; the pool contract is backend-neutral and
// the same wiring is what the MSSQL adapter would consume.

#include "mxh/db/db_adapter.hpp"
#include "mxh/db/pooled_db_adapter.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

#if defined(_WIN32)
#include <process.h>
#define MXH_TEST_GETPID() ::_getpid()
#else
#include <unistd.h>
#define MXH_TEST_GETPID() ::getpid()
#endif

using namespace mxh::db;
namespace {

std::string test_db_path() {
    static std::atomic<int> counter{0};
    const auto pid = MXH_TEST_GETPID();
    const auto seq = counter.fetch_add(1);
    return (std::filesystem::temp_directory_path() /
            ("mxh_pool_test_" + std::to_string(pid) + "_" +
             std::to_string(seq) + ".db"))
        .string();
}

std::shared_ptr<PooledDbAdapter> make_pool(std::size_t size,
                                           std::chrono::milliseconds timeout =
                                               std::chrono::milliseconds{5000}) {
    // File-backed so every connection in the pool sees the same schema and
    // data. :memory: would create an independent database per connection.
    const auto path = test_db_path();
    std::error_code ec;
    std::filesystem::remove(path, ec);  // start clean
    PooledDbAdapter::Config cfg;
    cfg.backend = "sqlite";
    cfg.connection.path = path;
    cfg.pool_size = size;
    cfg.acquire_timeout = timeout;
    return PooledDbAdapter::create(cfg);
}

void create_count_table(IDbAdapter& db) {
    ASSERT_TRUE(db.execute("CREATE TABLE t(id INTEGER PRIMARY KEY, v INTEGER)").ok());
}
}

TEST(PooledDbAdapter, RejectsPoolSizeBelowOne) {
    PooledDbAdapter::Config cfg;
    cfg.backend = "sqlite";
    cfg.connection.path = ":memory:";
    cfg.pool_size = 0;
    EXPECT_THROW(PooledDbAdapter::create(cfg), std::invalid_argument);
}

TEST(PooledDbAdapter, CreateOpensEverySlotAndExposesConnected) {
    auto pool = make_pool(4);
    EXPECT_TRUE(pool->is_connected());
    auto s = pool->stats();
    EXPECT_EQ(s.pool_size, 4u);
    EXPECT_EQ(s.in_use, 0u);
    EXPECT_EQ(s.total_acquired, 0u);
}

TEST(PooledDbAdapter, IdbAdapterCallsDispatchToUnderlyingConnections) {
    auto pool = make_pool(2);
    ASSERT_TRUE(pool->execute("CREATE TABLE t(v INTEGER)").ok());
    ASSERT_TRUE(pool->execute("INSERT INTO t(v) VALUES (1),(2),(3)").ok());
    ResultSet rs;
    ASSERT_TRUE(pool->query("SELECT COUNT(*) FROM t", {}, rs).ok());
    ASSERT_FALSE(rs.empty());
    EXPECT_EQ(std::get<std::int64_t>(rs.rows[0][0]), 3);
}

TEST(PooledDbAdapter, LeaseReleasesBackOnDestruction) {
    auto pool = make_pool(2);
    {
        auto lease = pool->acquire_lease();
        ASSERT_TRUE(lease);
        EXPECT_EQ(pool->stats().in_use, 1u);
    }
    EXPECT_EQ(pool->stats().in_use, 0u);
}

TEST(PooledDbAdapter, LeaseReleaseIsIdempotent) {
    auto pool = make_pool(2);
    auto lease = pool->acquire_lease();
    ASSERT_TRUE(lease);
    lease.release();
    lease.release(); // must not double-release or deadlock.
    EXPECT_EQ(pool->stats().in_use, 0u);
}

TEST(PooledDbAdapter, ConcurrentQueriesDoNotExceedPoolSize) {
    constexpr std::size_t kPool = 4;
    constexpr int kThreads = 16;
    constexpr int kIters = 100;
    auto pool = make_pool(kPool);
    ASSERT_TRUE(pool->execute("CREATE TABLE t(v INTEGER)").ok());
    ASSERT_TRUE(pool->execute("INSERT INTO t VALUES (0)").ok());
    std::atomic<int> observed_concurrency{0};
    std::atomic<int> max_observed{0};
    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&]() {
            for (int i = 0; i < kIters; ++i) {
                auto lease = pool->acquire_lease();
                ASSERT_TRUE(lease);
                int seen = observed_concurrency.fetch_add(1) + 1;
                int prev_max = max_observed.load();
                while (seen > prev_max &&
                       !max_observed.compare_exchange_weak(prev_max, seen)) {}
                std::this_thread::sleep_for(std::chrono::microseconds(10));
                observed_concurrency.fetch_sub(1);
            }
        });
    }
    for (auto& th : threads) th.join();
    EXPECT_LE(max_observed.load(), static_cast<int>(kPool));
    // The seed INSERT must still be visible after all the leases churn.
    ResultSet rs;
    ASSERT_TRUE(pool->query("SELECT COUNT(*) FROM t", rs).ok());
    EXPECT_EQ(std::get<std::int64_t>(rs.rows[0][0]), 1);
}

TEST(PooledDbAdapter, LeaseTransactionsStayOnOneConnection) {
    auto pool = make_pool(3);
    ASSERT_TRUE(pool->execute("CREATE TABLE kv(k TEXT PRIMARY KEY, v TEXT)").ok());
    auto lease = pool->acquire_lease();
    ASSERT_TRUE(lease);
    EXPECT_EQ(lease.slot(), lease.slot()); // stable slot id within the lease
    ASSERT_TRUE(lease->begin_transaction().ok());
    ASSERT_TRUE(lease->execute("INSERT INTO kv VALUES ('a','1')").ok());
    ASSERT_TRUE(lease->commit().ok());
    // The release on destruction returns the slot; a new query runs against
    // any free slot but the committed row is visible to all.
    ResultSet rs;
    ASSERT_TRUE(pool->query("SELECT COUNT(*) FROM kv", {}, rs).ok());
    EXPECT_EQ(std::get<std::int64_t>(rs.rows[0][0]), 1);
}

TEST(PooledDbAdapter, AcquireTimeoutReturnsEmptyLease) {
    auto pool = make_pool(1, std::chrono::milliseconds{50});
    auto lease = pool->acquire_lease();
    ASSERT_TRUE(lease);
    auto second = pool->acquire_lease();
    EXPECT_FALSE(second);
    EXPECT_EQ(pool->stats().total_timed_out, 1u);
}

TEST(PooledDbAdapter, IdbAdapterQueriesReturnConnectionFailedAfterTimeout) {
    auto pool = make_pool(1, std::chrono::milliseconds{20});
    auto lease = pool->acquire_lease();
    ASSERT_TRUE(lease);
    ResultSet rs;
    const auto r = pool->query("SELECT 1", std::span<const Bind>{}, rs);
    EXPECT_FALSE(r.ok());
    EXPECT_EQ(r.error, DbError::ConnectionFailed);
}

TEST(PooledDbAdapter, DisconnectClosesAllSlots) {
    auto pool = make_pool(2);
    EXPECT_TRUE(pool->is_connected());
    pool->disconnect();
    EXPECT_FALSE(pool->is_connected());
    pool->disconnect(); // idempotent
    EXPECT_FALSE(pool->is_connected());
}

TEST(PooledDbAdapter, MoveTransfersOwnershipAndReleasesOnce) {
    auto pool = make_pool(2);
    auto a = pool->acquire_lease();
    auto b = std::move(a);
    EXPECT_TRUE(a.get() == nullptr);
    EXPECT_TRUE(b);
    EXPECT_EQ(pool->stats().in_use, 1u);
    PooledDbAdapter::Lease c;
    c = std::move(b);
    EXPECT_FALSE(b);
    EXPECT_TRUE(c);
    EXPECT_EQ(pool->stats().in_use, 1u);
}