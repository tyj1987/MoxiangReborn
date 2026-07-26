// object_state_manager.cpp - Phase D6 ObjectStateManager 1:1 port implementations.

#include "mxh/server/object_state_manager.hpp"

namespace mxh::server {

namespace {
bool is_locked_state(ObjectState s) {
    switch (s) {
        case ObjectState::Ungijosik:
        case ObjectState::Exchange:
        case ObjectState::StreetStall_Owner:
        case ObjectState::StreetStall_Guest:
        case ObjectState::Deal:
        case ObjectState::Move:
        case ObjectState::Tactic:
        case ObjectState::TiedUp:
            return true;
        default:
            return false;
    }
}
}  // namespace

StartStateResult start_object_state(ObjectState current, ObjectState next) {
    // Legacy ObjectStateManager::StartObjectState transition table.
    if (current == ObjectState::Die) {
        // Die rejects all except Exit.
        if (next != ObjectState::Exit) return StartStateResult::RejectedDieBlocks;
        return StartStateResult::Accepted;
    }
    if (is_locked_state(current)) {
        // Locked states reject all except Die / Exit.
        if (next != ObjectState::Die && next != ObjectState::Exit) {
            return StartStateResult::RejectedLocked;
        }
    }
    return StartStateResult::Accepted;
}

EndStateResult end_object_state(ObjectState current,
                                ObjectState requested,
                                std::uint32_t end_state_count,
                                std::uint32_t now_ms,
                                std::uint32_t& out_end_time,
                                bool& out_b_end_state) {
    out_end_time = 0;
    out_b_end_state = false;
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
