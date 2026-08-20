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
#include "mxh/net/net.hpp"

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

namespace {

struct Args {
    std::uint16_t port    = 8001;
    std::uint16_t map_num = 0;
    std::string   db_backend = "sqlite";  // "sqlite" | "mssql_odbc"
    std::string   db_path = "modern/build/runtime/moxian_map.db";
    std::string   db_env;
    std::string   bind_address = "0.0.0.0";
    std::string   resource_root;
    bool          use_legacy = true;  // always legacy for MapServer
    bool          use_hsel   = false;
    bool          dev_stub_caster = false;  // M3 side-by-side only
    bool          allow_dev_fallbacks = false;
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
        else if (s == "--db-env" && i + 1 < argc)
            a.db_env = argv[++i];
        else if (s == "--resource-root" && i + 1 < argc)
            a.resource_root = argv[++i];
        else if (s == "--bind-address" && i + 1 < argc)
            a.bind_address = argv[++i];
        else if (s == "--backend" && i + 1 < argc)
            a.db_backend = argv[++i];
        else if (s == "--no-legacy")
            a.use_legacy = false;
        else if (s == "--use-hsel")
            a.use_hsel = true;
        else if (s == "--dev-stub-caster")
            a.dev_stub_caster = true;  // M3 side-by-side only
        else if (s == "--allow-dev-fallbacks")
            a.allow_dev_fallbacks = true;
        else if (s == "--help") {
            std::cout << "Usage: mxh_map_server [options]\n"
                      << "  --port N      listen port (default 8001)\n"
                      << "  --map N       map number (default 0)\n"
                      << "  --db PATH     db path (SQLite file or MSSQL DSN/conn string)\n"
                      << "  --db-env NAME read database path/DSN from an environment variable\n"
                      << "  --bind-address IP  listen interface (default 0.0.0.0)\n"
                      << "  --resource-root DIR  PlayDH root (loads real SkillList/DealItem/QuestScript/AIGroup)\n"
                      << "  --backend NAME 'sqlite' (default) or 'mssql_odbc'\n"
                      << "  --allow-dev-fallbacks  permit hardcoded test monster spawns\n"
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

    std::cout << std::unitbuf;

    std::cout << "[main] Moxian MapServer (Phase 8 P0)\n"
              << "  locale   = " << locale_name() << "\n"
              << "  port     = " << args.port << "\n"
              << "  map      = " << args.map_num << "\n"
              << "  db.bk    = " << args.db_backend << "\n"
              << "  db.source= " << (args.db_env.empty() ? "command-line/path" : "environment") << "\n"
              << "  bind     = " << args.bind_address << "\n"
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
    std::filesystem::path resource_base = args.resource_root.empty()
        ? std::filesystem::path("Resource")
        : (std::filesystem::path(args.resource_root) / "Resource");
    std::filesystem::path ai_groups_path = resource_base / "Server" /
        (std::string("Monster_") + std::to_string(args.map_num) + ".bin");
    if (!mxh::server::AISystem::instance().load_ai_group_list(ai_groups_path)) {
        if (!args.allow_dev_fallbacks) {
            std::cerr << "FATAL: missing or invalid AIGroup data at "
                      << ai_groups_path.string() << "\n";
            return 1;
        }
        std::cerr << "[WARN] no AIGroup data at " << ai_groups_path.string()
                  << "; development fallback explicitly enabled\n";
    } else {
        std::cout << "[main] AIGroup data loaded for map " << args.map_num
                  << " (groups="
                  << mxh::server::AISystem::instance().group_list().groups.size()
                  << " spawns="
                  << mxh::server::AISystem::instance().group_list().spawn_count()
                  << ")\n";
    }

    if (!cr) { std::cerr << "FATAL: db connect: " << cr.error_message << "\n"; return 1; }

    // 2. Build reply queue + handler + server. Schema migrations are a
    // deployment precondition and are never performed by service processes.
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

    // Load the real game data tables when a PlayDH root is supplied.
    // Falls back to hardcoded tables / empty catalogs otherwise so the
    // existing e2e and side-by-side paths keep their deterministic traces.
    if (!args.resource_root.empty()) {
        const auto root = std::filesystem::path(args.resource_root);
        handler.load_skill_list((root / "Resource" / "SkillList.bin").string());
        handler.load_dealitem((root / "Resource" / "Dealitem.bin").string());
        handler.load_item_prices((root / "Resource" / "ItemList.bin").string());
        handler.load_item_list((root / "Resource" / "ItemList.bin").string());
        handler.load_experience_curve((root / "Resource" / "CharacterExpPoint.bin").string());
        handler.load_quest_script(
            (root / "Resource" / "QuestScript" / "QuestScript.bin").string());
        handler.load_quest_npcs(
            (root / "Resource" / "QuestScript" / "questnpclist.bin").string());
    }

    // M3 dev-stub-caster (side-by-side harness only).
    handler.set_dev_stub_caster(args.dev_stub_caster);
    handler.set_allow_dev_monster_fallback(args.allow_dev_fallbacks);

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
    std::cout << "[main] MapServer[" << locale_name() << "] map=" << args.map_num
              << " listening on " << args.bind_address << ":" << args.port << "\n";

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
