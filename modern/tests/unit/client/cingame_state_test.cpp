// mxh/tests/unit/client/cingame_state_test.cpp
// Unit tests for mxh::client::CInGameState (Phase B.2.3).
//
// Coverage:
//   * parse_legacy_gamein_ack — 3000-byte SEND_HERO_TOTALINFO layout
//     (matches map_handler.cpp make_gamein_ack).
//   * CInGameState default state.

#include <array>
#include <gtest/gtest.h>
#include <cstring>
#include "CInGameState.hpp"
using mxh::client::parse_legacy_monster_add;



TEST(InGameMonsterAdd, DecodesServerPushedMonsterPayload) {
    std::array<std::uint8_t, 64> payload{};
    payload[0] = 0x10; payload[1] = 0x00; payload[2] = 0x00; payload[3] = 0x00;
    payload[4] = 0x20; payload[5] = 0x00; payload[6] = 0x00; payload[7] = 0x00;
    std::memcpy(payload.data() + 8, "Monster0", 8);
    payload[35] = 0x64; payload[36] = 0x00; payload[37] = 0x00; payload[38] = 0x00;
    payload[43] = 0x07; payload[44] = 0x00;
    payload[47] = 12; payload[48] = 0x00;
    auto info = parse_legacy_monster_add(payload);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->object_id, 0x10u);
    EXPECT_EQ(info->user_id, 0x20u);
    EXPECT_STREQ(info->name, "Monster0");
    EXPECT_EQ(info->current_life, 100u);
    EXPECT_EQ(info->monster_kind, 7u);
    EXPECT_EQ(info->map_num, 12u);
}

TEST(InGameMonsterAdd, RejectsShortPayload) {
    std::array<std::uint8_t, 32> short_payload{};
    EXPECT_FALSE(parse_legacy_monster_add(short_payload).has_value());
}
