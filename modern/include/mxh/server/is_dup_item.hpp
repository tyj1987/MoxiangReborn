// is_dup_item.hpp
//
// 1:1 port of legacy CItemManager::IsDupItem(WORD wItemIdx) from
// [Server]Map/ItemManager.cpp. Pure data plane: given an item-idx,
// returns true iff the inventory is allowed to stack duplicates of
// that item.
//
// 1:1 invariants:
//   - Switches on the legacy item-kind discriminator
//     ([CC]Header/CommonGameDefine.h ItemKind field).
//   - The always-dup-able set is the union of: youngyak (potion),
//     extra (jewel/material/metal/book/herb/etc/usable), and a
//     couple of shop items (CHARM / HERB). 13 cases, all return TRUE.
//   - Sundries (eSHOP_ITEM_SUNDRIES): dup-able UNLESS:
//       * SimMek != 0   (ChangeItem family)
//       * CheRyuk != 0  (slot/extend items)
//       * wItemIdx == eSundries_Shout (55631)
//   - Incantation (eSHOP_ITEM_INCANTATION): dup-able UNLESS:
//       * wItemIdx is one of the explicit non-dup list
//         (TownMove15, MemoryMove15, TownMove7, MemoryMove7, etc.)
//       * LimitLevel AND SellPrice both non-zero
//   - Skin (NOMALCLOTHES_SKIN / COSTUME_SKIN): NEVER dup-able
//   - All other kinds: FALSE (1:1 with legacy fall-through).
//
// The data plane needs the icon-idx + the ItemInfo row (for the
// Sundries / Incantation exceptions that look at SimMek/CheRyuk/
// LimitLevel/SellPrice). The ItemManager lookup is left to the
// caller via a pointer (nullptr on lookup miss).

#pragma once

#include <cstdint>

#include <mxh/game/item_list_types.hpp>

namespace mxh::server {

// 1:1 with [CC]Header/CommonGameDefine.h item-kind discriminators
// that drive the IsDupItem switch. The legacy enum values are kept
// here so the modern port does not depend on the legacy header.
inline constexpr std::uint16_t LEGACY_ITEM_KIND_YOUNGYAK_ITEM              = 512;
inline constexpr std::uint16_t LEGACY_ITEM_KIND_YOUNGYAK_ITEM_PET          = 513;
inline constexpr std::uint16_t LEGACY_ITEM_KIND_YOUNGYAK_ITEM_UPGRADE_PET  = 514;
inline constexpr std::uint16_t LEGACY_ITEM_KIND_YOUNGYAK_ITEM_TITAN        = 555;
inline constexpr std::uint16_t LEGACY_ITEM_KIND_EXTRA_JEWEL                = 4097;
inline constexpr std::uint16_t LEGACY_ITEM_KIND_EXTRA_MATERIAL             = 4098;
inline constexpr std::uint16_t LEGACY_ITEM_KIND_EXTRA_METAL                = 4099;
inline constexpr std::uint16_t LEGACY_ITEM_KIND_EXTRA_BOOK                 = 4100;
inline constexpr std::uint16_t LEGACY_ITEM_KIND_EXTRA_HERB                 = 4101;
inline constexpr std::uint16_t LEGACY_ITEM_KIND_EXTRA_ETC                  = 4102;
inline constexpr std::uint16_t LEGACY_ITEM_KIND_EXTRA_USABLE               = 4106;
inline constexpr std::uint16_t LEGACY_ITEM_KIND_SHOP_CHARM                 = 258;
inline constexpr std::uint16_t LEGACY_ITEM_KIND_SHOP_HERB                  = 259;
inline constexpr std::uint16_t LEGACY_ITEM_KIND_SHOP_SUNDRIES              = 263;
inline constexpr std::uint16_t LEGACY_ITEM_KIND_SHOP_INCANTATION           = 260;
inline constexpr std::uint16_t LEGACY_ITEM_KIND_SHOP_NOMALCLOTHES_SKIN     = 265;
inline constexpr std::uint16_t LEGACY_ITEM_KIND_SHOP_COSTUME_SKIN          = 266;

// 1:1 with the Sundries non-dup list (legacy: wItemIdx == eSundries_Shout).
inline constexpr std::uint16_t LEGACY_SUNDRIES_SHOUT                      = 55631;

// 1:1 with the Incantation non-dup list. Legacy has 30 entries:
// TownMove15 / MemoryMove15 / TownMove7 / MemoryMove7 /
// TownMove7_NoTrade / MemoryMove7_NoTrade / 55357 / 55362 /
// MemoryMoveExtend / MemoryMoveExtend7 / MemoryMove2 /
// MemoryMoveExtend30 / ShowPyoguk / ChangeName / ChangeName_Dntrade /
// Tracking / Tracking_Jin / ChangeJob / ShowPyoguk7 /
// ShowPyoguk7_NoTrade / Tracking7 / Tracking7_NoTrade /
// MugongExtend / PyogukExtend / InvenExtend / CharacterSlot /
// MugongExtend2 / PyogukExtend2 / InvenExtend2 / CharacterSlot2.
inline constexpr std::uint16_t LEGACY_INCANTATION_TOWN_MOVE_15            = 55303;
inline constexpr std::uint16_t LEGACY_INCANTATION_MEMORY_MOVE_15          = 55304;
inline constexpr std::uint16_t LEGACY_INCANTATION_TOWN_MOVE_7             = 57508;
inline constexpr std::uint16_t LEGACY_INCANTATION_TOWN_MOVE_7_NO_TRADE    = 57509;
inline constexpr std::uint16_t LEGACY_INCANTATION_MEMORY_MOVE_7           = 57510;
inline constexpr std::uint16_t LEGACY_INCANTATION_MEMORY_MOVE_7_NO_TRADE  = 57511;
inline constexpr std::uint16_t LEGACY_INCANTATION_55357                   = 55357;
inline constexpr std::uint16_t LEGACY_INCANTATION_55362                   = 55362;
inline constexpr std::uint16_t LEGACY_INCANTATION_MEMORY_MOVE_EXTEND      = 55365;
inline constexpr std::uint16_t LEGACY_INCANTATION_MEMORY_MOVE_EXTEND_7    = 55390;
inline constexpr std::uint16_t LEGACY_INCANTATION_MEMORY_MOVE_2           = 55371;
inline constexpr std::uint16_t LEGACY_INCANTATION_MEMORY_MOVE_EXTEND_30   = 58010;
inline constexpr std::uint16_t LEGACY_INCANTATION_SHOW_PYOGUK              = 55351;
inline constexpr std::uint16_t LEGACY_INCANTATION_CHANGE_NAME             = 55352;
inline constexpr std::uint16_t LEGACY_INCANTATION_CHANGE_NAME_DN_TRADE    = 57799;
inline constexpr std::uint16_t LEGACY_INCANTATION_TRACKING                = 55353;
inline constexpr std::uint16_t LEGACY_INCANTATION_TRACKING_JIN            = 55387;
inline constexpr std::uint16_t LEGACY_INCANTATION_CHANGE_JOB              = 55360;
inline constexpr std::uint16_t LEGACY_INCANTATION_SHOW_PYOGUK_7            = 57506;
inline constexpr std::uint16_t LEGACY_INCANTATION_SHOW_PYOGUK_7_NO_TRADE  = 57507;
inline constexpr std::uint16_t LEGACY_INCANTATION_TRACKING_7              = 57504;
inline constexpr std::uint16_t LEGACY_INCANTATION_TRACKING_7_NO_TRADE     = 57505;
inline constexpr std::uint16_t LEGACY_INCANTATION_MUGONG_EXTEND           = 55361;
inline constexpr std::uint16_t LEGACY_INCANTATION_PYOGUK_EXTEND           = 57544;
inline constexpr std::uint16_t LEGACY_INCANTATION_INVEN_EXTEND            = 57542;
inline constexpr std::uint16_t LEGACY_INCANTATION_CHARACTER_SLOT          = 57543;
inline constexpr std::uint16_t LEGACY_INCANTATION_MUGONG_EXTEND_2         = 57957;
inline constexpr std::uint16_t LEGACY_INCANTATION_PYOGUK_EXTEND_2         = 57960;
inline constexpr std::uint16_t LEGACY_INCANTATION_INVEN_EXTEND_2          = 57958;
inline constexpr std::uint16_t LEGACY_INCANTATION_CHARACTER_SLOT_2        = 57959;

// 1:1 with legacy CItemManager::IsDupItem. Returns true iff the
// inventory is allowed to stack duplicates of wItemIdx.
//
// The function inspects the item-kind discriminator + the per-row
// fields (SimMek, CheRyuk, LimitLevel, SellPrice) that drive the
// Sundries / Incantation exception branches. Missing ItemInfo rows
// (lookup miss = nullptr) follow the legacy behavior: false for
// Sundries and Incantation; all other kinds use the item-kind
// switch and never touch ItemInfo.
bool is_dup_item(std::uint16_t wItemIdx,
                 const game::ItemInfo* info_or_null) noexcept;

// 1:1 with legacy CItemManager::IsRareOptionItem. A rare option
// item is one whose rare-DB-idx is non-zero AND the item itself
// is not dup-able. Legacy: dwRareDBIdx != 0 AND !IsDupItem(idx).
inline bool is_rare_option_item(std::uint16_t wItemIdx,
                                std::uint32_t dw_rare_db_idx,
                                const game::ItemInfo* info_or_null) noexcept {
    return (dw_rare_db_idx != 0) && !is_dup_item(wItemIdx, info_or_null);
}

// 1:1 with legacy CItemManager::IsOptionItem. An option item is
// one whose durability is non-zero AND the item itself is not
// dup-able. Legacy: wDurability != 0 AND !IsDupItem(idx).
inline bool is_option_item(std::uint16_t wItemIdx,
                           std::uint32_t w_durability,
                           const game::ItemInfo* info_or_null) noexcept {
    return (w_durability != 0) && !is_dup_item(wItemIdx, info_or_null);
}

}  // namespace mxh::server
