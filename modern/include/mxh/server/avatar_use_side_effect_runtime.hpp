// avatar_use_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// avatar_use_side_effect_plan(). The data plane returns an empty-free
// single/2-step plan (success chain, async DB query, or NACK); this
// header walks the plan and dispatches each entry to a virtual
// AvatarUseSideEffectSink.
//
// 1:1 invariants (1:1 with legacy CItemManager::
// MP_ITEM_SHOPITEM_AVATAR_USE_SYN from
// [Server]Map/ItemManager.cpp:5410-5484):
//   - 4 gates in order: state in {None, Immortal} / IsUseAbleShopItem /
//     GetItemInfoAbsIn non-null / CheckWeaponToShopItem. Any failure ->
//     _Avataruse_failed -> MP_ITEM_SHOPITEM_AVATAR_USE_NACK (90).
//   - Not in the using list -> IsUseAbleShopAvatarItem async DB query
//     (no immediate reply).
//   - In using list + DBIdx mismatch -> NACK.
//   - In using list + DBIdx match -> PutOnAvatarItem; false -> NACK;
//     true -> MP_ITEM_SHOPITEM_AVATAR_USE_ACK (89).
//   - Success chain order: PutOnAvatarItem then SendAckToPlayer.
//
// Pattern mirrors item_discard_side_effect_runtime.hpp (D4.46) and
// the rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/avatar_use_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the AvatarUse side-effect chain.
class AvatarUseSideEffectSink {
public:
    virtual ~AvatarUseSideEffectSink() = default;

    // Legacy: SendMsg(MP_ITEM_SHOPITEM_AVATAR_USE_ACK).
    virtual void send_ack_to_player(std::uint32_t player_id,
                                    std::uint16_t item_idx,
                                    std::uint16_t item_pos) = 0;

    // Legacy: SendMsg(MP_ITEM_SHOPITEM_AVATAR_USE_NACK).
    virtual void send_nack_to_player(std::uint32_t player_id,
                                     std::uint16_t item_idx,
                                     std::uint16_t item_pos) = 0;

    // Legacy: ShopItemManager->PutOnAvatarItem(...).
    virtual void put_on_avatar_item(std::uint32_t player_id,
                                    std::uint16_t item_idx,
                                    std::uint16_t item_pos) = 0;

    // Legacy: IsUseAbleShopAvatarItem (async DB query).
    virtual void query_db_for_avatar_item(
        std::uint32_t player_id, std::uint32_t item_db_idx,
        std::uint32_t item_icon_idx, std::uint32_t item_position) = 0;
};

struct AvatarUseRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t acks_sent       = 0;
    std::size_t nacks_sent      = 0;
    std::size_t puts            = 0;
    std::size_t db_queries      = 0;
    bool ack_flag_consumed    = false;
    bool nack_flag_consumed   = false;
    bool put_flag_consumed    = false;
    bool db_flag_consumed     = false;
};

// Runtime: walks the plan and dispatches each entry in legacy order.
inline AvatarUseRuntimeOutcome apply_avatar_use_side_effects(
    const AvatarUseSideEffectPlan& plan,
    AvatarUseSideEffectSink& sink) {
    AvatarUseRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case AvatarUseSideEffectKind::SendAckToPlayer:
            sink.send_ack_to_player(
                effect.player_id, effect.item_idx, effect.item_pos);
            ++out.acks_sent;
            ++out.effects_applied;
            break;
        case AvatarUseSideEffectKind::SendNackToPlayer:
            sink.send_nack_to_player(
                effect.player_id, effect.item_idx, effect.item_pos);
            ++out.nacks_sent;
            ++out.effects_applied;
            break;
        case AvatarUseSideEffectKind::PutOnAvatarItem:
            sink.put_on_avatar_item(
                effect.player_id, effect.item_idx, effect.item_pos);
            ++out.puts;
            ++out.effects_applied;
            break;
        case AvatarUseSideEffectKind::QueryDbForAvatarItem:
            sink.query_db_for_avatar_item(
                effect.player_id, effect.item_db_idx,
                effect.item_icon_idx, effect.item_position);
            ++out.db_queries;
            ++out.effects_applied;
            break;
        }
    }
    out.ack_flag_consumed = plan.send_ack;
    out.nack_flag_consumed = plan.send_nack;
    out.put_flag_consumed = plan.put_on_avatar_item;
    out.db_flag_consumed = plan.query_db;
    return out;
}

}  // namespace mxh::server
