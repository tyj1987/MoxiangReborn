#pragma once

// Shared PlayDH root resolution for command-line tools.
// An explicit --resource-root wins. Otherwise MXH_PLAYDH_ROOT is the only
// accepted input. There is no parent walk and no modern/data/PlayDH default.

#include <filesystem>
#include <string>

namespace mxh::tools {

inline constexpr char kCliPlayDhRootMissing[] =
    "MXH_PLAYDH_ROOT is not set and no --resource-root was given";

struct CliPlayDhRoot {
    std::filesystem::path path;
    std::string error;
};

inline CliPlayDhRoot resolve_cli_playdh_root(const std::string& explicit_root,
                                             const char* env_value) {
    std::string chosen = explicit_root;
    if (chosen.empty() && env_value != nullptr && env_value[0] != '\0') {
        chosen = env_value;
    }
    if (chosen.empty()) {
        return CliPlayDhRoot{{}, kCliPlayDhRootMissing};
    }
    return CliPlayDhRoot{std::filesystem::path(chosen), {}};
}

}  // namespace mxh::tools
