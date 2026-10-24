#pragma once

// Endpoint gate for Map10InGameSmoke.GameInAckDoesNotFalseFireInRelease.
//
// That integration test drives MoxianClientE2E, which creates a character
// and therefore writes the game database. The E2E tool's own defaults are
// login host 192.168.2.244 and ports 16001/17001/18001 (CT930). This gate
// never substitutes those defaults.
//
// The test runs only when all four variables are set to a usable value:
//   MXH_MAP10_SMOKE_HOST         login and agent host (DNS name or IP)
//   MXH_MAP10_SMOKE_LOGIN_PORT   TCP port 1..65535
//   MXH_MAP10_SMOKE_AGENT_PORT   TCP port 1..65535, distinct from the others
//   MXH_MAP10_SMOKE_MAP_PORT     TCP port 1..65535, distinct from the others
//
// A second guard refuses the default stage ports even when they are set
// explicitly: login 16001, agent 17001, and map 18001-18003. If any
// configured port is one of those, the test skips unless
//   MXH_MAP10_SMOKE_ALLOW_DEFAULT_STAGE=1
// is also set. Other values, including unset, do not allow the stage.
//
// MXH_MAP10_SMOKE=1 is still required by the integration test itself; it
// does not supply a host or any port. MXH_LOGIN_HOST is not read here.

#include <charconv>
#include <cctype>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace mxh::test {

inline constexpr const char* kMap10SmokeHostEnv = "MXH_MAP10_SMOKE_HOST";
inline constexpr const char* kMap10SmokeLoginPortEnv = "MXH_MAP10_SMOKE_LOGIN_PORT";
inline constexpr const char* kMap10SmokeAgentPortEnv = "MXH_MAP10_SMOKE_AGENT_PORT";
inline constexpr const char* kMap10SmokeMapPortEnv = "MXH_MAP10_SMOKE_MAP_PORT";
inline constexpr const char* kMap10SmokeAllowDefaultStageEnv =
    "MXH_MAP10_SMOKE_ALLOW_DEFAULT_STAGE";

// Default VM112/CT930 stage listeners. Any configured port in this set is
// refused unless MXH_MAP10_SMOKE_ALLOW_DEFAULT_STAGE=1.
inline constexpr int kDefaultStagePorts[] = {16001, 17001, 18001, 18002, 18003};

inline bool is_default_stage_port(int port) {
    for (const int blocked : kDefaultStagePorts) {
        if (port == blocked) return true;
    }
    return false;
}

struct Map10SmokeEndpoint {
    bool ready = false;
    std::string skip_reason;
    std::string host;
    int login_port = 0;
    int agent_port = 0;
    int map_port = 0;

    // Empty unless ready. Passed straight to MoxianClientE2E so the tool
    // cannot apply its compiled-in CT930 ports.
    std::string e2e_arguments(int step_timeout_seconds = 30) const {
        if (!ready) return {};
        return "--map-number 10 --timeout " + std::to_string(step_timeout_seconds) +
               " --no-spawn --login-host " + host +
               " --agent-host " + host +
               " --login-port " + std::to_string(login_port) +
               " --agent-port " + std::to_string(agent_port) +
               " --map-port " + std::to_string(map_port);
    }

    std::string log_line() const {
        if (!ready) return {};
        return "Map10InGameSmoke resolved endpoint host=" + host +
               " login_port=" + std::to_string(login_port) +
               " agent_port=" + std::to_string(agent_port) +
               " map_port=" + std::to_string(map_port);
    }
};

inline std::string trim_copy(std::string_view text) {
    while (!text.empty() &&
           std::isspace(static_cast<unsigned char>(text.front()))) {
        text.remove_prefix(1);
    }
    while (!text.empty() &&
           std::isspace(static_cast<unsigned char>(text.back()))) {
        text.remove_suffix(1);
    }
    return std::string(text);
}

inline bool valid_smoke_host(std::string_view host) {
    if (host.empty() || host.size() > 253) return false;
    for (unsigned char c : host) {
        if (!(std::isalnum(c) || c == '.' || c == '-' || c == ':' ||
              c == '[' || c == ']')) {
            return false;
        }
    }
    return true;
}

inline bool parse_tcp_port(std::string_view text, int& port) {
    if (text.empty() || text.front() == '+' || text.front() == '-') return false;
    int value = 0;
    const char* const begin = text.data();
    const char* const end = begin + text.size();
    const auto parsed = std::from_chars(begin, end, value);
    if (parsed.ec != std::errc{} || parsed.ptr != end) return false;
    if (value < 1 || value > 65535) return false;
    port = value;
    return true;
}

template <typename Lookup>
Map10SmokeEndpoint resolve_map10_smoke_endpoint(Lookup&& lookup) {
    Map10SmokeEndpoint rejected;
    std::vector<std::string> problems;

    const auto raw = [&](const char* name) {
        const char* value = lookup(name);
        return value ? trim_copy(value) : std::string();
    };

    const std::string host = raw(kMap10SmokeHostEnv);
    if (host.empty()) {
        problems.push_back(std::string(kMap10SmokeHostEnv) + " is unset");
    } else if (!valid_smoke_host(host)) {
        problems.push_back(std::string(kMap10SmokeHostEnv) +
                           " is not a DNS name or IP address");
    }

    int login_port = 0;
    int agent_port = 0;
    int map_port = 0;
    const auto take_port = [&](const char* name, int& dest) {
        const std::string text = raw(name);
        if (text.empty()) {
            problems.push_back(std::string(name) + " is unset");
            return;
        }
        if (!parse_tcp_port(text, dest)) {
            problems.push_back(std::string(name) + " is not a TCP port in 1..65535");
        }
    };
    take_port(kMap10SmokeLoginPortEnv, login_port);
    take_port(kMap10SmokeAgentPortEnv, agent_port);
    take_port(kMap10SmokeMapPortEnv, map_port);

    if (problems.empty() &&
        (login_port == agent_port || login_port == map_port ||
         agent_port == map_port)) {
        problems.push_back(
            "MXH_MAP10_SMOKE_LOGIN_PORT, MXH_MAP10_SMOKE_AGENT_PORT, and "
            "MXH_MAP10_SMOKE_MAP_PORT must be distinct");
    }

    if (!problems.empty()) {
        rejected.skip_reason =
            "GameInAckDoesNotFalseFireInRelease writes the game database and "
            "is skipped unless the server host and ports are set explicitly. "
            "Refusing default ports 16001/17001/18001. Set "
            "MXH_MAP10_SMOKE_HOST, MXH_MAP10_SMOKE_LOGIN_PORT, "
            "MXH_MAP10_SMOKE_AGENT_PORT, and MXH_MAP10_SMOKE_MAP_PORT. ";
        for (std::size_t i = 0; i < problems.size(); ++i) {
            if (i != 0) rejected.skip_reason += "; ";
            rejected.skip_reason += problems[i];
        }
        return rejected;
    }

    const bool allow_default_stage = raw(kMap10SmokeAllowDefaultStageEnv) == "1";
    if (!allow_default_stage) {
        const struct {
            const char* name;
            int port;
        } configured[] = {
            {kMap10SmokeLoginPortEnv, login_port},
            {kMap10SmokeAgentPortEnv, agent_port},
            {kMap10SmokeMapPortEnv, map_port},
        };
        std::vector<std::string> offending;
        for (const auto& entry : configured) {
            if (is_default_stage_port(entry.port)) {
                offending.push_back(std::string(entry.name) + "=" +
                                    std::to_string(entry.port));
            }
        }
        if (!offending.empty()) {
            rejected.skip_reason =
                "GameInAckDoesNotFalseFireInRelease writes the game database "
                "and is skipped because a configured port is a default stage "
                "port (login 16001, agent 17001, map 18001-18003). Set "
                "MXH_MAP10_SMOKE_ALLOW_DEFAULT_STAGE=1 to proceed. Offending: ";
            for (std::size_t i = 0; i < offending.size(); ++i) {
                if (i != 0) rejected.skip_reason += "; ";
                rejected.skip_reason += offending[i];
            }
            return rejected;
        }
    }

    Map10SmokeEndpoint ready;
    ready.ready = true;
    ready.host = host;
    ready.login_port = login_port;
    ready.agent_port = agent_port;
    ready.map_port = map_port;
    return ready;
}

}  // namespace mxh::test
