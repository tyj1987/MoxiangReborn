// MoxianAgentServer - Phase 8: AgentServer-equivalent minimal server.
//
// Mirrors the legacy [Server]Agent/AgentServer.vcproj 5-locale build matrix
// (KOR / CHINA / JAPAN / HK / TL), but using modern mxh_server / mxh_net /
// mxh_db. The CMakeLists.txt adds one executable per locale, each defining
// the matching legacy preprocessor macros (_KOR_LOCAL_ / _CHINA_LOCAL_ /
// _JAPAN_LOCAL_ / _HK_LOCAL_ / _TL_LOCAL_) so any source pulled in that
// still references them via ifdef compiles correctly.
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
    std::string   db_path    = "./moxian_agent.db";
};

Args parse_args(int argc, char** argv) {
    Args a;
    for (int i = 1; i < argc; ++i) {
        std::string_view s = argv[i];
        if (s == "--port" && i + 1 < argc)
            a.port = static_cast<std::uint16_t>(std::stoi(argv[++i]));
        else if (s == "--db" && i + 1 < argc)
            a.db_path = argv[++i];
        else if (s == "--help") {
            std::cout << "Usage: mxh_agent_server [options]\n"
                      << "  --port N      listen port (default 7001)\n"
                      << "  --db PATH     SQLite db path\n";
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
        std::lock_guard<std::mutex> lk(mu);
        for (auto& [id, msgs] : pending) {
            for (auto& m : msgs) {
                auto e = server.send(mxh::net::ConnectionId{id}, m);
                if (e != mxh::net::NetError::Ok) {
                    std::cerr << "[main] reply send failed (id=" << id
                              << "): " << mxh::net::to_string(e) << "\n";
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

    // Force line-buffered stdout. When stdout is redirected to a file or
    // pipe (smoke harness, IDE console, etc.) the default is fully-buffered,
    // so any lines written before SIGKILL would be lost. std::unitbuf forces
    // flush after every << operation. Critical for the build-matrix smoke
    // harness, which captures output via redirected files.
    std::cout << std::unitbuf;

    std::cout << "[main] Moxian AgentServer (Phase 8 minimal demo)\n"
              << "  locale   = " << locale_name() << "\n"
              << "  port     = " << args.port << "\n"
              << "  db       = " << args.db_path << "\n";

    // 1. Connect to database (sqlite only — same as MoxianLoginServer).
    auto db = mxh::db::make_adapter("sqlite");
    if (!db) { std::cerr << "FATAL: cannot create sqlite adapter\n"; return 1; }
    auto db_cfg = mxh::db::ConnectionConfig::from_kv_string(args.db_path);
    if (db_cfg.backend.empty()) db_cfg.backend = "sqlite";
    auto cr = db->connect(db_cfg);
    if (!cr) { std::cerr << "FATAL: db connect: " << cr.error_message << "\n"; return 1; }

    // 2. Build reply queue + handler + server.
    auto queue = std::make_shared<ReplyQueue>();

    mxh::server::AgentHandler handler(*db,
        [queue](mxh::net::ConnectionId id, const mxh::net::Message& m) {
            queue->push(id.value, m);
        });

    mxh::net::TcpServer server(handler);
    mxh::net::ServerConfig scfg;
    scfg.port = args.port;
    scfg.bind_address = "0.0.0.0";
    auto sr = server.start(scfg);
    if (sr != mxh::net::NetError::Ok) {
        std::cerr << "FATAL: server start: " << mxh::net::to_string(sr) << "\n";
        return 1;
    }
    std::cout << "[main] AgentServer[" << locale_name() << "] listening on 0.0.0.0:"
              << args.port << "\n";

    // 3. Main loop: drain reply queue + sleep.
    while (g_running.load()) {
        queue->drain_to(server);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "[main] shutting down...\n";
    server.stop();
    return 0;
}