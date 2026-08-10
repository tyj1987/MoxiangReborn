// MoxianLoginServer - Phase 4 demo: DistributeServer-equivalent.

#include "mxh/crypto/crypto.hpp"
#include "mxh/db/db_adapter.hpp"
#include "mxh/db/sqlite_adapter.hpp"
#include "mxh/net/net.hpp"
#include "mxh/server/server.hpp"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>

// File-based logging for background process diagnostics.
static std::ofstream g_log;
static void log_file(const std::string& msg) {
    if (!g_log.is_open()) {
        g_log.open("d:/墨香全套源代码（源码+资源+客户端+服务端+教程）/modern/scratch/login_server.log",
                   std::ios::app);
    }
    g_log << msg << std::endl;
}

namespace {

struct Args {
    std::uint16_t port = 6001;
    std::string db_backend = "sqlite";  // "sqlite" | "mssql_odbc"
    std::string db_path = "modern/build/runtime/moxian.db";
    std::string agent_addr = "127.0.0.1";
    std::uint16_t agent_port = 7001;
    bool init_schema = false;
    bool use_legacy = false;  // Phase 7.6: 4DyuchiNET compatibility
    bool use_hsel   = false;  // Phase R-1: HSEL-encrypted legacy session
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
        else if (s == "--agent-addr" && i + 1 < argc)
            a.agent_addr = argv[++i];
        else if (s == "--agent-port" && i + 1 < argc)
            a.agent_port = static_cast<std::uint16_t>(std::stoi(argv[++i]));
        else if (s == "--init-schema")
            a.init_schema = true;
        else if (s == "--legacy")
            a.use_legacy = true;
        else if (s == "--use-hsel")
            a.use_hsel = true;
        else if (s == "--help") {
            std::cout << "Usage: mxh_login_server [options]\n"
                      << "  --port N          listen port (default 6001)\n"
                      << "  --db PATH         database path (SQLite file or MSSQL DSN)\n"
                      << "  --backend NAME    'sqlite' (default) or 'mssql_odbc'\n"
                      << "  --agent-addr ADDR AgentServer address\n"
                      << "  --agent-port N    AgentServer port\n"
                      << "  --init-schema     create tables before serving\n"
                      << "  --legacy          enable 4DyuchiNET legacy protocol framing\n"
                      << "  --use-hsel        encrypt the legacy session with the HSEL stream cipher\n";
            std::exit(0);
        }
    }
    return a;
}

std::atomic<bool> g_running{true};
void on_signal(int) { g_running.store(false); }

// Reply queue to break circular dependency between LoginHandler and TcpServer.
struct ReplyQueue {
    std::mutex mu;
    std::unordered_map<std::uint64_t, std::vector<mxh::net::Message>> pending;

    void push(std::uint64_t id, mxh::net::Message m) {
        std::lock_guard<std::mutex> lk(mu);
        log_file("[queue] push id=" + std::to_string(id)
                 + " cat=" + std::to_string(m.header.category)
                 + " proto=" + std::to_string(m.header.protocol)
                 + " obj=" + std::to_string(m.header.object_id)
                 + " payload=" + std::to_string(m.payload.size())
                 + " total=" + std::to_string(m.total_size()));
        pending[id].push_back(std::move(m));
    }

    // Drain all pending replies (call from main loop with server pointer).
    template <typename ServerT>
    void drain_to(ServerT& server) {
        std::lock_guard<std::mutex> lk(mu);
        for (auto& [id, msgs] : pending) {
            for (auto& m : msgs) {
                log_file("[drain] id=" + std::to_string(id)
                         + " cat=" + std::to_string((int)m.header.category)
                         + " proto=" + std::to_string((int)m.header.protocol)
                         + " payload=" + std::to_string(m.payload.size())
                         + " total=" + std::to_string(m.total_size()));
                auto e = server.send(mxh::net::ConnectionId{id}, m);
                if (e != mxh::net::NetError::Ok) {
                    log_file("[drain] SEND FAILED id=" + std::to_string(id)
                             + " error=" + mxh::net::to_string(e));
                    std::cerr << "[main] reply send failed (id=" << id
                              << "): " << mxh::net::to_string(e) << "\n";
                } else {
                    log_file("[drain] SEND OK id=" + std::to_string(id));
                }
            }
        }
        pending.clear();
    }
};

}  // namespace

int main(int argc, char** argv) {
    auto args = parse_args(argc, argv);

    std::signal(SIGINT, on_signal);
    std::signal(SIGTERM, on_signal);

    std::cout << "[main] Moxian LoginServer (Phase 4 demo)\n"
              << "  port       = " << args.port << "\n"
              << "  db.backend = " << args.db_backend << "\n"
              << "  db.path    = " << args.db_path << "\n"
              << "  agent      = " << args.agent_addr << ":" << args.agent_port << "\n"
              << "  legacy     = " << (args.use_legacy ? "yes (4DyuchiNET)" : "no (modern)") << "\n";

    // 1. Connect to database. Backend is selected via --backend.
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
    if (!cr) { std::cerr << "FATAL: db connect (backend='" << db_cfg.backend
                       << "' path='" << db_cfg.path
                       << "'): " << cr.error_message << "\n"; return 1; }

    // 2. Optionally create schema.
    if (args.init_schema) {
        const char* schema = R"SQL(
CREATE TABLE IF NOT EXISTS chr_log_info (
    id TEXT PRIMARY KEY,
    pw TEXT NOT NULL,
    userlevel INTEGER NOT NULL DEFAULT 0,
    registerdate TEXT,
    lastlogindate TEXT,
    lastloginip TEXT,
    usepoint INTEGER NOT NULL DEFAULT 0
);
INSERT OR IGNORE INTO chr_log_info (id, pw, userlevel) VALUES ('admin', 'admin', 2);
INSERT OR IGNORE INTO chr_log_info (id, pw, userlevel) VALUES ('test', 'test', 2);
INSERT OR IGNORE INTO chr_log_info (id, pw, userlevel) VALUES ('alice', 'wonderland', 0);
)SQL";
        // The bundled schema uses SQLite-only DDL (INSERT OR IGNORE, no MSSQL
        // counterpart); only run it when the runtime adapter is actually SQLite.
        // For MSSQL deployments the schema is created out-of-band by the restore
        // scripts in scripts/db/.
        if (auto* sqlite = dynamic_cast<mxh::db::SqliteAdapter*>(db.get())) {
            auto er = sqlite->exec_multi(schema);
            if (!er) std::cerr << "WARN: schema init: " << er.error_message << "\n";
            else std::cout << "[main] schema initialized\n";
        } else {
            std::cerr << "WARN: --init-schema skipped: bundled schema uses SQLite-only DDL "
                      << "(backend='" << db->backend_name() << "'); "
                      << "create schema out-of-band for non-SQLite backends\n";
        }
    }

    // 3. Build reply queue + handler + server.
    auto queue = std::make_shared<ReplyQueue>();

    mxh::net::TcpServer* server_ptr = nullptr;
    mxh::server::LoginHandler handler(*db, args.agent_addr, args.agent_port,
        [queue](mxh::net::ConnectionId id, const mxh::net::Message& m) {
            queue->push(id.value, m);
        },
        args.use_legacy, args.use_hsel,
        [&server_ptr](mxh::net::ConnectionId id, const mxh::net::Message& m) {
            if (server_ptr) server_ptr->send(id, m);
        });

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
    std::cout << "[main] LoginServer listening on 0.0.0.0:" << args.port << "\n";

    // 4. Main loop: drain reply queue + sleep.
    while (g_running.load()) {
        queue->drain_to(server);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "[main] shutting down...\n";
    server.stop();
    return 0;
}
