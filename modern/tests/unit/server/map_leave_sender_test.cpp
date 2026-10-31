#include "mxh/server/map_leave_sender.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>

namespace {

struct MapPlayer {
    bool present = false;
};

void disconnect_client(bool slot_key_present,
                       mxh::server::MapLeaveEndpoint slot_client,
                       mxh::server::MapLeaveEndpoint default_client,
                       MapPlayer& player) {
    const auto forward = mxh::server::select_map_leave_forward(
        slot_key_present, slot_client, default_client);
    if (forward != mxh::server::MapLeaveForward::None) player.present = false;
}

}  // namespace

TEST(MapLeaveSenderTest, ConnectedRouteForwardsLeave) {
    MapPlayer player{true};
    disconnect_client(true, {true, true}, {true, true}, player);
    EXPECT_FALSE(player.present);
    EXPECT_EQ(mxh::server::select_map_leave_forward(true, {true, true}, {true, true}),
              mxh::server::MapLeaveForward::Slot);
}

TEST(MapLeaveSenderTest, AbsentSlotUsesDefaultMapClient) {
    MapPlayer player{true};
    disconnect_client(false, {false, false}, {true, true}, player);
    EXPECT_FALSE(player.present);
    EXPECT_EQ(mxh::server::select_map_leave_forward(false, {false, false}, {true, true}),
              mxh::server::MapLeaveForward::DefaultMapClient);
}

// connect_extra_map installs map_routes_[map]=nullptr while map_client_
// stays connected. route_for_map returns that null slot and the leave
// is skipped. The player on the default map socket must still be cleared.
TEST(MapLeaveSenderTest, NullSlotFallsBackToDefaultMapClient) {
    MapPlayer player{true};
    const mxh::server::MapLeaveEndpoint null_slot{false, false};
    const mxh::server::MapLeaveEndpoint default_client{true, true};
    disconnect_client(true, null_slot, default_client, player);
    EXPECT_FALSE(player.present);
    EXPECT_EQ(mxh::server::select_map_leave_forward(true, null_slot, default_client),
              mxh::server::MapLeaveForward::DefaultMapClient);
}

TEST(MapLeaveSenderTest, ReleaseWaitsForHeldClaimThenReturns) {
    mxh::server::MapSenderClaimGate gate;
    auto claim = gate.acquire();
    std::atomic<int> phase{0};
    std::thread waiter([&] {
        phase.store(1);
        const bool idle = gate.wait_for_idle(std::chrono::milliseconds(2000));
        phase.store(idle ? 2 : 3);
    });
    const auto start = std::chrono::steady_clock::now();
    while (phase.load() == 0 &&
           std::chrono::steady_clock::now() - start < std::chrono::milliseconds(500)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_EQ(phase.load(), 1);
    claim.reset();
    waiter.join();
    EXPECT_EQ(phase.load(), 2);
}
