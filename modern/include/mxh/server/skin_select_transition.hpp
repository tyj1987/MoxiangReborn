// 1:1 data-plane port of CShopItemManager::PutSkinSelectItem from
// legacy [Server]Map/ShopItemManager.cpp:2638-2704. Splits the legacy
// function into:
//   1. Pure data plane (this header): given (dw_skin_index, dw_skin_kind),
//      the player's current wSkinItem[eSkinItem_Max] state, an Env wrapper
//      for the SKIN_SELECT_ITEM_INFO + ItemInfo lookups, and the player
//      level + skin-delay gate, return the new wSkinItem[5] state +
//      the eSkinResult_* status.
//   2. Orchestrator half (legacy): the function ultimately writes the
//      new wSkinItem[] back into m_pPlayer->GetShopItemStats()->wSkinItem,
//      then CharacterSkinInfoUpdate() + SEND_SKIN_INFO broadcast + DB write
//      follow in the calling code.

#pragma once

#include <array>
#include <cstdint>

namespace mxh::server {

// 1:1 with legacy [CC]Header/CommonGameDefine.h SKINITEM_LIST_MAX and
// eSKINITEM_EQUIP_KIND enum. The legacy defines SKINITEM_LIST_MAX as 3
// (number of item indices per skin entry) and the eSkinItem_* enum as
// 5 entries (Hat, Mask, Dress, Shoulder, Shoes) followed by the
// eSkinItem_Max sentinel.
inline constexpr std::size_t kSkinItemListMax = 3u;
inline constexpr std::size_t kSkinItemEquipMax = 5u;

// 1:1 with legacy eSHOP_ITEM_NOMALCLOTHES_SKIN / eSHOP_ITEM_COSTUME_SKIN
// from [CC]Header/CommonGameDefine.h:707-708.
inline constexpr std::uint16_t LEGACY_SHOP_ITEM_NOMALCLOTHES_SKIN = 265u;
inline constexpr std::uint16_t LEGACY_SHOP_ITEM_COSTUME_SKIN     = 266u;

// 1:1 with legacy eSKINITEM_EQUIP_KIND enum.
enum class SkinEquipSlot : std::uint8_t {
    Hat     = 0,
    Mask    = 1,
    Dress   = 2,
    Shoulder = 3,
    Shoes   = 4,
    Max     = 5,
};

// 1:1 with legacy Part3DType -> skin-slot mapping in PutSkinSelectItem.
// The legacy code special-cases (eSkinItem_Hat || 6) -> Hat because the
// Part3DType byte is a 3D-part identifier and 6 was the legacy "headband"
// slot that maps to the same visual hat slot.
inline constexpr std::uint16_t LEGACY_PART3D_HEADBAND = 6u;

// 1:1 with legacy SKIN_RESULT enum (success / fail / delay / level).
enum class SkinSelectResult : std::uint8_t {
    Success   = 0,  // legacy eSkinResult_Success
    Fail      = 1,  // legacy eSkinResult_Fail
    DelayFail = 2,  // legacy eSkinResult_DelayFail
    LevelFail = 3,  // legacy eSkinResult_LevelFail
};

struct SkinSelectItemInfo final {
    std::uint32_t dw_limit_level = 0;
    std::array<std::uint16_t, kSkinItemListMax> equip_item{};
};

struct SkinItemInfoView final {
    std::uint16_t part3d_type = 0;  // legacy Part3DType (item_info.Part3DType)
};

class SkinSelectEnv {
public:
    virtual ~SkinSelectEnv() = default;
    virtual const SkinSelectItemInfo* find_skin(
        std::uint16_t skin_kind,
        std::uint32_t skin_index) const noexcept = 0;
    virtual const SkinItemInfoView* find_item_info(
        std::uint16_t item_idx) const noexcept = 0;
};

using SkinItemSlots = std::array<std::uint16_t, kSkinItemEquipMax>;

inline std::uint8_t map_part3d_to_skin_slot(std::uint16_t part3d_type) noexcept {
    if (part3d_type == static_cast<std::uint16_t>(SkinEquipSlot::Hat) ||
        part3d_type == LEGACY_PART3D_HEADBAND) {
        return static_cast<std::uint8_t>(SkinEquipSlot::Hat);
    }
    if (part3d_type == static_cast<std::uint16_t>(SkinEquipSlot::Mask)) {
        return static_cast<std::uint8_t>(SkinEquipSlot::Mask);
    }
    if (part3d_type == static_cast<std::uint16_t>(SkinEquipSlot::Dress)) {
        return static_cast<std::uint8_t>(SkinEquipSlot::Dress);
    }
    if (part3d_type == static_cast<std::uint16_t>(SkinEquipSlot::Shoes)) {
        return static_cast<std::uint8_t>(SkinEquipSlot::Shoes);
    }
    // Legacy: unmapped Part3DType -> dwEquipIndex stays 0 (which is the
    // Hat slot). The legacy code writes the value regardless of whether
    // it was explicitly mapped, so we mirror that exactly here.
    return static_cast<std::uint8_t>(SkinEquipSlot::Hat);
}

struct SkinSelectTransition final {
    SkinSelectResult result = SkinSelectResult::Fail;
    SkinItemSlots skin_item{};
    bool slot_written = false;  // legacy writes at least one slot
};

// 1:1 with legacy CShopItemManager::PutSkinSelectItem data plane.
// Returns the new wSkinItem[] state + the legacy status code. The
// orchestrator applies the result by writing skin_item back to
// m_pPlayer->GetShopItemStats()->wSkinItem and dispatching the
// CharacterSkinInfoUpdate + SEND_SKIN_INFO broadcast + DB write.
inline SkinSelectTransition put_skin_select_item(
    const SkinSelectEnv& env,
    const SkinItemSlots* current_skin,
    std::uint32_t dw_skin_index,
    std::uint16_t dw_skin_kind,
    std::uint32_t player_level,
    bool skin_delay_active) {
    SkinSelectTransition out;
    if (current_skin == nullptr) {
        out.result = SkinSelectResult::Fail;
        return out;
    }
    out.skin_item = *current_skin;

    if (dw_skin_index < 1u) {
        out.result = SkinSelectResult::Fail;
        return out;
    }

    const SkinSelectItemInfo* skin_info =
        env.find_skin(dw_skin_kind, dw_skin_index);
    if (skin_info == nullptr) {
        out.result = SkinSelectResult::Fail;
        return out;
    }

    if (dw_skin_kind == LEGACY_SHOP_ITEM_NOMALCLOTHES_SKIN) {
        if (player_level < skin_info->dw_limit_level) {
            out.result = SkinSelectResult::LevelFail;
            return out;
        }
    }

    if (skin_delay_active) {
        out.result = SkinSelectResult::DelayFail;
        return out;
    }

    // Apply each of the 3 equipment slots to the wSkinItem[] state.
    for (std::size_t i = 0; i < kSkinItemListMax; ++i) {
        const std::uint16_t equip_item_idx = skin_info->equip_item[i];
        if (equip_item_idx == 0u) {
            continue;
        }
        const SkinItemInfoView* info = env.find_item_info(equip_item_idx);
        if (info == nullptr) {
            continue;
        }
        const std::uint8_t equip_index =
            map_part3d_to_skin_slot(info->part3d_type);
        out.skin_item[equip_index] = equip_item_idx;
        out.slot_written = true;

        // Costume dress -> shoes override (legacy: 코스튬 옷은 신발과
        // 일체형이므로 적용시 신발은 벗겨준다).
        if (equip_index == static_cast<std::uint8_t>(SkinEquipSlot::Dress) &&
            dw_skin_kind == LEGACY_SHOP_ITEM_COSTUME_SKIN) {
            out.skin_item[static_cast<std::uint8_t>(SkinEquipSlot::Shoes)] = 0u;
        }
    }

    out.result = SkinSelectResult::Success;
    return out;
}

}  // namespace mxh::server
