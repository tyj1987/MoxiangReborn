// state_param.hpp - Phase D6 StateParam 1:1 port.
//
// Source-of-truth: legacy [Server]Map/StateParam.h.
// Mirrors legacy CStateParam POD state-transition tracker.  The
// SetState/cur/old/new fields map 1:1 with the legacy layout.

#pragma once

#include <cstdint>

namespace mxh::server {

struct StateParam {
    std::uint32_t stateNew        = 0;
    std::uint32_t stateCur        = 0;
    std::uint32_t stateOld        = 0;
    std::uint32_t stateStartTime  = 0;
    std::uint32_t stateEndTime    = 0;
    std::uint32_t stateMidTime    = 0;
    bool          bStateUpdate    = false;
};

// Default-construct in the legacy style and apply SetState.
void state_param_init(StateParam& s);

// SetState legacy body: stateCur = dwState;
// The full legacy transition also writes stateOld = previous cur,
// stateNew = dwState, and resets bStateUpdate = true.  We provide
// the minimal transition helper here so callers can pin the legacy
// 3-field update behavior.
struct StateTransitionResult {
    std::uint32_t new_state;
    std::uint32_t old_state;
    std::uint32_t cur_state;
    bool          state_updated;
};
StateTransitionResult state_param_set_state(StateParam& s, std::uint32_t dwState);

// Computes elapsed ticks since stateStartTime.
std::uint32_t state_param_elapsed(const StateParam& s, std::uint32_t now_tick);

}  // namespace mxh::server
