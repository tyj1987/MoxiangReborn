// MoxianMapServer - Modernized per-map game server.
//
// Each MapServer instance manages ONE map (identified by --map N).
// The legacy [Server]Map/ has ~358 source files handling the full game world.
// This P0 implementation handles only the minimum to get a client into a map:
//
//   1. Client connects (after AgentServer tells it the MapServer address)
//   2. Client sends GAMEIN_SYN (cat=7, proto=28) with character info
//   3. Server responds with GAMEIN_ACK (cat=7, proto=29) containing
//      SEND_HERO_TOTALINFO (~2KB zero-filled structure)
//   4. Client renders the game world and can move/chat
//
// Usage: mxh_map_server_KOR --port 8001 --map 0 --legacy
//
// The --legacy flag is mandatory (original clients always use 4DyuchiNET framing).
// Map number defaults to 0 (village map). In the full implementation, each
// map server process would be started with a different --map value.

#include "mxh/server/server.hpp"
#include "mxh/server/ai_system.hpp"
#include "mxh/server/ai_group_loader.hpp"

#include <filesystem>
#include "mxh/db/db_adapter.hpp"
#include "mxh/db/sqlite_adapter.hpp"
#include "mxh/net/net.hpp"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace {

struct Args {
    std::uint16_t port    = 8001;
    std::uint16_t map_num = 0;
    std::string   db_backend = "sqlite";  // "sqlite" | "mssql_odbc"
    std::string   db_path = "modern/build/runtime/moxian_map.db";
    bool          use_legacy = true;  // always legacy for MapServer
    bool          use_hsel   = false;
    bool          dev_stub_caster = false;  // M3 side-by-side only
};

Args parse_args(int argc, char** argv) {
    Args a;
    for (int i = 1; i < argc; ++i) {
        std::string_view s = argv[i];
        if (s == "--port" && i + 1 < argc)
            a.port = static_cast<std::uint16_t>(std::stoi(argv[++i]));
        else if (s == "--map" && i + 1 < argc)
            a.map_num = static_cast<std::uint16_t>(std::stoi(argv[++i]));
        else if (s == "--db" && i + 1 < argc)
            a.db_path = argv[++i];
        else if (s == "--backend" && i + 1 < argc)
            a.db_backend = argv[++i];
        else if (s == "--no-legacy")
            a.use_legacy = false;
        else if (s == "--use-hsel")
            a.use_hsel = true;
        else if (s == "--dev-stub-caster")
            a.dev_stub_caster = true;  // M3 side-by-side only
        else if (s == "--help") {
            std::cout << "Usage: mxh_map_server [options]\n"
                      << "  --port N      listen port (default 8001)\n"
                      << "  --map N       map number (default 0)\n"
                      << "  --db PATH     db path (SQLite file or MSSQL DSN/conn string)\n"
                      << "  --backend NAME 'sqlite' (default) or 'mssql_odbc'\n"
                      << "  --no-legacy   disable 4DyuchiNET framing\n";
            std::exit(0);
        }
    }
    return a;
}

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

// Reply queue (same pattern as LoginServer and AgentServer).
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
        decltype(pending) batch;
        {
            std::lock_guard<std::mutex> lk(mu);
            batch.swap(pending);
        }
        for (auto& [id, msgs] : batch) {
            for (auto& m : msgs) {
                auto e = server.send(mxh::net::ConnectionId{id}, m);
                if (e != mxh::net::NetError::Ok) {
                    std::cerr << "[main] reply send failed (id=" << id
                              << "): " << mxh::net::to_string(e) << "\n";
                }
            }
        }
    }
};

}  // namespace

int main(int argc, char** argv) {
    auto args = parse_args(argc, argv);

    std::signal(SIGINT, on_signal);
    std::signal(SIGTERM, on_signal);

    std::cout << std::unitbuf;

    std::cout << "[main] Moxian MapServer (Phase 8 P0)\n"
              << "  locale   = " << locale_name() << "\n"
              << "  port     = " << args.port << "\n"
              << "  map      = " << args.map_num << "\n"
              << "  db       = " << args.db_path << "\n"
              << "  legacy   = " << (args.use_legacy ? "YES" : "no") << "\n";

    // 1. Connect to database.
    auto db = mxh::db::make_adapter(args.db_backend);
    if (!db) { std::cerr << "FATAL: cannot create '" << args.db_backend
                        << "' adapter (unknown backend or platform unsupported)\n";
              return 1; }
    auto db_cfg = mxh::db::ConnectionConfig::from_kv_string(args.db_path);
    if (db_cfg.path.empty()) db_cfg.path = args.db_path;  // raw path fallback
    if (db_cfg.backend.empty()) db_cfg.backend = args.db_backend;
    if (db_cfg.backend == "sqlite") {
        const auto parent = std::filesystem::path(db_cfg.path).parent_path();
        if (!parent.empty()) std::filesystem::create_directories(parent);
    }
    auto cr = db->connect(db_cfg);
    std::filesystem::path ai_groups_path =
        std::filesystem::path("Resource/Server") /
        (std::string("Monster_") + std::to_string(args.map_num) + ".bin");
    if (!mxh::server::AISystem::instance().load_ai_group_list(ai_groups_path)) {
        std::cerr << "[WARN] no AIGroup data at " << ai_groups_path.string()
                  << "; using default spawn points\n";
    } else {
        std::cout << "[main] AIGroup data loaded for map " << args.map_num
                  << " (groups="
                  << mxh::server::AISystem::instance().group_list().groups.size()
                  << " spawns="
                  << mxh::server::AISystem::instance().group_list().spawn_count()
                  << ")\n";
    }

    if (!cr) { std::cerr << "FATAL: db connect: " << cr.error_message << "\n"; return 1; }

    // Phase 9.1: Ensure character_info table exists with all needed columns.
    // Same schema as AgentServer - MapServer queries it for GameIn data.
    // SQLite-only DDL: skip for non-SQLite backends (MSSQL schema is
    // created out-of-band, e.g. deploy/database/mx_modern_schema_mssql.sql
    // or mxh_client_e2e --backend mssql_odbc --init-schema).
    if (db->backend_name() == "sqlite") {
        static const char* kSchemaInit =
            "CREATE TABLE IF NOT EXISTS character_info ("
            "    charname     TEXT PRIMARY KEY,"
            "    chrid        INTEGER NOT NULL UNIQUE,"
            "    userid       TEXT NOT NULL,"
            "    sex_type     INTEGER DEFAULT 0,"
            "    hair_type    INTEGER DEFAULT 0,"
            "    face_type    INTEGER DEFAULT 0,"
            "    body_type    INTEGER DEFAULT 0,"
            "    start_area   INTEGER DEFAULT 12,"
            "    height       REAL DEFAULT 1.0,"
            "    width        REAL DEFAULT 1.0,"
            "    level        INTEGER DEFAULT 1,"
            "    map_num      INTEGER DEFAULT 12,"
            "    standing_idx INTEGER DEFAULT 0,"
            "    character_data BLOB"
            ");";
        auto schema_result = db->execute(kSchemaInit);
        if (!schema_result.ok()) {
            std::cerr << "[main] schema init warning: "
                      << schema_result.error_message << "\n";
        }
    } else {
        std::cout << "[main] schema for backend '"
                  << db->backend_name()
                  << "' is managed out-of-band; skipping SQLite DDL\n";
    }

    // 2. Build reply queue + handler + server.
    auto queue = std::make_shared<ReplyQueue>();

    mxh::net::TcpServer* server_ptr = nullptr;
    mxh::server::MapHandler handler(*db, args.map_num,
        [queue](mxh::net::ConnectionId id, const mxh::net::Message& m) {
            queue->push(id.value, m);
        },
        args.use_legacy, args.use_hsel,
        [&server_ptr](mxh::net::ConnectionId id, const mxh::net::Message& m) {
            if (server_ptr) server_ptr->send(id, m);
        });

    // M3 dev-stub-caster (side-by-side harness only).
    handler.set_dev_stub_caster(args.dev_stub_caster);

    mxh::net::TcpServer server(handler);
    server_ptr = &server;
    mxh::net::ServerConfig scfg;
    scfg.port = args.port;
    scfg.bind_address = "0.0.0.0";
    scfg.use_legacy_framing = args.use_legacy;
    auto sr = server.start(scfg);
    if (sr != mxh::net::NetError::Ok) {
        std::cerr << "FATAL: server start: " << mxh::net::to_string(sr) << "\n";
        return 1;
    }
    std::cout << "[main] MapServer[" << locale_name() << "] map=" << args.map_num
              << " listening on 0.0.0.0:" << args.port << "\n";

    // 3. Main loop: drain reply queue + sleep.
    auto last_ai_tick = std::chrono::steady_clock::now();
    while (g_running.load()) {
        queue->drain_to(server);
        const auto now = std::chrono::steady_clock::now();
        if (now - last_ai_tick >= std::chrono::milliseconds(100)) {
            handler.tick_monster_ai();
            last_ai_tick = now;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    std::cout << "[main] shutting down...\n";
    server.stop();
    return 0;
}
