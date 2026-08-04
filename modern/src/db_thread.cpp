#include <mxh/db/db_thread.hpp>

#include <utility>

namespace mxh::db {

namespace {

DbResult stopped_result() {
    DbResult result;
    result.error = DbError::NotConnected;
    return result;
}

std::future<DbResult> ready_execute_future(DbResult result) {
    std::promise<DbResult> promise;
    auto future = promise.get_future();
    promise.set_value(std::move(result));
    return future;
}

std::future<DbQueryResult> ready_query_future(DbResult result) {
    std::promise<DbQueryResult> promise;
    auto future = promise.get_future();
    DbQueryResult query;
    query.status = std::move(result);
    promise.set_value(std::move(query));
    return future;
}

}  // namespace

DbThread::DbThread(std::unique_ptr<IDbAdapter> adapter, std::size_t worker_count)
    : adapter_(std::move(adapter)), workers_(worker_count) {}

DbThread::~DbThread() {
    shutdown();
}

DbResult DbThread::connect(const ConnectionConfig& cfg) {
    std::lock_guard<std::mutex> lk(lifecycle_mu_);
    if (stopping_.load() || !adapter_) return stopped_result();
    return adapter_->connect(cfg);
}

std::future<DbResult> DbThread::execute_async(
    std::string sql, std::vector<Bind> params) {
    std::lock_guard<std::mutex> lk(lifecycle_mu_);
    if (stopping_.load() || !adapter_) {
        return ready_execute_future(stopped_result());
    }
    return workers_.enqueue_with_future(
        [this, sql = std::move(sql), params = std::move(params)]() mutable {
            return adapter_->execute(sql, params);
        });
}

std::future<DbQueryResult> DbThread::query_async(
    std::string sql, std::vector<Bind> params) {
    std::lock_guard<std::mutex> lk(lifecycle_mu_);
    if (stopping_.load() || !adapter_) {
        return ready_query_future(stopped_result());
    }
    return workers_.enqueue_with_future(
        [this, sql = std::move(sql), params = std::move(params)]() mutable {
            DbQueryResult result;
            result.status = adapter_->query(sql, params, result.result);
            return result;
        });
}

void DbThread::shutdown() {
    std::lock_guard<std::mutex> lk(lifecycle_mu_);
    if (stopping_.exchange(true)) return;
    workers_.shutdown();
    if (adapter_) adapter_->disconnect();
}

bool DbThread::is_running() const noexcept {
    std::lock_guard<std::mutex> lk(lifecycle_mu_);
    return !stopping_.load() && adapter_ && adapter_->is_connected();
}

std::size_t DbThread::pending_tasks() {
    return workers_.pending_tasks();
}

std::size_t DbThread::active_tasks() const noexcept {
    return workers_.active_tasks();
}

std::string DbThread::backend_name() const {
    std::lock_guard<std::mutex> lk(lifecycle_mu_);
    return adapter_ ? adapter_->backend_name() : std::string{};
}

}  // namespace mxh::db
