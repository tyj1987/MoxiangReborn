#pragma once

// Test-only PlayDH location. Production code must not include this header.
// MoxianClientE2E is a test harness and may include it.
//
// MXH_PLAYDH_ROOT, when set, is the only PlayDH root. There is no parent
// walk and no read of modern/data/PlayDH. Unset throws
// kPlayDhRootMissing. Set but empty, missing, or not a directory fails
// with a message that names MXH_PLAYDH_ROOT and the path. A bad setting
// does not search anywhere else.
//
// The E2E resolver also requires Resource/Server and Image/InterfaceScript
// under that directory. A missing marker throws, names MXH_PLAYDH_ROOT,
// the path, and each missing marker. Other callers of
// playdh_dir_override() stay on the directory check only and still receive
// nullopt when the variable is unset.

#include <cstdlib>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace mxh::test {

inline constexpr const char* kPlayDhRootEnv = "MXH_PLAYDH_ROOT";
inline constexpr const char* kPlayDhRootMissing =
    "MXH_PLAYDH_ROOT is not set and no --resource-root/-ResourceDir was given";

// `value` is the raw environment value. nullptr means the variable is unset.
// One read of MXH_PLAYDH_ROOT lives here; helpers must not call getenv for it.
inline std::optional<std::filesystem::path> playdh_dir_from_env_value(const char* value) {
    if (value == nullptr) return std::nullopt;
    const std::string text(value);
    std::error_code ec;
    const std::filesystem::path path(text);
    if (text.empty() || !std::filesystem::is_directory(path, ec)) {
        throw std::runtime_error(
            std::string(kPlayDhRootEnv) + " is set to \"" + text +
            "\" but that directory does not exist");
    }
    return path;
}

// PlayDH root from MXH_PLAYDH_ROOT, or nullopt when the variable is unset.
inline std::optional<std::filesystem::path> playdh_dir_override() {
    return playdh_dir_from_env_value(std::getenv(kPlayDhRootEnv));
}

// Same relative markers the upward search accepts. Labels use '/' so the
// error text is identical on MSVC and libstdc++.
struct E2ePlayDhMarker {
    const char* first;
    const char* second;
    const char* label;
};

inline constexpr E2ePlayDhMarker kE2ePlayDhMarkers[] = {
    {"Resource", "Server", "Resource/Server"},
    {"Image", "InterfaceScript", "Image/InterfaceScript"},
};

inline bool e2e_playdh_marker_exists(const std::filesystem::path& root,
                                     const E2ePlayDhMarker& marker) {
    return std::filesystem::exists(root / marker.first / marker.second);
}

inline bool e2e_playdh_has_markers(const std::filesystem::path& root) {
    for (const auto& marker : kE2ePlayDhMarkers) {
        if (!e2e_playdh_marker_exists(root, marker)) return false;
    }
    return true;
}

// Throws when the env directory is missing a marker the upward search
// requires. Does not search elsewhere.
inline void require_e2e_playdh_markers(const std::filesystem::path& root) {
    std::string missing;
    for (const auto& marker : kE2ePlayDhMarkers) {
        if (e2e_playdh_marker_exists(root, marker)) continue;
        if (!missing.empty()) missing += ", ";
        missing += marker.label;
    }
    if (missing.empty()) return;
    throw std::runtime_error(
        std::string(kPlayDhRootEnv) + " is set to \"" + root.string() +
        "\" but required marker(s) are missing: " + missing);
}

// Historical marker walk. It no longer searches. `bases` is ignored.
inline std::filesystem::path find_marked_e2e_playdh_root(
    const std::vector<std::filesystem::path>& bases,
    std::error_code& ec) {
    (void)bases;
    (void)ec;
    throw std::runtime_error(kPlayDhRootMissing);
}

inline std::filesystem::path canonical_playdh_override(const std::filesystem::path& path) {
    std::error_code ec;
    const auto canonical = std::filesystem::weakly_canonical(path, ec);
    if (!ec && !canonical.empty()) return canonical;
    return path;
}

// `env_value` nullptr throws kPlayDhRootMissing. `bases` is not searched.
// Empty, missing, not a directory, or missing Resource/Server or
// Image/InterfaceScript throws and does not consult `bases`.
inline std::filesystem::path resolve_e2e_playdh_root(
    const char* env_value,
    const std::vector<std::filesystem::path>& bases) {
    (void)bases;
    if (env_value == nullptr) throw std::runtime_error(kPlayDhRootMissing);
    const auto override_dir = playdh_dir_from_env_value(env_value);
    if (!override_dir) throw std::runtime_error(kPlayDhRootMissing);
    require_e2e_playdh_markers(*override_dir);
    return canonical_playdh_override(*override_dir);
}

// Does not walk from `map_exe` or the working directory.
inline std::filesystem::path search_e2e_playdh_from_map_exe(
    const std::filesystem::path& map_exe) {
    (void)map_exe;
    throw std::runtime_error(kPlayDhRootMissing);
}

// Live MXH_PLAYDH_ROOT, read only through playdh_dir_override(). Unset
// throws kPlayDhRootMissing. A set value that is not an existing directory,
// or that lacks Resource/Server or Image/InterfaceScript, throws.
inline std::filesystem::path find_e2e_playdh_root_from_map_exe(
    const std::filesystem::path& map_exe) {
    (void)map_exe;
    if (const auto override_dir = playdh_dir_override()) {
        require_e2e_playdh_markers(*override_dir);
        return canonical_playdh_override(*override_dir);
    }
    throw std::runtime_error(kPlayDhRootMissing);
}

}  // namespace mxh::test
