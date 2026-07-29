// drop_item.hpp - drop table loaded from MonsterDropItemList.bin.
//
// 1:1 port of legacy [Server]Map/ItemDrop.h. The legacy drop table
// assigns, per (MonsterKind, DropItemId), a probability list of item
// ids each with a chance. Modern port keeps it as a flat array of
// entries so we can look up by (kind, drop_id).

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

// Maximum drops per monster.
inline constexpr std::uint8_t MAX_DROP_PER_MONSTER = 10;

// One drop entry (legacy CItemDrop::stDrop).
struct DropItemEntry final {
    std::uint32_t item_id    = 0;   // legacy wItemIdx
    std::uint16_t ratio      = 0;   // 0..10000 (basis points)
    std::uint16_t count_min  = 1;
    std::uint16_t count_max  = 1;
};

// Drop table (legacy CItemDrop).
struct DropTable final {
    std::uint32_t drop_id      = 0;
    std::uint32_t monster_kind = 0;
    std::vector<DropItemEntry> entries;
};

// In-memory drop-table registry.
class DropTableRegistry final {
public:
    // Add a DropTable to the registry. Tests construct one directly.
    void add(const DropTable& t) noexcept;

    // Find drop_table by (monster_kind, drop_id). Returns nullptr if missing.
    const DropTable* find(std::uint32_t monster_kind, std::uint32_t drop_id) const noexcept;

    // Roll one drop given a drop_id and monster_kind. Returns 0 if no drop.
    // ng is a 0..RAND_MAX source; we modulo at our own basis.
    std::uint32_t roll(std::uint32_t monster_kind, std::uint32_t drop_id,
                        std::uint32_t rng_value) const noexcept;

    std::size_t size() const noexcept { return tables_.size(); }

private:
    std::vector<DropTable> tables_;
};

}  // namespace mxh::server