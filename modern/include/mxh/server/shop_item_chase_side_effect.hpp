// 1:1 side-effect-dispatcher port of
// CItemManager::MP_ITEM_SHOPITEM_CHASE_SYN from legacy
// [Server]Map/ItemManager.cpp:5502-5545.
//
// The legacy handler resolves a target player's location for a
// chase (location-track) item. The flow is:
//   1. FindUser(pmsg->dwData2) -> pPlayer (return if null):
//      send MP_ITEM_SHOPITEM_CHASE_NACK with dwData = pmsg->dwData1
//      to the agent server.
//   2. pmsg->dwData3 is one of {eIncantation_Tracking,
//      eIncantation_Tracking7, eIncantation_Tracking7_NoTrade}:
//      a. Build SEND_CHASEINFO {MP_ITEM, CHASE_ACK, pPlayer->GetID(),
//         pPlayer->GetObjectName(), pPlayer->GetPosition(),
//         MapNum, EventMapNum (44 if suryun battle else channel id),
//         pmsg->dwData1 as CharacterIdx} and send to agent.
//      b. Send MSGBASE {MP_ITEM, CHASE_TRACKING} to the player.
//
// The handler does NOT send an ACK/NACK to the originating client
// directly; it goes through the agent server.

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

// 1:1 with legacy [CC]Header/Protocol.h MP_ITEM_SHOPITEM_CHASE_*.
inline constexpr std::uint8_t LEGACY_MP_ITEM_SHOPITEM_CHASE_ACK      = 98u;
inline constexpr std::uint8_t LEGACY_MP_ITEM_SHOPITEM_CHASE_NACK     = 99u;
inline constexpr std::uint8_t LEGACY_MP_ITEM_SHOPITEM_CHASE_TRACKING = 100u;

// 1:1 with legacy eIncantation_Tracking / _Tracking7 / _Tracking7_NoTrade.
inline constexpr std::uint32_t LEGACY_EINCANTATION_TRACKING             = 31u;
inline constexpr std::uint32_t LEGACY_EINCANTATION_TRACKING7            = 32u;
inline constexpr std::uint32_t LEGACY_EINCANTATION_TRACKING7_NOTRADE    = 33u;

// 1:1 with legacy eBATTLE_KIND_SURYUN event map id.
inline constexpr int LEGACY_SURYUN_EVENT_MAP_NUM = 44;

enum class ShopItemChaseOutcome : std::uint8_t {
    Resolve   = 0,  // legacy: target found + tracking variant
    NoTarget  = 1,  // legacy: FindUser returned null
    NotChase  = 2,  // legacy: dwData3 is not a chase variant
};

struct ShopItemChaseValidationInput final {
    bool target_found = false;
    std::uint32_t item_kind = 0;
};

inline bool is_chase_tracking_variant(std::uint32_t item_kind) noexcept {
    return item_kind == LEGACY_EINCANTATION_TRACKING ||
           item_kind == LEGACY_EINCANTATION_TRACKING7 ||
           item_kind == LEGACY_EINCANTATION_TRACKING7_NOTRADE;
}

inline ShopItemChaseOutcome classify_shop_item_chase_outcome(
    const ShopItemChaseValidationInput& in) noexcept {
    if (!in.target_found) {
        return ShopItemChaseOutcome::NoTarget;
    }
    if (!is_chase_tracking_variant(in.item_kind)) {
        return ShopItemChaseOutcome::NotChase;
    }
    return ShopItemChaseOutcome::Resolve;
}

enum class ShopItemChaseSideEffectKind : std::uint8_t {
    ForwardChaseAckToAgent  = 0,  // legacy Send2AgentServer(CHASE_ACK)
    ForwardChaseNackToAgent = 1,  // legacy Send2AgentServer(CHASE_NACK)
    BroadcastChaseTracking   = 2, // legacy SendMsg(CHASE_TRACKING)
};

struct ShopItemChaseSideEffect final {
    ShopItemChaseSideEffectKind kind =
        ShopItemChaseSideEffectKind::ForwardChaseAckToAgent;
    std::uint32_t target_id = 0;        // legacy pPlayer->GetID()
    std::uint32_t requester_char_idx = 0; // legacy pmsg->dwData1
    std::uint32_t item_kind = 0;        // legacy pmsg->dwData3
    int map_num = 0;                    // legacy GAMERESRCMNGR->GetLoadMapNum()
    int event_map_num = 0;              // legacy 44 or channel id
};

struct ShopItemChaseSideEffectPlan final {
    std::vector<ShopItemChaseSideEffect> effects;
    bool forward_ack = false;
    bool forward_nack = false;
    bool broadcast_tracking = false;
};

inline ShopItemChaseSideEffectPlan shop_item_chase_side_effect_plan(
    const ShopItemChaseValidationInput& in,
    std::uint32_t target_id,
    std::uint32_t requester_char_idx,
    int map_num,
    int event_map_num) {
    ShopItemChaseSideEffectPlan plan;
    const ShopItemChaseOutcome outcome =
        classify_shop_item_chase_outcome(in);
    if (outcome == ShopItemChaseOutcome::NoTarget) {
        plan.forward_nack = true;
        plan.effects.reserve(1u);
        ShopItemChaseSideEffect nack{};
        nack.kind = ShopItemChaseSideEffectKind::ForwardChaseNackToAgent;
        nack.requester_char_idx = requester_char_idx;
        plan.effects.push_back(nack);
        return plan;
    }
    if (outcome == ShopItemChaseOutcome::NotChase) {
        return plan;
    }
    plan.forward_ack = true;
    plan.broadcast_tracking = true;
    plan.effects.reserve(2u);

    ShopItemChaseSideEffect ack{};
    ack.kind = ShopItemChaseSideEffectKind::ForwardChaseAckToAgent;
    ack.target_id = target_id;
    ack.requester_char_idx = requester_char_idx;
    ack.item_kind = in.item_kind;
    ack.map_num = map_num;
    ack.event_map_num = event_map_num;
    plan.effects.push_back(ack);

    ShopItemChaseSideEffect track{};
    track.kind = ShopItemChaseSideEffectKind::BroadcastChaseTracking;
    track.target_id = target_id;
    track.item_kind = in.item_kind;
    plan.effects.push_back(track);
    return plan;
}

}  // namespace mxh::server
