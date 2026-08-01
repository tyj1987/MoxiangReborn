#include "CInGameState.hpp"
#include "mxh/game/hero_total_layout.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <vector>

using mxh::client::parse_legacy_gamein_ack;
using mxh::client::parse_legacy_monster_add;

TEST(InGameGameInAck, DecodesCurrentLegacyLayout) {
    std::vector<std::uint8_t> payload(mxh::game::HERO_TOTAL_EMPTY_PAYLOAD_SIZE, 0);
    const std::uint32_t player_id = 42;
    const std::uint32_t user_id = 84;
    const std::uint32_t life = 900;
    const std::uint32_t max_life = 1000;
    const std::uint16_t level = 33;
    const std::uint16_t map_num = 12;
    std::memcpy(payload.data(), &player_id, sizeof(player_id));
    std::memcpy(payload.data() + 4, &user_id, sizeof(user_id));
    std::memcpy(payload.data() + 8, "Hero", 5);
    std::memcpy(payload.data() + 35, &life, sizeof(life));
    std::memcpy(payload.data() + 39, &max_life, sizeof(max_life));
    payload[51] = 1;
    std::memcpy(payload.data() + 75, &level, sizeof(level));
    std::memcpy(payload.data() + 77, &map_num, sizeof(map_num));

    const std::uint16_t year = 2026;
    const std::uint16_t month = 8;
    const std::uint16_t day = 1;
    const std::uint16_t hour = 12;
    const auto time_offset = mxh::game::HERO_TOTAL_SERVER_TIME_OFFSET;
    std::memcpy(payload.data() + time_offset, &year, sizeof(year));
    std::memcpy(payload.data() + time_offset + 2, &month, sizeof(month));
    std::memcpy(payload.data() + time_offset + 6, &day, sizeof(day));
    std::memcpy(payload.data() + time_offset + 8, &hour, sizeof(hour));

    const auto info = parse_legacy_gamein_ack(payload);

    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->player_id, player_id);
    EXPECT_EQ(info->user_id, user_id);
    EXPECT_EQ(info->name, "Hero");
    EXPECT_EQ(info->life, 900u);
    EXPECT_EQ(info->max_life, 1000u);
    EXPECT_EQ(info->gender, 1u);
    EXPECT_EQ(info->level, level);
    EXPECT_EQ(info->map_num, map_num);
    EXPECT_EQ(info->server_year, year);
    EXPECT_EQ(info->server_month, month);
    EXPECT_EQ(info->server_day, day);
    EXPECT_EQ(info->server_hour, hour);
}

TEST(InGameGameInAck, RejectsPreTitanPayloadSize) {
    std::vector<std::uint8_t> payload(mxh::game::HERO_TOTAL_EMPTY_PAYLOAD_SIZE - 1, 0);
    EXPECT_FALSE(parse_legacy_gamein_ack(payload).has_value());
}

TEST(InGameMonsterAdd, DecodesServerPushedMonsterPayload) {
    std::array<std::uint8_t, 64> payload{};
    payload[0] = 0x10;
    payload[4] = 0x20;
    std::memcpy(payload.data() + 8, "Monster0", 8);
    payload[35] = 0x64;
    payload[43] = 0x07;
    payload[47] = 12;
    const auto info = parse_legacy_monster_add(payload);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->object_id, 0x10u);
    EXPECT_EQ(info->user_id, 0x20u);
    EXPECT_STREQ(info->name, "Monster0");
    EXPECT_EQ(info->current_life, 100u);
    EXPECT_EQ(info->monster_kind, 7u);
    EXPECT_EQ(info->map_num, 12u);
}

TEST(InGameMonsterAdd, RejectsShortPayload) {
    std::array<std::uint8_t, 32> payload{};
    EXPECT_FALSE(parse_legacy_monster_add(payload).has_value());
}
