// mssql_odbc_adapter.hpp - Microsoft SQL Server implementation of IDbAdapter.
//
// Uses the Windows native ODBC API (sql.h / sqlext.h, odbc32.lib) to talk
// to Microsoft SQL Server. This is the modern equivalent of the legacy
// [Lib]DBThread layer that used the same ODBC interface against the
// 墨香 MSSQL stored procedures.
//
// Reference: Microsoft ODBC API Reference (Windows SDK).
//   - SQLAllocHandle / SQLFreeHandle
//   - SQLDriverConnect  (preferred for DSN-less connection)
//   - SQLPrepare / SQLBindParameter / SQLExecute
//   - SQLFetch / SQLGetData
//
// Threading: SQLDisconnect/SQLConnect are serialized via mu_. ODBC drivers
// in the Microsoft default stack are thread-safe at the connection level
// (one connection = one thread) but not at the environment/handle level
// when statements are shared, so we lock for the entire statement lifetime.

#pragma once

#ifdef _WIN32
// <windows.h> MUST come before <sql.h>: sqltypes.h uses INT64/UINT64 which
// only exist when windows.h has been included first. This is the official
// Windows SDK include order (see sql.h line 14: "preconditions:
// #include "windows.h"").
#  include <windows.h>
#endif

#include "mxh/db/db_adapter.hpp"

#include <sql.h>
#include <sqlext.h>

#include <mutex>
#include <string>
#include <vector>

namespace mxh::db {

class MssqlOdbcAdapter final : public IDbAdapter {
public:
    MssqlOdbcAdapter();
    ~MssqlOdbcAdapter() override;

    MssqlOdbcAdapter(const MssqlOdbcAdapter&) = delete;
    MssqlOdbcAdapter& operator=(const MssqlOdbcAdapter&) = delete;

    [[nodiscard]] DbResult connect(const ConnectionConfig& cfg) override;
    void disconnect() override;
    [[nodiscard]] bool is_connected() const noexcept override { return dbc_ != SQL_NULL_HANDLE; }

    [[nodiscard]] const ConnectionConfig& config() const noexcept { return cfg_; }

    [[nodiscard]] DbResult execute(std::string_view sql,
                                   std::span<const Bind> params) override;
    [[nodiscard]] DbResult query(std::string_view sql,
                                 std::span<const Bind> params,
                                 ResultSet& out) override;

    [[nodiscard]] DbResult begin_transaction() override;
    [[nodiscard]] DbResult commit() override;
    [[nodiscard]] DbResult rollback() override;

    [[nodiscard]] std::string backend_name() const noexcept override { return "mssql_odbc"; }

    // Direct access (escape hatch for diagnostics).
    [[nodiscard]] SQLHANDLE dbc() const noexcept { return dbc_; }

private:
    ConnectionConfig cfg_{};
    SQLHANDLE env_  = SQL_NULL_HANDLE;  // environment handle
    SQLHANDLE dbc_  = SQL_NULL_HANDLE;  // connection handle
    SQLHANDLE stmt_ = SQL_NULL_HANDLE;  // reusable statement handle
    std::mutex mu_;
    bool in_txn_ = false;

    // Translate SQLSTATE / native error code into DbError + message.
    DbResult translate_error(SQLHANDLE handle, SQLSMALLINT handle_type,
                             const char* prefix);

    // Bind a Value to a parameter slot via SQLBindParameter.
    DbResult bind_param(SQLHSTMT stmt, SQLUSMALLINT param_index,
                        const Value& v, SQLLEN& out_strlen_or_indicator);

    // Read a single column into a Value.
    DbResult fetch_column(SQLHSTMT stmt, SQLUSMALLINT col, Value& out);
};

}  // namespace mxh::db
