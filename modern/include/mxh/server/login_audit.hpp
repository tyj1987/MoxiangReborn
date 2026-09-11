#pragma once

#include "mxh/db/db_adapter.hpp"

#include <string>
#include <string_view>

namespace mxh::server {

// Outcome taxonomy for modern_login_audit. Keep the strings short so the
// index on outcome stays small and queries like
//   SELECT * FROM modern_login_audit WHERE outcome='throttled'
// stay cheap. Operators read the table, not an enum.
constexpr const char* kLoginOutcomeAccepted      = "accepted";
constexpr const char* kLoginOutcomeBadPassword   = "bad_password";
constexpr const char* kLoginOutcomeNoAccount      = "no_account";
constexpr const char* kLoginOutcomeBlocked       = "blocked";
constexpr const char* kLoginOutcomeThrottled      = "throttled";
constexpr const char* kLoginOutcomeRateLimited   = "rate_limited";
constexpr const char* kLoginOutcomeRejected      = "rejected";

// Persist a single login attempt to modern_login_audit. Best-effort: any
// DB failure is logged but does not abort the caller, because losing an
// audit row is preferable to aborting a login flow. account_id and
// remote_addr may be empty when the failure happens before credentials
// are parsed.
bool record_login_audit(mxh::db::IDbAdapter& db,
                       std::string_view account_id,
                       std::string_view remote_addr,
                       std::string_view outcome,
                       std::string_view detail = "");

// Count failed login attempts for one account within the last `window_minutes`.
// Used by account_moderation to escalate blocking after N sustained failures.
std::int64_t count_login_audit_failures(mxh::db::IDbAdapter& db,
                                         std::string_view account_id,
                                         int window_minutes);

}  // namespace mxh::server