#pragma once

// MoxianClientE2E listen ports. Test code only.
//
// MXH_E2E_PORT_BASE is read at process start, not at CMake configure time.
//   unset or empty -> login 26001, agent 27001, map 28001, map12 28002
//   N              -> login N, agent N+1000, map N+2000, map12 N+2001
// N=26001 reproduces the defaults. Flags are --login-port, --agent-port,
// --map-port, and --map12-port. Every resolved port, including N+2001, must
// be in 1024..65535. Ports 16001, 17001, 18001, 36001, 37001, and 38001
// are refused.

#include <cerrno>
#include <cstdlib>
#include <stdexcept>
#include <string>

namespace mxh::test {

inline constexpr const char* kE2ePortBaseEnv = "MXH_E2E_PORT_BASE";
inline constexpr int kE2eDefaultLoginPort = 26001;
inline constexpr int kE2eDefaultAgentPort = 27001;
inline constexpr int kE2eDefaultMapPort = 28001;
inline constexpr int kE2eDefaultMap12Port = 28002;
inline constexpr int kE2eMinPort = 1024;
inline constexpr int kE2eMaxPort = 65535;
inline constexpr int kE2eForbiddenPorts[] = {
    16001, 17001, 18001,
    36001, 37001, 38001,
};

struct E2ePorts {
    int login = kE2eDefaultLoginPort;
    int agent = kE2eDefaultAgentPort;
    int map = kE2eDefaultMapPort;
    int map12 = kE2eDefaultMap12Port;
};

inline bool e2e_port_is_forbidden(int port) {
    for (const int forbidden : kE2eForbiddenPorts) {
        if (port == forbidden) return true;
    }
    return false;
}

inline bool e2e_port_base_text_ok(const char* value) {
    if (value == nullptr || value[0] == '\0') return false;
    const char* cursor = value;
    if (*cursor == '+') ++cursor;
    if (*cursor == '\0') return false;
    for (; *cursor != '\0'; ++cursor) {
        if (*cursor < '0' || *cursor > '9') return false;
    }
    return true;
}

inline long e2e_derived_port(long base, long offset, const char* value_text) {
    // Stay inside signed 32-bit before the range helper names the port.
    // 2147483647 is INT_MAX; spelled out so a windows.h max macro cannot fire.
    if (base < 0 || (offset > 0 && base > 2147483647L - offset)) {
        throw std::runtime_error(
            std::string(kE2ePortBaseEnv) + " is set to \"" + value_text +
            "\" but a derived port is outside 1024..65535");
    }
    return base + offset;
}

inline E2ePorts e2e_ports_from_base(long base, const char* value_text) {
    const long login = e2e_derived_port(base, 0, value_text);
    const long agent = e2e_derived_port(base, 1000, value_text);
    const long map = e2e_derived_port(base, 2000, value_text);
    const long map12 = e2e_derived_port(base, 2001, value_text);
    E2ePorts ports;
    ports.login = static_cast<int>(login);
    ports.agent = static_cast<int>(agent);
    ports.map = static_cast<int>(map);
    ports.map12 = static_cast<int>(map12);
    return ports;
}

// nullptr and "" mean the variable is unset.
inline E2ePorts e2e_ports_from_base_value(const char* value) {
    if (value == nullptr || value[0] == '\0') return E2ePorts{};
    if (!e2e_port_base_text_ok(value)) {
        throw std::runtime_error(
            std::string(kE2ePortBaseEnv) + " is set to \"" + value +
            "\" but it is not a positive integer port base");
    }
    errno = 0;
    char* end = nullptr;
    const long base = std::strtol(value, &end, 10);
    if (errno == ERANGE || end == value || end == nullptr || *end != '\0') {
        throw std::runtime_error(
            std::string(kE2ePortBaseEnv) + " is set to \"" + value +
            "\" but it is not a positive integer port base");
    }
    return e2e_ports_from_base(base, value);
}

// Explicit CLI ports replace the derived ones. The base is parsed only when
// at least one port was not passed on the command line.
inline E2ePorts resolve_e2e_cli_ports(const char* base_value,
                                      bool login_explicit, int login,
                                      bool agent_explicit, int agent,
                                      bool map_explicit, int map,
                                      bool map12_explicit, int map12) {
    const bool any_unset = !login_explicit || !agent_explicit || !map_explicit ||
                           !map12_explicit;
    E2ePorts ports = e2e_ports_from_base_value(any_unset ? base_value : nullptr);
    if (login_explicit) ports.login = login;
    if (agent_explicit) ports.agent = agent;
    if (map_explicit) ports.map = map;
    if (map12_explicit) ports.map12 = map12;
    return ports;
}

inline std::string e2e_forbidden_port_message(int login, int agent, int map, int map12) {
    struct NamedPort {
        const char* name;
        int port;
    };
    const NamedPort named[] = {
        {"login", login},
        {"agent", agent},
        {"map", map},
        {"map12", map12},
    };
    std::string listed;
    for (const auto& item : named) {
        if (!e2e_port_is_forbidden(item.port)) continue;
        if (!listed.empty()) listed += ", ";
        listed += item.name;
        listed += '=';
        listed += std::to_string(item.port);
    }
    if (listed.empty()) return {};
    return "refusing forbidden MoxianClientE2E port(s): " + listed +
           ". Ports 16001, 17001, 18001, 36001, 37001, and 38001 are not allowed";
}

inline std::string e2e_port_range_message(int login, int agent, int map, int map12) {
    struct NamedPort {
        const char* name;
        int port;
    };
    const NamedPort named[] = {
        {"login", login},
        {"agent", agent},
        {"map", map},
        {"map12", map12},
    };
    std::string listed;
    for (const auto& item : named) {
        if (item.port >= kE2eMinPort && item.port <= kE2eMaxPort) continue;
        if (!listed.empty()) listed += ", ";
        listed += item.name;
        listed += '=';
        listed += std::to_string(item.port);
    }
    if (listed.empty()) return {};
    return "MoxianClientE2E port(s) outside 1024..65535: " + listed;
}

// Launcher and the direct harness path both use this. Forbidden ports win
// over the range text; the two sets do not overlap.
inline std::string e2e_ports_reject_reason(int login, int agent, int map, int map12) {
    const auto forbidden = e2e_forbidden_port_message(login, agent, map, map12);
    if (!forbidden.empty()) return forbidden;
    return e2e_port_range_message(login, agent, map, map12);
}

}  // namespace mxh::test
