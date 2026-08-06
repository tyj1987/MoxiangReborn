// item_kind_predicates.hpp
//
// 1:1 port of three small data-plane predicates from
// [Server]Map/ItemManager.cpp: IsPetSummonItem / IsTitanCallItem /
// IsTitanEquipItem. Each is a 3-line switch on ItemKind that
// returns true iff the item is in the relevant category. The
// predicates share the same lookup-miss -> false convention as
// the legacy ITEMMGR->GetItemInfo(wItemIdx) == nullptr path.
//
// 1:1 invariants:
//   - IsPetSummonItem: TRUE iff ItemKind == eQUEST_ITEM_PET OR
//     ItemKind == eSHOP_ITEM_PET (16400 or 300).
//   - IsTitanCallItem: TRUE iff ItemKind == eTITAN_ITEM_PAPER
//     (65).
//   - IsTitanEquipItem: TRUE iff (ItemKind & eTITAN_EQUIPITEM)
//     is non-zero (i.e. ItemKind is 128..255 covering the 7
//     eTITAN_EQUIPITEM_* sub-cases 128..134 + the catch-all
//     umbrella 128).
//   - All three return FALSE on ItemInfo lookup miss (nullptr),
//     matching the legacy if(!pItemInfo) return FALSE; guard.

#pragma once

#include <cstdint>

#include <mxh/game/item_list_types.hpp>

namespace mxh::server {

// 1:1 with [CC]Header/CommonGameDefine.h. The legacy values are
// kept here so the modern port does not depend on the legacy header.
inline constexpr std::uint16_t LEGACY_ITEM_KIND_TITAN_PAPER        = 65;
inline constexpr std::uint16_t LEGACY_ITEM_KIND_TITAN_EQUIP_UMBRELLA = 128;  // bit
inline constexpr std::uint16_t LEGACY_ITEM_KIND_QUEST_PET          = 16400;
inline constexpr std::uint16_t LEGACY_ITEM_KIND_SHOP_PET           = 300;

// 1:1 with legacy CItemManager::IsPetSummonItem. TRUE iff the
// item is a pet summon scroll (quest or shop variant).
inline bool is_pet_summon_item(const game::ItemInfo* info_or_null) noexcept {
    if (info_or_null == nullptr) return false;
    const std::uint16_t k = info_or_null->ItemKind;
    return k == LEGACY_ITEM_KIND_QUEST_PET ||
           k == LEGACY_ITEM_KIND_SHOP_PET;
}

// 1:1 with legacy CItemManager::IsTitanCallItem. TRUE iff the
// item is a titan summon paper.
inline bool is_titan_call_item(const game::ItemInfo* info_or_null) noexcept {
    if (info_or_null == nullptr) return false;
    return info_or_null->ItemKind == LEGACY_ITEM_KIND_TITAN_PAPER;
}

// 1:1 with legacy CItemManager::IsTitanEquipItem. TRUE iff the
// item has the titan-equipment bit set (the legacy code uses
// ItemKind & eTITAN_EQUIPITEM, which is a bit-and against 128).
inline bool is_titan_equip_item(const game::ItemInfo* info_or_null) noexcept {
    if (info_or_null == nullptr) return false;
    return (info_or_null->ItemKind & LEGACY_ITEM_KIND_TITAN_EQUIP_UMBRELLA) != 0;
}

}  // namespace mxh::server
