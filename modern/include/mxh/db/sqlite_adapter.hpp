// sqlite_adapter.hpp - SQLite implementation of IDbAdapter.
//
// SQLite is the default development backend because it requires zero
// configuration: just point at a .db file. The schema can be migrated
// from MSSQL .bak files via the dedicated migration tool.

#pragma once

#include "mxh/db/db_adapter.hpp"

#include <sqlite3.h>

#include <mutex>

namespace mxh::db {

class SqliteAdapter final : public IDbAdapter {
public:
    SqliteAdapter();
    ~SqliteAdapter() override;

    SqliteAdapter(const SqliteAdapter&) = delete;
    SqliteAdapter& operator=(const SqliteAdapter&) = delete;

    [[nodiscard]] DbResult connect(const ConnectionConfig& cfg) override;
    void disconnect() override;
    [[nodiscard]] bool is_connected() const noexcept override { return db_ != nullptr; }

    [[nodiscard]] const ConnectionConfig& config() const noexcept { return cfg_; }

    [[nodiscard]] DbResult execute(std::string_view sql,
                                   std::span<const Bind> params) override;
    [[nodiscard]] DbResult query(std::string_view sql,
                                 std::span<const Bind> params,
                                 ResultSet& out) override;

    // Execute multi-statement SQL (e.g. schema bootstrap).
    [[nodiscard]] DbResult exec_multi(std::string_view sql);

    [[nodiscard]] DbResult begin_transaction() override;
    [[nodiscard]] DbResult commit() override;
    [[nodiscard]] DbResult rollback() override;

    [[nodiscard]] std::string backend_name() const noexcept override { return "sqlite"; }

    // Direct access (escape hatch).
    [[nodiscard]] sqlite3* raw() const noexcept { return db_; }

private:
    ConnectionConfig cfg_{};
    sqlite3* db_ = nullptr;
    std::mutex mu_;  // SQLite is not thread-safe by default
    DbResult translate_error(int rc, const char* errmsg);
};

}  // namespace mxh::db