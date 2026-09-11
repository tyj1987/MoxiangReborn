#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace mxh::server {

// Sliding-window rate limiter for the login (Distribute) server.
//
// The class is pure in-process state with no DB dependency so that it can be
// exercised by deterministic unit tests. It tracks two orthogonal budgets:
//
//  1. Per-source-IP connection attempts. Above `connections_per_window`
//     within `connection_window`, the caller must reject the TCP accept
//     (returning false from LoginHandler::on_connect). The intent is to
//     keep a single attacker from saturating the accept loop / handshake.
//
//  2. Per-account login attempts. Above `failures_per_window` failures
//     within `failure_window` the caller must NACK subsequent logins until
//     the window slides past the oldest failure. The intent is to slow
//     credential-stuffing attempts against a known account.
//
// Both budgets are sliding windows of timestamps; old entries are pruned
// lazily on every check, so the limiter stays bounded even under sustained
// attack.
class LoginRateLimiter final {
public:
    using TimePoint = std::chrono::steady_clock::time_point;
    using ClockFn = std::function<TimePoint()>;

    struct Config {
        // Per-source-IP connection budget.
        std::uint32_t connections_per_window = 10;
        std::chrono::seconds connection_window{10};
        // Per-account failure budget.
        std::uint32_t failures_per_window = 5;
        std::chrono::seconds failure_window{60};
        // Optional: throttle response after the first failure so a
        // network-bound attacker cannot grind as fast as the server replies.
        std::chrono::milliseconds failure_throttle{0};
    };

    // Build a limiter with sensible defaults; pass a custom Config to tune
    // for production or test runs. The clock is injectable so tests can
    // advance time deterministically via the `advance()` helper.
    explicit LoginRateLimiter(Config config = {}, ClockFn clock = {});

    // Returns true if a new connection from `remote_addr` is permitted.
    // A successful check registers the connection; the caller does not
    // need to bookkeep the connection explicitly. The connection is
    // forgotten once it ages out of the window.
    bool admit_connection(std::string_view remote_addr);

    // Returns true if a login attempt for `account` is permitted.
    // A successful check does NOT consume a slot; callers follow up with
    // record_login_success() or record_login_failure() based on the
    // outcome.
    bool admit_login_attempt(std::string_view account);

    // Record a successful login. Resets the per-account failure counter so
    // an honest user who finally typed the right password is not penalised.
    void record_login_success(std::string_view account);

    // Record a failed login. Counts toward the per-account failure budget.
    void record_login_failure(std::string_view account);

    // Test-only: deterministic time advance.
    void advance(std::chrono::milliseconds delta);
    // Test-only: forget all per-IP and per-account state.
    void clear();

    // Diagnostics for logging or status endpoints.
    struct Stats {
        std::size_t tracked_ips = 0;
        std::size_t tracked_accounts = 0;
        std::uint64_t total_admitted_connections = 0;
        std::uint64_t total_rejected_connections = 0;
        std::uint64_t total_admitted_logins = 0;
        std::uint64_t total_rejected_logins = 0;
        std::uint64_t total_recorded_failures = 0;
    };
    Stats stats() const;

    const Config& config() const noexcept { return config_; }

private:
    using StampList = std::vector<TimePoint>;

    bool admit_connection_locked(std::string_view remote_addr);
    bool admit_login_attempt_locked(std::string_view account);
    void prune_locked(StampList& stamps, TimePoint now,
                      std::chrono::seconds window) const;

    Config config_;
    ClockFn clock_;
    mutable std::mutex mu_;
    std::unordered_map<std::string, StampList> ip_connections_;
    std::unordered_map<std::string, StampList> account_failures_;
    Stats stats_;
};

}  // namespace mxh::server