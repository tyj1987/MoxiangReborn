// modern/src/portal/http_server.cpp

#include "portal/http_server.hpp"
#include "portal/config.hpp"
#include "portal/jwt_token.hpp"
#include "portal/rate_limiter.hpp"
#include "portal/portal_log.hpp"

#include <cpp-httplib/httplib.h>
#include <nlohmann/json.hpp>

#include <csignal>
#include <chrono>
#include <cstring>
#include <fstream>
#include <mutex>
#include <shared_mutex>
#include <thread>

namespace mxh::portal {
namespace {

// MIME types for common extensions
const char* mime_type(std::string_view path) {
    if (path.ends_with(".html")) return "text/html; charset=utf-8";
    if (path.ends_with(".css"))  return "text/css; charset=utf-8";
    if (path.ends_with(".js"))   return "application/javascript";
    if (path.ends_with(".json")) return "application/json";
    if (path.ends_with(".png"))  return "image/png";
    if (path.ends_with(".jpg") || path.ends_with(".jpeg")) return "image/jpeg";
    if (path.ends_with(".webp")) return "image/webp";
    if (path.ends_with(".svg"))  return "image/svg+xml";
    if (path.ends_with(".ico"))  return "image/x-icon";
    if (path.ends_with(".woff2")) return "font/woff2";
    if (path.ends_with(".woff"))  return "font/woff";
    if (path.ends_with(".tff"))   return "font/ttf";
    if (path.ends_with(".zip"))   return "application/zip";
    return "application/octet-stream";
}

void set_json_response(httplib::Response& res, int status, const nlohmann::json& body) {
    res.status = status;
    res.set_header("Content-Type", "application/json; charset=utf-8");
    res.set_header("X-Content-Type-Options", "nosniff");
    res.set_header("X-Frame-Options", "DENY");
    res.set_header("Referrer-Policy", "strict-origin-when-cross-origin");
    res.body = body.dump();
}

void set_error_json(httplib::Response& res, int status, const std::string& message) {
    set_json_response(res, status, nlohmann::json{{"error", message}});
}

void set_429(httplib::Response& res, std::chrono::seconds retry_after) {
    res.status = 429;
    res.set_header("Content-Type", "application/json; charset=utf-8");
    res.set_header("Retry-After", std::to_string(retry_after.count()));
    res.set_header("X-Content-Type-Options", "nosniff");
    set_json_response(res, 429,
        nlohmann::json{{"error", "rate limit exceeded"},
                        {"retry_after_seconds", retry_after.count()}});
}

// Extract client IP from request.
// Tries X-Forwarded-For first (for Cloudflare/proxy), then remote_addr.
std::string extract_ip(const httplib::Request& req) {
    // X-Forwarded-For: first IP (client) in comma-separated list
    auto xff = req.get_header_value("X-Forwarded-For");
    if (!xff.empty()) {
        auto comma = xff.find(',');
        if (comma != std::string::npos) {
            return std::string(xff.substr(0, comma));
        }
        return std::string(xff);
    }
    // X-Real-IP (nginx style)
    auto xri = req.get_header_value("X-Real-IP");
    if (!xri.empty()) return std::string(xri);
    // Fallback to remote IP
    return req.remote_addr;
}

// Parse Bearer token from Authorization header.
std::string_view parse_bearer(const httplib::Request& req) {
    auto auth = req.get_header_value("Authorization");
    if (auth.starts_with("Bearer ")) {
        return std::string_view(auth).substr(7);
    }
    return {};
}

}  // namespace

struct HttpServer::Impl {
    const Config& cfg;
    httplib::Server svr;
    std::atomic<bool> stopping{false};
    RateLimiter rate_limiter;
    std::string jwt_secret;

    explicit Impl(const Config& cfg_)
        : cfg(cfg_)
        , rate_limiter(std::chrono::minutes{5})
        , jwt_secret(cfg_.jwt_secret) {}

    static std::string path_join(std::string_view base, std::string_view sub) {
        std::string r;
        r.reserve(base.size() + 1 + sub.size());
        r.assign(base);
        if (!r.empty() && r.back() != '/' && r.back() != '\\') r += '/';
        r += sub;
        return r;
    }

    // Apply rate limit check and send 429 if rejected.
    // Returns true if the request should be rejected (429 sent).
    bool check_rate_limit(httplib::Response& res,
                          const httplib::Request& req,
                          std::string_view endpoint,
                          const RateLimitPolicy& policy) {
        auto ip = extract_ip(req);
        auto decision = rate_limiter.check(ip, endpoint, policy);
        if (decision.result == RateLimiter::Result::Rejected) {
            set_429(res, decision.retry_after);
            return true;
        }
        return false;
    }

    // Verify JWT Bearer token. Returns nullopt on error with response set.
    std::optional<AuthContext> authenticate(httplib::Response& res,
                                             const httplib::Request& req) {
        if (jwt_secret.empty()) {
            set_error_json(res, 500, "JWT secret not configured");
            return std::nullopt;
        }
        auto token = parse_bearer(req);
        if (token.empty()) {
            set_error_json(res, 401, "missing Authorization header");
            return std::nullopt;
        }
        JwtPayload payload;
        auto err = verify_jwt(jwt_secret, token, payload);
        if (err) {
            set_error_json(res, 401, *err);
            return std::nullopt;
        }
        AuthContext ctx;
        ctx.account_name = payload.sub;
        ctx.user_idx = payload.user_idx;
        return ctx;
    }
};

HttpServer::HttpServer(const Config& cfg) : p_(std::make_unique<Impl>(cfg)) {
    auto& svr = p_->svr;
    auto& cfg_ = p_->cfg;

    // ---- Global pre-handler: rate limit ALL requests (generous default) ----
    svr.set_pre_routing_handler([&](const httplib::Request& req, httplib::Response& res) {
        // Apply general rate limit to all routes
        if (p_->check_rate_limit(res, req, req.path, RateLimits::general)) {
            return httplib::Server::HandlerResponse::Handled;
        }
        return httplib::Server::HandlerResponse::Unhandled;
    });

    // ---- /api/healthz (public, still rate-limited globally) ----
    svr.Get("/api/healthz", [&cfg_](const httplib::Request&, httplib::Response& res) {
        set_json_response(res, 200, nlohmann::json{
            {"status", "ok"},
            {"version", cfg_.version},
            {"uptime_seconds", 0}
        });
    });

    // ---- Static file serving ----
    // /static/*  -> direct file lookup under static_root (raw assets)
    // /portal_dist/* -> direct file lookup under static_root/dist (Vue build artifacts)
    // /portal/* -> SPA fallback to static_root/index.html (no path traversal)
    static constexpr std::string_view kStaticPrefix     = "/static/";
    static constexpr std::string_view kPortalDistPrefix = "/portal_dist/";
    static constexpr std::string_view kPortalPrefix     = "/portal";
    static constexpr std::string_view kPortalSlash      = "/portal/";

    auto serve_static = [&](const httplib::Request& req, httplib::Response& res,
                            std::string_view rel) {
        if (rel.find("..") != std::string_view::npos ||
            rel.find('\\')  != std::string_view::npos ||
            rel.find('\0')  != std::string_view::npos) {
            set_error_json(res, 403, "forbidden");
            return;
        }
        std::string full_path = Impl::path_join(cfg_.static_root, std::string(rel));
        std::ifstream ifs(full_path, std::ios::binary | std::ios::ate);
        if (!ifs) {
            set_error_json(res, 404, std::string("not found: ") + std::string(rel));
            return;
        }
        auto size = static_cast<std::streamsize>(ifs.tellg());
        ifs.seekg(0);
        res.body.resize(static_cast<std::size_t>(size));
        ifs.read(res.body.data(), size);
        res.status = 200;
        res.set_header("Content-Type", mime_type(full_path));
        res.set_header("Cache-Control", "public, max-age=86400");
    };

    svr.Get("/static/..*", [&](const httplib::Request& req, httplib::Response& res) {
        std::string_view target = req.path;
        if (target.starts_with(kStaticPrefix)) target.remove_prefix(kStaticPrefix.size());
        serve_static(req, res, target);
    });

    // /portal_dist/<path>  -> static_root/dist/<path>  (Vue build artifacts)
    svr.Get("/portal_dist/..*", [&](const httplib::Request& req, httplib::Response& res) {
        std::string_view target = req.path;
        if (target.starts_with(kPortalDistPrefix)) target.remove_prefix(kPortalDistPrefix.size());
        std::string rel = "dist/";
        rel.append(std::string(target));
        serve_static(req, res, std::string_view(rel));
    });

    // ---- SPA fallback ----
    // /              -> static_root/index.html
    // /portal        -> static_root/index.html
    // /portal/..*    -> static_root/index.html (Vue Router takes over client-side)
    auto serve_index = [&](httplib::Response& res) {
        std::string full_path = Impl::path_join(cfg_.static_root, "index.html");
        std::ifstream ifs(full_path, std::ios::binary);
        if (!ifs) {
            set_error_json(res, 404, "index not found");
            return;
        }
        ifs.seekg(0, std::ios::end);
        auto sz = static_cast<std::streamsize>(ifs.tellg());
        ifs.seekg(0);
        res.body.resize(static_cast<std::size_t>(sz));
        ifs.read(res.body.data(), sz);
        res.status = 200;
        res.set_header("Content-Type", "text/html; charset=utf-8");
        res.set_header("Cache-Control", "no-cache");
    };

    svr.Get("/", [&](const httplib::Request&, httplib::Response& res) {
        serve_index(res);
    });

    // SPA routes for /portal and /portal/..* (no API prefixes)
    svr.Get("/portal", [&](const httplib::Request&, httplib::Response& res) {
        serve_index(res);
    });
    // Match /portal/anything but NOT /portal/api/... (those are handled by route-specific
    // handlers above). The catch-all here is installed AFTER API routes.
    svr.Get("/portal/..*", [&](const httplib::Request& req, httplib::Response& res) {
        std::string_view p = req.path;
        if (p.starts_with(kPortalSlash) && p.substr(kPortalSlash.size()).starts_with("api/")) {
            // /portal/api/... is reserved for proxied API path; serve SPA index
            // so a refresh on /portal/api/* still renders the SPA shell.
            // (In production, Cloudflare routes /portal/api/* to the gateway /api/*.)
        }
        serve_index(res);
    });

    // ---- Error handler ----
    svr.set_error_handler([](const httplib::Request&, httplib::Response& res) {
        if (res.status >= 500) {
            res.status = 500;
            set_error_json(res, 500, "internal server error");
        }
    });

    // ---- Catch-all: unknown API routes ----
    svr.Get("/api/..*", [](const httplib::Request& req, httplib::Response& res) {
        set_error_json(res, 404, std::string("not found: ") + req.path);
    });

    MLOG_INFO("[portal] HTTP server configured: bind=%s port=%u static_root=%s jwt_secret=%s",
              cfg_.bind.c_str(), cfg_.port, cfg_.static_root.c_str(),
              cfg_.jwt_secret.empty() ? "(empty)" : "***");
}

HttpServer::~HttpServer() = default;

void HttpServer::shutdown() {
    p_->stopping.store(true);
    p_->svr.stop();
}

int HttpServer::run() {
    auto& svr = p_->svr;
    auto& cfg_ = p_->cfg;

    svr.set_keep_alive_max_count(64);
    svr.set_payload_max_length(10 * 1024 * 1024);  // 10 MB

    MLOG_INFO("[portal] starting HTTP server on %s:%u with %u threads",
              cfg_.bind.c_str(), cfg_.port, cfg_.worker_threads);

    std::atomic<bool> running{true};
    std::thread listener([&]() {
        bool ok = svr.listen(cfg_.bind.c_str(), cfg_.port);
        running.store(false);
        if (!ok) {
            MLOG_ERROR("[portal] listen() returned false on %s:%u",
                       cfg_.bind.c_str(), cfg_.port);
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (!running.load()) {
        MLOG_ERROR("[portal] failed to bind to %s:%u", cfg_.bind.c_str(), cfg_.port);
        return 1;
    }

    MLOG_INFO("[portal] listening on http://%s:%u", cfg_.bind.c_str(), cfg_.port);

    while (running.load() && !p_->stopping.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    if (p_->stopping.load()) {
        MLOG_INFO("[portal] shutdown requested");
    } else {
        MLOG_INFO("[portal] server stopped");
    }

    listener.join();
    return 0;
}

void HttpServer::get_json(std::string_view path, JsonHandler handler) {
    std::string p(path);
    p_->svr.Get(p.c_str(), [h = std::move(handler)](const httplib::Request& req,
                                                     httplib::Response& res) {
        // Already rate-limited by global pre_routing_handler
        try {
            auto body = h();
            set_json_response(res, 200, std::move(body));
        } catch (const std::exception& e) {
            set_error_json(res, 500, e.what());
        }
    });
}

void HttpServer::get_protected(std::string_view path,
                                const RateLimitPolicy& rate_policy,
                                AuthenticatedHandler auth_handler) {
    std::string p(path);
    p_->svr.Get(p.c_str(), [this, p, rate_policy,
                             h = std::move(auth_handler)](const httplib::Request& req,
                                                          httplib::Response& res) {
        // Rate limit check for this specific endpoint
        if (p_->check_rate_limit(res, req, p, rate_policy)) return;

        // Authenticate
        auto ctx = p_->authenticate(res, req);
        if (!ctx) return;  // authenticate() already set the response

        // Execute handler
        try {
            auto body = h(*ctx);
            set_json_response(res, 200, std::move(body));
        } catch (const std::exception& e) {
            set_error_json(res, 500, e.what());
        }
    });
}

void HttpServer::post_protected(std::string_view path,
                                 const RateLimitPolicy& rate_policy,
                                 AuthenticatedPostHandler handler) {
    std::string p(path);
    p_->svr.Post(p.c_str(), [this, p, rate_policy,
                               h = std::move(handler)](const httplib::Request& req,
                                                        httplib::Response& res) {
        if (p_->check_rate_limit(res, req, p, rate_policy)) return;

        auto ctx = p_->authenticate(res, req);
        if (!ctx) return;

        nlohmann::json body;
        if (!req.body.empty()) {
            try {
                body = nlohmann::json::parse(req.body);
            } catch (const nlohmann::json::parse_error&) {
                set_error_json(res, 400, "invalid JSON body");
                return;
            }
        }

        try {
            auto result = h(*ctx, body);
            set_json_response(res, 200, std::move(result));
        } catch (const std::exception& e) {
            set_error_json(res, 500, e.what());
        }
    });
}

void HttpServer::post_public(std::string_view path,
                             const RateLimitPolicy& rate_policy,
                             PublicPostHandler handler) {
    std::string p(path);
    p_->svr.Post(p.c_str(), [this, p, rate_policy,
                               h = std::move(handler)](const httplib::Request& req,
                                                        httplib::Response& res) {
        if (p_->check_rate_limit(res, req, p, rate_policy)) return;

        nlohmann::json body;
        if (!req.body.empty()) {
            try {
                body = nlohmann::json::parse(req.body);
            } catch (const nlohmann::json::parse_error&) {
                set_error_json(res, 400, "invalid JSON body");
                return;
            }
        }

        try {
            auto result = h(body);
            set_json_response(res, 200, std::move(result));
        } catch (const std::exception& e) {
            set_error_json(res, 500, e.what());
        }
    });
}

// Variant: handler returns (status, body).
void HttpServer::post_public_with_status(std::string_view path,
                                          const RateLimitPolicy& rate_policy,
                                          StatusPublicHandler handler) {
    std::string p(path);
    p_->svr.Post(p.c_str(), [this, p, rate_policy,
                               h = std::move(handler)](const httplib::Request& req,
                                                        httplib::Response& res) {
        if (p_->check_rate_limit(res, req, p, rate_policy)) return;

        nlohmann::json body;
        if (!req.body.empty()) {
            try {
                body = nlohmann::json::parse(req.body);
            } catch (const nlohmann::json::parse_error&) {
                set_error_json(res, 400, "invalid JSON body");
                return;
            }
        }

        try {
            auto result = h(body);
            set_json_response(res, result.first, std::move(result.second));
        } catch (const std::exception& e) {
            set_error_json(res, 500, e.what());
        }
    });
}

void HttpServer::get_protected_with_status(std::string_view path,
                                            const RateLimitPolicy& rate_policy,
                                            StatusAuthHandler handler) {
    std::string p(path);
    p_->svr.Get(p.c_str(), [this, p, rate_policy,
                              h = std::move(handler)](const httplib::Request& req,
                                                       httplib::Response& res) {
        if (p_->check_rate_limit(res, req, p, rate_policy)) return;

        auto ctx = p_->authenticate(res, req);
        if (!ctx) return;

        try {
            auto result = h(*ctx);
            set_json_response(res, result.first, std::move(result.second));
        } catch (const std::exception& e) {
            set_error_json(res, 500, e.what());
        }
    });
}

void HttpServer::post_protected_with_status(std::string_view path,
                                             const RateLimitPolicy& rate_policy,
                                             StatusAuthPostHandler handler) {
    std::string p(path);
    p_->svr.Post(p.c_str(), [this, p, rate_policy,
                                h = std::move(handler)](const httplib::Request& req,
                                                         httplib::Response& res) {
        if (p_->check_rate_limit(res, req, p, rate_policy)) return;

        auto ctx = p_->authenticate(res, req);
        if (!ctx) return;

        nlohmann::json body;
        if (!req.body.empty()) {
            try {
                body = nlohmann::json::parse(req.body);
            } catch (const nlohmann::json::parse_error&) {
                set_error_json(res, 400, "invalid JSON body");
                return;
            }
        }

        try {
            auto result = h(*ctx, body);
            set_json_response(res, result.first, std::move(result.second));
        } catch (const std::exception& e) {
            set_error_json(res, 500, e.what());
        }
    });
}

std::optional<std::chrono::seconds>
HttpServer::rate_check(std::string_view ip,
                       std::string_view path,
                       const RateLimitPolicy& policy) {
    auto decision = p_->rate_limiter.check(ip, path, policy);
    if (decision.result == RateLimiter::Result::Rejected) {
        return decision.retry_after;
    }
    return std::nullopt;
}

void HttpServer::get_dynamic(std::string_view regex_pattern, DynamicGetHandler handler) {
    std::string pat(regex_pattern);
    p_->svr.Get(pat.c_str(), [this, pat, h = std::move(handler)](
        const httplib::Request& req, httplib::Response& res) {
        // Rate limit (general) for dynamic reads.
        if (p_->check_rate_limit(res, req, req.path, RateLimits::general)) return;

        // Authenticate (dynamic GETs are public-by-default unless the per-route
        // helper opts in; for portal we treat detail endpoints as public).
        // We intentionally skip JWT here; for protected variants, callers should
        // authenticate inside the handler using verify_jwt if needed.
        AuthContext ctx{};
        try {
            auto body = h(ctx, req.path);
            set_json_response(res, 200, std::move(body));
        } catch (const std::exception& e) {
            set_error_json(res, 500, e.what());
        }
    });
}

void HttpServer::get_public_dynamic(std::string_view regex_pattern, PublicDynamicGetHandler handler) {
    std::string pat(regex_pattern);
    p_->svr.Get(pat.c_str(), [this, pat, h = std::move(handler)](
        const httplib::Request& req, httplib::Response& res) {
        if (p_->check_rate_limit(res, req, req.path, RateLimits::general)) return;
        try {
            auto body = h(req.path);
            set_json_response(res, 200, std::move(body));
        } catch (const std::exception& e) {
            set_error_json(res, 500, e.what());
        }
    });
}

}  // namespace mxh::portal
