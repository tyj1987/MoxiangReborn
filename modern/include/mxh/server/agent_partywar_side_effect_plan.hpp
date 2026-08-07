//
// D4 -- AgentPartyWar side-effect plan.
//
// 1:1 port of the implicit default-branch side-effect applied by every
// legacy category parser in [Server]Agent/AgentNetworkMsgParser.cpp when
// MP_PARTYWAR traffic arrives. The default branch is:
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

#include "mxh/server/agent_partywar.hpp"

namespace mxh::server {

enum class AgentPartyWarSideEffectKind : std::uint8_t {
    Drop,
    ForwardToUser,
};

struct AgentPartyWarSideEffect final {
    AgentPartyWarSideEffectKind kind = AgentPartyWarSideEffectKind::Drop;
    std::uint8_t reply_protocol = 0u;
    std::uint32_t connection_index = 0u;
    std::uint32_t object_id = 0u;
};

struct AgentPartyWarSideEffectPlan final {
    std::vector<AgentPartyWarSideEffect> effects;
    bool dispatched = false;
    bool drop = true;
};

inline bool partywar_effect_targets_user(const AgentPartyWarSideEffect& e) noexcept {
    return e.kind == AgentPartyWarSideEffectKind::ForwardToUser;
}

inline AgentPartyWarSideEffectPlan agent_partywar_side_effect_plan(
    const AgentPartyWarRequest& r,
    std::uint32_t connection_index) {
    AgentPartyWarSideEffectPlan plan;
    using K = AgentPartyWarSideEffectKind;
    using O = AgentPartyWarOutcome;
    const auto outcome = classify_agent_partywar(r);
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
