#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace mxh::ui {

// Resolves the exact integer assigned by the recovered legacy WindowIDs.h.
// The conditional sections in that file remain controlled by the same locale
// defines, so CHINA/KOR/HK/JAPAN/TL builds retain their original numbering.
[[nodiscard]] std::optional<std::int32_t> resolve_legacy_window_id(
    std::string_view name) noexcept;

}  // namespace mxh::ui
