// agent_guild_notify_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// agent_guild_notify_side_effect_plan(). The data plane returns an
// empty plan (no user / filtered) or the action's effect chain (1-4
// entries); this header walks the plan and dispatches each entry to a
// virtual AgentGuildNotifySideEffectSink.
//
// 1:1 invariants (1:1 with legacy agent guild notify handlers):
//   - MunpaJoin: CopyNoteBuffers -> FilterCheckGuildName ->
//     NoteServerSendtoPlayer (+ SendJoinMasterAlram when the master
//     is online).
//   - Munha change/join: SendMunhaMasterAlram.
//   - MunpaDelete: the 3-step note chain.
//   - GuildCreate: ForwardToMapServer / SendCreateNack(err=5).
//   - GuildGiveNickname: ForwardToMapServer / SendNickNack(err=4).
//
// Pattern mirrors agent_note_side_effect_runtime.hpp (D4.93) and the
// rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/agent_guild_notify_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the AgentGuildNotify side-effect chain.
class AgentGuildNotifySideEffectSink {
public:
    virtual ~AgentGuildNotifySideEffectSink() = default;

    virtual void copy_note_buffers(std::uint32_t object_id) = 0;
    virtual void filter_check_guild_name(std::uint32_t object_id) = 0;
    virtual void note_server_sendto_player(std::uint32_t object_id) = 0;
    virtual void send_join_master_alram(std::uint32_t object_id,
                                        std::uint32_t master_id) = 0;
    virtual void send_munha_master_alram(std::uint32_t object_id) = 0;
    virtual void send_create_nack(std::uint32_t object_id,
                                  std::uint8_t nack_code) = 0;
    virtual void send_nick_nack(std::uint32_t object_id,
                                std::uint8_t nack_code) = 0;
    virtual void forward_to_map_server(std::uint32_t object_id) = 0;
};

struct AgentGuildNotifyRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t copies          = 0;
    std::size_t filters         = 0;
    std::size_t note_sends      = 0;
    std::size_t join_alarms     = 0;
    std::size_t munha_alarms    = 0;
    std::size_t create_nacks    = 0;
    std::size_t nick_nacks      = 0;
    std::size_t forwards        = 0;
    bool dispatched_flag_consumed = false;
    bool forward_flag_consumed    = false;
    bool nack_flag_consumed       = false;
};

// Runtime: walks the plan and dispatches each entry in legacy order.
inline AgentGuildNotifyRuntimeOutcome apply_agent_guild_notify_side_effects(
    const AgentGuildNotifySideEffectPlan& plan,
    AgentGuildNotifySideEffectSink& sink) {
    AgentGuildNotifyRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case AgentGuildNotifySideEffectKind::CopyNoteBuffers:
            sink.copy_note_buffers(effect.object_id);
            ++out.copies;
            ++out.effects_applied;
            break;
        case AgentGuildNotifySideEffectKind::FilterCheckGuildName:
            sink.filter_check_guild_name(effect.object_id);
            ++out.filters;
            ++out.effects_applied;
            break;
        case AgentGuildNotifySideEffectKind::NoteServerSendtoPlayer:
            sink.note_server_sendto_player(effect.object_id);
            ++out.note_sends;
            ++out.effects_applied;
            break;
        case AgentGuildNotifySideEffectKind::SendJoinMasterAlram:
            sink.send_join_master_alram(effect.object_id,
                                        effect.master_id);
            ++out.join_alarms;
            ++out.effects_applied;
            break;
        case AgentGuildNotifySideEffectKind::SendMunhaMasterAlram:
            sink.send_munha_master_alram(effect.object_id);
            ++out.munha_alarms;
            ++out.effects_applied;
            break;
        case AgentGuildNotifySideEffectKind::SendCreateNack:
            sink.send_create_nack(effect.object_id, effect.nack_code);
            ++out.create_nacks;
            ++out.effects_applied;
            break;
        case AgentGuildNotifySideEffectKind::SendNickNack:
            sink.send_nick_nack(effect.object_id, effect.nack_code);
            ++out.nick_nacks;
            ++out.effects_applied;
            break;
        case AgentGuildNotifySideEffectKind::ForwardToMapServer:
            sink.forward_to_map_server(effect.object_id);
            ++out.forwards;
            ++out.effects_applied;
            break;
        }
    }
    out.dispatched_flag_consumed = plan.dispatched;
    out.forward_flag_consumed = plan.forward_to_map;
    out.nack_flag_consumed = plan.send_nack;
    return out;
}

}  // namespace mxh::server
