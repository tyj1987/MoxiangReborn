// mxh/services/ISkillService.hpp
// Phase 13 service interface for skill access.
//
// Tier 3 dialog (MugongDialog, CharacterDialog skill tab,
// SkillOptionChangeDlg, etc.) currently reads the player's
// learned skills via legacy CSkillManager_Client (client side)
// or by querying the server. This service is the modern
// replacement: dialog code takes an `ISkillService*` and
// queries the player's learned skill set through it.
//
// The interface is deliberately minimal — only the read paths
// the Tier 2/3 dialogs need. Write paths (LearnSkill,
// UpgradeSkill, ForgetSkill) are out of scope; they belong to
// the network layer that mediates with the map server.
//
// Usage pattern (from a future MugongDialog::Refresh):
//   void MugongDialog::refresh() {
//     const auto* skills = m_skillService;  // injected
//     if (!skills) return;
//     for (std::uint32_t i = 0; i < skills->learnedSkillCount(); ++i) {
//       std::uint32_t idx = skills->getLearnedSkillAt(i);
//       std::uint8_t  lvl = skills->getSkillLevel(idx);
//       // ... render skill icon, level, exp bar
//     }
//   }

#pragma once

#include <cstdint>
#include <optional>

namespace mxh::services {

class ISkillService {
public:
    virtual ~ISkillService() = default;

    // ----- Learned skill set -----

    // Number of distinct skills the player has learned.
    virtual std::uint32_t learnedSkillCount() const noexcept = 0;

    // Return the skill_idx of the i-th learned skill in
    // implementation-defined order. `i` must be in
    // [0, learnedSkillCount()).
    virtual std::uint32_t getLearnedSkillAt(std::uint32_t i) const noexcept = 0;

    // ----- Single-skill lookups -----

    // True if the player has learned the skill `skillIdx`.
    virtual bool isLearned(std::uint32_t skillIdx) const noexcept = 0;

    // Current level of `skillIdx` (1..12 in the original
    // engine). Returns std::nullopt if the skill is not
    // learned.
    virtual std::optional<std::uint8_t> getSkillLevel(std::uint32_t skillIdx) const noexcept = 0;

    // Quick-position slot (0..9) where this skill is bound,
    // or std::nullopt if it is not on a quickslot bar. This
    // is the read counterpart of QuickDialog's drag-drop
    // assignment.
    virtual std::optional<std::uint8_t> getQuickSlotBinding(std::uint32_t skillIdx) const noexcept = 0;
};

}  // namespace mxh::services
