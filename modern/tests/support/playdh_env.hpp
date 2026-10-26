#pragma once

// Test-only PlayDH location. Production code must not include this header.
//
// MXH_PLAYDH_DIR, when set, is the PlayDH root and replaces every upward
// search. Unset leaves the caller's existing walk unchanged.
// Set but not an existing directory fails with a message that names
// MXH_PLAYDH_DIR (callers are test helpers; the exception fails the test).

#include <cstdlib>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>

namespace mxh::test {

inline constexpr const char* kPlayDhDirEnv = "MXH_PLAYDH_DIR";

// `value` is the raw environment value. nullptr means the variable is unset.
inline std::optional<std::filesystem::path> playdh_dir_from_env_value(const char* value) {
    if (value == nullptr) return std::nullopt;
    const std::string text(value);
    if (text.empty()) {
        throw std::runtime_error(
            std::string(kPlayDhDirEnv) +
            " is set but empty; it must name an existing directory");
    }
    std::error_code ec;
    const std::filesystem::path path(text);
    if (!std::filesystem::is_directory(path, ec)) {
        throw std::runtime_error(
            std::string(kPlayDhDirEnv) + " is set to \"" + text +
            "\" but that directory does not exist");
    }
    return path;
}

// PlayDH root from MXH_PLAYDH_DIR, or nullopt when the variable is unset.
inline std::optional<std::filesystem::path> playdh_dir_override() {
    return playdh_dir_from_env_value(std::getenv(kPlayDhDirEnv));
}

}  // namespace mxh::test
