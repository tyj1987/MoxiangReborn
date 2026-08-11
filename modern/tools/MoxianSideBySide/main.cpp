// main.cpp - MoxianSideBySide headless harness.
//
// Drives a deterministic packet sequence against both the legacy
// SWorking/* server and the modern Moxian* servers (login/agent/map),
// then diffs the responses byte-by-byte. Exits 0 when traces match.
//
// Usage:
//   mxh_side_by_side [--scenario login|enter_game|attack|shop|quest|chat|all]
//                    [--legacy-exe PATH] [--modern-server-dir DIR]
//                    [--legacy-port 6001] [--modern-port 16001]
//                    [--capture-dir DIR] [--timeout N] [--start]
//                    [--modern-legacy] [--allow-empty]

#include "packet.hpp"
#include "replay/replay.hpp"
#include "capture/packet_capture.hpp"
#include "diff/packet_diff.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")

namespace {

struct Args {
    std::string scenario = "all";
    std::string legacy_exe;
    std::string modern_exe;
    std::string modern_server_dir;
    int legacy_port = 6001;
    int legacy_agent_port = 7001;
    int legacy_map_port = 6001;
    int modern_port = 16001;
    int modern_agent_port = 17001;
    int modern_map_port = 18001;
    std::string capture_dir = "modern/scratch/sbs_captures";
    int timeout_sec = 10;
    bool start_processes = false;
    bool modern_legacy = false;
    bool allow_empty = false;
    bool ignore_trace_length = false;
    bool modern_only = false;
    std::string golden_path;
};

bool parse_args(int argc, char** argv, Args& a) {
    for (int i = 1; i < argc; ++i) {
        std::string s = argv[i];
        auto next = [&](const char* flag) -> std::string {
            if (i + 1 >= argc) {
                std::cerr << flag << " requires an argument\n";
                return {};
            }
            return argv[++i];
        };
        if      (s == "--scenario")    a.scenario    = next("--scenario");
        else if (s == "--legacy-exe")  a.legacy_exe  = next("--legacy-exe");
        else if (s == "--modern-exe")  a.modern_exe  = next("--modern-exe");
        else if (s == "--modern-server-dir") a.modern_server_dir = next("--modern-server-dir");
        else if (s == "--legacy-port") a.legacy_port = std::stoi(next("--legacy-port"));
        else if (s == "--legacy-agent-port") a.legacy_agent_port = std::stoi(next("--legacy-agent-port"));
        else if (s == "--legacy-map-port") a.legacy_map_port = std::stoi(next("--legacy-map-port"));
        else if (s == "--modern-port") a.modern_port = std::stoi(next("--modern-port"));
        else if (s == "--modern-agent-port") a.modern_agent_port = std::stoi(next("--modern-agent-port"));
        else if (s == "--modern-map-port") a.modern_map_port = std::stoi(next("--modern-map-port"));
        else if (s == "--capture-dir") a.capture_dir = next("--capture-dir");
        else if (s == "--timeout")     a.timeout_sec = std::stoi(next("--timeout"));
        else if (s == "--start")       a.start_processes = true;
        else if (s == "--modern-legacy") a.modern_legacy = true;
        else if (s == "--allow-empty") a.allow_empty = true;
        else if (s == "--ignore-trace-length") a.ignore_trace_length = true;
        else if (s == "--modern-only") a.modern_only = true;
        else if (s == "--golden") a.golden_path = next("--golden");
        else if (s == "--help" || s == "-h") {
            std::cout << "mxh_side_by_side [--scenario NAME] [--legacy-exe PATH] ...\n  [--modern-only] [--golden PATH]\n";
            return false;
        } else {
            std::cerr << "unknown arg: " << s << "\n";
            return false;
        }
    }
    return true;
}

int endpoint_port(mxh::tools::sidebyside::ReplayEndpoint endpoint,
                  const Args& args, bool modern) {
    switch (endpoint) {
    case mxh::tools::sidebyside::ReplayEndpoint::Login:
        return modern ? args.modern_port : args.legacy_port;
    case mxh::tools::sidebyside::ReplayEndpoint::Agent:
        return modern ? args.modern_agent_port : args.legacy_agent_port;
    case mxh::tools::sidebyside::ReplayEndpoint::Map:
        return modern ? args.modern_map_port : args.legacy_map_port;
    }
    return modern ? args.modern_port : args.legacy_port;
}

struct ChildProcess {
    PROCESS_INFORMATION info{};
    bool started = false;
};

struct ServerLaunch {
    std::string executable;
    std::vector<std::string> args;
};

ChildProcess start_process(const ServerLaunch& launch) {
    ChildProcess child;
    if (launch.executable.empty()) return child;
    std::string command = "\"" + launch.executable + "\"";
    for (const auto& a : launch.args) { command += " \""; command += a; command += "\""; }
    STARTUPINFOA startup{};
    startup.cb = sizeof(startup);
    if (!::CreateProcessA(nullptr, command.data(), nullptr, nullptr, FALSE,
                          CREATE_NEW_PROCESS_GROUP, nullptr, nullptr,
                          &startup, &child.info)) {
        std::cerr << "CreateProcess failed for " << launch.executable
                  << " error=" << ::GetLastError() << "\n";
        return child;
    }
    child.started = true;
    return child;
}

void stop_process(ChildProcess& child) {
    if (!child.started) return;
    if (::WaitForSingleObject(child.info.hProcess, 0) == WAIT_TIMEOUT)
        ::TerminateProcess(child.info.hProcess, 0);
    ::WaitForSingleObject(child.info.hProcess, 3000);
    ::CloseHandle(child.info.hThread);
    ::CloseHandle(child.info.hProcess);
    child = {};
}

SOCKET tcp_connect(int port) {
    SOCKET s = ::socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) return INVALID_SOCKET;
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<u_short>(port));
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (::connect(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        ::closesocket(s);
        return INVALID_SOCKET;
    }
    return s;
}

bool send_all(SOCKET s, const std::uint8_t* data, std::size_t n) {
    std::size_t sent = 0;
    while (sent < n) {
        int r = ::send(s, reinterpret_cast<const char*>(data + sent),
                       static_cast<int>(n - sent), 0);
        if (r <= 0) return false;
        sent += static_cast<std::size_t>(r);
    }
    return true;
}

bool recv_n(SOCKET s, std::uint8_t* out, std::size_t n) {
    std::size_t got = 0;
    while (got < n) {
        int r = ::recv(s, reinterpret_cast<char*>(out + got),
                       static_cast<int>(n - got), 0);
        if (r <= 0) return false;
        got += static_cast<std::size_t>(r);
    }
    return true;
}

mxh::tools::sidebyside::Packet recv_one(SOCKET s) {
    std::uint8_t prefix[2];
    if (!recv_n(s, prefix, 2)) return {};
    const auto bodyLength = static_cast<std::size_t>(prefix[0]) |
                            (static_cast<std::size_t>(prefix[1]) << 8u);
    if (bodyLength < 8u || bodyLength > 1024u * 1024u) return {};
    std::vector<std::uint8_t> body(bodyLength);
    std::size_t got = 0;
    got = 0; while (got < body.size()) { int rr = ::recv(s, reinterpret_cast<char*>(body.data() + got), static_cast<int>(body.size() - got), 0); if (rr <= 0) break; got += static_cast<std::size_t>(rr); }
    if (got < body.size()) {
        // EOF after a valid prefix: still surface the bytes we got.
        // The MapServer (and other modern servers) may close the
        // connection immediately after sending the response; we
        // should not drop the response just because the body
        // read ran out before filling the declared length.
        if (got < 8u) return {};
        body.resize(got);
    }
    mxh::tools::sidebyside::Packet p;
    p.checksum = body[0];
    p.code = static_cast<std::int8_t>(body[1]);
    p.category = body[2];
    p.protocol = body[3];
    p.object_id = static_cast<std::uint32_t>(body[4]) |
                  (static_cast<std::uint32_t>(body[5]) << 8u) |
                  (static_cast<std::uint32_t>(body[6]) << 16u) |
                  (static_cast<std::uint32_t>(body[7]) << 24u);
    p.length = static_cast<std::uint32_t>(bodyLength - 8u);
    p.payload.assign(body.begin() + 8, body.end());
    return p;
}

bool send_one(SOCKET s, const mxh::tools::sidebyside::Packet& p) {
    const auto bytes = p.wire_bytes();
    return send_all(s, bytes.data(), bytes.size());
}

std::vector<mxh::tools::sidebyside::Packet> run_scenario(
    int port, const mxh::tools::sidebyside::ReplayScenario& sc,
    int timeout_sec) {
    std::vector<mxh::tools::sidebyside::Packet> trace;
    SOCKET s = tcp_connect(port);
    if (s == INVALID_SOCKET) return trace;
    DWORD t = static_cast<DWORD>(timeout_sec) * 1000;
    ::setsockopt(s, SOL_SOCKET, SO_RCVTIMEO,
                 reinterpret_cast<const char*>(&t), sizeof(t));
    for (auto& p : sc.client_packets) {
        if (!send_one(s, p)) break;
    }
    auto start = std::chrono::steady_clock::now();
    while (true) {
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - start).count() >= timeout_sec)
            break;
        mxh::tools::sidebyside::Packet r = recv_one(s);
        if (r.length == 0 && r.category == 0 && r.protocol == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }
        r.direction = "s->c";
        r.timestamp_ns = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count());
        trace.push_back(std::move(r));
    }
    ::closesocket(s);
    return trace;
}

}  // namespace

int main(int argc, char** argv) {
    WSADATA wsadata;
    if (::WSAStartup(MAKEWORD(2, 2), &wsadata) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }
    Args a;
    if (!parse_args(argc, argv, a)) {
        ::WSACleanup();
        return 1;
    }
    std::filesystem::create_directories(a.capture_dir);
    ChildProcess legacy_login;
    ChildProcess modern_login;
    ChildProcess modern_agent;
    ChildProcess modern_map;
    if (a.start_processes) {
        if (!a.legacy_exe.empty()) {
            legacy_login = start_process({a.legacy_exe, {}});
        }
        if (!a.modern_server_dir.empty()) {
            const auto loginPath = a.modern_server_dir + "/MoxianLoginServer/Debug/mxh_login_server.exe";
            const auto agentPath = a.modern_server_dir + "/MoxianAgentServer/Debug/mxh_agent_server_CHINA.exe";
            const auto mapPath   = a.modern_server_dir + "/MoxianMapServer/Debug/mxh_map_server_CHINA.exe";
            ServerLaunch ml{loginPath, {}};
            if (a.modern_legacy) ml.args.push_back("--legacy");
            ml.args.push_back("--port"); ml.args.push_back(std::to_string(a.modern_port));
            ml.args.push_back("--init-schema");
            ml.args.push_back("--agent-addr"); ml.args.push_back("127.0.0.1");
            ml.args.push_back("--agent-port"); ml.args.push_back(std::to_string(a.modern_agent_port));
            std::vector<std::string> ma_args = {"--port", std::to_string(a.modern_agent_port),
                                                "--map-server", "127.0.0.1:" + std::to_string(a.modern_map_port)};
if (a.modern_legacy) ma_args.push_back("--legacy");
ServerLaunch ma{agentPath, std::move(ma_args)};
            ServerLaunch mm{mapPath, {"--port", std::to_string(a.modern_map_port), "--map", "12", "--dev-stub-caster"}};
            modern_login = start_process(ml);
            modern_agent = start_process(ma);
            modern_map   = start_process(mm);
        } else if (!a.modern_exe.empty()) {
            ServerLaunch ml{a.modern_exe, {}};
            if (a.modern_legacy) ml.args.push_back("--legacy");
            ml.args.push_back("--port"); ml.args.push_back(std::to_string(a.modern_port));
            modern_login = start_process(ml);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
        const bool modernOk = a.modern_server_dir.empty()
            ? (a.modern_exe.empty() || modern_login.started)
            : (modern_login.started && modern_agent.started && modern_map.started);
        const bool legacyOk = a.legacy_exe.empty() || legacy_login.started;
        if (!modernOk || !legacyOk) {
            std::cerr << "sbs: failed to spawn processes legacy_ok="
                      << (legacyOk?"y":"n") << " modern_ok="
                      << (modernOk?"y":"n") << "\n";
            stop_process(legacy_login); stop_process(modern_login);
            stop_process(modern_agent); stop_process(modern_map);
            ::WSACleanup(); return 2;
        }
    }
    std::cout << "MoxianSideBySide v0.3 scenario=" << a.scenario << "\n";
    const std::vector<mxh::tools::sidebyside::ReplayScenario> scenarios = {
        mxh::tools::sidebyside::login_scenario(),
        mxh::tools::sidebyside::enter_game_scenario(),
        mxh::tools::sidebyside::attack_scenario(),
        mxh::tools::sidebyside::shop_scenario(),
        mxh::tools::sidebyside::quest_scenario(),
        mxh::tools::sidebyside::chat_scenario(),
    };
    int rc = 0;
    mxh::tools::sidebyside::DiffOptions options;
    options.ignore_object_id = true;
    options.ignore_trace_length_mismatch = a.ignore_trace_length;
    for (const auto& scenario : scenarios) {
        if (a.scenario != "all" && a.scenario != scenario.name) continue;
        const int legacyPort = endpoint_port(scenario.endpoint, a, false);
        const int modernPort = endpoint_port(scenario.endpoint, a, true);
        std::cout << "[" << scenario.name << "] endpoint="
                  << mxh::tools::sidebyside::endpoint_name(scenario.endpoint)
                  << " legacy_port=" << legacyPort
                  << " modern_port=" << modernPort << " running...\n";
        std::vector<mxh::tools::sidebyside::Packet> legacyTrace;
        if (a.modern_only && !a.golden_path.empty()) {
            legacyTrace = mxh::tools::sidebyside::load_capture(a.golden_path);
            std::cout << "[" << scenario.name << "] modern-only golden=" << a.golden_path
                      << " loaded=" << legacyTrace.size() << " (expected)" << "\n";
        } else if (a.modern_only) {
            std::cout << "[" << scenario.name << "] modern-only (no golden) skipping legacy" << "\n";
        } else {
            legacyTrace = run_scenario(legacyPort, scenario, a.timeout_sec);
        }
        const auto modernTrace = run_scenario(modernPort, scenario, a.timeout_sec);
        const auto legacyPath = a.capture_dir + "/legacy_" + scenario.name + ".cap";
        const auto modernPath = a.capture_dir + "/modern_" + scenario.name + ".cap";
        if (!a.modern_only) {
            mxh::tools::sidebyside::save_capture(legacyTrace, legacyPath);
        }
        mxh::tools::sidebyside::save_capture(modernTrace, modernPath);
        const auto diffs = mxh::tools::sidebyside::diff_traces(legacyTrace, modernTrace, options);
        const bool missingTrace = legacyTrace.empty() || modernTrace.empty();
        std::cout << "  expected=" << legacyTrace.size() << " modern="
                  << modernTrace.size() << " diff=" << diffs.size();
        if (missingTrace) std::cout << " missing_trace=true";
        std::cout << "\n";
        for (const auto& d : diffs) {
            std::cout << "  DIFF index=" << d.index << " offset="
                      << d.first_diff_offset << " expected=0x" << std::hex
                      << static_cast<unsigned>(d.expected_byte) << " actual=0x"
                      << static_cast<unsigned>(d.actual_byte) << std::dec << "\n";
        }
        if (!diffs.empty() || (missingTrace && !a.allow_empty)) rc = 3;
    }
    stop_process(legacy_login);
    stop_process(modern_login);
    stop_process(modern_agent);
    stop_process(modern_map);
    ::WSACleanup();
    return rc;
}
