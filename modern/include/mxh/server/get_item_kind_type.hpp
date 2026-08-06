// get_item_kind_type.hpp
//
// 1:1 port of legacy CItemManager::GetItemKindType(WORD wItemIdx,
// WORD* wKind, WORD* wType) from [Server]Map/ItemManager.cpp. Pure
// data plane: looks up the ItemInfo row and writes (Kind, Type) to
// the out params; on miss, sets both to 0 (matches the legacy
// *wKind = 0; *wType = 0; path).
//
// 1:1 invariants:
//   - On ItemInfo lookup hit: out_kind = info->ItemKind,
//     out_type = info->ItemType.
//   - On lookup miss (nullptr): out_kind = 0, out_type = 0.
//   - The legacy function reads through m_ItemInfoList.GetData();
//     the modern port uses ItemInfo* (caller provides via lookup
//     callback pattern; the simplest form takes a raw pointer,
//     mirroring is_dup_item).

#pragma once

#include <cstdint>

#include <mxh/game/item_list_types.hpp>

namespace mxh::server {

// 1:1 with legacy CItemManager::GetItemKindType. Writes the
// item-kind and item-type discriminators from the ItemInfo row
// into the out params. On lookup miss, both are set to 0.
inline void get_item_kind_type(const game::ItemInfo* info_or_null,
                               std::uint16_t& out_kind,
                               std::uint16_t& out_type) noexcept {
    if (info_or_null == nullptr) {
        out_kind = 0;
        out_type = 0;
        return;
    }
    out_kind = info_or_null->ItemKind;
    out_type = info_or_null->ItemType;
}

}  // namespace mxh::server
