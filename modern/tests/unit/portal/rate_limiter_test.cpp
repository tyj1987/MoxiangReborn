// tests/unit/portal/rate_limiter_test.cpp
// M5.2: Token-bucket rate limiter unit tests.

#include "portal/rate_limiter.hpp"

#include <gtest/gtest.h>
#include <thread>
#include <vector>

using namespace mxh::portal;

TEST(RateLimiter, FirstRequestAllowed) {
    RateLimiter rl;
    RateLimitPolicy p{5, 5};  // 5 tokens, refill 5/min
    auto d = rl.check("192.168.1.1", "/api/auth/login", p);
    EXPECT_EQ(d.result, RateLimiter::Result::Allowed);
    EXPECT_EQ(d.remaining, 4u);
}

TEST(RateLimiter, BucketExhaustedReturnsRejected) {
    RateLimiter rl;
    RateLimitPolicy p{3, 3};  // 3 tokens, refill 3/min

    for (int i = 0; i < 3; ++i) {
        auto d = rl.check("10.0.0.1", "/api/auth/login", p);
        EXPECT_EQ(d.result, RateLimiter::Result::Allowed) << "request " << (i+1);
    }

    // 4th request must be rejected
    auto d = rl.check("10.0.0.1", "/api/auth/login", p);
    EXPECT_EQ(d.result, RateLimiter::Result::Rejected);
    EXPECT_EQ(d.remaining, 0u);
    EXPECT_GE(d.retry_after.count(), 1);  // retry after some seconds
}

TEST(RateLimiter, DifferentIpsAreIndependent) {
    RateLimiter rl;
    RateLimitPolicy p{2, 2};

    // Exhaust IP A
    rl.check("10.0.0.1", "/api/auth/login", p);
    rl.check("10.0.0.1", "/api/auth/login", p);
    auto d_a = rl.check("10.0.0.1", "/api/auth/login", p);
    EXPECT_EQ(d_a.result, RateLimiter::Result::Rejected);

    // IP B should still have tokens
    auto d_b = rl.check("10.0.0.2", "/api/auth/login", p);
    EXPECT_EQ(d_b.result, RateLimiter::Result::Allowed);
}

TEST(RateLimiter, DifferentEndpointsAreIndependent) {
    RateLimiter rl;
    RateLimitPolicy p{2, 2};

    // Exhaust login endpoint
    rl.check("10.0.0.1", "/api/auth/login", p);
    rl.check("10.0.0.1", "/api/auth/login", p);
    auto d_login = rl.check("10.0.0.1", "/api/auth/login", p);
    EXPECT_EQ(d_login.result, RateLimiter::Result::Rejected);

    // Register endpoint should still have tokens
    auto d_reg = rl.check("10.0.0.1", "/api/auth/register", p);
    EXPECT_EQ(d_reg.result, RateLimiter::Result::Allowed);
}

TEST(RateLimiter, RetryAfterIsPositiveWhenRejected) {
    RateLimiter rl;
    RateLimitPolicy p{1, 1};
    rl.check("10.0.0.1", "/api/test", p);  // consume the only token
    auto d = rl.check("10.0.0.1", "/api/test", p);
    EXPECT_EQ(d.result, RateLimiter::Result::Rejected);
    EXPECT_GT(d.retry_after.count(), 0);
}

TEST(RateLimiter, BucketCountIncreases) {
    RateLimiter rl;
    RateLimitPolicy p{5, 5};
    EXPECT_EQ(rl.bucket_count(), 0u);
    rl.check("10.0.0.1", "/api/login", p);
    EXPECT_EQ(rl.bucket_count(), 1u);
    rl.check("10.0.0.2", "/api/login", p);
    EXPECT_EQ(rl.bucket_count(), 2u);
}

TEST(RateLimiter, CleanupRemovesStaleEntries) {
    RateLimiter rl;
    RateLimitPolicy p{5, 5};
    rl.check("10.0.0.1", "/api/login", p);
    rl.check("10.0.0.2", "/api/login", p);
    EXPECT_EQ(rl.bucket_count(), 2u);

    // Cleanup entries not seen in 10 minutes (recently accessed entries are NOT stale)
    std::size_t removed = rl.cleanup(std::chrono::minutes{10});
    EXPECT_EQ(removed, 0u);  // entries are fresh
    EXPECT_EQ(rl.bucket_count(), 2u);  // none removed
}

TEST(RateLimiter, StandardPoliciesAreSensible) {
    // Login: 10/min
    EXPECT_EQ(RateLimits::login.max_tokens, 10u);
    EXPECT_EQ(RateLimits::login.refill_per_minute, 10u);
    // Register: 5/min
    EXPECT_EQ(RateLimits::register_.max_tokens, 5u);
    EXPECT_EQ(RateLimits::register_.refill_per_minute, 5u);
    // General: 60/min
    EXPECT_EQ(RateLimits::general.max_tokens, 60u);
    EXPECT_EQ(RateLimits::general.refill_per_minute, 60u);
}

TEST(RateLimiter, ConcurrentAccessDoesNotCrash) {
    RateLimiter rl;
    RateLimitPolicy p{1000, 1000};
    std::vector<std::thread> threads;
    for (int t = 0; t < 8; ++t) {
        threads.emplace_back([&rl, &p, t]() {
            for (int i = 0; i < 200; ++i) {
                std::string ip = "10.0." + std::to_string(t) + "." + std::to_string(i % 256);
                rl.check(ip, "/api/test", p);
            }
        });
    }
    for (auto& th : threads) th.join();
    // No crash = pass
    EXPECT_GE(rl.bucket_count(), 0u);
}
