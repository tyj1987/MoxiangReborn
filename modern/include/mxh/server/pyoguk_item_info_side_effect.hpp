// 1:1 side-effect-dispatcher port of
// CItemManager::MP_ITEM_PYOGUK_ITEM_INFO_SYN from legacy
// [Server]Map/ItemManager.cpp:4922-4940.
//
// The legacy handler triggers a DB load of warehouse (pyoguk) item
// info. The flow is:
//   1. FindUser(pmsg->dwObjectID) -> pPlayer (return if null).
//   2. CheckHackNpc(pPlayer, wData1, wData2) (return if false -
//      hack detected, silent drop).
//   3. pPlayer->IsGotWarehouseItems() check (return if true -
//      dedup: another load is already in progress).
//   4. pPlayer->SetGotWarehouseItems(TRUE) + fire
//      PyogukItemOptionInfo (DB query).
//
// The handler does NOT send any ACK/NACK to the client; the data
// arrives later via MP_ITEM_PYOGUKITEM_INFO broadcasts from the DB
// callback. The success here is a state mutation + DB trigger.

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

// 1:1 with legacy [CC]Header/Protocol.h MP_ITEM_PYOGUK_ITEM_INFO_SYN
// (single protocol code, no ACK/NACK pair on the SYN path).
inline constexpr std::uint8_t LEGACY_MP_ITEM_PYOGUK_ITEM_INFO_SYN = 76u;

enum class PyogukItemInfoOutcome : std::uint8_t {
    Triggered        = 0,  // legacy: player + NPC + not-loading
    NoPlayer         = 1,  // legacy: FindUser returned null
    HackNpc          = 2,  // legacy: CheckHackNpc returned false
    AlreadyLoading   = 3,  // legacy: IsGotWarehouseItems() == TRUE
};

struct PyogukItemInfoValidationInput final {
    bool player_found = false;
    bool npc_check_ok = false;
    bool got_warehouse_items = false;
};

inline PyogukItemInfoOutcome classify_pyoguk_item_info_outcome(
    const PyogukItemInfoValidationInput& in) noexcept {
    if (!in.player_found) {
        return PyogukItemInfoOutcome::NoPlayer;
    }
    if (!in.npc_check_ok) {
        return PyogukItemInfoOutcome::HackNpc;
    }
    if (in.got_warehouse_items) {
        return PyogukItemInfoOutcome::AlreadyLoading;
    }
    return PyogukItemInfoOutcome::Triggered;
}

enum class PyogukItemInfoSideEffectKind : std::uint8_t {
    SetGotWarehouseItems = 0,  // legacy pPlayer->SetGotWarehouseItems(TRUE)
    FirePyogukDbQuery    = 1,  // legacy PyogukItemOptionInfo (DB query)
};

struct PyogukItemInfoSideEffect final {
    PyogukItemInfoSideEffectKind kind =
        PyogukItemInfoSideEffectKind::SetGotWarehouseItems;
    std::uint32_t object_id = 0;   // legacy pPlayer->GetID()
    std::uint32_t user_id = 0;     // legacy pPlayer->GetUserID()
    std::uint32_t start_db_idx = 0; // legacy PyogukItemOptionInfo(0)
};

struct PyogukItemInfoSideEffectPlan final {
    std::vector<PyogukItemInfoSideEffect> effects;
    bool trigger_db = false;
    bool mark_loading = false;
};

inline PyogukItemInfoSideEffectPlan pyoguk_item_info_side_effect_plan(
    const PyogukItemInfoValidationInput& in,
    std::uint32_t object_id,
    std::uint32_t user_id) {
    PyogukItemInfoSideEffectPlan plan;
    const PyogukItemInfoOutcome outcome =
        classify_pyoguk_item_info_outcome(in);
    if (outcome != PyogukItemInfoOutcome::Triggered) {
        return plan;
    }
    plan.mark_loading = true;
    plan.trigger_db = true;
    plan.effects.reserve(2u);

    PyogukItemInfoSideEffect mark{};
    mark.kind = PyogukItemInfoSideEffectKind::SetGotWarehouseItems;
    mark.object_id = object_id;
    plan.effects.push_back(mark);

    PyogukItemInfoSideEffect db{};
    db.kind = PyogukItemInfoSideEffectKind::FirePyogukDbQuery;
    db.object_id = object_id;
    db.user_id = user_id;
    db.start_db_idx = 0;
    plan.effects.push_back(db);
    return plan;
}

}  // namespace mxh::server
