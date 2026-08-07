//
// D4 -- AgentSignal side-effect plan.
//
// 1:1 port of the implicit default-branch side-effect applied by every
// legacy category parser in [Server]Agent/AgentNetworkMsgParser.cpp when
// MP_MP_SIGNAL traffic arrives. The default branch is:
//
//   USERINFO* pUserInfo = g_pUserTableForObjectID->FindUser(pTempMsg->dwObjectID);
//   if( pUserInfo )
//       g_Network.Send2User( pUserInfo->dwConnectionIndex, pMsg, dwLength );
//
// Side effects:
//   - ForwardToUser: Send2User via the user table keyed on dwObjectID.
//   - Drop: silent no-op (no NACK, no log).

#pragma once

#include <cstdint>
#include <vector>

#include "mxh/server/agent_signal.hpp"

namespace mxh::server {

enum class AgentSignalSideEffectKind : std::uint8_t {
    Drop,
    ForwardToUser,
};

struct AgentSignalSideEffect final {
    AgentSignalSideEffectKind kind = AgentSignalSideEffectKind::Drop;
    std::uint8_t reply_protocol = 0u;
    std::uint32_t connection_index = 0u;
    std::uint32_t object_id = 0u;
};

struct AgentSignalSideEffectPlan final {
    std::vector<AgentSignalSideEffect> effects;
    bool dispatched = false;
    bool drop = true;
};

inline bool mp_signal_effect_targets_user(const AgentSignalSideEffect& e) noexcept {
    return e.kind == AgentSignalSideEffectKind::ForwardToUser;
}

inline AgentSignalSideEffectPlan agent_signal_side_effect_plan(
    const AgentSignalRequest& r,
    std::uint32_t connection_index) {
    AgentSignalSideEffectPlan plan;
    using K = AgentSignalSideEffectKind;
    using O = AgentSignalOutcome;
    const auto outcome = classify_agent_signal(r);
    switch (outcome) {
        case O::ForwardToUser:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.reserve(1u);
            plan.effects.push_back(
                {K::ForwardToUser, r.protocol, connection_index, r.object_id});
            return plan;
        case O::DropNoUser:
            plan.drop = true;
            plan.effects.reserve(1u);
            plan.effects.push_back(
                {K::Drop, r.protocol, 0u, r.object_id});
            return plan;
    }
    return plan;
}

}  // namespace mxh::server
