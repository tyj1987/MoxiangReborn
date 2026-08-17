// M5.3: portal binary — loads config, opens DB, registers auth routes, runs.
// M5.3 closeout: enforces JWT secret presence (exit 6 if empty); allow override
// via PORTAL_ALLOW_INSECURE_JWT=1 for local development only.

#include "portal/auth_routes.hpp"
#include "portal/config.hpp"
#include "portal/http_server.hpp"
#include "portal/portal_log.hpp"

#include "mxh/db/db_adapter.hpp"

#include <csignal>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

namespace {

std::atomic<bool> g_running{true};
void on_signal(int) { g_running.store(false); }

// Translate portal Config (env-driven) into mxh::db::ConnectionConfig.
mxh::db::ConnectionConfig build_db_cfg(const mxh::portal::Config& cfg) {
    mxh::db::ConnectionConfig out;
    out.backend = cfg.db_backend;
    out.path     = cfg.db_path;
    return out;
}

// Returns true if insecure-JWT override is enabled (PORTAL_ALLOW_INSECURE_JWT=1).
bool insecure_jwt_override_allowed() {
    const char* v = std::getenv("PORTAL_ALLOW_INSECURE_JWT");
    return v && std::string(v) == "1";
}

}  // namespace

int main(int argc, char** argv) {
    std::signal(SIGINT,  on_signal);
    std::signal(SIGTERM, on_signal);

    std::cout << "[portal] Moxian Portal - M5.3 (auth routes wired)\n";

    auto cfg = mxh::portal::load_config();

    std::cout << "[portal] Configuration:\n"
              << "  version       = " << cfg.version << "\n"
              << "  bind          = " << cfg.bind << "\n"
              << "  port          = " << cfg.port << "\n"
              << "  db_backend    = " << cfg.db_backend << "\n"
              << "  db_path       = " << cfg.db_path << "\n"
              << "  jwt_secret    = "
              << (cfg.jwt_secret.empty() ? "(not set)" : "***") << "\n";

    // Enforce JWT secret at startup. Production binds must fail hard; local
    // dev can opt in via PORTAL_ALLOW_INSECURE_JWT=1.
    if (cfg.jwt_secret.empty()) {
        if (!insecure_jwt_override_allowed()) {
            std::cerr << "[portal] FATAL: PORTAL_JWT_SECRET is not set. "
                         "Set it via environment variable before startup. "
                         "For local dev only, set PORTAL_ALLOW_INSECURE_JWT=1.\n";
            return 6;  // reserved exit code for JWT secret missing
        }
        std::cerr << "[portal] WARNING: PORTAL_JWT_SECRET is empty; "
                     "continuing because PORTAL_ALLOW_INSECURE_JWT=1.\n";
    }

    // Open the database (SQLite file or MSSQL ODBC DSN).
    auto db = mxh::db::make_adapter(cfg.db_backend);
    if (!db) {
        std::cerr << "[portal] FATAL: unknown db backend: "
                  << cfg.db_backend << "\n";
        return 1;
    }
    auto db_cfg = build_db_cfg(cfg);
    auto conn = db->connect(db_cfg);
    if (!conn.ok()) {
        std::cerr << "[portal] FATAL: db connect failed: "
                  << conn.error_message << "\n";
        return 1;
    }
    std::cout << "[portal] DB connected: " << db->backend_name() << "\n";

    mxh::portal::HttpServer server(cfg);
    mxh::portal::register_auth_routes(server, *db, cfg.jwt_secret);

    std::thread shutdown_monitor([&]() {
        while (g_running.load()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        server.shutdown();
    });

    int rc = server.run();

    if (shutdown_monitor.joinable()) {
        shutdown_monitor.detach();
    }

    std::cout << "[portal] exited with code " << rc << "\n";
    return rc;
}
