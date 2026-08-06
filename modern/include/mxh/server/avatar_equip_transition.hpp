// 1:1 data-plane port of CShopItemManager::PutOnAvatarItem and
// CShopItemManager::TakeOffAvatarItem from the legacy Map server.

#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include <mxh/game/avatar_item_option.hpp>
#include <mxh/server/discard_avatar_item.hpp>

namespace mxh::server {

inline constexpr std::size_t kAvatarCosmeticEnd = 12u;
inline constexpr std::size_t kAvatarWearedGum = 18u;
inline constexpr std::uint32_t kShopItemUseParamEquipAvatar = 10u;

using AvatarSlots = std::array<std::uint16_t, game::EAvatarCount>;

struct AvatarItemBaseView final {
    std::uint32_t db_idx = 0;
};

struct AvatarItemInfoView final {
    std::uint32_t sell_price = 0;
};

struct AvatarUsingItemView final {
    std::uint32_t db_idx = 0;
};

class AvatarEquipEnv {
public:
    virtual ~AvatarEquipEnv() = default;
    virtual const AvatarUsingItemView* find_using_item(
        std::uint16_t item_idx) const noexcept = 0;
    virtual const AvatarItemBaseView* find_item_at(
        std::uint16_t item_pos) const noexcept = 0;
    virtual const AvatarEquipRow* find_avatar_equip(
        std::uint16_t item_idx) const noexcept = 0;
    virtual const AvatarItemInfoView* find_item_info(
        std::uint16_t item_idx) const noexcept = 0;
};

enum class AvatarEquipStatus : std::uint8_t {
    Ok = 0,
    AvatarMissing,
    PositionOutOfRange,
    ItemBaseMissing,
    UsingItemMissing,
    ItemBaseMismatch,
    AvatarEquipMissing,
    ItemInfoMissing,
    AvatarMismatch,
    HatBlockedByDress,
    WeaponSlotMismatch,
    ExistingItemInfoMissing,
    DressEquipMissing,
    DependentItemInfoMissing,
};

enum class AvatarEquipEffectKind : std::uint8_t {
    ParamUpdateToDb = 0,
    ParamUpdateInMemory,
};

struct AvatarEquipEffect final {
    AvatarEquipEffectKind kind = AvatarEquipEffectKind::ParamUpdateToDb;
    std::uint16_t item_idx = 0;
    std::uint32_t param = 0;
};

struct AvatarEquipTransition final {
    AvatarEquipStatus status = AvatarEquipStatus::AvatarMissing;
    AvatarSlots avatar{};
    std::vector<AvatarEquipEffect> effects;
    bool send_avatar_info = false;
    bool recalculate_avatar_option = false;
    bool calc_stats = true;
};

inline void append_avatar_param_update(
    AvatarEquipTransition& out,
    const AvatarEquipEnv& env,
    std::uint16_t db_item_idx,
    std::uint16_t using_item_idx,
    std::uint32_t param) {
    out.effects.push_back({AvatarEquipEffectKind::ParamUpdateToDb,
                           db_item_idx, param});
    if (env.find_using_item(using_item_idx) != nullptr) {
        out.effects.push_back({AvatarEquipEffectKind::ParamUpdateInMemory,
                               using_item_idx, param});
    }
}

inline void apply_weared_default_fill(AvatarSlots& avatar,
                                      const AvatarEquipRow& equip) {
    for (std::size_t n = kAvatarCosmeticEnd; n < kAvatarWearedGum; ++n) {
        if (equip.item[n] == 0u) {
            avatar[n] = 1u;
        }
    }
}

inline void apply_weared_default_clear(AvatarSlots& avatar,
                                       const AvatarEquipRow& equip) {
    for (std::size_t n = kAvatarCosmeticEnd; n < kAvatarWearedGum; ++n) {
        if (equip.item[n] == 0u) {
            avatar[n] = 0u;
        }
    }
}

inline AvatarEquipTransition put_on_avatar_item(
    const AvatarEquipEnv& env,
    const AvatarSlots* current_avatar,
    std::uint16_t item_idx,
    std::uint16_t item_pos,
    bool player_inited,
    std::uint16_t weapon_equip_type,
    bool calc_stats = true) {
    AvatarEquipTransition out;
    out.calc_stats = calc_stats;
    out.effects.reserve(32u);

    if (current_avatar == nullptr) {
        out.status = AvatarEquipStatus::AvatarMissing;
        return out;
    }
    if (item_pos >= game::EAvatarCount) {
        out.status = AvatarEquipStatus::PositionOutOfRange;
        return out;
    }

    const AvatarUsingItemView* shop_item = env.find_using_item(item_idx);
    const AvatarItemBaseView* item_base = env.find_item_at(item_pos);
    if (item_base == nullptr) {
        out.status = AvatarEquipStatus::ItemBaseMissing;
        return out;
    }
    if (shop_item == nullptr) {
        out.status = AvatarEquipStatus::UsingItemMissing;
        return out;
    }
    if (shop_item->db_idx != item_base->db_idx) {
        out.status = AvatarEquipStatus::ItemBaseMismatch;
        return out;
    }

    out.avatar = *current_avatar;

    const AvatarEquipRow* avatar_equip = env.find_avatar_equip(item_idx);
    if (avatar_equip == nullptr) {
        out.status = AvatarEquipStatus::AvatarEquipMissing;
        return out;
    }
    const std::size_t position = avatar_equip->position;
    if (position >= game::EAvatarCount) {
        out.status = AvatarEquipStatus::PositionOutOfRange;
        return out;
    }
    if (env.find_item_info(item_idx) == nullptr) {
        out.status = AvatarEquipStatus::ItemInfoMissing;
        return out;
    }

    if (position == static_cast<std::size_t>(game::AvatarSlot::Hat) &&
        out.avatar[static_cast<std::size_t>(game::AvatarSlot::Dress)] != 0u) {
        const AvatarEquipRow* dress_equip = env.find_avatar_equip(
            out.avatar[static_cast<std::size_t>(game::AvatarSlot::Dress)]);
        if (dress_equip != nullptr &&
            dress_equip->item[static_cast<std::size_t>(game::AvatarSlot::Hat)] == 0u) {
            out.status = AvatarEquipStatus::HatBlockedByDress;
            return out;
        }
    }

    if (position >= kAvatarWearedGum) {
        if (player_inited &&
            position != kAvatarWearedGum + weapon_equip_type - 1u) {
            out.status = AvatarEquipStatus::WeaponSlotMismatch;
            return out;
        }
        if (out.avatar[position] > 1u) {
            const std::uint16_t old_item = out.avatar[position];
            const AvatarItemInfoView* old_info = env.find_item_info(old_item);
            if (old_info == nullptr) {
                out.status = AvatarEquipStatus::ExistingItemInfoMissing;
                return out;
            }
            append_avatar_param_update(out, env, old_item, old_item,
                                       old_info->sell_price);
        }
        out.avatar[position] = item_idx;
        append_avatar_param_update(out, env, item_idx, item_idx,
                                   kShopItemUseParamEquipAvatar);
    } else {
        if (out.avatar[position] != 0u) {
            const std::uint16_t old_item = out.avatar[position];
            const AvatarEquipRow* old_equip = env.find_avatar_equip(old_item);
            const AvatarItemInfoView* old_info = env.find_item_info(old_item);
            if (old_equip != nullptr && old_info != nullptr) {
                apply_weared_default_fill(out.avatar, *old_equip);
                append_avatar_param_update(out, env, old_item, old_item,
                                           old_info->sell_price);
            }
        }
        out.avatar[position] = item_idx;
        apply_weared_default_fill(out.avatar, *avatar_equip);
        append_avatar_param_update(out, env, item_idx, item_idx,
                                   kShopItemUseParamEquipAvatar);
    }

    for (std::size_t i = 0; i < kAvatarCosmeticEnd; ++i) {
        if (i == position) {
            continue;
        }
        if (avatar_equip->item[i] == 0u && out.avatar[i] != 0u) {
            const std::uint16_t removed_item = out.avatar[i];
            const AvatarEquipRow* removed_equip =
                env.find_avatar_equip(removed_item);
            const AvatarItemInfoView* removed_info =
                env.find_item_info(removed_item);
            if (removed_equip == nullptr || removed_info == nullptr) {
                continue;
            }
            apply_weared_default_fill(out.avatar, *removed_equip);
            append_avatar_param_update(out, env, removed_item, item_idx,
                                       removed_info->sell_price);
            out.avatar[i] = 0u;
        }
    }

    out.status = AvatarEquipStatus::Ok;
    out.send_avatar_info = item_pos != 0u;
    out.recalculate_avatar_option = true;
    return out;
}

inline AvatarEquipTransition take_off_avatar_item(
    const AvatarEquipEnv& env,
    const AvatarSlots* current_avatar,
    std::uint16_t item_idx,
    std::uint16_t item_pos,
    bool calc_stats = true) {
    AvatarEquipTransition out;
    out.calc_stats = calc_stats;
    out.effects.reserve(32u);

    if (current_avatar == nullptr) {
        out.status = AvatarEquipStatus::AvatarMissing;
        return out;
    }

    const AvatarEquipRow* avatar_equip = env.find_avatar_equip(item_idx);
    if (avatar_equip == nullptr) {
        out.status = AvatarEquipStatus::AvatarEquipMissing;
        return out;
    }
    const std::size_t position = avatar_equip->position;
    if (position >= game::EAvatarCount) {
        out.status = AvatarEquipStatus::PositionOutOfRange;
        return out;
    }
    const AvatarItemInfoView* item_info = env.find_item_info(item_idx);
    if (item_info == nullptr) {
        out.status = AvatarEquipStatus::ItemInfoMissing;
        return out;
    }

    out.avatar = *current_avatar;
    if (out.avatar[position] != item_idx) {
        out.status = AvatarEquipStatus::AvatarMismatch;
        return out;
    }

    if (position >= kAvatarWearedGum && out.avatar[position] > 1u) {
        bool check = false;
        if (out.avatar[static_cast<std::size_t>(game::AvatarSlot::Dress)] != 0u) {
            const AvatarEquipRow* dress_equip = env.find_avatar_equip(
                out.avatar[static_cast<std::size_t>(game::AvatarSlot::Dress)]);
            if (dress_equip == nullptr) {
                out.status = AvatarEquipStatus::DressEquipMissing;
                return out;
            }
            out.avatar[position] = dress_equip->item[position];
        } else {
            if (out.avatar[position] <= 1u) {
                out.avatar[position] = 0u;
            } else {
                check = true;
            }
        }
        if (!check) {
            append_avatar_param_update(out, env,
                                       out.avatar[position],
                                       out.avatar[position],
                                       item_info->sell_price);
        }
    }

    for (std::size_t i = 0; i < game::EAvatarCount; ++i) {
        if (out.avatar[i] == 0u) {
            continue;
        }
        if (i == position) {
            out.avatar[i] = 0u;
            apply_weared_default_fill(out.avatar, *avatar_equip);
            append_avatar_param_update(out, env, item_idx, item_idx,
                                       item_info->sell_price);
            continue;
        }
        if (i < kAvatarCosmeticEnd && avatar_equip->item[i] == 0u) {
            const AvatarItemInfoView* off_info = env.find_item_info(out.avatar[i]);
            if (off_info == nullptr) {
                out.status = AvatarEquipStatus::DependentItemInfoMissing;
                return out;
            }
            append_avatar_param_update(out, env, out.avatar[i], out.avatar[i],
                                       off_info->sell_price);
            out.avatar[i] = 0u;
        }
    }

    out.status = AvatarEquipStatus::Ok;
    out.send_avatar_info = true;
    out.recalculate_avatar_option = true;
    (void)item_pos;
    return out;
}

}  // namespace mxh::server
