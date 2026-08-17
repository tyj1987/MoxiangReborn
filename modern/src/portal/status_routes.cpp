// modern/src/portal/status_routes.cpp
// M5.4: /api/status + background TCP ping thread using raw sockets.

#include "portal/status_routes.hpp"
#include "portal/portal_log.hpp"

#include <atomic>
#include <chrono>
#include <ctime>
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>

// Cross-platform socket helpers (no asio dependency).
#if defined(_WIN32)
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  pragma comment(lib, "Ws2_32.lib")
   using socket_t = SOCKET;
   constexpr socket_t kInvalidSocket = INVALID_SOCKET;
   inline int close_socket(socket_t s) { return closesocket(s); }
   inline int last_socket_error() { return WSAGetLastError(); }
#else
#  include <arpa/inet.h>
#  include <errno.h>
#  include <fcntl.h>
#  include <netinet/in.h>
#  include <sys/socket.h>
#  include <sys/time.h>
#  include <unistd.h>
   using socket_t = int;
   constexpr socket_t kInvalidSocket = -1;
   inline int close_socket(socket_t s) { return close(s); }
   inline int last_socket_error() { return errno; }
#endif

namespace mxh::portal {

namespace {
std::string now_iso8601() {
    auto t = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now());
    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &t);
    tm = *gmtime_r(&t, &t);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
    return buf;
}

bool ping_once(const std::string& host, std::uint16_t port, std::int64_t timeout_ms) {
    socket_t s = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == kInvalidSocket) return false;
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1) {
        close_socket(s);
        return false;
    }
    // Set non-blocking.
#if defined(_WIN32)
    u_long mode = 1;
    ioctlsocket(s, FIONBIO, &mode);
#else
    int flags = fcntl(s, F_GETFL, 0);
    fcntl(s, F_SETFL, flags | O_NONBLOCK);
#endif
    int rc = ::connect(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    if (rc == 0) {
        close_socket(s);
        return true;
    }
#if defined(_WIN32)
    if (rc == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) {
        close_socket(s);
        return false;
    }
#else
    if (rc != 0 && errno != EINPROGRESS) {
        close_socket(s);
        return false;
    }
#endif
    fd_set wfds; FD_ZERO(&wfds); FD_SET(s, &wfds);
    timeval tv{};
    tv.tv_sec  = static_cast<long>(timeout_ms / 1000);
    tv.tv_usec = static_cast<long>((timeout_ms % 1000) * 1000);
    int sel = ::select(static_cast<int>(s) + 1, nullptr, &wfds, nullptr, &tv);
    bool ok = false;
    if (sel > 0 && FD_ISSET(s, &wfds)) {
        int err = 0; socklen_t len = sizeof(err);
        getsockopt(s, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&err), &len);
        ok = (err == 0);
    }
    close_socket(s);
    return ok;
}
}  // namespace

StatusPinger::StatusPinger(const Config& cfg) : cfg_(cfg) {}

StatusPinger::~StatusPinger() { stop(); }

void StatusPinger::start() {
    if (running_.exchange(true)) return;
#if defined(_WIN32)
    WSADATA wsadata;
    WSAStartup(MAKEWORD(2, 2), &wsadata);
#endif
    worker_ = std::thread([this]() { run_loop(); });
}

void StatusPinger::stop() {
    if (!running_.exchange(false)) return;
    if (worker_.joinable()) worker_.join();
#if defined(_WIN32)
    WSACleanup();
#endif
}

void StatusPinger::run_loop() {
    using namespace std::chrono_literals;
    while (running_.load()) {
        StatusSnapshot s;
        s.login = ping_once("127.0.0.1", cfg_.game_login_port, 800) ? "up" : "down";
        s.agent = ping_once("127.0.0.1", cfg_.game_agent_port, 800) ? "up" : "down";
        s.map   = ping_once("127.0.0.1", cfg_.game_map_port,   800) ? "up" : "down";
        s.last_check_at = std::chrono::system_clock::now();
        s.version = cfg_.version;
        {
            std::lock_guard<std::mutex> g(mu_);
            snap_ = s;
        }
        for (int i = 0; i < 50 && running_.load(); ++i) {
            std::this_thread::sleep_for(100ms);
        }
    }
}

StatusSnapshot StatusPinger::snapshot() const {
    std::lock_guard<std::mutex> g(mu_);
    return snap_;
}

void register_status_routes(HttpServer& server, const Config& cfg, StatusPinger& pinger) {
    server.get_json("/api/status", [&cfg, &pinger]() {
        auto s = pinger.snapshot();
        nlohmann::json body = {
            {"login", s.login},
            {"agent", s.agent},
            {"map",   s.map},
            {"online_count", s.online_count >= 0 ? nlohmann::json(s.online_count) : nlohmann::json(nullptr)},
            {"last_check_at", now_iso8601()},
            {"version", cfg.version},
        };
        return body;
    });

    MLOG_INFO("[portal] /api/status route registered (background ping 5s)");
}

}  // namespace mxh::portal
