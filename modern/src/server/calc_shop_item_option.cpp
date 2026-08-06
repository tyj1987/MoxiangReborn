// calc_shop_item_option.cpp
//
// Pure data plane implementation of CShopItemManager::CalcShopItemOption.
// See calc_shop_item_option.hpp for the 1:1 invariants and the
// legacy-line citations.

#include "mxh/server/calc_shop_item_option.hpp"

#include <cstdint>

namespace mxh::server {

namespace {

// Clamp-to-zero helper for signed accumulator under flow. Legacy uses
// if (X < 0) X = 0; for signed fields and if (X <= 0) X = 0; for the
// unsigned WORD fields that the legacy code reads as `if (X < 0)`.
// We split the two flavors so the unsigned case stays unsigned.
inline std::int8_t clamp_signed8(std::int8_t v) noexcept {
    return v < 0 ? std::int8_t{0} : v;
}

inline std::uint16_t clamp_unsigned16_legacy(std::uint16_t v) noexcept {
    // Legacy `if (X < 0) X = 0;` with a 16-bit unsigned field -- the
    // value is treated as int, the < 0 check is false for all valid
    // WORD values, so the only way to hit the clamp is when the field
    // was constructed negative (which never happens at runtime). The
    // no-op preserves the legacy test semantics.
    return v;
}

// Pure clamp helper for the WORD fields that the legacy code wraps
// in `if (X <= 0) X = 0;` (Life / Shield / Naeryuk in the HERB branch).
inline std::uint16_t clamp_word_legacy(std::uint16_t v) noexcept {
    return v == 0 ? std::uint16_t{0} : v;
}

}  // namespace

CalcShopItemOptionStatus calc_shop_item_option(
    mxh::game::ShopItemOption& stats,
    std::uint32_t w_idx,
    bool b_add,
    std::uint32_t param,
    const CalcShopItemOptionInfo& info,
    const CalcShopItemOptionEnv& env,
    std::uint32_t current_protect_item_idx,
    CalcShopItemOptionSideEffects& out_side_effects) noexcept {
    (void)current_protect_item_idx;

    // Legacy guard 1: wIdx == 0 -> return FALSE.
    if (w_idx == 0) {
        return CalcShopItemOptionStatus::InvalidIcon;
    }

    // Legacy guard 2: pShopItemOption == null -> return FALSE.
    // We pass references; the caller is the null-checker. Mark as
    // NullStats only if the orchestrator hands us an unset stats
    // (Avatar array empty AND all stat fields zero AND w_idx!=0 is
    // a non-special-inc case, but legacy has no real "null stats"
    // surface -- the guard is the pointer null check). We keep the
    // status for symmetry with the other guards; the caller can
    // detect "null stats" by pre-validating.

    // Legacy guard 3: pItemInfo == null AND wIdx is not SkPointRedist or
    // StatePoint -> ASSERTMSG + return FALSE. For the special incantations
    // the legacy code returns FALSE silently (the early-out path is
    // handled by the UsedShopItem caller, not CalcShopItemOption).
    const bool is_special_inc =
        w_idx == static_cast<std::uint32_t>(IncantationId::SkPointRedist) ||
        w_idx == static_cast<std::uint32_t>(IncantationId::StatePoint);
    // Legacy semantics: missing ItemInfo is always a hard FALSE for
    // CalcShopItemOption, regardless of whether the wIdx is a special
    // incantation. The orchestrator routes the special incantations
    // through UsedShopItem, not CalcShopItemOption, so the data plane
    // returns ItemInfoMissing either way.
    if (info.ItemKind == 0 && info.ItemIdx == 0) {
        return CalcShopItemOptionStatus::ItemInfoMissing;
    }
    (void)is_special_inc;

    // Legacy `int calc = -1; if (bAdd) calc = 1;`.
    const int calc = b_add ? 1 : -1;

    // Default side effects: protect-item only mutates on the CheRyuk
    // incantation branch; the locale-bounded extras fire only on the
    // locale-bounded incantation IDs.
    out_side_effects.new_protect_item_idx = current_protect_item_idx;
    out_side_effects.expanded_inven_slot = false;
    out_side_effects.expanded_pyoguk_slot = false;
    out_side_effects.expanded_mugong_slot = false;
    out_side_effects.expanded_character_slot = false;

    // ---- Branch 1: eSHOP_ITEM_INCANTATION ----
    if (info.ItemKind == LEGACY_SHOP_ITEM_INCANTATION) {
        // MixUp: ItemMixSuccess += 10 * calc, clamp to 0.
        if (info.ItemIdx == static_cast<std::uint32_t>(IncantationId::MixUp)) {
            // Legacy: int mixNext = m_ShopItemOption.ItemMixSuccess + 10*calc;
            // if (mixNext < 0) m_ShopItemOption.ItemMixSuccess = 0;
            // else m_ShopItemOption.ItemMixSuccess = mixNext;
            const int next = static_cast<int>(stats.ItemMixSuccess) + 10 * calc;
            stats.ItemMixSuccess = next < 0 ? std::int8_t{0}
                                            : static_cast<std::int8_t>(next);
        }

        // ItemIdx == eIncantation_StatePoint && Info->GenGol: legacy
        // adds (WORD)Param to StatePoint unconditionally (no calc, no
        // bAdd gate). This is the special SkPointRedist 55300 / 55323
        // / etc. branch that doesn't fit the GenGol name in the
        // legacy source but is documented in the comment about
        // "SetPlayerLevelUpPoint" (commented out).
        if (info.GenGol != 0) {
            if (param > 0) {
                stats.StatePoint = static_cast<std::uint16_t>(
                    stats.StatePoint + static_cast<std::uint16_t>(param));
            }
            // The legacy commented-out branch (else: SetPlayerLevelUpPoint
            // + StatePoint=0 + DeleteUsingShopItem) is intentionally
            // not ported -- it was disabled at the source level.
        }
        // ItemIdx == eIncantation_SkPointRedist family (Legacy BOOL
        // UsedShopItem path) sets SkillPoint = Param + UseSkillPoint = RemainTime.
        // CalcShopItemOption does not perform this assignment; the
        // UsedShopItem path does. We document but do not act here.
        else if (info.Life != 0) {
            // The legacy path for the SkillPoint family: SkillPoint += Life
            // if Param > 0. Note: the legacy code does NOT clamp the
            // SkillPoint DWORD accumulator.
            if (param > 0) {
                stats.SkillPoint = stats.SkillPoint + info.Life;
            }
        }
        else if (info.CheRyuk != 0) {
            // Protect-count incantation (MoneyProtect / ExpProtect).
            if (b_add) {
                if (param == 0) {
                    stats.ProtectCount = static_cast<std::int8_t>(info.CheRyuk);
                } else {
                    stats.ProtectCount = static_cast<std::int8_t>(param);
                }
                out_side_effects.new_protect_item_idx = info.ItemIdx;
            } else {
                out_side_effects.new_protect_item_idx = 0;
            }
        }
        else if (info.LimitJob != 0) {
            // EquipLevelFree incantation. The legacy operator +=(BYTE)
            // for bAdd and -=(BYTE) for bAdd==false. The legacy code
            // does NOT clamp the BYTE field.
            if (b_add) {
                stats.EquipLevelFree = static_cast<std::uint8_t>(
                    stats.EquipLevelFree + static_cast<std::uint8_t>(info.LimitJob));
            } else {
                stats.EquipLevelFree = static_cast<std::uint8_t>(
                    stats.EquipLevelFree - static_cast<std::uint8_t>(info.LimitJob));
            }
        }
        // Locale-bounded incantations (InvenExtend / PyogukExtend /
        // MugongExtend / CharacterSlot). The legacy code does NOT
        // touch SHOPITEMOPTION for these -- it calls m_pPlayer->
        // SetExtraSlotCount / SetExtraCharacterSlot side effects.
        // The data plane captures the intent in the side-effects struct
        // so the orchestrator can dispatch the player-side updates.
        else {
            switch (static_cast<IncantationId>(info.ItemIdx)) {
                case IncantationId::InvenExtend:
                case IncantationId::InvenExtend2:
                    out_side_effects.expanded_inven_slot = true;
                    break;
                case IncantationId::PyogukExtend:
                case IncantationId::PyogukExtend2:
                    out_side_effects.expanded_pyoguk_slot = true;
                    break;
                case IncantationId::MugongExtend:
                case IncantationId::MugongExtend2:
                    out_side_effects.expanded_mugong_slot = true;
                    break;
                case IncantationId::CharacterSlot:
                case IncantationId::CharacterSlot2:
                    out_side_effects.expanded_character_slot = true;
                    break;
                default:
                    break;
            }
        }
    }
    // ---- Branch 2: eSHOP_ITEM_CHARM ----
    else if (info.ItemKind == LEGACY_SHOP_ITEM_CHARM) {
        // Helper macro for the legacy accumulate + clamp-to-zero pattern.
        // The legacy usage is uniform: `if (X > 0) { dst += X*calc; if (dst<0) dst=0; }`.
        // We inline the pattern to keep the C++17-friendly brace style.
        if (info.GenGol > 0) {
            const int next = static_cast<int>(stats.Gengol) +
                              static_cast<int>(info.GenGol) * calc;
            stats.Gengol = next < 0 ? std::uint16_t{0}
                                    : static_cast<std::uint16_t>(next);
        }
        if (info.MinChub > 0) {
            const int next = static_cast<int>(stats.Minchub) +
                              static_cast<int>(info.MinChub) * calc;
            stats.Minchub = next < 0 ? std::uint16_t{0}
                                     : static_cast<std::uint16_t>(next);
        }
        if (info.CheRyuk > 0) {
            const int next = static_cast<int>(stats.Cheryuk) +
                              static_cast<int>(info.CheRyuk) * calc;
            stats.Cheryuk = next < 0 ? std::uint16_t{0}
                                     : static_cast<std::uint16_t>(next);
        }
        if (info.SimMek > 0) {
            const int next = static_cast<int>(stats.Simmek) +
                              static_cast<int>(info.SimMek) * calc;
            stats.Simmek = next < 0 ? std::uint16_t{0}
                                    : static_cast<std::uint16_t>(next);
        }
        if (info.Life > 0) {
            // Legacy: dst += (char)(Life*calc); if (dst<0) dst=0;
            const int next = static_cast<int>(stats.NeagongDamage) +
                              static_cast<int>(info.Life) * calc;
            stats.NeagongDamage = clamp_signed8(static_cast<std::int8_t>(next));
        }
        if (info.Shield > 0) {
            const int next = static_cast<int>(stats.WoigongDamage) +
                              static_cast<int>(info.Shield) * calc;
            stats.WoigongDamage = clamp_signed8(static_cast<std::int8_t>(next));
        }
        if (info.NaeRyuk > 0) {
            const int next = static_cast<int>(stats.AddSung) +
                              static_cast<int>(info.NaeRyuk) * calc;
            stats.AddSung = next < 0 ? std::uint16_t{0}
                                     : static_cast<std::uint16_t>(next);
        }
        if (info.LimitJob > 0) {
            const int next = static_cast<int>(stats.ComboDamage) +
                              static_cast<int>(info.LimitJob) * calc;
            stats.ComboDamage = clamp_signed8(static_cast<std::int8_t>(next));
        }
        if (info.LimitGender > 0) {
            const int next = static_cast<int>(stats.Critical) +
                              static_cast<int>(info.LimitGender) * calc;
            stats.Critical = next < 0 ? std::uint16_t{0}
                                      : static_cast<std::uint16_t>(next);
        }
        if (info.LimitLevel > 0) {
            const int next = static_cast<int>(stats.StunByCri) +
                              static_cast<int>(info.LimitLevel) * calc;
            stats.StunByCri = clamp_signed8(static_cast<std::int8_t>(next));
        }
        if (info.LimitGenGol > 0) {
            const int next = static_cast<int>(stats.RegistPhys) +
                              static_cast<int>(info.LimitGenGol) * calc;
            stats.RegistPhys = clamp_signed8(static_cast<std::int8_t>(next));
        }
        if (info.LimitMinChub > 0) {
            const int next = static_cast<int>(stats.RegistAttr) +
                              static_cast<int>(info.LimitMinChub) * calc;
            stats.RegistAttr = clamp_signed8(static_cast<std::int8_t>(next));
        }
        // LimitCheRyuk: plustime branch if MeleeAttackMin != 0.
        if (info.LimitCheRyuk > 0) {
            if (info.MeleeAttackMin != 0) {
                if (b_add && env.event_rate_active(info.MeleeAttackMin)) {
                    stats.PlustimeNaeruyk = static_cast<std::int8_t>(info.LimitCheRyuk);
                } else {
                    stats.PlustimeNaeruyk = 0;
                }
            } else {
                const int next = static_cast<int>(stats.NeaRyukSpend) +
                                  static_cast<int>(info.LimitCheRyuk) * calc;
                stats.NeaRyukSpend = clamp_signed8(static_cast<std::int8_t>(next));
            }
        }
        // LimitSimMek: plustime branch if MeleeAttackMin != 0.
        if (info.LimitSimMek > 0) {
            if (info.MeleeAttackMin != 0) {
                if (b_add && env.event_rate_active(info.MeleeAttackMin)) {
                    stats.PlustimeExp = static_cast<std::int8_t>(info.LimitSimMek);
                } else {
                    stats.PlustimeExp = 0;
                }
            } else {
                const int next = static_cast<int>(stats.AddExp) +
                                  static_cast<int>(info.LimitSimMek) * calc;
                stats.AddExp = next < 0 ? std::uint16_t{0}
                                        : static_cast<std::uint16_t>(next);
            }
        }
        // ItemGrade: plustime branch if MeleeAttackMin != 0.
        if (info.ItemGrade > 0) {
            if (info.MeleeAttackMin != 0) {
                if (b_add && env.event_rate_active(info.MeleeAttackMin)) {
                    stats.PlustimeAbil = static_cast<std::int8_t>(info.ItemGrade);
                } else {
                    stats.PlustimeAbil = 0;
                }
            } else {
                const int next = static_cast<int>(stats.AddAbility) +
                                  static_cast<int>(info.ItemGrade) * calc;
                stats.AddAbility = next < 0 ? std::uint16_t{0}
                                            : static_cast<std::uint16_t>(next);
            }
        }
        if (info.RangeType > 0) {
            const int next = static_cast<int>(stats.AddMugongExp) +
                              static_cast<int>(info.RangeType) * calc;
            stats.AddMugongExp = next < 0 ? std::uint16_t{0}
                                          : static_cast<std::uint16_t>(next);
        }
        // Plus_MugongIdx: legacy uses `if (Plus_MugongIdx)` (non-zero),
        // then `Life += (Plus_MugongIdx * calc); if (Life <= 0) Life = 0;`.
        if (info.Plus_MugongIdx != 0) {
            const int next = static_cast<int>(stats.Life) +
                              static_cast<int>(info.Plus_MugongIdx) * calc;
            stats.Life = next <= 0 ? std::uint16_t{0}
                                    : static_cast<std::uint16_t>(next);
        }
        if (info.Plus_Value != 0) {
            const int next = static_cast<int>(stats.Shield) +
                              static_cast<int>(info.Plus_Value) * calc;
            stats.Shield = next <= 0 ? std::uint16_t{0}
                                     : static_cast<std::uint16_t>(next);
        }
        if (info.AllPlus_Kind != 0) {
            const int next = static_cast<int>(stats.Naeryuk) +
                              static_cast<int>(info.AllPlus_Kind) * calc;
            stats.Naeryuk = next <= 0 ? std::uint16_t{0}
                                      : static_cast<std::uint16_t>(next);
        }
        if (info.RangeAttackMin != 0) {
            // No clamp for these two (legacy uses BYTE arithmetic with
            // signed char that wraps; the data plane keeps the unsigned
            // semantics to match the BYTE field).
            stats.bKyungGong = static_cast<std::uint8_t>(
                stats.bKyungGong + static_cast<std::uint8_t>(info.RangeAttackMin) * static_cast<std::uint8_t>(calc));
        }
        if (info.RangeAttackMax != 0) {
            stats.KyungGongSpeed = static_cast<std::uint8_t>(
                stats.KyungGongSpeed + static_cast<std::uint8_t>(info.RangeAttackMax) * static_cast<std::uint8_t>(calc));
        }
        if (info.CriticalPercent != 0) {
            stats.ReinforceAmp = static_cast<std::uint8_t>(
                stats.ReinforceAmp + static_cast<std::uint8_t>(info.CriticalPercent) * static_cast<std::uint8_t>(calc));
        }
        if (info.PhyDef != 0) {
            // No clamp in legacy: AddItemDrop += PhyDef*calc.
            stats.AddItemDrop = static_cast<std::uint16_t>(
                stats.AddItemDrop + static_cast<std::uint16_t>(info.PhyDef) * static_cast<std::uint16_t>(calc));
        }
        if (info.NaeRyukRecover > 0) {
            const int next = static_cast<int>(stats.Decisive) +
                              static_cast<int>(info.NaeRyukRecover) * calc;
            stats.Decisive = next < 0 ? std::uint16_t{0}
                                      : static_cast<std::uint16_t>(next);
        }
        // AttrFire: dwStreetStallDecoration = (bAdd? ItemIdx : 0).
        if (info.AttrFire > 0) {
            if (b_add) {
                stats.dwStreetStallDecoration = info.ItemIdx;
            } else {
                stats.dwStreetStallDecoration = 0;
            }
        }
    }
    // ---- Branch 3: eSHOP_ITEM_HERB ----
    else if (info.ItemKind == LEGACY_SHOP_ITEM_HERB) {
        if (info.Life > 0) {
            const int next = static_cast<int>(stats.Life) +
                              static_cast<int>(info.Life) * calc;
            stats.Life = next <= 0 ? std::uint16_t{0}
                                    : static_cast<std::uint16_t>(next);
        }
        if (info.Shield > 0) {
            const int next = static_cast<int>(stats.Shield) +
                              static_cast<int>(info.Shield) * calc;
            stats.Shield = next <= 0 ? std::uint16_t{0}
                                     : static_cast<std::uint16_t>(next);
        }
        if (info.NaeRyuk > 0) {
            const int next = static_cast<int>(stats.Naeryuk) +
                              static_cast<int>(info.NaeRyuk) * calc;
            stats.Naeryuk = next <= 0 ? std::uint16_t{0}
                                      : static_cast<std::uint16_t>(next);
        }
    }
    // ---- Branch 4: eSHOP_ITEM_MAKEUP / eSHOP_ITEM_DECORATION ----
    // Legacy body is a single commented-out CheckAvatarEquip call. The
    // data plane has no SHOPITEMOPTION mutation for this branch.
    // ---- Branch 5: eSHOP_ITEM_SUNDRIES ----
    // Legacy: HK_LOCAL only -- bStreetStall += CheRyuk*calc. Other
    // locales have the branch effectively empty (the SimMek comment is
    // commented out). The data plane applies the HK_LOCAL behavior
    // unconditionally when CheRyuk != 0; orchestrators in non-HK
    // locales can short-circuit by passing CheRyuk=0.
    else if (info.ItemKind == LEGACY_SHOP_ITEM_SUNDRIES) {
        if (info.CheRyuk != 0) {
            const int next = static_cast<int>(stats.bStreetStall) +
                              static_cast<int>(info.CheRyuk) * calc;
            stats.bStreetStall = next < 0 ? std::uint8_t{0}
                                          : static_cast<std::uint8_t>(next);
        }
    }

    return CalcShopItemOptionStatus::Ok;
}

}  // namespace mxh::server
