#pragma once

// Test-only PlayDH location. Production code must not include this header.
//
// MXH_PLAYDH_ROOT, when set, is the PlayDH root and replaces every upward
// search. Unset leaves the caller's existing walk unchanged.
// Set but empty, missing, or not a directory fails with a message that names
// MXH_PLAYDH_ROOT and the path (callers are test helpers; the exception
// fails the test).

#include <cstdlib>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>

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

}  // namespace mxh::test
