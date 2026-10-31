#pragma once

// AgentServer command line. main.cpp calls parse_args and passes the
// resulting default_map_num / start_area into AgentHandler.

#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace mxh::agent_cli {

// Map used when --default-map is omitted. Original evidence: client
// CharMake_SelectOption.bin CHINA StartArea = 17 (CharMakeOptions.cpp:99,
// test char_make_options_test.cpp:66) and Linear comment 6e08ed6d on
// TYJ-156 (original MP_CHARACTER_CreateCharacter writes map 17).
inline constexpr std::uint16_t kOmittedDefaultMapNum = 17;

struct Args {
    std::uint16_t port       = 7001;
    std::string   db_backend = "sqlite";  // "sqlite" | "mssql_odbc"
    std::string   db_path    = "modern/build/runtime/moxian_agent.db";
    std::string   db_env;
    std::string   bind_address = "0.0.0.0";
    bool          use_legacy = false;
    bool          use_hsel   = false;
    // Phase 9: optional MapServer connection for GameIn forwarding.
    std::string   map_server_addr;  // empty = no MapServer (stub mode)
    std::uint16_t map_server_port   = 8001;
    std::uint16_t default_map_num   = kOmittedDefaultMapNum;
    // Filled from --start-area, or from default_map_num when that flag is omitted.
    std::uint16_t start_area        = 0;
    bool          start_area_explicit = false;
    struct MapRouteSpec {
        std::uint16_t map_num = 0;
        std::string address;
        std::uint16_t port = 0;
    };
    std::vector<MapRouteSpec> map_routes;
};

inline std::uint16_t parse_default_map(std::string_view token) {
    unsigned long value = 0;
    const auto result = std::from_chars(token.data(), token.data() + token.size(), value);
    if (result.ec != std::errc{} || result.ptr != token.data() + token.size() ||
        value > 0xffu) {
        std::cerr << "invalid --default-map (expected 0..255): " << token << "\n";
        std::exit(2);
    }
    return static_cast<std::uint16_t>(value);
}

inline std::uint16_t parse_u16_option(std::string_view token, const char* option,
                                      std::uint16_t minimum = 1) {
    unsigned long value = 0;
    const auto result = std::from_chars(token.data(), token.data() + token.size(), value);
    if (result.ec != std::errc{} || result.ptr != token.data() + token.size() ||
        value > 0xffffu || value < minimum) {
        std::cerr << "invalid " << option << " (expected " << minimum
                  << "..65535): " << token << "\n";
        std::exit(2);
    }
    return static_cast<std::uint16_t>(value);
}

inline Args parse_args(int argc, char** argv) {
    Args a;
    for (int i = 1; i < argc; ++i) {
        std::string_view s = argv[i];
        if (s == "--port" && i + 1 < argc)
            a.port = parse_u16_option(argv[++i], "--port");
        else if (s == "--db" && i + 1 < argc)
            a.db_path = argv[++i];
        else if (s == "--db-env" && i + 1 < argc)
            a.db_env = argv[++i];
        else if (s == "--backend" && i + 1 < argc)
            a.db_backend = argv[++i];
        else if (s == "--bind-address" && i + 1 < argc)
            a.bind_address = argv[++i];
        else if (s == "--legacy")
            a.use_legacy = true;
        else if (s == "--use-hsel")
            a.use_hsel = true;
        else if (s == "--map-server" && i + 1 < argc) {
            // Parse HOST:PORT format
            std::string spec = argv[++i];
            auto colon = spec.rfind(':');
            if (colon != std::string::npos) {
                a.map_server_addr = spec.substr(0, colon);
                a.map_server_port = parse_u16_option(
                    spec.substr(colon + 1), "--map-server port");
            } else {
                a.map_server_addr = spec;
            }
        }
        else if (s == "--default-map" && i + 1 < argc)
            a.default_map_num = parse_default_map(argv[++i]);
        else if (s == "--start-area" && i + 1 < argc) {
            a.start_area = parse_default_map(argv[++i]);
            a.start_area_explicit = true;
        }
        else if (s == "--map-server-map" && i + 1 < argc) {
            // Parse MAP=HOST:PORT. This is explicit so a production run
            // cannot silently infer a destination from the process name.
            const std::string spec = argv[++i];
            const auto equals = spec.find('=');
            const auto colon = spec.rfind(':');
            if (equals == std::string::npos || colon <= equals + 1 ||
                colon + 1 >= spec.size()) {
                std::cerr << "invalid --map-server-map (expected MAP=HOST:PORT): "
                          << spec << "\n";
                std::exit(2);
            }
            Args::MapRouteSpec route;
            route.map_num = static_cast<std::uint16_t>(
                parse_u16_option(spec.substr(0, equals), "--map-server-map map", 0));
            route.address = spec.substr(equals + 1, colon - equals - 1);
            route.port = parse_u16_option(spec.substr(colon + 1),
                                          "--map-server-map port");
            if (route.map_num == 0 || route.port == 0 || route.address.empty())
                std::exit(2);
            a.map_routes.push_back(std::move(route));
        }
        else if (s == "--help") {
            std::cout << "Usage: mxh_agent_server [options]\n"
                      << "  --port N              listen port (default 7001)\n"
                      << "  --db PATH             db path (SQLite file or MSSQL DSN)\n"
                      << "  --db-env NAME         read database path/DSN from an environment variable\n"
                      << "  --backend NAME        'sqlite' (default) or 'mssql_odbc'\n"
                      << "  --bind-address IP     listen interface (default 0.0.0.0)\n"
                      << "  --legacy              use 4DyuchiNET legacy framing\n"
                      << "  --map-server H:P      connect to MapServer at H:P\n"
                      << "  --map-server-map M=H:P register an additional map route\n"
                      << "  --default-map N       route used by --map-server (default "
                      << static_cast<unsigned>(kOmittedDefaultMapNum) << ")\n"
                      << "  --start-area N        new-character StartArea that must be registered\n"
                      << "                        (default: the --default-map value)\n";
            std::exit(0);
        }
        else {
            std::cerr << "unknown option: " << s << "\n"
                      << "use --help for valid options\n";
            std::exit(2);
        }
    }
    // --start-area omitted: the effective StartArea is default_map_num,
    // which is kOmittedDefaultMapNum when --default-map is also omitted.
    if (!a.start_area_explicit) a.start_area = a.default_map_num;
    return a;
}

}  // namespace mxh::agent_cli
