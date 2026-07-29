// skill_manager.cpp - 1:1 port of legacy CSkillManager.

#include "mxh/server/skill_manager.hpp"

namespace mxh::server {

bool MugongManager::set_slot(const MugongSlot& slot) noexcept {
    auto it = idx_.find(slot.mugong_idx);
    if (it != idx_.end()) {
        slots_[it->second] = slot;
        return true;
    }
    if (slots_.size() >= MXH_MAX_MUGONG_SLOT) return false;
    slots_.push_back(slot);
    idx_[slot.mugong_idx] = slots_.size() - 1;
    return true;
}

bool MugongManager::remove(std::uint32_t mugong_idx) noexcept {
    auto it = idx_.find(mugong_idx);
    if (it == idx_.end()) return false;
    const std::size_t i = it->second;
    slots_.erase(slots_.begin() + i);
    idx_.erase(it);
    // Reindex.
    for (std::size_t k = 0; k < slots_.size(); ++k) {
        idx_[slots_[k].mugong_idx] = k;
    }
    return true;
}

MugongSlot* MugongManager::find(std::uint32_t mugong_idx) noexcept {
    auto it = idx_.find(mugong_idx);
    if (it == idx_.end()) return nullptr;
    return &slots_[it->second];
}

const MugongSlot* MugongManager::find(std::uint32_t mugong_idx) const noexcept {
    auto it = idx_.find(mugong_idx);
    if (it == idx_.end()) return nullptr;
    return &slots_[it->second];
}

std::uint32_t MugongManager::total_sp() const noexcept {
    std::uint32_t total = 0;
    for (const auto& s : slots_) total += s.sp;
    return total;
}

// -------- SkillManager --------

void SkillManager::register_skill(const mxh::game::SkillInfo& s) noexcept {
    auto it = idx_.find(s.SkillIdx);
    if (it != idx_.end()) {
        skills_[it->second] = s;
        return;
    }
    skills_.push_back(s);
    idx_[s.SkillIdx] = skills_.size() - 1;
}

const mxh::game::SkillInfo* SkillManager::find(std::uint32_t skill_idx) const noexcept {
    auto it = idx_.find(skill_idx);
    if (it == idx_.end()) return nullptr;
    return &skills_[it->second];
}

std::uint16_t SkillManager::phy_attack_lv1(std::uint32_t skill_idx) const noexcept {
    const auto* s = find(skill_idx);
    if (!s) return 0;
    return static_cast<std::uint16_t>(s->UpPhyAttack[0]);
}

std::uint16_t SkillManager::att_attack_lv1(std::uint32_t skill_idx) const noexcept {
    const auto* s = find(skill_idx);
    if (!s) return 0;
    return static_cast<std::uint16_t>(s->FirstAttAttack[0]);
}

std::uint16_t SkillManager::att_rate_lv1(std::uint32_t skill_idx) const noexcept {
    const auto* s = find(skill_idx);
    if (!s) return 0;
    return static_cast<std::uint16_t>(s->AttackSuccessRate[0]);
}

std::uint16_t SkillManager::naeryuk_lv1(std::uint32_t skill_idx) const noexcept {
    const auto* s = find(skill_idx);
    if (!s) return 0;
    return static_cast<std::uint16_t>(s->NeedNaeRyuk[0]);
}

}  // namespace mxh::server

