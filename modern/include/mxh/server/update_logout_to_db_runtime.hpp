// update_logout_to_db_runtime.hpp
//
// Runtime orchestrator for CShopItemManager::UpdateLogoutToDB. The
// data plane in update_logout_to_db.hpp returns a per-row decision
// (Persist / Skip / Drop); this header turns that into side effects
// on a real ShopItemManager and dispatches the DB writes through a
// virtual SqlDispatch interface.
//
// 1:1 invariants (1:1 with legacy CShopItemManager::UpdateLogoutToDB):
//   - Iterates over the using-items table.
//   - Skips rows whose SellPrice != eShopItemUseParam_Playtime (Drop).
//   - For plustime items (ItemKind == Charm AND MeleeAttackMin != 0)
//     when env.event_rate_active(id) is FALSE: stamp LastCheckTime
//     and skip (Skip action).
//   - For all other rows: decrement Remaintime by checktime (clamped
//     to 30s), write the new Remaintime to the table, and dispatch
//     ShopItemUpdatetimeToDB (Persist action).
//
// Pattern mirrors calc_plus_time_runtime.hpp (D4.28): data plane in
// the matching data-plane header, runtime orchestrator also inline
// in this header, tests verify behavior through the public surface.

#pragma once

#include <cstdint>

#include <mxh/server/calc_shop_item_option.hpp>
#include <mxh/server/legacy_shop_item_kind.hpp>
#include <mxh/server/shop_item_manager.hpp>
#include <mxh/server/shop_item_update_sql.hpp>
#include <mxh/server/update_logout_to_db.hpp>

namespace mxh::server {

// Item-info provider for the using-items table. The legacy code calls
// ITEMMGR->GetItemInfo(dwItemIdx) to obtain SellPrice/ItemKind/
// MeleeAttackMin per row. Production wires this to the legacy ITEMMGR
// singleton; tests wire it to a stub.
class UpdateLogoutItemInfoProvider {
public:
    virtual ~UpdateLogoutItemInfoProvider() = default;
    // 1:1 with legacy ITEMMGR->GetItemInfo. Returns false on null info.
    virtual bool lookup(
        std::uint64_t item_idx,
        UpdateLogoutToDBInfo& out_info) const = 0;
};

// SQL dispatcher for the persist writes. The legacy code calls
// g_DB.Query(eQueryType_FreeQuery, txt) with the SQL string built by
// ShopItemUpdatetimeToDB. The modern port uses a virtual interface so
// the orchestrator stays testable without a real DB connection.
class UpdateLogoutSqlDispatch {
public:
    virtual ~UpdateLogoutSqlDispatch() = default;
    virtual void dispatch(std::uint32_t character_idx,
                          std::uint32_t item_idx,
                          std::uint32_t new_remaintime) = 0;
};

// Outcome counters / flags returned by the orchestrator.
struct UpdateLogoutToDBOutcome {
    std::size_t rows_visited        = 0;
    std::size_t rows_dropped        = 0;  // SellPrice != PLAYTIME / rem=0 / no info
    std::size_t rows_skipped        = 0;  // plustime + event_rate_active==false
    std::size_t rows_persisted      = 0;  // Remaintime decremented + SQL dispatched
    std::size_t last_check_updates  = 0;  // LastCheckTime mutated on row
    std::size_t remaintime_updates  = 0;  // Remaintime mutated on row
};

// Runtime: walks the manager.using_items() table, builds a
// UpdateLogoutToDBInfo per row via the info provider, calls the
// data plane, and applies side effects. Dispatches SQL via the
// dispatch interface for Persist rows.
inline UpdateLogoutToDBOutcome apply_update_logout_to_db(
    ShopItemManager& mgr,
    std::uint32_t character_idx,
    std::uint32_t g_cur_time,
    const UpdateLogoutItemInfoProvider& info_provider,
    const CalcShopItemOptionEnv& env,
    UpdateLogoutSqlDispatch& sql_dispatch) {
    UpdateLogoutToDBOutcome out;
    for (const auto& kv : mgr.using_items()) {
        const auto& entry = kv.second;
        ++out.rows_visited;
        UpdateLogoutToDBInfo info;
        if (!info_provider.lookup(entry.ItemIdx, info)) {
            ++out.rows_dropped;
            continue;
        }
        const auto decision = update_logout_to_db_decision(
            entry.Data.ShopItem.Remaintime,
            entry.Data.LastCheckTime,
            g_cur_time,
            info,
            env);
        switch (decision.action) {
        case UpdateLogoutRowDecision::Action::Drop:
            ++out.rows_dropped;
            break;
        case UpdateLogoutRowDecision::Action::Skip: {
            // Stamp LastCheckTime but leave Remaintime alone.
            auto* mutable_entry =
                mgr.find_using_item_by_icon_idx_mutable(
                    static_cast<std::uint16_t>(entry.ItemIdx));
            if (mutable_entry != nullptr) {
                mutable_entry->Data.LastCheckTime = decision.new_last_check;
                ++out.last_check_updates;
            }
            ++out.rows_skipped;
            break;
        }
        case UpdateLogoutRowDecision::Action::Persist: {
            // Decrement Remaintime + stamp LastCheckTime + dispatch SQL.
            auto* mutable_entry =
                mgr.find_using_item_by_icon_idx_mutable(
                    static_cast<std::uint16_t>(entry.ItemIdx));
            if (mutable_entry != nullptr) {
                mutable_entry->Data.ShopItem.Remaintime =
                    decision.new_remaintime;
                mutable_entry->Data.LastCheckTime = decision.new_last_check;
                ++out.remaintime_updates;
                ++out.last_check_updates;
            }
            sql_dispatch.dispatch(
                character_idx,
                static_cast<std::uint32_t>(entry.ItemIdx),
                decision.new_remaintime);
            ++out.rows_persisted;
            break;
        }
        }
    }
    return out;
}

}  // namespace mxh::server
