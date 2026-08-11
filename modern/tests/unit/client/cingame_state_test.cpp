#include "CInGameState.hpp"
#include "mxh/game/hero_total_layout.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <vector>

using mxh::client::parse_legacy_gamein_ack;
using mxh::client::parse_legacy_monster_add;
using mxh::client::parse_legacy_mugong_total;
using mxh::client::parse_legacy_item_total;
using mxh::client::parse_legacy_npc_add;

TEST(InGameGameInAck, DecodesCurrentLegacyLayout) {
    std::vector<std::uint8_t> payload(mxh::game::HERO_TOTAL_EMPTY_PAYLOAD_SIZE, 0);
    const std::uint32_t player_id = 42;
    const std::uint32_t user_id = 84;
    const std::uint32_t life = 900;
    const std::uint32_t max_life = 1000;
    const std::uint16_t level = 33;
    const std::uint16_t map_num = 12;
    const std::uint16_t position_x = 25000;
    const std::uint16_t position_z = 25000;
    std::memcpy(payload.data(), &player_id, sizeof(player_id));
    std::memcpy(payload.data() + 4, &user_id, sizeof(user_id));
    std::memcpy(payload.data() + 8, "Hero", 5);
    std::memcpy(payload.data() + 35, &life, sizeof(life));
    std::memcpy(payload.data() + 39, &max_life, sizeof(max_life));
    payload[51] = 1;
    payload[52] = 3;
    payload[53] = 4;
    for (std::size_t slot = 0; slot < 10; ++slot) {
        const auto value = static_cast<std::uint16_t>(1000 + slot);
        std::memcpy(payload.data() + 54 + slot * 2, &value, sizeof(value));
    }
    std::memcpy(payload.data() + 75, &level, sizeof(level));
    std::memcpy(payload.data() + 77, &map_num, sizeof(map_num));
    const std::uint32_t mp = 1200;
    const std::uint32_t max_mp = 1500;
    const std::uint32_t exp = 980;
    const std::uint32_t money = 123456;
    std::memcpy(payload.data() + mxh::game::HERO_TOTAL_HERO_OFFSET + 8,
                &mp, sizeof(mp));
    std::memcpy(payload.data() + mxh::game::HERO_TOTAL_HERO_OFFSET + 12,
                &max_mp, sizeof(max_mp));
    std::memcpy(payload.data() + mxh::game::HERO_TOTAL_HERO_OFFSET + 20,
                &exp, sizeof(exp));
    std::memcpy(payload.data() + mxh::game::HERO_TOTAL_HERO_OFFSET + 30,
                &money, sizeof(money));
    std::memcpy(payload.data() + mxh::game::HERO_TOTAL_MOVE_OFFSET, &position_x, sizeof(position_x));
    std::memcpy(payload.data() + mxh::game::HERO_TOTAL_MOVE_OFFSET + 2, &position_z, sizeof(position_z));

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
    EXPECT_EQ(info->face_type, 3u);
    EXPECT_EQ(info->hair_type, 4u);
    EXPECT_EQ(info->weared_item_idx.front(), 1000u);
    EXPECT_EQ(info->weared_item_idx.back(), 1009u);
    EXPECT_EQ(info->level, level);
    EXPECT_EQ(info->map_num, map_num);
    EXPECT_EQ(info->mp, mp);
    EXPECT_EQ(info->max_mp, max_mp);
    EXPECT_EQ(info->exp, exp);
    EXPECT_EQ(info->money, money);
    EXPECT_EQ(info->position_x, position_x);
    EXPECT_EQ(info->position_z, position_z);
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
    payload[49] = 0x34;
    payload[50] = 0x12;
    payload[51] = 0x78;
    payload[52] = 0x56;
    const auto info = parse_legacy_monster_add(payload);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->object_id, 0x10u);
    EXPECT_EQ(info->user_id, 0x20u);
    EXPECT_STREQ(info->name, "Monster0");
    EXPECT_EQ(info->current_life, 100u);
    EXPECT_EQ(info->monster_kind, 7u);
    EXPECT_EQ(info->map_num, 12u);
    EXPECT_EQ(info->position_x, 0x1234u);
    EXPECT_EQ(info->position_z, 0x5678u);
}

TEST(InGameMonsterAdd, RejectsShortPayload) {
    std::array<std::uint8_t, 32> payload{};
    EXPECT_FALSE(parse_legacy_monster_add(payload).has_value());
}

TEST(InGameMugong, DecodesMugongTotalFromGameInAck) {
    std::vector<std::uint8_t> payload(
        mxh::game::HERO_TOTAL_EMPTY_PAYLOAD_SIZE, 0);
    const auto off = mxh::game::HERO_TOTAL_MUGONG_OFFSET;
    const std::uint32_t db_idx = 9001;
    const std::uint16_t icon_idx = 7;
    const std::uint16_t position = 3;
    const std::uint32_t exp = 100;
    payload[off + 0] = db_idx & 0xFF;
    payload[off + 1] = (db_idx >> 8) & 0xFF;
    payload[off + 4] = icon_idx & 0xFF;
    payload[off + 5] = (icon_idx >> 8) & 0xFF;
    payload[off + 6] = position & 0xFF;
    payload[off + 8] = exp & 0xFF;
    payload[off + 12] = 1;  // sung
    payload[off + 13] = 1;  // wear
    payload[off + 14] = 5;  // quick_position
    payload[off + 16] = 9;  // option_idx

    const auto mugong = parse_legacy_mugong_total(payload);
    EXPECT_EQ(mugong[0].db_idx, db_idx);
    EXPECT_EQ(mugong[0].icon_idx, icon_idx);
    EXPECT_EQ(mugong[0].position, position);
    EXPECT_EQ(mugong[0].exp, exp);
    EXPECT_EQ(mugong[0].sung, 1u);
    EXPECT_EQ(mugong[0].wear, 1u);
    EXPECT_EQ(mugong[0].quick_position, 5u);
    EXPECT_EQ(mugong[0].option_idx, 9u);
    EXPECT_EQ(mugong[1].db_idx, 0u);  // untouched slot stays zero
}

TEST(InGameMugong, ShortPayloadReturnsZeros) {
    std::array<std::uint8_t, 64> payload{};
    const auto mugong = parse_legacy_mugong_total(payload);
    EXPECT_EQ(mugong[0].db_idx, 0u);
}

TEST(InGameItem, DecodesItemTotalFromGameInAck) {
    std::vector<std::uint8_t> payload(
        mxh::game::HERO_TOTAL_EMPTY_PAYLOAD_SIZE, 0);
    const auto off = mxh::game::HERO_TOTAL_ITEM_OFFSET;
    const std::uint32_t db_idx = 123;
    const std::uint16_t icon_idx = 456;
    std::memcpy(payload.data() + off + 0, &db_idx, sizeof(db_idx));
    std::memcpy(payload.data() + off + 4, &icon_idx, sizeof(icon_idx));
    const std::uint32_t dur = 50;
    std::memcpy(payload.data() + off + 8, &dur, sizeof(dur));

    const auto items = parse_legacy_item_total(payload);
    EXPECT_FALSE(mxh::game::is_empty_slot(items.Inventory[0]));
    EXPECT_EQ(items.Inventory[0].dwDBIdx, db_idx);
    EXPECT_EQ(items.Inventory[0].wIconIdx, icon_idx);
    EXPECT_EQ(items.Inventory[0].Durability, dur);
    EXPECT_TRUE(mxh::game::is_empty_slot(items.Inventory[1]));
}

TEST(InGameItem, ShortPayloadReturnsEmptyItems) {
    std::array<std::uint8_t, 64> payload{};
    const auto items = parse_legacy_item_total(payload);
    EXPECT_TRUE(mxh::game::is_empty_slot(items.Inventory[0]));
}

TEST(InGameNpc, DecodesNpcAddPayload) {
    std::array<std::uint8_t, 64> payload{};
    payload[0] = 0x2A;
    std::memcpy(payload.data() + 8, "Merchant", 9);
    payload[35] = 0x10;
    payload[45] = 0x34;
    payload[46] = 0x12;
    payload[47] = 0x78;
    payload[48] = 0x56;

    const auto npc = parse_legacy_npc_add(payload);
    ASSERT_TRUE(npc.has_value());
    EXPECT_EQ(npc->npc_id, 0x2Au);
    EXPECT_STREQ(npc->name, "Merchant");
    EXPECT_EQ(npc->npc_kind, 0x10u);
    EXPECT_EQ(npc->position_x, 0x1234u);
    EXPECT_EQ(npc->position_z, 0x5678u);
}

TEST(InGameNpc, RejectsShortPayload) {
    std::array<std::uint8_t, 32> payload{};
    EXPECT_FALSE(parse_legacy_npc_add(payload).has_value());
}
