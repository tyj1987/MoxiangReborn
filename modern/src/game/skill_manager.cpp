// mxh/src/game/skill_manager.cpp - Phase D1.3
//
// Implementation of the SkillManager lookup table.  Kept in a .cpp
// (rather than the header) so the SkillManager member definitions
// don't leak into every translation unit that includes the header.
//
// D1.3 adds init_from_bin(): loads the real legacy SkillList.bin
// (packed-text format) into the table.  This is the 1:1 equivalent
// of the legacy CSkillManager::LoadSkillList() in
// `墨香【源码】\[CC]Skill\SkillManager_client.cpp` lines 174-200.

#include "mxh/game/skill_manager.hpp"
#include "mxh/game/skill_list_parser.hpp"
#include "mxh/game/skill_types.hpp"

#include <stdexcept>

namespace mxh::game {

void SkillManager::init() {
    clear();
    for (const auto& s : get_default_skills()) {
        add(s);
    }
}

void SkillManager::init_from_bin(const std::string& path,
                                 std::uint32_t* out_errors) {
    auto result = load_skill_list(path);
    if (!result.error_message.empty()
        && result.skills.empty()) {
        // I/O or header failure: no rows loaded, propagate.
        throw std::runtime_error(
            "SkillManager::init_from_bin: " + result.error_message);
    }
    clear();
    for (auto& s : result.skills) {
        // add() throws on duplicate skill_idx; for the legacy bin
        // (which has unique skill indices by construction) this
        // signals a malformed file, not normal flow.
        add(std::move(s));
    }
    if (out_errors) *out_errors = result.parse_errors;
}

void SkillManager::add(const SkillInfo& s) {
    if (m_idx.find(s.SkillIdx) != m_idx.end()) {
        throw std::invalid_argument(
            "SkillManager::add: duplicate skill_idx " +
            std::to_string(s.SkillIdx));
    }
    m_idx.emplace(s.SkillIdx, m_skills.size());
    m_skills.push_back(s);
}

void SkillManager::add(SkillInfo&& s) {
    if (m_idx.find(s.SkillIdx) != m_idx.end()) {
        throw std::invalid_argument(
            "SkillManager::add: duplicate skill_idx " +
            std::to_string(s.SkillIdx));
    }
    m_idx.emplace(s.SkillIdx, m_skills.size());
    m_skills.push_back(std::move(s));
}

const SkillInfo& SkillManager::get(std::uint32_t skill_idx) const {
    auto it = m_idx.find(skill_idx);
    if (it == m_idx.end()) {
        throw SkillNotFound(skill_idx);
    }
    return m_skills[it->second];
}

bool SkillManager::try_get(std::uint32_t skill_idx,
                            SkillInfo& out) const noexcept {
    auto it = m_idx.find(skill_idx);
    if (it == m_idx.end()) return false;
    out = m_skills[it->second];
    return true;
}

bool SkillManager::exists(std::uint32_t skill_idx) const noexcept {
    return m_idx.find(skill_idx) != m_idx.end();
}

void SkillManager::clear() noexcept {
    m_skills.clear();
    m_idx.clear();
}

}  // namespace mxh::game
