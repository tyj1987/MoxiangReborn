// check_end_time_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect chain emitted by
// check_end_time_side_effect(). The data plane returns an ordered
// list of CheckEndTimeStep entries; this header walks the list and
// dispatches each step to its respective subsystem via virtual
// callback interfaces.
//
// 1:1 invariants (1:1 with legacy CShopItemManager::CheckEndTime):
//   - Steps are applied in the legacy order:
//       1. DiscardItemAttempt  (ITEMMGR->DiscardItem)
//       2. BumpDupCounter      (m_DupXxx++)
//       3. BroadcastUseEnd     (SendMsgDwordToPlayer MP_ITEM_SHOPITEM_USEEND)
//       4. ShopItemDeleteToDB  (ShopItemDeleteToDB)
//       5. LogItemMoney        (LogItemMoney)
//   - The DiscardItemAttempt failure path: legacy ASSERTs and
//     continues. The runtime reports the failure but still applies
//     the remaining steps so the legacy invariant (always broadcast +
//     DB delete + log even on discard failure) is preserved.
//
// Pattern mirrors calc_plus_time_runtime.hpp (D4.28) and
// update_logout_to_db_runtime.hpp (D4.*): data plane in the
// matching header, runtime orchestrator also inline here, tests
// verify behavior through the public surface.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/check_end_time_side_effect.hpp>
#include <mxh/server/shop_item_manager.hpp>

namespace mxh::server {

// Subsystem callbacks for the CheckEndTime side-effect chain.
// Production wires each method to the live subsystem (ITEMMGR,
// Player connection, DB thread, log sink). Tests wire them to
// recording stubs so each test starts with empty call lists.
class CheckEndTimeSideEffectSink {
public:
    virtual ~CheckEndTimeSideEffectSink() = default;

    // Legacy: ITEMMGR->DiscardItem(player, position, wIconIdx, 1).
    // Returns true on success. The runtime records the outcome but
    // always proceeds with the remaining steps (legacy invariant).
    virtual bool discard_item(std::uint16_t position,
                             std::uint16_t w_icon_idx,
                             std::uint64_t db_idx) = 0;

    // Legacy: m_pPlayer->bump_dup_<slot>() -> the dup counter the
    // caller (data plane) associated with this row.
    virtual void bump_dup_counter(ShopItemDupSlot slot,
                                  std::uint64_t db_idx) = 0;

    // Legacy: SendMsgDwordToPlayer(MP_ITEM_SHOPITEM_USEEND, ...).
    // The runtime passes the player_id and the row's wIconIdx + db_idx.
    virtual void broadcast_use_end(std::uint32_t player_id,
                                   std::uint16_t w_icon_idx,
                                   std::uint64_t db_idx) = 0;

    // Legacy: ShopItemDeleteToDB(player_id, dwDBIdx).
    virtual void shop_item_delete_to_db(std::uint32_t player_id,
                                        std::uint64_t db_idx) = 0;

    // Legacy: LogItemMoney(player_id, name, ..., eLog_ShopItemUseEnd).
    virtual void log_item_money(std::uint32_t player_id,
                                std::uint16_t w_icon_idx,
                                std::uint64_t db_idx) = 0;
};

// Outcome counters returned by the runtime so tests can assert
// which subsystems fired and in what order.
struct CheckEndTimeRuntimeOutcome {
    std::size_t steps_applied           = 0;
    std::size_t discard_attempts        = 0;
    std::size_t discard_failures        = 0;
    std::size_t dup_counter_bumps       = 0;
    std::size_t broadcasts              = 0;
    std::size_t db_deletes              = 0;
    std::size_t log_calls               = 0;
    bool        discard_failure_observed = false;
};

// Runtime: walks the step list emitted by the data plane and
// dispatches each step to the sink in legacy order. Returns the
// outcome counters.
//
// Production callers pass the live sink (wired to ITEMMGR / Player /
// DB / log); tests pass a recording sink so each test starts with
// an empty call history.
inline CheckEndTimeRuntimeOutcome apply_check_end_time_side_effects(
    const std::vector<CheckEndTimeStep>& steps,
    std::uint32_t player_id,
    CheckEndTimeSideEffectSink& sink) {
    CheckEndTimeRuntimeOutcome out;
    for (const auto& step : steps) {
        switch (step.kind) {
        case CheckEndTimeStepKind::DiscardItemAttempt: {
            ++out.discard_attempts;
            const bool ok = sink.discard_item(
                step.item_pos, step.w_icon_idx, step.db_idx);
            if (!ok) {
                ++out.discard_failures;
                out.discard_failure_observed = true;
                // Legacy invariant: continue with the remaining steps
                // even when DiscardItem fails (the legacy code ASSERTs
                // and falls through).
            }
            ++out.steps_applied;
            break;
        }
        case CheckEndTimeStepKind::BumpDupCounter:
            sink.bump_dup_counter(step.dup_slot, step.db_idx);
            ++out.dup_counter_bumps;
            ++out.steps_applied;
            break;
        case CheckEndTimeStepKind::BroadcastUseEnd:
            sink.broadcast_use_end(
                player_id, step.w_icon_idx, step.db_idx);
            ++out.broadcasts;
            ++out.steps_applied;
            break;
        case CheckEndTimeStepKind::ShopItemDeleteToDB:
            sink.shop_item_delete_to_db(player_id, step.db_idx);
            ++out.db_deletes;
            ++out.steps_applied;
            break;
        case CheckEndTimeStepKind::LogItemMoney:
            sink.log_item_money(
                player_id, step.w_icon_idx, step.db_idx);
            ++out.log_calls;
            ++out.steps_applied;
            break;
        }
    }
    return out;
}

}  // namespace mxh::server
