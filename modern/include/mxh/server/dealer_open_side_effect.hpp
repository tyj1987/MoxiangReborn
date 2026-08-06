// 1:1 side-effect-dispatcher port of
// CItemManager::MP_ITEM_DEALER_SYN from legacy
// [Server]Map/ItemManager.cpp:4851-4871.
//
// The legacy handler opens a street-stall dealer UI for the player.
// The flow is:
//   1. FindUser(pmsg->dwObjectID) -> pPlayer (return if null).
//   2. CheckHackNpc(pPlayer, pmsg->wData) (return if false - hack
//      detected, silent drop).
//   3. Build MSG_WORD {Category=MP_ITEM, Protocol=DEALER_ACK,
//      wData=pmsg->wData} and SendMsg.
//
// The handler does NOT send a NACK on failure; it just returns.

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

// 1:1 with legacy [CC]Header/Protocol.h MP_ITEM_DEALER_ACK.
inline constexpr std::uint8_t LEGACY_MP_ITEM_DEALER_ACK = 75u;

enum class DealerOpenOutcome : std::uint8_t {
    Opened   = 0,  // legacy: player + NPC ok
    HackNpc  = 1,  // legacy: CheckHackNpc returned false
    NoPlayer = 2,  // legacy: FindUser returned null
};

struct DealerOpenValidationInput final {
    bool player_found = false;
    bool npc_check_ok = false;
};

inline DealerOpenOutcome classify_dealer_open_outcome(
    const DealerOpenValidationInput& in) noexcept {
    if (!in.player_found) {
        return DealerOpenOutcome::NoPlayer;
    }
    if (!in.npc_check_ok) {
        return DealerOpenOutcome::HackNpc;
    }
    return DealerOpenOutcome::Opened;
}

enum class DealerOpenSideEffectKind : std::uint8_t {
    BroadcastDealerAck = 0,  // legacy SendMsg(MP_ITEM_DEALER_ACK)
};

struct DealerOpenSideEffect final {
    DealerOpenSideEffectKind kind =
        DealerOpenSideEffectKind::BroadcastDealerAck;
    std::uint16_t npc_pos = 0;  // legacy pmsg->wData
};

struct DealerOpenSideEffectPlan final {
    std::vector<DealerOpenSideEffect> effects;
    bool send_ack = false;
};

inline DealerOpenSideEffectPlan dealer_open_side_effect_plan(
    const DealerOpenValidationInput& in,
    std::uint16_t npc_pos) {
    DealerOpenSideEffectPlan plan;
    const DealerOpenOutcome outcome = classify_dealer_open_outcome(in);
    if (outcome != DealerOpenOutcome::Opened) {
        return plan;
    }
    plan.send_ack = true;
    plan.effects.reserve(1u);
    DealerOpenSideEffect eff{};
    eff.kind = DealerOpenSideEffectKind::BroadcastDealerAck;
    eff.npc_pos = npc_pos;
    plan.effects.push_back(eff);
    return plan;
}

}  // namespace mxh::server
