// mxh/src/services/SkillServiceImpl.hpp
// Phase 13.2: Real ISkillService implementation backed by a
// learned-skill table (per-player vector of {skill_idx, level,
// optional quick-slot binding}).
//
// Architecture (Phase 13 service model):
//   See InventoryServiceImpl.hpp for the service-binding pattern.
//   The dialog holds a reference to the service; the MapHandler
//   owns the learned-skill vector via PlayerInfo.
//
// The legacy engine persists learned skills in
// `character_mugong.bin` (per-character) and binds quickslots in
// `character_quickslot.bin`. The modern port doesn't have those
// binary loaders yet, so the constructor takes a reference to
// the in-memory vector directly. The MapHandler will populate the
// vector on player login (Phase 13.3) and the dialog will see
// the current state through the service.
//
// Threading: read-only from the dialog; mutations happen under
// PlayerInfo's player_mu_ on the server side.

#pragma once

#include "mxh/services/ISkillService.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace mxh::services {

// Per-player learned-skill entry. Mirrors the legacy
// CHARACTER_MUGONGTOTALINFO (MugongIdx, Slot, QuickPosition, etc.).
struct LearnedSkill {
    std::uint32_t                idx        = 0;  // SkillInfo::skill_idx
    std::uint8_t                 level      = 1;  // 1..12
    std::optional<std::uint8_t>  quick_slot;       // 0..9 if bound, nullopt otherwise
};

class SkillServiceImpl final : public ISkillService {
public:
    // Bind the service to a specific player's learned-skill
    // table. The reference must remain valid for the lifetime
    // of the service.
    explicit SkillServiceImpl(const std::vector<LearnedSkill>& learned) noexcept
        : m_learned(learned) {}

    std::uint32_t learnedSkillCount() const noexcept override {
        return static_cast<std::uint32_t>(m_learned.size());
    }

    std::uint32_t getLearnedSkillAt(std::uint32_t i) const noexcept override {
        return i < m_learned.size() ? m_learned[i].idx : 0;
    }

    bool isLearned(std::uint32_t skillIdx) const noexcept override {
        for (const auto& e : m_learned) if (e.idx == skillIdx) return true;
        return false;
    }

    std::optional<std::uint8_t> getSkillLevel(std::uint32_t skillIdx) const noexcept override {
        for (const auto& e : m_learned) if (e.idx == skillIdx) return e.level;
        return std::nullopt;
    }

    std::optional<std::uint8_t> getQuickSlotBinding(std::uint32_t skillIdx) const noexcept override {
        for (const auto& e : m_learned) if (e.idx == skillIdx) return e.quick_slot;
        return std::nullopt;
    }

private:
    const std::vector<LearnedSkill>& m_learned;
};

}  // namespace mxh::services
