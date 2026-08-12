#pragma once

#include <string_view>

#include "mxh/db/db_adapter.hpp"

namespace mxh::server {

// Missing moderation tables mean an unmigrated legacy database; login remains
// available until the explicit commercial migration is applied.
bool is_account_login_blocked(mxh::db::IDbAdapter& db, std::string_view account);

mxh::db::DbResult set_account_login_blocked(mxh::db::IDbAdapter& db,
                                             std::string_view account,
                                             bool blocked,
                                             std::string_view actor,
                                             std::string_view reason);

} // namespace mxh::server
