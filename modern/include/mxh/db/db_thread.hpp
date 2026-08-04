#pragma once

#include <atomic>
#include <cstddef>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <mxh/db/db_adapter.hpp>
#include <mxh/util/thread_pool.hpp>

namespace mxh::db {

struct DbQueryResult final {
    DbResult status{};
    ResultSet result{};
};

class DbThread final {
public:
    explicit DbThread(std::unique_ptr<IDbAdapter> adapter,
                      std::size_t worker_count = 1);
    ~DbThread();

    DbThread(const DbThread&) = delete;
    DbThread& operator=(const DbThread&) = delete;

    [[nodiscard]] DbResult connect(const ConnectionConfig& cfg);
    [[nodiscard]] std::future<DbResult> execute_async(
        std::string sql, std::vector<Bind> params = {});
    [[nodiscard]] std::future<DbQueryResult> query_async(
        std::string sql, std::vector<Bind> params = {});

    void shutdown();
    [[nodiscard]] bool is_running() const noexcept;
    [[nodiscard]] std::size_t pending_tasks();
    [[nodiscard]] std::size_t active_tasks() const noexcept;
    [[nodiscard]] std::string backend_name() const;

private:
    std::unique_ptr<IDbAdapter> adapter_;
    mxh::util::ThreadPool workers_;
    std::atomic<bool> stopping_{false};
    mutable std::mutex lifecycle_mu_;
};

}  // namespace mxh::db
