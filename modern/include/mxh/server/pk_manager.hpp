// pk_manager.hpp + pk_looting_manager.hpp - 1:1 port of legacy [Server]Map/PKManager + PKLootingManager.
//
// Legacy CPKManager owns the per-player PK state (allow/PK/KillerPenalty)
// and CPKLootingManager handles looting rules on death.
//
// Modern port provides:
//   - PKManager: 3-state machine per player (Peace / PK / KillerPenalty).
//   - PKLootingManager: drops table per victim.

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

// 1:1 with legacy ePKMode enum.
enum class PkMode : std::uint8_t {
    Peace         = 0,
    PkAllow       = 1,
    KillerPenalty = 2,
};

inline constexpr std::uint32_t MXH_PK_KILLER_LIMIT  = 3;     // legacy m_nKillerNumMax
inline constexpr std::uint32_t MXH_PK_PENALTY_MS    = 30ULL * 60ULL * 1000ULL;   // 30 min

// One PK state row (legacy m_PlayerPKState).
struct PkState final {
    std::uint32_t player_id = 0;
    PkMode        mode       = PkMode::Peace;
    std::uint32_t kill_count = 0;       // legacy m_nKillNum
    std::uint32_t penalty_until_ms = 0; // 0 = none
    std::uint8_t  reserved0  = 0;
    std::uint16_t reserved1  = 0;
};

// PKManager
class PKManager final {
public:
    bool set_peace(std::uint32_t player_id) noexcept;
    bool allow_pk(std::uint32_t player_id) noexcept;            // toggles PKAllow
    bool on_kill(std::uint32_t player_id, std::uint32_t now_ms) noexcept;
    bool tick(std::uint32_t now_ms) noexcept;                   // release penalty
    PkState* find(std::uint32_t player_id) noexcept;
    const PkState* find(std::uint32_t player_id) const noexcept;
    std::size_t size() const noexcept { return states_.size(); }
private:
    std::vector<PkState> states_;
};

// One looting drop entry (legacy CPKLootingManager::ItemDrop).
struct LootDrop final {
    std::uint32_t item_idx = 0;
    std::uint16_t count    = 1;
    std::uint16_t ratio    = 100;     // 0..10000 basis points
};

// PKLootingManager — per-victim loot table.
class PKLootingManager final {
public:
    bool set_drops(std::uint32_t victim_id, const std::vector<LootDrop>& drops) noexcept;
    // Returns the dropped item_idx, 0 if nothing rolled.
    std::uint32_t roll(std::uint32_t victim_id, std::uint32_t rng_value) const noexcept;
    std::size_t size() const noexcept { return tables_.size(); }
private:
    struct Table { std::uint32_t victim_id = 0; std::vector<LootDrop> drops; };
    std::vector<Table> tables_;
};

}  // namespace mxh::server
