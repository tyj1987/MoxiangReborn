//
// D4.166 -- AgentDebug side-effect plan.
//
// 1:1 port of legacy side-effects applied by [Server]Agent/AgentNetworkMsgParser.cpp
// MP_DebugMsgParser (lines 2837-2854). The data plane (classify_agent_debug)
// decides the outcome; this header captures the ordered side-effect list
// the orchestrator must execute.
//
// Legacy semantics (preserved verbatim):
//   MP_DEBUG_CLIENTASSERT (0):
//       - sprintf temp = "\t%s\t", "coffee tools attacking."
//       - WriteAssertMsg("CLIENT", 0, temp);  // NO network response.
//   default: silent no-op.
//
// Side effects:
//   - LogAssert: WriteAssertMsg side effect; no network I/O.
//   - Drop: silent no-op for unknown protocols or missing payload.

#pragma once

#include <cstdint>
#include <vector>

#include "mxh/server/agent_debug.hpp"

namespace mxh::server {

// Side-effect kinds the AgentDebug dispatcher must execute in order.
enum class AgentDebugSideEffectKind : std::uint8_t {
    Drop,
    LogAssert,
};

struct AgentDebugSideEffect final {
    AgentDebugSideEffectKind kind = AgentDebugSideEffectKind::Drop;
    std::uint8_t reply_protocol = 0u;
    bool payload_present = true;
};

struct AgentDebugSideEffectPlan final {
    std::vector<AgentDebugSideEffect> effects;
    bool dispatched = false;
    bool drop = true;
    bool log_assert = false;
};

inline AgentDebugSideEffectPlan agent_debug_side_effect_plan(
    const AgentDebugOutcome& outcome) {
    AgentDebugSideEffectPlan plan;
    using K = AgentDebugOutcome;
    using S = AgentDebugSideEffectKind;
    switch (outcome) {
        case K::Logged:
            plan.dispatched = true;
            plan.drop = false;
            plan.log_assert = true;
            plan.effects.push_back({S::LogAssert, debug_clientassert, true});
            return plan;
        case K::Dropped:
            plan.drop = true;
            plan.effects.push_back({S::Drop, 0u, false});
            return plan;
    }
    return plan;
}

// Mirror plan-builder: classify-and-build from an AgentDebugRequest.
inline AgentDebugSideEffectPlan agent_debug_user_side_effect_plan(
    const AgentDebugRequest& r) {
    const auto outcome = classify_agent_debug(r);
    return agent_debug_side_effect_plan(outcome);
}

}  // namespace mxh::server
