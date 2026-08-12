// MoxianGMTool - Modern GM (Game Master) management tool
//
// Replaces legacy DS_RMTool with a modern C++ HTTP server.
// Provides REST API for:
//   - Player management (ban, mute, kick, teleport)
//   - Item management (give, remove, search)
//   - Server monitoring (status, player count, performance)
//   - Chat moderation (logs, filters)
//   - Event management (create, schedule, monitor)
//
// Usage:
//   MoxianGMTool [--bind 127.0.0.1] [--port 8080] --token-file <path>
//
// API Endpoints:
//   GET  /api/status          - Server status
//   GET  /api/players         - List players
//   POST /api/players/:id/action - Player actions
//   GET  /api/items           - Search items
//   POST /api/items/give      - Give item to player
//   GET  /api/chat/logs       - Chat logs
//   POST /api/events          - Create event

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <thread>
#include <atomic>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <mutex>
#include <cstdlib>

#include "gm_security.hpp"
#include "gm_repository.hpp"
#include "mxh/server/account_moderation.hpp"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

// ============================================================================
// Simple JSON parser (minimal implementation)
// ============================================================================

class JsonValue {
public:
    enum Type { Null, Bool, Number, String, Array, Object };

    Type type = Null;
    bool bool_value = false;
    double number_value = 0;
    std::string string_value;
    std::vector<JsonValue> array_value;
    std::map<std::string, JsonValue> object_value;

    JsonValue() = default;
    JsonValue(bool v) : type(Bool), bool_value(v) {}
    JsonValue(double v) : type(Number), number_value(v) {}
    JsonValue(const std::string& v) : type(String), string_value(v) {}

    static JsonValue parse(const std::string& json, size_t& pos) {
        skip_whitespace(json, pos);
        if (pos >= json.size()) return JsonValue();

        char c = json[pos];
        if (c == '"') return parse_string(json, pos);
        if (c == '{') return parse_object(json, pos);
        if (c == '[') return parse_array(json, pos);
        if (c == 't' || c == 'f') return parse_bool(json, pos);
        if (c == 'n') return parse_null(json, pos);
        return parse_number(json, pos);
    }

    std::string dump(int indent = 0) const {
        std::ostringstream ss;
        dump_impl(ss, indent);
        return ss.str();
    }

private:
    static void skip_whitespace(const std::string& s, size_t& pos) {
        while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\t' || s[pos] == '\n' || s[pos] == '\r'))
            ++pos;
    }

    static JsonValue parse_string(const std::string& s, size_t& pos) {
        ++pos; // skip opening "
        std::string result;
        while (pos < s.size() && s[pos] != '"') {
            if (s[pos] == '\\') {
                ++pos;
                if (pos < s.size()) {
                    switch (s[pos]) {
                        case 'n': result += '\n'; break;
                        case 't': result += '\t'; break;
                        case '\\': result += '\\'; break;
                        case '"': result += '"'; break;
                        default: result += s[pos];
                    }
                }
            } else {
                result += s[pos];
            }
            ++pos;
        }
        ++pos; // skip closing "
        return JsonValue(result);
    }

    static JsonValue parse_number(const std::string& s, size_t& pos) {
        size_t start = pos;
        if (pos < s.size() && s[pos] == '-') ++pos;
        while (pos < s.size() && isdigit(s[pos])) ++pos;
        if (pos < s.size() && s[pos] == '.') {
            ++pos;
            while (pos < s.size() && isdigit(s[pos])) ++pos;
        }
        return JsonValue(std::stod(s.substr(start, pos - start)));
    }

    static JsonValue parse_bool(const std::string& s, size_t& pos) {
        if (s.substr(pos, 4) == "true") { pos += 4; return JsonValue(true); }
        if (s.substr(pos, 5) == "false") { pos += 5; return JsonValue(false); }
        return JsonValue();
    }

    static JsonValue parse_null(const std::string& s, size_t& pos) {
        pos += 4; // skip "null"
        return JsonValue();
    }

    static JsonValue parse_array(const std::string& s, size_t& pos) {
        ++pos; // skip [
        JsonValue result;
        result.type = Array;
        skip_whitespace(s, pos);
        while (pos < s.size() && s[pos] != ']') {
            result.array_value.push_back(parse(s, pos));
            skip_whitespace(s, pos);
            if (pos < s.size() && s[pos] == ',') ++pos;
            skip_whitespace(s, pos);
        }
        ++pos; // skip ]
        return result;
    }

    static JsonValue parse_object(const std::string& s, size_t& pos) {
        ++pos; // skip {
        JsonValue result;
        result.type = Object;
        skip_whitespace(s, pos);
        while (pos < s.size() && s[pos] != '}') {
            JsonValue key = parse_string(s, pos);
            skip_whitespace(s, pos);
            if (pos < s.size() && s[pos] == ':') ++pos;
            skip_whitespace(s, pos);
            JsonValue value = parse(s, pos);
            result.object_value[key.string_value] = value;
            skip_whitespace(s, pos);
            if (pos < s.size() && s[pos] == ',') ++pos;
            skip_whitespace(s, pos);
        }
        ++pos; // skip }
        return result;
    }

    void dump_impl(std::ostringstream& ss, int indent) const {
        switch (type) {
            case Null: ss << "null"; break;
            case Bool: ss << (bool_value ? "true" : "false"); break;
            case Number: ss << number_value; break;
            case String: ss << "\"" << string_value << "\""; break;
            case Array:
                ss << "[\n";
                for (size_t i = 0; i < array_value.size(); ++i) {
                    ss << std::string(indent + 2, ' ');
                    array_value[i].dump_impl(ss, indent + 2);
                    if (i < array_value.size() - 1) ss << ",";
                    ss << "\n";
                }
                ss << std::string(indent, ' ') << "]";
                break;
            case Object:
                ss << "{\n";
                size_t i = 0;
                for (const auto& [key, value] : object_value) {
                    ss << std::string(indent + 2, ' ') << "\"" << key << "\": ";
                    value.dump_impl(ss, indent + 2);
                    if (i < object_value.size() - 1) ss << ",";
                    ss << "\n";
                    ++i;
                }
                ss << std::string(indent, ' ') << "}";
                break;
        }
    }
};

// ============================================================================
// HTTP Server
// ============================================================================

struct HttpRequest {
    std::string method;
    std::string path;
    std::map<std::string, std::string> headers;
    std::string body;
};

struct HttpResponse {
    int status = 200;
    std::string status_text = "OK";
    std::map<std::string, std::string> headers;
    std::string body;
};

using RouteHandler = std::function<HttpResponse(const HttpRequest&)>;

class HttpServer {
public:
    HttpServer(std::string bind_address, int port, std::string auth_token)
        : bind_address_(std::move(bind_address)), port_(port), auth_token_(std::move(auth_token)) {}

    void get(const std::string& path, RouteHandler handler) {
        routes_["GET:" + path] = handler;
    }

    void post(const std::string& path, RouteHandler handler) {
        routes_["POST:" + path] = handler;
    }

    void start() {
#ifdef _WIN32
        using socket_handle = SOCKET;
        constexpr socket_handle invalid_socket = INVALID_SOCKET;
        WSADATA wsa;
        WSAStartup(MAKEWORD(2, 2), &wsa);
#else
        using socket_handle = int;
        constexpr socket_handle invalid_socket = -1;
#endif

        socket_handle server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd == invalid_socket) {
            std::cerr << "Error creating socket" << std::endl;
            return;
        }

        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

        struct sockaddr_in address;
        address.sin_family = AF_INET;
        if (inet_pton(AF_INET, bind_address_.c_str(), &address.sin_addr) != 1) {
            std::cerr << "Invalid IPv4 bind address: " << bind_address_ << std::endl;
            return;
        }
        address.sin_port = htons(port_);

        if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            std::cerr << "Error binding to port " << port_ << std::endl;
            return;
        }

        if (listen(server_fd, 10) < 0) {
            std::cerr << "Error listening" << std::endl;
            return;
        }

        std::cout << "GM Tool server running on http://" << bind_address_ << ':' << port_ << std::endl;
        std::cout << "API endpoints:" << std::endl;
        std::cout << "  GET  /api/status" << std::endl;
        std::cout << "  GET  /api/players" << std::endl;
        std::cout << "  POST /api/players/:id/action" << std::endl;
        std::cout << "  GET  /api/items" << std::endl;
        std::cout << "  POST /api/items/give" << std::endl;
        std::cout << "  GET  /api/chat/logs" << std::endl;
        std::cout << "  POST /api/events" << std::endl;

        while (true) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            socket_handle client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);

            if (client_fd == invalid_socket) {
                std::cerr << "Error accepting connection" << std::endl;
                continue;
            }

            std::thread([this, client_fd]() {
                handle_client(client_fd);
            }).detach();
        }
    }

private:
#ifdef _WIN32
    using socket_handle = SOCKET;
#else
    using socket_handle = int;
#endif
    std::string bind_address_;
    int port_;
    std::string auth_token_;
    std::map<std::string, RouteHandler> routes_;

    void handle_client(socket_handle client_fd) {
        char buffer[4096] = {0};
        int bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

        if (bytes_read <= 0) {
#ifdef _WIN32
            closesocket(client_fd);
#else
            close(client_fd);
#endif
            return;
        }

        HttpRequest request = parse_request(std::string(buffer, bytes_read));
        HttpResponse response = route_request(request);

        std::string response_str = build_response(response);
        send(client_fd, response_str.c_str(), static_cast<int>(response_str.size()), 0);

#ifdef _WIN32
        closesocket(client_fd);
#else
        close(client_fd);
#endif
    }

    HttpRequest parse_request(const std::string& raw) {
        HttpRequest request;
        std::istringstream ss(raw);
        std::string line;

        // Parse request line
        std::getline(ss, line);
        std::istringstream line_ss(line);
        line_ss >> request.method >> request.path;

        // Parse headers
        while (std::getline(ss, line) && line != "\r" && !line.empty()) {
            size_t colon = line.find(':');
            if (colon != std::string::npos) {
                std::string key = line.substr(0, colon);
                std::string value = line.substr(colon + 1);
                // Trim whitespace
                while (!value.empty() && value[0] == ' ') value = value.substr(1);
                while (!value.empty() && value.back() == '\r') value.pop_back();
                request.headers[key] = value;
            }
        }

        // Parse body
        std::string body;
        while (std::getline(ss, line)) {
            body += line;
        }
        request.body = body;

        return request;
    }

    HttpResponse route_request(const HttpRequest& request) {
        if (!mxh::gm::authorize_bearer(request.headers, auth_token_)) {
            HttpResponse response;
            response.status = 401;
            response.status_text = "Unauthorized";
            response.body = "{\"error\":\"unauthorized\"}";
            response.headers["WWW-Authenticate"] = "Bearer";
            return response;
        }
        // Try exact match first
        std::string key = request.method + ":" + request.path;
        if (routes_.count(key)) {
            return routes_[key](request);
        }

        // Try pattern matching
        for (const auto& [route_key, handler] : routes_) {
            if (route_key.substr(0, request.method.size() + 1) == request.method + ":") {
                std::string pattern = route_key.substr(request.method.size() + 1);
                if (match_pattern(pattern, request.path)) {
                    return handler(request);
                }
            }
        }

        // 404 Not Found
        HttpResponse response;
        response.status = 404;
        response.status_text = "Not Found";
        response.body = "{\"error\": \"Not found\"}";
        response.headers["Content-Type"] = "application/json";
        return response;
    }

    bool match_pattern(const std::string& pattern, const std::string& path) {
        // Simple pattern matching with :param support
        size_t pi = 0, pa = 0;
        while (pi < pattern.size() && pa < path.size()) {
            if (pattern[pi] == ':') {
                // Skip parameter name in pattern
                while (pi < pattern.size() && pattern[pi] != '/') ++pi;
                // Skip parameter value in path
                while (pa < path.size() && path[pa] != '/') ++pa;
            } else {
                if (pattern[pi] != path[pa]) return false;
                ++pi;
                ++pa;
            }
        }
        return pi == pattern.size() && pa == path.size();
    }

    std::string build_response(const HttpResponse& response) {
        std::ostringstream ss;
        ss << "HTTP/1.1 " << response.status << " " << response.status_text << "\r\n";
        ss << "Content-Length: " << response.body.size() << "\r\n";
        ss << "Content-Type: application/json\r\n";
        ss << "Cache-Control: no-store\r\n";
        ss << "X-Content-Type-Options: nosniff\r\n";
        for (const auto& [key, value] : response.headers) {
            ss << key << ": " << value << "\r\n";
        }
        ss << "\r\n";
        ss << response.body;
        return ss.str();
    }
};

// ============================================================================
// GM API Handlers
// ============================================================================

class GMTool {
public:
    explicit GMTool(mxh::db::IDbAdapter& db) : db_(db), repository_(db) {}

    HttpResponse get_status(const HttpRequest& request) {
        JsonValue status;
        status.type = JsonValue::Object;
        status.object_value["server_name"] = JsonValue(std::string("Moxian Server"));
        std::vector<mxh::gm::PlayerRecord> players;
        const auto query = repository_.list_players(players);
        if (!query.ok()) return database_error(query);
        status.object_value["player_count"] = JsonValue(static_cast<double>(players.size()));
        status.object_value["max_players"] = JsonValue(1000.0);
        status.object_value["status"] = JsonValue(std::string("running"));

        HttpResponse response;
        response.body = status.dump();
        response.headers["Content-Type"] = "application/json";
        return response;
    }

    HttpResponse get_players(const HttpRequest& request) {
        JsonValue players;
        players.type = JsonValue::Array;

        std::vector<mxh::gm::PlayerRecord> records;
        const auto query = repository_.list_players(records);
        if (!query.ok()) return database_error(query);
        for (const auto& player : records) {
            JsonValue p;
            p.type = JsonValue::Object;
            p.object_value["id"] = JsonValue(static_cast<double>(player.character_id));
            p.object_value["name"] = JsonValue(player.character_name);
            p.object_value["account_id"] = JsonValue(player.account_id);
            p.object_value["level"] = JsonValue(static_cast<double>(player.level));
            p.object_value["gold"] = JsonValue(static_cast<double>(player.money));
            p.object_value["is_banned"] = JsonValue(player.login_blocked);
            players.array_value.push_back(p);
        }

        HttpResponse response;
        response.body = players.dump();
        response.headers["Content-Type"] = "application/json";
        return response;
    }

    HttpResponse player_action(const HttpRequest& request) {
        // Parse player ID from path
        size_t last_slash = request.path.rfind('/');
        size_t prev_slash = request.path.rfind('/', last_slash - 1);
        std::int64_t player_id = std::stoll(request.path.substr(prev_slash + 1, last_slash - prev_slash - 1));

        // Parse action from body
        size_t pos = 0;
        JsonValue body = JsonValue::parse(request.body, pos);
        std::string action = body.object_value.count("action") ? 
                           body.object_value.at("action").string_value : "";
        const std::string actor = body.object_value.count("actor") ? body.object_value.at("actor").string_value : "";
        const std::string reason = body.object_value.count("reason") ? body.object_value.at("reason").string_value : "";
        if (actor.empty() || reason.empty()) return bad_request("actor and reason are required");
        std::string account_id;
        const auto found = repository_.find_account_for_character(player_id, account_id);
        if (!found.ok()) return database_error(found);
        if (account_id.empty()) {
            HttpResponse response;
            response.status = 404;
            response.status_text = "Not Found";
            response.body = "{\"error\": \"Player not found\"}";
            response.headers["Content-Type"] = "application/json";
            return response;
        }

        std::string result;

        if (action == "ban") {
            const auto update = mxh::server::set_account_login_blocked(db_, account_id, true, actor, reason);
            if (!update.ok()) return database_error(update);
            result = "Player banned";
        } else if (action == "unban") {
            const auto update = mxh::server::set_account_login_blocked(db_, account_id, false, actor, reason);
            if (!update.ok()) return database_error(update);
            result = "Player unbanned";
        } else {
            return not_implemented("only persistent ban/unban actions are implemented");
        }

        JsonValue response_json;
        response_json.type = JsonValue::Object;
        response_json.object_value["success"] = JsonValue(true);
        response_json.object_value["message"] = JsonValue(result);

        HttpResponse response;
        response.body = response_json.dump();
        response.headers["Content-Type"] = "application/json";
        return response;
    }

    HttpResponse get_items(const HttpRequest& request) {
        return not_implemented("item catalog API is not connected to the authoritative resource catalog");
    }

    HttpResponse give_item(const HttpRequest& request) {
        return not_implemented("item grants are disabled until inventory transactions are authoritative");
    }

    HttpResponse get_chat_logs(const HttpRequest&) {
        mxh::db::ResultSet rows;
        const auto query = repository_.list_chat(rows);
        if (!query.ok()) return database_error(query);
        JsonValue entries;
        entries.type = JsonValue::Array;
        for (const auto& row : rows.rows) {
            if (row.size() < 5) continue;
            JsonValue entry;
            entry.type = JsonValue::Object;
            entry.object_value["id"] = JsonValue(static_cast<double>(std::get<std::int64_t>(row[0])));
            entry.object_value["character"] = JsonValue(std::get<std::string>(row[1]));
            entry.object_value["channel"] = JsonValue(std::get<std::string>(row[2]));
            entry.object_value["message"] = JsonValue(std::get<std::string>(row[3]));
            entry.object_value["created_at"] = JsonValue(std::get<std::string>(row[4]));
            entries.array_value.push_back(std::move(entry));
        }
        HttpResponse response;
        response.body = entries.dump();
        return response;
    }

    HttpResponse create_event(const HttpRequest& request) {
        return not_implemented("event creation is disabled until scheduler persistence is authoritative");
    }

    HttpResponse get_audit(const HttpRequest&) {
        mxh::db::ResultSet rows;
        const auto query = repository_.list_audit(rows);
        if (!query.ok()) return database_error(query);
        JsonValue entries;
        entries.type = JsonValue::Array;
        for (const auto& row : rows.rows) {
            if (row.size() < 6) continue;
            JsonValue entry;
            entry.type = JsonValue::Object;
            entry.object_value["id"] = JsonValue(static_cast<double>(std::get<std::int64_t>(row[0])));
            entry.object_value["actor"] = JsonValue(std::get<std::string>(row[1]));
            entry.object_value["target_account"] = JsonValue(std::get<std::string>(row[2]));
            entry.object_value["action"] = JsonValue(std::get<std::string>(row[3]));
            entry.object_value["reason"] = JsonValue(std::get<std::string>(row[4]));
            entry.object_value["created_at"] = JsonValue(std::get<std::string>(row[5]));
            entries.array_value.push_back(std::move(entry));
        }
        HttpResponse response;
        response.body = entries.dump();
        return response;
    }

private:
    static HttpResponse database_error(const mxh::db::DbResult& result) {
        HttpResponse response;
        response.status = 500; response.status_text = "Internal Server Error";
        response.body = "{\"error\":\"database operation failed\"}";
        std::cerr << "GM database error: " << result.error_message << std::endl;
        return response;
    }
    static HttpResponse bad_request(const std::string& message) {
        HttpResponse response;
        response.status = 400; response.status_text = "Bad Request";
        response.body = "{\"error\":\"" + message + "\"}";
        return response;
    }
    static HttpResponse not_implemented(const std::string& message) {
        HttpResponse response;
        response.status = 501; response.status_text = "Not Implemented";
        response.body = "{\"error\":\"" + message + "\"}";
        return response;
    }

    mxh::db::IDbAdapter& db_;
    mxh::gm::Repository repository_;
};

// ============================================================================
// Main
// ============================================================================

int main(int argc, char* argv[]) {
    int port = 8080;
    std::string bind_address = "127.0.0.1";
    std::string token;
    std::string db_config;

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else if (arg == "--bind" && i + 1 < argc) {
            bind_address = argv[++i];
        } else if (arg == "--token-file" && i + 1 < argc) {
            std::ifstream input(argv[++i], std::ios::binary);
            if (!input) {
                std::cerr << "FATAL: cannot read GM token file" << std::endl;
                return 2;
            }
            std::getline(input, token);
        } else if (arg == "--db" && i + 1 < argc) {
            db_config = argv[++i];
        }
    }

    if (token.empty()) {
        if (const char* value = std::getenv("MXH_GM_TOKEN")) token = value;
    }
    if (token.size() < 32) {
        std::cerr << "FATAL: MXH_GM_TOKEN or --token-file must provide at least 32 characters" << std::endl;
        return 2;
    }
    if (db_config.empty()) {
        std::cerr << "FATAL: --db <connection-config> is required" << std::endl;
        return 2;
    }
    auto db_cfg = mxh::db::ConnectionConfig::from_kv_string(db_config);
    if (db_cfg.path.empty() && db_cfg.backend == "sqlite") db_cfg.path = db_config;
    auto db = mxh::db::make_adapter(db_cfg.backend);
    if (!db) { std::cerr << "FATAL: unsupported database backend" << std::endl; return 2; }
    const auto connected = db->connect(db_cfg);
    if (!connected.ok()) {
        std::cerr << "FATAL: database connection failed: " << connected.error_message << std::endl;
        return 2;
    }

    GMTool gm_tool(*db);
    HttpServer server(bind_address, port, token);

    // Register routes
    server.get("/api/status", [&](const HttpRequest& req) {
        return gm_tool.get_status(req);
    });

    server.get("/api/players", [&](const HttpRequest& req) {
        return gm_tool.get_players(req);
    });

    server.post("/api/players/:id/action", [&](const HttpRequest& req) {
        return gm_tool.player_action(req);
    });

    server.get("/api/items", [&](const HttpRequest& req) {
        return gm_tool.get_items(req);
    });

    server.post("/api/items/give", [&](const HttpRequest& req) {
        return gm_tool.give_item(req);
    });

    server.get("/api/chat/logs", [&](const HttpRequest& req) {
        return gm_tool.get_chat_logs(req);
    });

    server.post("/api/events", [&](const HttpRequest& req) {
        return gm_tool.create_event(req);
    });

    server.get("/api/audit", [&](const HttpRequest& req) {
        return gm_tool.get_audit(req);
    });

    std::cout << "Starting Moxian GM Tool..." << std::endl;
    server.start();

    return 0;
}
