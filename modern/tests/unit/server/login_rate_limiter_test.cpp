// login_rate_limiter_test.cpp - LoginRateLimiter sliding-window tests.
//
// Scope: pure in-process limiter; no DB, no network, no time flakiness.
// The tests inject a deterministic virtual clock via advance() and assert
// the exact behaviour the LoginHandler relies on:
//   - per-IP admit_connection caps at connections_per_window
//   - per-account admit_login_attempt tracks failures over failure_window
//   - record_login_success resets the failure counter
//   - sliding window: an old failure outside the window must not count
//   - failure throttle exists but does not block (advisory metric)

#include "mxh/server/login_rate_limiter.hpp"

#include <gtest/gtest.h>
#include <chrono>

namespace mxh::server {
namespace {

class VirtualClock {
public:
    LoginRateLimiter::TimePoint now() const { return now_; }
    void advance(std::chrono::milliseconds delta) { now_ += delta; }
private:
    LoginRateLimiter::TimePoint now_{};
};

TEST(LoginRateLimiter, AdmitsUpToTheConfiguredConnectionBudget) {
    LoginRateLimiter::Config cfg;
    cfg.connections_per_window = 3;
    cfg.connection_window = std::chrono::seconds(10);
    VirtualClock clock;
    LoginRateLimiter limiter(cfg, [&clock]() { return clock.now(); });
    EXPECT_TRUE(limiter.admit_connection("10.0.0.1"));
    EXPECT_TRUE(limiter.admit_connection("10.0.0.1"));
    EXPECT_TRUE(limiter.admit_connection("10.0.0.1"));
    EXPECT_FALSE(limiter.admit_connection("10.0.0.1"));
    EXPECT_FALSE(limiter.admit_connection("10.0.0.1"));
    // Different IP is independent.
    EXPECT_TRUE(limiter.admit_connection("10.0.0.2"));
}

TEST(LoginRateLimiter, ConnectionBudgetSlidesAfterWindowExpires) {
    LoginRateLimiter::Config cfg;
    cfg.connections_per_window = 2;
    cfg.connection_window = std::chrono::seconds(10);
    VirtualClock clock;
    LoginRateLimiter limiter(cfg, [&clock]() { return clock.now(); });
    EXPECT_TRUE(limiter.admit_connection("10.0.0.1"));
    EXPECT_TRUE(limiter.admit_connection("10.0.0.1"));
    EXPECT_FALSE(limiter.admit_connection("10.0.0.1"));
    clock.advance(std::chrono::seconds(11));
    EXPECT_TRUE(limiter.admit_connection("10.0.0.1"));
}

TEST(LoginRateLimiter, LoginAttemptCountedAgainstFailureBudget) {
    LoginRateLimiter::Config cfg;
    cfg.failures_per_window = 3;
    cfg.failure_window = std::chrono::seconds(60);
    VirtualClock clock;
    LoginRateLimiter limiter(cfg, [&clock]() { return clock.now(); });
    EXPECT_TRUE(limiter.admit_login_attempt("alice"));
    EXPECT_TRUE(limiter.admit_login_attempt("alice"));
    EXPECT_TRUE(limiter.admit_login_attempt("alice"));
    EXPECT_TRUE(limiter.admit_login_attempt("alice")); // admit_login_attempt itself never blocks
    limiter.record_login_failure("alice");
    limiter.record_login_failure("alice");
    limiter.record_login_failure("alice");
    EXPECT_FALSE(limiter.admit_login_attempt("alice"));
}

TEST(LoginRateLimiter, FailureWindowSlidesOutOldFailures) {
    LoginRateLimiter::Config cfg;
    cfg.failures_per_window = 2;
    cfg.failure_window = std::chrono::seconds(30);
    VirtualClock clock;
    LoginRateLimiter limiter(cfg, [&clock]() { return clock.now(); });
    limiter.record_login_failure("alice");
    limiter.record_login_failure("alice");
    EXPECT_FALSE(limiter.admit_login_attempt("alice"));
    clock.advance(std::chrono::seconds(31));
    EXPECT_TRUE(limiter.admit_login_attempt("alice"));
}

TEST(LoginRateLimiter, RecordLoginSuccessResetsFailureCounter) {
    LoginRateLimiter limiter;
    limiter.record_login_failure("alice");
    limiter.record_login_failure("alice");
    limiter.record_login_failure("alice");
    limiter.record_login_failure("alice");
    limiter.record_login_failure("alice");
    // Default budget is 5; we are over.
    EXPECT_FALSE(limiter.admit_login_attempt("alice"));
    limiter.record_login_success("alice");
    EXPECT_TRUE(limiter.admit_login_attempt("alice"));
}

TEST(LoginRateLimiter, ConcurrentConnectionAdmitsDoNotExceedBudget) {
    LoginRateLimiter::Config cfg;
    cfg.connections_per_window = 100;
    cfg.connection_window = std::chrono::seconds(60);
    LoginRateLimiter limiter(cfg);
    constexpr int kThreads = 8;
    constexpr int kAttempts = 200;
    int admitted = 0;
    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    std::mutex admit_mu;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&]() {
            int local = 0;
            for (int i = 0; i < kAttempts; ++i) {
                if (limiter.admit_connection("10.0.0.9")) ++local;
            }
            std::lock_guard lock(admit_mu);
            admitted += local;
        });
    }
    for (auto& t : threads) t.join();
    EXPECT_EQ(admitted, 100);
    const auto s = limiter.stats();
    EXPECT_EQ(s.total_admitted_connections, 100u);
    EXPECT_EQ(s.total_rejected_connections,
              static_cast<std::uint64_t>(kThreads * kAttempts - 100));
}

TEST(LoginRateLimiter, StatsReportTrackedBucketsAndCounters) {
    LoginRateLimiter limiter;
    limiter.admit_connection("10.0.0.1");
    limiter.admit_connection("10.0.0.2");
    limiter.record_login_failure("alice");
    limiter.record_login_failure("bob");
    auto s = limiter.stats();
    EXPECT_EQ(s.tracked_ips, 2u);
    EXPECT_EQ(s.tracked_accounts, 2u);
    EXPECT_EQ(s.total_admitted_connections, 2u);
    EXPECT_EQ(s.total_recorded_failures, 2u);
}

TEST(LoginRateLimiter, ClearForgetsEverything) {
    LoginRateLimiter limiter;
    limiter.admit_connection("10.0.0.1");
    limiter.record_login_failure("alice");
    limiter.clear();
    auto s = limiter.stats();
    EXPECT_EQ(s.tracked_ips, 0u);
    EXPECT_EQ(s.tracked_accounts, 0u);
    EXPECT_EQ(s.total_admitted_connections, 0u);
    EXPECT_EQ(s.total_recorded_failures, 0u);
    EXPECT_TRUE(limiter.admit_connection("10.0.0.1"));
}

}  // namespace
}  // namespace mxh::server