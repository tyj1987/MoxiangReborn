#include "mxh/server/live_events.hpp"
#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
namespace mxh::server {
namespace {
std::string text(const mxh::db::Value& value) {
    return std::holds_alternative<std::string>(value) ? std::get<std::string>(value) : std::string{};
}
double multiplier(const std::string& json) {
    const auto key = json.find("\"multiplier\"");
    if (key == std::string::npos) return 1.0;
    const auto colon = json.find(':', key);
    if (colon == std::string::npos) return 1.0;
    try { return std::clamp(std::stod(json.substr(colon + 1)), 0.1, 10.0); }
    catch (...) { return 1.0; }
}
std::string message(const std::string& json) {
    const auto key = json.find("\"message\"");
    if (key == std::string::npos) return {};
    const auto quote = json.find('"', json.find(':', key) + 1);
    if (quote == std::string::npos) return {};
    const auto end = json.find('"', quote + 1);
    return end == std::string::npos ? std::string{} : json.substr(quote + 1, end - quote - 1);
}
}
LiveEventSnapshot load_active_live_events(mxh::db::IDbAdapter& db, const std::string& utc_now) {
    LiveEventSnapshot snapshot;
    mxh::db::ResultSet rows;
    const auto result = db.query("SELECT event_type,config_json,starts_at,ends_at FROM modern_live_event WHERE enabled=1", rows);
    if (!result.ok()) return snapshot;
    for (const auto& row : rows.rows) {
        if (row.size() < 4) continue;
        const auto type = text(row[0]); const auto config = text(row[1]);
        if (utc_now < text(row[2]) || utc_now >= text(row[3])) continue;
        if (type == "experience_multiplier") snapshot.experience_multiplier *= multiplier(config);
        else if (type == "drop_multiplier") snapshot.drop_multiplier *= multiplier(config);
        else if (type == "announcement") { const auto value = message(config); if (!value.empty()) snapshot.announcements.push_back(value); }
    }
    snapshot.experience_multiplier = std::clamp(snapshot.experience_multiplier, 0.1, 10.0);
    snapshot.drop_multiplier = std::clamp(snapshot.drop_multiplier, 0.1, 10.0);
    return snapshot;
}
std::string utc_now_iso8601() {
    const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm utc{};
#ifdef _WIN32
    gmtime_s(&utc, &now);
#else
    gmtime_r(&now, &utc);
#endif
    std::ostringstream out; out << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ"); return out.str();
}
} // namespace mxh::server
