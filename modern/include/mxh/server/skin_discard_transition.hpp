// 1:1 data-plane port of CShopItemManager::DiscardSkinItem and
// CShopItemManager::RemoveEquipSkin from legacy
// [Server]Map/ShopItemManager.cpp:2707-2773. Splits the legacy code
// into:
//   1. Pure data plane (this header): given the player's current
//      wSkinItem[eSkinItem_Max] state, a dwSkinKind, and an Env wrapper
//      that walks the skin-table (NOMALCLOTHES_SKIN / COSTUME_SKIN) +
//      reports each row's wEquipItem[3] mask, return the new wSkinItem[]
//      state with any matching slot cleared to 0.
//   2. Orchestrator half (legacy): the function ultimately writes the
//      new wSkinItem[] back into m_pPlayer->GetShopItemStats()->wSkinItem,
//      then CharacterSkinInfoUpdate() + SEND_SKIN_INFO broadcast +
//      DB write follow in the calling code.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <mxh/server/skin_select_transition.hpp>

namespace mxh::server {

// 1:1 with legacy eSkinItem_Hat..eSkinItem_Shoes (5 entries).
inline constexpr std::size_t kSkinEquipSlotMax = 5u;

class SkinDiscardEnv {
public:
    virtual ~SkinDiscardEnv() = default;
    // Returns the total number of skin entries for the given skin kind.
    // The legacy code calls pSkinListTable->GetDataNum().
    virtual std::size_t skin_count(std::uint16_t skin_kind) const noexcept = 0;
    // Returns the i-th skin entry (0 <= i < skin_count(skin_kind)) for
    // the given skin kind, or nullptr on lookup miss. The legacy code
    // walks the CYHHashTable and returns each SKIN_SELECT_ITEM_INFO
    // entry in turn.
    virtual const SkinSelectItemInfo* skin_at(
        std::uint16_t skin_kind, std::size_t index) const noexcept = 0;
};

inline SkinItemSlots remove_equip_skin(
    const SkinDiscardEnv& env,
    const SkinItemSlots* current_skin,
    std::uint16_t dw_skin_kind) {
    SkinItemSlots out{};
    if (current_skin != nullptr) {
        out = *current_skin;
    }

    // Legacy: only the two known skin kinds walk the table; anything
    // else returns early without mutation.
    if (dw_skin_kind != LEGACY_SHOP_ITEM_NOMALCLOTHES_SKIN &&
        dw_skin_kind != LEGACY_SHOP_ITEM_COSTUME_SKIN) {
        return out;
    }

    const std::size_t total = env.skin_count(dw_skin_kind);
    for (std::size_t n = 0; n < total; ++n) {
        const SkinSelectItemInfo* info = env.skin_at(dw_skin_kind, n);
        if (info == nullptr) {
            continue;
        }
        for (std::size_t i = 0; i < kSkinEquipSlotMax; ++i) {
            if (out[i] == 0u) {
                continue;
            }
            for (std::size_t j = 0; j < kSkinItemListMax; ++j) {
                if (out[i] == info->equip_item[j]) {
                    out[i] = 0u;
                }
            }
        }
    }

    return out;
}

inline SkinItemSlots discard_skin_item(
    const SkinDiscardEnv& env,
    std::uint16_t dw_skin_kind,
    const SkinItemSlots* current_skin) {
    // Legacy: DiscardSkinItem(dwItemIndex) looks up the item's
    // ItemKind and forwards to RemoveEquipSkin(ItemKind). The data
    // plane takes dw_skin_kind directly so the orchestrator can call
    // this after a single ItemInfo lookup (kept outside the data
    // plane to match the pure-function pattern of other D4 helpers).
    return remove_equip_skin(env, current_skin, dw_skin_kind);
}

}  // namespace mxh::server
