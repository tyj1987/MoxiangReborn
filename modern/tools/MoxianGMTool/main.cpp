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
#include <optional>
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
#include "gm_item_catalog.hpp"
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

    static void dump_string(std::ostringstream& ss, const std::string& value) {
        static constexpr char hex[] = "0123456789abcdef";
        ss << '"';
        for (const unsigned char ch : value) {
            switch (ch) {
                case '"': ss << "\\\""; break;
                case '\\': ss << "\\\\"; break;
                case '\b': ss << "\\b"; break;
                case '\f': ss << "\\f"; break;
                case '\n': ss << "\\n"; break;
                case '\r': ss << "\\r"; break;
                case '\t': ss << "\\t"; break;
                default:
                    if (ch < 0x20) ss << "\\u00" << hex[ch >> 4] << hex[ch & 0x0f];
                    else ss << static_cast<char>(ch);
            }
        }
        ss << '"';
    }

    void dump_impl(std::ostringstream& ss, int indent) const {
        switch (type) {
            case Null: ss << "null"; break;
            case Bool: ss << (bool_value ? "true" : "false"); break;
            case Number: ss << number_value; break;
            case String: dump_string(ss, string_value); break;
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
                    ss << std::string(indent + 2, ' ');
                    dump_string(ss, key);
                    ss << ": ";
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
    std::map<std::string, std::string> query;
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
        std::string raw;
        raw.reserve(8192);
        char buffer[4096];
        std::size_t expected_size = 0;
        while (raw.size() < 1024 * 1024) {
            const int bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);
            if (bytes_read <= 0) break;
            raw.append(buffer, static_cast<std::size_t>(bytes_read));
            const auto header_end = raw.find("\r\n\r\n");
            if (header_end != std::string::npos && expected_size == 0) {
                const auto length_header = raw.find("Content-Length:");
                std::size_t content_length = 0;
                if (length_header != std::string::npos && length_header < header_end) {
                    const auto value_start = length_header + 15;
                    content_length = std::stoul(raw.substr(value_start));
                }
                expected_size = header_end + 4 + content_length;
            }
            if (expected_size != 0 && raw.size() >= expected_size) break;
        }
        if (raw.empty() || raw.size() >= 1024 * 1024) {
#ifdef _WIN32
            closesocket(client_fd);
#else
            close(client_fd);
#endif
            return;
        }

        HttpRequest request = parse_request(raw);
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
        const auto query_pos = request.path.find('?');
        if (query_pos != std::string::npos) {
            std::string query_text = request.path.substr(query_pos + 1);
            request.path.resize(query_pos);
            std::istringstream query_stream(query_text);
            std::string pair;
            while (std::getline(query_stream, pair, '&')) {
                const auto equals = pair.find('=');
                request.query[pair.substr(0, equals)] = equals == std::string::npos
                    ? std::string{} : pair.substr(equals + 1);
            }
        }

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

        const auto body_pos = raw.find("\r\n\r\n");
        if (body_pos != std::string::npos) request.body = raw.substr(body_pos + 4);

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
    GMTool(mxh::db::IDbAdapter& db, const mxh::gm::ItemCatalog& item_catalog)
        : db_(db), repository_(db), item_catalog_(item_catalog) {}

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
        std::size_t offset = 0;
        std::size_t limit = 100;
        try {
            if (const auto it = request.query.find("offset"); it != request.query.end()) offset = std::stoul(it->second);
            if (const auto it = request.query.find("limit"); it != request.query.end()) limit = std::stoul(it->second);
        } catch (...) {
            return bad_request("offset and limit must be non-negative integers");
        }
        if (limit == 0 || limit > 500) return bad_request("limit must be between 1 and 500");
        std::optional<std::uint32_t> requested_id;
        try {
            if (const auto it = request.query.find("id"); it != request.query.end()) requested_id = std::stoul(it->second);
        } catch (...) {
            return bad_request("id must be an integer");
        }
        JsonValue items;
        items.type = JsonValue::Array;
        std::size_t matched = 0;
        for (const auto& item : item_catalog_.items()) {
            if (requested_id && item.ItemIdx != *requested_id) continue;
            if (matched++ < offset) continue;
            if (items.array_value.size() == limit) break;
            JsonValue entry;
            entry.type = JsonValue::Object;
            entry.object_value["id"] = JsonValue(static_cast<double>(item.ItemIdx));
            entry.object_value["name"] = JsonValue(mxh::gm::item_name_utf8(item));
            entry.object_value["kind"] = JsonValue(static_cast<double>(item.ItemKind));
            entry.object_value["type"] = JsonValue(static_cast<double>(item.ItemType));
            entry.object_value["buy_price"] = JsonValue(static_cast<double>(item.BuyPrice));
            entry.object_value["sell_price"] = JsonValue(static_cast<double>(item.SellPrice));
            entry.object_value["rarity"] = JsonValue(static_cast<double>(item.Rarity));
            entry.object_value["required_level"] = JsonValue(static_cast<double>(item.LimitLevel));
            items.array_value.push_back(std::move(entry));
        }
        HttpResponse response;
        response.body = items.dump();
        response.headers["Content-Type"] = "application/json; charset=utf-8";
        return response;
    }

    HttpResponse give_item(const HttpRequest& request) {
        size_t pos = 0; JsonValue body;
        try { body = JsonValue::parse(request.body, pos); }
        catch (...) { return bad_request("invalid JSON body"); }
        const auto string_field = [&](const char* name) -> std::string {
            const auto it = body.object_value.find(name);
            return it == body.object_value.end() || it->second.type != JsonValue::String ? std::string{} : it->second.string_value;
        };
        const auto number_field = [&](const char* name) -> std::int64_t {
            const auto it = body.object_value.find(name);
            return it == body.object_value.end() || it->second.type != JsonValue::Number ? 0 : static_cast<std::int64_t>(it->second.number_value);
        };
        const auto key = string_field("idempotency_key");
        const auto actor = string_field("actor");
        const auto reason = string_field("reason");
        const auto character_id = number_field("character_id");
        const auto item_id = number_field("item_id");
        const auto count = number_field("count");
        if (key.empty() || actor.empty() || reason.empty() || character_id <= 0 || item_id <= 0 || count <= 0) {
            return bad_request("idempotency_key, character_id, item_id, count, actor and reason are required");
        }
        if (key.size() > 128 || actor.size() > 64 || reason.size() > 256 || count > 9999 || item_id > 65535) {
            return bad_request("grant field exceeds limit");
        }
        bool known_item = false;
        for (const auto& item : item_catalog_.items()) if (item.ItemIdx == item_id) { known_item = true; break; }
        if (!known_item) return bad_request("unknown authoritative item id");
        std::string account;
        const auto character = repository_.find_account_for_character(character_id, account);
        if (!character.ok()) return database_error(character);
        if (account.empty()) { HttpResponse r; r.status=404; r.status_text="Not Found"; r.body="{\"error\":\"character not found\"}"; return r; }
        bool already_exists = false;
        const auto result = repository_.enqueue_item_grant(key, character_id, item_id, count, actor, reason, already_exists);
        if (!result.ok()) return database_error(result);
        HttpResponse response;
        response.status = already_exists ? 200 : 202;
        response.status_text = already_exists ? "OK" : "Accepted";
        response.body = already_exists ? "{\"status\":\"already_queued\"}" : "{\"status\":\"pending\"}";
        return response;
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
        size_t pos = 0;
        JsonValue body;
        try { body = JsonValue::parse(request.body, pos); }
        catch (...) { return bad_request("invalid JSON body"); }
        if (body.type != JsonValue::Object) return bad_request("JSON object is required");
        const auto field = [&](const char* name) -> std::string {
            const auto it = body.object_value.find(name);
            return it == body.object_value.end() || it->second.type != JsonValue::String
                ? std::string{} : it->second.string_value;
        };
        const std::string type = field("type");
        const std::string title = field("title");
        const std::string starts_at = field("starts_at");
        const std::string ends_at = field("ends_at");
        const std::string actor = field("actor");
        const auto config_it = body.object_value.find("config");
        if (type.empty() || title.empty() || starts_at.empty() || ends_at.empty() || actor.empty()
            || config_it == body.object_value.end() || config_it->second.type != JsonValue::Object) {
            return bad_request("type, title, starts_at, ends_at, actor and config object are required");
        }
        if (type != "experience_multiplier" && type != "drop_multiplier" && type != "announcement") {
            return bad_request("unsupported event type");
        }
        const auto valid_time = [](const std::string& value) {
            return value.size() == 20 && value[4] == '-' && value[7] == '-'
                && value[10] == 'T' && value[13] == ':' && value[16] == ':' && value[19] == 'Z';
        };
        if (!valid_time(starts_at) || !valid_time(ends_at) || starts_at >= ends_at) {
            return bad_request("UTC time window must use YYYY-MM-DDTHH:MM:SSZ and start before end");
        }
        if (title.size() > 128 || actor.size() > 64 || config_it->second.dump().size() > 4096) {
            return bad_request("event field exceeds size limit");
        }
        const auto created = repository_.create_event(type, title, config_it->second.dump(),
                                                       starts_at, ends_at, actor);
        if (!created.ok()) return database_error(created);
        HttpResponse response;
        response.status = 201; response.status_text = "Created";
        response.body = "{\"success\":true}";
        return response;
    }

    HttpResponse get_events(const HttpRequest&) {
        mxh::db::ResultSet rows;
        const auto result = repository_.list_events(rows);
        if (!result.ok()) return database_error(result);
        JsonValue events; events.type = JsonValue::Array;
        for (const auto& row : rows.rows) {
            if (row.size() < 10) continue;
            JsonValue event; event.type = JsonValue::Object;
            event.object_value["id"] = JsonValue(static_cast<double>(std::get<std::int64_t>(row[0])));
            event.object_value["type"] = JsonValue(std::get<std::string>(row[1]));
            event.object_value["title"] = JsonValue(std::get<std::string>(row[2]));
            event.object_value["config_json"] = JsonValue(std::get<std::string>(row[3]));
            event.object_value["starts_at"] = JsonValue(std::get<std::string>(row[4]));
            event.object_value["ends_at"] = JsonValue(std::get<std::string>(row[5]));
            event.object_value["enabled"] = JsonValue(std::get<std::int64_t>(row[6]) != 0);
            event.object_value["created_by"] = JsonValue(std::get<std::string>(row[7]));
            events.array_value.push_back(std::move(event));
        }
        HttpResponse response; response.body = events.dump(); return response;
    }

    HttpResponse event_action(const HttpRequest& request) {
        const auto action_slash = request.path.rfind('/');
        const auto id_slash = request.path.rfind('/', action_slash - 1);
        std::int64_t event_id = 0;
        try { event_id = std::stoll(request.path.substr(id_slash + 1, action_slash - id_slash - 1)); }
        catch (...) { return bad_request("invalid event id"); }
        size_t pos = 0; JsonValue body;
        try { body = JsonValue::parse(request.body, pos); }
        catch (...) { return bad_request("invalid JSON body"); }
        const auto get = [&](const char* name) -> std::string {
            const auto it = body.object_value.find(name);
            return it == body.object_value.end() || it->second.type != JsonValue::String ? std::string{} : it->second.string_value;
        };
        const auto actor = get("actor"); const auto reason = get("reason");
        if (actor.empty() || reason.empty()) return bad_request("actor and reason are required");
        const auto result = repository_.disable_event(event_id, actor, reason);
        if (!result.ok()) return database_error(result);
        HttpResponse response; response.body = "{\"success\":true}"; return response;
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
    const mxh::gm::ItemCatalog& item_catalog_;
};

// ============================================================================
// Main
// ============================================================================

int main(int argc, char* argv[]) {
    int port = 8080;
    std::string bind_address = "127.0.0.1";
    std::string token;
    std::string db_config;
    std::string item_list_path;

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
        } else if (arg == "--item-list" && i + 1 < argc) {
            item_list_path = argv[++i];
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
    if (item_list_path.empty()) {
        std::cerr << "FATAL: --item-list <ItemList.bin> is required" << std::endl;
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

    mxh::gm::ItemCatalog item_catalog;
    std::string item_error;
    if (!item_catalog.load(item_list_path, item_error)) {
        std::cerr << "FATAL: authoritative item catalog failed: " << item_error << std::endl;
        return 2;
    }
    std::cout << "Loaded authoritative item catalog: " << item_catalog.items().size()
              << " items, " << item_catalog.parse_errors() << " rejected rows" << std::endl;
    GMTool gm_tool(*db, item_catalog);
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
    server.get("/api/events", [&](const HttpRequest& req) {
        return gm_tool.get_events(req);
    });
    server.post("/api/events/:id/disable", [&](const HttpRequest& req) {
        return gm_tool.event_action(req);
    });

    server.get("/api/audit", [&](const HttpRequest& req) {
        return gm_tool.get_audit(req);
    });

    std::cout << "Starting Moxian GM Tool..." << std::endl;
    server.start();

    return 0;
}
