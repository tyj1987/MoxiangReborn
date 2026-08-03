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

#include "mxh/server/server.hpp"
#include "mxh/db/db_adapter.hpp"
#include "mxh/db/sqlite_adapter.hpp"
#include "mxh/net/net.hpp"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstring>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace {

struct Args {
    std::uint16_t port       = 7001;
    std::string   db_backend = "sqlite";  // "sqlite" | "mssql_odbc"
    std::string   db_path    = "./moxian_agent.db";
    bool          use_legacy = false;
    // Phase 9: optional MapServer connection for GameIn forwarding.
    std::string   map_server_addr;  // empty = no MapServer (stub mode)
    std::uint16_t map_server_port   = 8001;
};

Args parse_args(int argc, char** argv) {
    Args a;
    for (int i = 1; i < argc; ++i) {
        std::string_view s = argv[i];
        if (s == "--port" && i + 1 < argc)
            a.port = static_cast<std::uint16_t>(std::stoi(argv[++i]));
        else if (s == "--db" && i + 1 < argc)
            a.db_path = argv[++i];
        else if (s == "--backend" && i + 1 < argc)
            a.db_backend = argv[++i];
        else if (s == "--legacy")
            a.use_legacy = true;
        else if (s == "--map-server" && i + 1 < argc) {
            // Parse HOST:PORT format
            std::string spec = argv[++i];
            auto colon = spec.rfind(':');
            if (colon != std::string::npos) {
                a.map_server_addr = spec.substr(0, colon);
                a.map_server_port = static_cast<std::uint16_t>(
                    std::stoi(spec.substr(colon + 1)));
            } else {
                a.map_server_addr = spec;
            }
        }
        else if (s == "--help") {
            std::cout << "Usage: mxh_agent_server [options]\n"
                      << "  --port N              listen port (default 7001)\n"
                      << "  --db PATH             db path (SQLite file or MSSQL DSN)\n"
                      << "  --backend NAME        'sqlite' (default) or 'mssql_odbc'\n"
                      << "  --legacy              use 4DyuchiNET legacy framing\n"
                      << "  --map-server H:P      connect to MapServer at H:P\n";
            std::exit(0);
        }
    }
    return a;
}

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
    explicit MapClientHandler(mxh::server::AgentHandler& agent)
        : agent_(agent) {}

    bool on_connect(mxh::net::ConnectionId id,
                    const std::string& remote_addr) override {
        std::cout << "[MapClient] connected to MapServer at " << remote_addr << "\n";
        map_conn_id_ = id;
        return true;
    }

    void on_message(mxh::net::ConnectionId id,
                    const mxh::net::Message& msg) override {
        auto cat = static_cast<mxh::proto::Category>(msg.header.category);
        std::cout << "[MapClient] recv cat=" << mxh::proto::category_name(cat)
                  << " proto=" << (int)msg.header.protocol
                  << " obj=" << msg.header.object_id << "\n";
        // Forward MapServer response to AgentHandler for client routing.
        agent_.forward_from_map(id, msg);
    }

    void on_disconnect(mxh::net::ConnectionId id,
                       mxh::net::NetError reason) override {
        std::cout << "[MapClient] disconnected from MapServer (" << id.value
                  << ") reason=" << mxh::net::to_string(reason) << "\n";
    }

    mxh::net::ConnectionId get_map_conn_id() const { return map_conn_id_; }

private:
    mxh::server::AgentHandler& agent_;
    mxh::net::ConnectionId map_conn_id_{};
};

int main(int argc, char** argv) {
    auto args = parse_args(argc, argv);

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
              << "  db.path  = " << args.db_path << "\n"
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
    auto cr = db->connect(db_cfg);
    if (!cr) { std::cerr << "FATAL: db connect: " << cr.error_message << "\n"; return 1; }

    // Phase 8: ensure character_info table exists with all needed columns.
    // CREATE TABLE IF NOT EXISTS is idempotent. ALTER TABLE ADD COLUMN is
    // also idempotent (silently fails if column already exists).
    static const char* kSchemaInit =
        "CREATE TABLE IF NOT EXISTS character_info ("
        "    charname     TEXT PRIMARY KEY,"
        "    chrid        INTEGER NOT NULL UNIQUE,"
        "    userid       TEXT NOT NULL,"
        "    character_data BLOB"
        ");";
    auto schema_result = db->execute(kSchemaInit);
    (void)schema_result;

    static const char* kSchemaAlters[] = {
        "ALTER TABLE character_info ADD COLUMN sex_type INTEGER DEFAULT 0",
        "ALTER TABLE character_info ADD COLUMN hair_type INTEGER DEFAULT 0",
        "ALTER TABLE character_info ADD COLUMN face_type INTEGER DEFAULT 0",
        "ALTER TABLE character_info ADD COLUMN body_type INTEGER DEFAULT 0",
        "ALTER TABLE character_info ADD COLUMN start_area INTEGER DEFAULT 0",
        "ALTER TABLE character_info ADD COLUMN height REAL DEFAULT 1.0",
        "ALTER TABLE character_info ADD COLUMN width REAL DEFAULT 1.0",
        "ALTER TABLE character_info ADD COLUMN level INTEGER DEFAULT 1",
        "ALTER TABLE character_info ADD COLUMN map_num INTEGER DEFAULT 12",
        "ALTER TABLE character_info ADD COLUMN standing_idx INTEGER DEFAULT 0",
    };
    for (const auto* sql : kSchemaAlters) {
        auto sr2 = db->execute(sql);
        // Ignore errors (column already exists).
        (void)sr2;
    }
    std::cout << "[main] DB schema initialized\n";

    // 2. Build reply queue + handler + server.
    auto queue = std::make_shared<ReplyQueue>();

    mxh::server::AgentHandler handler(*db,
        [queue](mxh::net::ConnectionId id, const mxh::net::Message& m) {
            queue->push(id.value, m);
        },
        args.use_legacy);

    mxh::net::TcpServer server(handler);
    mxh::net::ServerConfig scfg;
    scfg.port = args.port;
    scfg.bind_address = "0.0.0.0";
    scfg.use_legacy_framing = args.use_legacy;
    auto sr = server.start(scfg);
    if (sr != mxh::net::NetError::Ok) {
        std::cerr << "FATAL: server start: " << mxh::net::to_string(sr) << "\n";
        return 1;
    }
    std::cout << "[main] AgentServer[" << locale_name() << "] listening on 0.0.0.0:"
              << args.port << "\n";

    // Phase 9: Connect to MapServer if specified.
    std::unique_ptr<MapClientHandler> map_handler;
    std::unique_ptr<mxh::net::TcpClient> map_client;
    auto next_map_reconnect = std::chrono::steady_clock::now();

    const auto connect_map_server = [&]() -> bool {
        map_handler = std::make_unique<MapClientHandler>(handler);
        map_client = std::make_unique<mxh::net::TcpClient>(*map_handler);
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
    server.stop();
    return 0;
}