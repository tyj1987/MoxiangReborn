#pragma once

// Test-only PlayDH location. Production code must not include this header.
// MoxianClientE2E is a test harness and may include it.
//
// MXH_PLAYDH_ROOT, when set, is the PlayDH root and replaces every upward
// search. Unset leaves the caller's existing walk unchanged.
// Set but empty, missing, or not a directory fails with a message that names
// MXH_PLAYDH_ROOT and the path (callers are test helpers; the exception
// fails the test). The value is never treated as optional: a bad setting
// does not fall through to the upward search.
//
// The E2E resolver also requires the upward-search markers under that
// directory: Resource/Server and Image/InterfaceScript. A missing marker
// throws, names MXH_PLAYDH_ROOT, the path, and each missing marker, and
// does not fall back. Other callers of playdh_dir_override() stay on the
// directory check only.

#include <cstdlib>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace mxh::test {

inline constexpr const char* kPlayDhRootEnv = "MXH_PLAYDH_ROOT";

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

// MoxianClientE2E marker search. `ec` is the same error_code the historical
// walk passed to weakly_canonical after current_path/absolute.
inline std::filesystem::path find_marked_e2e_playdh_root(
    const std::vector<std::filesystem::path>& bases,
    std::error_code& ec) {
    for (const auto& candidate_base : bases) {
        for (const auto& root : {
                 candidate_base / "modern" / "data" / "PlayDH",
                 candidate_base / "data" / "PlayDH"}) {
            if (e2e_playdh_has_markers(root)) {
                return std::filesystem::weakly_canonical(root, ec);
            }
        }
    }
    return {};
}

inline std::filesystem::path canonical_playdh_override(const std::filesystem::path& path) {
    std::error_code ec;
    const auto canonical = std::filesystem::weakly_canonical(path, ec);
    if (!ec && !canonical.empty()) return canonical;
    return path;
}

// `env_value` nullptr keeps the marker search over `bases`. Any other value
// is the only candidate. Empty, missing, not a directory, or missing
// Resource/Server or Image/InterfaceScript throws and does not consult `bases`.
inline std::filesystem::path resolve_e2e_playdh_root(
    const char* env_value,
    const std::vector<std::filesystem::path>& bases) {
    if (const auto override_dir = playdh_dir_from_env_value(env_value)) {
        require_e2e_playdh_markers(*override_dir);
        return canonical_playdh_override(*override_dir);
    }
    std::error_code ec;
    return find_marked_e2e_playdh_root(bases, ec);
}

// Upward search used when MXH_PLAYDH_ROOT is unset. cwd first, then eight
// parents of the map executable. Either base may hold modern/data/PlayDH
// or data/PlayDH, and both marker directories must exist.
inline std::filesystem::path search_e2e_playdh_from_map_exe(
    const std::filesystem::path& map_exe) {
    std::error_code ec;
    std::vector<std::filesystem::path> bases;
    bases.push_back(std::filesystem::current_path(ec));
    auto base = std::filesystem::absolute(map_exe, ec).parent_path();
    for (int depth = 0; !base.empty() && depth < 8; ++depth) {
        bases.push_back(base);
        const auto parent = base.parent_path();
        if (parent == base) break;
        base = parent;
    }
    return find_marked_e2e_playdh_root(bases, ec);
}

// Live MXH_PLAYDH_ROOT, read only through playdh_dir_override(). A set value
// that is not an existing directory, or that lacks Resource/Server or
// Image/InterfaceScript, throws before the upward search.
inline std::filesystem::path find_e2e_playdh_root_from_map_exe(
    const std::filesystem::path& map_exe) {
    if (const auto override_dir = playdh_dir_override()) {
        require_e2e_playdh_markers(*override_dir);
        return canonical_playdh_override(*override_dir);
    }
    return search_e2e_playdh_from_map_exe(map_exe);
}

}  // namespace mxh::test
