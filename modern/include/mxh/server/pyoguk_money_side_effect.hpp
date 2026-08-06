// 1:1 data-plane + side-effect-dispatcher port of
// CPyoGukManager::PutInMoneyPyoguk and CPyoGukManager::PutOutMoneyPyoguk
// from legacy [Server]Map/PyogukManager.cpp:304-395.
//
// Both functions follow the same pattern:
//   1. Clamp the requested amount against the source purse and the
//      target purse's available space:
//        PutIn:  setMoney = min(setMoney, inven_money, maxmon - pyoguk_money)
//        PutOut: getMoney = min(getMoney, pyoguk_money, inven_max - inven_money)
//   2. If the clamped amount is 0, the put-in variant sends a NACK;
//      the put-out variant silently returns (legacy: no message).
//   3. On non-zero: mutate the player purses (subtract + add), call
//      PyogukMoneyUpdateToDB, InsertLogMoney, LogItemMoney, and send
//      a MSG_DWORD ACK with dwData = remaining pyoguk money.
//
// The data plane below encodes the clamp logic + success/failure
// decision; the side-effect dispatcher emits the structured steps so
// the orchestrator can route them to runtime player + DBThread + log
// subsystems without re-reading the legacy body.

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

// 1:1 with legacy [CC]Header/Protocol.h MP_PYOGUK_PUTIN_MONEY_ACK /
// _NACK and MP_PYOGUK_PUTOUT_MONEY_ACK. The Category is MP_PYOGUK
// (locked by the dispatcher).
inline constexpr std::uint8_t LEGACY_MP_PYOGUK_PUTIN_ACK  = 5u;
inline constexpr std::uint8_t LEGACY_MP_PYOGUK_PUTIN_NACK = 6u;
inline constexpr std::uint8_t LEGACY_MP_PYOGUK_PUTOUT_ACK = 8u;

// 1:1 with legacy [CC]Header/CommonGameDefine.h eMoneyLog_LosePyoguk /
// eMoneyLog_GetPyoguk / eLog_ItemMoveInvenToPyoguk / eLog_ItemMovePyogukToInven.
// The data plane captures the log code the orchestrator should pass
// to InsertLogMoney / LogItemMoney so the legacy log table stays
// populated exactly the same way.
inline constexpr std::uint32_t LEGACY_EMONEYLOG_LOSE_PYOGUK = 21u;
inline constexpr std::uint32_t LEGACY_EMONEYLOG_GET_PYOGUK  = 22u;
inline constexpr std::uint32_t LEGACY_ELOG_ITEM_MOVE_INVEN_TO_PYOGUK  = 43u;
inline constexpr std::uint32_t LEGACY_ELOG_ITEM_MOVE_PYOGUK_TO_INVEN = 44u;

// The clamp + decision for PutInMoneyPyoguk. Given the player's
// current inven money, the pyoguk money, and the pyoguk max, returns
// the actual amount that would be transferred (0 means NACK).
inline std::uint64_t pyoguk_put_in_clamp(
    std::uint64_t requested,
    std::uint64_t inven_money,
    std::uint64_t pyoguk_money,
    std::uint64_t pyoguk_max) {
    std::uint64_t actual = requested;
    if (actual > inven_money) {
        actual = inven_money;
    }
    const std::uint64_t pyoguk_space =
        (pyoguk_max > pyoguk_money) ? (pyoguk_max - pyoguk_money) : 0u;
    if (actual > pyoguk_space) {
        actual = pyoguk_space;
    }
    return actual;
}

// The clamp + decision for PutOutMoneyPyoguk. Given the player's
// current pyoguk money, inven money, and inven max, returns the
// actual amount that would be transferred (0 means silent).
inline std::uint64_t pyoguk_put_out_clamp(
    std::uint64_t requested,
    std::uint64_t pyoguk_money,
    std::uint64_t inven_money,
    std::uint64_t inven_max) {
    std::uint64_t actual = requested;
    if (actual > pyoguk_money) {
        actual = pyoguk_money;
    }
    const std::uint64_t inven_space =
        (inven_max > inven_money) ? (inven_max - inven_money) : 0u;
    if (actual > inven_space) {
        actual = inven_space;
    }
    return actual;
}

// Side-effect kinds (apply to both PutIn and PutOut; the
// orchestrator chooses the right category/protocol pair based on
// whether the plan was produced by the put-in or put-out helper).
enum class PyogukMoneySideEffectKind : std::uint8_t {
    SubtractFromSource = 0,    // legacy pPlayer->SetMoney(SUBTRACTION, source)
    AddToTarget = 1,           // legacy pPlayer->SetMoney(ADDITION, target)
    UpdatePyogukMoneyDB = 2,   // legacy PyogukMoneyUpdateToDB(user_id, new_pyoguk_mon)
    InsertLogMoney = 3,        // legacy InsertLogMoney(eMoneyLog_*, ...)
    LogItemMoney = 4,          // legacy LogItemMoney(player_id, name, ..., eLog_ItemMove*, ...)
    BroadcastAck = 5,          // legacy pPlayer->SendMsg(MP_PYOGUK_PUT*_MONEY_ACK, dwData=pyoguk_mon)
    BroadcastNack = 6,         // legacy pPlayer->SendMsg(MP_PYOGUK_PUTIN_MONEY_NACK)
    SilentSkip = 7,            // legacy put-out getMoney == 0: no message
};

// Carries the data needed by each step. The orchestrator reads
// source_kind to know which purse to mutate and which category/protocol
// to broadcast.
enum class PyogukMoneySource : std::uint8_t {
    PutIn  = 0,   // legacy PutInMoneyPyoguk: source=inven, target=pyoguk
    PutOut = 1,   // legacy PutOutMoneyPyoguk: source=pyoguk, target=inven
};

struct PyogukMoneySideEffect final {
    PyogukMoneySideEffectKind kind =
        PyogukMoneySideEffectKind::SubtractFromSource;
    std::uint64_t amount = 0;
    std::uint64_t new_pyoguk_money = 0;
    std::uint64_t new_inven_money = 0;
    std::uint32_t money_log_code = 0;
    std::uint32_t item_log_code = 0;
};

struct PyogukMoneySideEffectPlan final {
    std::vector<PyogukMoneySideEffect> effects;
    bool send_ack = false;
    bool send_nack = false;
    bool silent = false;
    PyogukMoneySource source = PyogukMoneySource::PutIn;
};

// 1:1 with legacy PyogukManager.cpp:304-352 PutInMoneyPyoguk side
// effects. On success: 6 steps in legacy order (subtract, add, DB
// update, InsertLogMoney, LogItemMoney, broadcast ACK). On failure
// (clamped to 0): single NACK broadcast step.
inline PyogukMoneySideEffectPlan pyoguk_money_put_in_side_effect_plan(
    std::uint64_t requested,
    std::uint64_t inven_money,
    std::uint64_t pyoguk_money,
    std::uint64_t pyoguk_max) {
    PyogukMoneySideEffectPlan plan;
    plan.source = PyogukMoneySource::PutIn;
    const std::uint64_t actual = pyoguk_put_in_clamp(
        requested, inven_money, pyoguk_money, pyoguk_max);
    if (actual == 0u) {
        plan.send_nack = true;
        plan.effects.reserve(1u);
        PyogukMoneySideEffect broadcast{};
        broadcast.kind = PyogukMoneySideEffectKind::BroadcastNack;
        plan.effects.push_back(broadcast);
        return plan;
    }
    plan.send_ack = true;
    plan.effects.reserve(6u);

    const std::uint64_t new_pyoguk = pyoguk_money + actual;
    const std::uint64_t new_inven  = inven_money  - actual;

    PyogukMoneySideEffect subtract{};
    subtract.kind = PyogukMoneySideEffectKind::SubtractFromSource;
    subtract.amount = actual;
    subtract.new_inven_money = new_inven;
    plan.effects.push_back(subtract);

    PyogukMoneySideEffect add{};
    add.kind = PyogukMoneySideEffectKind::AddToTarget;
    add.amount = actual;
    add.new_pyoguk_money = new_pyoguk;
    plan.effects.push_back(add);

    PyogukMoneySideEffect db{};
    db.kind = PyogukMoneySideEffectKind::UpdatePyogukMoneyDB;
    db.amount = actual;
    db.new_pyoguk_money = new_pyoguk;
    plan.effects.push_back(db);

    PyogukMoneySideEffect insert_log{};
    insert_log.kind = PyogukMoneySideEffectKind::InsertLogMoney;
    insert_log.amount = actual;
    insert_log.new_inven_money = new_inven;
    insert_log.new_pyoguk_money = new_pyoguk;
    insert_log.money_log_code = LEGACY_EMONEYLOG_LOSE_PYOGUK;
    plan.effects.push_back(insert_log);

    PyogukMoneySideEffect item_log{};
    item_log.kind = PyogukMoneySideEffectKind::LogItemMoney;
    item_log.amount = actual;
    item_log.new_inven_money = new_inven;
    item_log.new_pyoguk_money = new_pyoguk;
    item_log.item_log_code = LEGACY_ELOG_ITEM_MOVE_INVEN_TO_PYOGUK;
    plan.effects.push_back(item_log);

    PyogukMoneySideEffect ack{};
    ack.kind = PyogukMoneySideEffectKind::BroadcastAck;
    ack.amount = actual;
    ack.new_pyoguk_money = new_pyoguk;
    plan.effects.push_back(ack);

    return plan;
}

// 1:1 with legacy PyogukManager.cpp:354-395 PutOutMoneyPyoguk side
// effects. On success: same 6-step chain. On failure (clamped to 0):
// silent return (no message, legacy has no NACK for put-out).
inline PyogukMoneySideEffectPlan pyoguk_money_put_out_side_effect_plan(
    std::uint64_t requested,
    std::uint64_t pyoguk_money,
    std::uint64_t inven_money,
    std::uint64_t inven_max) {
    PyogukMoneySideEffectPlan plan;
    plan.source = PyogukMoneySource::PutOut;
    const std::uint64_t actual = pyoguk_put_out_clamp(
        requested, pyoguk_money, inven_money, inven_max);
    if (actual == 0u) {
        plan.silent = true;
        return plan;
    }
    plan.send_ack = true;
    plan.effects.reserve(6u);

    const std::uint64_t new_pyoguk = pyoguk_money - actual;
    const std::uint64_t new_inven  = inven_money  + actual;

    PyogukMoneySideEffect subtract{};
    subtract.kind = PyogukMoneySideEffectKind::SubtractFromSource;
    subtract.amount = actual;
    subtract.new_pyoguk_money = new_pyoguk;
    plan.effects.push_back(subtract);

    PyogukMoneySideEffect add{};
    add.kind = PyogukMoneySideEffectKind::AddToTarget;
    add.amount = actual;
    add.new_inven_money = new_inven;
    plan.effects.push_back(add);

    PyogukMoneySideEffect db{};
    db.kind = PyogukMoneySideEffectKind::UpdatePyogukMoneyDB;
    db.amount = actual;
    db.new_pyoguk_money = new_pyoguk;
    plan.effects.push_back(db);

    PyogukMoneySideEffect insert_log{};
    insert_log.kind = PyogukMoneySideEffectKind::InsertLogMoney;
    insert_log.amount = actual;
    insert_log.new_inven_money = new_inven;
    insert_log.new_pyoguk_money = new_pyoguk;
    insert_log.money_log_code = LEGACY_EMONEYLOG_GET_PYOGUK;
    plan.effects.push_back(insert_log);

    PyogukMoneySideEffect item_log{};
    item_log.kind = PyogukMoneySideEffectKind::LogItemMoney;
    item_log.amount = actual;
    item_log.new_inven_money = new_inven;
    item_log.new_pyoguk_money = new_pyoguk;
    item_log.item_log_code = LEGACY_ELOG_ITEM_MOVE_PYOGUK_TO_INVEN;
    plan.effects.push_back(item_log);

    PyogukMoneySideEffect ack{};
    ack.kind = PyogukMoneySideEffectKind::BroadcastAck;
    ack.amount = actual;
    ack.new_pyoguk_money = new_pyoguk;
    plan.effects.push_back(ack);

    return plan;
}

}  // namespace mxh::server
