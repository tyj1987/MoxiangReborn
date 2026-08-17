// modern/src/portal/status_routes.hpp
// M5.4: /api/status + background TCP ping of game servers.

#pragma once

#include "portal/config.hpp"
#include "portal/http_server.hpp"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

namespace mxh::portal {

struct StatusSnapshot {
    std::string login = "unknown";     // "up" | "down" | "unknown"
    std::string agent = "unknown";
    std::string map   = "unknown";
    std::int64_t online_count = -1;    // -1 = unknown
    std::chrono::system_clock::time_point last_check_at{};
    std::string version;
};

// Background ping thread — uses async TCP connect to each game port
// (Login 16001, Agent 17001, Map 18001). Pinned at 5s interval.
class StatusPinger {
public:
    StatusPinger(const Config& cfg);
    ~StatusPinger();

    StatusPinger(const StatusPinger&) = delete;
    StatusPinger& operator=(const StatusPinger&) = delete;

    void start();
    void stop();

    // Thread-safe snapshot read.
    StatusSnapshot snapshot() const;

private:
    void run_loop();
    bool ping_once(const std::string& host, std::uint16_t port, std::int64_t timeout_ms);

    const Config& cfg_;
    std::thread worker_;
    std::atomic<bool> running_{false};
    std::mutex mu_;
    StatusSnapshot snap_;
};

// Register GET /api/status with the given HttpServer.
void register_status_routes(HttpServer& server, const Config& cfg, StatusPinger& pinger);

}  // namespace mxh::portal
