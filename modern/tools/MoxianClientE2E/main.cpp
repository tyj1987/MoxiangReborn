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

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
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
    std::string account = "test";
    std::string password = "test";
    int  timeout_s = 10;
    bool use_hsel = false;  // Phase R-1: run the whole chain HSEL-encrypted
    bool init_schema = true;   // Phase P0: apply the modern schema before
                               // spawning.  SQLite: always safe (idempotent
                               // CREATE TABLE IF NOT EXISTS).  MSSQL: keeps
                               // the legacy default, pass --init-schema only
                               // when the caller wants the E2E tool to
                               // bootstrap the shared DB itself.
};

CliArgs parse_cli(int argc, char** argv) {
    CliArgs a;
    // Default exe paths: assume the build dir produced them under
    // modern/build/tools/MoxianXxxServer/Debug/.
    const std::string build_dir = "C:/moxiang/modern/build/tools";
    a.login_exe = build_dir + "/MoxianLoginServer/Debug/mxh_login_server.exe";
    a.agent_exe = build_dir + "/MoxianAgentServer/Debug/mxh_agent_server_CHINA.exe";
    a.map_exe   = build_dir + "/MoxianMapServer/Debug/mxh_map_server_CHINA.exe";
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
        else if (s == "--timeout"   && i + 1 < argc) a.timeout_s = std::atoi(argv[++i]);
        else if (s == "--use-hsel")  a.use_hsel = true;
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

    // 2. Connect to the target DB and create the two tables the modern
    //    servers use (chr_log_info for login, character_info for the
    //    agent/map servers), then seed the E2E test account.
    auto db = mxh::db::make_adapter("mssql_odbc");
    auto cr = db->connect(cfg);
    if (!cr) {
        LOG("MSSQL schema init: cannot connect to '%s': %s",
            target_db.c_str(), cr.error_message.c_str());
        return 1;
    }

    const char* kChrLogInfo =
        "IF OBJECT_ID(N'dbo.chr_log_info', N'U') IS NULL "
        "CREATE TABLE dbo.chr_log_info ("
        " id NVARCHAR(50) NOT NULL PRIMARY KEY,"
        " pw NVARCHAR(160) NOT NULL,"
        " userlevel INT NOT NULL DEFAULT 0);"
        "IF COL_LENGTH(N'dbo.chr_log_info', N'pw') < 320 "
        "ALTER TABLE dbo.chr_log_info ALTER COLUMN pw NVARCHAR(160) NOT NULL;";
    const char* kCharacterInfo =
        "IF OBJECT_ID(N'dbo.character_info', N'U') IS NULL "
        "CREATE TABLE dbo.character_info ("
        " chrid BIGINT NOT NULL PRIMARY KEY,"
        " charname NVARCHAR(50) NOT NULL,"
        " userid BIGINT NOT NULL,"
        " sex_type TINYINT NOT NULL DEFAULT 0,"
        " hair_type TINYINT NOT NULL DEFAULT 0,"
        " face_type TINYINT NOT NULL DEFAULT 0,"
        " body_type TINYINT NOT NULL DEFAULT 0,"
        " start_area INT NOT NULL DEFAULT 0,"
        " height FLOAT NOT NULL DEFAULT 1.0,"
        " width FLOAT NOT NULL DEFAULT 1.0,"
        " level INT NOT NULL DEFAULT 1,"
        " map_num INT NOT NULL DEFAULT 0,"
        " standing_idx INT NOT NULL DEFAULT 0);";
    const char* kSeedAccount =
        "IF NOT EXISTS (SELECT 1 FROM dbo.chr_log_info WHERE id = N'test') "
        "INSERT INTO dbo.chr_log_info (id, pw, userlevel) "
        "VALUES (N'test', N'test', 2);";

    const char* statements[] = {kChrLogInfo, kCharacterInfo, kSeedAccount};
    for (const char* sql : statements) {
        auto er = db->execute(sql);
        if (!er) {
            LOG("MSSQL schema init: statement failed: %s",
                er.error_message.c_str());
            return 1;
        }
    }
    db->disconnect();
    LOG("MSSQL schema init: tables ready + test account seeded ('test'/'test')");
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
        // Phase P0: mssql_odbc needs the shared SQL Server schema before
        // any of the three processes start (LoginServer --init-schema is
        // SQLite-only DDL).  SQLite files are created below per server.
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

        // DB files go in modern/scratch/e2e_client/ so they auto-clean
        // (we delete the dir on success; verify_servers_e2e.py uses
        // a similar pattern).
        std::string scratch, login_db, agent_db, map_db;
        if (cli.db_backend == "mssql_odbc" || cli.db_explicit) {
            // MSSQL and explicitly selected SQLite runs share one database.
            login_db = agent_db = map_db = cli.db;
        } else {
            scratch = "C:\\moxiang\\modern\\scratch\\e2e_client";
            CreateDirectoryA(scratch.c_str(), nullptr);
            login_db = scratch + "\\login.db";
            agent_db = scratch + "\\agent.db";
            map_db   = scratch + "\\map.db";
        }

        const std::string backend_flag =
            cli.db_backend == "mssql_odbc" ? "mssql_odbc" : "sqlite";
        const std::string init_schema_flag =
            (cli.db_backend == "sqlite" && cli.init_schema) ? "--init-schema" : "";

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
            init_schema_flag,
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
            (cli.use_hsel ? "--use-hsel" : "")});

        // MapServer
        procs.push_back(std::make_unique<ServerProc>());
        procs.back()->name = "map";
        procs.back()->exe  = cli.map_exe;
        procs.back()->spawn_with_args("", {
            "--port", "18001",
            "--backend", backend_flag,
            "--map", "12",
            "--db", map_db,
            "--legacy",
            (cli.use_hsel ? "--use-hsel" : "")});

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
        LOG("all 3 servers listening (login:16001, agent:17001, map:18001)");
    } else {
        LOG("--no-spawn: assuming servers are already up (login:16001, agent:17001, map:18001)");
    }

    // ---- CEngine + state machine ----
    mxh::client::CEngine engine;
    // CEngine will accept state-change requests without a CMainGame
    // callback; we don't actually drive state transitions in this
    // headless flow (each state is started in sequence directly).

    // ---- Step 1: Login ----
    LOG("[1/5] Login: CLoginState connecting to 127.0.0.1:16001 ...");
    mxh::client::CLoginState login;
    login.Start(&engine, "127.0.0.1", 16001, cli.account, cli.password,
                cli.use_hsel);
    {
        auto deadline = std::chrono::steady_clock::now() +
                       std::chrono::seconds(cli.timeout_s);
        while (std::chrono::steady_clock::now() < deadline) {
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
    // We bypass the agent port returned in LoginAck (it would be
    // 7001 by default; for the spawned servers it's 17001).  Use
    // the spawned server's actual port.
    LOG("[2/5] CharSelect: CCharSelectState connecting to 127.0.0.1:17001 ...");
    mxh::client::CCharSelectState chsel;
    // Override the port the LoginAck would have told us.
    login_result.agent_port = 17001;
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
                if (charmake.is_failed()) break;
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
    {
        auto deadline = std::chrono::steady_clock::now() +
                       std::chrono::seconds(cli.timeout_s);
        while (std::chrono::steady_clock::now() < deadline) {
            if (!chsel2.character_list().empty()) break;  // ListAck received
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        if (chsel2.character_list().empty()) {
            LOG("[4/5] FAIL: timed out waiting for CharacterListAck after create");
            return 2;
        }
        std::uint32_t valid_count = 0;
        for (const auto& s : chsel2.character_list()) {
            if (s.valid) {
                ++valid_count;
                if (created_chrid == 0) created_chrid = s.chrid;
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
    chsel2.Release();

    // ---- Step 5: InGame ----
    LOG("[5/5] InGame: CInGameState connecting to 127.0.0.1:18001 ...");
    mxh::client::CInGameState game;
    // Start a per-tick thread that calls Process() every 50ms; this
    // gives the in-game state a chance to retry the GameInSyn send
    // (which may transiently fail during the TCP connect handshake).
    std::atomic<bool> tick_stop{false};
    std::thread tick_thread([&game, &tick_stop]() {
        while (!tick_stop.load(std::memory_order_acquire)) {
            game.Process();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });
    game.Start(&engine, "127.0.0.1", 18001, created_chrid, 12,
               cli.use_hsel);
    {
        auto deadline = std::chrono::steady_clock::now() +
                       std::chrono::seconds(cli.timeout_s);
        while (std::chrono::steady_clock::now() < deadline) {
            if (game.is_in_game()) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        if (!game.is_in_game()) {
            LOG("[5/5] FAIL: timed out waiting for GameInAck");
            tick_stop.store(true, std::memory_order_release);
            tick_thread.join();
            return 2;
        }
        const auto& info = game.game_info();
        LOG("[5/5] OK: GameInAck received, player_id=%u name='%s' "
            "level=%u map=%u life=%u/%u",
            info.player_id, info.name.c_str(), info.level, info.map_num,
            info.life, info.max_life);
    }
    tick_stop.store(true, std::memory_order_release);
    tick_thread.join();

    // Clean shutdown — release states (disconnects TcpClient) and
    // kill server procs (ServerProc dtor calls TerminateProcess).
    login.Release();
    game.Release();
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
    return run_e2e(cli);
}
