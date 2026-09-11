#include "mxh/server/account_lockout.hpp"
#include "mxh/server/account_moderation.hpp"
#include "mxh/server/login_audit.hpp"

#include <iostream>
#include <string>

namespace mxh::server {

std::string evaluate_lockout(mxh::db::IDbAdapter& db,
                              std::string_view account_id,
                              std::int32_t failure_threshold,
                              std::int32_t window_minutes,
                              std::string_view actor,
                              std::string_view reason) {
    if (account_id.empty()) return "below_threshold";
    if (failure_threshold <= 0) return "below_threshold";  // policy disabled
    if (window_minutes <= 0) window_minutes = 60;  // safe default

    // Honor a pre-existing block so we don't audit "blocked" twice.
    if (is_account_login_blocked(db, account_id)) return "already_blocked";

    const auto failures =
        count_login_audit_failures(db, account_id, window_minutes);
    if (failures < failure_threshold) return "below_threshold";

    const auto r = set_account_login_blocked(db, account_id, /*blocked=*/true,
                                              actor, reason);
    if (!r.ok()) {
        std::cerr << "[AccountLockout] failed to block account="
                  << account_id << " err=" << r.error_message << "\n";
        return "db_error";
    }
    record_login_audit(db, account_id, /*remote_addr=*/"", "blocked",
                       "auto_lockout_policy");
    std::cout << "[AccountLockout] blocked account=" << account_id
              << " failures=" << failures
              << " window_minutes=" << window_minutes << "\n";
    return "blocked";
}

}  // namespace mxh::server