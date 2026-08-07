// guild_warehouse_leave_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// guild_warehouse_leave_side_effect_plan(). The data plane returns an
// empty plan (no player) or a single FireGuildLeaveWarehouse entry;
// this header walks the plan and dispatches the entry to a virtual
// GuildWarehouseLeaveSideEffectSink.
//
// 1:1 invariants (1:1 with legacy CItemManager::
// MP_ITEM_GUILD_WAREHOUSE_LEAVE from
// [Server]Map/ItemManager.cpp:4910-4919):
//   - FindUser returns null: handler returns (empty plan).
//   - Player found: handler fires GUILDMGR->LeaveWareHouse(pPlayer,
//     pmsg->bData) -- marks the player as having left the guild
//     warehouse UI.
//   - The handler sends NO ACK/NACK to the client; the state is
//     purely server-side.
//
// Pattern mirrors guild_warehouse_info_side_effect_runtime.hpp
// (D4.66) and the rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/guild_warehouse_leave_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the GuildWarehouseLeave side-effect chain.
class GuildWarehouseLeaveSideEffectSink {
public:
    virtual ~GuildWarehouseLeaveSideEffectSink() = default;

    // Legacy: GUILDMGR->LeaveWareHouse(pPlayer, request_type) -- marks
    // the player as having left the guild warehouse UI.
    virtual void fire_guild_leave_warehouse(
        std::uint32_t object_id, std::uint8_t request_type) = 0;
};

struct GuildWarehouseLeaveRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t fires           = 0;
    bool fired_flag_consumed    = false;
};

// Runtime: walks the plan and dispatches the single entry.
inline GuildWarehouseLeaveRuntimeOutcome
apply_guild_warehouse_leave_side_effects(
    const GuildWarehouseLeaveSideEffectPlan& plan,
    GuildWarehouseLeaveSideEffectSink& sink) {
    GuildWarehouseLeaveRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case GuildWarehouseLeaveSideEffectKind::FireGuildLeaveWarehouse:
            sink.fire_guild_leave_warehouse(
                effect.object_id, effect.request_type);
            ++out.fires;
            ++out.effects_applied;
            break;
        }
    }
    out.fired_flag_consumed = plan.fired;
    return out;
}

}  // namespace mxh::server
