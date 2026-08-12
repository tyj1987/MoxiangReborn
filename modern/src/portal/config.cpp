// modern/src/portal/config.cpp

#include "portal/config.hpp"
#include <cstdlib>
#include <thread>

namespace mxh::portal {

namespace {
std::string env_or(const char* key, const char* fallback) {
    const char* v = std::getenv(key);
    return v ? std::string(v) : std::string(fallback);
}
std::uint16_t env_u16(const char* key, std::uint16_t fallback) {
    const char* v = std::getenv(key);
    if (!v) return fallback;
    try { return static_cast<std::uint16_t>(std::stoul(v)); }
    catch (...) { return fallback; }
}
}  // namespace

Config load_config() {
    Config c;
    c.bind         = env_or("PORTAL_BIND",       "0.0.0.0");
    c.port         = env_u16("PORTAL_PORT",       8080);
    c.db_backend   = env_or("PORTAL_DB_BACKEND", "sqlite");
    c.db_path      = env_or("PORTAL_DB_PATH",    "modern/build/runtime/moxian.db");
    c.jwt_secret   = env_or("PORTAL_JWT_SECRET", "");
    c.static_root  = env_or("PORTAL_STATIC_ROOT", "deploy/portal/static");
    c.content_root = env_or("PORTAL_CONTENT_ROOT", "deploy/portal/content");
    c.shop_catalog = env_or("PORTAL_SHOP_CATALOG", "deploy/portal/shop/catalog.json");
    c.game_login_port = env_u16("PORTAL_GAME_LOGIN_PORT", 16001);
    c.game_agent_port = env_u16("PORTAL_GAME_AGENT_PORT", 17001);
    c.game_map_port   = env_u16("PORTAL_GAME_MAP_PORT",   18001);
    c.rate_limit_register = env_u16("PORTAL_RL_REGISTER", 5);
    c.rate_limit_login    = env_u16("PORTAL_RL_LOGIN",    10);
    c.version      = env_or("PORTAL_VERSION", "1.0.0");
    if (c.worker_threads == 0) {
        c.worker_threads = std::thread::hardware_concurrency();
        if (c.worker_threads == 0) c.worker_threads = 4;
    }
    return c;
}

}  // namespace mxh::portal
