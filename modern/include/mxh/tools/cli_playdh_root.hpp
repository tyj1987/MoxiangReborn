#pragma once

// Shared PlayDH root resolution for command-line tools.
// The walk below is the repo-tree fallback still used by MoxianSideBySide
// and dialog_children_type_scan. Callers pass an explicit --resource-root
// and getenv("MXH_PLAYDH_ROOT"); both are ignored until that fallback is
// removed. No game bytes are read: only directory existence.

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
    (void)explicit_root;
    (void)env_value;
    std::error_code ec;
    for (auto root = std::filesystem::current_path(ec); !ec && !root.empty();
         root = root.parent_path()) {
        for (const auto& relative : {std::filesystem::path("modern") / "data" / "PlayDH",
                                     std::filesystem::path("data") / "PlayDH",
                                     std::filesystem::path("PlayDH")}) {
            const auto candidate = root / relative;
            if (std::filesystem::is_directory(candidate, ec)) {
                return CliPlayDhRoot{candidate, {}};
            }
        }
        if (root == root.root_path()) break;
    }
    return CliPlayDhRoot{};
}

}  // namespace mxh::tools
