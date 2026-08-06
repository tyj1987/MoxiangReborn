// 1:1 side-effect-dispatcher port of CItemManager's SAVEPOINT update /
// delete handlers from legacy [Server]Map/ItemManager.cpp:5148-5188.
//
// Both handlers share the same pattern: try to mutate the in-memory
// saved-move-point table via the ShopItemManager, on success mutate
// the inbound pmsg->Protocol field to ACK and call the matching DB
// helper, on failure mutate the protocol field to NACK. The original
// pmsg buffer is then echoed back to the player via SendMsg(pmsg).
//
// The data plane below captures both outcomes in structured payloads
// so the orchestrator can route them to the runtime player + DBThread
// subsystems without re-reading the legacy body.

#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include <mxh/game/shop_item_types.hpp>

namespace mxh::server {

// 1:1 with legacy [CC]Header/Protocol.h MP_ITEM_SHOPITEM_SAVEPOINT_*.
// The Category is MP_ITEM (locked by the existing dispatcher), only the
// protocol bytes are needed at the data plane.
inline constexpr std::uint8_t LEGACY_MP_SAVEPOINT_UPDATE_ACK  = 154u;
inline constexpr std::uint8_t LEGACY_MP_SAVEPOINT_UPDATE_NACK = 155u;
inline constexpr std::uint8_t LEGACY_MP_SAVEPOINT_DEL_ACK     = 156u;
inline constexpr std::uint8_t LEGACY_MP_SAVEPOINT_DEL_NACK    = 157u;

// The legacy handler uses MAX_SAVED_MOVE_NAME=21 for the rename buffer.
// (Same constant as MoveData::Name.)
inline constexpr std::size_t LEGACY_MAX_SAVED_MOVE_NAME = 21u;

// ---------- SavePointUpdate (MP_ITEM_SHOPITEM_SAVEPOINT_UPDATE_SYN) ----------

enum class SavePointUpdateSideEffectKind : std::uint8_t {
    BroadcastUpdateAck = 0,   // legacy SendMsg(pmsg) with Protocol=UPDATE_ACK
    RenameSavedMovePoint = 1, // legacy SavedMovePointUpdate DB call
    BroadcastUpdateNack = 2,  // legacy SendMsg(pmsg) with Protocol=UPDATE_NACK
};

struct SavePointUpdateSideEffect final {
    SavePointUpdateSideEffectKind kind =
        SavePointUpdateSideEffectKind::BroadcastUpdateAck;
    std::uint32_t db_idx = 0;  // legacy pmsg->Data.DBIdx
    std::array<char, LEGACY_MAX_SAVED_MOVE_NAME> new_name{};
};

struct SavePointUpdateSideEffectPlan final {
    std::vector<SavePointUpdateSideEffect> effects;
    bool send_ack = false;     // true -> success path
    bool send_nack = false;    // true -> failure path
    std::uint8_t ack_protocol = LEGACY_MP_SAVEPOINT_UPDATE_ACK;
    std::uint8_t nack_protocol = LEGACY_MP_SAVEPOINT_UPDATE_NACK;
};

// 1:1 with legacy ItemManager.cpp:5148-5167. The success plan emits
// two steps in legacy order: DB rename first, then echo the original
// pmsg buffer back with Protocol flipped to UPDATE_ACK. The failure
// plan emits a single step: echo pmsg with Protocol=UPDATE_NACK.
//
// has_match = the legacy pPlayer->GetShopItemManager()->ReNameMovePoint
// return value: true means the DBIdx exists in the saved-move-point
// table, false means the lookup miss.
//
// The orchestrator routes steps via ShopItemManager::rename_move_point
// for the DB step and SendMsg for the broadcast step.
inline SavePointUpdateSideEffectPlan save_point_update_side_effect_plan(
    std::uint32_t db_idx,
    const std::array<char, LEGACY_MAX_SAVED_MOVE_NAME>& new_name,
    bool has_match) {
    SavePointUpdateSideEffectPlan plan;
    if (has_match) {
        plan.effects.reserve(2u);
        plan.send_ack = true;

        SavePointUpdateSideEffect db{};
        db.kind = SavePointUpdateSideEffectKind::RenameSavedMovePoint;
        db.db_idx = db_idx;
        db.new_name = new_name;
        plan.effects.push_back(db);

        SavePointUpdateSideEffect broadcast{};
        broadcast.kind = SavePointUpdateSideEffectKind::BroadcastUpdateAck;
        broadcast.db_idx = db_idx;
        broadcast.new_name = new_name;
        plan.effects.push_back(broadcast);
    } else {
        plan.effects.reserve(1u);
        plan.send_nack = true;

        SavePointUpdateSideEffect broadcast{};
        broadcast.kind = SavePointUpdateSideEffectKind::BroadcastUpdateNack;
        broadcast.db_idx = db_idx;
        broadcast.new_name = new_name;
        plan.effects.push_back(broadcast);
    }
    return plan;
}

// ---------- SavePointDel (MP_ITEM_SHOPITEM_SAVEPOINT_DEL_SYN) ----------

enum class SavePointDelSideEffectKind : std::uint8_t {
    BroadcastDelAck = 0,      // legacy SendMsg(pmsg) with Protocol=DEL_ACK
    DeleteSavedMovePoint = 1, // legacy SavedMovePointDelete DB call
    BroadcastDelNack = 2,     // legacy SendMsg(pmsg) with Protocol=DEL_NACK
};

struct SavePointDelSideEffect final {
    SavePointDelSideEffectKind kind =
        SavePointDelSideEffectKind::BroadcastDelAck;
    std::uint32_t db_idx = 0;  // legacy pmsg->Data.DBIdx
};

struct SavePointDelSideEffectPlan final {
    std::vector<SavePointDelSideEffect> effects;
    bool send_ack = false;
    bool send_nack = false;
    std::uint8_t ack_protocol = LEGACY_MP_SAVEPOINT_DEL_ACK;
    std::uint8_t nack_protocol = LEGACY_MP_SAVEPOINT_DEL_NACK;
};

// 1:1 with legacy ItemManager.cpp:5169-5188. Same pattern as the
// update handler: success -> DB delete + echo ACK, failure -> echo NACK.
// has_match = legacy pPlayer->GetShopItemManager()->DeleteMovePoint
// return value.
inline SavePointDelSideEffectPlan save_point_del_side_effect_plan(
    std::uint32_t db_idx, bool has_match) {
    SavePointDelSideEffectPlan plan;
    if (has_match) {
        plan.effects.reserve(2u);
        plan.send_ack = true;

        SavePointDelSideEffect db{};
        db.kind = SavePointDelSideEffectKind::DeleteSavedMovePoint;
        db.db_idx = db_idx;
        plan.effects.push_back(db);

        SavePointDelSideEffect broadcast{};
        broadcast.kind = SavePointDelSideEffectKind::BroadcastDelAck;
        broadcast.db_idx = db_idx;
        plan.effects.push_back(broadcast);
    } else {
        plan.effects.reserve(1u);
        plan.send_nack = true;

        SavePointDelSideEffect broadcast{};
        broadcast.kind = SavePointDelSideEffectKind::BroadcastDelNack;
        broadcast.db_idx = db_idx;
        plan.effects.push_back(broadcast);
    }
    return plan;
}

}  // namespace mxh::server
