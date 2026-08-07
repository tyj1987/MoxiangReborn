// guild_warehouse_info_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// guild_warehouse_info_side_effect_plan(). The data plane returns an
// empty plan (no player) or a single FireGuildWarehouseDbQuery
// entry; this header walks the plan and dispatches the entry to a
// virtual GuildWarehouseInfoSideEffectSink.
//
// 1:1 invariants (1:1 with legacy CItemManager::
// MP_ITEM_GUILD_WAREHOUSE_INFO_SYN from
// [Server]Map/ItemManager.cpp:4874-4882):
//   - FindUser returns null: handler returns (empty plan).
//   - Player found: handler fires GUILDMGR->GuildWarehouseInfo(
//     pPlayer, pmsg->bData) -- the DB query for the guild warehouse
//     data.
//   - The handler sends NO ACK/NACK to the client; the data arrives
//     later via MP_ITEM_GUILD_WAREHOUSE broadcasts from the guild
//     manager.
//
// Pattern mirrors pet_inven_info_side_effect_runtime.hpp (D4.63) and
// the rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/guild_warehouse_info_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the GuildWarehouseInfo side-effect chain.
class GuildWarehouseInfoSideEffectSink {
public:
    virtual ~GuildWarehouseInfoSideEffectSink() = default;

    // Legacy: GUILDMGR->GuildWarehouseInfo(pPlayer, request_type) --
    // fires the DB query for the guild warehouse data.
    virtual void fire_guild_warehouse_db_query(
        std::uint32_t object_id, std::uint8_t request_type) = 0;
};

struct GuildWarehouseInfoRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t db_queries      = 0;
    bool trigger_db_flag_consumed = false;
};

// Runtime: walks the plan and dispatches the single entry.
inline GuildWarehouseInfoRuntimeOutcome
apply_guild_warehouse_info_side_effects(
    const GuildWarehouseInfoSideEffectPlan& plan,
    GuildWarehouseInfoSideEffectSink& sink) {
    GuildWarehouseInfoRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case GuildWarehouseInfoSideEffectKind::FireGuildWarehouseDbQuery:
            sink.fire_guild_warehouse_db_query(
                effect.object_id, effect.request_type);
            ++out.db_queries;
            ++out.effects_applied;
            break;
        }
    }
    out.trigger_db_flag_consumed = plan.trigger_db;
    return out;
}

}  // namespace mxh::server
