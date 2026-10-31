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

// Original agent logout does not look the map up again. It sends
// MP_USERCONN_DISCONNECTED on USERINFO::dwMapServerConnectionIndex
// (墨香【源码】/[Server]Map/UserTable.cpp:390-397, _AGENTSERVER
// CUserTable::OnDisconnectUser). A cleared route slot must still use
// that retained connection when it is up.
inline MapLeaveForward select_map_leave_forward(MapLeaveEndpoint route,
                                                MapLeaveEndpoint retained) noexcept {
    if (route.present && route.connected) return MapLeaveForward::Route;
    if (retained.present && retained.connected) return MapLeaveForward::RetainedConnection;
    return MapLeaveForward::None;
}

}  // namespace mxh::server
