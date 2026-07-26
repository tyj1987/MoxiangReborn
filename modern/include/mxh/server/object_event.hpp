// object_event.hpp - Phase D6 ObjectEvent 1:1 port.
//
// Source-of-truth: legacy [Server]Map/ObjectEvent.h + .cpp.
// Mirrors legacy CObjectEvent singleton as a free function over a
// numeric event code.  The dispatch itself (ObjectEvent -> state
// changes) is left to a future commit that wires the receivers in;
// here we expose the wire-level event enum so callers can route
// events consistently.

#pragma once

#include <cstdint>

namespace mxh::server {

// ---- Event code enum (mirror legacy eOBJECTEVENT) ----
// 1:1 byte ordering with legacy OE_LEVELUP=0, OE_DIE=1, ...
enum class ObjectEventCode : std::uint8_t {
    LevelUp                 = 0,
    Die                     = 1,
    LifeRecoverCompleted    = 2,
};

// ---- Routing ----
// ObjectEvent is the legacy entry point used by CObject::Release,
// CCharacterCalcManager::LevelUp, etc.  In modern it becomes a
// pure notification function the server framework can hook.
struct Object;
struct ObjectEventSink {
    // Receivers return true to acknowledge the event.
    bool (*on_levelup)(Object* obj, void* user_data) = nullptr;
    bool (*on_die)(Object* obj, void* user_data) = nullptr;
    bool (*on_life_recover_completed)(Object* obj, void* user_data) = nullptr;
    void* user_data = nullptr;
};

// Dispatch a legacy ObjectEvent to the active sink.  Returns true if
// a sink was registered and acknowledged the event.
bool object_event_dispatch(ObjectEventCode code, Object* obj);

// Test/inspection helpers.
void set_object_event_sink(ObjectEventSink sink);
ObjectEventSink get_object_event_sink();

}  // namespace mxh::server
