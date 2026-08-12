// modern/src/portal/config.hpp
// Portal configuration loaded from environment variables.
// No secrets are hardcoded; secret values must be set at runtime via env.

#pragma once

#include <cstdint>
#include <string>

namespace mxh::portal {

struct Config {
    // HTTP server
    std::string  bind        = "0.0.0.0";
    std::uint16_t port       = 8080;

    // Database
    std::string  db_backend  = "sqlite";
    std::string  db_path     = "modern/build/runtime/moxian.db";

    // JWT (required — must be set via PORTAL_JWT_SECRET env)
    std::string  jwt_secret  = "";

    // Static file root (served at /static/*)
    std::string  static_root = "deploy/portal/static";

    // News content root (markdown files)
    std::string  content_root = "deploy/portal/content";

    // Shop catalog JSON
    std::string  shop_catalog = "deploy/portal/shop/catalog.json";

    // Game server ports for status ping
    std::uint16_t game_login_port = 16001;
    std::uint16_t game_agent_port = 17001;
    std::uint16_t game_map_port   = 18001;

    // Rate limit (requests per minute per IP)
    std::uint16_t rate_limit_register = 5;
    std::uint16_t rate_limit_login    = 10;

    // Version string shown in /api/status
    std::string  version = "1.0.0";

    // Number of server worker threads (0 = hardware_concurrency)
    unsigned int worker_threads = 0;
};

Config load_config();

}  // namespace mxh::portal
