#pragma once

#include <cstdint>
#include <vector>

namespace mxh::server {

inline constexpr std::uint8_t LEGACY_PARTY_REQUEST_CONSENT_NACK = 55u;
inline constexpr std::uint8_t LEGACY_PARTY_REQUEST_REFUSAL_NACK = 58u;
inline constexpr std::uint8_t LEGACY_PARTY_ERROR = 50u;
inline constexpr std::uint32_t LEGACY_EERR_REQUEST_NOT_MASTER = 1u;

enum class AgentPartyAction : std::uint8_t {
    NotifyAddToMapServer,
    NotifyDeleteToMapServer,
    NotifyChangeMasterToMapServer,
    NotifyBreakupToMapServer,
    NotifyBanToMapServer,
    NotifyMemberLoginToMapServer,
    NotifyMemberLogoutToMapServer,
    NotifyMemberLoginMessage,
    NotifyCreateToMapServer,
    NotifyMemberLevel,
    RequestConsentAck,
    RequestRefusalAck,
    Error,
    MatchingInfo,
    MasterToRequestSyn,
};

enum class AgentPartyOutcome : std::uint8_t {
    BroadcastedToOtherMaps,
    ForwardedToTargetMap,
    ForwardedToTargetUser,
    SentConsentNack,
    SentRefusalNack,
    SentPartyError,
    ForwardedToClient,
    NoTarget,
};

struct AgentPartyValidationInput final {
    AgentPartyAction action = AgentPartyAction::NotifyAddToMapServer;
    bool object_found = false;
    bool alternate_found = false;
    bool master_found = false;
    bool requester_found = false;
};

inline AgentPartyOutcome classify_agent_party_outcome(
    const AgentPartyValidationInput& input) noexcept {
    switch (input.action) {
        case AgentPartyAction::NotifyAddToMapServer:
        case AgentPartyAction::NotifyDeleteToMapServer:
        case AgentPartyAction::NotifyChangeMasterToMapServer:
        case AgentPartyAction::NotifyBreakupToMapServer:
        case AgentPartyAction::NotifyBanToMapServer:
        case AgentPartyAction::NotifyMemberLoginToMapServer:
        case AgentPartyAction::NotifyMemberLogoutToMapServer:
        case AgentPartyAction::NotifyMemberLoginMessage:
        case AgentPartyAction::NotifyCreateToMapServer:
        case AgentPartyAction::NotifyMemberLevel:
            return AgentPartyOutcome::BroadcastedToOtherMaps;
        case AgentPartyAction::RequestConsentAck:
            if (input.object_found) return AgentPartyOutcome::ForwardedToTargetMap;
            if (input.alternate_found) return AgentPartyOutcome::SentConsentNack;
            return AgentPartyOutcome::NoTarget;
        case AgentPartyAction::RequestRefusalAck:
            if (input.object_found) return AgentPartyOutcome::ForwardedToTargetUser;
            if (input.alternate_found) return AgentPartyOutcome::SentRefusalNack;
            return AgentPartyOutcome::NoTarget;
        case AgentPartyAction::Error:
            if (input.object_found) return AgentPartyOutcome::ForwardedToTargetUser;
            return AgentPartyOutcome::NoTarget;
        case AgentPartyAction::MatchingInfo:
            return AgentPartyOutcome::ForwardedToClient;
        case AgentPartyAction::MasterToRequestSyn:
            if (input.master_found) return AgentPartyOutcome::ForwardedToTargetMap;
            if (input.requester_found) return AgentPartyOutcome::SentPartyError;
            return AgentPartyOutcome::NoTarget;
    }
    return AgentPartyOutcome::NoTarget;
}

enum class AgentPartySideEffectKind : std::uint8_t {
    BroadcastToOtherMapServers,
    SendToTargetMapServer,
    SendToTargetUser,
    SendConsentNack,
    SendRefusalNack,
    SendPartyError,
    SendToClient,
};

struct AgentPartySideEffect final {
    AgentPartySideEffectKind kind =
        AgentPartySideEffectKind::BroadcastToOtherMapServers;
    std::uint32_t object_id = 0;
    std::uint32_t alternate_object_id = 0;
    std::uint32_t error_code = 0;
    std::uint8_t protocol = 0;
};

struct AgentPartySideEffectPlan final {
    std::vector<AgentPartySideEffect> effects;
    bool dispatched = false;
    bool broadcast_to_maps = false;
    bool forward_to_map = false;
    bool forward_to_client = false;
    bool send_nack = false;
};

inline AgentPartySideEffectPlan agent_party_side_effect_plan(
    const AgentPartyValidationInput& input,
    std::uint32_t object_id,
    std::uint32_t alternate_object_id) {
    AgentPartySideEffectPlan plan;
    const AgentPartyOutcome outcome = classify_agent_party_outcome(input);

    switch (input.action) {
        case AgentPartyAction::NotifyAddToMapServer:
        case AgentPartyAction::NotifyDeleteToMapServer:
        case AgentPartyAction::NotifyChangeMasterToMapServer:
        case AgentPartyAction::NotifyBreakupToMapServer:
        case AgentPartyAction::NotifyBanToMapServer:
        case AgentPartyAction::NotifyMemberLoginToMapServer:
        case AgentPartyAction::NotifyMemberLogoutToMapServer:
        case AgentPartyAction::NotifyMemberLoginMessage:
        case AgentPartyAction::NotifyCreateToMapServer:
        case AgentPartyAction::NotifyMemberLevel:
            if (outcome != AgentPartyOutcome::BroadcastedToOtherMaps) return plan;
            plan.dispatched = true;
            plan.broadcast_to_maps = true;
            plan.effects.push_back({AgentPartySideEffectKind::BroadcastToOtherMapServers, object_id, 0u, 0u, 0u});
            return plan;
        case AgentPartyAction::RequestConsentAck:
            if (outcome == AgentPartyOutcome::ForwardedToTargetMap) {
                plan.dispatched = true;
                plan.forward_to_map = true;
                plan.effects.push_back({AgentPartySideEffectKind::SendToTargetMapServer, object_id, alternate_object_id, 0u, 54u});
                return plan;
            }
            if (outcome == AgentPartyOutcome::SentConsentNack) {
                plan.send_nack = true;
                plan.effects.push_back({AgentPartySideEffectKind::SendConsentNack, alternate_object_id, 0u, 0u, LEGACY_PARTY_REQUEST_CONSENT_NACK});
            }
            return plan;
        case AgentPartyAction::RequestRefusalAck:
            if (outcome == AgentPartyOutcome::ForwardedToTargetUser) {
                plan.dispatched = true;
                plan.effects.push_back({AgentPartySideEffectKind::SendToTargetUser, object_id, alternate_object_id, 0u, 57u});
                return plan;
            }
            if (outcome == AgentPartyOutcome::SentRefusalNack) {
                plan.send_nack = true;
                plan.effects.push_back({AgentPartySideEffectKind::SendRefusalNack, alternate_object_id, 0u, 0u, LEGACY_PARTY_REQUEST_REFUSAL_NACK});
            }
            return plan;
        case AgentPartyAction::Error:
            if (outcome != AgentPartyOutcome::ForwardedToTargetUser) return plan;
            plan.dispatched = true;
            plan.effects.push_back({AgentPartySideEffectKind::SendToTargetUser, object_id, 0u, 0u, LEGACY_PARTY_ERROR});
            return plan;
        case AgentPartyAction::MatchingInfo:
            if (outcome != AgentPartyOutcome::ForwardedToClient) return plan;
            plan.dispatched = true;
            plan.forward_to_client = true;
            plan.effects.push_back({AgentPartySideEffectKind::SendToClient, object_id, 0u, 0u, 60u});
            return plan;
        case AgentPartyAction::MasterToRequestSyn:
            if (outcome == AgentPartyOutcome::ForwardedToTargetMap) {
                plan.dispatched = true;
                plan.forward_to_map = true;
                plan.effects.push_back({AgentPartySideEffectKind::SendToTargetMapServer, alternate_object_id, object_id, 0u, 51u});
                return plan;
            }
            if (outcome == AgentPartyOutcome::SentPartyError) {
                plan.send_nack = true;
                plan.effects.push_back({AgentPartySideEffectKind::SendPartyError, object_id, 0u, LEGACY_EERR_REQUEST_NOT_MASTER, LEGACY_PARTY_ERROR});
            }
            return plan;
    }
    return plan;
}

}  // namespace mxh::server
