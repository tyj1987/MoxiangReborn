#pragma once

// Resolves the PlayDH root for CInGameState.
// An explicit engine root wins. Otherwise the value of MXH_PLAYDH_ROOT is
// the only accepted input. Callers pass getenv("MXH_PLAYDH_ROOT") (nullptr
// when the variable is unset).

#include <filesystem>
#include <optional>
#include <string>

namespace mxh::client {

inline constexpr char kInGamePlayDhRootMissing[] =
    "MXH_PLAYDH_ROOT is not set and no --playdh-root/-ResourceDir was given";

struct InGamePlayDhRoot {
    std::filesystem::path path;
    std::string error;
};

// `explicit_root` is the host-supplied engine root. `env_value` is the raw
// MXH_PLAYDH_ROOT value. Neither a relative walk nor modern/data/PlayDH is
// consulted.
inline InGamePlayDhRoot resolve_ingame_playdh_root(
    const std::optional<std::filesystem::path>& explicit_root,
    const char* env_value) {
    if (explicit_root.has_value() && !explicit_root->empty()) {
        return InGamePlayDhRoot{*explicit_root, {}};
    }
    if (env_value == nullptr || env_value[0] == '\0') {
        return InGamePlayDhRoot{{}, kInGamePlayDhRootMissing};
    }
    std::error_code ec;
    const std::filesystem::path path(env_value);
    if (!std::filesystem::is_directory(path, ec) || ec) {
        return InGamePlayDhRoot{
            {},
            std::string("MXH_PLAYDH_ROOT is set to \"") + env_value +
                "\" but that directory does not exist"};
    }
    return InGamePlayDhRoot{path, {}};
}

}  // namespace mxh::client
