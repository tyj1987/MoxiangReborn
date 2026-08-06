// 1:1 data-plane + side-effect-dispatcher port of CItemManager's
// MP_ITEM_MOVE_SYN handler from legacy [Server]Map/ItemManager.cpp:4042-4067.
//
// The legacy handler calls MoveItem(player, FromItemIdx, FromPos, ToItemIdx,
// ToPos) and routes the result into one of three branches:
//   1. EI_TRUE (rt == 0): success -> echo pmsg as MSG_ITEM_MOVE_ACK.
//   2. rt != 99 AND rt != EI_TRUE: failure -> send MSG_ITEM_ERROR with
//      ECode = eItemUseErr_Move (= 2) and the original rt as the
//      SendErrorMsg auxiliary code.
//   3. rt == 99: silent skip (legacy suppresses both ACK and NACK; the
//      handler returns with no message).
//
// The data plane below encodes the 3-way decision; the side-effect
// dispatcher emits the structured steps so the orchestrator can route
// each branch to its subsystem (BroadcastAck / BroadcastNack / Silent).

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

// 1:1 with legacy [Server]Map/ItemSlot.h ERROR_ITEM enum.
inline constexpr int LEGACY_EI_TRUE          = 0;
inline constexpr int LEGACY_EI_OUTOFPOS      = 1;
inline constexpr int LEGACY_EI_NOTEQUALDATA  = 2;
inline constexpr int LEGACY_EI_EXISTED       = 3;
inline constexpr int LEGACY_EI_NOTEXIST      = 4;
inline constexpr int LEGACY_EI_LOCKED        = 5;
inline constexpr int LEGACY_EI_PASSWD        = 6;
inline constexpr int LEGACY_EI_NOTENOUGHMONEY = 7;
inline constexpr int LEGACY_EI_NOSPACE       = 8;
inline constexpr int LEGACY_EI_MAXMONEY      = 9;

// 1:1 with legacy rt == 99 silent-skip sentinel. The legacy code
// suppresses both ACK and NACK when MoveItem returns this value; the
// data plane preserves that exact behavior.
inline constexpr int LEGACY_MOVE_RT_SILENT = 99;

// 1:1 with legacy [CC]Header/CommonGameDefine.h eItemUse_Err.
inline constexpr int LEGACY_EITEMUSE_SUCCESS = 0;
inline constexpr int LEGACY_EITEMUSE_PREINSERT = 1;
inline constexpr int LEGACY_EITEMUSE_MOVE = 2;
inline constexpr int LEGACY_EITEMUSE_COMBINE = 3;
inline constexpr int LEGACY_EITEMUSE_DIVIDE = 4;
inline constexpr int LEGACY_EITEMUSE_DISCARD = 5;

// 1:1 with legacy [CC]Header/Protocol.h MP_ITEM_MOVE_ACK and the
// generic error-NACK protocol used by ItemManager. The Category is
// MP_ITEM (locked by the dispatcher), only the protocol bytes are
// needed at the data plane.
inline constexpr std::uint8_t LEGACY_MP_ITEM_MOVE_ACK     = 50u;
inline constexpr std::uint8_t LEGACY_MP_ITEM_ERROR_NACK  = 99u;

// 3-way decision from MoveItem.
enum class ItemMoveOutcome : std::uint8_t {
    Success = 0,  // legacy rt == 0 -> echo ACK
    Failure = 1,  // legacy rt != 0 && rt != 99 -> emit NACK
    Silent  = 2,  // legacy rt == 99 -> emit no message
};

// Pure decision function. The caller passes the legacy rt value
// returned by MoveItem and the data plane produces the 3-way outcome.
inline ItemMoveOutcome classify_item_move_outcome(int move_rt) noexcept {
    if (move_rt == LEGACY_EI_TRUE) {
        return ItemMoveOutcome::Success;
    }
    if (move_rt == LEGACY_MOVE_RT_SILENT) {
        return ItemMoveOutcome::Silent;
    }
    return ItemMoveOutcome::Failure;
}

// Side-effect kinds.
enum class ItemMoveSideEffectKind : std::uint8_t {
    BroadcastMoveAck = 0,    // legacy SendAckMsg(MP_ITEM_MOVE_ACK)
    BroadcastMoveNack = 1,   // legacy SendErrorMsg(MP_ITEM_ERROR_NACK, ECode=eItemUseErr_Move, rt)
    SilentSkip = 2,          // legacy rt==99: emit no message
};

// Carries the in/out positions so the orchestrator can reuse the
// inbound pmsg payload (legacy: memcpy(&msg, pmsg, sizeof(MSG_ITEM_MOVE_SYN))
// before flipping the Protocol field).
struct ItemMoveSideEffect final {
    ItemMoveSideEffectKind kind = ItemMoveSideEffectKind::BroadcastMoveAck;
    std::uint16_t from_item_idx = 0;   // legacy pmsg->wFromItemIdx
    std::uint16_t from_pos      = 0;   // legacy pmsg->FromPos
    std::uint16_t to_item_idx   = 0;   // legacy pmsg->wToItemIdx
    std::uint16_t to_pos        = 0;   // legacy pmsg->ToPos
    int original_rt = 0;               // legacy rt from MoveItem
};

struct ItemMoveSideEffectPlan final {
    std::vector<ItemMoveSideEffect> effects;
    bool send_ack = false;
    bool send_nack = false;
    bool silent = false;
};

// 1:1 with legacy ItemManager::MP_ITEM_MOVE_SYN. The data plane
// produces the structured steps; the orchestrator routes them to
// SendAckMsg (success), SendErrorMsg (failure), or no-op (silent).
//
// from_item_idx/from_pos/to_item_idx/to_pos are the four inbound
// fields from MSG_ITEM_MOVE_SYN, captured so the broadcast step can
// reuse the inbound payload via memcpy (the legacy ACK is just a
// Protocol-field-flipped copy of the SYN).
inline ItemMoveSideEffectPlan item_move_side_effect_plan(
    int move_rt,
    std::uint16_t from_item_idx,
    std::uint16_t from_pos,
    std::uint16_t to_item_idx,
    std::uint16_t to_pos) {
    ItemMoveSideEffectPlan plan;
    const ItemMoveOutcome outcome = classify_item_move_outcome(move_rt);

    if (outcome == ItemMoveOutcome::Success) {
        plan.send_ack = true;
        plan.effects.reserve(1u);
        ItemMoveSideEffect broadcast{};
        broadcast.kind = ItemMoveSideEffectKind::BroadcastMoveAck;
        broadcast.from_item_idx = from_item_idx;
        broadcast.from_pos = from_pos;
        broadcast.to_item_idx = to_item_idx;
        broadcast.to_pos = to_pos;
        broadcast.original_rt = move_rt;
        plan.effects.push_back(broadcast);
    } else if (outcome == ItemMoveOutcome::Failure) {
        plan.send_nack = true;
        plan.effects.reserve(1u);
        ItemMoveSideEffect broadcast{};
        broadcast.kind = ItemMoveSideEffectKind::BroadcastMoveNack;
        broadcast.from_item_idx = from_item_idx;
        broadcast.from_pos = from_pos;
        broadcast.to_item_idx = to_item_idx;
        broadcast.to_pos = to_pos;
        broadcast.original_rt = move_rt;
        plan.effects.push_back(broadcast);
    } else {
        // Silent: legacy rt == 99 -> no message.
        plan.silent = true;
    }
    return plan;
}

}  // namespace mxh::server
