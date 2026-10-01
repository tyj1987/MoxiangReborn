#pragma once

#include "mxh/game/base_attack_power.hpp"
#include "mxh/game/item_list_types.hpp"
#include "mxh/server/player_state.hpp"
#include <algorithm>

namespace mxh::server {

// Ordinary KR/CN equipment only. Recompute from scratch; never accumulate a
// previous result. Unsupported templates/options fail before publishing stats.
// Caller retains/restores the equipment slots if a proposed move is rejected.
template<class Lookup>
bool rebuild_equipment_stats(PlayerState& state, game::PlayerCombatStats& combat,
                             Lookup lookup) {
    PlayerAttributes attributes{};
    std::uint16_t melee_min=0, melee_max=0, range_min=0, range_max=0, defence=0;
    std::uint32_t life=0, shield=0;
    std::uint16_t mp=0, weapon_kind=0;
    bool weapon_present=false;
    float melee_rate_min=0, melee_rate_max=0, range_rate_min=0, range_rate_max=0;
    constexpr float rates[]{1.f,.6f,.55f,.5f,.45f,.4f,.35f,.30f,.3f,.25f,.2f,.15f,.1f,.05f};
    for (std::size_t slot=0; slot<state.equipment.items.size(); ++slot) {
        const auto& item=state.equipment.items[slot];
        if (!item.dwDBIdx) continue; // Appearance-only icons confer no stats.
        game::ItemInfo info{};
        if (!lookup(item.wIconIdx,info) || !(info.ItemKind & 2048u) ||
            !(info.EquipKind == slot || (info.EquipKind == game::WEARED_RING1 && slot == game::WEARED_RING2)) ||
            (item.Durability != 0u && item.Durability != 100u) ||
            item.RareIdx || info.wSetItemKind) return false;
        // Modern make_item/grant/drop/shop creation uses exactly 100 for plain
        // durability; zero is also used for plain fixtures/records. These are
        // not a general legacy option-index range. Original nonzero option IDs
        // require an explicit import/option model (including a legacy ID100).
        // Elemental options require the separate per-element combat pipeline.
        for (unsigned i=0;i<game::ITEM_ELEM_MAX;++i)
            if (info.AttrAttack.Element[i] != 0 || info.AttrRegist.Element[i] != 0) return false;
        const auto gap=std::clamp(int(info.LimitLevel)-int(state.progress.level)-int(state.shop_options.EquipLevelFree),0,13);
        const float rate=rates[gap];
        const auto word=[&](auto value) { return static_cast<std::uint16_t>(value*rate); };
        if (info.ItemKind == 2058u) { // eEQUIP_ITEM_ARMLET: percentage, not flat damage.
            melee_rate_min += info.MeleeAttackMin*rate*.01f;
            melee_rate_max += info.MeleeAttackMax*rate*.01f;
            range_rate_min += info.RangeAttackMin*rate*.01f;
            range_rate_max += info.RangeAttackMax*rate*.01f;
        } else {
            melee_min += word(info.MeleeAttackMin); melee_max += word(info.MeleeAttackMax);
            range_min += word(info.RangeAttackMin); range_max += word(info.RangeAttackMax);
        }
        defence += word(info.PhyDef);
        attributes.gengol += word(info.GenGol); attributes.minchub += word(info.MinChub);
        attributes.cheryuk += word(info.CheRyuk); attributes.simmek += word(info.SimMek);
        life += static_cast<std::uint32_t>(info.Life*rate);
        shield += static_cast<std::uint32_t>(info.Shield*rate); mp += word(info.NaeRyuk);
        if (slot == game::WEARED_WEAPON) { weapon_present=true; weapon_kind=info.WeaponType; }
    }
    const auto gengol=static_cast<std::uint16_t>(state.attributes.gengol+attributes.gengol+state.shop_options.Gengol+state.avatar_options.Gengol);
    const auto minchub=static_cast<std::uint16_t>(state.attributes.minchub+attributes.minchub+state.shop_options.Minchub+state.avatar_options.Minchub);
    const auto cheryuk=static_cast<std::uint16_t>(state.attributes.cheryuk+attributes.cheryuk+state.shop_options.Cheryuk+state.avatar_options.Cheryuk);
    const bool ranged=weapon_kind == 5 || weapon_kind == 6; // WP_GUNG / WP_AMGI.
    const auto endpoint=[&](bool maximum) -> std::optional<std::uint32_t> {
        if (weapon_kind == 9) return 0u; // WP_EVENT_HAMMER.
        const auto weapon=static_cast<std::uint16_t>((ranged ? (maximum?range_max:range_min) :
            (maximum?melee_max:melee_min)+(weapon_present?0:5))+state.avatar_options.Attack);
        const auto power=game::base_attack_power(ranged?minchub:gengol,weapon);
        if (!power) return std::nullopt;
        const float bonus=ranged ? (maximum?range_rate_max:range_rate_min) : (maximum?melee_rate_max:melee_rate_min);
        const double value=static_cast<float>(*power)*(1.f+bonus);
        if (value > UINT32_MAX) return std::nullopt;
        return static_cast<std::uint32_t>(value);
    };
    const auto minimum=endpoint(false), maximum=endpoint(true);
    if (!minimum || !maximum || *maximum < *minimum) return false;
    state.equipment_attributes=attributes;
    state.bonuses.item_max_life=static_cast<std::int32_t>(life);
    state.bonuses.item_max_shield=static_cast<std::int32_t>(shield);
    state.bonuses.item_max_naeryuk=mp;
    state.recompute_max_stats();
    state.vitals.current_hp=std::min(state.vitals.current_hp,state.vitals.max_hp);
    state.vitals.current_mp=std::min(state.vitals.current_mp,state.vitals.max_mp);
    state.vitals.current_shield=std::min(state.vitals.current_shield,state.vitals.max_shield);
    combat.phy_attack_min=*minimum; combat.phy_attack_max=*maximum;
    combat.has_physical_range=true;
    combat.phy_defence=static_cast<std::uint16_t>(cheryuk/1.5)+defence;
    return true;
}

} // namespace mxh::server
