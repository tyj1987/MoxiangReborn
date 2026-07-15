// mssql_odbc_adapter.cpp - Microsoft SQL Server via native Windows ODBC.
//
// Include order (Windows SDK requirement):
//   1. <windows.h>     — defines INT64/UINT64/HANDLE/etc. that sqltypes.h needs.
//   2. <sql.h>          — pulls in sqltypes.h, defines SQL* API macros + funcs.
//   3. <sqlext.h>       — SQL extensions (SQLDriverConnectW etc.).
//   4. our own header   — uses SQLTYPES already in scope.
//
// WIN32_LEAN_AND_MEAN is intentionally NOT set: ODBC headers are part of
// the "lean" set on modern Windows SDKs and the WIN32_LEAN_AND_MEAN
// guard in sqltypes.h (around INT64/UINT64 typedefs) needs the full
// windows.h to expose them.

#ifdef _WIN32
#  include <windows.h>
#endif

#include "mxh/db/mssql_odbc_adapter.hpp"

#include <cstring>
#include <string>
#include <vector>

namespace mxh::db {

namespace {

// Allocate an ODBC handle. Returns SqlResult-style error if allocation
// fails; otherwise returns Ok and writes the new handle to *out.
DbResult alloc_handle(SQLSMALLINT handle_type, SQLHANDLE input_handle,
                      SQLHANDLE* out) {
    DbResult r;
    SQLRETURN rc = SQLAllocHandle(handle_type, input_handle, out);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
        r.error = DbError::Unknown;
        r.error_message = "SQLAllocHandle failed";
    }
    return r;
}

// Format a Windows-style ODBC connection string from ConnectionConfig.
// Equivalent to what `mxh_db_tool --db "mssql_odbc;..."` would parse.
//
//   "mssql_odbc;host=...;port=...;database=...;user=...;password=...;dsn=...;"
std::string build_conn_string(const ConnectionConfig& cfg) {
    // If user supplied a DSN name, prefer that.
    if (!cfg.path.empty() && cfg.host.empty() && cfg.database.empty()) {
        return "DSN=" + cfg.path + ";";
    }
    std::string s = "Driver={ODBC Driver 17 for SQL Server};"
                    "Server=" + cfg.host + "," + std::to_string(cfg.port) + ";"
                    "Database=" + cfg.database + ";"
                    "Uid=" + cfg.user + ";"
                    "Pwd=" + cfg.password + ";";
    return s;
}

}  // namespace

MssqlOdbcAdapter::MssqlOdbcAdapter() = default;

MssqlOdbcAdapter::~MssqlOdbcAdapter() {
    disconnect();
}

DbResult MssqlOdbcAdapter::connect(const ConnectionConfig& cfg) {
    std::lock_guard<std::mutex> lk(mu_);
    cfg_ = cfg;
    DbResult r;

    if (dbc_ != SQL_NULL_HANDLE) {
        SQLDisconnect(dbc_);
        SQLFreeHandle(SQL_HANDLE_DBC, dbc_);
        dbc_ = SQL_NULL_HANDLE;
    }
    if (stmt_ != SQL_NULL_HANDLE) {
        SQLFreeHandle(SQL_HANDLE_STMT, stmt_);
        stmt_ = SQL_NULL_HANDLE;
    }
    if (env_ == SQL_NULL_HANDLE) {
        r = alloc_handle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env_);
        if (!r) return r;
        // ODBC 3.x
        SQLSetEnvAttr(env_, SQL_ATTR_ODBC_VERSION,
                      reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3), 0);
    }

    r = alloc_handle(SQL_HANDLE_DBC, env_, &dbc_);
    if (!r) return r;

    // Connection timeout 5 s — keep login snappy.
    SQLSetConnectAttr(dbc_, SQL_LOGIN_TIMEOUT, reinterpret_cast<SQLPOINTER>(5), 0);

    std::string conn_str = build_conn_string(cfg);
    SQLRETURN rc = SQLDriverConnectA(
        dbc_, nullptr,
        reinterpret_cast<SQLCHAR*>(conn_str.data()),
        static_cast<SQLSMALLINT>(conn_str.size()),
        nullptr, 0, nullptr, SQL_DRIVER_NOPROMPT);

    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
        return translate_error(dbc_, SQL_HANDLE_DBC, "SQLDriverConnect");
    }

    r = alloc_handle(SQL_HANDLE_STMT, dbc_, &stmt_);
    if (!r) {
        SQLDisconnect(dbc_);
        SQLFreeHandle(SQL_HANDLE_DBC, dbc_);
        dbc_ = SQL_NULL_HANDLE;
        return r;
    }
    in_txn_ = false;
    return r;
}

void MssqlOdbcAdapter::disconnect() {
    std::lock_guard<std::mutex> lk(mu_);
    if (stmt_ != SQL_NULL_HANDLE) {
        SQLFreeHandle(SQL_HANDLE_STMT, stmt_);
        stmt_ = SQL_NULL_HANDLE;
    }
    if (dbc_ != SQL_NULL_HANDLE) {
        SQLDisconnect(dbc_);
        SQLFreeHandle(SQL_HANDLE_DBC, dbc_);
        dbc_ = SQL_NULL_HANDLE;
    }
    if (env_ != SQL_NULL_HANDLE) {
        SQLFreeHandle(SQL_HANDLE_ENV, env_);
        env_ = SQL_NULL_HANDLE;
    }
    in_txn_ = false;
}

DbResult MssqlOdbcAdapter::translate_error(SQLHANDLE handle,
                                            SQLSMALLINT handle_type,
                                            const char* prefix) {
    DbResult r;
    r.error = DbError::Unknown;
    if (prefix) r.error_message = prefix;
    SQLINTEGER native = 0;
    SQLCHAR state[6] = {0};
    SQLCHAR msg[1024] = {0};
    SQLSMALLINT msg_len = 0;
    SQLRETURN drc = SQLGetDiagRecA(handle_type, handle, 1, state, &native,
                                   msg, sizeof(msg), &msg_len);
    if (drc == SQL_SUCCESS || drc == SQL_SUCCESS_WITH_INFO) {
        std::string s((const char*)state);
        if (s == "08001" || s == "08003" || s == "08S01") {
            r.error = DbError::ConnectionFailed;
        } else if (s == "23000" || s == "23001") {
            r.error = DbError::ConstraintViolation;
        } else if (s == "42S02" || s == "S0002") {
            r.error = DbError::NoSuchTable;
        } else if (s == "42S01" || s == "S0001") {
            // table already exists — not a fatal error in our use cases
            r.error = DbError::ConstraintViolation;
        } else if (s == "42000" || s == "37000" || s == "S0003") {
            r.error = DbError::QuerySyntaxError;
        } else {
            r.error = DbError::Unknown;
        }
        if (!r.error_message.empty()) r.error_message += ": ";
        r.error_message += std::string((const char*)state) + " " +
                           std::string((const char*)msg, msg_len);
    }
    return r;
}

DbResult MssqlOdbcAdapter::bind_param(SQLHSTMT stmt, SQLUSMALLINT param_index,
                                      const Value& v, SQLLEN& out_indicator) {
    DbResult r;
    SQLRETURN rc;
    if (std::holds_alternative<std::monostate>(v)) {
        out_indicator = SQL_NULL_DATA;
        // For a NULL parameter, ODBC accepts any C type + any SQL type; we
        // pass SQL_C_DEFAULT / SQL_DEFAULT which is the documented "no
        // conversion" pair.
        rc = SQLBindParameter(stmt, param_index, SQL_PARAM_INPUT,
                              SQL_C_DEFAULT, SQL_DEFAULT, 0, 0, nullptr, 0,
                              &out_indicator);
    } else if (std::holds_alternative<std::int64_t>(v)) {
        out_indicator = 0;
        std::int64_t i = std::get<std::int64_t>(v);
        rc = SQLBindParameter(stmt, param_index, SQL_PARAM_INPUT,
                              SQL_C_SBIGINT, SQL_BIGINT, 0, 0, &i, 0, &out_indicator);
    } else if (std::holds_alternative<double>(v)) {
        out_indicator = 0;
        double d = std::get<double>(v);
        rc = SQLBindParameter(stmt, param_index, SQL_PARAM_INPUT,
                              SQL_C_DOUBLE, SQL_DOUBLE, 0, 0, &d, 0, &out_indicator);
    } else if (std::holds_alternative<std::string>(v)) {
        const std::string& s = std::get<std::string>(v);
        out_indicator = SQL_NTS;
        rc = SQLBindParameter(stmt, param_index, SQL_PARAM_INPUT,
                              SQL_C_CHAR, SQL_VARCHAR,
                              static_cast<SQLULEN>(s.size()), 0,
                              const_cast<char*>(s.c_str()),
                              static_cast<SQLLEN>(s.size()), &out_indicator);
    } else if (std::holds_alternative<std::vector<std::uint8_t>>(v)) {
        const auto& b = std::get<std::vector<std::uint8_t>>(v);
        out_indicator = static_cast<SQLLEN>(b.size());
        rc = SQLBindParameter(stmt, param_index, SQL_PARAM_INPUT,
                              SQL_C_BINARY, SQL_VARBINARY,
                              static_cast<SQLULEN>(b.size()), 0,
                              const_cast<std::uint8_t*>(b.data()),
                              static_cast<SQLLEN>(b.size()), &out_indicator);
    } else {
        r.error = DbError::NotImplemented;
        r.error_message = "unsupported bind type";
        return r;
    }
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
        return translate_error(stmt, SQL_HANDLE_STMT, "SQLBindParameter");
    }
    return r;
}

DbResult MssqlOdbcAdapter::fetch_column(SQLHSTMT stmt, SQLUSMALLINT col,
                                        Value& out) {
    DbResult r;
    SQLLEN indicator = 0;
    SQLCHAR buf[8192];
    SQLRETURN rc = SQLGetData(stmt, col, SQL_C_DEFAULT, buf, sizeof(buf),
                              &indicator);
    if (rc == SQL_NULL_DATA) {
        out = std::monostate{};
        return r;
    }
    if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO) {
        if (rc == SQL_SUCCESS_WITH_INFO && indicator > (SQLLEN)sizeof(buf)) {
            // Truncation: try again with bigger buffer up to 16 MiB.
            std::vector<std::uint8_t> big;
            big.resize(static_cast<std::size_t>(indicator));
            rc = SQLGetData(stmt, col, SQL_C_BINARY, big.data(),
                            static_cast<SQLLEN>(big.size()), &indicator);
            if (rc != SQL_SUCCESS) {
                return translate_error(stmt, SQL_HANDLE_STMT, "SQLGetData (large)");
            }
            out = std::move(big);
            return r;
        }
        // Inspect column type via SQLColAttribute (or fall back to buffer).
        SQLLEN type = 0;
        SQLColAttribute(stmt, col, SQL_DESC_TYPE, nullptr, 0, nullptr, &type);
        switch (type) {
            case SQL_BIGINT:
            case SQL_INTEGER:
            case SQL_SMALLINT:
            case SQL_TINYINT:
                out = static_cast<std::int64_t>(*reinterpret_cast<long long*>(buf));
                break;
            case SQL_DOUBLE:
            case SQL_FLOAT:
            case SQL_REAL:
                out = *reinterpret_cast<double*>(buf);
                break;
            case SQL_CHAR:
            case SQL_VARCHAR:
            case SQL_LONGVARCHAR:
            case SQL_WCHAR:
            case SQL_WVARCHAR:
                out = std::string(reinterpret_cast<const char*>(buf),
                                  static_cast<std::size_t>(indicator));
                break;
            case SQL_BINARY:
            case SQL_VARBINARY:
            case SQL_LONGVARBINARY:
                out = std::vector<std::uint8_t>(
                    reinterpret_cast<const std::uint8_t*>(buf),
                    reinterpret_cast<const std::uint8_t*>(buf) + indicator);
                break;
            default:
                out = std::string(reinterpret_cast<const char*>(buf),
                                  static_cast<std::size_t>(indicator));
                break;
        }
        return r;
    }
    return translate_error(stmt, SQL_HANDLE_STMT, "SQLGetData");
}

DbResult MssqlOdbcAdapter::execute(std::string_view sql,
                                   std::span<const Bind> params) {
    std::lock_guard<std::mutex> lk(mu_);
    DbResult r;
    if (dbc_ == SQL_NULL_HANDLE || stmt_ == SQL_NULL_HANDLE) {
        r.error = DbError::NotConnected;
        r.error_message = "not connected";
        return r;
    }
    std::string sql_str(sql);
    SQLRETURN rc = SQLPrepareA(stmt_,
                               reinterpret_cast<SQLCHAR*>(sql_str.data()),
                               static_cast<SQLINTEGER>(sql_str.size()));
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
        return translate_error(stmt_, SQL_HANDLE_STMT, "SQLPrepare");
    }
    SQLUSMALLINT p = 1;
    for (const auto& b : params) {
        SQLLEN ind = 0;
        DbResult br = bind_param(stmt_, p++, b.value, ind);
        if (!br) return br;
    }
    rc = SQLExecute(stmt_);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO &&
        rc != SQL_NO_DATA) {
        return translate_error(stmt_, SQL_HANDLE_STMT, "SQLExecute");
    }
    SQLLEN affected = 0;
    SQLRowCount(stmt_, &affected);
    r.rows_affected = static_cast<std::int64_t>(affected);
    // Best-effort: skip any result set (so callers can issue another query).
    while ((rc = SQLMoreResults(stmt_)) == SQL_SUCCESS ||
           rc == SQL_SUCCESS_WITH_INFO) { /* drain */ }
    SQLFreeStmt(stmt_, SQL_CLOSE);
    return r;
}

DbResult MssqlOdbcAdapter::query(std::string_view sql,
                                 std::span<const Bind> params,
                                 ResultSet& out) {
    std::lock_guard<std::mutex> lk(mu_);
    DbResult r;
    out = ResultSet{};
    if (dbc_ == SQL_NULL_HANDLE || stmt_ == SQL_NULL_HANDLE) {
        r.error = DbError::NotConnected;
        r.error_message = "not connected";
        return r;
    }
    std::string sql_str(sql);
    SQLRETURN rc = SQLPrepareA(stmt_,
                               reinterpret_cast<SQLCHAR*>(sql_str.data()),
                               static_cast<SQLINTEGER>(sql_str.size()));
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
        return translate_error(stmt_, SQL_HANDLE_STMT, "SQLPrepare");
    }
    SQLUSMALLINT p = 1;
    for (const auto& b : params) {
        SQLLEN ind = 0;
        DbResult br = bind_param(stmt_, p++, b.value, ind);
        if (!br) return br;
    }
    rc = SQLExecute(stmt_);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
        return translate_error(stmt_, SQL_HANDLE_STMT, "SQLExecute");
    }
    // Column names.
    SQLSMALLINT ncols = 0;
    SQLNumResultCols(stmt_, &ncols);
    out.columns.reserve(ncols);
    for (SQLSMALLINT i = 1; i <= ncols; ++i) {
        SQLCHAR name[256] = {0};
        SQLSMALLINT name_len = 0;
        SQLColAttribute(stmt_, i, SQL_DESC_NAME, name, sizeof(name),
                        &name_len, nullptr);
        out.columns.emplace_back(reinterpret_cast<const char*>(name),
                                static_cast<std::size_t>(name_len));
    }
    // Rows.
    while ((rc = SQLFetch(stmt_)) == SQL_SUCCESS ||
           rc == SQL_SUCCESS_WITH_INFO) {
        Row row;
        row.reserve(ncols);
        for (SQLSMALLINT i = 1; i <= ncols; ++i) {
            Value v;
            DbResult fr = fetch_column(stmt_, i, v);
            if (!fr) {
                SQLFreeStmt(stmt_, SQL_CLOSE);
                return fr;
            }
            row.push_back(std::move(v));
        }
        out.rows.push_back(std::move(row));
    }
    SQLFreeStmt(stmt_, SQL_CLOSE);
    return r;
}

DbResult MssqlOdbcAdapter::begin_transaction() {
    if (in_txn_) {
        DbResult r;
        r.error = DbError::ConstraintViolation;
        r.error_message = "transaction already in progress";
        return r;
    }
    DbResult r = execute("BEGIN TRANSACTION", {});
    if (r) in_txn_ = true;
    return r;
}

DbResult MssqlOdbcAdapter::commit() {
    if (!in_txn_) {
        DbResult r;
        r.error = DbError::ConstraintViolation;
        r.error_message = "no active transaction";
        return r;
    }
    DbResult r = execute("COMMIT", {});
    in_txn_ = false;
    return r;
}

DbResult MssqlOdbcAdapter::rollback() {
    if (!in_txn_) {
        DbResult r;
        r.error = DbError::ConstraintViolation;
        r.error_message = "no active transaction";
        return r;
    }
    DbResult r = execute("ROLLBACK", {});
    in_txn_ = false;
    return r;
}

}  // namespace mxh::db
