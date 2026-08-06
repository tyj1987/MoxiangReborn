// discard_avatar_item.hpp
//
// 1:1 port of the data-plane half of legacy
// CShopItemManager::DiscardAvatarItem(WORD ItemIdx, WORD ItemPos)
// from [Server]Map/ShopItemManager.cpp. Splits the legacy function
// into:
//   1. Pure data plane (this header): given an AvatarEquip row
//      (Position + Item[24] mask), an ItemIdx, and the current
//      avatar[24] array, return the new avatar[24] array after
//      discarding the slot.
//   2. Orchestrator side effects (legacy): ShopItemUseParamUpdateToDB,
//      SEND_AVATARITEM_INFO broadcast, CalcAvatarOption recompute.
//      These hooks live outside the data plane.
//
// 1:1 invariants:
//   - No-op conditions: missing equip row, avatar[Position] != ItemIdx.
//   - Clear rule: avatar[Position] = 0.
//   - Default fill: for n in [Weared_Hair, Weared_Gum):
//       if equip.Item[n] == 0 then avatar[n] = 1.
//   - The legacy outer loop over [0, eAvatar_Max) only acts on
//       i == Position and the inner fill does not depend on i.
//       Modern port collapses the loop to a single conditional.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <mxh/game/avatar_item_option.hpp>

namespace mxh::server {

// 1:1 with legacy AVATARITEM struct from [CC]Header/CommonStruct.h.
// Legacy fields: Gender (BYTE), Position (BYTE), Item[24] (WORD).
// The data plane needs only Position + Item[24]; Gender is
// irrelevant to the discard logic (the legacy code does not
// consult it).
struct AvatarEquipRow {
    std::uint8_t position = 0;     // legacy Position (eAvatar_*)
    std::array<std::uint16_t, game::EAvatarCount> item{};  // legacy Item[24]
};

// Legacy enum values used as the bounds of the default-fill loop.
// The loop runs [12, 18) = 6 slots: Weared_Hair, Weared_Face,
// Weared_Hat, Weared_Dress, Weared_Shoes, Weared_Gum.
inline constexpr std::size_t kAvatarDefaultFillStart = 12;  // eAvatar_Weared_Hair
inline constexpr std::size_t kAvatarDefaultFillEnd   = 18;  // eAvatar_Weared_Gum

// 1:1 with legacy DiscardAvatarItem (data plane half). Given the
// equip row (or null on lookup miss), the ItemIdx the player is
// trying to discard, and the current avatar[24] array, returns
// the new avatar[24] after the discard.
//
// Returns the (potentially modified) avatar array. The caller can
// detect no-op by comparing input vs output -- the function does
// not mutate its input array.
std::array<std::uint16_t, game::EAvatarCount> discard_avatar_item(
    const AvatarEquipRow* equip_or_null,
    std::uint16_t item_idx,
    const std::array<std::uint16_t, game::EAvatarCount>& current_avatar) noexcept;

}  // namespace mxh::server