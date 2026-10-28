#pragma once

#include "mxh/server/player_state.hpp"
#include "mxh/game/experience_curve.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>

namespace mxh::server {

struct OriginalExperienceAward {
    PlayerProgress progress;
    bool leveled = false;
    bool refill_vitals = false;
    bool points_changed = false;
};

// Keep malformed live-event data from turning a normal kill into an
// unbounded or undefined integer award. The event multiplier is a separate
// operational setting; this only validates its conversion to the existing
// unsigned experience amount.
inline std::optional<std::uint32_t> bounded_original_experience_amount(
    std::uint32_t base, double multiplier) noexcept {
    const double raw = static_cast<double>(base) * multiplier;
    if (!std::isfinite(raw) || raw < 0.0) return std::nullopt;
    return static_cast<std::uint32_t>(std::min<double>(
        raw, static_cast<double>(std::numeric_limits<std::uint32_t>::max())));
}

// Source: reference/legacy-source/4dddd9a6/[Server]Map/Player.cpp
// SetPlayerExpPoint (1665-1738), SetLevelUpStatPoint (1743-1775), and
// SetNormalStatPoint (1777-1815). One grant can raise at most one level.
// A null result means the source-backed bounded contract cannot represent
// the award; callers must reject before mutating monster/player state.
inline std::optional<OriginalExperienceAward> calculate_original_experience_award(
    const PlayerProgress& before, std::uint32_t amount,
    std::uint64_t current_threshold, std::uint64_t level_plus_one_threshold) noexcept {
    if (before.level < 1 || before.level >= mxh::game::MAX_CHARACTER_LEVEL_NUM ||
        before.max_level < before.level ||
        before.max_level >= mxh::game::MAX_CHARACTER_LEVEL_NUM ||
        before.level_up_points > 32767 || (before.exp_flag & ~0x1fu) != 0 ||
        (before.hp_mp_flag & ~0x0fu) != 0 || current_threshold == 0 ||
        current_threshold > std::numeric_limits<std::uint32_t>::max() ||
        amount > std::numeric_limits<std::uint32_t>::max() - before.level_exp) return std::nullopt;

    OriginalExperienceAward out{before};
    if (amount == 0) return out;
    auto& p = out.progress;
    const std::uint32_t total = before.level_exp + amount;
    p.total_exp = before.total_exp > std::numeric_limits<std::uint32_t>::max() - amount
        ? std::numeric_limits<std::uint32_t>::max() : before.total_exp + amount;
    const auto segment_count = [](std::uint32_t exp, std::uint64_t threshold) -> int {
        // The original uses float32 and truncation, not integer percentages.
        const auto segments = (static_cast<float>(exp) /
            static_cast<float>(threshold) * 100.0f) / 20.0f;
        return segments >= 5.0f ? 5 : static_cast<int>(segments);
    };
    if (total >= current_threshold) {
        if (before.level >= mxh::game::MAX_CHARACTER_LEVEL_NUM - 1 ||
            level_plus_one_threshold == 0 ||
            level_plus_one_threshold > std::numeric_limits<std::uint32_t>::max()) return std::nullopt;
        p.level_exp = total - static_cast<std::uint32_t>(current_threshold);
        ++p.level;
        out.leveled = true;
        out.refill_vitals = true;  // SetLevel fills HP, shield, and MP.
        if (p.level > before.max_level) {
            p.max_level = p.level;
            const std::uint32_t base = p.level > 70 ? 4u : 2u;
            const auto unclaimed = 4u - static_cast<std::uint32_t>(
                std::popcount(static_cast<unsigned>(before.exp_flag & 0x0fu)));
            const auto points = static_cast<std::uint32_t>(p.level_up_points) + base + unclaimed;
            if (points > 32767u) return std::nullopt;
            p.level_up_points = static_cast<std::uint16_t>(points);
            out.points_changed = points != before.level_up_points;
            p.exp_flag = 0x10u;
            p.hp_mp_flag = 0u;
            const auto count = segment_count(p.level_exp, level_plus_one_threshold);
            for (int i = 0; i < std::min(count, 4); ++i) {
                if ((p.exp_flag & (1u << i)) == 0u) {
                    if (p.level_up_points == 32767u) return std::nullopt;
                    ++p.level_up_points;
                    p.exp_flag |= static_cast<std::uint8_t>(1u << i);
                    out.points_changed = true;
                }
            }
        }
    } else {
        p.level_exp = total;
        if (p.level == p.max_level) {
            const auto count = segment_count(p.level_exp, current_threshold);
            for (int i = 0; i < std::min(count, 4); ++i) {
                if ((p.exp_flag & (1u << i)) == 0u) {
                    if (p.level_up_points == 32767u) return std::nullopt;
                    ++p.level_up_points;
                    p.exp_flag |= static_cast<std::uint8_t>(1u << i);
                    out.points_changed = true;
                }
            }
            // Preserve the source's SetOnBit(i) loop (rather than silently
            // correcting it to j). The persisted flag avoids relog healing.
            for (int i = 0; i < 4; ++i) {
                const auto bit = static_cast<std::uint8_t>(1u << i);
                if (count == i + 1 && (p.hp_mp_flag & bit) == 0u) {
                    out.refill_vitals = true;
                    p.hp_mp_flag = bit;
                }
                if (count == i && (p.hp_mp_flag & bit) != 0u) {
                    p.hp_mp_flag = i == 0 ? 0u : bit;
                }
            }
        }
    }
    return out;
}

}  // namespace mxh::server
