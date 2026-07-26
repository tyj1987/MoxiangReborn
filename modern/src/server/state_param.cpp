// state_param.cpp - Phase D6 StateParam 1:1 port.

#include "mxh/server/state_param.hpp"

namespace mxh::server {

void state_param_init(StateParam& s) {
    s.stateNew        = 0u;
    s.stateCur        = 0u;
    s.stateOld        = 0u;
    s.stateStartTime  = 0u;
    s.stateEndTime    = 0u;
    s.stateMidTime    = 0u;
    s.bStateUpdate    = false;
}

StateTransitionResult state_param_set_state(StateParam& s, std::uint32_t dwState) {
    // Legacy transition semantics: stateOld = previous cur, stateNew = new,
    // stateCur = new, bStateUpdate = true.
    StateTransitionResult r{};
    s.stateOld     = s.stateCur;
    s.stateNew     = dwState;
    s.stateCur     = dwState;
    s.bStateUpdate = true;
    r.new_state     = s.stateNew;
    r.old_state     = s.stateOld;
    r.cur_state     = s.stateCur;
    r.state_updated = s.bStateUpdate;
    return r;
}

std::uint32_t state_param_elapsed(const StateParam& s, std::uint32_t now_tick) {
    if (now_tick < s.stateStartTime) return 0u;
    return now_tick - s.stateStartTime;
}

}  // namespace mxh::server

namespace {
[[maybe_unused]] constexpr int state_param_translation_unit_anchor = 0;
}
