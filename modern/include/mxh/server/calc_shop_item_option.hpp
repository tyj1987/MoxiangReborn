// calc_shop_item_option.hpp
//
// 1:1 port of legacy CShopItemManager::CalcShopItemOption() from
// [Server]Map/ShopItemManager.cpp:1246-1691. Splits the function into a
// pure data plane (this header) and an orchestrator half (the legacy
// runtime calls STATSMGR->CalcCharStats / m_pPlayer->SetExtraSlotCount /
// etc. -- those hooks stay in the orchestrator and are documented here).
//
// 1:1 invariants:
//   - Branch dispatch mirrors legacy pItemInfo->ItemKind (eSHOP_ITEM_*).
//   - The +/- accumulator sign is `calc` (legacy `int calc = -1;
//     if (bAdd) calc = 1;`).
//   - The clamp-to-zero semantics preserve unsigned wrap for the underflow
//     case (e.g. Gengol -= Gengol clamps to 0, NOT wraps to 65535).
//   - The plustime gates (legacy gEventRate / gEventRateFile) are exposed
//     via a virtual hook on CalcShopItemOptionEnv so the data plane does
//     not depend on the runtime globals.
//   - The protect-item index redirect (legacy m_ProtectItemIdx) is part
//     of the function's effect; the data plane returns the new value
//     instead of mutating a member field.
//
// Locale-gated branches (legacy #ifdef _JAPAN_LOCAL_ / _HK_LOCAL_ /
// _TL_LOCAL_): only the InvenExtend / PyogukExtend / MugongExtend /
// CharacterSlot branches had locale splits, and all four locales agreed
// on the incantation indices. The locale splits only changed the
// indices applied (e.g. JP/HK also recognized the *2 variants). The
// modern port inlines the locale-agnostic behavior (a single ItemIdx
// per incantation) and documents the locale variants in the incantation
// id enum. The SetExtraSlotCount side effect is the orchestrator's
// responsibility.

#pragma once

#include <mxh/server/legacy_shop_item_kind.hpp>

#include <cstdint>

#include "mxh/game/shop_item_option.hpp"

namespace mxh::server {

// ---- 1:1 legacy enums ----

// Legacy eSHOP_ITEM_* constants live in legacy_shop_item_kind.hpp.

// Legacy eIncantation_* values from [CC]Header/CommonGameDefine.h:2755-2815.
// Only the ones referenced by CalcShopItemOption are exposed.
enum class IncantationId : std::uint32_t {
    SkPointRedist      = 55300u,  // eIncantation_SkPointRedist
    StatePoint         = 55299u,  // eIncantation_StatePoint
    MixUp              = 55322u,  // eIncantation_MixUp
    InvenExtend        = 57542u,  // eIncantation_InvenExtend
    InvenExtend2       = 57958u,  // eIncantation_InvenExtend_2 (HK_LOCAL variant)
    PyogukExtend       = 57544u,  // eIncantation_PyogukExtend
    PyogukExtend2      = 57960u,  // eIncantation_PyogukExtend_2 (HK_LOCAL variant)
    MugongExtend       = 55361u,  // eIncantation_MugongExtend
    MugongExtend2      = 57957u,  // eIncantation_MugongExtend_2 (HK_LOCAL variant)
    CharacterSlot      = 57543u,  // eIncantation_CharacterSlot
    CharacterSlot2     = 57959u,  // eIncantation_CharacterSlot_2 (HK_LOCAL variant)
};

// 1:1 with legacy ITEM_INFO surface that CalcShopItemOption actually
// consults. The modern port does not load the full ITEM_INFO table; the
// orchestrator queries only the fields below and hands them in.
struct CalcShopItemOptionInfo {
    std::uint16_t ItemKind   = 0;    // eSHOP_ITEM_* discriminator
    std::uint32_t ItemIdx    = 0;    // eIncantation_* / specific item
    std::uint16_t ItemType   = 0;    // 10 = buff / 11 = avatar etc.

    // Charm stat inputs (legacy GenGol..PhyDef).
    std::uint16_t GenGol        = 0;
    std::uint16_t MinChub       = 0;
    std::uint16_t CheRyuk       = 0;
    std::uint16_t SimMek        = 0;
    std::uint32_t Life          = 0;
    std::uint32_t Shield        = 0;
    std::uint16_t NaeRyuk       = 0;

    // Charm limit / classifier fields (legacy LimitJob..LimitSimMek).
    std::uint16_t LimitJob      = 0;
    std::uint16_t LimitGender   = 0;
    std::uint16_t LimitLevel    = 0;
    std::uint16_t LimitGenGol   = 0;
    std::uint16_t LimitMinChub  = 0;
    std::uint16_t LimitCheRyuk  = 0;
    std::uint16_t LimitSimMek   = 0;

    // Charm misc fields used for plustime gates (legacy ItemGrade,
    // RangeType, Plus_MugongIdx, Plus_Value, AllPlus_Kind, RangeAttackMin,
    // RangeAttackMax, CriticalPercent, PhyDef, NaeRyukRecover,
    // MeleeAttackMin).
    std::uint16_t ItemGrade        = 0;
    std::uint16_t RangeType        = 0;
    std::uint16_t Plus_MugongIdx   = 0;
    std::uint16_t Plus_Value       = 0;
    std::uint16_t AllPlus_Kind     = 0;
    std::uint16_t RangeAttackMin   = 0;
    std::uint16_t RangeAttackMax   = 0;
    std::uint16_t CriticalPercent  = 0;
    std::uint16_t PhyDef           = 0;
    std::uint16_t NaeRyukRecover   = 0;
    std::uint16_t MeleeAttackMin   = 0;

    // AttrRegist.GetElement_Val(ATTR_FIRE) of the legacy item info.
    // The data plane does not drag in the ATTRIBUTEREGIST struct;
    // the orchestrator extracts the fire element and passes it here.
    std::uint16_t AttrFire         = 0;
};

// Environment for runtime-dependent gate checks. The legacy function
// reads two globals (gEventRate / gEventRateFile) and only enables the
// plustime branch when the rate is active. The default implementation
// returns true (matches the legacy "default rate is active" assumption).
// Tests can subclass to force the inactive branch.
class CalcShopItemOptionEnv {
public:
    virtual ~CalcShopItemOptionEnv() = default;

    // 1:1 with legacy gEventRate[rate_id] == gEventRateFile[rate_id].
    // The orchestrator wires gEventRate / gEventRateFile to this hook.
    virtual bool event_rate_active(std::uint16_t rate_id) const noexcept {
        (void)rate_id;
        return true;
    }
};

// 1:1 with legacy CShopItemManager::CalcShopItemOption return value:
//   return FALSE -> bail out (status != Ok)
//   return TRUE  -> applied one or more stat fields (status == Ok)
enum class CalcShopItemOptionStatus : std::uint8_t {
    Ok                   = 0,  // legacy TRUE
    InvalidIcon          = 1,  // legacy wIdx == 0
    NullStats            = 2,  // legacy pShopItemOption == null
    ItemInfoMissing      = 3,  // legacy pItemInfo == null with non-special wIdx
};

// Side effects that the legacy function applied to non-stat state.
// Captured here so the data plane can be tested without a Player /
// ShopItemManager instance.
struct CalcShopItemOptionSideEffects {
    // New value of legacy m_ProtectItemIdx after the call.
    // Set to ItemIdx when bAdd && CheRyuk != 0; set to 0 on bAdd==false
    // for the CheRyuk incantation; untouched otherwise.
    std::uint32_t new_protect_item_idx = 0;

    // Whether the locale-bounded incantation path matched. The
    // orchestrator uses this to dispatch SetExtraSlotCount /
    // SetExtraCharacterSlot side effects to the Player.
    bool expanded_inven_slot   = false;
    bool expanded_pyoguk_slot  = false;
    bool expanded_mugong_slot  = false;
    bool expanded_character_slot = false;
};

// 1:1 with legacy CShopItemManager::CalcShopItemOption( DWORD wIdx,
// BOOL bAdd, DWORD Param ). The legacy function also read m_pPlayer
// (Pointer) and ITEMMGR (singleton); both are replaced by the explicit
// info / env arguments. The legacy m_ProtectItemIdx is returned in the
// side-effects struct instead of being mutated through a pointer.
//
// The function mutates `stats` in place. Behavior matches legacy:
//   - wIdx == 0 -> InvalidIcon (no mutation).
//   - null info -> status reflects ItemInfoMissing (no mutation; legacy
//     would have ASSERTMSG'd and early-returned FALSE on non-special wIdx).
//   - The +/- accumulator is `calc = bAdd ? 1 : -1` per branch.
//   - Charm / Herb clamp-to-zero uses literal 0 (legacy: if (X < 0) X = 0).
//   - Plustime fields use the env.event_rate_active gate.
CalcShopItemOptionStatus calc_shop_item_option(
    mxh::game::ShopItemOption& stats,
    std::uint32_t w_idx,
    bool b_add,
    std::uint32_t param,
    const CalcShopItemOptionInfo& info,
    const CalcShopItemOptionEnv& env,
    std::uint32_t current_protect_item_idx,
    CalcShopItemOptionSideEffects& out_side_effects) noexcept;

}  // namespace mxh::server
