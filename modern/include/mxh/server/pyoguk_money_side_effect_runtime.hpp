// pyoguk_money_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// pyoguk_money_put_in_side_effect_plan() /
// pyoguk_money_put_out_side_effect_plan(). The data plane returns a
// 6-step success chain, a single NACK (put-in only), or an empty
// silent plan (put-out clamped to 0); this header walks the plan and
// dispatches each entry to a virtual PyogukMoneySideEffectSink.
//
// 1:1 invariants (1:1 with legacy CPyoGukManager::PutInMoneyPyoguk /
// PutOutMoneyPyoguk from [Server]Map/PyogukManager.cpp:304-395):
//   - PutIn: setMoney = min(setMoney, inven_money,
//     maxmon - pyoguk_money); if clamped to 0 -> NACK.
//   - PutOut: getMoney = min(getMoney, pyoguk_money,
//     inven_max - inven_money); if clamped to 0 -> SILENT (legacy
//     has no put-out NACK).
//   - On non-zero: success chain in legacy order -- subtract source,
//     add target, PyogukMoneyUpdateToDB, InsertLogMoney, LogItemMoney,
//     broadcast ACK with dwData = remaining pyoguk money.
//   - Log codes: put-in InsertLogMoney=21 / LogItemMoney=43;
//     put-out InsertLogMoney=22 / LogItemMoney=44.
//
// Pattern mirrors item_move_side_effect_runtime.hpp (D4.43) and the
// rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/pyoguk_money_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the PyogukMoney side-effect chain. The
// player_id is threaded from the handler because the data plane
// carries the purse amounts but not the player identity.
class PyogukMoneySideEffectSink {
public:
    virtual ~PyogukMoneySideEffectSink() = default;

    // Legacy: pPlayer->SetMoney(SUBTRACTION, amount) on the source
    // purse (inven for PutIn / pyoguk for PutOut).
    virtual void subtract_from_source(
        std::uint32_t player_id, PyogukMoneySource source,
        std::uint64_t amount, std::uint64_t new_inven_money,
        std::uint64_t new_pyoguk_money) = 0;

    // Legacy: pPlayer->SetMoney(ADDITION, amount) on the target purse.
    virtual void add_to_target(
        std::uint32_t player_id, PyogukMoneySource source,
        std::uint64_t amount, std::uint64_t new_inven_money,
        std::uint64_t new_pyoguk_money) = 0;

    // Legacy: PyogukMoneyUpdateToDB(user_id, new_pyoguk_mon).
    virtual void update_pyoguk_money_db(
        std::uint32_t player_id, std::uint64_t amount,
        std::uint64_t new_pyoguk_money) = 0;

    // Legacy: InsertLogMoney(eMoneyLog_*, ...).
    virtual void insert_log_money(
        std::uint32_t player_id, std::uint32_t money_log_code,
        std::uint64_t amount, std::uint64_t new_inven_money,
        std::uint64_t new_pyoguk_money) = 0;

    // Legacy: LogItemMoney(..., eLog_ItemMove*, ...).
    virtual void log_item_money(
        std::uint32_t player_id, std::uint32_t item_log_code,
        std::uint64_t amount, std::uint64_t new_inven_money,
        std::uint64_t new_pyoguk_money) = 0;

    // Legacy: pPlayer->SendMsg(MP_PYOGUK_PUT*_MONEY_ACK, dwData =
    // pyoguk_mon).
    virtual void broadcast_ack(
        std::uint32_t player_id, PyogukMoneySource source,
        std::uint64_t amount, std::uint64_t new_pyoguk_money) = 0;

    // Legacy: pPlayer->SendMsg(MP_PYOGUK_PUTIN_MONEY_NACK).
    virtual void broadcast_nack(std::uint32_t player_id,
                                PyogukMoneySource source) = 0;

    // Legacy: put-out clamped to 0 -> silent return (no message).
    virtual void silent_skip(std::uint32_t player_id,
                             PyogukMoneySource source) = 0;
};

struct PyogukMoneyRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t subtractions    = 0;
    std::size_t additions       = 0;
    std::size_t db_updates      = 0;
    std::size_t money_logs      = 0;
    std::size_t item_logs       = 0;
    std::size_t acks_sent       = 0;
    std::size_t nacks_sent      = 0;
    std::size_t silent_skips    = 0;
    bool ack_flag_consumed    = false;
    bool nack_flag_consumed   = false;
    bool silent_flag_consumed = false;
};

// Runtime: walks the plan and dispatches each entry in legacy order.
inline PyogukMoneyRuntimeOutcome apply_pyoguk_money_side_effects(
    std::uint32_t player_id, const PyogukMoneySideEffectPlan& plan,
    PyogukMoneySideEffectSink& sink) {
    PyogukMoneyRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case PyogukMoneySideEffectKind::SubtractFromSource:
            sink.subtract_from_source(
                player_id, plan.source, effect.amount,
                effect.new_inven_money, effect.new_pyoguk_money);
            ++out.subtractions;
            ++out.effects_applied;
            break;
        case PyogukMoneySideEffectKind::AddToTarget:
            sink.add_to_target(
                player_id, plan.source, effect.amount,
                effect.new_inven_money, effect.new_pyoguk_money);
            ++out.additions;
            ++out.effects_applied;
            break;
        case PyogukMoneySideEffectKind::UpdatePyogukMoneyDB:
            sink.update_pyoguk_money_db(player_id, effect.amount,
                                        effect.new_pyoguk_money);
            ++out.db_updates;
            ++out.effects_applied;
            break;
        case PyogukMoneySideEffectKind::InsertLogMoney:
            sink.insert_log_money(
                player_id, effect.money_log_code, effect.amount,
                effect.new_inven_money, effect.new_pyoguk_money);
            ++out.money_logs;
            ++out.effects_applied;
            break;
        case PyogukMoneySideEffectKind::LogItemMoney:
            sink.log_item_money(
                player_id, effect.item_log_code, effect.amount,
                effect.new_inven_money, effect.new_pyoguk_money);
            ++out.item_logs;
            ++out.effects_applied;
            break;
        case PyogukMoneySideEffectKind::BroadcastAck:
            sink.broadcast_ack(player_id, plan.source, effect.amount,
                               effect.new_pyoguk_money);
            ++out.acks_sent;
            ++out.effects_applied;
            break;
        case PyogukMoneySideEffectKind::BroadcastNack:
            sink.broadcast_nack(player_id, plan.source);
            ++out.nacks_sent;
            ++out.effects_applied;
            break;
        case PyogukMoneySideEffectKind::SilentSkip:
            sink.silent_skip(player_id, plan.source);
            ++out.silent_skips;
            ++out.effects_applied;
            break;
        }
    }
    // Put-out clamped to 0: the data plane emits no effect entries;
    // the silent flag alone carries the branch (1:1 with the legacy
    // no-message return).
    if (plan.silent && out.silent_skips == 0u) {
        sink.silent_skip(player_id, plan.source);
        ++out.silent_skips;
    }
    out.ack_flag_consumed = plan.send_ack;
    out.nack_flag_consumed = plan.send_nack;
    out.silent_flag_consumed = plan.silent;
    return out;
}

}  // namespace mxh::server
