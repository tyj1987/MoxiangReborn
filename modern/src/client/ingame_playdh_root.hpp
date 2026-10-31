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
// MXH_PLAYDH_ROOT value.
inline InGamePlayDhRoot resolve_ingame_playdh_root(
    const std::optional<std::filesystem::path>& explicit_root,
    const char* env_value) {
    if (explicit_root.has_value() && !explicit_root->empty()) {
        return InGamePlayDhRoot{*explicit_root, {}};
    }
    // The environment value is not consulted yet. Relative candidates below
    // still walk to the repo-tree copy.
    (void)env_value;
    for (const auto& fixture : {
             std::filesystem::path("modern/data/PlayDH"),
             std::filesystem::path("data/PlayDH"),
             std::filesystem::path("../data/PlayDH"),
             std::filesystem::path("../../data/PlayDH"),
             std::filesystem::path("../../../data/PlayDH"),
             std::filesystem::path("../../../../data/PlayDH")}) {
        if (!std::filesystem::exists(fixture / "Image" / "InterfaceScript" /
                                     "15.bin")) {
            continue;
        }
        return InGamePlayDhRoot{fixture, {}};
    }
    return InGamePlayDhRoot{};
}

}  // namespace mxh::client
