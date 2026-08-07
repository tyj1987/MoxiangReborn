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
//                DB, so we do NOT issue CharacterSelectSyn (we expect
//                Nack anyway).
//   3. Map:      CInGameState connects to MapServer (:8001), sends
//                GameInSyn, parses GameInAck (3000B SEND_HERO_TOTALINFO).
//
// All three steps must PASS for the tool to exit 0.
//
// Build:
//   cmake --build modern/build --config Debug --target mxh_client_e2e
//
// Usage:
//   mxh_client_e2e [--login-exe PATH] [--agent-exe PATH] [--map-exe PATH]
//                  [--no-spawn]  # assume servers are already running
//                  [--timeout N] # per-step timeout in seconds (default 10)
//
// Exit codes:
//   0  - all 3 protocol steps passed
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
#include "CInGameState.hpp"
#include "CEngine.hpp"

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
    bool no_spawn = false;
    int  timeout_s = 10;
    bool use_hsel = false;  // Phase R-1: run the whole chain HSEL-encrypted
};

CliArgs parse_cli(int argc, char** argv) {
    CliArgs a;
    // Default exe paths: assume the build dir produced them under
    // modern/build/tools/MoxianXxxServer/Debug/.
    const std::string build_dir = "C:/moxiang/modern/build/tools";
    a.login_exe = build_dir + "/MoxianLoginServer/Debug/mxh_login_server.exe";
    a.agent_exe = build_dir + "/MoxianAgentServer/Debug/mxh_agent_server_CHINA.exe";
    a.map_exe   = build_dir + "/MoxianMapServer/Debug/mxh_map_server_CHINA.exe";
    for (int i = 1; i < argc; ++i) {
        const std::string_view s = argv[i];
        if      (s == "--login-exe" && i + 1 < argc) a.login_exe = argv[++i];
        else if (s == "--agent-exe" && i + 1 < argc) a.agent_exe = argv[++i];
        else if (s == "--map-exe"   && i + 1 < argc) a.map_exe   = argv[++i];
        else if (s == "--no-spawn") a.no_spawn = true;
        else if (s == "--timeout"   && i + 1 < argc) a.timeout_s = std::atoi(argv[++i]);
        else if (s == "--use-hsel")  a.use_hsel = true;
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
        // DB files go in modern/scratch/e2e_client/ so they auto-clean
        // (we delete the dir on success; verify_servers_e2e.py uses
        // a similar pattern).
        const std::string scratch = "C:\\moxiang\\modern\\scratch\\e2e_client";
        CreateDirectoryA(scratch.c_str(), nullptr);
        const std::string login_db = scratch + "\\login.db";
        const std::string agent_db = scratch + "\\agent.db";
        const std::string map_db   = scratch + "\\map.db";

        // LoginServer
        procs.push_back(std::make_unique<ServerProc>());
        procs.back()->name = "login";
        procs.back()->exe  = cli.login_exe;
        procs.back()->spawn_with_args("", {
            "--port", "16001",
            "--db", login_db,
            "--agent-addr", "127.0.0.1",
            "--agent-port", "17001",
            "--init-schema",
            "--legacy",
            (cli.use_hsel ? "--use-hsel" : "")});

        // AgentServer
        procs.push_back(std::make_unique<ServerProc>());
        procs.back()->name = "agent";
        procs.back()->exe  = cli.agent_exe;
        procs.back()->spawn_with_args("", {
            "--port", "17001",
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
    LOG("[1/3] Login: CLoginState connecting to 127.0.0.1:16001 ...");
    mxh::client::CLoginState login;
    login.Start(&engine, "127.0.0.1", 16001, "test", "test",
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
            LOG("[1/3] FAIL: %s", login.failure_reason().c_str());
            return 2;
        }
        if (!login.is_ack_received()) {
            LOG("[1/3] FAIL: timed out waiting for LoginAck (auth_key=%u)",
                login.auth_key());
            return 2;
        }
    }
    auto login_result = login.TakeLoginResult();
    LOG("[1/3] OK: LoginAck received, user_idx=%u agent=%s:%u",
        login_result.user_idx, login_result.agent_addr.c_str(),
        static_cast<unsigned>(login_result.agent_port));
    if (login_result.user_idx == 0) {
        LOG("[1/3] FAIL: TakeLoginResult returned user_idx=0");
        return 2;
    }

    // ---- Step 2: CharSelect ----
    // We bypass the agent port returned in LoginAck (it would be
    // 7001 by default; for the spawned servers it's 17001).  Use
    // the spawned server's actual port.
    LOG("[2/3] CharSelect: CCharSelectState connecting to 127.0.0.1:17001 ...");
    mxh::client::CCharSelectState chsel;
    // Override the port the LoginAck would have told us.
    login_result.agent_port = 17001;
    chsel.SetLoginResult(login_result);
    chsel.Start(&engine, cli.use_hsel);
    {
        auto deadline = std::chrono::steady_clock::now() +
                       std::chrono::seconds(cli.timeout_s);
        while (std::chrono::steady_clock::now() < deadline) {
            if (!chsel.character_list().empty()) break;  // ListAck received
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        if (chsel.character_list().empty()) {
            LOG("[2/3] FAIL: timed out waiting for CharacterListAck");
            return 2;
        }
        const auto valid_count =
            std::count_if(chsel.character_list().begin(),
                          chsel.character_list().end(),
                          [](const auto& s) { return s.valid; });
        LOG("[2/3] OK: CharacterListAck received, %zu valid slot(s)",
            static_cast<std::size_t>(valid_count));
    }
    // DB is empty so no valid slot exists; we don't issue
    // CharacterSelectSyn.  Move on to the in-game state with the
    // login-ack user_idx as chrid (the server treats it as
    // MSGBASE.object_id regardless of character_info state).

    // ---- Step 3: InGame ----
    LOG("[3/3] InGame: CInGameState connecting to 127.0.0.1:18001 ...");
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
    game.Start(&engine, "127.0.0.1", 18001, login_result.user_idx, 12,
               cli.use_hsel);
    {
        auto deadline = std::chrono::steady_clock::now() +
                       std::chrono::seconds(cli.timeout_s);
        while (std::chrono::steady_clock::now() < deadline) {
            if (game.is_in_game()) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        if (!game.is_in_game()) {
            LOG("[3/3] FAIL: timed out waiting for GameInAck");
            tick_stop.store(true, std::memory_order_release);
            tick_thread.join();
            return 2;
        }
        const auto& info = game.game_info();
        LOG("[3/3] OK: GameInAck received, player_id=%u name='%s' "
            "level=%u map=%u life=%u/%u",
            info.player_id, info.name.c_str(), info.level, info.map_num,
            info.life, info.max_life);
    }
    tick_stop.store(true, std::memory_order_release);
    tick_thread.join();

    // Clean shutdown — release states (disconnects TcpClient) and
    // kill server procs (ServerProc dtor calls TerminateProcess).
    login.Release();
    chsel.Release();
    game.Release();
    procs.clear();
    ::WSACleanup();
    LOG("Phase B.2.5 e2e: all 3 protocol steps passed");
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
