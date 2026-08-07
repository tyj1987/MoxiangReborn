// calc_plus_time_runtime.hpp
//
// Runtime orchestrator for the legacy CShopItemManager::CalcPlusTime
// function. The data plane in calc_plus_time.hpp returns a per-row
// decision; this header turns that into side effects on a real
// ShopItemManager + invokes calc_shop_item_option for should_calc rows.
//
// 1:1 invariants:
//   - Iterates m_UsingItemTable.SetPositionHead + GetData loop
//     equivalent (unordered_map iteration; stable enough for tests).
//   - Skips rows whose ItemKind lookup fails or != eSHOP_ITEM_CHARM
//     (data plane handles this via the row input).
//   - For should_calc rows, invokes the calc callback (production
//     wires this to a calc_shop_item_option call with the legacy
//     item info + stats + option_env).
//   - For update_last_check rows, sets entry->Data.LastCheckTime
//     via find_using_item_by_icon_idx_mutable (1:1 with legacy
//     pShopItem->LastCheckTime = gCurTime).
//   - For stop_iteration rows, breaks out of the loop (1:1 with
//     legacy return statement inside the switch.

#pragma once

#include <cstddef>
#include <cstdint>

#include <mxh/server/calc_plus_time.hpp>
#include <mxh/server/calc_shop_item_option.hpp>
#include <mxh/server/shop_item_manager.hpp>

// LEGACY_SHOP_ITEM_CHARM is defined exactly once in
// legacy_shop_item_kind.hpp; the three headers above all include it.


namespace mxh::server {

// Item info provider for plustime iteration. The legacy code calls
// ITEMMGR->GetItemInfo(wIconIdx) to obtain the CalcShopItemOptionInfo
// inputs. Production wires this to the legacy ITEMMGR singleton;
// tests wire it to a stub that returns predetermined values.
class PlustimeItemInfoProvider {
public:
    virtual ~PlustimeItemInfoProvider() = default;
    virtual bool lookup(
        std::uint16_t icon_idx,
        CalcShopItemOptionInfo& out_info) const = 0;
};

// Calc callback: invoked once per row whose decision.should_calc is
// true. The runtime does not own the stats / option_env / side
// effects plumbing -- the callback wraps calc_shop_item_option and
// returns the legacy outcome via side_effects.
class PlustimeCalcCallback {
public:
    virtual ~PlustimeCalcCallback() = default;
    virtual void on_calc(
        std::uint32_t item_idx,
        bool b_add) = 0;
};

// Counters / flags returned by the orchestrator so tests can assert
// which rows triggered which side effects.
struct CalcPlusTimeOutcome {
    std::size_t rows_visited = 0;
    std::size_t rows_skipped_no_item = 0;
    std::size_t rows_skipped_non_charm = 0;
    std::size_t calc_invocations = 0;
    std::size_t last_check_updates = 0;
    bool stopped_early = false;
};

// Runtime: walks the manager.using_items() table, builds a
// CalcPlusTimeRowInput per row via the info provider, calls the data
// plane, and applies side effects. Stops early on stop_iteration.
inline CalcPlusTimeOutcome apply_calc_plus_time(
    ShopItemManager& mgr,
    std::uint32_t dw_event_idx,
    CalcPlusTimeType type,
    const CalcPlusTimeEnv& plustime_env,
    const PlustimeItemInfoProvider& info_provider,
    PlustimeCalcCallback& calc_callback) {
    CalcPlusTimeOutcome out;
    for (const auto& kv : mgr.using_items()) {
        const auto& entry = kv.second;
        ++out.rows_visited;
        const std::uint16_t icon_idx =
            entry.Data.ShopItem.ItemBase.wIconIdx;
        CalcShopItemOptionInfo info{};
        if (!info_provider.lookup(icon_idx, info)) {
            ++out.rows_skipped_no_item;
            continue;
        }
        CalcPlusTimeRowInput row_input;
        row_input.w_icon_idx = icon_idx;
        row_input.item_kind = info.ItemKind;
        row_input.melee_attack_min = info.MeleeAttackMin;
        row_input.remaintime = entry.Data.ShopItem.Remaintime;
        auto decision = calc_plus_time_row_decision(
            dw_event_idx, type, row_input, plustime_env);
        if (decision.should_calc) {
            calc_callback.on_calc(info.ItemIdx, decision.b_add);
            ++out.calc_invocations;
        }
        if (decision.update_last_check) {
            auto* mutable_entry =
                mgr.find_using_item_by_icon_idx_mutable(icon_idx);
            if (mutable_entry != nullptr) {
                mutable_entry->Data.LastCheckTime =
                    plustime_env.current_time_ms();
                ++out.last_check_updates;
            }
        }
        if (decision.stop_iteration) {
            out.stopped_early = true;
            break;
        }
    }
    return out;
}

}  // namespace mxh::server