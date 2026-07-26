// object_event.cpp - Phase D6 ObjectEvent 1:1 port implementations.
//
// Mirrors legacy [Server]Map/ObjectEvent.cpp routing.  The dispatch
// is intentionally a thin pass-through; state changes (LevelUp
// stat recalc, Die cleanup) are owned by their respective modules.

#include "mxh/server/object_event.hpp"

namespace mxh::server {

namespace {
ObjectEventSink g_sink{};
}  // namespace

bool object_event_dispatch(ObjectEventCode code, Object* obj) {
    if (!g_sink.user_data && !g_sink.on_levelup && !g_sink.on_die
        && !g_sink.on_life_recover_completed) {
        return false;
    }
    switch (code) {
        case ObjectEventCode::LevelUp:
            return g_sink.on_levelup ? g_sink.on_levelup(obj, g_sink.user_data) : false;
        case ObjectEventCode::Die:
            return g_sink.on_die ? g_sink.on_die(obj, g_sink.user_data) : false;
        case ObjectEventCode::LifeRecoverCompleted:
            return g_sink.on_life_recover_completed
                ? g_sink.on_life_recover_completed(obj, g_sink.user_data)
                : false;
    }
    return false;
}

void set_object_event_sink(ObjectEventSink sink) { g_sink = sink; }
ObjectEventSink get_object_event_sink() { return g_sink; }

}  // namespace mxh::server

namespace {
[[maybe_unused]] constexpr int object_event_translation_unit_anchor = 0;
}
