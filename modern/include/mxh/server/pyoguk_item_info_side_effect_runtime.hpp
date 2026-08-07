// pyoguk_item_info_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// pyoguk_item_info_side_effect_plan(). The data plane returns an
// empty plan (no player / hack NPC / already loading) or a 2-step
// chain (SetGotWarehouseItems -> FirePyogukDbQuery); this header
// walks the plan and dispatches each entry to a virtual
// PyogukItemInfoSideEffectSink.
//
// 1:1 invariants (1:1 with legacy CItemManager::
// MP_ITEM_PYOGUK_ITEM_INFO_SYN from
// [Server]Map/ItemManager.cpp:4922-4940):
//   - FindUser returns null: handler returns (empty plan).
//   - CheckHackNpc returns false: handler returns (empty plan,
//     silent drop).
//   - IsGotWarehouseItems() == TRUE: handler returns (empty plan,
//     dedup -- another load is already in progress).
//   - All pass: (1) pPlayer->SetGotWarehouseItems(TRUE), then
//     (2) PyogukItemOptionInfo (DB query), in legacy order.
//   - The handler sends NO ACK/NACK to the client; the data arrives
//     later via MP_ITEM_PYOGUKITEM_INFO broadcasts from the DB
//     callback.
//
// Pattern mirrors shop_item_info_side_effect_runtime.hpp (D4.64) and
// the rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/pyoguk_item_info_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the PyogukItemInfo side-effect chain.
class PyogukItemInfoSideEffectSink {
public:
    virtual ~PyogukItemInfoSideEffectSink() = default;

    // Legacy: pPlayer->SetGotWarehouseItems(TRUE) -- marks the player
    // as having a warehouse-items load in progress (dedup gate).
    virtual void set_got_warehouse_items(std::uint32_t object_id) = 0;

    // Legacy: PyogukItemOptionInfo(...) -- fires the DB query that
    // loads the warehouse (pyoguk) item info.
    virtual void fire_pyoguk_db_query(std::uint32_t object_id,
                                      std::uint32_t user_id,
                                      std::uint32_t start_db_idx) = 0;
};

struct PyogukItemInfoRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t marks_loading   = 0;
    std::size_t db_queries      = 0;
    bool mark_loading_flag_consumed = false;
    bool trigger_db_flag_consumed   = false;
};

// Runtime: walks the plan and dispatches each entry in legacy order.
inline PyogukItemInfoRuntimeOutcome apply_pyoguk_item_info_side_effects(
    const PyogukItemInfoSideEffectPlan& plan,
    PyogukItemInfoSideEffectSink& sink) {
    PyogukItemInfoRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case PyogukItemInfoSideEffectKind::SetGotWarehouseItems:
            sink.set_got_warehouse_items(effect.object_id);
            ++out.marks_loading;
            ++out.effects_applied;
            break;
        case PyogukItemInfoSideEffectKind::FirePyogukDbQuery:
            sink.fire_pyoguk_db_query(effect.object_id,
                                      effect.user_id,
                                      effect.start_db_idx);
            ++out.db_queries;
            ++out.effects_applied;
            break;
        }
    }
    out.mark_loading_flag_consumed = plan.mark_loading;
    out.trigger_db_flag_consumed = plan.trigger_db;
    return out;
}

}  // namespace mxh::server
