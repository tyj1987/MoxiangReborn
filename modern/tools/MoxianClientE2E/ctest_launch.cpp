// CTest launcher for mxh_client_e2e.
//
// Reads MXH_E2E_PORT_BASE when the test starts (not at CMake configure time)
// and execs mxh_client_e2e with --login-port, --agent-port, --map-port, and
// --map12-port. Unset -> 26001/27001/28001/28002. N -> N/N+1000/N+2000/N+2001.

#include "../../tests/support/e2e_ports.hpp"

#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <process.h>
#else
#include <unistd.h>
#endif

int main(int argc, char** argv) {
    if (argc < 2 || argv[1] == nullptr || argv[1][0] == '\0') {
        std::fprintf(stderr, "mxh_client_e2e_ctest: missing mxh_client_e2e path\n");
        return 3;
    }

    mxh::test::E2ePorts ports;
    try {
        ports = mxh::test::e2e_ports_from_base_value(
            std::getenv(mxh::test::kE2ePortBaseEnv));
    } catch (const std::runtime_error& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 3;
    }
    const auto forbidden = mxh::test::e2e_forbidden_port_message(
        ports.login, ports.agent, ports.map, ports.map12);
    if (!forbidden.empty()) {
        std::fprintf(stderr, "%s\n", forbidden.c_str());
        return 3;
    }

    std::vector<std::string> owned;
    owned.emplace_back(argv[1]);
    owned.emplace_back("--login-port");
    owned.emplace_back(std::to_string(ports.login));
    owned.emplace_back("--agent-port");
    owned.emplace_back(std::to_string(ports.agent));
    owned.emplace_back("--map-port");
    owned.emplace_back(std::to_string(ports.map));
    owned.emplace_back("--map12-port");
    owned.emplace_back(std::to_string(ports.map12));
    for (int index = 2; index < argc; ++index) {
        owned.emplace_back(argv[index] == nullptr ? "" : argv[index]);
    }

    std::vector<char*> args;
    args.reserve(owned.size() + 1);
    for (auto& item : owned) args.push_back(item.data());
    args.push_back(nullptr);

#ifdef _WIN32
    _execv(owned[0].c_str(), args.data());
#else
    execv(owned[0].c_str(), args.data());
#endif
    std::fprintf(stderr, "mxh_client_e2e_ctest: failed to start %s\n", owned[0].c_str());
    return 3;
}
