// 1:1 side-effect-dispatcher port of
// CItemManager::MP_ITEM_GUILD_WAREHOUSE_LEAVE from legacy
// [Server]Map/ItemManager.cpp:4910-4919.
//
// The legacy handler removes the player from the guild warehouse
// state when they leave the warehouse UI. The flow is:
//   1. FindUser(pmsg->dwObjectID) -> pPlayer (return if null).
//   2. GUILDMGR->LeaveWareHouse(pPlayer, pmsg->bData) - mark
//      player as having left the warehouse.
//
// The handler does NOT send any ACK/NACK to the client; the state
// is purely server-side.

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

// 1:1 with legacy [CC]Header/Protocol.h MP_ITEM_GUILD_WAREHOUSE_LEAVE
// (single protocol code, no ACK/NACK pair).
inline constexpr std::uint8_t LEGACY_MP_ITEM_GUILD_WAREHOUSE_LEAVE = 81u;

enum class GuildWarehouseLeaveOutcome : std::uint8_t {
    Left      = 0,  // legacy: player found, GUILDMGR->LeaveWareHouse fired
    NoPlayer  = 1,  // legacy: FindUser returned null
};

struct GuildWarehouseLeaveValidationInput final {
    bool player_found = false;
};

inline GuildWarehouseLeaveOutcome classify_guild_warehouse_leave_outcome(
    const GuildWarehouseLeaveValidationInput& in) noexcept {
    if (!in.player_found) {
        return GuildWarehouseLeaveOutcome::NoPlayer;
    }
    return GuildWarehouseLeaveOutcome::Left;
}

enum class GuildWarehouseLeaveSideEffectKind : std::uint8_t {
    FireGuildLeaveWarehouse = 0,  // legacy GUILDMGR->LeaveWareHouse
};

struct GuildWarehouseLeaveSideEffect final {
    GuildWarehouseLeaveSideEffectKind kind =
        GuildWarehouseLeaveSideEffectKind::FireGuildLeaveWarehouse;
    std::uint32_t object_id = 0;  // legacy pPlayer (object id)
    std::uint8_t request_type = 0;  // legacy pmsg->bData
};

struct GuildWarehouseLeaveSideEffectPlan final {
    std::vector<GuildWarehouseLeaveSideEffect> effects;
    bool fired = false;
};

inline GuildWarehouseLeaveSideEffectPlan guild_warehouse_leave_side_effect_plan(
    const GuildWarehouseLeaveValidationInput& in,
    std::uint32_t object_id,
    std::uint8_t request_type) {
    GuildWarehouseLeaveSideEffectPlan plan;
    const GuildWarehouseLeaveOutcome outcome =
        classify_guild_warehouse_leave_outcome(in);
    if (outcome != GuildWarehouseLeaveOutcome::Left) {
        return plan;
    }
    plan.fired = true;
    plan.effects.reserve(1u);
    GuildWarehouseLeaveSideEffect eff{};
    eff.kind = GuildWarehouseLeaveSideEffectKind::FireGuildLeaveWarehouse;
    eff.object_id = object_id;
    eff.request_type = request_type;
    plan.effects.push_back(eff);
    return plan;
}

}  // namespace mxh::server
