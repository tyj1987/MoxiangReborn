// modern/src/portal/http_server.cpp

#include "portal/http_server.hpp"
#include "portal/config.hpp"
#include "portal/portal_log.hpp"

#include <cpp-httplib/httplib.h>
#include <nlohmann/json.hpp>

#include <csignal>
#include <chrono>
#include <fstream>
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

}  // namespace

struct HttpServer::Impl {
    const Config& cfg;
    httplib::Server svr;
    std::atomic<bool> stopping{false};

    explicit Impl(const Config& cfg_) : cfg(cfg_) {}

    static std::string path_join(std::string_view base, std::string_view sub) {
        std::string r;
        r.reserve(base.size() + 1 + sub.size());
        r.assign(base);
        if (!r.empty() && r.back() != '/' && r.back() != '\\') r += '/';
        r += sub;
        return r;
    }
};

HttpServer::HttpServer(const Config& cfg) : p_(std::make_unique<Impl>(cfg)) {
    auto& svr = p_->svr;
    auto& cfg_ = p_->cfg;

    // Health check
    svr.Get("/api/healthz", [&cfg_](const httplib::Request&, httplib::Response& res) {
        set_json_response(res, 200, nlohmann::json{
            {"status", "ok"},
            {"version", cfg_.version},
            {"uptime_seconds", 0}  // placeholder; updated per-request
        });
    });

    // Static file serving — maps /static/* → static_root/
    svr.Get("/static/..*", [&](const httplib::Request& req, httplib::Response& res) {
        // Strip the leading /static/ prefix
        std::string_view target = req.path;
        if (target.starts_with("/static/")) {
            target.remove_prefix(8);  // len("/static/")
        } else {
            target = req.path;  // shouldn't happen
        }

        std::string rel_path(target.data(), target.size());
        std::string full_path = Impl::path_join(cfg_.static_root, rel_path);

        // Basic path traversal guard
        for (char c : rel_path) {
            if (c == '\\' || c == '\0') {
                set_error_json(res, 400, "invalid path");
                return;
            }
        }
        if (rel_path.find("..") != std::string_view::npos) {
            set_error_json(res, 403, "forbidden");
            return;
        }

        std::ifstream ifs(full_path, std::ios::binary | std::ios::ate);
        if (!ifs) {
            set_error_json(res, 404, std::string("not found: ") + std::string(rel_path));
            return;
        }

        auto size = static_cast<std::streamsize>(ifs.tellg());
        ifs.seekg(0);
        res.body.resize(static_cast<std::size_t>(size));
        ifs.read(res.body.data(), size);
        res.status = 200;
        res.set_header("Content-Type", mime_type(full_path));
        res.set_header("Cache-Control", "public, max-age=86400");
    });

    // SPA fallback: non-API, non-static paths → index.html
    // Match "/" and any path that doesn't start with /api/ or /static/ or /download/
    svr.Get("/", [&](const httplib::Request&, httplib::Response& res) {
        std::string full_path = Impl::path_join(cfg_.static_root, "index.html");
        std::ifstream ifs(full_path, std::ios::binary);
        if (!ifs) {
            set_error_json(res, 404, std::string("index not found"));
            return;
        }
        ifs.seekg(0, std::ios::end);
        auto sz = static_cast<std::streamsize>(ifs.tellg());
        ifs.seekg(0);
        res.body.resize(static_cast<std::size_t>(sz));
        ifs.read(res.body.data(), sz);
        res.status = 200;
        res.set_header("Content-Type", "text/html; charset=utf-8");
    });

    // Error handler
    svr.set_error_handler([](const httplib::Request&, httplib::Response& res) {
        if (res.status >= 500) {
            res.status = 500;
            set_error_json(res, 500, "internal server error");
        }
    });

    // Catch-all for unknown API routes (registered LAST — specific routes match first)
    svr.Get("/api/..*", [](const httplib::Request& req, httplib::Response& res) {
        set_error_json(res, 404, std::string("not found: ") + req.path);
    });

    MLOG_INFO("[portal] HTTP server configured: bind=%s port=%u static_root=%s",
              cfg_.bind.c_str(), cfg_.port, cfg_.static_root.c_str());
}

HttpServer::~HttpServer() = default;

void HttpServer::shutdown() {
    p_->stopping.store(true);
    p_->svr.stop();
}

int HttpServer::run() {
    auto& svr = p_->svr;
    auto& cfg_ = p_->cfg;

    // Set up the server
    svr.set_keep_alive_max_count(64);
    svr.set_payload_max_length(10 * 1024 * 1024);  // 10 MB

    MLOG_INFO("[portal] starting HTTP server on %s:%u with %u threads",
              cfg_.bind.c_str(), cfg_.port, cfg_.worker_threads);

    // cpp-httplib's threaded server API (v0.15.x)
    std::atomic<bool> running{true};
    std::thread listener([&]() {
        bool ok = svr.listen(cfg_.bind.c_str(), cfg_.port);
        running.store(false);
        if (!ok) {
            MLOG_ERROR("[portal] listen() returned false on %s:%u", cfg_.bind.c_str(), cfg_.port);
        }
    });

    // Give it a moment to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    if (!running.load()) {
        MLOG_ERROR("[portal] failed to bind to %s:%u — is something already listening?", cfg_.bind.c_str(), cfg_.port);
        return 1;
    }

    MLOG_INFO("[portal] listening on http://%s:%u", cfg_.bind.c_str(), cfg_.port);

    // Wait for shutdown signal
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
    p_->svr.Get(p.c_str(), [h = std::move(handler)](const httplib::Request&, httplib::Response& res) {
        try {
            auto body = h();
            set_json_response(res, 200, std::move(body));
        } catch (const std::exception& e) {
            set_error_json(res, 500, e.what());
        }
    });
}

}  // namespace mxh::portal
