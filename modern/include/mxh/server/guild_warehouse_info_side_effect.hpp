// 1:1 side-effect-dispatcher port of
// CItemManager::MP_ITEM_GUILD_WAREHOUSE_INFO_SYN from legacy
// [Server]Map/ItemManager.cpp:4874-4882.
//
// The legacy handler requests the player's guild warehouse info.
// The flow is:
//   1. FindUser(pmsg->dwObjectID) -> pPlayer (return if null).
//   2. GUILDMGR->GuildWarehouseInfo(pPlayer, pmsg->bData) - DB
//      query for the guild warehouse data.
//
// The handler does NOT send any ACK/NACK to the client; the data
// arrives later via MP_ITEM_GUILD_WAREHOUSE broadcasts from the
// guild manager.

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

// 1:1 with legacy [CC]Header/Protocol.h MP_ITEM_GUILD_WAREHOUSE_INFO_SYN
// (single protocol code, no ACK/NACK pair on the SYN path).
inline constexpr std::uint8_t LEGACY_MP_ITEM_GUILD_WAREHOUSE_INFO_SYN = 80u;

enum class GuildWarehouseInfoOutcome : std::uint8_t {
    Triggered = 0,  // legacy: player found, GUILDMGR->GuildWarehouseInfo fired
    NoPlayer  = 1,  // legacy: FindUser returned null
};

struct GuildWarehouseInfoValidationInput final {
    bool player_found = false;
};

inline GuildWarehouseInfoOutcome classify_guild_warehouse_info_outcome(
    const GuildWarehouseInfoValidationInput& in) noexcept {
    if (!in.player_found) {
        return GuildWarehouseInfoOutcome::NoPlayer;
    }
    return GuildWarehouseInfoOutcome::Triggered;
}

enum class GuildWarehouseInfoSideEffectKind : std::uint8_t {
    FireGuildWarehouseDbQuery = 0,  // legacy GUILDMGR->GuildWarehouseInfo
};

struct GuildWarehouseInfoSideEffect final {
    GuildWarehouseInfoSideEffectKind kind =
        GuildWarehouseInfoSideEffectKind::FireGuildWarehouseDbQuery;
    std::uint32_t object_id = 0;  // legacy pPlayer (object id)
    std::uint8_t request_type = 0;  // legacy pmsg->bData
};

struct GuildWarehouseInfoSideEffectPlan final {
    std::vector<GuildWarehouseInfoSideEffect> effects;
    bool trigger_db = false;
};

inline GuildWarehouseInfoSideEffectPlan guild_warehouse_info_side_effect_plan(
    const GuildWarehouseInfoValidationInput& in,
    std::uint32_t object_id,
    std::uint8_t request_type) {
    GuildWarehouseInfoSideEffectPlan plan;
    const GuildWarehouseInfoOutcome outcome =
        classify_guild_warehouse_info_outcome(in);
    if (outcome != GuildWarehouseInfoOutcome::Triggered) {
        return plan;
    }
    plan.trigger_db = true;
    plan.effects.reserve(1u);
    GuildWarehouseInfoSideEffect db{};
    db.kind = GuildWarehouseInfoSideEffectKind::FireGuildWarehouseDbQuery;
    db.object_id = object_id;
    db.request_type = request_type;
    plan.effects.push_back(db);
    return plan;
}

}  // namespace mxh::server
