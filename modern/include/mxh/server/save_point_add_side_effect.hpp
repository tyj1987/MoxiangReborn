// 1:1 side-effect-dispatcher port of CItemManager::MP_ITEM_SHOPITEM_SAVEPOINT_ADD_SYN
// from legacy [Server]Map/ItemManager.cpp:5104-5146.
//
// After the legacy handler validates the Validsavenum gate and calls
// UseShopItem successfully, the legacy code applies:
//   1. SEND_SHOPITEM_BASEINFO broadcast (Category=MP_ITEM,
//      Protocol=MP_ITEM_SHOPITEM_USE_ACK; payload = the SHOPITEMBASE
//      returned by UseShopItem + the original ShopItemPos/ShopItemIdx
//      from the request).
//   2. SavedMovePointInsert DB call (legacy: inserts a row into the
//      TB_SavedMovePoint table with PlayerID + Name + MapNum + Point).
//
// On failure (Validsavenum gate or UseShopItem != eItemUseSuccess), the
// legacy code emits a single MSG_ITEM_ERROR broadcast:
//   Category=MP_ITEM, Protocol=MP_ITEM_SHOPITEM_USE_NACK,
//   ECode = the BYTE error code (eItemUseSuccess=0 normally).
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

// 1:1 with legacy [CC]Header/CommonGameDefine.h MAX_MOVEDATA_PERPAGE /
// MAX_MOVEPOINT_PAGE. Used by the orchestrator to size the Validsavenum
// gate; the data plane does not own the gate, it only captures its
// outcome (which the caller already computed).
inline constexpr std::uint32_t MAX_MOVEDATA_PERPAGE_LEGACY = 10u;
inline constexpr std::uint32_t MAX_MOVEPOINT_PAGE_LEGACY   = 2u;
inline constexpr std::uint32_t MAX_SAVED_MOVE_BASE         = 10u;
inline constexpr std::uint32_t MAX_SAVED_MOVE_EXTENDED     =
    MAX_MOVEDATA_PERPAGE_LEGACY * MAX_MOVEPOINT_PAGE_LEGACY;  // 20

// 1:1 with legacy eItemUseSuccess (= 0 in legacy ItemUseErr enum).
inline constexpr std::uint8_t LEGACY_ITEM_USE_SUCCESS = 0u;

// 1:1 with legacy ItemManager.cpp:5110-5115 Validsavenum computation:
//   base = MAX_MOVEDATA_PERPAGE = 10
//   extended = MAX_MOVEDATA_PERPAGE * MAX_MOVEPOINT_PAGE = 20
//   The gate extends to 20 iff the player has any of the four
//   MemoryMoveExtend incantations in their using-item table.
inline std::uint32_t save_point_add_max_num(bool memory_move_extend_active) {
    return memory_move_extend_active ? MAX_SAVED_MOVE_EXTENDED
                                     : MAX_SAVED_MOVE_BASE;
}

// Success-path side-effect kinds (legacy ItemManager.cpp:5124-5135).
enum class SavePointAddSideEffectKind : std::uint8_t {
    BroadcastUseAck = 0,        // legacy SEND_SHOPITEM_BASEINFO MP_ITEM_SHOPITEM_USE_ACK
    InsertSavedMovePoint = 1,   // legacy SavedMovePointInsert DB call
};

struct SavePointAddSideEffect final {
    SavePointAddSideEffectKind kind =
        SavePointAddSideEffectKind::BroadcastUseAck;
    game::ShopItemBase shop_item_base{};  // legacy msg.ShopItemBase (ReturnItem)
    std::uint16_t shop_item_pos = 0;      // legacy msg.ShopItemPos
    std::uint16_t shop_item_idx = 0;      // legacy msg.ShopItemIdx
    std::array<char, 21u> move_name{};    // legacy SavedMovePointInsert name
    std::uint16_t map_num = 0;            // legacy SavedMovePointInsert MapNum
    std::uint32_t point_value = 0;        // legacy SavedMovePointInsert Point.value
};

struct SavePointAddSideEffectPlan final {
    std::vector<SavePointAddSideEffect> effects;
    bool send_use_ack = false;
};

// 1:1 with legacy ItemManager.cpp:5124-5135 success path: UseShopItem
// returned eItemUseSuccess (rt == 0). The plan always emits exactly two
// steps in legacy order: broadcast first, then DB insert.
inline SavePointAddSideEffectPlan save_point_add_success_side_effect_plan(
    const game::ShopItemBase& return_item,
    std::uint16_t shop_item_pos,
    std::uint16_t shop_item_idx,
    const std::array<char, 21u>& move_name,
    std::uint16_t map_num,
    std::uint32_t point_value) {
    SavePointAddSideEffectPlan plan;
    plan.effects.reserve(2u);
    plan.send_use_ack = true;

    SavePointAddSideEffect broadcast{};
    broadcast.kind = SavePointAddSideEffectKind::BroadcastUseAck;
    broadcast.shop_item_base = return_item;
    broadcast.shop_item_pos = shop_item_pos;
    broadcast.shop_item_idx = shop_item_idx;
    plan.effects.push_back(broadcast);

    SavePointAddSideEffect db{};
    db.kind = SavePointAddSideEffectKind::InsertSavedMovePoint;
    db.move_name = move_name;
    db.map_num = map_num;
    db.point_value = point_value;
    plan.effects.push_back(db);

    return plan;
}

// Failure-path side-effect kinds (legacy ItemManager.cpp:5136-5144).
enum class SavePointAddNackKind : std::uint8_t {
    BroadcastUseNack = 0,   // legacy MSG_ITEM_ERROR MP_ITEM_SHOPITEM_USE_NACK
};

struct SavePointAddNackStep final {
    SavePointAddNackKind kind = SavePointAddNackKind::BroadcastUseNack;
    std::uint8_t e_code = 0;  // legacy msg.ECode (= UseShopItem rt)
};

struct SavePointAddNackPlan final {
    std::vector<SavePointAddNackStep> steps;
    bool send_use_nack = false;
};

// 1:1 with legacy ItemManager.cpp:5136-5144 failure path: either the
// Validsavenum gate tripped (goto SAVEPOINT_ADD_FAILED) or UseShopItem
// returned a non-success code. The plan emits a single MSG_ITEM_ERROR
// NACK with the eCode the orchestrator already obtained.
inline SavePointAddNackPlan save_point_add_nack_side_effect_plan(
    std::uint8_t e_code) {
    SavePointAddNackPlan plan;
    SavePointAddNackStep step{};
    step.kind = SavePointAddNackKind::BroadcastUseNack;
    step.e_code = e_code;
    plan.steps.push_back(step);
    plan.send_use_nack = true;
    return plan;
}

}  // namespace mxh::server
