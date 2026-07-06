// db_factory.cpp - Factory for IDbAdapter implementations.

#include "mxh/db/db_adapter.hpp"
#include "mxh/db/sqlite_adapter.hpp"

#include <memory>
#include <string>

namespace mxh::db {

std::unique_ptr<IDbAdapter> make_adapter(std::string_view backend) {
    std::string b(backend);
    if (b == "sqlite" || b.empty()) {
        return std::make_unique<SqliteAdapter>();
    }
    // Future: mssql_odbc, postgres, etc.
    return nullptr;
}

}  // namespace mxh::db