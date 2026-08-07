//
// D4.173 -- AgentNpc side-effect plan.
//
// 1:1 port of the implicit default-branch side-effect applied by every
// legacy category parser in [Server]Agent/AgentNetworkMsgParser.cpp when
// MP_NPC traffic arrives. The default branch is:
//
//   USERINFO* pUserInfo = g_pUserTableForObjectID->FindUser(pTempMsg->dwObjectID);
//   if( pUserInfo )
//       g_Network.Send2User( pUserInfo->dwConnectionIndex, pMsg, dwLength );
//
// The data plane (classify_agent_npc) decides the outcome; this header
// captures the ordered side-effect list the orchestrator must execute.
//
// Side effects:
//   - ForwardToUser: Send2User via the user table keyed on dwObjectID.
//   - Drop: silent no-op (no NACK, no log).

#pragma once

#include <cstdint>
#include <vector>

#include "mxh/server/agent_npc.hpp"

namespace mxh::server {

enum class AgentNpcSideEffectKind : std::uint8_t {
    Drop,
    ForwardToUser,
};

struct AgentNpcSideEffect final {
    AgentNpcSideEffectKind kind = AgentNpcSideEffectKind::Drop;
    std::uint8_t reply_protocol = 0u;
    std::uint32_t connection_index = 0u;
    std::uint32_t object_id = 0u;
};

struct AgentNpcSideEffectPlan final {
    std::vector<AgentNpcSideEffect> effects;
    bool dispatched = false;
    bool drop = true;
};

inline bool npc_effect_targets_user(const AgentNpcSideEffect& e) noexcept {
    return e.kind == AgentNpcSideEffectKind::ForwardToUser;
}

inline AgentNpcSideEffectPlan agent_npc_side_effect_plan(
    const AgentNpcRequest& r,
    std::uint32_t connection_index) {
    AgentNpcSideEffectPlan plan;
    using K = AgentNpcSideEffectKind;
    using O = AgentNpcOutcome;
    const auto outcome = classify_agent_npc(r);
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
