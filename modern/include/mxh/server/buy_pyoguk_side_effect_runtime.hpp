// buy_pyoguk_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// buy_pyoguk_side_effect_plan(). The data plane returns a 5-step
// success chain or a single NACK (cap or money gate); this header
// walks the plan and dispatches each entry to a virtual
// BuyPyogukSideEffectSink.
//
// 1:1 invariants (1:1 with legacy CPyoGukManager::BuyPyogukSyn from
// [Server]Map/PyogukManager.cpp:201-302):
//   - Gate 1: pyoguknum >= TAB_PYOGUK_NUM(5) -> NACK (cap).
//   - Gate 2: inven money < BuyPrice -> NACK.
//   - Success chain in legacy order -- subtract BuyPrice, increment
//     pyoguknum, set MaxPurseMoney for the new pyoguknum,
//     PyogukBuyPyoguk DB helper, broadcast MP_PYOGUK_BUY_ACK with
//     bData = new pyoguknum.
//
// Pattern mirrors pyoguk_money_side_effect_runtime.hpp (D4.44) and
// the rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/buy_pyoguk_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the BuyPyoguk side-effect chain. The
// player_id is threaded from the handler because the data plane
// carries the purse/price numbers but not the player identity.
class BuyPyogukSideEffectSink {
public:
    virtual ~BuyPyogukSideEffectSink() = default;

    // Legacy: pPlayer->SetMoney(SUBTRACTION, BuyPrice,
    // eMoneyLog_LosePyogukBuy).
    virtual void subtract_buy_price(std::uint32_t player_id,
                                    std::uint64_t buy_price) = 0;

    // Legacy: pPlayer->SetPyogukNum(pyoguknum + 1).
    virtual void increment_pyoguk_num(std::uint32_t player_id,
                                      std::uint8_t new_pyoguk_num) = 0;

    // Legacy: pPlayer->SetMaxPurseMoney(eItemTable_Pyoguk, MaxMoney).
    virtual void update_max_purse_money(
        std::uint32_t player_id, std::uint8_t new_pyoguk_num,
        std::uint64_t new_max_money) = 0;

    // Legacy: PyogukBuyPyoguk(player_id).
    virtual void insert_pyoguk_buy_db(std::uint32_t player_id,
                                      std::uint8_t new_pyoguk_num) = 0;

    // Legacy: pPlayer->SendMsg(MP_PYOGUK_BUY_ACK, bData =
    // new_pyoguknum).
    virtual void broadcast_buy_ack(std::uint32_t player_id,
                                   std::uint8_t new_pyoguk_num) = 0;

    // Legacy: pPlayer->SendMsg(MP_PYOGUK_BUY_NACK).
    virtual void broadcast_buy_nack(std::uint32_t player_id) = 0;
};

struct BuyPyogukRuntimeOutcome {
    std::size_t effects_applied   = 0;
    std::size_t subtractions      = 0;
    std::size_t increments        = 0;
    std::size_t max_money_updates = 0;
    std::size_t db_inserts        = 0;
    std::size_t acks_sent         = 0;
    std::size_t nacks_sent        = 0;
    bool ack_flag_consumed  = false;
    bool nack_flag_consumed = false;
};

// Runtime: walks the plan and dispatches each entry in legacy order.
inline BuyPyogukRuntimeOutcome apply_buy_pyoguk_side_effects(
    std::uint32_t player_id, const BuyPyogukSideEffectPlan& plan,
    BuyPyogukSideEffectSink& sink) {
    BuyPyogukRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case BuyPyogukSideEffectKind::SubtractBuyPrice:
            sink.subtract_buy_price(player_id, effect.buy_price);
            ++out.subtractions;
            ++out.effects_applied;
            break;
        case BuyPyogukSideEffectKind::IncrementPyogukNum:
            sink.increment_pyoguk_num(player_id, effect.new_pyoguk_num);
            ++out.increments;
            ++out.effects_applied;
            break;
        case BuyPyogukSideEffectKind::UpdateMaxPurseMoney:
            sink.update_max_purse_money(
                player_id, effect.new_pyoguk_num, effect.new_max_money);
            ++out.max_money_updates;
            ++out.effects_applied;
            break;
        case BuyPyogukSideEffectKind::InsertPyogukBuyDB:
            sink.insert_pyoguk_buy_db(player_id, effect.new_pyoguk_num);
            ++out.db_inserts;
            ++out.effects_applied;
            break;
        case BuyPyogukSideEffectKind::BroadcastBuyAck:
            sink.broadcast_buy_ack(player_id, effect.new_pyoguk_num);
            ++out.acks_sent;
            ++out.effects_applied;
            break;
        case BuyPyogukSideEffectKind::BroadcastBuyNack:
            sink.broadcast_buy_nack(player_id);
            ++out.nacks_sent;
            ++out.effects_applied;
            break;
        }
    }
    out.ack_flag_consumed = plan.send_ack;
    out.nack_flag_consumed = plan.send_nack;
    return out;
}

}  // namespace mxh::server
