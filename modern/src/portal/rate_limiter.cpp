// modern/src/portal/rate_limiter.cpp

#include "portal/rate_limiter.hpp"
#include "portal/portal_log.hpp"

#include <chrono>
#include <cstring>
#include <shared_mutex>

namespace mxh::portal {
namespace {

// Monotonic seconds since epoch (UTC).
std::uint64_t now_sec() {
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
}

}  // namespace

std::string RateLimiter::make_key(std::string_view ip,
                                   std::string_view endpoint) const {
    // Key = ip + "|" + endpoint  (endpoint distinguishes /api/auth/login vs /api/auth/register)
    std::string key;
    key.reserve(ip.size() + 1 + endpoint.size());
    key.assign(ip.data(), ip.size());
    key.push_back('|');
    key.append(endpoint.data(), endpoint.size());
    return key;
}

void RateLimiter::refill_bucket(Bucket& b, std::uint64_t now_sec) const {
    auto last = b.last_refill_ts.load(std::memory_order_acquire);
    if (now_sec <= last) return;  // no time has passed

    auto elapsed = now_sec - last;
    // tokens_per_second = refill_per_minute / 60.0
    // Add tokens capped at max_tokens.
    double refill = (static_cast<double>(b.refill_per_minute) / 60.0)
                    * static_cast<double>(elapsed);
    auto current = b.tokens.load(std::memory_order_relaxed);
    auto add = static_cast<std::uint64_t>(refill);
    auto new_tokens = (current + add > b.max_tokens)
                      ? b.max_tokens
                      : current + add;
    b.tokens.store(new_tokens, std::memory_order_release);
    b.last_refill_ts.store(now_sec, std::memory_order_release);
}

RateLimiter::Bucket&
RateLimiter::get_or_create_bucket(std::string_view ip,
                                    std::string_view endpoint,
                                    const RateLimitPolicy& policy) {
    auto key = make_key(ip, endpoint);
    std::unique_lock lock(mutex_);
    auto it = buckets_.find(key);
    if (it != buckets_.end()) {
        // Update refill params in case policy changed (policy is static here, but safe)
        it->second.max_tokens = policy.max_tokens;
        it->second.refill_per_minute = policy.refill_per_minute;
        return it->second;
    }
    auto [new_it, inserted] = buckets_.try_emplace(std::move(key));
    if (inserted) {
        new_it->second.max_tokens = policy.max_tokens;
        new_it->second.refill_per_minute = policy.refill_per_minute;
        new_it->second.tokens.store(policy.max_tokens, std::memory_order_release);
        new_it->second.last_refill_ts.store(now_sec(), std::memory_order_release);
        new_it->second.last_access_ts.store(now_sec(), std::memory_order_release);
    }
    return new_it->second;
}

RateLimiter::Decision
RateLimiter::check(std::string_view client_ip,
                    std::string_view endpoint,
                    const RateLimitPolicy& policy) {
    auto now = now_sec();
    Bucket& b = get_or_create_bucket(client_ip, endpoint, policy);

    // Refill
    refill_bucket(b, now);

    // Consume one token
    auto current = b.tokens.load(std::memory_order_acquire);
    if (current == 0) {
        // When does one token become available again?
        // tokens_needed = 1, refill_rate_per_sec = policy.refill_per_minute / 60.0
        double rate = static_cast<double>(policy.refill_per_minute) / 60.0;
        double seconds_needed = (rate > 0.0) ? (1.0 / rate) : 60.0;
        auto retry = static_cast<std::chrono::seconds>(
            static_cast<std::uint64_t>(seconds_needed) + 1);
        b.last_access_ts.store(now, std::memory_order_release);
        return {Result::Rejected, 0, retry};
    }

    // Try to decrement. Use CAS loop to avoid TOCTOU.
    std::uint64_t desired = current - 1;
    if (b.tokens.compare_exchange_strong(current, desired,
                                          std::memory_order_acq_rel,
                                          std::memory_order_acquire)) {
        b.last_access_ts.store(now, std::memory_order_release);
        return {Result::Allowed, static_cast<std::uint32_t>(desired), std::chrono::seconds{0}};
    }
    // Lost the race — treat as rejected (very rare)
    return {Result::Rejected, 0, std::chrono::seconds{60}};
}

std::optional<std::chrono::seconds>
RateLimiter::retry_after(std::string_view client_ip,
                           std::string_view endpoint) const {
    auto key = make_key(client_ip, endpoint);
    std::shared_lock lock(mutex_);
    auto it = buckets_.find(key);
    if (it == buckets_.end()) return std::nullopt;
    const Bucket& b = it->second;
    auto tokens = b.tokens.load(std::memory_order_acquire);
    if (tokens > 0) return std::chrono::seconds{0};
    // estimate based on refill rate
    double rate = static_cast<double>(b.refill_per_minute) / 60.0;
    double seconds_needed = (rate > 0.0) ? (1.0 / rate) : 60.0;
    return std::chrono::seconds{static_cast<std::uint64_t>(seconds_needed) + 1};
}

std::size_t RateLimiter::cleanup(std::chrono::minutes max_age) {
    auto now = now_sec();
    auto cutoff = now - static_cast<std::uint64_t>(max_age.count()) * 60ULL;
    std::size_t removed = 0;
    std::unique_lock lock(mutex_);
    for (auto it = buckets_.begin(); it != buckets_.end(); ) {
        auto last = it->second.last_access_ts.load(std::memory_order_acquire);
        if (last < cutoff) {
            it = buckets_.erase(it);
            ++removed;
        } else {
            ++it;
        }
    }
    if (removed > 0) {
        MLOG_INFO("[ratelimit] cleaned up %zu stale entries, %zu remain",
                  removed, buckets_.size());
    }
    return removed;
}

std::size_t RateLimiter::bucket_count() const {
    std::shared_lock lock(mutex_);
    return buckets_.size();
}

}  // namespace mxh::portal
