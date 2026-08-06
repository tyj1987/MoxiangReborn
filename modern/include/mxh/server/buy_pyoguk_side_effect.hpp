// 1:1 data-plane + side-effect-dispatcher port of
// CPyoGukManager::BuyPyogukSyn from legacy
// [Server]Map/PyogukManager.cpp:201-302.
//
// The legacy function validates two gates (TAB_PYOGUK_NUM cap + money
// vs BuyPrice) and routes to one of two branches:
//   1. Failure: send MP_PYOGUK_BUY_NACK (no DB write, no money change).
//   2. Success: subtract money from inven purse, increment pyoguknum,
//      update MaxPurseMoney for the new pyoguknum, call
//      PyogukBuyPyoguk DB helper, send MP_PYOGUK_BUY_ACK with bData =
//      new pyoguknum.
//
// The data plane below encodes the decision + 4-step success side
// effect chain so the orchestrator can route the steps to the
// runtime player + DBThread subsystems without re-reading the legacy
// body.

#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

// 1:1 with legacy [CC]Header/CommonGameDefine.h TAB_PYOGUK_NUM.
inline constexpr std::uint8_t LEGACY_TAB_PYOGUK_NUM = 5u;

// 1:1 with legacy [CC]Header/CommonGameDefine.h GIVEN_PYOGUK_SLOT.
// Legacy default (_KOR_LOCAL_) is 3; JP uses 3; HK/TL use 2. The data
// plane uses 3 as the most-permissive default; the orchestrator can
// override by passing a different given_slot_to_l into the plan.
inline constexpr std::uint8_t LEGACY_GIVEN_PYOGUK_SLOT = 3u;

// 1:1 with legacy [CC]Header/CommonGameDefine.h eMoneyLog_LosePyogukBuy.
inline constexpr std::uint32_t LEGACY_EMONEYLOG_LOSE_PYOGUK_BUY = 23u;

// 1:1 with legacy [CC]Header/Protocol.h MP_PYOGUK_BUY_ACK / _NACK.
// The Category is MP_PYOGUK (locked by the dispatcher).
inline constexpr std::uint8_t LEGACY_MP_PYOGUK_BUY_ACK  = 2u;
inline constexpr std::uint8_t LEGACY_MP_PYOGUK_BUY_NACK = 3u;

// 3-way decision from BuyPyogukSyn gates.
enum class BuyPyogukOutcome : std::uint8_t {
    Success = 0,  // legacy: money >= BuyPrice AND pyoguknum < TAB_PYOGUK_NUM
    Failure = 1,  // legacy: money < BuyPrice OR pyoguknum >= TAB_PYOGUK_NUM
};

// Pure gate classifier. given_slot_to_l is the legacy GIVEN_PYOGUK_SLOT
// plus extra-pyoguk slots (legacy: GIVEN_PYOGUK_SLOT + GetExtraPyogukSlot);
// it widens the TAB_PYOGUK_NUM gate for non-Korean locales.
inline BuyPyogukOutcome classify_buy_pyoguk_outcome(
    std::uint8_t current_pyoguk_num,
    std::uint64_t inven_money,
    std::uint64_t buy_price) noexcept {
    if (current_pyoguk_num >= LEGACY_TAB_PYOGUK_NUM) {
        return BuyPyogukOutcome::Failure;
    }
    if (inven_money < buy_price) {
        return BuyPyogukOutcome::Failure;
    }
    (void)0;
    return BuyPyogukOutcome::Success;
}

// Side-effect kinds.
enum class BuyPyogukSideEffectKind : std::uint8_t {
    SubtractBuyPrice = 0,        // legacy pPlayer->SetMoney(SUBTRACTION, BuyPrice, eMoneyLog_LosePyogukBuy)
    IncrementPyogukNum = 1,      // legacy pPlayer->SetPyogukNum(pyoguknum+1)
    UpdateMaxPurseMoney = 2,     // legacy pPlayer->SetMaxPurseMoney(eItemTable_Pyoguk, MaxMoney)
    InsertPyogukBuyDB = 3,       // legacy PyogukBuyPyoguk(player_id)
    BroadcastBuyAck = 4,         // legacy SendMsg(MP_PYOGUK_BUY_ACK, bData=new_pyoguknum)
    BroadcastBuyNack = 5,        // legacy SendMsg(MP_PYOGUK_BUY_NACK)
};

struct BuyPyogukSideEffect final {
    BuyPyogukSideEffectKind kind =
        BuyPyogukSideEffectKind::SubtractBuyPrice;
    std::uint64_t buy_price = 0;
    std::uint8_t new_pyoguk_num = 0;
    std::uint64_t new_max_money = 0;
};

struct BuyPyogukSideEffectPlan final {
    std::vector<BuyPyogukSideEffect> effects;
    bool send_ack = false;
    bool send_nack = false;
};

// 1:1 with legacy PyogukManager.cpp:201-302 BuyPyogukSyn. The plan
// emits the 4-step success chain in legacy order (subtract, set
// pyoguknum, set max money, DB write, broadcast ACK). Failure emits
// a single NACK broadcast step.
inline BuyPyogukSideEffectPlan buy_pyoguk_side_effect_plan(
    std::uint8_t current_pyoguk_num,
    std::uint64_t inven_money,
    std::uint64_t buy_price,
    std::uint64_t new_max_money_after_buy) {
    BuyPyogukSideEffectPlan plan;
    const BuyPyogukOutcome outcome = classify_buy_pyoguk_outcome(
        current_pyoguk_num, inven_money, buy_price);

    if (outcome == BuyPyogukOutcome::Failure) {
        plan.send_nack = true;
        plan.effects.reserve(1u);
        BuyPyogukSideEffect broadcast{};
        broadcast.kind = BuyPyogukSideEffectKind::BroadcastBuyNack;
        plan.effects.push_back(broadcast);
        return plan;
    }

    plan.send_ack = true;
    plan.effects.reserve(5u);
    const std::uint8_t new_pyoguk_num =
        static_cast<std::uint8_t>(current_pyoguk_num + 1u);

    BuyPyogukSideEffect subtract{};
    subtract.kind = BuyPyogukSideEffectKind::SubtractBuyPrice;
    subtract.buy_price = buy_price;
    plan.effects.push_back(subtract);

    BuyPyogukSideEffect inc{};
    inc.kind = BuyPyogukSideEffectKind::IncrementPyogukNum;
    inc.new_pyoguk_num = new_pyoguk_num;
    plan.effects.push_back(inc);

    BuyPyogukSideEffect max_money{};
    max_money.kind = BuyPyogukSideEffectKind::UpdateMaxPurseMoney;
    max_money.new_pyoguk_num = new_pyoguk_num;
    max_money.new_max_money = new_max_money_after_buy;
    plan.effects.push_back(max_money);

    BuyPyogukSideEffect db{};
    db.kind = BuyPyogukSideEffectKind::InsertPyogukBuyDB;
    db.new_pyoguk_num = new_pyoguk_num;
    plan.effects.push_back(db);

    BuyPyogukSideEffect ack{};
    ack.kind = BuyPyogukSideEffectKind::BroadcastBuyAck;
    ack.new_pyoguk_num = new_pyoguk_num;
    plan.effects.push_back(ack);

    return plan;
}

}  // namespace mxh::server
