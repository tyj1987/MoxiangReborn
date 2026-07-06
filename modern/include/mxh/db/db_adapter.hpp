// db_adapter.hpp - Modern database abstraction layer.
//
// Phase 2 of Moxian-Reborn modernization:
// Replace [Lib]DBThread ODBC-only layer with a portable IDbAdapter interface
// that supports multiple backends while preserving 1:1 SQL semantics.
//
// Backends:
//   - mxh_db_sqlite   (default; zero-config, file-based, immediate)
//   - mxh_db_mssql_odbc (optional; native compatibility with .bak restoration)
//
// All backends expose the same interface so the rest of the codebase doesn't
// need to know which DB it's talking to.

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace mxh::db {

// Connection parameters (backend-agnostic where possible).
struct ConnectionConfig {
    std::string backend = "sqlite";        // "sqlite" | "mssql_odbc"
    std::string path;                      // SQLite: file path. MSSQL: ODBC DSN name.
    std::string host = "localhost";        // MSSQL only
    std::uint16_t port = 1433;             // MSSQL only
    std::string database;                  // MSSQL only
    std::string user;                      // MSSQL only
    std::string password;                  // MSSQL only

    // Construct from a simple "key=value" string for tooling.
    static ConnectionConfig from_kv_string(std::string_view s);
    [[nodiscard]] std::string to_kv_string() const;
};

// Row = ordered list of values. NULL is represented as std::monostate.
using Value = std::variant<std::monostate, std::int64_t, double, std::string,
                           std::vector<std::uint8_t>>;
using Row = std::vector<Value>;

// Result set returned by SELECT queries.
struct ResultSet {
    std::vector<std::string> columns;
    std::vector<Row> rows;

    [[nodiscard]] std::size_t size() const noexcept { return rows.size(); }
    [[nodiscard]] bool empty() const noexcept { return rows.empty(); }

    // Convenience: get column index by name (case-insensitive).
    [[nodiscard]] int column_index(std::string_view name) const;

    // Convenience: get value at row, column.
    [[nodiscard]] const Value& at(std::size_t row, std::size_t col) const {
        return rows.at(row).at(col);
    }
};

// Errors returned by all operations.
enum class DbError {
    Ok = 0,
    NotConnected,
    ConnectionFailed,
    QuerySyntaxError,
    ConstraintViolation,
    NoSuchTable,
    IoError,
    NotImplemented,
    Unknown,
};

[[nodiscard]] const char* to_string(DbError e) noexcept;

struct DbResult {
    DbError error = DbError::Ok;
    std::string error_message;
    std::int64_t rows_affected = 0;
    std::int64_t last_insert_id = 0;

    [[nodiscard]] bool ok() const noexcept { return error == DbError::Ok; }
    [[nodiscard]] explicit operator bool() const noexcept { return ok(); }
};

// Bind parameter for prepared statements (positional).
struct Bind {
    Value value;
};

// Abstract database adapter.
class IDbAdapter {
public:
    virtual ~IDbAdapter() = default;

    // Lifecycle.
    [[nodiscard]] virtual DbResult connect(const ConnectionConfig& cfg) = 0;
    virtual void disconnect() = 0;
    [[nodiscard]] virtual bool is_connected() const noexcept = 0;

    // Execute a statement that returns no rows (INSERT/UPDATE/DELETE/DDL).
    [[nodiscard]] virtual DbResult execute(std::string_view sql,
                                           std::span<const Bind> params = {}) = 0;

    // Execute a query and return rows.
    [[nodiscard]] virtual DbResult query(std::string_view sql,
                                         std::span<const Bind> params,
                                         ResultSet& out) = 0;

    // Convenience overload: query with no params.
    [[nodiscard]] DbResult query(std::string_view sql, ResultSet& out) {
        return query(sql, {}, out);
    }

    // Transaction support.
    [[nodiscard]] virtual DbResult begin_transaction() = 0;
    [[nodiscard]] virtual DbResult commit() = 0;
    [[nodiscard]] virtual DbResult rollback() = 0;

    // Backend-specific escape hatch.
    [[nodiscard]] virtual std::string backend_name() const noexcept = 0;
};

// Factory: create an adapter for a given backend.
[[nodiscard]] std::unique_ptr<IDbAdapter> make_adapter(std::string_view backend);

// Helpers for binding.
inline Bind bind() { return {}; }
inline Bind bind(std::int64_t v) { return {v}; }
inline Bind bind(int v) { return {static_cast<std::int64_t>(v)}; }
inline Bind bind(double v) { return {v}; }
inline Bind bind(const std::string& v) { return {v}; }
inline Bind bind(std::vector<std::uint8_t> v) { return {std::move(v)}; }
inline Bind bind_null() { return {}; }

}  // namespace mxh::db