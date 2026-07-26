// ai_define.hpp - Phase D6 AIDefine 1:1 port.
//
// Source-of-truth: legacy [Server]Map/AIDefine.h.
// Mirrors legacy macros (BEGIN_STATE, END_STATE, STATEOF, etc.) and
// the enums eStateEvent, eMONSTER_EMOTION, enumMESSAGEKINDS.
//
// The macros in legacy use computed gotos inside giant switch
// statements; modern collapses them to enum tags so callers can
// dispatch via plain switch/case.  The semantic ordering (event
// -> state -> message) is preserved exactly so the test suite can
// pin it.

#pragma once

#include <cstdint>

namespace mxh::server {

// ---- Event code (mirror legacy eStateEvent) ----
enum class StateEvent : std::uint8_t {
    Null      = 0,
    Process   = 1,
    Message   = 2,
    Enter     = 3,
    Leave     = 4,
};

// ---- Emotion (mirror legacy eMONSTER_EMOTION) ----
enum class MonsterEmotion : std::uint8_t {
    Pleasure = 0,
    Comfort  = 1,
    Doze     = 2,
    Sad      = 3,
    Anger    = 4,
};

// ---- Message kind (mirror legacy enumMESSAGEKINDS) ----
enum class MessageKind : std::uint8_t {
    Chat            = 0,
    RecallDirect    = 1,
    RecallScript    = 2,
    PlayerCurPos    = 3,
    MobCurPos       = 4,
    HelpRequest     = 5,
    HelpShout       = 6,
    HelpObey        = 7,
};

}  // namespace mxh::server
