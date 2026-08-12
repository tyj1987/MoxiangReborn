// modern/src/portal/http_server.hpp
// Thin wrapper around cpp-httplib; adds JWT auth middleware and rate limiting.

#pragma once

#include "portal/config.hpp"
#include "portal/rate_limiter.hpp"
#include <cpp-httplib/httplib.h>
#include <nlohmann/json.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <functional>
#include <optional>

namespace mxh::portal {

// Authenticated user context passed to protected route handlers.
struct AuthContext {
    std::string account_name;  // "sub" from JWT
    std::uint32_t user_idx = 0;
};

// Opaque handle for an HTTP server instance
class HttpServer {
public:
    explicit HttpServer(const Config& cfg);
    ~HttpServer();

    // Prevent copy/move
    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(const HttpServer&) = delete;

    // Start the server (blocks).
    // Returns 0 on normal shutdown, non-zero on error.
    int run();

    // Request a graceful shutdown.
    void shutdown();

    // Register a JSON-returning GET handler (public, still rate-limited globally).
    using JsonHandler = std::function<nlohmann::json()>;
    void get_json(std::string_view path, JsonHandler handler);

    // Register a GET handler that needs JWT auth.
    // auth_handler is called with the verified AuthContext on success.
    // Returns 401 if no/ invalid token; 429 if rate limited.
    using AuthenticatedHandler = std::function<nlohmann::json(const AuthContext&)>;
    void get_protected(std::string_view path,
                       const RateLimitPolicy& rate_policy,
                       AuthenticatedHandler auth_handler);

    // Register a POST handler that needs JWT auth + reads JSON body.
    using AuthenticatedPostHandler = std::function<nlohmann::json(const AuthContext&,
                                                                  const nlohmann::json& body)>;
    void post_protected(std::string_view path,
                        const RateLimitPolicy& rate_policy,
                        AuthenticatedPostHandler handler);

    // Register a POST handler that is public but rate-limited.
    // Returns 429 if rate limit exceeded.
    using PublicPostHandler = std::function<nlohmann::json(const nlohmann::json& body)>;
    void post_public(std::string_view path,
                     const RateLimitPolicy& rate_policy,
                     PublicPostHandler handler);

    // Manual rate-limit check for a given IP + path.
    // Returns nullopt if allowed, or seconds until retry if rejected.
    std::optional<std::chrono::seconds> rate_check(std::string_view ip,
                                                    std::string_view path,
                                                    const RateLimitPolicy& policy);

private:
    struct Impl;
    std::unique_ptr<Impl> p_;
};

}  // namespace mxh::portal
