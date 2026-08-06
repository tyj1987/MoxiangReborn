// avatar_calc.hpp
//
// 1:1 port of legacy CShopItemManager::CalcAvatarOption() from
// [Server]Map/ShopItemManager.cpp. The accumulator walks the player`s
// 24-slot avatar[] array (m_pPlayer->GetShopItemStats()->Avatar) and,
// for every non-empty slot (wIconIdx >= 2), looks up the corresponding
// ITEM_INFO and folds the per-field deltas into an AVATARITEMOPTION.
//
// The legacy function does 3 things:
//   1. zero AVATARITEMOPTION via memset()
//   2. for i in [0, eAvatar_Max): skip slot if Avatar[i] < 2 (cosmetic
//      slot has wIconIdx 0 = empty, wIconIdx 1 = base skin); otherwise
//      ITEMMGR->GetItemInfo(Avatar[i]) and, on success, accumulate
//      28 field deltas into the output struct
//   3. (deferred) optional snow-weather event bonuses (CAT_HAT,
//      CAT_DRESS, WEDDING outfits). Not ported here yet - the snow
//      event tables live in the resource manager.
//
// This header splits the data-plane (steps 1 + 2) from the orchestrator
// side effects (legacy `Weather`, `bCalcStats` flag, Player recalc,
// StatChanged broadcasts). It mirrors the use_shop_item_decision pattern:
// pure accumulator, no IO, no Player state changes.
//
// 1:1 invariants:
//   - Field accumulation order, predicates (> 0 vs == 1), and the
//     narrow cast from uint16 (ItemInfo) to uint8 (AvatarItemOption)
//     match legacy byte-for-byte. Saturating arithmetic is *not* used
//     because legacy also relies on implicit truncation at the WORD/BYTE
//     boundary (the legacy output struct holds those smaller types and
//     the modern port preserves the same truncation semantics).
//   - Empty slots (Avatar[i] < 2) are skipped, matching legacy.
//   - Lookup miss (ItemInfo not found) is skipped, matching legacy.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <mxh/game/avatar_item_option.hpp>
#include <mxh/game/item_list_types.hpp>
#include <mxh/game/item_manager.hpp>

namespace mxh::server {

// 1:1 port of legacy CShopItemManager::CalcAvatarOption().
//
// Walks avatar[0..EAvatarCount), skipping slots whose wIconIdx is < 2.
// For every other slot, looks up ItemInfo via the supplied manager and
// folds 28 field deltas into the returned AvatarItemOption.
//
// ItemInfo fields consumed (with their accumulator):
//   - GenGol             -> Gengol (byte cast)
//   - MinChub            -> Minchub (byte cast)
//   - CheRyuk            -> Cheryuk (byte cast)
//   - SimMek             -> Simmek (byte cast)
//   - Life               -> Life (word cast)
//   - Shield             -> Shield (word cast)
//   - NaeRyuk            -> Naeruyk (uint16 pass-through)
//   - MeleeAttackMin     -> Attack (byte cast)
//   - CriticalPercent    -> Critical (byte cast)
//   - Plus_MugongIdx     -> CounterPercent (uint16)
//   - Plus_Value         -> CounterDamage (uint16)
//   - AllPlus_Kind == 1  -> bKyungGong = 1 (flag, not sum)
//   - LimitCheRyuk       -> NeaRyukSpend (byte cast)
//   - LimitJob           -> NeagongDamage (uint16)
//   - LimitGender        -> WoigongDamage (uint16)
//   - LimitLevel         -> TargetPhyDefDown (uint16)
//   - LimitGenGol        -> TargetAttrDefDown (uint16)
//   - LimitMinChub       -> TargetAtkDown (uint16)
//   - LimitSimMek        -> RecoverRate (uint16)
//   - ItemGrade          -> KyunggongSpeed (uint16)
//   - RangeType          -> MussangCharge (uint16)
//   - EquipKind == 1     -> NaeruykspendbyKG = 1 (flag, not sum)
//   - NaeRyukRecover     -> Decisive (byte cast)
//   - RangeAttackMin     -> ShieldRecoverRate (uint16)
//   - RangeAttackMax     -> MussangDamage (byte cast)
inline game::AvatarItemOption calc_avatar_option(
    const std::array<std::uint16_t, game::EAvatarCount>& avatar,
    const game::ItemManager& mgr) noexcept {
    game::AvatarItemOption out{};
    game::ItemInfo info{};
    for (std::size_t i = 0; i < game::EAvatarCount; ++i) {
        const std::uint16_t wIconIdx = avatar[i];
        // Legacy: skip if (pAvatar[i] < 2) -> continue;
        // Cosmetic slots have wIconIdx 0 (empty) or 1 (base skin);
        // avatar-equipped items use wIconIdx >= 2 (item table index).
        if (wIconIdx < 2) continue;
        if (!mgr.try_get(static_cast<std::uint32_t>(wIconIdx), info)) {
            continue;
        }
        // 28-field accumulator. Predicate matches legacy exactly:
        // the legacy code writes `if (info.X > 0) opt->Y += info.X;`
        // for sums, and `if (info.X == 1) opt->Y = 1;` for flags.
        if (info.GenGol > 0)         out.Gengol  = static_cast<std::uint8_t>(out.Gengol  + static_cast<std::uint8_t>(info.GenGol));
        if (info.MinChub > 0)        out.Minchub = static_cast<std::uint8_t>(out.Minchub + static_cast<std::uint8_t>(info.MinChub));
        if (info.CheRyuk > 0)        out.Cheryuk = static_cast<std::uint8_t>(out.Cheryuk + static_cast<std::uint8_t>(info.CheRyuk));
        if (info.SimMek > 0)         out.Simmek  = static_cast<std::uint8_t>(out.Simmek  + static_cast<std::uint8_t>(info.SimMek));
        if (info.Life > 0)           out.Life    = static_cast<std::uint16_t>(out.Life    + static_cast<std::uint16_t>(info.Life));
        if (info.Shield > 0)         out.Shield  = static_cast<std::uint16_t>(out.Shield  + static_cast<std::uint16_t>(info.Shield));
        if (info.NaeRyuk > 0)        out.Naeruyk = static_cast<std::uint16_t>(out.Naeruyk + info.NaeRyuk);
        if (info.MeleeAttackMin > 0) out.Attack  = static_cast<std::uint8_t>(out.Attack  + static_cast<std::uint8_t>(info.MeleeAttackMin));
        if (info.CriticalPercent > 0) out.Critical = static_cast<std::uint8_t>(out.Critical + static_cast<std::uint8_t>(info.CriticalPercent));
        if (info.Plus_MugongIdx > 0) out.CounterPercent = static_cast<std::uint16_t>(out.CounterPercent + info.Plus_MugongIdx);
        if (info.Plus_Value > 0)     out.CounterDamage  = static_cast<std::uint16_t>(out.CounterDamage  + info.Plus_Value);
        if (info.AllPlus_Kind == 1)  out.bKyungGong = 1;
        if (info.LimitCheRyuk > 0)   out.NeaRyukSpend = static_cast<std::uint8_t>(out.NeaRyukSpend + static_cast<std::uint8_t>(info.LimitCheRyuk));
        if (info.LimitJob > 0)       out.NeagongDamage = static_cast<std::uint16_t>(out.NeagongDamage + info.LimitJob);
        if (info.LimitGender > 0)    out.WoigongDamage = static_cast<std::uint16_t>(out.WoigongDamage + info.LimitGender);
        if (info.LimitLevel > 0)     out.TargetPhyDefDown = static_cast<std::uint16_t>(out.TargetPhyDefDown + info.LimitLevel);
        if (info.LimitGenGol > 0)    out.TargetAttrDefDown = static_cast<std::uint16_t>(out.TargetAttrDefDown + info.LimitGenGol);
        if (info.LimitMinChub > 0)   out.TargetAtkDown = static_cast<std::uint16_t>(out.TargetAtkDown + info.LimitMinChub);
        if (info.LimitSimMek)        out.RecoverRate = static_cast<std::uint16_t>(out.RecoverRate + info.LimitSimMek);
        if (info.ItemGrade)          out.KyunggongSpeed = static_cast<std::uint16_t>(out.KyunggongSpeed + info.ItemGrade);
        if (info.RangeType)          out.MussangCharge = static_cast<std::uint16_t>(out.MussangCharge + info.RangeType);
        if (info.EquipKind == 1)     out.NaeruykspendbyKG = 1;
        if (info.NaeRyukRecover > 0) out.Decisive = static_cast<std::uint8_t>(out.Decisive + static_cast<std::uint8_t>(info.NaeRyukRecover));
        if (info.RangeAttackMin > 0) out.ShieldRecoverRate = static_cast<std::uint16_t>(out.ShieldRecoverRate + info.RangeAttackMin);
        if (info.RangeAttackMax > 0) out.MussangDamage = static_cast<std::uint8_t>(out.MussangDamage + static_cast<std::uint8_t>(info.RangeAttackMax));
    }
    return out;
}

}  // namespace mxh::server
