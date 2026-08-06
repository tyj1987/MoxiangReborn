// playtime_decrement.hpp
//
// 1:1 port of the playtime decrement inside the legacy
// CShopItemManager::CheckEndTime() / UpdateLogoutToDB() PLAYTIME
// branch from [Server]Map/ShopItemManager.cpp. The legacy code:
//
//   DWORD checktime = gCurTime - pShopItem->LastCheckTime;
//   if( checktime > 30000 ) checktime = 30000;
//   if( checktime >= pShopItem->ShopItem.Remaintime )
//       pShopItem->ShopItem.Remaintime = 0;
//   else
//       pShopItem->ShopItem.Remaintime -= checktime;
//
// Pure data plane: takes the inputs (current_remaintime,
// last_check_time, current_time) and returns the new
// remaintime + the elapsed-and-clamped checktime. The
// orchestrator applies the new value to the table and dispatches
// ShopItemUpdatetimeToDB.
//
// 1:1 invariants:
//   - elapsed = current_time - last_check_time (signed-ish DWORD
//     underflow: if last_check_time > current_time the result is
//     a huge positive number; legacy uses DWORD arithmetic so the
//     behaviour is preserved).
//   - checktime = clamp(elapsed, 0, 30000). Legacy: if( checktime
//     > 30000 ) checktime = 30000; (no lower bound on
//     underflow).
//   - new_remaintime = (checktime >= current_remaintime) ? 0 :
//     current_remaintime - checktime.
//   - If current_remaintime == 0 the legacy code calls
//     ShopItemUpdatetimeToDB with 0 (the loop continues) but
//     legacy UpdateLogoutToDB drops zero-remtime rows from the
//     DB write. We mirror the legacy CheckEndTime semantics:
//     always return the result; the orchestrator decides whether
//     to drop the row.

#pragma once

#include <cstdint>

namespace mxh::server {

// PLAYTIME throttle from legacy CheckEndTime / UpdateLogoutToDB.
// The checktime used to decrement Remaintime is clamped to 30000
// ms (30 seconds) so a long stall (lag spike, server pause) does
// not jump Remaintime by an unbounded amount.
inline constexpr std::uint32_t SHOP_ITEM_PLAYTIME_CHECKTIME_MAX_MS = 30000u;

struct PlaytimeDecrementResult {
    std::uint32_t new_remaintime      = 0;
    std::uint32_t elapsed_clamped_ms  = 0;  // 0..SHOP_ITEM_PLAYTIME_CHECKTIME_MAX_MS
};

// 1:1 with legacy CShopItemManager::CheckEndTime PLAYTIME
// decrement (also the same arithmetic in UpdateLogoutToDB).
inline PlaytimeDecrementResult playtime_decrement(
    std::uint32_t current_remaintime,
    std::uint32_t last_check_time,
    std::uint32_t current_time) noexcept {
    PlaytimeDecrementResult out{};
    std::uint32_t checktime = current_time - last_check_time;
    if (checktime > SHOP_ITEM_PLAYTIME_CHECKTIME_MAX_MS) {
        checktime = SHOP_ITEM_PLAYTIME_CHECKTIME_MAX_MS;
    }
    out.elapsed_clamped_ms = checktime;
    if (checktime >= current_remaintime) {
        out.new_remaintime = 0;
    } else {
        out.new_remaintime = current_remaintime - checktime;
    }
    return out;
}

}  // namespace mxh::server
