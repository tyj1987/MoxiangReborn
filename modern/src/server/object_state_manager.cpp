// object_state_manager.cpp - Phase D6 ObjectStateManager 1:1 port implementations.

#include "mxh/server/object_state_manager.hpp"

namespace mxh::server {

StartStateResult start_object_state(ObjectState current, ObjectState next) {
    // Legacy Die is terminal for StartObjectState, including an Exit
    // request: the assertion branch returns FALSE before the common
    // OnStartObjectState / SetState path.
    if (current == ObjectState::Die) {
        return StartStateResult::RejectedDieBlocks;
    }

    // Legacy locked states assert for requests other than Die / Exit,
    // but the assertion does not return. The state transition still
    // proceeds and the method returns TRUE. The modern pure function
    // cannot expose the assertion side effect, so every non-Die
    // current state maps to Accepted.
    (void)next;
    return StartStateResult::Accepted;
}

EndStateResult end_object_state(ObjectState current,
                                ObjectState requested,
                                std::uint32_t end_state_count,
                                std::uint32_t now_ms,
                                std::uint32_t& out_end_time,
                                bool& out_b_end_state) {
    if (current != requested) {
        // Legacy: if current is Die, silently return; otherwise assert+return.
        if (current == ObjectState::Die) return EndStateResult::MismatchedButDie;
        return EndStateResult::MismatchedAndNotDie;
    }
    if (end_state_count == 0u) {
        // Immediate clear.
        out_end_time = 0;
        out_b_end_state = false;
        return EndStateResult::EndedImmediate;
    }
    // Delayed: legacy sets State_End_Time = gCurTime + EndStateCount.
    out_end_time = now_ms + end_state_count;
    out_b_end_state = true;
    return EndStateResult::EndedDelayed;
}

}  // namespace mxh::server

namespace {
[[maybe_unused]] constexpr int object_state_manager_translation_unit_anchor = 0;
}
