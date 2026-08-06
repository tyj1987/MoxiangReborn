// is_dup_item.cpp
//
// Pure data plane implementation of CItemManager::IsDupItem. See
// is_dup_item.hpp for the 1:1 invariants and the legacy-line
// citations.

#include <mxh/server/is_dup_item.hpp>

namespace mxh::server {

namespace {

// 1:1 with legacy CItemManager::IsDupItem(eSHOP_ITEM_INCANTATION)
// non-dup list. The legacy code is a long || chain in a single
// if-statement; modern port keeps the same semantics but factors
// it into a helper for readability and testability.
bool incantation_is_non_dup(std::uint16_t wItemIdx) noexcept {
    switch (wItemIdx) {
    case LEGACY_INCANTATION_TOWN_MOVE_15:
    case LEGACY_INCANTATION_MEMORY_MOVE_15:
    case LEGACY_INCANTATION_TOWN_MOVE_7:
    case LEGACY_INCANTATION_TOWN_MOVE_7_NO_TRADE:
    case LEGACY_INCANTATION_MEMORY_MOVE_7:
    case LEGACY_INCANTATION_MEMORY_MOVE_7_NO_TRADE:
    case LEGACY_INCANTATION_55357:
    case LEGACY_INCANTATION_55362:
    case LEGACY_INCANTATION_MEMORY_MOVE_EXTEND:
    case LEGACY_INCANTATION_MEMORY_MOVE_EXTEND_7:
    case LEGACY_INCANTATION_MEMORY_MOVE_2:
    case LEGACY_INCANTATION_MEMORY_MOVE_EXTEND_30:
    case LEGACY_INCANTATION_SHOW_PYOGUK:
    case LEGACY_INCANTATION_CHANGE_NAME:
    case LEGACY_INCANTATION_CHANGE_NAME_DN_TRADE:
    case LEGACY_INCANTATION_TRACKING:
    case LEGACY_INCANTATION_TRACKING_JIN:
    case LEGACY_INCANTATION_CHANGE_JOB:
    case LEGACY_INCANTATION_SHOW_PYOGUK_7:
    case LEGACY_INCANTATION_SHOW_PYOGUK_7_NO_TRADE:
    case LEGACY_INCANTATION_TRACKING_7:
    case LEGACY_INCANTATION_TRACKING_7_NO_TRADE:
    case LEGACY_INCANTATION_MUGONG_EXTEND:
    case LEGACY_INCANTATION_PYOGUK_EXTEND:
    case LEGACY_INCANTATION_INVEN_EXTEND:
    case LEGACY_INCANTATION_CHARACTER_SLOT:
    case LEGACY_INCANTATION_MUGONG_EXTEND_2:
    case LEGACY_INCANTATION_PYOGUK_EXTEND_2:
    case LEGACY_INCANTATION_INVEN_EXTEND_2:
    case LEGACY_INCANTATION_CHARACTER_SLOT_2:
        return true;
    default:
        return false;
    }
}

}  // namespace

bool is_dup_item(std::uint16_t wItemIdx,
                 const game::ItemInfo* info_or_null) noexcept {
    if (info_or_null == nullptr) {
        return false;
    }

    const std::uint16_t kind = info_or_null->ItemKind;
    switch (kind) {
    case LEGACY_ITEM_KIND_YOUNGYAK_ITEM:
    case LEGACY_ITEM_KIND_YOUNGYAK_ITEM_PET:
    case LEGACY_ITEM_KIND_YOUNGYAK_ITEM_UPGRADE_PET:
    case LEGACY_ITEM_KIND_YOUNGYAK_ITEM_TITAN:
    case LEGACY_ITEM_KIND_EXTRA_JEWEL:
    case LEGACY_ITEM_KIND_EXTRA_MATERIAL:
    case LEGACY_ITEM_KIND_EXTRA_METAL:
    case LEGACY_ITEM_KIND_EXTRA_BOOK:
    case LEGACY_ITEM_KIND_EXTRA_HERB:
    case LEGACY_ITEM_KIND_EXTRA_ETC:
    case LEGACY_ITEM_KIND_EXTRA_USABLE:
    case LEGACY_ITEM_KIND_SHOP_CHARM:
    case LEGACY_ITEM_KIND_SHOP_HERB:
        return true;
    case LEGACY_ITEM_KIND_SHOP_SUNDRIES:
        if (info_or_null->SimMek) return false;
        if (info_or_null->CheRyuk) return false;
        if (wItemIdx == LEGACY_SUNDRIES_SHOUT) return false;
        return true;
    case LEGACY_ITEM_KIND_SHOP_INCANTATION:
        if (incantation_is_non_dup(wItemIdx)) return false;
        if (info_or_null->LimitLevel && info_or_null->SellPrice) {
            return false;
        }
        return true;
    case LEGACY_ITEM_KIND_SHOP_NOMALCLOTHES_SKIN:
    case LEGACY_ITEM_KIND_SHOP_COSTUME_SKIN:
        return false;
    default:
        return false;
    }
}

}  // namespace mxh::server
