#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>

namespace mxh::game {

enum class BaseAttackLocale { KoreaChina, Japan };

// CommonCalcFunc.cpp, recovered source reported by ROG (not a tracked blob):
// SHA256 338fc7bbb57113c5044a7b6cd41f1b78b7a4a9537eb4b9bbc783235a64e04d11.
// See docs/EQUIPMENT_COMBAT_FORMULA_20261001.md for provenance and limits.
// Both legacy functions have the same arithmetic; callers select GenGol for
// melee, MinChub for ranged. Weapon is already the appropriate summed endpoint.
// This does NOT add the caller's unarmed +5, Japan level*4, or other bonuses.
// Missing/uninitialized attributes must not become a wrapped huge attack:
// reject values outside DWORD's representable domain rather than guessing the
// original compiler's negative floating-point-to-unsigned conversion behavior.
inline std::optional<std::uint32_t> base_attack_power(
    std::uint16_t stat, std::uint16_t weapon,
    BaseAttackLocale locale = BaseAttackLocale::KoreaChina) noexcept {
    const double s = stat;
    const double w = weapon;
    const double value = locale == BaseAttackLocale::Japan
        ? w + s + s / 3.0
        : (w * ((s + 200.0) / 200.0) * ((s + 1000.0) / 500.0) + s)
            * 0.74 + std::min(s - 12.0, 25.0);
    if (value < 0.0 || value > std::numeric_limits<std::uint32_t>::max())
        return std::nullopt;
    return static_cast<std::uint32_t>(value);
}

} // namespace mxh::game
