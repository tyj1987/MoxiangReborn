// guild_move_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// guild_move_side_effect_plan(). The data plane returns an empty plan
// (no player) or a single ACK/NACK entry; this header walks the plan
// and dispatches the entry to a virtual GuildMoveSideEffectSink.
//
// 1:1 invariants (1:1 with legacy CItemManager::MP_ITEM_GUILD_MOVE_SYN
// from [Server]Map/ItemManager.cpp:4883-4908):
//   - FindUser returns null: handler returns (empty plan).
//   - CanMovetoGuildWare returns false: handler sends
//     MP_ITEM_GUILD_MOVE_NACK with error code 4.
//   - MoveItem returns EI_TRUE (0): handler sends MP_ITEM_GUILD_MOVE_ACK
//     echoing the original pmsg fields.
//   - MoveItem returns non-zero: handler sends MP_ITEM_GUILD_MOVE_NACK
//     with the per-row return code.
//
// Pattern mirrors item_buy_side_effect_runtime.hpp (D4.51) and the
// rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/guild_move_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the GuildMove side-effect chain.
class GuildMoveSideEffectSink {
public:
    virtual ~GuildMoveSideEffectSink() = default;

    // Legacy: SendAckMsg(MP_ITEM_GUILD_MOVE_ACK) -- echoes the
    // original pmsg fields (from/to pos + item idx).
    virtual void broadcast_guild_move_ack(std::uint16_t from_pos,
                                          std::uint16_t to_pos,
                                          std::uint16_t from_item_idx,
                                          std::uint16_t to_item_idx,
                                          int original_rt) = 0;

    // Legacy: SendGuildErrorMsg(MP_ITEM_GUILD_MOVE_NACK, error_code) --
    // sends the per-row error (move rt) or the fixed not-moveable
    // code (4).
    virtual void broadcast_guild_move_nack(std::uint16_t from_pos,
                                           std::uint16_t to_pos,
                                           std::uint16_t from_item_idx,
                                           std::uint16_t to_item_idx,
                                           int original_rt,
                                           int error_code) = 0;
};

struct GuildMoveRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t acks_sent       = 0;
    std::size_t nacks_sent      = 0;
    bool ack_flag_consumed   = false;
    bool nack_flag_consumed  = false;
};

// Runtime: walks the plan and dispatches the single entry.
inline GuildMoveRuntimeOutcome apply_guild_move_side_effects(
    const GuildMoveSideEffectPlan& plan,
    GuildMoveSideEffectSink& sink) {
    GuildMoveRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case GuildMoveSideEffectKind::BroadcastGuildMoveAck:
            sink.broadcast_guild_move_ack(
                effect.from_pos, effect.to_pos,
                effect.from_item_idx, effect.to_item_idx,
                effect.original_rt);
            ++out.acks_sent;
            ++out.effects_applied;
            break;
        case GuildMoveSideEffectKind::BroadcastGuildMoveNack:
            sink.broadcast_guild_move_nack(
                effect.from_pos, effect.to_pos,
                effect.from_item_idx, effect.to_item_idx,
                effect.original_rt, effect.error_code);
            ++out.nacks_sent;
            ++out.effects_applied;
            break;
        }
    }
    out.ack_flag_consumed = plan.send_ack;
    out.nack_flag_consumed = plan.send_nack;
    return out;
}

}  // namespace mxh::server
