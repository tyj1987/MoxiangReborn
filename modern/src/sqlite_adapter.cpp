// sqlite_adapter.cpp - SQLite implementation of IDbAdapter.

#include "mxh/db/sqlite_adapter.hpp"

#include <cstring>
#include <utility>

namespace mxh::db {

SqliteAdapter::SqliteAdapter() = default;

SqliteAdapter::~SqliteAdapter() {
    disconnect();
}

DbResult SqliteAdapter::connect(const ConnectionConfig& cfg) {
    std::lock_guard<std::mutex> lk(mu_);
    DbResult r;
    cfg_ = cfg;
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
    // Translate forward slashes to backslashes on Windows for SQLite.
    std::string path = cfg.path;
#ifdef _WIN32
    for (auto& c : path) if (c == '/') c = '\\';
#endif
    int rc = sqlite3_open(path.c_str(), &db_);
    if (rc != SQLITE_OK) {
        r.error = DbError::ConnectionFailed;
        r.error_message = db_ ? sqlite3_errmsg(db_) : "unknown";
        if (db_) { sqlite3_close(db_); db_ = nullptr; }
        return r;
    }
    // Foreign keys, WAL mode for better concurrency.
    sqlite3_exec(db_, "PRAGMA foreign_keys=ON;", nullptr, nullptr, nullptr);
    sqlite3_exec(db_, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
    return r;
}

void SqliteAdapter::disconnect() {
    std::lock_guard<std::mutex> lk(mu_);
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

DbResult SqliteAdapter::translate_error(int rc, const char* errmsg) {
    DbResult r;
    if (rc == SQLITE_OK) return r;
    r.error_message = errmsg ? errmsg : sqlite3_errstr(rc);
    switch (rc) {
        case SQLITE_BUSY: r.error = DbError::IoError; break;
        case SQLITE_LOCKED: r.error = DbError::IoError; break;
        case SQLITE_READONLY: r.error = DbError::IoError; break;
        case SQLITE_IOERR: r.error = DbError::IoError; break;
        case SQLITE_CORRUPT: r.error = DbError::IoError; break;
        case SQLITE_FULL: r.error = DbError::IoError; break;
        case SQLITE_CANTOPEN: r.error = DbError::ConnectionFailed; break;
        case SQLITE_PROTOCOL: r.error = DbError::IoError; break;
        case SQLITE_SCHEMA: r.error = DbError::NoSuchTable; break;
        case SQLITE_TOOBIG: r.error = DbError::IoError; break;
        case SQLITE_CONSTRAINT: r.error = DbError::ConstraintViolation; break;
        case SQLITE_MISMATCH: r.error = DbError::QuerySyntaxError; break;
        case SQLITE_MISUSE: r.error = DbError::QuerySyntaxError; break;
        case SQLITE_NOLFS: r.error = DbError::IoError; break;
        case SQLITE_AUTH: r.error = DbError::NotImplemented; break;
        case SQLITE_FORMAT: r.error = DbError::QuerySyntaxError; break;
        case SQLITE_RANGE: r.error = DbError::QuerySyntaxError; break;
        case SQLITE_NOTADB: r.error = DbError::ConnectionFailed; break;
        default: r.error = DbError::Unknown; break;
    }
    return r;
}

namespace {

void bind_value(sqlite3_stmt* stmt, int idx, const Value& v) {
    std::visit([stmt, idx](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            sqlite3_bind_null(stmt, idx);
        } else if constexpr (std::is_same_v<T, std::int64_t>) {
            sqlite3_bind_int64(stmt, idx, arg);
        } else if constexpr (std::is_same_v<T, double>) {
            sqlite3_bind_double(stmt, idx, arg);
        } else if constexpr (std::is_same_v<T, std::string>) {
            sqlite3_bind_text(stmt, idx, arg.c_str(),
                              static_cast<int>(arg.size()), SQLITE_TRANSIENT);
        } else if constexpr (std::is_same_v<T, std::vector<std::uint8_t>>) {
            if (arg.empty()) {
                sqlite3_bind_zeroblob(stmt, idx, 0);
            } else {
                sqlite3_bind_blob(stmt, idx, arg.data(),
                                  static_cast<int>(arg.size()), SQLITE_TRANSIENT);
            }
        }
    }, v);
}

Value read_column(sqlite3_stmt* stmt, int col) {
    switch (sqlite3_column_type(stmt, col)) {
        case SQLITE_INTEGER:
            return static_cast<std::int64_t>(sqlite3_column_int64(stmt, col));
        case SQLITE_FLOAT:
            return sqlite3_column_double(stmt, col);
        case SQLITE_TEXT: {
            const auto* p = reinterpret_cast<const char*>(sqlite3_column_text(stmt, col));
            int n = sqlite3_column_bytes(stmt, col);
            return std::string(p, n);
        }
        case SQLITE_BLOB: {
            const auto* p = static_cast<const std::uint8_t*>(sqlite3_column_blob(stmt, col));
            int n = sqlite3_column_bytes(stmt, col);
            return std::vector<std::uint8_t>(p, p + n);
        }
        case SQLITE_NULL:
        default:
            return std::monostate{};
    }
}

}  // namespace

DbResult SqliteAdapter::execute(std::string_view sql,
                                std::span<const Bind> params) {
    std::lock_guard<std::mutex> lk(mu_);
    DbResult r;
    if (!db_) { r.error = DbError::NotConnected; return r; }

    sqlite3_stmt* stmt = nullptr;
    const std::string sql_str(sql);
    int rc = sqlite3_prepare_v2(db_, sql_str.c_str(),
                                static_cast<int>(sql_str.size()), &stmt, nullptr);
    if (rc != SQLITE_OK) {
        const char* err = sqlite3_errmsg(db_);
        r = translate_error(rc, err);
        return r;
    }

    for (std::size_t i = 0; i < params.size(); ++i) {
        bind_value(stmt, static_cast<int>(i + 1), params[i].value);
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_DONE) {
        r.rows_affected = sqlite3_changes(db_);
        r.last_insert_id = sqlite3_last_insert_rowid(db_);
    } else if (rc == SQLITE_ROW) {
        // Statement returned a row (e.g. a SELECT used via execute). Unusual but valid.
        r.rows_affected = 1;
    } else {
        r = translate_error(rc, sqlite3_errmsg(db_));
    }
    sqlite3_finalize(stmt);
    return r;
}

DbResult SqliteAdapter::query(std::string_view sql,
                              std::span<const Bind> params,
                              ResultSet& out) {
    std::lock_guard<std::mutex> lk(mu_);
    DbResult r;
    if (!db_) { r.error = DbError::NotConnected; return r; }

    sqlite3_stmt* stmt = nullptr;
    const std::string sql_str(sql);
    int rc = sqlite3_prepare_v2(db_, sql_str.c_str(),
                                static_cast<int>(sql_str.size()), &stmt, nullptr);
    if (rc != SQLITE_OK) {
        r = translate_error(rc, sqlite3_errmsg(db_));
        return r;
    }

    for (std::size_t i = 0; i < params.size(); ++i) {
        bind_value(stmt, static_cast<int>(i + 1), params[i].value);
    }

    int ncols = sqlite3_column_count(stmt);
    out.columns.clear();
    out.columns.reserve(ncols);
    for (int i = 0; i < ncols; ++i) {
        out.columns.emplace_back(sqlite3_column_name(stmt, i));
    }
    out.rows.clear();

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        Row row;
        row.reserve(ncols);
        for (int i = 0; i < ncols; ++i) {
            row.push_back(read_column(stmt, i));
        }
        out.rows.push_back(std::move(row));
    }
    if (rc != SQLITE_DONE) {
        r = translate_error(rc, sqlite3_errmsg(db_));
    }
    sqlite3_finalize(stmt);
    return r;
}

DbResult SqliteAdapter::begin_transaction() {
    return execute("BEGIN TRANSACTION;", {});
}

DbResult SqliteAdapter::commit() {
    return execute("COMMIT;", {});
}

DbResult SqliteAdapter::rollback() {
    return execute("ROLLBACK;", {});
}

DbResult SqliteAdapter::exec_multi(std::string_view sql) {
    std::lock_guard<std::mutex> lk(mu_);
    DbResult r;
    if (!db_) { r.error = DbError::NotConnected; return r; }
    char* errmsg = nullptr;
    int rc = sqlite3_exec(db_, std::string(sql).c_str(), nullptr, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
        r = translate_error(rc, errmsg);
        if (errmsg) sqlite3_free(errmsg);
    }
    return r;
}

}  // namespace mxh::db