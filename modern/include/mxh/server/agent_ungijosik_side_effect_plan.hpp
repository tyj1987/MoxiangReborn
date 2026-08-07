//
// D4 -- AgentUngiJosik side-effect plan.
//
// 1:1 port of the implicit default-branch side-effect applied by every
// legacy category parser in [Server]Agent/AgentNetworkMsgParser.cpp when
// MP_MP_UNGIJOSIK traffic arrives. The default branch is:
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

#include "mxh/server/agent_ungijosik.hpp"

namespace mxh::server {

enum class AgentUngiJosikSideEffectKind : std::uint8_t {
    Drop,
    ForwardToUser,
};

struct AgentUngiJosikSideEffect final {
    AgentUngiJosikSideEffectKind kind = AgentUngiJosikSideEffectKind::Drop;
    std::uint8_t reply_protocol = 0u;
    std::uint32_t connection_index = 0u;
    std::uint32_t object_id = 0u;
};

struct AgentUngiJosikSideEffectPlan final {
    std::vector<AgentUngiJosikSideEffect> effects;
    bool dispatched = false;
    bool drop = true;
};

inline bool mp_ungijosik_effect_targets_user(const AgentUngiJosikSideEffect& e) noexcept {
    return e.kind == AgentUngiJosikSideEffectKind::ForwardToUser;
}

inline AgentUngiJosikSideEffectPlan agent_ungijosik_side_effect_plan(
    const AgentUngiJosikRequest& r,
    std::uint32_t connection_index) {
    AgentUngiJosikSideEffectPlan plan;
    using K = AgentUngiJosikSideEffectKind;
    using O = AgentUngiJosikOutcome;
    const auto outcome = classify_agent_ungijosik(r);
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
