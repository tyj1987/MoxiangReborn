// discard_avatar_item.cpp
//
// Pure data plane implementation of CShopItemManager::DiscardAvatarItem.
// See discard_avatar_item.hpp for the 1:1 invariants.

#include <mxh/server/discard_avatar_item.hpp>

namespace mxh::server {

std::array<std::uint16_t, game::EAvatarCount> discard_avatar_item(
    const AvatarEquipRow* equip_or_null,
    std::uint16_t item_idx,
    const std::array<std::uint16_t, game::EAvatarCount>& current_avatar) noexcept {
    if (equip_or_null == nullptr) {
        return current_avatar;  // no-op: lookup miss
    }
    const std::size_t pos = equip_or_null->position;
    if (pos >= game::EAvatarCount) {
        return current_avatar;  // no-op: bad position
    }
    if (current_avatar[pos] != item_idx) {
        return current_avatar;  // no-op: position not held
    }
    // Match: clear the slot, then default-fill the weared slots.
    std::array<std::uint16_t, game::EAvatarCount> out = current_avatar;
    out[pos] = 0;
    for (std::size_t n = kAvatarDefaultFillStart; n < kAvatarDefaultFillEnd; ++n) {
        if (equip_or_null->item[n] == 0) {
            out[n] = 1;
        }
    }
    return out;
}

}  // namespace mxh::server