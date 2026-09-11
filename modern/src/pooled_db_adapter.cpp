#include "mxh/db/pooled_db_adapter.hpp"

#include <stdexcept>
#include <utility>

namespace mxh::db {

namespace {
DbResult timed_out_result() {
    DbResult r;
    r.error = DbError::ConnectionFailed;
    r.error_message = "pool acquire timeout";
    return r;
}
}  // namespace

PooledDbAdapter::Lease::Lease(PooledDbAdapter* pool,
                               std::unique_ptr<IDbAdapter> inner,
                               std::size_t slot) noexcept
    : pool_(pool), inner_(std::move(inner)), slot_(slot) {}

PooledDbAdapter::Lease::Lease(Lease&& other) noexcept
    : pool_(other.pool_), inner_(std::move(other.inner_)), slot_(other.slot_) {
    other.pool_ = nullptr;
}

PooledDbAdapter::Lease& PooledDbAdapter::Lease::operator=(Lease&& other) noexcept {
    if (this != &other) {
        release();
        pool_ = other.pool_;
        inner_ = std::move(other.inner_);
        slot_ = other.slot_;
        other.pool_ = nullptr;
    }
    return *this;
}

PooledDbAdapter::Lease::~Lease() { release(); }

void PooledDbAdapter::Lease::release() noexcept {
    if (!pool_ || !inner_) return;
    {
        std::lock_guard<std::mutex> lock(pool_->mu_);
        if (slot_ < pool_->in_use_.size()) {
            pool_->in_use_[slot_] = false;
            // Return the underlying adapter to its slot so a subsequent
            // acquire finds a non-null IDbAdapter pointer.
            pool_->slots_[slot_] = std::move(inner_);
        }
    }
    pool_->cv_.notify_one();
    pool_ = nullptr;
    slot_ = 0;
}

PooledDbAdapter::PooledDbAdapter(
    Config cfg, std::vector<std::unique_ptr<IDbAdapter>> slots)
    : config_(std::move(cfg)), slots_(std::move(slots)),
      in_use_(slots_.size(), false) {}

PooledDbAdapter::~PooledDbAdapter() {
    // The owning code is expected to call disconnect(); the destructor is
    // best-effort so a partial construction still releases any opened
    // SQLite handles.
    for (auto& slot : slots_) {
        if (slot && slot->is_connected()) slot->disconnect();
    }
}

std::shared_ptr<PooledDbAdapter> PooledDbAdapter::create(const Config& cfg) {
    if (cfg.pool_size < 1) {
        throw std::invalid_argument("PooledDbAdapter requires pool_size >= 1");
    }
    std::vector<std::unique_ptr<IDbAdapter>> slots;
    slots.reserve(cfg.pool_size);
    for (std::size_t i = 0; i < cfg.pool_size; ++i) {
        auto adapter = make_adapter(cfg.backend);
        if (!adapter) {
            throw std::runtime_error("make_adapter returned null for backend: " +
                                     cfg.backend);
        }
        slots.push_back(std::move(adapter));
    }
    auto pool = std::shared_ptr<PooledDbAdapter>(
        new PooledDbAdapter(cfg, std::move(slots)));
    DbResult first_error;
    std::size_t opened = 0;
    for (auto& slot : pool->slots_) {
        const auto r = slot->connect(cfg.connection);
        if (r.ok()) {
            ++opened;
        } else if (!first_error.ok()) {
            first_error = r;
        }
    }
    if (opened == 0) {
        // Surface the first failure; the destructor of pool->slots_ will
        // disconnect anything that did open.
        throw std::runtime_error("PooledDbAdapter failed to open any connection: " +
                                 first_error.error_message);
    }
    if (opened == cfg.pool_size) pool->connected_ = true;
    return pool;
}

DbResult PooledDbAdapter::connect(const ConnectionConfig& cfg) {
    std::lock_guard<std::mutex> lock(mu_);
    DbResult last;
    std::size_t opened = 0;
    for (auto& slot : slots_) {
        const auto r = slot->connect(cfg);
        if (r.ok()) ++opened;
        else last = r;
    }
    config_.connection = cfg;
    connected_ = opened == slots_.size();
    return opened == slots_.size() ? DbResult{} : last;
}

void PooledDbAdapter::disconnect() {
    std::lock_guard<std::mutex> lock(mu_);
    for (auto& slot : slots_) {
        if (slot && slot->is_connected()) slot->disconnect();
    }
    connected_ = false;
    cv_.notify_all();
}

bool PooledDbAdapter::is_connected() const noexcept {
    std::lock_guard<std::mutex> lock(mu_);
    return connected_;
}

PooledDbAdapter::Lease PooledDbAdapter::try_acquire_locked(
    std::unique_lock<std::mutex>& lock) {
    Lease lease;
    auto pred = [this]() {
        for (std::size_t i = 0; i < slots_.size(); ++i) {
            if (!in_use_[i]) return true;
        }
        return false;
    };
    if (config_.acquire_timeout.count() == 0) {
        cv_.wait(lock, pred);
    } else if (!cv_.wait_for(lock, config_.acquire_timeout, pred)) {
        ++stats_.total_timed_out;
        ++stats_.total_lease_failures;
        return lease;
    }
    for (std::size_t i = 0; i < slots_.size(); ++i) {
        if (!in_use_[i]) {
            in_use_[i] = true;
            ++stats_.total_acquired;
            lease = Lease(this, std::move(slots_[i]), i);
            // Move the slot pointer out so subsequent acquires don't
            // observe it; on release() we move it back. This keeps the
            // connection alive across the lease lifetime.
            slots_[i] = nullptr;
            return lease;
        }
    }
    // Unreachable; pred() would have blocked otherwise.
    ++stats_.total_lease_failures;
    return lease;
}

PooledDbAdapter::Lease PooledDbAdapter::acquire_lease() {
    std::unique_lock<std::mutex> lock(mu_);
    return try_acquire_locked(lock);
}

DbResult PooledDbAdapter::execute(std::string_view sql,
                                  std::span<const Bind> params) {
    auto lease = acquire_lease();
    if (!lease) return timed_out_result();
    return (*lease).execute(sql, params);
}

DbResult PooledDbAdapter::query(std::string_view sql,
                                std::span<const Bind> params, ResultSet& out) {
    auto lease = acquire_lease();
    if (!lease) return timed_out_result();
    return (*lease).query(sql, params, out);
}

DbResult PooledDbAdapter::begin_transaction() {
    auto lease = acquire_lease();
    if (!lease) return timed_out_result();
    return (*lease).begin_transaction();
}

DbResult PooledDbAdapter::commit() {
    auto lease = acquire_lease();
    if (!lease) return timed_out_result();
    return (*lease).commit();
}

DbResult PooledDbAdapter::rollback() {
    auto lease = acquire_lease();
    if (!lease) return timed_out_result();
    return (*lease).rollback();
}

std::string PooledDbAdapter::backend_name() const noexcept {
    return config_.backend;
}

PooledDbAdapter::Stats PooledDbAdapter::stats() const {
    std::lock_guard<std::mutex> lock(mu_);
    auto s = stats_;
    s.pool_size = slots_.size();
    for (bool busy : in_use_) if (busy) ++s.in_use;
    return s;
}

}  // namespace mxh::db