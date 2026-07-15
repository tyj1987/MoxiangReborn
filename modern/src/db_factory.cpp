// db_factory.cpp - Factory for IDbAdapter implementations.

#include "mxh/db/db_adapter.hpp"
#include "mxh/db/sqlite_adapter.hpp"
#ifdef _WIN32
#include "mxh/db/mssql_odbc_adapter.hpp"
#endif

#include <memory>
#include <string>

namespace mxh::db {

std::unique_ptr<IDbAdapter> make_adapter(std::string_view backend) {
    std::string b(backend);
    if (b == "sqlite" || b.empty()) {
        return std::make_unique<SqliteAdapter>();
    }
#ifdef _WIN32
    if (b == "mssql_odbc" || b == "mssql" || b == "sqlserver") {
        return std::make_unique<MssqlOdbcAdapter>();
    }
#endif
    return nullptr;
}

}  // namespace mxh::db
