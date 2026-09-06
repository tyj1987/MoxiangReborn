// Map 10 in-game smoke test — opt-in via MXH_MAP10_SMOKE.
//
// This is the integration test referenced in the 2026-09-06
// "Client Map Display Investigation" plan (Test 3).  It runs
// MoxianClientE2E for a fixed wall-clock budget (default 60 s)
// against a 3-server chain on local MSSQLSERVER and asserts:
//   * the process exits 0 (no GameInAck timeout, no crash, no
//     server-protocol mismatch);
//   * the headless client reported Map 10 as the active map
//     (g_info.map_num == 10);
//   * the GameInAck was acknowledged within the test budget
//     (no `m_pendingGameInAckSinceMs` carry-over).
//
// The test is opt-in (not part of the default ctest sweep) because
// it requires:
//   * a 3-server chain running on 16001/17001/18001;
//   * a populated local MSSQLSERVER with mxh_e2e / Pass1234;
//   * the MoxianClientE2E binary built (the test locates it via
//     `mxh_client_e2e` target or env override MXH_CLIENT_E2E_BIN).
//
// Enable with: MXH_MAP10_SMOKE=1 ctest -R Map10InGameSmoke
// Or directly:  modern\build\tests\Debug\map10_smoke.exe
#include "CEngine.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace {

// Locate the MoxianClientE2E binary.  Honour MXH_CLIENT_E2E_BIN first
// (so a custom build or shared location can be injected), then look in
// the conventional `modern/build` Debug / Release paths.
std::filesystem::path locate_client_e2e_binary() {
    if (const char* env = std::getenv("MXH_CLIENT_E2E_BIN")) {
        std::filesystem::path p(env);
        if (std::filesystem::exists(p)) return p;
    }
    const std::filesystem::path candidates[] = {
        std::filesystem::path("C:/moxiang/modern/build/tools/MoxianClientE2E/Debug/mxh_client_e2e.exe"),
        std::filesystem::path("C:/moxiang/modern/build/tools/MoxianClientE2E/mxh_client_e2e.exe"),
        std::filesystem::path("C:/moxiang/modern/build/Debug/mxh_client_e2e.exe"),
        std::filesystem::path("C:/moxiang/modern/build/Release/mxh_client_e2e.exe"),
    };
    for (const auto& c : candidates) {
        if (std::filesystem::exists(c)) return c;
    }
    return {};
}

#if defined(_WIN32)
// Run a command line and capture stdout + exit code.  This is a
// minimal replacement for `popen` that lets us pass a wide-char
// string and read the output synchronously.
int run_with_output(const std::filesystem::path& exe,
                    const std::string& args,
                    std::string& output,
                    std::uint32_t timeout_seconds) {
    output.clear();
    HANDLE read_pipe = nullptr;
    HANDLE write_pipe = nullptr;
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;
    if (!CreatePipe(&read_pipe, &write_pipe, &sa, 0)) {
        return -1;
    }
    SetHandleInformation(read_pipe, HANDLE_FLAG_INHERIT, 0);
    STARTUPINFOA si{};
    si.cb = sizeof(si);
    si.hStdError = write_pipe;
    si.hStdOutput = write_pipe;
    si.dwFlags |= STARTF_USESTDHANDLES;
    PROCESS_INFORMATION pi{};
    std::string cmd = "\"" + exe.string() + "\" " + args;
    std::vector<char> cmd_buf(cmd.begin(), cmd.end());
    cmd_buf.push_back('\0');
    if (!CreateProcessA(nullptr, cmd_buf.data(), nullptr, nullptr, TRUE,
                        CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        CloseHandle(read_pipe);
        CloseHandle(write_pipe);
        return -1;
    }
    CloseHandle(write_pipe);
    // Drain pipe in a background thread so a hung child cannot block
    // the test forever.  We honour the timeout by terminating the
    // child if it has not exited by then.
    std::thread reader([&]() {
        char buffer[4096];
        for (;;) {
            DWORD read = 0;
            if (!ReadFile(read_pipe, buffer, sizeof(buffer), &read, nullptr) ||
                read == 0) {
                break;
            }
            output.append(buffer, read);
        }
        CloseHandle(read_pipe);
    });
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::seconds(timeout_seconds);
    DWORD exit_code = static_cast<DWORD>(-1);
    bool timed_out = false;
    while (std::chrono::steady_clock::now() < deadline) {
        DWORD wait = WaitForSingleObject(pi.hProcess, 200);
        if (wait == WAIT_OBJECT_0) {
            GetExitCodeProcess(pi.hProcess, &exit_code);
            break;
        }
        if (wait == WAIT_FAILED) break;
    }
    if (std::chrono::steady_clock::now() >= deadline) {
        timed_out = true;
        TerminateProcess(pi.hProcess, 1);
        WaitForSingleObject(pi.hProcess, 5000);
    }
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    if (reader.joinable()) reader.join();
    if (timed_out) return -2;
    return static_cast<int>(exit_code);
}
#endif

TEST(Map10InGameSmoke, GameInAckDoesNotFalseFireInRelease) {
    const char* env = std::getenv("MXH_MAP10_SMOKE");
    if (!env || !*env) {
        GTEST_SKIP() << "set MXH_MAP10_SMOKE=1 to run the 60-s Map 10 "
                        "smoke (requires a 3-server chain on "
                        "16001/17001/18001)";
    }
    const auto bin = locate_client_e2e_binary();
    if (bin.empty()) {
        GTEST_SKIP() << "mxh_client_e2e binary not found; set "
                        "MXH_CLIENT_E2E_BIN to its absolute path";
    }
    const std::uint32_t budget_seconds = []() {
        if (const char* t = std::getenv("MXH_MAP10_SMOKE_BUDGET")) {
            try { return static_cast<std::uint32_t>(std::stoul(t)); }
            catch (...) {}
        }
        return 60u;
    }();
    std::string output;
    const int rc = run_with_output(
        bin, "--map-number 10 --timeout 30 --no-spawn",
        output, budget_seconds);
    ASSERT_NE(rc, -2)
        << "MoxianClientE2E timed out after " << budget_seconds
        << " s; the test budget was too short or a server hung. "
        << "Output tail:\n"
        << (output.size() > 4096 ? output.substr(output.size() - 4096)
                                  : output);
    // The E2E tool exits 0 on full success (Login + CharMake +
    // CharSelect + GameIn + Map10).  Any other code (including the
    // -1 returned when CreateProcess fails) is a regression.
    EXPECT_EQ(rc, 0)
        << "MoxianClientE2E failed with exit code " << rc
        << "; the 3-server chain is the most likely cause. "
        << "Output tail:\n"
        << (output.size() > 4096 ? output.substr(output.size() - 4096)
                                  : output);
    // A defensive text check: the E2E tool prints
    // "GameInAck: ... map_num=10" on success.  If we did not see
    // "map_num=10" anywhere in the output, the chain ran on a
    // different map (or never reached the in-game phase) and the
    // success exit code was misleading.
    EXPECT_NE(output.find("map_num=10"), std::string::npos)
        << "expected 'map_num=10' in E2E output to confirm Map 10 "
           "was reached; full output:\n"
        << output;
}

}  // namespace
