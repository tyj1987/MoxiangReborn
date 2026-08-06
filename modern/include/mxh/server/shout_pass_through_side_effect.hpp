//
// CItemManager::MP_ITEM_SHOPITEM_SHOUT_ACK/_NACK from legacy
// [Server]Map/ItemManager.cpp:6071-6091.
//
// The legacy handlers are simple pass-through dispatchers:
//   SHOUT_ACK (from agent -> map, after broadcast):
//     1. FindUser(pmsg->Receive.CharacterIdx) -> pPlayer.
//     2. If null, return (no message).
//     3. Rewrite pmsg->Protocol = MP_ITEM_SHOPITEM_SHOUT_SENDACK and
//        SendMsg to pPlayer.
//   SHOUT_NACK:
//     1. FindUser(pmsg->dwData) -> pPlayer.
//     2. If null, return.
//     3. SendMsg(pmsg, sizeof(*pmsg)).

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

// 1:1 with legacy [CC]Header/Protocol.h
inline constexpr std::uint8_t LEGACY_MP_ITEM_SHOPITEM_SHOUT_SENDACK = 94u;

enum class ShoutPassThroughOutcome : std::uint8_t {
    Forwarded   = 0,  // legacy: player found, message forwarded
    NoPlayer    = 1,  // legacy: FindUser returned null
};

enum class ShoutPassThroughVariant : std::uint8_t {
    SendAck  = 0,  // SHOUT_ACK -> SHOUT_SENDACK
    SendNack = 1,  // SHOUT_NACK passthrough
};

struct ShoutPassThroughValidationInput final {
    ShoutPassThroughVariant variant = ShoutPassThroughVariant::SendAck;
    bool player_found = false;
    std::uint32_t player_id = 0;
};

inline ShoutPassThroughOutcome classify_shout_pass_through_outcome(
    const ShoutPassThroughValidationInput& in) noexcept {
    if (!in.player_found) {
        return ShoutPassThroughOutcome::NoPlayer;
    }
    return ShoutPassThroughOutcome::Forwarded;
}

enum class ShoutPassThroughSideEffectKind : std::uint8_t {
    ForwardToPlayer = 0,  // legacy pPlayer->SendMsg(...)
};

struct ShoutPassThroughSideEffect final {
    ShoutPassThroughSideEffectKind kind =
        ShoutPassThroughSideEffectKind::ForwardToPlayer;
    ShoutPassThroughVariant variant =
        ShoutPassThroughVariant::SendAck;
    std::uint32_t player_id = 0;
    bool rewrite_to_sendack = false;
};

struct ShoutPassThroughSideEffectPlan final {
    std::vector<ShoutPassThroughSideEffect> effects;
    bool forward = false;
    bool rewrite_protocol = false;
};

inline ShoutPassThroughSideEffectPlan shout_pass_through_side_effect_plan(
    const ShoutPassThroughValidationInput& in) {
    ShoutPassThroughSideEffectPlan plan;
    const ShoutPassThroughOutcome outcome =
        classify_shout_pass_through_outcome(in);
    if (outcome == ShoutPassThroughOutcome::NoPlayer) {
        return plan;
    }
    plan.forward = true;
    plan.rewrite_protocol = (in.variant == ShoutPassThroughVariant::SendAck);
    plan.effects.reserve(1u);
    ShoutPassThroughSideEffect eff{};
    eff.kind = ShoutPassThroughSideEffectKind::ForwardToPlayer;
    eff.variant = in.variant;
    eff.player_id = in.player_id;
    eff.rewrite_to_sendack = plan.rewrite_protocol;
    plan.effects.push_back(eff);
    return plan;
}

}  // namespace mxh::server
