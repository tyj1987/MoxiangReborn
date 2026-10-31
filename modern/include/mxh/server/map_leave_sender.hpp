#pragma once

#include <cstdint>

namespace mxh::server {

// Which agent->map socket receives the leave notice for one character.
// Route is the current map_routes_ slot. RetainedConnection is the last
// live map-server socket for that map (legacy USERINFO::dwMapServerConnectionIndex).
struct MapLeaveEndpoint {
    bool present = false;
    bool connected = false;
};

enum class MapLeaveForward : std::uint8_t {
    None = 0,
    Route = 1,
    RetainedConnection = 2,
};

// Mirrors AgentHandler::clear_session_routes before the retained-connection
// fallback: a missing or disconnected route slot skips the leave.
inline MapLeaveForward select_map_leave_forward(MapLeaveEndpoint route,
                                                MapLeaveEndpoint retained) noexcept {
    (void)retained;
    if (route.present && route.connected) return MapLeaveForward::Route;
    return MapLeaveForward::None;
}

}  // namespace mxh::server
