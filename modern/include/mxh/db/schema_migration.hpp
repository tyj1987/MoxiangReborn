#pragma once

#include "mxh/db/db_adapter.hpp"

namespace mxh::db {

inline constexpr int kModernSchemaVersion = 1;

// Applies every idempotent migration required by Login/Agent/Map. The caller
// must connect the adapter first. No accounts or gameplay rows are seeded.
[[nodiscard]] DbResult migrate_modern_schema(IDbAdapter& db);

// Returns zero when the version table has not been created yet.
[[nodiscard]] int modern_schema_version(IDbAdapter& db);

}  // namespace mxh::db
