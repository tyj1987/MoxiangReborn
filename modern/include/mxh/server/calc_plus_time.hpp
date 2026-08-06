// calc_plus_time.hpp
//
// 1:1 port of legacy CShopItemManager::CalcPlusTime(DWORD dwEventIdx,
// DWORD dwType) from [Server]Map/ShopItemManager.cpp. Splits the
// legacy function (a per-row dispatch over the using-items table)
// into:
//   1. Pure data plane (this header): given one using-item row
//      (wIconIdx, ItemKind, MeleeAttackMin, Remaintime), the
//      (dwEventIdx, dwType) inputs, and an env wrapper for
//      gEventRate / gEventRateFile, return the per-row
//      decision: whether to call CalcShopItemOption, with which
//      bAdd flag, whether to update LastCheckTime, and whether
//      to break out of the loop (legacy return).
//   2. Orchestrator half: the legacy function calls
//      CalcShopItemOption(...) (already ported in D4.25) and
//      mutates pShopItem-LastCheckTime. The orchestrator
//      applies those side effects based on the per-row decision.
//
// 1:1 invariants:
//   - Skip if ItemKind != eSHOP_ITEM_CHARM (258).
//   - dwType == 0 (default): if Remaintime AND env event_rate
//     [MeleeAttackMin] != event_rate_file[MeleeAttackMin], call
//     CalcShopItemOption(FALSE). Loop continues.
//   - dwType == MP_CHEAT_PLUSTIME_ON: if dwEventIdx == MeleeAttackMin
//     AND Remaintime, call CalcShopItemOption(FALSE). Loop stops.
//   - dwType == MP_CHEAT_PLUSTIME_OFF: if dwEventIdx == MeleeAttackMin
//     AND Remaintime, set LastCheckTime = current_time_ms, call
//     CalcShopItemOption(TRUE). Loop stops.
//   - dwType == MP_CHEAT_PLUSTIME_ALLOFF: if Remaintime AND env
//     event_rate[MeleeAttackMin] != event_rate_file[MeleeAttackMin],
//     set LastCheckTime = current_time_ms, call CalcShopItemOption
//     (TRUE). Loop continues.
//
// dwType == 0 is the legacy default-init sentinel (case 0).

#pragma once

#include <cstdint>

namespace mxh::server {

// 1:1 with eSHOP_ITEM_CHARM.
inline constexpr std::uint16_t LEGACY_SHOP_ITEM_CHARM = 258u;

// 1:1 with [CC]Header/Protocol.h MP_CHEAT_PLUSTIME_* enum members.
// dwType == 0 is the legacy default sentinel.
inline constexpr std::uint32_t LEGACY_MP_CHEAT_PLUSTIME_ALLOFF_VAL = 0u;
// The actual enum values follow the legacy cheat-on/off order:
// MP_CHEAT_PLUSTIME_ALLOFF = 0 (default), then ON, then OFF.
// The MP_CHEAT_PLUSTIME_ON/OFF values are runtime protocol codes;
// the legacy code only checks the dwType discriminator against
// 3 cheat types + the default sentinel. The data plane uses
// symbolic constants to make the dispatch table readable.

// Per-row input that CalcPlusTime reads off the using-item row.
// The data plane is per-row; the orchestrator loops over the
// table and calls calc_plus_time_row_decision once per row.
struct CalcPlusTimeRowInput {
    std::uint16_t w_icon_idx          = 0;
    std::uint16_t item_kind           = 0;  // eSHOP_ITEM_CHARM gate
    std::uint16_t melee_attack_min    = 0;  // plustime event id
    std::uint32_t remaintime          = 0;
};

// Per-row decision the orchestrator applies.
//   should_calc: true => call CalcShopItemOption(icon_idx, b_add).
//   b_add: passed to CalcShopItemOption (1:1 with legacy bAdd).
//   update_last_check: true => set LastCheckTime = env.current_time_ms.
//   stop_iteration: true => break out of the legacy while(...) loop.
struct CalcPlusTimeRowDecision {
    bool should_calc         = false;
    bool b_add               = false;
    bool update_last_check   = false;
    bool stop_iteration      = false;
};

// Env wrapper for the legacy runtime globals. Mirrors the
// CalcShopItemOptionEnv pattern: the data plane does not depend on
// gEventRate / gEventRateFile / gCurTime globals.
class CalcPlusTimeEnv {
public:
    virtual ~CalcPlusTimeEnv() = default;
    // 1:1 with legacy gEventRate[rate_id] != gEventRateFile[rate_id].
    // Default implementation returns false (legacy: the two tables
    // match by default; the plustime gate is only active when the
    // rate has been mutated).
    virtual bool event_rate_active(std::uint16_t rate_id) const noexcept {
        (void)rate_id;
        return false;
    }
    // 1:1 with legacy gCurTime. Used only by the OFF/ALLOFF branches
    // to write LastCheckTime.
    virtual std::uint32_t current_time_ms() const noexcept { return 0u; }
};

// Legacy dwType values (the cheat enum + default sentinel).
// The data plane compares dwType against these constants; the
// cheat names are reused so the dispatch reads like the legacy code.
enum class CalcPlusTimeType : std::uint32_t {
    DefaultSentinel = 0,                  // legacy case 0
    PlustimeAllOff  = 1,                  // legacy MP_CHEAT_PLUSTIME_ALLOFF (mapped)
    PlustimeOn      = 2,                  // legacy MP_CHEAT_PLUSTIME_ON (mapped)
    PlustimeOff     = 3,                  // legacy MP_CHEAT_PLUSTIME_OFF (mapped)
};

// 1:1 with legacy CShopItemManager::CalcPlusTime per-row body.
// Pure data plane: takes the (dwEventIdx, dwType) inputs + the
// row snapshot + the env wrapper and returns the decision struct.
// The orchestrator iterates over the using-items table, calls this
// once per row, and applies the side effects based on the decision.
//
// dw_type raw values are mapped through the CalcPlusTimeType enum:
//   0 -> DefaultSentinel
//   1 -> PlustimeAllOff (legacy MP_CHEAT_PLUSTIME_ALLOFF)
//   2 -> PlustimeOn     (legacy MP_CHEAT_PLUSTIME_ON)
//   3 -> PlustimeOff    (legacy MP_CHEAT_PLUSTIME_OFF)
// Other values are treated as DefaultSentinel (legacy: switch
// fall-through to the case 0 default branch, which means no-op).
inline CalcPlusTimeRowDecision calc_plus_time_row_decision(
    std::uint32_t dw_event_idx,
    CalcPlusTimeType type,
    const CalcPlusTimeRowInput& row,
    const CalcPlusTimeEnv& env) noexcept {
    CalcPlusTimeRowDecision out{};
    // Legacy: if (pItem-ItemKind == eSHOP_ITEM_CHARM) { switch(dwType) { ... } }
    if (row.item_kind != LEGACY_SHOP_ITEM_CHARM) {
        return out;  // skip: no decision
    }
    switch (type) {
    case CalcPlusTimeType::DefaultSentinel:
        // Legacy case 0:
        //   if (pShopItem->ShopItem.Remaintime)
        //   if (gEventRate[pItem->MeleeAttackMin] != gEventRateFile[pItem->MeleeAttackMin])
        //       CalcShopItemOption(pItem->ItemIdx, FALSE);
        if (row.remaintime != 0 &&
            env.event_rate_active(row.melee_attack_min)) {
            out.should_calc = true;
            out.b_add = false;
        }
        break;
    case CalcPlusTimeType::PlustimeOn:
        // Legacy MP_CHEAT_PLUSTIME_ON:
        //   if (dwEventIdx == pItem->MeleeAttackMin && pShopItem->ShopItem.Remaintime)
        //       CalcShopItemOption(pItem->ItemIdx, FALSE);
        //       return;
        if (dw_event_idx == row.melee_attack_min && row.remaintime != 0) {
            out.should_calc = true;
            out.b_add = false;
            out.stop_iteration = true;
        }
        break;
    case CalcPlusTimeType::PlustimeOff:
        // Legacy MP_CHEAT_PLUSTIME_OFF:
        //   if (dwEventIdx == pItem->MeleeAttackMin && pShopItem->ShopItem.Remaintime)
        //       pShopItem->LastCheckTime = gCurTime;
        //       CalcShopItemOption(pItem->ItemIdx, TRUE);
        //       return;
        if (dw_event_idx == row.melee_attack_min && row.remaintime != 0) {
            out.update_last_check = true;
            out.should_calc = true;
            out.b_add = true;
            out.stop_iteration = true;
        }
        break;
    case CalcPlusTimeType::PlustimeAllOff:
        // Legacy MP_CHEAT_PLUSTIME_ALLOFF:
        //   if (pShopItem->ShopItem.Remaintime)
        //   if (gEventRate[pItem->MeleeAttackMin] != gEventRateFile[pItem->MeleeAttackMin])
        //       pShopItem->LastCheckTime = gCurTime;
        //       CalcShopItemOption(pItem->ItemIdx, TRUE);
        if (row.remaintime != 0 &&
            env.event_rate_active(row.melee_attack_min)) {
            out.update_last_check = true;
            out.should_calc = true;
            out.b_add = true;
        }
        break;
    }
    return out;
}

}  // namespace mxh::server
