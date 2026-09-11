#include "mxh/server/login_rate_limiter.hpp"

#include <algorithm>
#include <utility>

namespace mxh::server {

namespace {
LoginRateLimiter::TimePoint default_now() {
    return std::chrono::steady_clock::now();
}
}  // namespace

LoginRateLimiter::LoginRateLimiter(Config config, ClockFn clock)
    : config_(std::move(config)),
      clock_(clock ? std::move(clock) : ClockFn{&default_now}) {}

void LoginRateLimiter::advance(std::chrono::milliseconds delta) {
    std::lock_guard<std::mutex> lock(mu_);
    // The clock is monotonic. Tests rely on a virtual clock; production
    // never calls this.
    const auto offset = delta;
    if (ip_connections_.empty() && account_failures_.empty()) return;
    for (auto& [key, stamps] : ip_connections_) {
        for (auto& stamp : stamps) stamp += offset;
    }
    for (auto& [key, stamps] : account_failures_) {
        for (auto& stamp : stamps) stamp += offset;
    }
}

void LoginRateLimiter::clear() {
    std::lock_guard<std::mutex> lock(mu_);
    ip_connections_.clear();
    account_failures_.clear();
    stats_ = Stats{};
}

bool LoginRateLimiter::admit_connection(std::string_view remote_addr) {
    std::lock_guard<std::mutex> lock(mu_);
    const bool ok = admit_connection_locked(remote_addr);
    if (ok) ++stats_.total_admitted_connections;
    else ++stats_.total_rejected_connections;
    return ok;
}

bool LoginRateLimiter::admit_login_attempt(std::string_view account) {
    std::lock_guard<std::mutex> lock(mu_);
    const bool ok = admit_login_attempt_locked(account);
    if (ok) ++stats_.total_admitted_logins;
    else ++stats_.total_rejected_logins;
    return ok;
}

void LoginRateLimiter::record_login_success(std::string_view account) {
    std::lock_guard<std::mutex> lock(mu_);
    account_failures_.erase(std::string(account));
}

void LoginRateLimiter::record_login_failure(std::string_view account) {
    std::lock_guard<std::mutex> lock(mu_);
    const auto now = clock_();
    auto& stamps = account_failures_[std::string(account)];
    prune_locked(stamps, now, config_.failure_window);
    stamps.push_back(now);
    ++stats_.total_recorded_failures;
}

bool LoginRateLimiter::admit_connection_locked(std::string_view remote_addr) {
    const auto now = clock_();
    auto& stamps = ip_connections_[std::string(remote_addr)];
    prune_locked(stamps, now, config_.connection_window);
    if (stamps.size() >= config_.connections_per_window) {
        return false;
    }
    stamps.push_back(now);
    return true;
}

bool LoginRateLimiter::admit_login_attempt_locked(std::string_view account) {
    const auto now = clock_();
    auto& stamps = account_failures_[std::string(account)];
    prune_locked(stamps, now, config_.failure_window);
    return stamps.size() < config_.failures_per_window;
}

void LoginRateLimiter::prune_locked(StampList& stamps, TimePoint now,
                                    std::chrono::seconds window) const {
    const auto cutoff = now - window;
    const auto begin = stamps.begin();
    const auto end = std::lower_bound(begin, stamps.end(), cutoff);
    if (begin != end) stamps.erase(begin, end);
}

LoginRateLimiter::Stats LoginRateLimiter::stats() const {
    std::lock_guard<std::mutex> lock(mu_);
    auto copy = stats_;
    copy.tracked_ips = ip_connections_.size();
    copy.tracked_accounts = account_failures_.size();
    return copy;
}

}  // namespace mxh::server