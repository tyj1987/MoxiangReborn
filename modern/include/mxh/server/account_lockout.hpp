#pragma once

#include "mxh/db/db_adapter.hpp"

#include <cstdint>
#include <string_view>

namespace mxh::server {

// Configurable auto-lockout policy. When evaluate_lockout() finds at
// least `failure_threshold` bad-password / throttled / blocked audit
// rows for `account_id` within `window_minutes`, the account is
// permanently blocked via set_account_login_blocked. The decision is
// made under the DB session so concurrent failures can't race past
// the threshold between read and write.
//
// Reasons returned:
//   "below_threshold"   too few failures within the window
//   "blocked"           newly blocked (logged + audit row inserted)
//   "already_blocked"   modern_account_status already has login_blocked
//   "db_error"          DB read or write failed; caller should not
//                       treat the outcome as success
std::string evaluate_lockout(mxh::db::IDbAdapter& db,
                              std::string_view account_id,
                              std::int32_t failure_threshold,
                              std::int32_t window_minutes,
                              std::string_view actor = "auto_lockout",
                              std::string_view reason = "excessive_failures");

}  // namespace mxh::server