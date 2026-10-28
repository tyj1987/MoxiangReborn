#include "legacy_window_ids.hpp"

namespace mxh::ui {
namespace {

constexpr std::int32_t IG_MSGBOX_STRARTINDEX = 4000;
constexpr std::int32_t IG_DEBUG_START = 10000;

enum LegacyWindowId : std::int32_t {
#define WINDOW_ID(name) name
#define WINDOW_ID_DEFINE(name, value) name = value
#include "legacy_window_ids.inc"
#undef WINDOW_ID_DEFINE
#undef WINDOW_ID
};

struct LegacyWindowIdEntry {
    std::string_view name;
    std::int32_t value;
};

constexpr LegacyWindowIdEntry kLegacyWindowIds[] = {
#define WINDOW_ID(name) LegacyWindowIdEntry{#name, static_cast<std::int32_t>(name)}
#define WINDOW_ID_DEFINE(name, value) \
    LegacyWindowIdEntry{#name, static_cast<std::int32_t>(name)}
#include "legacy_window_ids.inc"
#undef WINDOW_ID_DEFINE
#undef WINDOW_ID
};

}  // namespace

std::optional<std::int32_t> resolve_legacy_window_id(
    std::string_view name) noexcept {
    for (const auto& entry : kLegacyWindowIds) {
        if (entry.name == name) {
            return entry.value;
        }
    }
    return std::nullopt;
}

}  // namespace mxh::ui
