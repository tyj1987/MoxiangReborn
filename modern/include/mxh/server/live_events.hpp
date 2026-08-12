#pragma once
#include "mxh/db/db_adapter.hpp"
#include <string>
#include <vector>
namespace mxh::server {
struct LiveEventSnapshot {
    double experience_multiplier = 1.0;
    double drop_multiplier = 1.0;
    std::vector<std::string> announcements;
};
LiveEventSnapshot load_active_live_events(mxh::db::IDbAdapter& db, const std::string& utc_now);
std::string utc_now_iso8601();
} // namespace mxh::server
