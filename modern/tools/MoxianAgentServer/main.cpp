// MoxianAgentServer - Phase 8/9: AgentServer-equivalent minimal server.
//
// Mirrors the legacy [Server]Agent/AgentServer.vcproj 5-locale build matrix
// (KOR / CHINA / JAPAN / HK / TL), but using modern mxh_server / mxh_net /
// mxh_db. The CMakeLists.txt adds one executable per locale, each defining
// the matching legacy preprocessor macros (_KOR_LOCAL_ / _CHINA_LOCAL_ /
// _JAPAN_LOCAL_ / _HK_LOCAL_ / _TL_LOCAL_) so any source pulled in that
// still references them via ifdef compiles correctly.
//
// Phase 9: --map-server HOST:PORT connects to a MoxianMapServer instance
// and forwards GameInSyn to it. MapServer responses are relayed to clients.
//
// Locale-specific extras (mirroring legacy vcxproj):
//   - CHINA: also TAIWAN_LOCAL (legacy define pair for 2008 China client)
//   - HK:    + _IGNORE_ASSERT_, _TW_LOCAL_, _NPROTECT_
//   - JAPAN: just _JAPAN_LOCAL_
//   - KOR:   just _KOR_LOCAL_
//   - TL:    just _TL_LOCAL_
//
// On startup, logs which locale macro is active so the operator can verify
// the right binary is running. The AgentHandler is wired identically to the
// legacy handler shape: open SQLite, accept TCP on --port (default 7001),
// handle UserConn::CharacterListSyn → return a 1-entry dummy character list.

#include "agent_cli.hpp"
#include "mxh/server/server.hpp"
#include "mxh/server/start_area_route.hpp"
#include "mxh/db/db_adapter.hpp"
#include "mxh/net/net.hpp"
#include "mxh/net/packet_trace.hpp"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

using mxh::agent_cli::Args;
using mxh::agent_cli::parse_args;

namespace {

// Report which locale macro this binary was built with. Only ONE of these
// is ever defined (CMakeLists.txt ensures that), so the log line identifies
// the exact binary flavor that produced it.
const char* locale_name() {
#if defined(_KOR_LOCAL_)
    return "KOR";
#elif defined(_CHINA_LOCAL_)
    return "CHINA";
#elif defined(_JAPAN_LOCAL_)
    return "JAPAN";
#elif defined(_HK_LOCAL_)
    return "HK";
#elif defined(_TL_LOCAL_)
    return "TL";
#else
    return "(unset)";
#endif
}

std::atomic<bool> g_running{true};
void on_signal(int) { g_running.store(false); }

// Reply queue to break circular dependency between AgentHandler and TcpServer
// (same pattern as MoxianLoginServer).
struct ReplyQueue {
    std::mutex mu;
    std::unordered_map<std::uint64_t, std::vector<mxh::net::Message>> pending;

    void push(std::uint64_t id, mxh::net::Message m) {
        std::lock_guard<std::mutex> lk(mu);
        pending[id].push_back(std::move(m));
    }

    template <typename ServerT>
    void drain_to(ServerT& server) {
        // Swap-then-send: hold lock only for the swap, send outside lock
        // to avoid blocking handler threads' push() during I/O.
        // Phase 10e: server.send() is now non-blocking (per-connection async
        // send queue), so no fairness limit needed — just drain everything.
        decltype(pending) batch;
        {
            std::lock_guard<std::mutex> lk(mu);
            batch.swap(pending);
        }
        for (auto& [id, msgs] : batch) {
            for (auto& m : msgs) {
                auto e = server.send(mxh::net::ConnectionId{id}, m);
                if (e != mxh::net::NetError::Ok) {
                    std::cerr << "[main] reply send_failed (id=" << id
                              << "): " << mxh::net::to_string(e) << "\n";
                }
            }
        }
    }
};

}  // namespace

// Phase 9: MapClientHandler receives messages from MapServer and forwards
// them to AgentHandler for routing to the correct client.
class MapClientHandler : public mxh::net::IConnectionHandler {
public:
    MapClientHandler(mxh::server::AgentHandler& agent, std::uint16_t map_num)
        : agent_(agent), map_num_(map_num) {}

    void set_sender(mxh::net::ITcpSender* sender) noexcept { sender_ = sender; }

    bool on_connect(mxh::net::ConnectionId id,
                    const std::string& remote_addr) override {
        std::cout << "[MapClient] connected to MapServer at " << remote_addr << "\n";
        map_conn_id_ = id;
        return true;
    }

    void on_message(mxh::net::ConnectionId id,
                    const mxh::net::Message& msg) override {
        auto cat = static_cast<mxh::proto::Category>(msg.header.category);
        if (mxh::net::packet_trace_enabled() ||
            msg.header.protocol == static_cast<std::uint8_t>(
                mxh::proto::UserConnProtocol::GameInAck)) {
            std::cout << "[MapClient] recv cat=" << mxh::proto::category_name(cat)
                      << " proto=" << (int)msg.header.protocol
                      << " obj=" << msg.header.object_id << "\n";
        }
        // Forward MapServer response to AgentHandler for client routing.
        agent_.forward_from_map(id, msg, map_num_, sender_);
    }

    void on_disconnect(mxh::net::ConnectionId id,
                       mxh::net::NetError reason) override {
        std::cout << "[MapClient] disconnected from MapServer (" << id.value
                  << ") reason=" << mxh::net::to_string(reason) << "\n";
    }

    mxh::net::ConnectionId get_map_conn_id() const { return map_conn_id_; }

private:
    mxh::server::AgentHandler& agent_;
    std::uint16_t map_num_ = 0;
    mxh::net::ITcpSender* sender_ = nullptr;
    mxh::net::ConnectionId map_conn_id_{};
};

int main(int argc, char** argv) {
    // Omitted --default-map is kOmittedDefaultMapNum (17). Omitted
    // --start-area copies that value. Original evidence: client
    // CharMake_SelectOption.bin CHINA StartArea = 17 (CharMakeOptions.cpp:99,
    // test char_make_options_test.cpp:66) and Linear comment 6e08ed6d on
    // TYJ-156 (original MP_CHARACTER_CreateCharacter writes map 17).
    auto args = parse_args(argc, argv);
    if (const auto conflict = mxh::agent_cli::extra_map_equals_default_error(args);
        !conflict.empty()) {
        std::cerr << conflict << "\n";
        return 2;
    }
    std::vector<std::uint16_t> configured_maps;
    configured_maps.reserve(args.map_routes.size());
    for (const auto& route : args.map_routes) configured_maps.push_back(route.map_num);
    if (const int startup = mxh::server::agent_startup_exit_if_start_area_unregistered(
            args.start_area, !args.map_server_addr.empty(), args.default_map_num,
            configured_maps.data(), configured_maps.size())) {
        return startup;
    }

    if (!args.db_env.empty()) {
        const auto* value = std::getenv(args.db_env.c_str());
        if (value == nullptr || *value == '\0') {
            std::cerr << "FATAL: database environment variable is not set: "
                      << args.db_env << "\n";
            return 1;
        }
        args.db_path = value;
    }

    std::signal(SIGINT, on_signal);
    std::signal(SIGTERM, on_signal);

    // Force line-buffered stdout. When stdout is redirected to a file or
    // pipe (smoke harness, IDE console, etc.) the default is fully-buffered,
    // so any lines written before SIGKILL would be lost. std::unitbuf forces
    // flush after every << operation. Critical for the build-matrix smoke
    // harness, which captures output via redirected files.
    std::cout << std::unitbuf;

    std::cout << "[main] Moxian AgentServer (Phase 8 minimal demo)\n"
              << "  locale   = " << locale_name() << "\n"
              << "  port     = " << args.port << "\n"
              << "  db.bk    = " << args.db_backend << "\n"
              << "  db.source= " << (args.db_env.empty() ? "command-line/path" : "environment") << "\n"
              << "  bind     = " << args.bind_address << "\n"
              << "  legacy   = " << (args.use_legacy ? "YES" : "no") << "\n";

    // 1. Connect to database. Backend selected via --backend (sqlite
    //    default; mssql_odbc when talking to the legacy 墨香 SQL Server).
    auto db = mxh::db::make_adapter(args.db_backend);
    if (!db) { std::cerr << "FATAL: cannot create '" << args.db_backend
                        << "' adapter (unknown backend or platform unsupported)\n";
              return 1; }
    auto db_cfg = mxh::db::ConnectionConfig::from_kv_string(args.db_path);
    if (db_cfg.path.empty()) db_cfg.path = args.db_path;  // raw path fallback
    if (db_cfg.backend.empty()) db_cfg.backend = args.db_backend;
    if (db_cfg.backend.empty()) db_cfg.backend = "sqlite";
    if (db_cfg.backend == "sqlite") {
        const auto parent = std::filesystem::path(db_cfg.path).parent_path();
        if (!parent.empty()) std::filesystem::create_directories(parent);
    }
    auto cr = db->connect(db_cfg);
    if (!cr) { std::cerr << "FATAL: db connect: " << cr.error_message << "\n"; return 1; }

    std::cout << "[main] DB connected; schema migration is an external precondition"
              << " (backend=" << db->backend_name() << ")\n";

    // 2. Build reply queue + handler + server.
    auto queue = std::make_shared<ReplyQueue>();

    mxh::net::TcpServer* server_ptr = nullptr;
    mxh::server::AgentHandler handler(*db,
        [queue](mxh::net::ConnectionId id, const mxh::net::Message& m) {
            queue->push(id.value, m);
        },
        args.use_legacy, args.use_hsel,
        [&server_ptr](mxh::net::ConnectionId id, const mxh::net::Message& m) {
            if (server_ptr) (void)server_ptr->send(id, m);
        }, args.default_map_num, args.start_area);
    handler.set_allow_dev_gamein_fallback(false);

    mxh::net::TcpServer server(handler);
    server_ptr = &server;
    mxh::net::ServerConfig scfg;
    scfg.port = args.port;
    scfg.bind_address = args.bind_address;
    scfg.use_legacy_framing = args.use_legacy;
    auto sr = server.start(scfg);
    if (sr != mxh::net::NetError::Ok) {
        std::cerr << "FATAL: server start: " << mxh::net::to_string(sr) << "\n";
        return 1;
    }
    std::cout << "[main] AgentServer[" << locale_name() << "] listening on "
              << args.bind_address << ":"
              << args.port << "\n";

    // Phase 9: Connect to MapServer if specified.
    std::unique_ptr<MapClientHandler> map_handler;
    std::unique_ptr<mxh::net::TcpClient> map_client;
    struct MapRouteConnection {
        Args::MapRouteSpec spec;
        std::unique_ptr<MapClientHandler> handler;
        std::unique_ptr<mxh::net::TcpClient> client;
        std::chrono::steady_clock::time_point next_retry{};
    };
    std::vector<std::unique_ptr<MapRouteConnection>> map_route_connections;
    auto next_map_reconnect = std::chrono::steady_clock::now();

    const auto connect_map_server = [&]() -> bool {
        map_handler = std::make_unique<MapClientHandler>(handler, args.default_map_num);
        map_client = std::make_unique<mxh::net::TcpClient>(*map_handler);
        map_handler->set_sender(map_client.get());
        mxh::net::ClientConfig ccfg;
        ccfg.remote_address = args.map_server_addr;
        ccfg.port = args.map_server_port;
        ccfg.use_legacy_framing = true;
        ccfg.connect_timeout = std::chrono::milliseconds(500);
        const auto error = map_client->connect(ccfg);
        if (error != mxh::net::NetError::Ok) {
            std::cerr << "[main] cannot connect to MapServer at "
                      << args.map_server_addr << ":" << args.map_server_port
                      << " (" << mxh::net::to_string(error) << ")\n";
            map_client.reset();
            map_handler.reset();
            return false;
        }
        handler.set_map_server(map_client.get(), map_handler->get_map_conn_id());
        std::cout << "[main] connected to MapServer at "
                  << args.map_server_addr << ":" << args.map_server_port << "\n";
        return true;
    };

    if (!args.map_server_addr.empty()) {
        if (!connect_map_server()) {
            std::cerr << "[main] MapServer unavailable; retrying in 500ms\n";
            next_map_reconnect = std::chrono::steady_clock::now() +
                                 std::chrono::milliseconds(500);
        }
    } else {
        std::cout << "[main] no --map-server specified, GameInSyn will use stub mode\n";
    }

    // Additional map endpoints are explicit and independent of the default
    // connection. A failed endpoint stays unregistered until its own retry
    // succeeds. It is never replaced by another map's connection.
    const auto connect_extra_map = [&](MapRouteConnection& route) -> bool {
        handler.set_map_server_for_map(route.spec.map_num, nullptr,
                                       mxh::net::ConnectionId{});
        if (route.client) route.client->disconnect();
        route.client.reset();
        route.handler.reset();
        route.handler = std::make_unique<MapClientHandler>(handler, route.spec.map_num);
        route.client = std::make_unique<mxh::net::TcpClient>(*route.handler);
        route.handler->set_sender(route.client.get());
        mxh::net::ClientConfig ccfg;
        ccfg.remote_address = route.spec.address;
        ccfg.port = route.spec.port;
        ccfg.use_legacy_framing = true;
        ccfg.connect_timeout = std::chrono::milliseconds(500);
        const auto error = route.client->connect(ccfg);
        if (error != mxh::net::NetError::Ok) {
            std::cerr << "[main] cannot connect map=" << route.spec.map_num
                      << " MapServer at " << route.spec.address << ":"
                      << route.spec.port << " (" << mxh::net::to_string(error)
                      << ")\n";
            route.client.reset();
            route.handler.reset();
            route.next_retry = std::chrono::steady_clock::now() +
                               std::chrono::milliseconds(500);
            return false;
        }
        handler.set_map_server_for_map(route.spec.map_num, route.client.get(),
                                       route.handler->get_map_conn_id());
        std::cout << "[main] connected map=" << route.spec.map_num
                  << " MapServer at " << route.spec.address << ":"
                  << route.spec.port << "\n";
        return true;
    };
    for (const auto& spec : args.map_routes) {
        auto route = std::make_unique<MapRouteConnection>();
        route->spec = spec;
        (void)connect_extra_map(*route);
        map_route_connections.push_back(std::move(route));
    }

    // 3. Main loop: drain reply queue + sleep + auto-reconnect MapClient.
    while (g_running.load()) {
        queue->drain_to(server);

        const auto now = std::chrono::steady_clock::now();
        const bool map_disconnected = !map_client || !map_client->is_connected();
        if (!args.map_server_addr.empty() && map_disconnected &&
            now >= next_map_reconnect) {
            handler.set_map_server(nullptr, mxh::net::ConnectionId{});
            if (map_client) map_client->disconnect();
            map_client.reset();
            map_handler.reset();

            std::cout << "[main] reconnecting to MapServer...\n";
            if (!connect_map_server()) {
                next_map_reconnect = now + std::chrono::milliseconds(500);
            }
        }
        for (auto& route : map_route_connections) {
            const bool down = !route->client || !route->client->is_connected();
            if (down && now >= route->next_retry) {
                (void)connect_extra_map(*route);
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    std::cout << "[main] shutting down...\n";
    // Phase 9: disconnect MapServer before stopping the server.
    if (map_client) {
        handler.set_map_server(nullptr, mxh::net::ConnectionId{});
        map_client->disconnect();
        map_client.reset();
        map_handler.reset();
    }
    for (auto& route : map_route_connections) {
        handler.set_map_server_for_map(route->spec.map_num, nullptr,
                                       mxh::net::ConnectionId{});
        if (route->client) route->client->disconnect();
        route->client.reset();
        route->handler.reset();
    }
    server.stop();
    return 0;
}
