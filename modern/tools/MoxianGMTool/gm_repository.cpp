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
} // namespace mxh::gm
