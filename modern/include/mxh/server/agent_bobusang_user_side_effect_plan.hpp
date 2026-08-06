
//
// D4.107 -- AgentBobusang USER side-effect plan.
//
// 1:1 port of legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_BOBUSANGUserMsgParser (lines 5159-5190). The data plane (classify_bobusang_user)
// decides which action to take; this header captures the ordered side-effect
// list the orchestrator must execute.
//
// Legacy branches:
//   - user not found: return silently (drop).
//   - user is GM but GMPower > eGM_POWER_MASTER: return silently (drop).
//   - default: TransToMapServerMsgParser (raw forward to user->dwMapServerConnectionIndex).
//
// NOTE: Server-side MP_BOBUSANGServerMsgParser is handled separately (it touches
// bobusang manager state via BOBUSANGMGR->SetChannelState; not pure Agent routing).
//

#include <cstdint>
#include <vector>

#include "mxh/server/agent_bobusang_user.hpp"

namespace mxh::server {

// USER side-effect kinds the orchestrator must dispatch in order.
enum class BobusangUserSideEffectKind : std::uint8_t {
    Drop,                                  // no_user or wrong_gm_power
    ForwardRawToMap,                       // TransToMapServerMsgParser
};

struct BobusangUserSideEffect final {
    BobusangUserSideEffectKind kind = BobusangUserSideEffectKind::Drop;
    std::uint8_t protocol = 0u;
    std::uint32_t object_id = 0u;
};

struct BobusangUserSideEffectPlan final {
    std::vector<BobusangUserSideEffect> effects;
    bool dispatched = false;
    bool drop = true;
};

inline bool bobusang_user_effect_targets_map(const BobusangUserSideEffect& e) noexcept {
    return e.kind == BobusangUserSideEffectKind::ForwardRawToMap;
}

inline BobusangUserSideEffectPlan bobusang_user_side_effect_plan(const BobusangUserAction& a) {
    BobusangUserSideEffectPlan plan;
    using K = BobusangUserSideEffectKind;
    using A = BobusangUserActionKind;
    switch (a.kind) {
        case A::drop_no_user:
        case A::drop_wrong_gm_power:
            plan.drop = true;
            plan.effects.push_back({K::Drop, a.protocol, a.object_id});
            return plan;
        case A::forward_to_map:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::ForwardRawToMap, a.protocol, a.object_id});
            return plan;
    }
    return plan;
}

}  // namespace mxh::server