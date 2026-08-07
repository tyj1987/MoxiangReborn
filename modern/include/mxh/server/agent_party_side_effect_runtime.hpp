// agent_party_side_effect_runtime.hpp
//
// Runtime orchestrator for the side-effect plan emitted by
// agent_party_side_effect_plan(). The data plane returns an empty
// plan (no target) or a single routing entry; this header walks the
// plan and dispatches each entry to a virtual AgentPartySideEffectSink.
//
// 1:1 invariants (1:1 with legacy agent party handlers):
//   - 10 notify actions -> BroadcastToOtherMapServers.
//   - RequestConsentAck: target map forward (54) / consent NACK (55).
//   - RequestRefusalAck: target user forward (57) / refusal NACK (58).
//   - Error: target user forward with MP_PARTY_ERROR(50).
//   - MatchingInfo: client forward (60).
//   - MasterToRequestSyn: master map forward (51) / not-master error.
//
// Pattern mirrors agent_friend_side_effect_runtime.hpp (D4.96) and
// the rest of the runtime orchestrator family.

#pragma once

#include <cstdint>
#include <vector>

#include <mxh/server/agent_party_side_effect.hpp>

namespace mxh::server {

// Subsystem callbacks for the AgentParty side-effect chain.
class AgentPartySideEffectSink {
public:
    virtual ~AgentPartySideEffectSink() = default;

    virtual void broadcast_to_other_map_servers(std::uint32_t object_id) = 0;
    virtual void send_to_target_map_server(
        std::uint32_t object_id, std::uint32_t alternate_object_id,
        std::uint8_t protocol) = 0;
    virtual void send_to_target_user(
        std::uint32_t object_id, std::uint32_t alternate_object_id,
        std::uint8_t protocol) = 0;
    virtual void send_consent_nack(std::uint32_t alternate_object_id,
                                   std::uint8_t protocol) = 0;
    virtual void send_refusal_nack(std::uint32_t alternate_object_id,
                                   std::uint8_t protocol) = 0;
    virtual void send_party_error(std::uint32_t object_id,
                                  std::uint32_t error_code,
                                  std::uint8_t protocol) = 0;
    virtual void send_to_client(std::uint32_t object_id,
                                std::uint8_t protocol) = 0;
};

struct AgentPartyRuntimeOutcome {
    std::size_t effects_applied = 0;
    std::size_t broadcasts      = 0;
    std::size_t map_forwards    = 0;
    std::size_t user_forwards   = 0;
    std::size_t consent_nacks   = 0;
    std::size_t refusal_nacks   = 0;
    std::size_t party_errors    = 0;
    std::size_t client_forwards = 0;
    bool dispatched_flag_consumed = false;
    bool broadcast_flag_consumed  = false;
    bool map_flag_consumed        = false;
    bool client_flag_consumed     = false;
    bool nack_flag_consumed       = false;
};

// Runtime: walks the plan and dispatches each entry in legacy order.
inline AgentPartyRuntimeOutcome apply_agent_party_side_effects(
    const AgentPartySideEffectPlan& plan,
    AgentPartySideEffectSink& sink) {
    AgentPartyRuntimeOutcome out;
    for (const auto& effect : plan.effects) {
        switch (effect.kind) {
        case AgentPartySideEffectKind::BroadcastToOtherMapServers:
            sink.broadcast_to_other_map_servers(effect.object_id);
            ++out.broadcasts;
            ++out.effects_applied;
            break;
        case AgentPartySideEffectKind::SendToTargetMapServer:
            sink.send_to_target_map_server(
                effect.object_id, effect.alternate_object_id,
                effect.protocol);
            ++out.map_forwards;
            ++out.effects_applied;
            break;
        case AgentPartySideEffectKind::SendToTargetUser:
            sink.send_to_target_user(
                effect.object_id, effect.alternate_object_id,
                effect.protocol);
            ++out.user_forwards;
            ++out.effects_applied;
            break;
        case AgentPartySideEffectKind::SendConsentNack:
            sink.send_consent_nack(effect.object_id,
                                   effect.protocol);
            ++out.consent_nacks;
            ++out.effects_applied;
            break;
        case AgentPartySideEffectKind::SendRefusalNack:
            sink.send_refusal_nack(effect.object_id,
                                   effect.protocol);
            ++out.refusal_nacks;
            ++out.effects_applied;
            break;
        case AgentPartySideEffectKind::SendPartyError:
            sink.send_party_error(effect.object_id, effect.error_code,
                                  effect.protocol);
            ++out.party_errors;
            ++out.effects_applied;
            break;
        case AgentPartySideEffectKind::SendToClient:
            sink.send_to_client(effect.object_id, effect.protocol);
            ++out.client_forwards;
            ++out.effects_applied;
            break;
        }
    }
    out.dispatched_flag_consumed = plan.dispatched;
    out.broadcast_flag_consumed = plan.broadcast_to_maps;
    out.map_flag_consumed = plan.forward_to_map;
    out.client_flag_consumed = plan.forward_to_client;
    out.nack_flag_consumed = plan.send_nack;
    return out;
}

}  // namespace mxh::server
