// object_state_manager.hpp - Phase D6 ObjectStateManager 1:1 port.
//
// Source-of-truth: legacy [Server]Map/ObjectStateManager.h + .cpp.
// Mirrors legacy CObjectStateManager singleton as a set of pure
// state-machine transition rules over a numeric state byte.
//
// The 1:1 state enum mirrors the complete legacy eObjectState_*
// numeric sequence. Die rejects every StartObjectState request;
// locked states assert on disallowed requests but still continue the
// transition, exactly as the legacy server does.

#pragma once

#include <cstdint>

namespace mxh::server {

// ---- State enum (mirror legacy eObjectState_*) ----
enum class ObjectState : std::uint8_t {
    None                 = 0,
    Enter                = 1,
    Move                 = 2,
    Ungijosik            = 3,
    Tactic               = 4,
    Rest                 = 5,
    Deal                 = 6,
    Exchange             = 7,
    StreetStall_Owner    = 8,
    StreetStall_Guest    = 9,
    PrivateWarehouse     = 10,
    Munpa                = 11,
    SkillStart           = 12,
    SkillSyn             = 13,
    SkillBinding         = 14,
    SkillUsing           = 15,
    SkillDelay           = 16,
    TiedUp_CanMove       = 17,
    TiedUp_CanSkill      = 18,
    TiedUp               = 19,
    Die                  = 20,
    BattleReady          = 21,
    Exit                 = 22,
    Immortal             = 23,
    Society              = 24,
    ItemUse              = 25,
    TournamentReady      = 26,
    TournamentProcess    = 27,
    TournamentEnd        = 28,
    TournamentDead       = 29,
    Engrave              = 30,
    TitanRecall          = 31,
    Max                  = 32,
};

// ---- Result of a StartObjectState transition ----
enum class StartStateResult : std::uint8_t {
    Accepted          = 0,
    RejectedDieBlocks = 1,
    // Retained as an API value for callers compiled against the
    // earlier modern port. Legacy locked-state requests assert but
    // still return TRUE and therefore map to Accepted.
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
