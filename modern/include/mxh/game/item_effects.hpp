// item_effects.hpp - Per-item effect resolution for MapHandler::UseSyn.
//
// Modern C++ replacement for the legacy ItemList.bin table lookup. The
// original game resolves item effects at runtime by reading
// [CC]Skill/ItemList.bin (or GameResourceStruct.h) and applying HP / MP
// recovery + buff durations based on the item's wIconIdx (item type
// index) and player level. The modern server does not yet have a
// ItemList.bin parser, so this header ships a small hardcoded
// effect table covering the most common consumables.
//
// Scope:
//   - Pure function: given wIconIdx, return the {hp_delta, mp_delta,
//     buff} struct (no side effects, no player lookup).
//   - MapHandler::handle_item / UseSyn calls apply_item_effect() with
//     the player state and the struct.
//
// Not in scope (deferred to a future ItemList.bin parser):
//   - Level-scaled recovery amounts (legacy has per-level curves in
//     the bin file; we just use fixed values).
//   - Buff duration tables.
//   - Equipment / stat-mod items (wIconIdx ranges 1000+).
//   - Skill scroll triggers.
//
// Conventions:
//   - wIconIdx 1-99   : HP potions (red)
//   - wIconIdx 100-199: MP potions (blue)
//   - wIconIdx 200-299: HP+MP potions (purple)
//   - wIconIdx 300-399: Buff potions (atk / def / speed up)
//   - wIconIdx >= 400  : Not consumable (use_nack)
//
//   Recovery amounts scale linearly with wIconIdx within each range
//   so that "stronger" item types (higher index) give more recovery,
//   matching the legacy convention where item stat growth roughly
//   tracks wIconIdx.

#pragma once

#include <cstdint>

namespace mxh::game {

// Effect description for one use of a consumable item.
// hp_delta / mp_delta are SIGNED; negative values would mean "drain"
// but in practice all current items are positive (recovery only).
// buff is a future-use field — currently 0 means "no buff". When
// buff resolution is implemented, this will hold a buff id.
struct ItemEffect {
    std::int32_t hp_delta = 0;   // HP change (positive = heal)
    std::int32_t mp_delta = 0;   // MP change (positive = restore)
    std::uint16_t buff = 0;      // 0 = none, else buff table index
};

// Classify a wIconIdx into one of the four consumable ranges.
// Returns the resolution result + an out-parameter for the
// "is this consumable" decision.
//
// On the wire: ItemBase.wIconIdx is std::uint16_t, but only a
// small subset is actually used (the upper 8 bits are reserved).
// We narrow to std::uint16_t for the signature and let callers
// pass any 16-bit value.
enum class ItemEffectKind {
    None,        // not consumable / out of range
    HpPotion,    // 1-99
    MpPotion,    // 100-199
    BothPotion,  // 200-299
    BuffPotion,  // 300-399
};

[[nodiscard]] inline ItemEffectKind classify_item(
    std::uint16_t w_icon_idx) noexcept {
    if (w_icon_idx >= 1 && w_icon_idx < 100)   return ItemEffectKind::HpPotion;
    if (w_icon_idx >= 100 && w_icon_idx < 200) return ItemEffectKind::MpPotion;
    if (w_icon_idx >= 200 && w_icon_idx < 300) return ItemEffectKind::BothPotion;
    if (w_icon_idx >= 300 && w_icon_idx < 400) return ItemEffectKind::BuffPotion;
    return ItemEffectKind::None;
}

// Compute the effect for a consumable item. Returns {0, 0, 0} (no
// effect) for non-consumables — caller is expected to first check
// classify_item() if it wants to send UseNack instead.
//
// Linear scaling within each range: wIconIdx==1 → smallest heal,
// wIconIdx==99 → largest. Same for MP (100/199) and combined
// (200/299). Buff range uses buff id = (wIconIdx - 300 + 1) as a
// placeholder until a real buff table lands.
[[nodiscard]] ItemEffect resolve_item_effect(
    std::uint16_t w_icon_idx) noexcept;

}  // namespace mxh::game
