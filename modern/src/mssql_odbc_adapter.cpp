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
    // Named-pipe / LocalDB endpoints like (localdb)\\MSSQLLocalDB have no
    // TCP listener; omit the ",port" suffix for them (also when the kv
    // string explicitly sets port=0).
    std::string server = cfg.host;
    const bool pipe_style = cfg.host.find('(') != std::string::npos ||
                            cfg.host.find('\\') != std::string::npos;
    if (cfg.port != 0 && !pipe_style) {
        server += "," + std::to_string(cfg.port);
    }
    std::string auth;
    if (cfg.user.empty()) {
        // Windows integrated auth (LocalDB / domain setups): the process
        // identity connects directly.
        auth = "Trusted_Connection=yes;";
    } else {
        auth = "Uid=" + cfg.user + ";Pwd=" + cfg.password + ";";
    }
    std::string s = "Driver={ODBC Driver 17 for SQL Server};"
                    "Server=" + server + ";"
                    "Database=" + cfg.database + ";" +
                    auth;
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
        DbResult error = translate_error(dbc_, SQL_HANDLE_DBC, "SQLDriverConnect");
        // Any ODBC failure while establishing the connection (DNS
        // resolution, driver auth, network timeout, ...) is a connection
        // failure, regardless of the driver's exact SQLSTATE.
        error.error = DbError::ConnectionFailed;
        SQLFreeHandle(SQL_HANDLE_DBC, dbc_);
        dbc_ = SQL_NULL_HANDLE;
        in_txn_ = false;
        return error;
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
        rc = SQLBindParameter(stmt, param_index, SQL_PARAM_INPUT,
                              SQL_C_SBIGINT, SQL_BIGINT, 0, 0,
                              const_cast<std::int64_t*>(&std::get<std::int64_t>(v)),
                              0, &out_indicator);
    } else if (std::holds_alternative<double>(v)) {
        out_indicator = 0;
        rc = SQLBindParameter(stmt, param_index, SQL_PARAM_INPUT,
                              SQL_C_DOUBLE, SQL_DOUBLE, 0, 0,
                              const_cast<double*>(&std::get<double>(v)),
                              0, &out_indicator);
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
    SQLLEN type = SQL_UNKNOWN_TYPE;
    SQLColAttribute(stmt, col, SQL_DESC_TYPE, nullptr, 0, nullptr, &type);

    if (type == SQL_BIGINT || type == SQL_INTEGER ||
        type == SQL_SMALLINT || type == SQL_TINYINT || type == SQL_BIT) {
        std::int64_t value = 0;
        SQLLEN indicator = 0;
        const SQLRETURN rc = SQLGetData(stmt, col, SQL_C_SBIGINT,
                                        &value, sizeof(value), &indicator);
        if (rc == SQL_NULL_DATA || indicator == SQL_NULL_DATA) {
            out = std::monostate{};
            return r;
        }
        if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
            return translate_error(stmt, SQL_HANDLE_STMT, nullptr);
        }
        out = value;
        return r;
    }

    if (type == SQL_DOUBLE || type == SQL_FLOAT || type == SQL_REAL) {
        double value = 0.0;
        SQLLEN indicator = 0;
        const SQLRETURN rc = SQLGetData(stmt, col, SQL_C_DOUBLE,
                                        &value, sizeof(value), &indicator);
        if (rc == SQL_NULL_DATA || indicator == SQL_NULL_DATA) {
            out = std::monostate{};
            return r;
        }
        if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
            return translate_error(stmt, SQL_HANDLE_STMT, nullptr);
        }
        out = value;
        return r;
    }

    if (type == SQL_BINARY || type == SQL_VARBINARY ||
        type == SQL_LONGVARBINARY) {
        std::vector<std::uint8_t> value;
        for (;;) {
            SQLCHAR chunk[4096] = {};
            SQLLEN indicator = 0;
            const SQLRETURN rc = SQLGetData(stmt, col, SQL_C_BINARY,
                                            chunk, sizeof(chunk), &indicator);
            if (rc == SQL_NULL_DATA || indicator == SQL_NULL_DATA) {
                out = std::monostate{};
                return r;
            }
            if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
                return translate_error(stmt, SQL_HANDLE_STMT, nullptr);
            }
            std::size_t count = sizeof(chunk);
            if (rc == SQL_SUCCESS && indicator >= 0 &&
                static_cast<std::size_t>(indicator) < count) {
                count = static_cast<std::size_t>(indicator);
            }
            value.insert(value.end(), chunk, chunk + count);
            if (rc == SQL_SUCCESS) break;
        }
        out = std::move(value);
        return r;
    }

    std::string value;
    for (;;) {
        SQLCHAR chunk[4096] = {};
        SQLLEN indicator = 0;
        const SQLRETURN rc = SQLGetData(stmt, col, SQL_C_CHAR,
                                        chunk, sizeof(chunk), &indicator);
        if (rc == SQL_NULL_DATA || indicator == SQL_NULL_DATA) {
            out = std::monostate{};
            return r;
        }
        if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
            return translate_error(stmt, SQL_HANDLE_STMT, nullptr);
        }
        std::size_t count = 0;
        while (count + 1 < sizeof(chunk) && chunk[count] != 0) ++count;
        value.append(reinterpret_cast<const char*>(chunk), count);
        if (rc == SQL_SUCCESS) break;
    }
    out = std::move(value);
    return r;
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
    std::vector<SQLLEN> indicators(params.size(), 0);
    for (std::size_t i = 0; i < params.size(); ++i) {
        DbResult br = bind_param(stmt_, static_cast<SQLUSMALLINT>(i + 1),
                                 params[i].value, indicators[i]);
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
    std::vector<SQLLEN> indicators(params.size(), 0);
    for (std::size_t i = 0; i < params.size(); ++i) {
        DbResult br = bind_param(stmt_, static_cast<SQLUSMALLINT>(i + 1),
                                 params[i].value, indicators[i]);
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
    std::lock_guard<std::mutex> lk(mu_);
    DbResult r;
    if (dbc_ == SQL_NULL_HANDLE || stmt_ == SQL_NULL_HANDLE) {
        r.error = DbError::NotConnected;
        return r;
    }
    if (in_txn_) {
        r.error = DbError::ConstraintViolation;
        return r;
    }
    const SQLRETURN rc = SQLSetConnectAttr(
        dbc_, SQL_ATTR_AUTOCOMMIT,
        reinterpret_cast<SQLPOINTER>(SQL_AUTOCOMMIT_OFF), 0);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
        return translate_error(dbc_, SQL_HANDLE_DBC, nullptr);
    }
    in_txn_ = true;
    return r;
}

DbResult MssqlOdbcAdapter::commit() {
    std::lock_guard<std::mutex> lk(mu_);
    DbResult r;
    if (dbc_ == SQL_NULL_HANDLE || stmt_ == SQL_NULL_HANDLE) {
        r.error = DbError::NotConnected;
        return r;
    }
    if (!in_txn_) {
        r.error = DbError::ConstraintViolation;
        return r;
    }
    const SQLRETURN end_rc = SQLEndTran(SQL_HANDLE_DBC, dbc_, SQL_COMMIT);
    if (end_rc != SQL_SUCCESS && end_rc != SQL_SUCCESS_WITH_INFO) {
        return translate_error(dbc_, SQL_HANDLE_DBC, nullptr);
    }
    in_txn_ = false;
    const SQLRETURN auto_rc = SQLSetConnectAttr(
        dbc_, SQL_ATTR_AUTOCOMMIT,
        reinterpret_cast<SQLPOINTER>(SQL_AUTOCOMMIT_ON), 0);
    if (auto_rc != SQL_SUCCESS && auto_rc != SQL_SUCCESS_WITH_INFO) {
        return translate_error(dbc_, SQL_HANDLE_DBC, nullptr);
    }
    return r;
}

DbResult MssqlOdbcAdapter::rollback() {
    std::lock_guard<std::mutex> lk(mu_);
    DbResult r;
    if (dbc_ == SQL_NULL_HANDLE || stmt_ == SQL_NULL_HANDLE) {
        r.error = DbError::NotConnected;
        return r;
    }
    if (!in_txn_) {
        r.error = DbError::ConstraintViolation;
        return r;
    }
    const SQLRETURN end_rc = SQLEndTran(SQL_HANDLE_DBC, dbc_, SQL_ROLLBACK);
    if (end_rc != SQL_SUCCESS && end_rc != SQL_SUCCESS_WITH_INFO) {
        return translate_error(dbc_, SQL_HANDLE_DBC, nullptr);
    }
    in_txn_ = false;
    const SQLRETURN auto_rc = SQLSetConnectAttr(
        dbc_, SQL_ATTR_AUTOCOMMIT,
        reinterpret_cast<SQLPOINTER>(SQL_AUTOCOMMIT_ON), 0);
    if (auto_rc != SQL_SUCCESS && auto_rc != SQL_SUCCESS_WITH_INFO) {
        return translate_error(dbc_, SQL_HANDLE_DBC, nullptr);
    }
    return r;
}

}  // namespace mxh::db
