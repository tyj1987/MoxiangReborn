// modern/src/portal/http_server.hpp
// Thin wrapper around cpp-httplib; adds middleware and route registration.

#pragma once

#include "portal/config.hpp"
#include <cpp-httplib/httplib.h>
#include <nlohmann/json.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <functional>

namespace mxh::portal {

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

    // Inline convenience: register a JSON-returning GET handler.
    using JsonHandler = std::function<nlohmann::json()>;
    void get_json(std::string_view path, JsonHandler handler);

private:
    struct Impl;
    std::unique_ptr<Impl> p_;
};

}  // namespace mxh::portal
