#include "gm_repository.hpp"

#include <variant>

namespace mxh::gm {
namespace {
std::int64_t integer(const mxh::db::Value& value, std::int64_t fallback = 0) {
    if (const auto* number = std::get_if<std::int64_t>(&value)) return *number;
    return fallback;
}
std::string text(const mxh::db::Value& value) {
    if (const auto* string = std::get_if<std::string>(&value)) return *string;
    if (const auto* number = std::get_if<std::int64_t>(&value)) return std::to_string(*number);
    return {};
}
} // namespace

mxh::db::DbResult Repository::list_players(std::vector<PlayerRecord>& players) {
    mxh::db::ResultSet rows;
    auto result = db_.query(
        "SELECT c.chrid, c.charname, COALESCE(i.account_id, ''), "
        "COALESCE(s.level, 1), COALESCE(s.money, 0), COALESCE(a.login_blocked, 0) "
        "FROM character_info c LEFT JOIN modern_account_identity i ON i.user_idx = c.userid "
        "LEFT JOIN modern_player_state s ON s.player_id = c.chrid "
        "LEFT JOIN modern_account_status a ON a.account_id = i.account_id ORDER BY c.chrid", rows);
    if (!result.ok()) return result;
    players.clear();
    for (const auto& row : rows.rows) {
        if (row.size() < 6) continue;
        players.push_back({integer(row[0]), text(row[1]), text(row[2]),
                           integer(row[3], 1), integer(row[4]), integer(row[5]) != 0});
    }
    return result;
}

mxh::db::DbResult Repository::find_account_for_character(std::int64_t character_id,
                                                          std::string& account_id) {
    mxh::db::ResultSet rows;
    const std::vector<mxh::db::Bind> args{mxh::db::bind(character_id)};
    auto result = db_.query("SELECT i.account_id FROM character_info c JOIN modern_account_identity i "
                            "ON i.user_idx = c.userid WHERE c.chrid = ?", args, rows);
    if (result.ok() && !rows.empty() && !rows.rows[0].empty()) account_id = text(rows.rows[0][0]);
    else account_id.clear();
    return result;
}

mxh::db::DbResult Repository::list_audit(mxh::db::ResultSet& rows) {
    return db_.query("SELECT audit_id, actor, target_account, action, reason, created_at "
                     "FROM modern_gm_audit ORDER BY audit_id DESC", rows);
}

mxh::db::DbResult Repository::list_chat(mxh::db::ResultSet& rows) {
    return db_.query("SELECT logid, chrname, channel, message, logtime "
                     "FROM log_chat ORDER BY logid DESC", rows);
}

mxh::db::DbResult Repository::list_events(mxh::db::ResultSet& rows) {
    return db_.query("SELECT event_id, event_type, title, config_json, starts_at, ends_at, "
                     "enabled, created_by, created_at, updated_at "
                     "FROM modern_live_event ORDER BY event_id DESC", rows);
}

mxh::db::DbResult Repository::create_event(const std::string& type, const std::string& title,
                                            const std::string& config_json,
                                            const std::string& starts_at, const std::string& ends_at,
                                            const std::string& actor) {
    const std::vector<mxh::db::Bind> args{mxh::db::bind(type), mxh::db::bind(title),
        mxh::db::bind(config_json), mxh::db::bind(starts_at), mxh::db::bind(ends_at),
        mxh::db::bind(actor)};
    auto result = db_.begin_transaction();
    if (!result.ok()) return result;
    result = db_.execute("INSERT INTO modern_live_event "
                       "(event_type,title,config_json,starts_at,ends_at,enabled,created_by) "
                       "VALUES (?,?,?,?,?,1,?)", args);
    if (result.ok()) {
        const std::vector<mxh::db::Bind> audit_args{mxh::db::bind(actor),
            mxh::db::bind(std::string("event:") + title), mxh::db::bind(type)};
        result = db_.execute("INSERT INTO modern_gm_audit "
            "(actor,target_account,action,reason,created_at) VALUES (?,?,'event.create',?,CURRENT_TIMESTAMP)",
            audit_args);
    }
    if (result.ok()) result = db_.commit();
    else (void)db_.rollback();
    return result;
}

mxh::db::DbResult Repository::disable_event(std::int64_t event_id, const std::string& actor,
                                             const std::string& reason) {
    auto result = db_.begin_transaction();
    if (!result.ok()) return result;
    const std::vector<mxh::db::Bind> update_args{mxh::db::bind(event_id)};
    result = db_.execute("UPDATE modern_live_event SET enabled=0, updated_at=CURRENT_TIMESTAMP WHERE event_id=?",
                         update_args);
    if (result.ok()) {
        const std::vector<mxh::db::Bind> audit_args{mxh::db::bind(actor),
            mxh::db::bind(std::string("event:") + std::to_string(event_id)), mxh::db::bind(reason)};
        result = db_.execute("INSERT INTO modern_gm_audit "
            "(actor,target_account,action,reason,created_at) VALUES (?,?,'event.disable',?,CURRENT_TIMESTAMP)",
            audit_args);
    }
    if (result.ok()) result = db_.commit();
    else (void)db_.rollback();
    return result;
}
} // namespace mxh::gm
