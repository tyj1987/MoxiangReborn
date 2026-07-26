// skill_delay_manager.hpp - Phase 6.3 AgentServer 1:1 port of legacy
// [Server]Agent/SkillDalayManager.h + SkillDalayManager.cpp (CSkillDelayManager).
//
// SkillDelayManager tracks per-character skill cooldowns for "premier" skills
// (configured via the legacy PremierSkill.bin resource). Modern port exposes
// the pure decision logic so it can be unit-tested without a network stack.
//
// Locked invariants (1:1 with legacy):
//   - AddSkillUse(character, skill, now, force) returns TRUE to allow a
//     skill cast and FALSE to reject it.
//   - If the skill is NOT a premier skill, the cast is ALWAYS allowed
//     (return TRUE) regardless of history.
//   - If the character has no prior use, a new entry is created with
//     dwStartTime = now and the cast is allowed.
//   - If the character has a prior use and force == TRUE, the entry is
//     reset (dwStartTime = now, dwDelay refreshed) and the cast is allowed.
//   - If the character has a prior use and force == FALSE, the cast is
//     allowed iff `now - prior.dwStartTime + 5000 >= prior.dwDelay` (the
//     legacy 5-second latency tolerance). The entry is reset on allow.
//   - Remaining-delay helper exposes
//       max(0, prior.dwDelay - (now - prior.dwStartTime))
//     for clients / servers to render cooldown UI.
//
// Out of scope for this port:
//   - LoadSkillUseInfo from PremierSkill.bin (depends on legacy MHFile).
//     Modern exposes add_premier_skill() so tests / callers can populate
//     the table directly.
//   - SendMsgToAgentServer + SendSkillDelayMsgToClient: depend on g_Network
//     and g_pUserTable (out of scope here).

#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace mxh::server {

// ---- Constants 1:1 with legacy ----

inline constexpr std::uint32_t SKILL_DELAY_LATENCY_TOLERANCE_MS = 5000u;

// ---- POD (1:1 with legacy PRIMERESKILL / SKILLUSE) ----

struct PrimeReskill {
    std::uint32_t dwSkillIndex = 0;
    std::uint32_t dwDelay      = 0;
};

struct SkillUse {
    std::uint32_t dwCharacterID = 0;
    std::uint32_t dwSkillIndex  = 0;
    std::uint32_t dwDelay       = 0;
    std::uint32_t dwStartTime   = 0;
};

// Mirrors legacy CSkillDelayManager state.
struct SkillDelayManager {
    std::unordered_map<std::uint32_t, PrimeReskill> m_PremierSkills;
    std::unordered_map<std::uint32_t, SkillUse>     m_SkillUses;
};

// ---- Lifecycle ----

inline SkillDelayManager make_skill_delay_manager() {
    return SkillDelayManager{};
}

inline void skill_delay_manager_clear(SkillDelayManager& m) {
    m.m_PremierSkills.clear();
    m.m_SkillUses.clear();
}

// ---- Premier skill table ----

inline void add_premier_skill(SkillDelayManager& m, const PrimeReskill& p) {
    m.m_PremierSkills[p.dwSkillIndex] = p;
}

inline void add_premier_skill(SkillDelayManager& m,
                              std::uint32_t dwSkillIndex,
                              std::uint32_t dwDelay) {
    add_premier_skill(m, PrimeReskill{dwSkillIndex, dwDelay});
}

inline bool is_premier_skill(const SkillDelayManager& m,
                             std::uint32_t dwSkillIndex) {
    return m.m_PremierSkills.find(dwSkillIndex) != m.m_PremierSkills.end();
}

// ---- Per-character skill use tracking ----

inline SkillUse* find_skill_use(SkillDelayManager& m,
                                 std::uint32_t dwCharacterID) {
    auto it = m.m_SkillUses.find(dwCharacterID);
    return it == m.m_SkillUses.end() ? nullptr : &it->second;
}

inline const SkillUse* find_skill_use(const SkillDelayManager& m,
                                       std::uint32_t dwCharacterID) {
    auto it = m.m_SkillUses.find(dwCharacterID);
    return it == m.m_SkillUses.end() ? nullptr : &it->second;
}

inline void remove_skill_use(SkillDelayManager& m, std::uint32_t dwCharacterID) {
    m.m_SkillUses.erase(dwCharacterID);
}

// ---- AddSkillUse (decision logic) ----
//
// Returns true if the cast is allowed, false if rejected (still in delay).
// Pure function: caller passes `now_ms` (analog of legacy gCurTime) so tests
// can drive the time deterministically.
//
// `force == true` mirrors the legacy ServerMsg path that bypasses the
// latency check (always allow + reset).
inline bool add_skill_use(SkillDelayManager& m,
                           std::uint32_t dwCharacterID,
                           std::uint32_t dwSkillIndex,
                           std::uint32_t now_ms,
                           bool force = false) {
    auto pit = m.m_PremierSkills.find(dwSkillIndex);
    if (pit == m.m_PremierSkills.end()) {
        // Non-premier skill: always allowed (legacy returns TRUE).
        return true;
    }
    const std::uint32_t dwDelay = pit->second.dwDelay;

    auto uit = m.m_SkillUses.find(dwCharacterID);
    if (uit == m.m_SkillUses.end()) {
        // No prior use: alloc + set time + allow.
        SkillUse s;
        s.dwCharacterID = dwCharacterID;
        s.dwSkillIndex  = dwSkillIndex;
        s.dwDelay       = dwDelay;
        s.dwStartTime   = now_ms;
        m.m_SkillUses[dwCharacterID] = s;
        return true;
    }

    SkillUse& prior = uit->second;
    if (force) {
        prior.dwSkillIndex = dwSkillIndex;
        prior.dwDelay      = dwDelay;
        prior.dwStartTime  = now_ms;
        return true;
    }

    // Legacy: allow iff (now - start + 5000 >= delay)
    if ((now_ms - prior.dwStartTime + SKILL_DELAY_LATENCY_TOLERANCE_MS)
        >= prior.dwDelay) {
        prior.dwSkillIndex = dwSkillIndex;
        prior.dwDelay      = dwDelay;
        prior.dwStartTime  = now_ms;
        return true;
    }
    return false;
}

// Remaining delay (ms) for a character / skill pair. Returns 0 if the
// character has no prior use, or if the raw delay window has elapsed.
// Modern returns the raw remaining value (delay - elapsed), matching the
// legacy SendSkillDelayMsgToClient payload (dwData2 = dwDelay - elapsed);
// the +5000 tolerance only applies to the AddSkillUse decision, not to
// what the client renders.
inline std::uint32_t remaining_skill_delay_ms(const SkillDelayManager& m,
                                               std::uint32_t dwCharacterID,
                                               std::uint32_t now_ms) {
    const SkillUse* prior = find_skill_use(m, dwCharacterID);
    if (prior == nullptr) return 0;
    const std::uint64_t elapsed = now_ms - prior->dwStartTime;
    if (elapsed >= prior->dwDelay) return 0;
    return static_cast<std::uint32_t>(prior->dwDelay - elapsed);
}

inline std::size_t skill_use_count(const SkillDelayManager& m) {
    return m.m_SkillUses.size();
}

inline std::size_t premier_skill_count(const SkillDelayManager& m) {
    return m.m_PremierSkills.size();
}

} // namespace mxh::server


