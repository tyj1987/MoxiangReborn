#include "mxh/server/login_audit.hpp"

#include <iostream>
#include <string>

namespace mxh::server {

namespace {
// MSSQL uses GETDATE() instead of CURRENT_TIMESTAMP for parity with the
// rest of the modern_gm_audit / modern_live_event inserts. The
// schema_migration kSqliteSchema literal only feeds SQLite, so we branch
// at run time.
std::string audit_insert_sql(const std::string& backend) {
    if (backend == "mssql_odbc") {
        return "INSERT INTO modern_login_audit "
               "(account_id, remote_addr, outcome, detail) "
               "VALUES (?, ?, ?, ?)";
    }
    return "INSERT INTO modern_login_audit "
           "(account_id, remote_addr, outcome, detail, created_at) "
           "VALUES (?, ?, ?, ?, strftime('%Y-%m-%dT%H:%M:%fZ', 'now'))";
}
}  // namespace

bool record_login_audit(mxh::db::IDbAdapter& db,
                       std::string_view account_id,
                       std::string_view remote_addr,
                       std::string_view outcome,
                       std::string_view detail) {
    const std::vector<mxh::db::Bind> params{
        mxh::db::bind(std::string(account_id)),
        mxh::db::bind(std::string(remote_addr)),
        mxh::db::bind(std::string(outcome)),
        mxh::db::bind(std::string(detail))};
    auto r = db.execute(audit_insert_sql(db.backend_name()), params);
    if (!r.ok()) {
        std::cerr << "[LoginAudit] failed to record outcome=" << outcome
                  << " account=" << account_id
                  << " remote=" << remote_addr
                  << " err=" << r.error_message << "\n";
        return false;
    }
    return true;
}

std::int64_t count_login_audit_failures(mxh::db::IDbAdapter& db,
                                         std::string_view account_id,
                                         int window_minutes) {
    mxh::db::ResultSet rs;
    // SQLite uses strftime('%J', 'now') for seconds since epoch; MSSQL uses
    // DATEDIFF(MINUTE, created_at, SYSUTCDATETIME()). The placeholder is
    // intentionally simple so the call works on both engines.
    const std::string sql =
        db.backend_name() == "mssql_odbc"
            ? "SELECT COUNT(*) FROM modern_login_audit "
              "WHERE account_id = ? AND outcome IN ('bad_password','throttled','blocked') "
              "AND DATEDIFF(MINUTE, created_at, SYSUTCDATETIME()) < ?"
            : "SELECT COUNT(*) FROM modern_login_audit "
              "WHERE account_id = ? AND outcome IN ('bad_password','throttled','blocked') "
              "AND CAST((julianday('now') - julianday(created_at)) * 24 * 60 AS INTEGER) < ?";
    const std::vector<mxh::db::Bind> params{
        mxh::db::bind(std::string(account_id)),
        mxh::db::bind(static_cast<std::int64_t>(window_minutes))};
    if (!db.query(sql, params, rs).ok() || rs.empty()) return 0;
    const auto* value = std::get_if<std::int64_t>(&rs.rows[0][0]);
    return value ? *value : 0;
}

}  // namespace mxh::server