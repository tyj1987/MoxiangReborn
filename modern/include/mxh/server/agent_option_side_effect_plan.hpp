//
// D4.165 -- AgentOption side-effect plan.
//
// 1:1 port of legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_OPTIONUserMsgParser (lines 2707-2730). The data plane (classify_option_user)
// decides which action to take; this header captures the ordered side-effect
// list the orchestrator must execute.
//
// Legacy semantics (preserved verbatim):
//   MP_OPTION_SET_SYN (0):
//       - find user by dwConnectionIndex; if not found -> drop silently.
//       - mutate pInfo->GameOption.bNoWhisper / bNoFriend based on MSG_WORD bits.
//       - TransToMapServerMsgParser(dwConnectionIndex, pMsg, dwLength).
//   all other sub-protocols: default TransToMapServerMsgParser forward.
//
// Side effects:
//   - ForwardToMapServer: raw forward to map server with mutated USERINFO state.
//   - Drop: silent no-op when user not found.

#pragma once

#include <cstdint>
#include <vector>

#include "mxh/server/agent_option.hpp"

namespace mxh::server {

// Side-effect kinds the AgentOption dispatcher must execute in order.
enum class OptionSideEffectKind : std::uint8_t {
    Drop,
    ForwardToMapServer,
};

struct OptionSideEffect final {
    OptionSideEffectKind kind = OptionSideEffectKind::Drop;
    std::uint8_t reply_protocol = 0u;
    bool forward_payload = false;
    bool mutate_user_options = false;
    std::uint16_t option_bits = 0u;
};

struct OptionSideEffectPlan final {
    std::vector<OptionSideEffect> effects;
    bool dispatched = false;
    bool drop = true;
};

inline bool option_effect_targets_map(const OptionSideEffect& e) noexcept {
    return e.kind == OptionSideEffectKind::ForwardToMapServer;
}

inline OptionSideEffectPlan option_side_effect_plan(const OptionUserAction& a) {
    OptionSideEffectPlan plan;
    using K = OptionUserActionKind;
    using S = OptionSideEffectKind;
    switch (a.kind) {
        case K::drop_no_user:
            plan.drop = true;
            plan.effects.push_back({S::Drop, a.reply_protocol, false, false, 0u});
            return plan;
        case K::forward_set_syn_to_map:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({S::ForwardToMapServer, a.reply_protocol, a.forward_to_map, true, a.option_bits});
            return plan;
        case K::forward_avatarview_to_map:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({S::ForwardToMapServer, a.reply_protocol, a.forward_to_map, false, a.option_bits});
            return plan;
        case K::forward_default:
            plan.dispatched = true;
            plan.drop = false;
            plan.effects.push_back({S::ForwardToMapServer, a.reply_protocol, a.forward_to_map, false, a.option_bits});
            return plan;
    }
    return plan;
}

}  // namespace mxh::server
