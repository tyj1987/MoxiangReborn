// 1:1 side-effect-dispatcher port of
// CItemManager::MP_ITEM_GUILD_MOVE_SYN from legacy
// [Server]Map/ItemManager.cpp:4883-4908.
//
// The legacy handler moves an item into/out of the guild warehouse.
// The flow is:
//   1. FindUser(pmsg->dwObjectID) -> pPlayer (return if null).
//   2. CanMovetoGuildWare(pPlayer, FromPos, ToPos) (return with
//      NACK=4 if false - the legacy code uses ASSERT(0) in debug
//      builds but always returns on failure).
//   3. rt = MoveItem(pPlayer, FromIdx, FromPos, ToIdx, ToPos)
//      a. rt == EI_TRUE: send MP_ITEM_GUILD_MOVE_ACK with the
//         original pmsg fields.
//      b. rt != EI_TRUE: send MP_ITEM_GUILD_MOVE_NACK with rt
//         (a per-row error code).

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

// 1:1 with legacy [CC]Header/Protocol.h MP_ITEM_GUILD_MOVE_ACK/_NACK.
inline constexpr std::uint8_t LEGACY_MP_ITEM_GUILD_MOVE_ACK  = 82u;
inline constexpr std::uint8_t LEGACY_MP_ITEM_GUILD_MOVE_NACK = 83u;

// 1:1 with legacy error code 4 returned by CanMovetoGuildWare.
inline constexpr int LEGACY_GUILD_MOVE_ERR_NOT_MOVEABLE = 4;

enum class GuildMoveOutcome : std::uint8_t {
    Success    = 0,  // legacy: rt == EI_TRUE
    Failure    = 1,  // legacy: rt != EI_TRUE
    NotMoveable = 2, // legacy: CanMovetoGuildWare == false
    NoPlayer   = 3,  // legacy: FindUser returned null
};

struct GuildMoveValidationInput final {
    bool player_found = false;
    bool can_move = false;
    int  move_rt = 0;
};

inline GuildMoveOutcome classify_guild_move_outcome(
    const GuildMoveValidationInput& in) noexcept {
    if (!in.player_found) {
        return GuildMoveOutcome::NoPlayer;
    }
    if (!in.can_move) {
        return GuildMoveOutcome::NotMoveable;
    }
    if (in.move_rt == 0) {  // legacy EI_TRUE
        return GuildMoveOutcome::Success;
    }
    return GuildMoveOutcome::Failure;
}

enum class GuildMoveSideEffectKind : std::uint8_t {
    BroadcastGuildMoveAck  = 0,  // legacy SendAckMsg(GUILD_MOVE_ACK)
    BroadcastGuildMoveNack = 1,  // legacy SendGuildErrorMsg(NACK, err)
};

struct GuildMoveSideEffect final {
    GuildMoveSideEffectKind kind =
        GuildMoveSideEffectKind::BroadcastGuildMoveAck;
    std::uint16_t from_pos = 0;     // legacy pmsg->FromPos
    std::uint16_t to_pos = 0;       // legacy pmsg->ToPos
    std::uint16_t from_item_idx = 0; // legacy pmsg->wFromItemIdx
    std::uint16_t to_item_idx = 0;   // legacy pmsg->wToItemIdx
    int error_code = 0;              // legacy NACK payload (rt or 4)
    int original_rt = 0;
};

struct GuildMoveSideEffectPlan final {
    std::vector<GuildMoveSideEffect> effects;
    bool send_ack = false;
    bool send_nack = false;
    int error_code = 0;
};

inline GuildMoveSideEffectPlan guild_move_side_effect_plan(
    const GuildMoveValidationInput& in,
    std::uint16_t from_pos,
    std::uint16_t to_pos,
    std::uint16_t from_item_idx,
    std::uint16_t to_item_idx) {
    GuildMoveSideEffectPlan plan;
    const GuildMoveOutcome outcome = classify_guild_move_outcome(in);
    plan.effects.reserve(1u);
    GuildMoveSideEffect eff{};
    eff.from_pos = from_pos;
    eff.to_pos = to_pos;
    eff.from_item_idx = from_item_idx;
    eff.to_item_idx = to_item_idx;
    eff.original_rt = in.move_rt;
    if (outcome == GuildMoveOutcome::Success) {
        plan.send_ack = true;
        eff.kind = GuildMoveSideEffectKind::BroadcastGuildMoveAck;
        plan.effects.push_back(eff);
        return plan;
    }
    if (outcome == GuildMoveOutcome::NotMoveable) {
        plan.send_nack = true;
        eff.kind = GuildMoveSideEffectKind::BroadcastGuildMoveNack;
        eff.error_code = LEGACY_GUILD_MOVE_ERR_NOT_MOVEABLE;
        plan.error_code = LEGACY_GUILD_MOVE_ERR_NOT_MOVEABLE;
        plan.effects.push_back(eff);
        return plan;
    }
    if (outcome == GuildMoveOutcome::Failure) {
        plan.send_nack = true;
        eff.kind = GuildMoveSideEffectKind::BroadcastGuildMoveNack;
        eff.error_code = in.move_rt;
        plan.error_code = in.move_rt;
        plan.effects.push_back(eff);
        return plan;
    }
    return plan;
}

}  // namespace mxh::server
