// item_effects.cpp - Per-item effect resolution.
//
// See item_effects.hpp for the design and the deferred-scope list.
// This is intentionally a tiny .cpp because the only logic is
// the resolve_item_effect() linear-scale function. A future
// ItemList.bin parser will replace this with a real table lookup.

#include "mxh/game/item_effects.hpp"
#include "mxh/game/item_manager.hpp"

namespace mxh::game {

ItemEffect resolve_item_effect(std::uint16_t w_icon_idx) noexcept {
    ItemEffect e{};
    const auto kind = classify_item(w_icon_idx);
    switch (kind) {
        case ItemEffectKind::HpPotion: {
            // Range 1..99 → 50..4950 HP, step 50.
            // (w_icon_idx - 1) * 50 + 50 = w_icon_idx * 50.
            e.hp_delta = static_cast<std::int32_t>(w_icon_idx) * 50;
            break;
        }
        case ItemEffectKind::MpPotion: {
            // Range 100..199 → 30..2970 MP, step 30.
            // (w_icon_idx - 100) * 30 + 30 = (w_icon_idx - 99) * 30.
            e.mp_delta = static_cast<std::int32_t>(w_icon_idx - 99) * 30;
            break;
        }
        case ItemEffectKind::BothPotion: {
            // Range 200..299 → both 50% of HP-only and MP-only at
            // the same level index. (w_icon_idx - 200) gives
            // 0..99, so use it to match a same-level split potion.
            const std::int32_t level = static_cast<std::int32_t>(w_icon_idx - 200);
            e.hp_delta = (level + 1) * 50 / 2;  // half of red at same level
            e.mp_delta = (level + 1) * 30 / 2;  // half of blue at same level
            break;
        }
        case ItemEffectKind::BuffPotion: {
            // Range 300..399 → buff id 1..100. HP/MP delta 0 for now
            // (buff items in the legacy game apply a timed status
            // change, not immediate HP/MP — that's deferred to the
            // buff system). We still mark buff != 0 so callers can
            // detect "this is a buff consumable".
            e.buff = static_cast<std::uint16_t>(w_icon_idx - 300 + 1);
            break;
        }
        case ItemEffectKind::None:
            // Leave all fields zero.
            break;
    }
    return e;
}


// resolve_item_effect_with_manager: looks up ItemInfo from a real
// ItemManager (loaded from ItemList.bin) and reads real LifeRecover /
// LifeRecoverRate / NaeRyukRecover / NaeRyukRecoverRate fields.  Falls
// back to the linear-scale placeholder when the manager is empty or the
// w_icon_idx is not in the table.  1:1 with [Server]Map/ItemManager.cpp
// ::UseItemEffect behavior for known item indices.
ItemEffect resolve_item_effect_with_manager(
    std::uint16_t w_icon_idx, const ItemManager& mgr) noexcept {
    ItemInfo info;
    if (mgr.try_get(static_cast<std::uint32_t>(w_icon_idx), info)) {
        ItemEffect e{};
        // Real bin entries: pump amount is sum of fixed recover + max_hp * rate.
        e.hp_delta = static_cast<std::int32_t>(info.LifeRecover)
                   + static_cast<std::int32_t>(info.LifeRecoverRate * 10000.0f);
        e.mp_delta = static_cast<std::int32_t>(info.NaeRyukRecover)
                   + static_cast<std::int32_t>(info.NaeRyukRecoverRate * 10000.0f);
        // buff field is left 0 for now (buff durations come from
        // BuffPotion tables which are a separate resource).
        return e;
    }
    // Manager has no row for this w_icon_idx -- fall back to the
    // original range-based linear scale so legacy tests keep passing.
    return resolve_item_effect(w_icon_idx);
}

}  // namespace mxh::game
