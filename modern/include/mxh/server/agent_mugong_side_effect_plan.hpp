
//
// D4.117 -- AgentMugong side-effect plan.
//
// 1:1 port of legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_MUGONG. The data plane (classify_mugong) decides which action to take;
// this header captures the ordered side-effect list the orchestrator must execute.
//
// Legacy branches:
//   - option_syn (19) with required_level>0 AND mugong_level<required_level: send_nack
//     (Protocol=OPTION_NACK=21, dwData=1) to user->dwConnectionIndex.
//   - default: forward_to_map (TransToMapServerMsgParser raw forward).
//
// Side effects:
//   - send_nack: build MSG_DWORD (Protocol=OPTION_NACK, dwData=error_code) to user.
//   - forward_to_map: raw forward MSG_DWORD to user->dwMapServerConnectionIndex.
//

#include <cstdint>
#include <vector>

#include "mxh/server/agent_mugong.hpp"

namespace mxh::server {

// Side-effect kinds the orchestrator must dispatch in order.
enum class MugongSideEffectKind : std::uint8_t {
    ForwardRawToMap,                       // raw forward MSG_DWORD to user->dwMapServerConnectionIndex
    SendOptionNackToUser,                  // MSG_DWORD (Protocol=OPTION_NACK, dwData=error_code)
};

struct MugongSideEffect final {
    MugongSideEffectKind kind = MugongSideEffectKind::ForwardRawToMap;
    std::uint8_t reply_protocol = 0u;
    std::uint32_t object_id = 0u;
    std::uint8_t error_code = 0u;
};

struct MugongSideEffectPlan final {
    std::vector<MugongSideEffect> effects;
    bool dispatched = false;
    bool drop = true;
};

inline bool mugong_effect_targets_map(const MugongSideEffect& e) noexcept {
    return e.kind == MugongSideEffectKind::ForwardRawToMap;
}

inline bool mugong_effect_targets_user(const MugongSideEffect& e) noexcept {
    return e.kind == MugongSideEffectKind::SendOptionNackToUser;
}

inline MugongSideEffectPlan mugong_side_effect_plan(const MugongAction& a) {
    MugongSideEffectPlan plan;
    using K = MugongSideEffectKind;
    using A = MugongActionKind;
    switch (a.kind) {
        case A::send_nack:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::SendOptionNackToUser, a.protocol, a.object_id, a.error_code});
            return plan;
        case A::forward_to_map:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::ForwardRawToMap, a.protocol, a.object_id, 0u});
            return plan;
        case A::forward_to_map_if_level_ok:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({K::ForwardRawToMap, a.protocol, a.object_id, 0u});
            return plan;
    }
    return plan;
}

}  // namespace mxh::server