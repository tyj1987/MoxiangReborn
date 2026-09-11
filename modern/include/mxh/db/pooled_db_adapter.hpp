#pragma once

#include "mxh/db/db_adapter.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <memory>
#include <mutex>
#include <vector>

namespace mxh::db {

// Pool of IDbAdapter instances sharing the same ConnectionConfig.
//
// Why: every backend currently ships a single connection protected by one
// mutex; SQLite serialises through that mutex and MSSQL through its own
// single-handle. Under production load (a MapServer walking every player
// every tick + a LoginServer validating credentials + AccountService
// looking up admins concurrently) one connection is the throughput
// bottleneck.
//
// PooledDbAdapter keeps N underlying adapters pre-opened, hands them out
// via RAII Lease, and runs the standard IDbAdapter calls by acquiring one
// slot per call. Transactions that must span multiple statements use the
// explicit acquire_lease() entry so the same connection is held for the
// whole transaction.
//
// Construction:
//
//   PooledDbAdapter::Config cfg;
//   cfg.backend = "sqlite";
//   cfg.path = "/var/lib/moxian/game.db";
//   cfg.pool_size = 8;
//   auto pool = std::make_shared<PooledDbAdapter>(cfg);
//
// IDbAdapter usage (drop-in):
//
//   pool->query("SELECT 1", out);  // implicit acquire/release
//
// Transaction usage:
//
//   auto lease = pool->acquire_lease();
//   lease->begin_transaction();
//   lease->execute(...);
//   lease->commit();  // lease releases on destruction
//
class PooledDbAdapter final : public IDbAdapter {
public:
    struct Config {
        // Backend name passed to make_adapter(). "sqlite" or "mssql_odbc".
        std::string backend = "sqlite";
        // Pool size. 1 effectively reduces to the single-adapter behaviour
        // and is rejected at construction; the smallest valid pool is 2.
        std::size_t pool_size = 4;
        // Connection details forwarded to the underlying adapter.
        ConnectionConfig connection;
        // Bound on time a caller can wait for a free slot before
        // acquire_lease() throws PooledDbTimeout. 0 = unbounded.
        std::chrono::milliseconds acquire_timeout{5000};
    };

    // Build a pool of `cfg.pool_size` adapters all sharing `cfg.connection`.
    // Returns an error DbResult if zero adapters could be opened; the pool
    // is still constructed with the ones that did open so the caller can
    // log the partial failure. Each adapter is connected immediately so
    // connect()-time failures surface at startup.
    static std::shared_ptr<PooledDbAdapter> create(const Config& cfg);

    // IDbAdapter implementation. Every call acquires a slot, dispatches,
    // and releases. Errors from the underlying adapter are surfaced verbatim
    // with `DbResult.error == DbError::ConnectionFailed` when no slot was
    // available inside the acquire timeout.
    DbResult connect(const ConnectionConfig& cfg) override;
    void disconnect() override;
    bool is_connected() const noexcept override;
    DbResult execute(std::string_view sql,
                     std::span<const Bind> params = {}) override;
    DbResult query(std::string_view sql, std::span<const Bind> params,
                   ResultSet& out) override;
    DbResult query(std::string_view sql, ResultSet& out) {
        return query(sql, std::span<const Bind>{}, out);
    }
    DbResult begin_transaction() override;
    DbResult commit() override;
    DbResult rollback() override;
    std::string backend_name() const noexcept override;

    // RAII handle for transaction-bearing or batch-bearing work.
    class Lease {
    public:
        Lease() noexcept = default;
        Lease(PooledDbAdapter* pool, std::unique_ptr<IDbAdapter> inner,
              std::size_t slot) noexcept;
        Lease(const Lease&) = delete;
        Lease& operator=(const Lease&) = delete;
        Lease(Lease&& other) noexcept;
        Lease& operator=(Lease&& other) noexcept;
        ~Lease();

        IDbAdapter* operator->() const noexcept { return inner_.get(); }
        IDbAdapter& operator*() const noexcept { return *inner_; }
        IDbAdapter* get() const noexcept { return inner_.get(); }
        explicit operator bool() const noexcept { return inner_ != nullptr; }
        // Return the slot back to the pool early. Idempotent; safe to call
        // from a destructor or a partial-failure path.
        void release() noexcept;

        // Underlying connection identity (sequence number); primarily useful
        // for log diagnostics that need to confirm a transaction stayed on
        // one connection.
        std::size_t slot() const noexcept { return slot_; }

    private:
        PooledDbAdapter* pool_ = nullptr;
        std::unique_ptr<IDbAdapter> inner_;
        std::size_t slot_ = 0;
    };

    // Borrow a connection. Blocks until one is free or acquire_timeout
    // elapses. Throws std::runtime_error on timeout; production callers
    // that want a DbResult should wrap the call themselves.
    Lease acquire_lease();

    // Snapshot of pool statistics for the operator console / health endpoint.
    struct Stats {
        std::size_t pool_size = 0;
        std::size_t in_use = 0;
        std::uint64_t total_acquired = 0;
        std::uint64_t total_timed_out = 0;
        std::uint64_t total_lease_failures = 0;
    };
    Stats stats() const;

    ~PooledDbAdapter() override;

private:
    explicit PooledDbAdapter(Config cfg,
                             std::vector<std::unique_ptr<IDbAdapter>> slots);

    Lease try_acquire_locked(std::unique_lock<std::mutex>& lock);

    Config config_;
    mutable std::mutex mu_;
    std::condition_variable cv_;
    std::vector<std::unique_ptr<IDbAdapter>> slots_;
    std::vector<bool> in_use_;
    bool connected_ = false;
    Stats stats_;
};

}  // namespace mxh::db