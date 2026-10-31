#pragma once

#include <cstdint>

namespace mxh::server {

// One agent->map socket as seen by the leave selector.
// present: the pointer is non-null. connected: is_connected() is true.
struct MapLeaveEndpoint {
    bool present = false;
    bool connected = false;
};

enum class MapLeaveForward : std::uint8_t {
    None = 0,
    Slot = 1,
    DefaultMapClient = 2,
};

// route_for_map (agent_handler.cpp) returns the map_routes_ entry when the
// key exists, including a null client, and returns map_client_ only when
// the key is absent. clear_session_routes then skips the leave when that
// result is null or not connected (the historical line-319 branch).
// A present slot whose client pointer is null therefore does not fall
// back to map_client_, even if the default map socket is still up.
inline MapLeaveForward select_map_leave_forward(
    bool slot_key_present,
    MapLeaveEndpoint slot_client,
    MapLeaveEndpoint default_client) noexcept {
    if (slot_key_present) {
        if (slot_client.present && slot_client.connected) return MapLeaveForward::Slot;
        return MapLeaveForward::None;
    }
    if (default_client.present && default_client.connected)
        return MapLeaveForward::DefaultMapClient;
    return MapLeaveForward::None;
}

}  // namespace mxh::server
