#include "mxh/server/map_leave_sender.hpp"

#include <gtest/gtest.h>

namespace {

struct MapPlayer {
    bool present = false;
};

// Applies one client disconnect to the map player entry. The multiplexed
// agent-map socket is not the socket that closed, so the entry changes
// only when a leave is forwarded.
void disconnect_client(mxh::server::MapLeaveEndpoint route,
                       mxh::server::MapLeaveEndpoint retained,
                       MapPlayer& player) {
    const auto forward = mxh::server::select_map_leave_forward(route, retained);
    if (forward != mxh::server::MapLeaveForward::None) player.present = false;
}

}  // namespace

TEST(MapLeaveSenderTest, ConnectedRouteForwardsLeave) {
    MapPlayer player{true};
    disconnect_client({true, true}, {false, false}, player);
    EXPECT_FALSE(player.present);
    EXPECT_EQ(mxh::server::select_map_leave_forward({true, true}, {false, false}),
              mxh::server::MapLeaveForward::Route);
}

TEST(MapLeaveSenderTest, MissingRouteStillClearsRetainedMapPlayer) {
    // Route slot cleared. The map server connection recorded on the user
    // is still up, so the player entry has to be cleared on client RST.
    MapPlayer player{true};
    const mxh::server::MapLeaveEndpoint route{false, false};
    const mxh::server::MapLeaveEndpoint retained{true, true};
    disconnect_client(route, retained, player);
    EXPECT_FALSE(player.present);
    EXPECT_EQ(mxh::server::select_map_leave_forward(route, retained),
              mxh::server::MapLeaveForward::RetainedConnection);
}
