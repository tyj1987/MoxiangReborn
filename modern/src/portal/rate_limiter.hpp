// modern/src/portal/rate_limiter.hpp
// Token-bucket rate limiter: per-IP, per-endpoint, in-memory, thread-safe.

#pragma once

#include <atomic>
#include <chrono>
#include <shared_mutex>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace mxh::portal {

// Named rate-limit policy applied to an endpoint group.
struct RateLimitPolicy {
    std::uint32_t max_tokens = 10;     // bucket size
    std::uint32_t refill_per_minute = 10;  // tokens added per minute
};

// Standard policies used across the portal.
struct RateLimits {
    static constexpr RateLimitPolicy login   = {10, 10};  // 10/min
    static constexpr RateLimitPolicy register_ = {5, 5};   // 5/min
    static constexpr RateLimitPolicy general  = {60, 60};  // 60/min (read endpoints)
    static constexpr RateLimitPolicy strict   = {5, 5};   // 5/min (sensitive ops)
};

// RateLimiter — token bucket per IP per policy.
// Thread-safe: uses a mutex per bucket map and atomic ticks inside each bucket.
class RateLimiter {
public:
    explicit RateLimiter(std::chrono::minutes cleanup_interval = std::chrono::minutes{5})
        : cleanup_interval_(cleanup_interval) {}

    // Result of a check.
    enum class Result {
        Allowed,    // token consumed, request may proceed
        Rejected,   // no tokens left
    };

    struct Decision {
        Result result;
        std::uint32_t remaining;   // tokens left after this request
        std::chrono::seconds retry_after;
    };

    // Check (and consume) one token for client_ip under policy.
    // Returns Decision with result + metadata for Retry-After header.
    Decision check(std::string_view client_ip,
                   std::string_view endpoint,
                   const RateLimitPolicy& policy);

    // How many seconds until at least 1 token is available for client_ip/endpoint.
    // Returns nullopt if client_ip has never been seen.
    std::optional<std::chrono::seconds> retry_after(std::string_view client_ip,
                                                      std::string_view endpoint) const;

    // Remove entries with last_access older than max_age.
    std::size_t cleanup(std::chrono::minutes max_age);

    // Total number of tracked IP buckets (for monitoring).
    std::size_t bucket_count() const;

private:
    struct Bucket {
        std::atomic<std::uint64_t> tokens;
        std::atomic<std::uint64_t> last_refill_ts;  // seconds since epoch
        std::atomic<std::uint64_t> last_access_ts;  // seconds since epoch
        std::uint32_t max_tokens = 0;
        std::uint32_t refill_per_minute = 0;
    };

    std::string make_key(std::string_view ip, std::string_view endpoint) const;
    Bucket& get_or_create_bucket(std::string_view ip,
                                  std::string_view endpoint,
                                  const RateLimitPolicy& policy);
    void refill_bucket(Bucket& b, std::uint64_t now_sec) const;

    const std::chrono::minutes cleanup_interval_;
    mutable std::shared_mutex mutex_;
    mutable std::unordered_map<std::string, Bucket> buckets_;
};

}  // namespace mxh::portal
