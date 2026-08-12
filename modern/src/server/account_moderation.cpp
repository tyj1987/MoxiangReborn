#include "mxh/server/account_moderation.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace mxh::server {

bool is_account_login_blocked(mxh::db::IDbAdapter& db, std::string_view account) {
    mxh::db::ResultSet rows;
    const std::vector<mxh::db::Bind> args{mxh::db::bind(std::string(account))};
    const auto result = db.query(
        "SELECT login_blocked FROM modern_account_status WHERE account_id = ?", args, rows);
    if (!result.ok() || rows.empty() || rows.rows[0].empty()) return false;
    const auto* value = std::get_if<std::int64_t>(&rows.rows[0][0]);
    return value && *value != 0;
}

mxh::db::DbResult set_account_login_blocked(mxh::db::IDbAdapter& db,
                                             std::string_view account,
                                             bool blocked,
                                             std::string_view actor,
                                             std::string_view reason) {
    auto transaction = db.begin_transaction();
    if (!transaction.ok()) return transaction;

    const std::vector<mxh::db::Bind> status_args{
        mxh::db::bind(std::string(account)), mxh::db::bind(blocked ? 1 : 0),
        mxh::db::bind(std::string(reason))};
    const bool mssql = db.backend_name() == "mssql_odbc";
    auto result = db.execute(mssql
        ? "MERGE modern_account_status AS target USING (SELECT ? AS account_id, ? AS login_blocked, ? AS reason) AS source "
          "ON target.account_id = source.account_id WHEN MATCHED THEN UPDATE SET login_blocked = source.login_blocked, "
          "reason = source.reason, updated_at = CURRENT_TIMESTAMP WHEN NOT MATCHED THEN INSERT "
          "(account_id, login_blocked, reason, updated_at) VALUES "
          "(source.account_id, source.login_blocked, source.reason, CURRENT_TIMESTAMP);"
        : "INSERT INTO modern_account_status (account_id, login_blocked, reason, updated_at) "
          "VALUES (?, ?, ?, CURRENT_TIMESTAMP) ON CONFLICT (account_id) DO UPDATE SET "
          "login_blocked = excluded.login_blocked, reason = excluded.reason, updated_at = CURRENT_TIMESTAMP",
        status_args);
    if (!result.ok()) {
        const auto rollback_result = db.rollback();
        (void)rollback_result;
        return result;
    }

    const std::vector<mxh::db::Bind> audit_args{
        mxh::db::bind(std::string(actor)), mxh::db::bind(std::string(account)),
        mxh::db::bind(blocked ? std::string("ban") : std::string("unban")),
        mxh::db::bind(std::string(reason))};
    result = db.execute(
        "INSERT INTO modern_gm_audit (actor, target_account, action, reason, created_at) "
        "VALUES (?, ?, ?, ?, CURRENT_TIMESTAMP)", audit_args);
    if (!result.ok()) {
        const auto rollback_result = db.rollback();
        (void)rollback_result;
        return result;
    }
    return db.commit();
}

} // namespace mxh::server
