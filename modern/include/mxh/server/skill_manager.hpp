#pragma once

// skill_manager.hpp - 1:1 port of legacy [Server]Map/SkillManager.h CSkillManager.

#include "mxh/game/skill_types.hpp"
#include <cstdint>
#include <vector>
#include <unordered_map>

namespace mxh::server {

// Maximum mugong slots per character.
inline constexpr std::uint8_t MXH_MAX_MUGONG_SLOT = 100;

// Per-character slot entry for a learned mugong (legacy MUGONGBASE).
struct MugongSlot final {
    std::uint32_t mugong_idx   = 0;
    std::uint32_t exp          = 0;
    std::uint16_t level        = 0;
    std::uint16_t sp           = 0;
    std::uint32_t db_idx       = 0;
    std::uint8_t  kind         = 0;
    std::uint8_t  reserved0    = 0;
    std::uint16_t reserved1    = 0;
};

// Per-character skill tree (slot 0..99).
class MugongManager final {
public:
    explicit MugongManager(std::uint32_t owner_id = 0) noexcept : owner_id_(owner_id) {}

    bool set_slot(const MugongSlot& slot) noexcept;
    bool remove(std::uint32_t mugong_idx) noexcept;
    std::size_t size() const noexcept { return slots_.size(); }
    MugongSlot* find(std::uint32_t mugong_idx) noexcept;
    const MugongSlot* find(std::uint32_t mugong_idx) const noexcept;
    const std::vector<MugongSlot>& slots() const noexcept { return slots_; }
    void clear() noexcept { slots_.clear(); }
    std::uint32_t total_sp() const noexcept;
    std::uint32_t owner_id() const noexcept { return owner_id_; }
    void set_owner_id(std::uint32_t v) noexcept { owner_id_ = v; }

private:
    std::uint32_t owner_id_;
    std::vector<MugongSlot> slots_;
    std::unordered_map<std::uint32_t, std::size_t> idx_;
};

// Server-side SkillManager: maps skill_idx to the static SkillInfo.
class SkillManager final {
public:
    void register_skill(const mxh::game::SkillInfo& s) noexcept;
    const mxh::game::SkillInfo* find(std::uint32_t skill_idx) const noexcept;
    std::size_t size() const noexcept { return skills_.size(); }
    const std::vector<mxh::game::SkillInfo>& skills() const noexcept { return skills_; }
    std::uint16_t phy_attack_lv1(std::uint32_t skill_idx) const noexcept;
    std::uint16_t att_attack_lv1(std::uint32_t skill_idx) const noexcept;
    std::uint16_t att_rate_lv1(std::uint32_t skill_idx) const noexcept;
    std::uint16_t naeryuk_lv1(std::uint32_t skill_idx) const noexcept;

private:
    std::vector<mxh::game::SkillInfo> skills_;
    std::unordered_map<std::uint32_t, std::size_t> idx_;
};

}  // namespace mxh::server
