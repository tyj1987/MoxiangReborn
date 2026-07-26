// mxh/game/skill_manager.hpp - Phase D1.2
//
// Skill lookup table for the modern server.  Holds an ordered vector
// of SkillInfo plus an O(1) skill_idx → vector-index hash for fast
// lookups.  This is the modern equivalent of the legacy CSkillManager
// in `墨香【源码】\[CC]Skill\SkillManager.h` -- same conceptual surface
// (init / get / size / exists) but with std::vector + std::unordered_map
// instead of the legacy cPtrList machinery.
//
// Phase D1.1 ships the placeholder get_default_skills() table; D1.3
// will replace init() with SkillList.bin parsing.  add() is provided
// now so the bin parser can fill the table incrementally without
// changing the public API.

#pragma once

#include "skill_types.hpp"

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace mxh::game {

// Out-of-range skill access.
class SkillNotFound : public std::out_of_range {
public:
    explicit SkillNotFound(std::uint32_t skill_idx)
        : std::out_of_range("SkillManager: skill_idx " +
                            std::to_string(skill_idx) +
                            " not found"),
          m_skill_idx(skill_idx) {}
    std::uint32_t skill_idx() const noexcept { return m_skill_idx; }
private:
    std::uint32_t m_skill_idx;
};

class SkillManager {
public:
    SkillManager() = default;

    SkillManager(const SkillManager&) = delete;
    SkillManager& operator=(const SkillManager&) = delete;

    // Reset to the D1.1 placeholder table.  Replaces whatever was
    // there; safe to call multiple times.
    void init();

    // Phase D1.3 hook: load the real skill table from a packed-text
    // SkillList.bin file (the 1:1 legacy file format).  Clears any
    // existing table on success.  Throws std::runtime_error on I/O
    // failure or header corruption; per-row parse errors are
    // accumulated and returned via out_errors (may be nullptr).
    // 1:1 with the legacy CSkillManager::LoadSkillList() in
    // `墨香【源码】\[CC]Skill\SkillManager_client.cpp` lines 174-200.
    void init_from_bin(const std::string& path,
                       std::uint32_t* out_errors = nullptr);

    // Phase D1.3 hook: append a single SkillInfo to the table and
    // update the hash index.  The skill must not already exist
    // (skill_idx must be unique) -- callers are expected to enforce
    // this for bin parsing.  The 1:1 behaviour is "the second add()
    // of the same skill_idx throws" so duplicate .bin entries are
    // caught at load time.
    void add(const SkillInfo& s);
    void add(SkillInfo&& s);

    // Lookup.  Throws SkillNotFound if the skill isn't in the table.
    const SkillInfo& get(std::uint32_t skill_idx) const;

    // Non-throwing variant.  Returns true and populates `out` on
    // success; returns false and leaves `out` untouched on miss.
    bool try_get(std::uint32_t skill_idx, SkillInfo& out) const noexcept;

    // Returns true iff skill_idx is in the table.
    bool exists(std::uint32_t skill_idx) const noexcept;

    // Number of skills currently loaded.
    std::size_t size() const noexcept { return m_skills.size(); }

    // Clear all skills.  Mainly for tests.
    void clear() noexcept;

    // Read-only access to the underlying vector (for iteration /
    // debugging).  Order is insertion order.
    const std::vector<SkillInfo>& skills() const noexcept { return m_skills; }

private:
    std::vector<SkillInfo> m_skills;
    std::unordered_map<std::uint32_t, std::size_t> m_idx;
};

}  // namespace mxh::game
