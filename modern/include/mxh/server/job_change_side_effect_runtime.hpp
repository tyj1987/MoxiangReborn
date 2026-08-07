// job_change_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// job_change_side_effect_plan(). The data plane returns a single NACK
// (3 gate categories) or a 5-step success chain; this header walks
// the plan and dispatches each entry to a virtual
// JobChangeSideEffectSink.
//
// 1:1 invariants (1:1 with legacy CItemManager::
// MP_ITEM_SHOPITEM_JOBCHANGE_SYN from
// [Server]Map/ItemManager.cpp:5776-5832):
//   - Gate 1: stage must be Hwa(1) or Geuk(2) -> else NACK code 1.
//   - Gate 2: slot + pItem exist, wIconIdx == eIncantation_ChangeJob
//     (36), dwDBIdx == dwData2 -> else NACK code 2.
//   - Gate 3: DiscardItem == EI_TRUE -> else NACK code 3.
//   - Success chain in legacy order: DiscardChangeJobItem ->
//     ChangeCharacterStageAbility -> SetStage(Hwa<->Geuk flip) ->
//     JOBCHANGE_ACK -> LogItemMoney.
//
// Pattern mirrors item_discard_side_effect_runtime.hpp (D4.46) and
// the rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/job_change_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the JobChange side-effect chain.
class JobChangeSideEffectSink {
public:
    virtual ~JobChangeSideEffectSink() = default;

    // Legacy: SendMsg(MP_ITEM_SHOPITEM_JOBCHANGE_ACK).
    virtual void send_ack_to_player(std::uint32_t player_id) = 0;

    // Legacy: SendMsg(MP_ITEM_SHOPITEM_JOBCHANGE_NACK, nack_code).
    virtual void send_nack_to_player(std::uint32_t player_id,
                                     std::uint32_t nack_code) = 0;

    // Legacy: DiscardItem(...) -- consumes the change-job incantation.
    virtual void discard_change_job_item(std::uint32_t player_id) = 0;

    // Legacy: ChangeCharacterStageAbility(player, stage, group) -- DB
    // call swapping the stage ability group.
    virtual void change_character_stage_ability(
        std::uint32_t player_id, std::uint8_t current_stage,
        std::uint32_t ability_group) = 0;

    // Legacy: pPlayer->SetStage(new_stage) -- Hwa->Geuk / Geuk->Hwa.
    virtual void set_stage(std::uint32_t player_id,
                           std::uint8_t current_stage,
                           std::uint8_t new_stage) = 0;

    // Legacy: LogItemMoney(...) for the change-job transaction.
    virtual void log_item_money(std::uint32_t player_id,
                                std::uint8_t current_stage,
                                std::uint8_t new_stage) = 0;
};

struct JobChangeRuntimeOutcome {
    std::size_t effects_applied     = 0;
    std::size_t acks_sent           = 0;
    std::size_t nacks_sent          = 0;
    std::size_t discards            = 0;
    std::size_t ability_changes     = 0;
    std::size_t stage_sets          = 0;
    std::size_t money_logs          = 0;
    bool ack_flag_consumed         = false;
    bool nack_flag_consumed        = false;
    bool discard_flag_consumed     = false;
    bool ability_flag_consumed     = false;
    bool stage_flag_consumed       = false;
    bool log_flag_consumed         = false;
};

// Runtime: walks the plan and dispatches each entry in legacy order.
inline JobChangeRuntimeOutcome apply_job_change_side_effects(
    const JobChangeSideEffectPlan& plan,
    JobChangeSideEffectSink& sink) {
    JobChangeRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case JobChangeSideEffectKind::SendAckToPlayer:
            sink.send_ack_to_player(effect.player_id);
            ++out.acks_sent;
            ++out.effects_applied;
            break;
        case JobChangeSideEffectKind::SendNackToPlayer:
            sink.send_nack_to_player(effect.player_id, effect.nack_code);
            ++out.nacks_sent;
            ++out.effects_applied;
            break;
        case JobChangeSideEffectKind::DiscardChangeJobItem:
            sink.discard_change_job_item(effect.player_id);
            ++out.discards;
            ++out.effects_applied;
            break;
        case JobChangeSideEffectKind::ChangeCharacterStageAbility:
            sink.change_character_stage_ability(
                effect.player_id, effect.current_stage,
                effect.ability_group);
            ++out.ability_changes;
            ++out.effects_applied;
            break;
        case JobChangeSideEffectKind::SetStage:
            sink.set_stage(effect.player_id, effect.current_stage,
                           effect.new_stage);
            ++out.stage_sets;
            ++out.effects_applied;
            break;
        case JobChangeSideEffectKind::LogItemMoney:
            sink.log_item_money(effect.player_id, effect.current_stage,
                                effect.new_stage);
            ++out.money_logs;
            ++out.effects_applied;
            break;
        }
    }
    out.ack_flag_consumed = plan.send_ack;
    out.nack_flag_consumed = plan.send_nack;
    out.discard_flag_consumed = plan.discard_item;
    out.ability_flag_consumed = plan.change_stage_ability;
    out.stage_flag_consumed = plan.set_stage;
    out.log_flag_consumed = plan.log_item_money;
    return out;
}

}  // namespace mxh::server
