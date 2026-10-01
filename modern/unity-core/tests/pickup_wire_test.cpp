#include "PickupWire.hpp"
#include <gtest/gtest.h>
#include <array>
#include <vector>

TEST(UnityPickupWire, AcceptedResponsePreservesFullIdentityAndQuantity) {
    const std::array<std::uint8_t, 8> bytes{0x78, 0x56, 0x34, 0x12, 0xff, 0xff, 0xff, 0xff};
    const auto response = mxh::unity::decode_pickup_response(bytes, true);
    ASSERT_TRUE(response);
    EXPECT_EQ(response->drop_id, 0x12345678u);
    EXPECT_EQ(response->item_id, 65535u);
    EXPECT_EQ(response->count, 65535u);
}
TEST(UnityPickupWire, RejectionPreservesDropWithoutInventingAnItem) {
    const std::array<std::uint8_t, 8> bytes{41, 0, 0, 0, 0, 0, 0, 0};
    const auto response = mxh::unity::decode_pickup_response(bytes, false);
    ASSERT_TRUE(response);
    EXPECT_EQ(response->drop_id, 41u);
    EXPECT_EQ(response->item_id, 0u);
    EXPECT_EQ(response->count, 0u);
    EXPECT_FALSE(mxh::unity::decode_pickup_response(bytes, true));
}
TEST(UnityPickupWire, RejectsEveryWrongLengthForBothOutcomes) {
    for (std::size_t length = 0; length <= 16; ++length) {
        if (length == 8) continue;
        const std::vector<std::uint8_t> bytes(length, 1);
        EXPECT_FALSE(mxh::unity::decode_pickup_response(bytes, true));
        EXPECT_FALSE(mxh::unity::decode_pickup_response(bytes, false));
    }
}
TEST(UnityPickupWire, RejectsZeroIdentityAndIncompleteSuccess) {
    for (const auto bytes : {std::array<std::uint8_t,8>{0,0,0,0,9,0,1,0},
                            std::array<std::uint8_t,8>{41,0,0,0,0,0,1,0},
                            std::array<std::uint8_t,8>{41,0,0,0,9,0,0,0}})
        EXPECT_FALSE(mxh::unity::decode_pickup_response(bytes, true));
    EXPECT_FALSE(mxh::unity::decode_pickup_response(std::array<std::uint8_t,8>{}, false));
}
TEST(UnityPickupWire, RejectionCannotAwardItemOrQuantity) {
    for (const auto bytes : {std::array<std::uint8_t,8>{41,0,0,0,9,0,0,0},
                            std::array<std::uint8_t,8>{41,0,0,0,0,0,1,0},
                            std::array<std::uint8_t,8>{41,0,0,0,9,0,1,0}})
        EXPECT_FALSE(mxh::unity::decode_pickup_response(bytes, false));
}
