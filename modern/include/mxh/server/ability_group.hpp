// ability_group.hpp - 1:1 port of legacy AbilityGroup (CSkillInfo combo containers).
//
// The legacy AbilityGroup owns the Ability/Buff/Effect stacks that ride
// on top of a character. Modern port keeps simple FIFO stacks of
// EffectEntry per kind so the per-tick ability calculation can
// reproduce legacy behavior deterministically.

#pragma once

#include <cstdint>
#include <vector>
#include <array>

namespace mxh::server {

// Number of Effect kinds tracked per player (legacy AbilityGroup fields).
inline constexpr std::uint8_t MXH_ABILITY_KIND_COUNT = 64;

// One Effect/Buff entry (legacy Ability / Buff / Effect struct).
struct EffectEntry final {
    std::uint32_t effect_idx    = 0;   // legacy wAbilityIdx
    std::uint32_t caster_id     = 0;
    std::uint32_t start_ms      = 0;
    std::uint32_t duration_ms   = 0;
    std::uint16_t value         = 0;   // legacy EffectParamValue
    std::uint8_t  kind          = 0;   // EffectKind
    std::uint8_t  reserved0     = 0;
    std::uint16_t reserved1     = 0;
};

// Per-player Effect stack.
class AbilityGroup final {
public:
    bool push(const EffectEntry& e) noexcept;
    bool remove(std::uint32_t effect_idx) noexcept;
    void tick(std::uint32_t now_ms) noexcept;     // expire effects past duration
    std::size_t size() const noexcept { return effects_.size(); }
    const std::vector<EffectEntry>& effects() const noexcept { return effects_; }
    std::uint32_t total_value(std::uint8_t kind) const noexcept;

private:
    std::vector<EffectEntry> effects_;
};

// Skill cooldown queue: holds last-activation timestamps so the legacy
// "is the skill on delay" gate is reproducible per skill.
class DelayGroup final {
public:
    // Mark a skill as activated at now_ms.
    void activate(std::uint32_t skill_idx, std::uint32_t now_ms) noexcept;
    // Clear delay on a skill (e.g. on logout).
    void clear(std::uint32_t skill_idx) noexcept;
    // Returns remaining cooldown ms; 0 if ready.
    std::uint32_t remaining_ms(std::uint32_t skill_idx, std::uint32_t now_ms,
                                std::uint32_t delay_ms) const noexcept;

private:
    std::array<std::uint32_t, 100> last_used_ms_{};  // index = skill_idx % 100
    std::array<std::uint32_t, 100> last_idx_{};      // stores skill_idx for chain detect
};

}  // namespace mxh::server
