// object_state_manager.hpp - Phase D6 ObjectStateManager 1:1 port.
//
// Source-of-truth: legacy [Server]Map/ObjectStateManager.h + .cpp.
// Mirrors legacy CObjectStateManager singleton as a set of pure
// state-machine transition rules over a numeric state byte.
//
// The 1:1 state enum mirrors legacy eObjectState_*.  Locked states
// (Ungijosik..Tactic) reject most transitions; Die rejects
// everything except Exit; the unlocked state accepts all.

#pragma once

#include <cstdint>

namespace mxh::server {

// ---- State enum (mirror legacy eObjectState_*) ----
enum class ObjectState : std::uint8_t {
    None                 = 0,
    Enter                = 1,
    Die                  = 2,
    Exit                 = 2,  // legacy alias of Die for transition table
    Ungijosik            = 3,
    Exchange             = 4,
    StreetStall_Owner    = 5,
    StreetStall_Guest    = 6,
    Deal                 = 7,
    Move                 = 8,
    Tactic               = 9,
    TiedUp               = 10,
};

// ---- Result of a StartObjectState transition ----
enum class StartStateResult : std::uint8_t {
    Accepted          = 0,
    RejectedDieBlocks = 1,
    RejectedLocked    = 2,
};

// ---- Result of an EndObjectState transition ----
enum class EndStateResult : std::uint8_t {
    EndedImmediate = 0,   // cleared now, EndStateCount==0
    EndedDelayed   = 1,   // bEndState set, State_End_Time updated
    MismatchedAndNotDie = 2,  // legacy: asserts and returns
    MismatchedButDie    = 3,  // legacy: silently returns
};

StartStateResult start_object_state(ObjectState current, ObjectState next);

EndStateResult end_object_state(ObjectState current,
                                ObjectState requested,
                                std::uint32_t end_state_count,
                                std::uint32_t now_ms,
                                std::uint32_t& out_end_time,
                                bool& out_b_end_state);

}  // namespace mxh::server
