//
// D4 -- AgentPeaceWarMode side-effect plan.
//
// 1:1 port of the implicit default-branch side-effect applied by every
// legacy category parser in [Server]Agent/AgentNetworkMsgParser.cpp when
// MP_MP_PEACEWARMODE traffic arrives. The default branch is:
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

#include "mxh/server/agent_peacewarmode.hpp"

namespace mxh::server {

enum class AgentPeaceWarModeSideEffectKind : std::uint8_t {
    Drop,
    ForwardToUser,
};

struct AgentPeaceWarModeSideEffect final {
    AgentPeaceWarModeSideEffectKind kind = AgentPeaceWarModeSideEffectKind::Drop;
    std::uint8_t reply_protocol = 0u;
    std::uint32_t connection_index = 0u;
    std::uint32_t object_id = 0u;
};

struct AgentPeaceWarModeSideEffectPlan final {
    std::vector<AgentPeaceWarModeSideEffect> effects;
    bool dispatched = false;
    bool drop = true;
};

inline bool mp_peacewarmode_effect_targets_user(const AgentPeaceWarModeSideEffect& e) noexcept {
    return e.kind == AgentPeaceWarModeSideEffectKind::ForwardToUser;
}

inline AgentPeaceWarModeSideEffectPlan agent_peacewarmode_side_effect_plan(
    const AgentPeaceWarModeRequest& r,
    std::uint32_t connection_index) {
    AgentPeaceWarModeSideEffectPlan plan;
    using K = AgentPeaceWarModeSideEffectKind;
    using O = AgentPeaceWarModeOutcome;
    const auto outcome = classify_agent_peacewarmode(r);
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
