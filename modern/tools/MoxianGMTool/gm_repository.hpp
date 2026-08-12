#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "mxh/db/db_adapter.hpp"

namespace mxh::gm {

struct PlayerRecord {
    std::int64_t character_id{};
    std::string character_name;
    std::string account_id;
    std::int64_t level{1};
    std::int64_t money{};
    bool login_blocked{};
};

class Repository {
public:
    explicit Repository(mxh::db::IDbAdapter& db) : db_(db) {}

    mxh::db::DbResult list_players(std::vector<PlayerRecord>& players);
    mxh::db::DbResult find_account_for_character(std::int64_t character_id,
                                                  std::string& account_id);
    mxh::db::DbResult list_audit(mxh::db::ResultSet& rows);
    mxh::db::DbResult list_chat(mxh::db::ResultSet& rows);
    mxh::db::DbResult list_events(mxh::db::ResultSet& rows);
    mxh::db::DbResult create_event(const std::string& type, const std::string& title,
                                    const std::string& config_json,
                                    const std::string& starts_at, const std::string& ends_at,
                                    const std::string& actor);
    mxh::db::DbResult disable_event(std::int64_t event_id, const std::string& actor,
                                     const std::string& reason);

private:
    mxh::db::IDbAdapter& db_;
};

} // namespace mxh::gm
