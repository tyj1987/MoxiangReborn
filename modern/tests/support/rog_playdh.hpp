#pragma once

// Real PlayDH reads for tests that cannot be replaced by a hand-written
// fixture. MXH_PLAYDH_ROOT is the only location. Unset, empty, or not a
// directory fails the caller. There is no walk back to the repo tree.

#include "playdh_env.hpp"

#include <stdexcept>
#include <string>

namespace mxh::test {

inline std::filesystem::path require_rog_playdh_root() {
    if (const auto root = playdh_dir_override()) return *root;
    throw std::runtime_error(std::string(kPlayDhRootEnv) + " is unset");
}

}  // namespace mxh::test
