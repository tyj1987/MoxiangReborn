// modern/tools/MoxianPortal/main.cpp
// M5.1 skeleton: minimal portal binary — just the HTTP server + /api/healthz.
// No auth, no DB, no game-status ping yet.

#include "portal/config.hpp"
#include "portal/http_server.hpp"
#include "portal/portal_log.hpp"

#include <csignal>
#include <iostream>
#include <thread>

namespace {

std::atomic<bool> g_running{true};
void on_signal(int) { g_running.store(false); }

}  // namespace

int main(int argc, char** argv) {
    std::signal(SIGINT,  on_signal);
    std::signal(SIGTERM, on_signal);

    std::cout << "[portal] Moxian Portal — M5.1 skeleton\n";
    std::cout << "[portal] (c) 2026 Moxian-Reborn\n\n";

    auto cfg = mxh::portal::load_config();

    std::cout << "[portal] Configuration:\n";
    std::cout << "  version       = " << cfg.version << "\n";
    std::cout << "  bind          = " << cfg.bind << "\n";
    std::cout << "  port          = " << cfg.port << "\n";
    std::cout << "  db_backend    = " << cfg.db_backend << "\n";
    std::cout << "  db_path       = " << cfg.db_path << "\n";
    std::cout << "  static_root   = " << cfg.static_root << "\n";
    std::cout << "  content_root  = " << cfg.content_root << "\n";
    std::cout << "  shop_catalog  = " << cfg.shop_catalog << "\n";
    std::cout << "  jwt_secret    = " << (cfg.jwt_secret.empty() ? "(not set)" : "***") << "\n";
    std::cout << "  workers       = " << cfg.worker_threads << "\n";

    if (cfg.jwt_secret.empty()) {
        std::cerr << "[portal] WARNING: PORTAL_JWT_SECRET is not set. "
                     "Set it via environment variable before production use.\n";
    }

    mxh::portal::HttpServer server(cfg);

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
