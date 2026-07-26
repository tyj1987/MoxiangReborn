// mxh/src/game/skill_manager.cpp - Phase D1.2
//
// Implementation of the SkillManager lookup table.  Kept in a .cpp
// (rather than the header) so the SkillManager member definitions
// don't leak into every translation unit that includes the header.

#include "mxh/game/skill_manager.hpp"
#include "mxh/game/skill_types.hpp"

namespace mxh::game {

void SkillManager::init() {
    clear();
    for (const auto& s : get_default_skills()) {
        add(s);
    }
}

void SkillManager::add(const SkillInfo& s) {
    if (m_idx.find(s.skill_idx) != m_idx.end()) {
        throw std::invalid_argument(
            "SkillManager::add: duplicate skill_idx " +
            std::to_string(s.skill_idx));
    }
    m_idx.emplace(s.skill_idx, m_skills.size());
    m_skills.push_back(s);
}

void SkillManager::add(SkillInfo&& s) {
    if (m_idx.find(s.skill_idx) != m_idx.end()) {
        throw std::invalid_argument(
            "SkillManager::add: duplicate skill_idx " +
            std::to_string(s.skill_idx));
    }
    m_idx.emplace(s.skill_idx, m_skills.size());
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
