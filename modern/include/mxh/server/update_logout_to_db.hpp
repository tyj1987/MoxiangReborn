// update_logout_to_db.hpp
//
// 1:1 port of legacy CShopItemManager::UpdateLogoutToDB() from
// [Server]Map/ShopItemManager.cpp:1195-1238. Splits the legacy monolith
// into a pure data plane (this header) + an orchestrator half (the legacy
// function calls ShopItemUpdatetimeToDB / g_DB.Query via g_pServerMsg).
//
// 1:1 invariants:
//   - Iterates over every row in the using-items table.
//   - Skips rows whose SellPrice != eShopItemUseParam_Playtime.
//   - For plustime items (ItemKind == eSHOP_ITEM_CHARM AND
//     MeleeAttackMin != 0), the env.event_rate_active gate decides
//     whether time decrements; legacy: gEventRate vs gEventRateFile.
//   - Clamps checktime to 30000 ms (legacy m_Checktime throttle).
//   - Decrements Remaintime by checktime; sets Remaintime=0 if it would
//     underflow.
//   - Returns the new Remaintime + the old LastCheckTime so the
//     orchestrator can dispatch ShopItemUpdatetimeToDB and update
//     LastCheckTime on the table.
//
// The legacy function uses three globals (gCurTime, gEventRate,
// gEventRateFile) and the ITEMMGR singleton. The modern port maps:
//   gCurTime            -> caller-provided `g_cur_time` parameter
//   gEventRate / File   -> CalcShopItemOptionEnv::event_rate_active
//   ITEMMGR             -> caller-provided info lookup

#pragma once

#include <cstdint>
#include <vector>

#include "mxh/server/calc_shop_item_option.hpp"  // for CalcShopItemOptionEnv

namespace mxh::server {

// Legacy eShopItemUseParam_Playtime discriminator from
// [CC]Header/CommonGameDefine.h. Inlined here so the data plane does
// not depend on the legacy header.
inline constexpr std::uint32_t LEGACY_SHOP_ITEM_USE_PARAM_PLAYTIME = 2u;

// The row-level info that CalcShopItemOption needs to consult when
// computing the plustime gate. The legacy function reads three fields
// off the legacy ITEM_INFO; the modern port factors them down.
struct UpdateLogoutToDBInfo {
    std::uint32_t sell_price       = 0;  // eShopItemUseParam_*
    std::uint16_t item_kind        = 0;  // eSHOP_ITEM_*
    std::uint16_t melee_attack_min = 0;  // plustime gate discriminator
};

// Per-row decision the data plane returns. The orchestrator applies
// the new Remaintime + LastCheckTime to the table and dispatches the
// DB write.
struct UpdateLogoutRowDecision {
    enum class Action : std::uint8_t {
        Persist = 0,  // Update Remaintime and write to DB (default path).
        Skip    = 1,  // Update LastCheckTime only, leave Remaintime alone.
                       // Legacy: plustime item AND event rate ACTIVE.
        Drop    = 2,  // Row does not need a DB write (no PLAYTIME / remtime==0).
    };
    Action         action          = Action::Drop;
    std::uint32_t new_remaintime  = 0;  // valid when Action == Persist
    std::uint32_t new_last_check  = 0;  // updates LastCheckTime on hit
};

// 1:1 with the legacy CShopItemManager::UpdateLogoutToDB per-row
// behavior. The orchestrator iterates over the using-items table and
// calls this once per row.
//
// Legacy pseudo-code for each row:
//   if ItemInfo == null -> continue (drop)
//   if SellPrice != Playtime -> continue (drop)
//   if (ItemKind == Charm && MeleeAttackMin):
//       if (Remaintime && event_rate[MeleeAttackMin] != event_rate_file[MeleeAttackMin]):
//           LastCheckTime = gCurTime; continue (skip)
//       else if (Remaintime == 0): continue (skip)
//   checktime = gCurTime - LastCheckTime; clamp to 30000.
//   if (checktime >= Remaintime) Remaintime = 0;
//   else Remaintime -= checktime;
//   ShopItemUpdatetimeToDB(PlayerId, wIconIdx, Remaintime);
UpdateLogoutRowDecision update_logout_to_db_decision(
    std::uint32_t current_remaintime,
    std::uint32_t last_check_time,
    std::uint32_t g_cur_time,
    const UpdateLogoutToDBInfo& info,
    const CalcShopItemOptionEnv& env) noexcept;

}  // namespace mxh::server
