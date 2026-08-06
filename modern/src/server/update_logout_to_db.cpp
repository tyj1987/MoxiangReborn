// update_logout_to_db.cpp
//
// Pure data plane implementation of CShopItemManager::UpdateLogoutToDB.
// See update_logout_to_db.hpp for the 1:1 invariants and the legacy-line
// citation.

#include "mxh/server/update_logout_to_db.hpp"

namespace mxh::server {

UpdateLogoutRowDecision update_logout_to_db_decision(
    std::uint32_t current_remaintime,
    std::uint32_t last_check_time,
    std::uint32_t g_cur_time,
    const UpdateLogoutToDBInfo& info,
    const CalcShopItemOptionEnv& env) noexcept {
    UpdateLogoutRowDecision out;
    out.new_remaintime = current_remaintime;
    out.new_last_check = g_cur_time;

    // Legacy: if SellPrice != Playtime -> continue (drop).
    if (info.sell_price != LEGACY_SHOP_ITEM_USE_PARAM_PLAYTIME) {
        out.action = UpdateLogoutRowDecision::Action::Drop;
        return out;
    }

    // Legacy plustime branch:
    //   if (ItemKind == Charm && MeleeAttackMin):
    //       if (Remaintime && gEventRate[id] != gEventRateFile[id]):
    //           LastCheckTime = gCurTime; continue (skip)
    //       else if (Remaintime == 0): continue (skip)
    if (info.item_kind == LEGACY_SHOP_ITEM_CHARM && info.melee_attack_min != 0) {
        if (current_remaintime != 0 &&
            !env.event_rate_active(info.melee_attack_min)) {
            // Legacy: env rate is INACTIVE -> stamp LastCheckTime and skip
            // the Remaintime update. The orchestrator should write the
            // updated LastCheckTime back to the table.
            out.action = UpdateLogoutRowDecision::Action::Skip;
            out.new_last_check = g_cur_time;
            out.new_remaintime = current_remaintime;
            return out;
        }
        if (current_remaintime == 0) {
            out.action = UpdateLogoutRowDecision::Action::Drop;
            return out;
        }
    }

    // Compute checktime = gCurTime - LastCheckTime, clamped to 30000 ms.
    std::uint32_t checktime = (g_cur_time > last_check_time)
                                  ? (g_cur_time - last_check_time)
                                  : 0u;
    constexpr std::uint32_t kCheckTimeCap = 30000u;
    if (checktime > kCheckTimeCap) {
        checktime = kCheckTimeCap;
    }

    // Decrement Remaintime by checktime; clamp to 0 on underflow.
    if (checktime >= current_remaintime) {
        out.new_remaintime = 0u;
    } else {
        out.new_remaintime = current_remaintime - checktime;
    }
    out.action = UpdateLogoutRowDecision::Action::Persist;
    return out;
}

}  // namespace mxh::server
