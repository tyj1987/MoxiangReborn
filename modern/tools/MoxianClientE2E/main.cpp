// MoxianClientE2E - Phase B.2.5 headless end-to-end smoke test.
//
// Drives the modern C++ state classes (CLoginState + CCharSelectState
// + CInGameState) against a real 3-server chain (Login/Agent/Map),
// asserting the protocol round-trips on each hop.  This complements
// (rather than replaces) modern/scripts/verify_servers_e2e.py: the
// Python script simulates the wire format directly, while this tool
// exercises the actual modern state class implementations end-to-end.
//
// What it covers:
//   1. Login:    CLoginState connects to LoginServer (:6001), receives
//                DistConnectSuccess, sends RequestLogin, parses LoginAck.
//   2. Agent:    CCharSelectState connects to AgentServer (:7001),
//                receives AgentConnectSuccess, sends CharacterListSyn,
//                parses CharacterListAck.  No character exists in the
//                DB, so we create one via CCharMake (CharacterMakeSyn)
//                and then re-enter CharSelect to verify the refreshed
//                list, which also exercises the real DB insert path.
//   3. CharMake: CCharMake submits CharacterMakeSyn (59B legacy payload),
//                the agent inserts into character_info and re-sends the
//                refreshed CharacterListAck (creation success).
//   4. Re-list:  A fresh CCharSelectState re-fetches the list and must
//                see the created character (real DB round-trip).
//   5. Map:      CInGameState connects to MapServer (:8001) with the
//                created chrid, sends GameInSyn, parses GameInAck
//                (3000B SEND_HERO_TOTALINFO).
//
// All five steps must PASS for the tool to exit 0.
//
// Build:
//   cmake --build modern/build --config Debug --target mxh_client_e2e
//
// Usage:
//   mxh_client_e2e [--login-exe PATH] [--agent-exe PATH] [--map-exe PATH]
//                  [--map-number N]
//                  [--exercise-combat]  # Debug-only live attack/effect/drop gate
//                  [--no-spawn]  # assume servers are already running
//                  [--timeout N] # per-step timeout in seconds (default 10)
//                  [--backend NAME]   'sqlite' (default) or 'mssql_odbc'
//                  [--db DB]          SQLite file or MSSQL kv string
//                  [--init-schema]    apply schema before spawning
//                  [--use-hsel]       run the whole chain HSEL-encrypted
//
// MSSQL single-command example (Phase P0; LocalDB):
//   mxh_client_e2e --backend mssql_odbc --init-schema
//   (defaults to host=(localdb)\MSSQLLocalDB, database=Moxiang)
//
// With --backend mssql_odbc all three servers share one SQL Server
// database (the kv string passed via --db).  --init-schema bootstraps
// the modern schema directly through the ODBC adapter (creates the DB if
// missing, then the chr_log_info / character_info tables + test account),
// so the whole chain is reproducible with one command and no sqlcmd.
//
// Exit codes:
//   0  - all 5 protocol steps passed
//   1  - server failed to spawn
//   2  - protocol step failed
//   3  - usage error / server exe not found
//
// Phase B.2.5 design:
//   * Headless — no HWND, no DX11, no message pump.  We drive
//     CLoginState / CCharSelectState / CInGameState directly and let
//     the underlying TcpClient recv threads call IConnectionHandler
//     on_message from the network stack.  Per-frame Process() is a
//     no-op for these states (only ticks m_dwDialogProcessTickCount).
//   * No CMainGame — we sequence the states by hand.  Real client
//     wires CMainGame's state-change rising edge to Start() each new
//     state; here we just call Start() after the previous state
//     reaches its terminal condition.
//   * Server process management uses CreateProcessW + WaitForInputIdle
//     to keep the spawn logic Windows-native and match what
//     verify_servers_e2e.py does via subprocess.Popen.

#include "CLoginState.hpp"
#include "CCharSelectState.hpp"
#include "CCharMake.hpp"
#include "CInGameState.hpp"
#include "CEngine.hpp"
#include "CMainGame.hpp"

#include "mxh/db/db_adapter.hpp"
#include "mxh/db/mssql_odbc_adapter.hpp"
#include "mxh/db/schema_migration.hpp"
#include "mxh/server/account_service.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
// CMake-generated defines may already set WIN32_LEAN_AND_MEAN; only
// define it if it isn't already, to avoid macro-redefinition warnings.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

namespace {

// ---------------------------------------------------------------------------
// CLI parsing
// ---------------------------------------------------------------------------
struct CliArgs {
    std::string login_exe;
    std::string agent_exe;
    std::string map_exe;
    std::string db_backend = "sqlite";  // "sqlite" | "mssql_odbc"
    std::string db;                     // SQLite file path or MSSQL kv string
    bool db_explicit = false;
    bool no_spawn = false;
    std::string character_name;
    std::string account = "mxh_e2e";
    std::string password = "Pass1234";
    std::string login_host = "127.0.0.1";
    std::string agent_host;  // empty = LoginAck agent_addr, else override
    std::string map_host;    // reserved; GameIn uses persistent AgentSession
    bool dump_cli = false;
    int  timeout_s = 10;
    int  map_number = 10;
    bool use_hsel = false;  // Phase R-1: run the whole chain HSEL-encrypted
    bool exercise_combat = false; // opt-in live combat gate; never implicit
    bool exercise_skills = false; // opt-in quick-slot skill/effect gate
    bool exercise_mapchange = false; // opt-in cross-map route/load gate
    bool init_schema = true;   // Phase P0: apply the modern schema before
                               // spawning.  SQLite: always safe (idempotent
                               // CREATE TABLE IF NOT EXISTS).  MSSQL: keeps
                               // the legacy default, pass --init-schema only
                               // when the caller wants the E2E tool to
                               // bootstrap the shared DB itself.
};

std::string resolve_server_exe(const char* argv0,
                               const char* directory,
                               const char* executable) {
    std::error_code error;
    auto tool_dir = std::filesystem::absolute(argv0, error).parent_path().parent_path();
    if (error) tool_dir = std::filesystem::current_path() / "modern" / "build" / "tools";
    const auto single_config = tool_dir / directory / executable;
    if (std::filesystem::exists(single_config)) return single_config.string();
    const auto multi_config = tool_dir / directory / "Debug" / executable;
    if (std::filesystem::exists(multi_config)) return multi_config.string();
    return single_config.string();
}

std::filesystem::path find_e2e_playdh_root(const std::string& map_exe) {
    std::error_code ec;
    std::vector<std::filesystem::path> bases;
    bases.push_back(std::filesystem::current_path(ec));
    auto base = std::filesystem::absolute(std::filesystem::path(map_exe), ec).parent_path();
    for (int depth = 0; !base.empty() && depth < 8; ++depth) {
        bases.push_back(base);
        const auto parent = base.parent_path();
        if (parent == base) break;
        base = parent;
    }
    for (const auto& candidate_base : bases) {
        for (const auto& root : {
                 candidate_base / "modern" / "data" / "PlayDH",
                 candidate_base / "data" / "PlayDH"}) {
            if (std::filesystem::exists(root / "Resource" / "Server") &&
                std::filesystem::exists(root / "Image" / "InterfaceScript")) {
                return std::filesystem::weakly_canonical(root, ec);
            }
        }
    }
    return {};
}

CliArgs parse_cli(int argc, char** argv) {
    CliArgs a;
    // Resolve next to this executable so both Ninja single-config and Visual
    // Studio multi-config build trees work without a machine-specific path.
    a.login_exe = resolve_server_exe(
        argv[0], "MoxianLoginServer", "mxh_login_server.exe");
    a.agent_exe = resolve_server_exe(
        argv[0], "MoxianAgentServer", "mxh_agent_server_CHINA.exe");
    a.map_exe = resolve_server_exe(
        argv[0], "MoxianMapServer", "mxh_map_server_CHINA.exe");
    // MSSQL default matches the verified LocalDB command from the P0 E2E:
    //   --backend mssql_odbc --db "backend=mssql_odbc;host=(localdb)\MSSQLLocalDB;database=Moxiang;"
    a.db = "backend=mssql_odbc;host=(localdb)\\MSSQLLocalDB;database=Moxiang;encrypt=no;trust_server_certificate=yes;";
    for (int i = 1; i < argc; ++i) {
        const std::string_view s = argv[i];
        if      (s == "--login-exe" && i + 1 < argc) a.login_exe = argv[++i];
        else if (s == "--agent-exe" && i + 1 < argc) a.agent_exe = argv[++i];
        else if (s == "--map-exe"   && i + 1 < argc) a.map_exe   = argv[++i];
        else if (s == "--backend"   && i + 1 < argc) a.db_backend = argv[++i];
        else if (s == "--db"        && i + 1 < argc) {
            a.db = argv[++i];
            a.db_explicit = true;
        }
        else if (s == "--no-spawn") a.no_spawn = true;
        else if (s == "--character-name" && i + 1 < argc) a.character_name = argv[++i];
        else if (s == "--account" && i + 1 < argc) a.account = argv[++i];
        else if (s == "--password" && i + 1 < argc) a.password = argv[++i];
        else if (s == "--login-host" && i + 1 < argc) a.login_host = argv[++i];
        else if (s == "--agent-host" && i + 1 < argc) a.agent_host = argv[++i];
        else if (s == "--map-host" && i + 1 < argc) a.map_host = argv[++i];
        else if (s == "--map-number" && i + 1 < argc) a.map_number = std::atoi(argv[++i]);
        else if (s == "--dump-cli") a.dump_cli = true;
        else if (s == "--timeout"   && i + 1 < argc) a.timeout_s = std::atoi(argv[++i]);
        else if (s == "--use-hsel")  a.use_hsel = true;
        else if (s == "--exercise-combat") a.exercise_combat = true;
        else if (s == "--exercise-skills") a.exercise_skills = true;
        else if (s == "--exercise-mapchange") a.exercise_mapchange = true;
        else if (s == "--init-schema") a.init_schema = true;
        else {
            std::fprintf(stderr, "unknown arg: %s\n", std::string(s).c_str());
            std::exit(3);
        }
    }
    return a;
}

// ---------------------------------------------------------------------------
// Lightweight logging
// ---------------------------------------------------------------------------
#define LOG(fmt, ...) std::fprintf(stderr, "[e2e] " fmt "\n", ##__VA_ARGS__)

// ---------------------------------------------------------------------------
// Server process management (CreateProcessW)
// ---------------------------------------------------------------------------
#ifdef _WIN32
struct ServerProc {
    std::string name;
    std::string exe;
    std::string cmdline;
    HANDLE      process = nullptr;
    HANDLE      thread  = nullptr;
    DWORD       pid     = 0;

    void spawn_with_args(const std::string& workdir,
                         const std::vector<std::string>& args) {
        // Build a single command-line string.  CreateProcessW wants the
        // whole thing in one string (no argv split).
        std::string line = "\"" + exe + "\"";
        for (const auto& a : args) {
            line += " \"" + a + "\"";
        }
        cmdline = line;

        STARTUPINFOW si{};
        si.cb = sizeof(si);
        // Do not inherit our redirected stdout/stderr handles: the three
        // servers hold the pipes open for their whole lifetime, which
        // would keep a parent that waits on the tool from ever seeing
        // EOF (the commercial smoke gate hangs at exit).  Null handles
        // give the children their own console-less std streams.
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdInput  = nullptr;
        si.hStdOutput = nullptr;
        si.hStdError  = nullptr;
        PROCESS_INFORMATION pi{};
        std::wstring wcmd(line.begin(), line.end());
        std::wstring wdir;
        if (!workdir.empty()) wdir.assign(workdir.begin(), workdir.end());

        BOOL ok = CreateProcessW(
            nullptr, wcmd.data(),
            nullptr, nullptr,
            FALSE, CREATE_NEW_PROCESS_GROUP,
            nullptr,
            wdir.empty() ? nullptr : wdir.c_str(),
            &si, &pi);
        if (!ok) {
            DWORD err = GetLastError();
            std::fprintf(stderr,
                "[e2e] %s: CreateProcessW failed (exe=%s, err=%lu)\n",
                name.c_str(), exe.c_str(), err);
            std::exit(1);
        }
        process = pi.hProcess;
        thread  = pi.hThread;
        pid     = pi.dwProcessId;
    }

    void kill() {
        if (process) {
            TerminateProcess(process, 0);
            WaitForSingleObject(process, 2000);
            CloseHandle(process);
            process = nullptr;
        }
        if (thread) {
            CloseHandle(thread);
            thread = nullptr;
        }
    }

    ~ServerProc() { kill(); }
};

bool wait_for_port(int port, int timeout_s) {
    auto deadline = std::chrono::steady_clock::now() +
                   std::chrono::seconds(timeout_s);
    while (std::chrono::steady_clock::now() < deadline) {
        SOCKET s = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (s != INVALID_SOCKET) {
            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port   = htons(static_cast<u_short>(port));
            addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            int rc = ::connect(s, reinterpret_cast<sockaddr*>(&addr),
                               sizeof(addr));
            ::closesocket(s);
            if (rc == 0) return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return false;
}

// Phase P0: bootstrap the modern MSSQL schema before spawning the three
// servers.  The LoginServer --init-schema path is SQLite-only DDL, so for
// mssql_odbc the E2E tool applies the schema itself (mirrors
// deploy/database/mx_modern_schema_mssql.sql, which needs sqlcmd + a GO
// batch splitter; the embedded subset below is exactly what the three
// modern servers query).  Idempotent: safe to run over an existing DB.
//
// Returns 0 on success (schema ready), nonzero on failure.
int ensure_mssql_schema(const CliArgs& cli) {
    auto cfg = mxh::db::ConnectionConfig::from_kv_string(cli.db);
    if (cfg.database.empty()) {
        LOG("MSSQL schema init: --db must contain 'database=...'");
        return 3;
    }
    const std::string target_db = cfg.database;

    // 1. Connect to [master] and create the target database if missing.
    auto master_cfg = cfg;
    master_cfg.database = "master";
    auto master = mxh::db::make_adapter("mssql_odbc");
    auto mr = master->connect(master_cfg);
    if (!mr) {
        LOG("MSSQL schema init: cannot connect to master (host='%s'): %s",
            cfg.host.c_str(), mr.error_message.c_str());
        return 1;
    }
    std::string create_db =
        "IF DB_ID(N'" + target_db + "') IS NULL EXEC('CREATE DATABASE [" +
        target_db + "]');";
    auto cdr = master->execute(create_db);
    if (!cdr) {
        LOG("MSSQL schema init: CREATE DATABASE %s failed: %s",
            target_db.c_str(), cdr.error_message.c_str());
        return 1;
    }
    master->disconnect();
    LOG("MSSQL schema init: database '%s' ready", target_db.c_str());

    // 2. Connect to the target DB, run the same migration entrypoint used by
    //    deployment, then create the explicitly requested E2E account.
    auto db = mxh::db::make_adapter("mssql_odbc");
    auto cr = db->connect(cfg);
    if (!cr) {
        LOG("MSSQL schema init: cannot connect to '%s': %s",
            target_db.c_str(), cr.error_message.c_str());
        return 1;
    }

    const auto migrated = mxh::db::migrate_modern_schema(*db);
    if (!migrated.ok()) {
        LOG("MSSQL schema migration failed: %s", migrated.error_message.c_str());
        return 1;
    }
    const auto account = mxh::server::create_account(
        *db, cli.account, cli.password);
    if (!account.ok() && account.status != mxh::server::AccountCreateStatus::AlreadyExists) {
        LOG("MSSQL E2E account creation failed: %s", account.message.c_str());
        return 1;
    }
    db->disconnect();
    LOG("MSSQL schema migrated to version %d", mxh::db::kModernSchemaVersion);
    return 0;
}

int prepare_sqlite_database(const std::string& path, const CliArgs& cli) {
    auto db = mxh::db::make_adapter("sqlite");
    mxh::db::ConnectionConfig cfg;
    cfg.backend = "sqlite";
    cfg.path = path;
    const auto connected = db->connect(cfg);
    if (!connected.ok()) {
        LOG("SQLite E2E connect failed: %s", connected.error_message.c_str());
        return 1;
    }
    const auto migrated = mxh::db::migrate_modern_schema(*db);
    if (!migrated.ok()) {
        LOG("SQLite schema migration failed: %s", migrated.error_message.c_str());
        return 1;
    }
    const auto account = mxh::server::create_account(*db, cli.account, cli.password);
    if (!account.ok() && account.status != mxh::server::AccountCreateStatus::AlreadyExists) {
        LOG("SQLite E2E account creation failed: %s", account.message.c_str());
        return 1;
    }
    return 0;
}
#endif  // _WIN32

// ---------------------------------------------------------------------------
// E2E flow
// ---------------------------------------------------------------------------
int run_e2e(const CliArgs& cli) {
#ifdef _WIN32
    // Initialise Winsock (WSAStartup).  TcpClient on Windows uses
    // Winsock under the hood, so it must be up.
    WSADATA wsad;
    int rc = ::WSAStartup(MAKEWORD(2, 2), &wsad);
    if (rc != 0) {
        LOG("WSAStartup failed (err=%d)", rc);
        return 1;
    }

    // ---- spawn servers (unless --no-spawn) ----
    std::vector<std::unique_ptr<ServerProc>> procs;
    if (!cli.no_spawn) {
        // MSSQL needs its target database before the shared migration runs.
        if (cli.db_backend == "mssql_odbc") {
            if (cli.init_schema) {
                const int schema_rc = ensure_mssql_schema(cli);
                if (schema_rc != 0) {
                    LOG("MSSQL schema init failed (rc=%d)", schema_rc);
                    return schema_rc;
                }
            } else {
                LOG("WARN: --backend mssql_odbc without --init-schema; "
                    "assumes schema already exists in the target DB");
            }
        }

        // All three processes intentionally share one database.
        std::string scratch, login_db, agent_db, map_db;
        if (cli.db_backend == "mssql_odbc" || cli.db_explicit) {
            // MSSQL and explicitly selected SQLite runs share one database.
            login_db = agent_db = map_db = cli.db;
        } else {
            std::error_code temp_error;
            const auto temp_root = std::filesystem::temp_directory_path(temp_error);
            if (temp_error || temp_root.empty()) {
                LOG("unable to resolve temporary directory for SQLite E2E");
                return 1;
            }
            scratch = (temp_root / "moxian-e2e-client").string();
            std::filesystem::create_directories(scratch, temp_error);
            if (temp_error) {
                LOG("unable to create SQLite E2E directory: %s", scratch.c_str());
                return 1;
            }
            login_db = scratch + "\\moxian.db";
            agent_db = map_db = login_db;
            DeleteFileA(login_db.c_str());
        }

        if (cli.db_backend == "sqlite") {
            const int schema_rc = prepare_sqlite_database(login_db, cli);
            if (schema_rc != 0) return schema_rc;
        }

        const std::string backend_flag =
            cli.db_backend == "mssql_odbc" ? "mssql_odbc" : "sqlite";
        const auto e2e_playdh_root = find_e2e_playdh_root(cli.map_exe);
        if (e2e_playdh_root.empty()) {
            LOG("unable to locate canonical PlayDH root for MapServer");
            return 3;
        }
        // LoginServer
        procs.push_back(std::make_unique<ServerProc>());
        procs.back()->name = "login";
        procs.back()->exe  = cli.login_exe;
        procs.back()->spawn_with_args("", {
            "--port", "16001",
            "--backend", backend_flag,
            "--db", login_db,
            "--agent-addr", "127.0.0.1",
            "--agent-port", "17001",
            "--legacy",
            (cli.use_hsel ? "--use-hsel" : "")});

        // AgentServer
        procs.push_back(std::make_unique<ServerProc>());
        procs.back()->name = "agent";
        procs.back()->exe  = cli.agent_exe;
        procs.back()->spawn_with_args("", {
            "--port", "17001",
            "--backend", backend_flag,
            "--db", agent_db,
            "--legacy",
            "--map-server", "127.0.0.1:18001",
            "--default-map", std::to_string(cli.map_number),
            (cli.exercise_mapchange ? "--map-server-map" : ""),
            (cli.exercise_mapchange ? "12=127.0.0.1:18002" : ""),
            (cli.use_hsel ? "--use-hsel" : "")});

        // MapServer
        procs.push_back(std::make_unique<ServerProc>());
        procs.back()->name = "map";
        procs.back()->exe  = cli.map_exe;
        procs.back()->spawn_with_args("", {
            "--port", "18001",
            "--backend", backend_flag,
            "--map", std::to_string(cli.map_number),
            "--db", map_db,
            "--resource-root", e2e_playdh_root.string(),
            "--server-resource-root", (e2e_playdh_root / "Resource" / "Server").string(),
            "--resource-profile", "playdh-current",
            "--legacy",
            (cli.use_hsel ? "--use-hsel" : "")});

        if (cli.exercise_mapchange) {
            procs.push_back(std::make_unique<ServerProc>());
            procs.back()->name = "map12";
            procs.back()->exe = cli.map_exe;
            procs.back()->spawn_with_args("", {
                "--port", "18002",
                "--backend", backend_flag,
                "--map", "12",
                "--db", map_db,
                "--resource-root", e2e_playdh_root.string(),
                "--server-resource-root", (e2e_playdh_root / "Resource" / "Server").string(),
                "--resource-profile", "playdh-current",
                "--legacy",
                (cli.use_hsel ? "--use-hsel" : "")});
        }

        // Wait for the three ports.
        if (!wait_for_port(16001, cli.timeout_s)) {
            LOG("LoginServer failed to listen on :16001 within %ds", cli.timeout_s);
            return 1;
        }
        if (!wait_for_port(17001, cli.timeout_s)) {
            LOG("AgentServer failed to listen on :17001 within %ds", cli.timeout_s);
            return 1;
        }
        if (!wait_for_port(18001, cli.timeout_s)) {
            LOG("MapServer failed to listen on :18001 within %ds", cli.timeout_s);
            return 1;
        }
        if (cli.exercise_mapchange && !wait_for_port(18002, cli.timeout_s)) {
            LOG("MapServer[12] failed to listen on :18002 within %ds", cli.timeout_s);
            return 1;
        }
        LOG("all 3 servers listening (login:16001, agent:17001, map:18001)");
    } else {
        LOG("--no-spawn: assuming servers are already up (login:16001, agent:17001, map:18001)");
    }

    // ---- CEngine + state machine ----
    mxh::client::CEngine engine;
    std::size_t audio_skill_cues = 0;
    std::size_t audio_attack_cues = 0;
    engine.SetAudioEventFn([&](mxh::client::CEngine::AudioCue cue) {
        if (cue == mxh::client::CEngine::AudioCue::Skill) ++audio_skill_cues;
        if (cue == mxh::client::CEngine::AudioCue::Attack) ++audio_attack_cues;
    });
    engine.SetSpatialAudioEventFn(
        [&](mxh::client::CEngine::AudioCue cue, float) {
            if (cue == mxh::client::CEngine::AudioCue::Skill) ++audio_skill_cues;
            if (cue == mxh::client::CEngine::AudioCue::Attack) ++audio_attack_cues;
        });
    // CEngine will accept state-change requests without a CMainGame
    // callback; we don't actually drive state transitions in this
    // headless flow (each state is started in sequence directly).
    // Give the client states the same explicit profile root used by the
    // servers so GameIn exercises the real UI/effect dependency gate rather
    // than falling back to a headless "root missing" path.
    {
        const auto root = find_e2e_playdh_root(cli.map_exe);
        if (!root.empty()) {
            engine.SetPlaydhRoot(root);
            LOG("using explicit PlayDH root: %s",
                engine.playdh_root()->string().c_str());
        }
    }

    // ---- Step 1: Login ----
    LOG("[1/5] Login: CLoginState connecting to %s:16001 ...",
        cli.login_host.c_str());
    mxh::client::CLoginState login;
    login.Start(&engine, cli.login_host.c_str(), 16001, cli.account, cli.password,
                cli.use_hsel);
    {
        auto deadline = std::chrono::steady_clock::now() +
                       std::chrono::seconds(cli.timeout_s);
        while (std::chrono::steady_clock::now() < deadline) {
            login.Process();
            // TakeLoginResult() is destructive (it zeroes the cached
            // user_idx), so wait for the ack to be *flagged* received
            // first, then drain.
            if (login.is_ack_received() || login.is_failed()) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        if (login.is_failed()) {
            LOG("[1/5] FAIL: %s", login.failure_reason().c_str());
            return 2;
        }
        if (!login.is_ack_received()) {
            LOG("[1/5] FAIL: timed out waiting for LoginAck (auth_key=%u)",
                login.auth_key());
            return 2;
        }
    }
    auto login_result = login.TakeLoginResult();
    LOG("[1/5] OK: LoginAck received, user_idx=%u agent=%s:%u",
        login_result.user_idx, login_result.agent_addr.c_str(),
        static_cast<unsigned>(login_result.agent_port));
    if (login_result.user_idx == 0) {
        LOG("[1/5] FAIL: TakeLoginResult returned user_idx=0");
        return 2;
    }

    // ---- Step 2: CharSelect ----
    // LoginAck advertises Agent IP+port. Spawned local servers listen on
    // 17001 (not the legacy 7001). Remote --no-spawn runs keep LoginAck IP
    // unless --agent-host overrides it.
    login_result.agent_port = 17001;
    if (!cli.agent_host.empty()) {
        login_result.agent_addr = cli.agent_host;
    }
    LOG("[2/5] CharSelect: CCharSelectState connecting to %s:%u ...",
        login_result.agent_addr.c_str(),
        static_cast<unsigned>(login_result.agent_port));
    mxh::client::CCharSelectState chsel;
    chsel.SetLoginResult(login_result);
    chsel.Start(&engine, cli.use_hsel);
    bool char_select_done = false;
    engine.SetStateChangeRequestFn(
        [&char_select_done](int state_id) {
            if (state_id == static_cast<int>(mxh::client::GameStateId::CharSelect)) {
                char_select_done = true;
            }
        });
    {
        auto deadline = std::chrono::steady_clock::now() +
                       std::chrono::seconds(cli.timeout_s);
        while (std::chrono::steady_clock::now() < deadline) {
            chsel.Process();
            if (!chsel.character_list().empty()) break;  // ListAck received
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        if (chsel.character_list().empty()) {
            LOG("[2/5] FAIL: timed out waiting for CharacterListAck");
            return 2;
        }
        const auto valid_count =
            std::count_if(chsel.character_list().begin(),
                          chsel.character_list().end(),
                          [](const auto& s) { return s.valid; });
        LOG("[2/5] OK: CharacterListAck received, %zu valid slot(s)",
            static_cast<std::size_t>(valid_count));
    }

    // ---- Step 3: CharMake (character creation) ----
    // Skip if the user already has a valid character slot — a fresh DB
    // starts empty, but on subsequent cycles the previous run already
    // filled one of the 5 slots.  Without this guard the harness would
    // try to create 6+ characters and fail at the slot-full limit.
    // Note: count MUST happen BEFORE chsel.Release() — the dtor clears
    // m_characters, so a post-Release count would always be 0.
    std::size_t valid_count_existing =
        std::count_if(chsel.character_list().begin(),
                      chsel.character_list().end(),
                      [](const auto& s) { return s.valid; });
    if (valid_count_existing > 0) char_select_done = true;
    chsel.Release();
    if (valid_count_existing == 0) {
        LOG("[3/5] CharMake: creating character via CCharMake ...");
        mxh::client::CCharMake charmake;
        charmake.SetLoginResult(login_result);
        charmake.Start(&engine, cli.use_hsel);
        {
            // Wait for the agent connection, then submit the creation form.
            auto deadline = std::chrono::steady_clock::now() +
                           std::chrono::seconds(cli.timeout_s);
            while (std::chrono::steady_clock::now() < deadline) {
                charmake.Process();
                if (charmake.is_connected()) break;
                if (charmake.is_failed()) break;
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            if (!charmake.is_connected()) {
                LOG("[3/5] FAIL: CCharMake never connected to AgentServer (%s)",
                    charmake.failure_reason().c_str());
                return 2;
            }
            // Unique per-run name so repeated runs on a persistent MSSQL DB
            // never collide (the agent rejects duplicate charnames).
            char name_buf[17] = {};
            if (!cli.character_name.empty()) {
                std::snprintf(name_buf, sizeof(name_buf), "%s", cli.character_name.c_str());
            } else {
                const auto now = std::chrono::steady_clock::now()
                                 .time_since_epoch()
                                 .count();
                std::snprintf(name_buf, sizeof(name_buf), "E2E%lld",
                              static_cast<long long>(now % 100000000LL));
            }
            mxh::client::CharacterMakeParams params;
            params.name       = name_buf;
            params.sex_type   = 1;
            params.body_type  = 0;
            params.hair_type  = 1;
            params.face_type  = 1;
            params.start_area = 18;
            params.height     = 1.0f;
            params.width      = 0.9f;
            if (!charmake.SubmitCharacter(params)) {
                LOG("[3/5] FAIL: SubmitCharacter rejected: %s",
                    charmake.failure_reason().c_str());
                return 2;
            }
            LOG("[3/5] submitting CharacterMakeSyn name='%s'", name_buf);
        }
        {
            // Success = the agent re-sent CharacterListAck and CCharMake
            // requested the CharSelect transition (legacy client behaviour).
            auto deadline = std::chrono::steady_clock::now() +
                           std::chrono::seconds(cli.timeout_s);
            while (std::chrono::steady_clock::now() < deadline) {
                charmake.Process();
                if (charmake.is_failed()) break;
                if (char_select_done) break;
                // char_select_done is set via the engine callback.
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            if (charmake.is_failed()) {
                LOG("[3/5] FAIL: character creation rejected: %s",
                    charmake.failure_reason().c_str());
                return 2;
            }
            // The agent dispatcher transitions CCharMake -> CCharSelect
            // when ListAck is received; the engine callback flips
            // char_select_done.  Polled via the state-change fn.
            int poll_ticks = 0;
            while (!char_select_done && poll_ticks++ < 200) {
                charmake.Process();
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            if (!char_select_done) {
                LOG("[3/5] FAIL: timed out waiting for ListAck after create");
                return 2;
            }
            LOG("[3/5] OK: character created, agent re-sent CharacterListAck");
        }
        charmake.Release();
    } else {
        LOG("[3/5] SKIP: %zu valid slot(s) already exist, reusing",
            valid_count_existing);
    }

    // ---- Step 4: re-enter CharSelect to verify the created char ----
    // The agent's refreshed list must now contain the character we just
    // created.  We do not auto-select here; the fresh state stays idle.
    LOG("[4/5] CharSelect: re-fetching list after create ...");
    mxh::client::CCharSelectState chsel2;
    chsel2.SetLoginResult(login_result);
    chsel2.Start(&engine, cli.use_hsel);
    std::uint32_t created_chrid = 0;
    std::size_t created_slot = 0;
    {
        auto deadline = std::chrono::steady_clock::now() +
                       std::chrono::seconds(cli.timeout_s);
        while (std::chrono::steady_clock::now() < deadline) {
            chsel2.Process();
            if (!chsel2.character_list().empty()) break;  // ListAck received
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        if (chsel2.character_list().empty()) {
            LOG("[4/5] FAIL: timed out waiting for CharacterListAck after create");
            return 2;
        }
        std::uint32_t valid_count = 0;
        for (std::size_t slot = 0; slot < chsel2.character_list().size(); ++slot) {
            const auto& s = chsel2.character_list()[slot];
            if (s.valid) {
                ++valid_count;
                if (created_chrid == 0) {
                    created_chrid = s.chrid;
                    created_slot = slot;
                }
            }
        }
        if (valid_count == 0) {
            LOG("[4/5] FAIL: list after create has no valid slot");
            return 2;
        }
        LOG("[4/5] OK: created character present (chrid=%u, %u valid slot(s))",
            static_cast<unsigned>(created_chrid),
            static_cast<unsigned>(valid_count));
    }
    if (!chsel2.SelectSlot(created_slot) || !chsel2.ConfirmSelection()) {
        LOG("[4/5] FAIL: cannot confirm the first character slot");
        return 2;
    }
    {
        auto deadline = std::chrono::steady_clock::now() +
                       std::chrono::seconds(cli.timeout_s);
        while (std::chrono::steady_clock::now() < deadline) {
            chsel2.Process();
            if (chsel2.selected_map() != 0) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        if (chsel2.selected_map() == 0) {
            LOG("[4/5] FAIL: timed out waiting for CharacterSelectAck");
            return 2;
        }
    }
    chsel2.Release();

    // ---- Step 5: InGame ----
    LOG("[5/5] InGame: sending GameInSyn through persistent AgentSession ...");
    mxh::client::CInGameState game;
    game.Start(&engine, created_chrid,
               static_cast<std::uint16_t>(cli.map_number));
    {
        auto deadline = std::chrono::steady_clock::now() +
                       std::chrono::seconds(cli.timeout_s);
        while (std::chrono::steady_clock::now() < deadline) {
            game.Process();
            if (game.is_in_game()) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        if (!game.is_in_game()) {
            LOG("[5/5] FAIL: timed out waiting for GameInAck");
            return 2;
        }
        // Map10 is the first vertical-slice gate.  Let the receive thread
        // drain the server's initial spawn burst, then require the complete
        // decoded AIGroup population rather than merely the first packet.
        if (cli.map_number == 10) {
            const auto spawn_deadline = std::chrono::steady_clock::now() +
                                         std::chrono::seconds(2);
            while (game.monsters().size() < 228 &&
                   std::chrono::steady_clock::now() < spawn_deadline) {
                game.Process();
                std::this_thread::sleep_for(std::chrono::milliseconds(25));
            }
            if (game.monsters().size() != 228) {
                LOG("[5/5] FAIL: Map10 monster population=%zu, expected 228",
                    game.monsters().size());
                return 2;
            }
            LOG("[5/5] OK: Map10 initial monster population=%zu", game.monsters().size());
        }
        const auto& info = game.game_info();
        LOG("[5/5] OK: GameInAck received, player_id=%u name='%s' "
            "level=%u map=%u life=%u/%u",
            info.player_id, info.name.c_str(), info.level, info.map_num,
            info.life, info.max_life);

        if (cli.exercise_mapchange) {
            constexpr std::uint16_t target_map = 12;
            LOG("[5/5] MapChange: requesting authoritative map=%u ...",
                static_cast<unsigned>(target_map));
            mxh::net::Message change;
            change.header.category = static_cast<std::uint8_t>(
                mxh::proto::Category::UserConn);
            change.header.protocol = static_cast<std::uint8_t>(
                mxh::proto::UserConnProtocol::ChangeMapSyn);
            change.header.object_id = created_chrid;
            change.payload.resize(4, 0);
            std::memcpy(change.payload.data(), &target_map, sizeof(target_map));
            if (engine.agent_session().send(change) != mxh::net::NetError::Ok) {
                LOG("[5/5] FAIL: ChangeMapSyn send failed");
                return 2;
            }
            const auto change_deadline = std::chrono::steady_clock::now() +
                                         std::chrono::seconds(cli.timeout_s * 3);
            while (game.game_info().map_num != target_map &&
                   std::chrono::steady_clock::now() < change_deadline) {
                game.Process();
                std::this_thread::sleep_for(std::chrono::milliseconds(25));
            }
            if (game.game_info().map_num != target_map) {
                LOG("[5/5] FAIL: target GameInAck map=%u expected=%u",
                    static_cast<unsigned>(game.game_info().map_num),
                    static_cast<unsigned>(target_map));
                return 2;
            }
            LOG("[5/5] OK: MapChange target GameInAck map=%u monsters=%zu npcs=%zu",
                static_cast<unsigned>(game.game_info().map_num),
                game.monsters().size(), game.npcs().size());
        }

        if (cli.exercise_combat) {
            // This is deliberately opt-in: it exercises the real client
            // request path and waits only for server-authoritative replies.
            // No damage, life, drop, or inventory value is synthesized here.
            LOG("[5/5] Combat: exercising server-authoritative attack/effect/drop path ...");
            std::size_t initial_alive = 0;
            for (const auto& monster : game.monsters()) {
                if (monster.current_life != 0) ++initial_alive;
            }
            const auto combat_deadline = std::chrono::steady_clock::now() +
                                         std::chrono::seconds(cli.timeout_s * 15);
            std::uint32_t observed_target = 0;
            std::uint32_t initial_life = 0;
            bool observed_hit = false;
            bool observed_life_change = false;
            bool observed_drop = false;
            bool observed_pickup = false;
            std::uint16_t observed_item_id = 0;
            std::unordered_map<std::uint32_t, std::uint32_t> life_before;
            for (const auto& monster : game.monsters()) {
                life_before.emplace(monster.object_id, monster.current_life);
            }
            std::size_t inventory_before = 0;
            for (const auto& item : info.items.Inventory) {
                if (!mxh::game::is_empty_slot(item)) ++inventory_before;
            }
            // Use the same authoritative movement path as the player client
            // before attacking.  Map10's spawn groups are intentionally
            // spread across the map, so an attack-only probe would otherwise
            // prove only the range guard rather than combat.
            if (!game.monsters().empty()) {
                const auto nearest = std::min_element(
                    game.monsters().begin(), game.monsters().end(),
                    [&game](const auto& lhs, const auto& rhs) {
                        const auto dx1 = static_cast<float>(lhs.position_x) - game.local_x();
                        const auto dz1 = static_cast<float>(lhs.position_z) - game.local_z();
                        const auto dx2 = static_cast<float>(rhs.position_x) - game.local_x();
                        const auto dz2 = static_cast<float>(rhs.position_z) - game.local_z();
                        return dx1 * dx1 + dz1 * dz1 < dx2 * dx2 + dz2 * dz2;
                    });
                game.send_move(nearest->position_x, nearest->position_z,
                               mxh::proto::MoveProtocol::OneTarget);
                const auto move_deadline = std::chrono::steady_clock::now() +
                                           std::chrono::seconds(3);
                while (std::chrono::steady_clock::now() < move_deadline) {
                    game.Process();
                    const auto dx = static_cast<float>(nearest->position_x) - game.local_x();
                    const auto dz = static_cast<float>(nearest->position_z) - game.local_z();
                    if (dx * dx + dz * dz <= 500.0f * 500.0f) break;
                    std::this_thread::sleep_for(std::chrono::milliseconds(25));
                }
            }
            auto next_attack = std::chrono::steady_clock::now();
            while (std::chrono::steady_clock::now() < combat_deadline) {
                game.Process();
                if (std::chrono::steady_clock::now() >= next_attack) {
                    game.try_attack();
                    next_attack = std::chrono::steady_clock::now() +
                                  std::chrono::milliseconds(850);
                }
                for (const auto& event : game.drain_effect_events()) {
                    if (event.kind == mxh::client::EffectEventKind::Hit) {
                        observed_hit = true;
                        observed_target = event.target_object_id;
                    }
                }
                for (const auto& monster : game.monsters()) {
                    const auto prior = life_before.find(monster.object_id);
                    if (prior != life_before.end() &&
                        monster.current_life < prior->second) {
                        observed_life_change = true;
                    }
                    if (monster.object_id == observed_target && initial_life == 0) {
                        initial_life = prior == life_before.end()
                            ? monster.current_life : prior->second;
                    }
                }
                if (!game.ground_drops().empty()) {
                    observed_drop = true;
                    if (observed_item_id == 0)
                        observed_item_id = game.ground_drops().front().item_id;
                    game.try_pickup();
                }
                std::size_t inventory_now = 0;
                for (const auto& item : game.game_info().items.Inventory) {
                    if (!mxh::game::is_empty_slot(item)) ++inventory_now;
                }
                if (inventory_now > inventory_before) observed_pickup = true;
                if (observed_hit && observed_life_change && observed_drop &&
                    observed_pickup) break;
                std::this_thread::sleep_for(std::chrono::milliseconds(25));
            }
            const std::size_t alive_after = std::count_if(
                game.monsters().begin(), game.monsters().end(),
                [](const auto& monster) { return monster.current_life != 0; });
            if (!observed_hit || !observed_life_change || !observed_drop ||
                !observed_pickup) {
                LOG("[5/5] FAIL: combat gate hit=%s life_change=%s drop=%s "
                    "pickup=%s alive=%zu->%zu target=%u",
                    observed_hit ? "yes" : "no",
                    observed_life_change ? "yes" : "no",
                    observed_drop ? "yes" : "no",
                    observed_pickup ? "yes" : "no",
                    initial_alive, alive_after, observed_target);
                return 2;
            }
            LOG("[5/5] OK: combat hit/effect/life/drop/pickup target=%u "
                "alive=%zu->%zu", observed_target, initial_alive, alive_after);

            // Close the live session and re-enter through a fresh state.  The
            // server must flush the picked item on GameOutSyn; merely seeing it
            // in the old state's inventory is not persistence evidence.
            game.Release();
            mxh::client::CInGameState relog;
            relog.Start(&engine, created_chrid,
                        static_cast<std::uint16_t>(cli.map_number));
            const auto relog_deadline = std::chrono::steady_clock::now() +
                                        std::chrono::seconds(cli.timeout_s * 3);
            while (!relog.is_in_game() &&
                   std::chrono::steady_clock::now() < relog_deadline) {
                relog.Process();
                std::this_thread::sleep_for(std::chrono::milliseconds(25));
            }
            if (!relog.is_in_game()) {
                LOG("[5/5] FAIL: re-login GameInAck after GameOutSyn timed out");
                return 2;
            }
            bool persisted_item = false;
            for (const auto& item : relog.game_info().items.Inventory) {
                if (item.wIconIdx == observed_item_id) {
                    persisted_item = true;
                    break;
                }
            }
            if (!persisted_item) {
                LOG("[5/5] FAIL: picked item=%u missing after fresh GameIn",
                    static_cast<unsigned>(observed_item_id));
                return 2;
            }
            LOG("[5/5] OK: GameOutSyn persistence re-login item=%u",
                static_cast<unsigned>(observed_item_id));
            relog.Release();
        }
        if (cli.exercise_skills) {
            LOG("[5/5] Skills: exercising quick-slot skill/effect path ...");
            std::size_t casts = 0;
            std::size_t life_changes = 0;
            std::size_t runtime_effect_events = 0;
            std::unordered_map<std::uint32_t, std::uint32_t> skill_life;
            for (const auto& monster : game.monsters()) {
                if (monster.current_life != 0) skill_life.emplace(
                    monster.object_id, monster.current_life);
            }
            if (!skill_life.empty()) {
                const auto target = std::find_if(
                    game.monsters().begin(), game.monsters().end(),
                    [](const auto& monster) { return monster.current_life != 0; });
                if (target != game.monsters().end()) {
                    game.send_move(target->position_x, target->position_z,
                                   mxh::proto::MoveProtocol::OneTarget);
                    const auto move_until = std::chrono::steady_clock::now() +
                                            std::chrono::seconds(3);
                    while (std::chrono::steady_clock::now() < move_until) {
                        game.Process();
                        const auto dx = static_cast<float>(target->position_x) - game.local_x();
                        const auto dz = static_cast<float>(target->position_z) - game.local_z();
                        if (dx * dx + dz * dz <= 500.0f * 500.0f) break;
                        std::this_thread::sleep_for(std::chrono::milliseconds(25));
                    }
                }
            }
            for (std::size_t slot = 0; slot < 4; ++slot) {
                game.use_quick_slot(slot);
                const auto until = std::chrono::steady_clock::now() +
                                   std::chrono::milliseconds(1100);
                while (std::chrono::steady_clock::now() < until) {
                    game.Process();
                    for (const auto& event : game.drain_effect_events()) {
                        if (event.kind == mxh::client::EffectEventKind::CastStart)
                            ++casts;
                    }
                    runtime_effect_events += game.drain_runtime_effect_events().size();
                    for (const auto& monster : game.monsters()) {
                        const auto it = skill_life.find(monster.object_id);
                        if (it != skill_life.end() && monster.current_life < it->second) {
                            ++life_changes;
                            it->second = monster.current_life;
                        }
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(25));
                }
            }
            if (casts == 0 || life_changes == 0 || runtime_effect_events == 0 ||
                audio_skill_cues == 0) {
                LOG("[5/5] FAIL: skill gate casts=%zu life_changes=%zu runtime_effect_events=%zu audio_skill_cues=%zu",
                    casts, life_changes, runtime_effect_events, audio_skill_cues);
                return 2;
            }
            LOG("[5/5] OK: quick-slot skills/effects casts=%zu life_changes=%zu runtime_effect_events=%zu audio_skill_cues=%zu",
                casts, life_changes, runtime_effect_events, audio_skill_cues);
        }
    }
    // Clean shutdown — release states and the persistent AgentSession, then
    // kill server procs (ServerProc dtor calls TerminateProcess).
    game.Release();
    login.Release();
    engine.Release();
    procs.clear();
    ::WSACleanup();
    LOG("Phase B.2.5 e2e: all 5 protocol steps passed (login/charselect/charcreate/relist/gamein)");
    return 0;
#else
    LOG("MoxianClientE2E is Windows-only (uses CreateProcessW + Winsock).");
    return 3;
#endif
}

}  // namespace

int main(int argc, char** argv) {
    auto cli = parse_cli(argc, argv);
    if (cli.dump_cli) {
        std::fprintf(stdout, "login_host=%s\n", cli.login_host.c_str());
        std::fprintf(stdout, "agent_host=%s\n", cli.agent_host.empty()
            ? "(loginack)" : cli.agent_host.c_str());
        std::fprintf(stdout, "map_host=%s\n", cli.map_host.empty()
            ? "(agentsession)" : cli.map_host.c_str());
        std::fprintf(stdout, "login_port=16001\nagent_port=17001\nmap_port=18001\n");
        std::fprintf(stdout, "no_spawn=%s\n", cli.no_spawn ? "true" : "false");
        return 0;
    }
    return run_e2e(cli);
}
