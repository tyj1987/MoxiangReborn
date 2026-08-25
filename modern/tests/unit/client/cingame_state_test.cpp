#include "CInGameState.hpp"
#include "mxh/game/hero_total_layout.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <vector>

using mxh::client::parse_legacy_gamein_ack;
using mxh::client::parse_legacy_character_add;
using mxh::client::parse_legacy_monster_add;
using mxh::client::parse_legacy_mugong_total;
using mxh::client::parse_legacy_item_total;
using mxh::client::parse_legacy_npc_add;

TEST(InGameQuestWire, BuildsLegacyTwoByteQuestRequest) {
    const auto message = mxh::client::make_quest_message(
        1234u, mxh::proto::QuestProtocol::StartSyn, 0x2345u);
    EXPECT_EQ(message.header.category,
              static_cast<std::uint8_t>(mxh::proto::Category::Quest));
    EXPECT_EQ(message.header.protocol,
              static_cast<std::uint8_t>(mxh::proto::QuestProtocol::StartSyn));
    EXPECT_EQ(message.header.object_id, 1234u);
    ASSERT_EQ(message.payload.size(), 2u);
    EXPECT_EQ(message.payload[0], 0x45u);
    EXPECT_EQ(message.payload[1], 0x23u);
}

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
    const std::array<std::uint16_t, 4> attributes{111, 222, 333, 444};
    for (std::size_t i = 0; i < attributes.size(); ++i) {
        std::memcpy(payload.data() + mxh::game::HERO_TOTAL_HERO_OFFSET + i * 2,
                    &attributes[i], sizeof(attributes[i]));
    }
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
    EXPECT_EQ(info->gen_gol, attributes[0]);
    EXPECT_EQ(info->min_chub, attributes[1]);
    EXPECT_EQ(info->che_ryuk, attributes[2]);
    EXPECT_EQ(info->sim_mek, attributes[3]);
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

TEST(InGameEffects, ConfirmedSkillMessagesProduceDeterministicTimeline) {
    mxh::client::CInGameState state;

    mxh::net::Message start;
    start.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Skill);
    start.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::SkillProtocol::StartAck);
    start.header.object_id = 90001u;
    start.payload.resize(8);
    const std::uint32_t skill_id = 42u;
    const std::uint32_t skill_object = 70001u;
    std::memcpy(start.payload.data(), &skill_id, sizeof(skill_id));
    std::memcpy(start.payload.data() + 4, &skill_object, sizeof(skill_object));
    state.on_message({}, start);

    mxh::net::Message result;
    result.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Skill);
    result.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::SkillProtocol::SingleResult);
    result.header.object_id = 90001u;
    result.payload.resize(9);
    const std::uint32_t target = 50001u;
    const std::int32_t damage = 123;
    std::memcpy(result.payload.data(), &target, sizeof(target));
    std::memcpy(result.payload.data() + 4, &damage, sizeof(damage));
    result.payload[8] = 1u;
    state.on_message({}, result);

    ASSERT_EQ(state.effect_events().size(), 3u);
    EXPECT_EQ(state.effect_events()[0].kind,
              mxh::client::EffectEventKind::CastRelease);
    EXPECT_EQ(state.effect_events()[0].skill_id, skill_id);
    EXPECT_EQ(state.effect_events()[0].source_object_id, 90001u);
    EXPECT_EQ(state.effect_events()[0].target_object_id, skill_object);
    EXPECT_EQ(state.effect_events()[1].kind,
              mxh::client::EffectEventKind::Hit);
    EXPECT_EQ(state.effect_events()[1].target_object_id, target);
    EXPECT_EQ(state.effect_events()[1].source_object_id, 90001u);
    EXPECT_EQ(state.effect_events()[1].damage, damage);
    EXPECT_EQ(state.effect_events()[2].kind,
              mxh::client::EffectEventKind::End);
    const auto drained = state.drain_effect_events();
    ASSERT_EQ(drained.size(), 3u);
    EXPECT_TRUE(state.effect_events().empty());
}

TEST(InGameEffects, RemoteSkillObjectBroadcastProducesCastTimeline) {
    mxh::client::CInGameState state;
    mxh::net::Message add;
    add.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Skill);
    add.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::SkillProtocol::SkillObjectAdd);
    add.payload.resize(22, 0);
    const std::uint32_t skill_id = 43u;
    const std::uint32_t object_id = 70002u;
    const std::uint32_t caster_id = 90002u;
    std::memcpy(add.payload.data(), &skill_id, 4);
    std::memcpy(add.payload.data() + 4, &object_id, 4);
    std::memcpy(add.payload.data() + 8, &caster_id, 4);
    state.on_message({}, add);
    ASSERT_EQ(state.effect_events().size(), 1u);
    EXPECT_EQ(state.effect_events()[0].kind,
              mxh::client::EffectEventKind::CastStart);
    EXPECT_EQ(state.effect_events()[0].skill_id, skill_id);
    EXPECT_EQ(state.effect_events()[0].source_object_id, caster_id);
    EXPECT_EQ(state.effect_events()[0].target_object_id, object_id);

    mxh::net::Message remove;
    remove.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Skill);
    remove.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::SkillProtocol::SkillObjectRemove);
    remove.header.object_id = object_id;
    state.on_message({}, remove);
    ASSERT_EQ(state.effect_events().size(), 2u);
    EXPECT_EQ(state.effect_events()[1].kind,
              mxh::client::EffectEventKind::End);
    EXPECT_EQ(state.effect_events()[1].target_object_id, object_id);
}

TEST(InGameCharacterAdd, DecodesServerPushedPlayerPayload) {
    std::array<std::uint8_t, 288> payload{};
    const std::uint32_t objectId = 0x12345678u;
    const std::uint32_t userId = 0x87654321u;
    const std::uint32_t life = 912u;
    const std::uint32_t maxLife = 1000u;
    const std::uint16_t level = 33u;
    const std::uint16_t mapNum = 12u;
    const std::uint16_t positionX = 0x2345u;
    const std::uint16_t positionZ = 0x6789u;
    const float height = 170.0f;
    const float width = 60.0f;
    std::memcpy(payload.data(), &objectId, sizeof(objectId));
    std::memcpy(payload.data() + 4, &userId, sizeof(userId));
    std::memcpy(payload.data() + 8, "RemoteHero", 11);
    std::memcpy(payload.data() + 35, &life, sizeof(life));
    std::memcpy(payload.data() + 39, &maxLife, sizeof(maxLife));
    payload[51] = 1;
    payload[52] = 3;
    payload[53] = 4;
    for (std::size_t slot = 0; slot < 10; ++slot) {
        const auto item = static_cast<std::uint16_t>(1000 + slot);
        std::memcpy(payload.data() + 54 + slot * 2, &item, sizeof(item));
    }
    std::memcpy(payload.data() + 75, &level, sizeof(level));
    std::memcpy(payload.data() + 77, &mapNum, sizeof(mapNum));
    payload[95] = 1;
    std::memcpy(payload.data() + 105, &height, sizeof(height));
    std::memcpy(payload.data() + 109, &width, sizeof(width));
    std::memcpy(payload.data() + 147, &positionX, sizeof(positionX));
    std::memcpy(payload.data() + 149, &positionZ, sizeof(positionZ));

    const auto info = parse_legacy_character_add(payload);

    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->object_id, objectId);
    EXPECT_EQ(info->user_id, userId);
    EXPECT_EQ(info->name, "RemoteHero");
    EXPECT_EQ(info->life, life);
    EXPECT_EQ(info->max_life, maxLife);
    EXPECT_EQ(info->gender, 1u);
    EXPECT_EQ(info->face_type, 3u);
    EXPECT_EQ(info->hair_type, 4u);
    EXPECT_EQ(info->weared_item_idx.front(), 1000u);
    EXPECT_EQ(info->weared_item_idx.back(), 1009u);
    EXPECT_EQ(info->level, level);
    EXPECT_EQ(info->map_num, mapNum);
    EXPECT_EQ(info->position_x, positionX);
    EXPECT_EQ(info->position_z, positionZ);
    EXPECT_FLOAT_EQ(info->height, height);
    EXPECT_FLOAT_EQ(info->width, width);
    EXPECT_TRUE(info->visible);
    EXPECT_TRUE(info->appearance_known);
}

TEST(InGameCharacterAdd, RejectsPayloadWithoutCompleteMoveInfo) {
    std::array<std::uint8_t, 160> payload{};
    EXPECT_FALSE(parse_legacy_character_add(payload).has_value());
}

TEST(InGameCharacterAdd, CreatesMovesAndRemovesRemotePlayerByStableId) {
    mxh::client::CInGameState state;
    mxh::net::Message add;
    add.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    add.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::CharacterAdd);
    add.payload.resize(288);
    const std::uint32_t objectId = 7001u;
    const std::uint16_t initialX = 120u;
    const std::uint16_t initialZ = 220u;
    std::memcpy(add.payload.data(), &objectId, sizeof(objectId));
    std::memcpy(add.payload.data() + 8, "VisibleNow", 11);
    add.payload[51] = 1;
    add.payload[95] = 1;
    std::memcpy(add.payload.data() + 147, &initialX, sizeof(initialX));
    std::memcpy(add.payload.data() + 149, &initialZ, sizeof(initialZ));

    state.on_message({}, add);

    ASSERT_EQ(state.remote_players().size(), 1u);
    ASSERT_TRUE(state.remote_players().contains(objectId));
    EXPECT_EQ(state.remote_players().at(objectId).name, "VisibleNow");
    EXPECT_EQ(state.remote_players().at(objectId).position_x, initialX);
    EXPECT_TRUE(state.remote_players().at(objectId).appearance_known);

    mxh::net::Message move;
    move.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Move);
    move.header.object_id = objectId;
    move.payload.resize(4);
    const std::uint16_t movedX = 320u;
    const std::uint16_t movedZ = 420u;
    std::memcpy(move.payload.data(), &movedX, sizeof(movedX));
    std::memcpy(move.payload.data() + 2, &movedZ, sizeof(movedZ));
    state.on_message({}, move);

    EXPECT_EQ(state.remote_players().at(objectId).position_x, movedX);
    EXPECT_EQ(state.remote_players().at(objectId).position_z, movedZ);
    EXPECT_EQ(state.remote_players().at(objectId).name, "VisibleNow");
    EXPECT_TRUE(state.remote_players().at(objectId).appearance_known);
    EXPECT_TRUE(state.remote_players().at(objectId).moving);
    EXPECT_NEAR(state.remote_players().at(objectId).facing_yaw,
                0.7853982f, 0.0001f);

    move.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::MoveProtocol::Stop);
    state.on_message({}, move);
    EXPECT_FALSE(state.remote_players().at(objectId).moving);
    EXPECT_NEAR(state.remote_players().at(objectId).facing_yaw,
                0.7853982f, 0.0001f);

    mxh::net::Message remove;
    remove.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    remove.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::ObjectRemove);
    remove.payload.resize(4);
    std::memcpy(remove.payload.data(), &objectId, sizeof(objectId));
    state.on_message({}, remove);

    EXPECT_TRUE(state.remote_players().empty());
}

TEST(InGameCharacterAdd, SelfRefreshUpdatesLiveVitalsAndEquipment) {
    mxh::client::CInGameState state;
    mxh::net::Message game_in;
    game_in.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    game_in.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::GameInAck);
    game_in.payload.resize(mxh::game::HERO_TOTAL_EMPTY_PAYLOAD_SIZE, 0);
    const std::uint32_t player_id = 123u;
    std::memcpy(game_in.payload.data(), &player_id, sizeof(player_id));
    state.on_message({}, game_in);
    ASSERT_EQ(state.game_info().player_id, player_id);

    mxh::net::Message refresh;
    refresh.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    refresh.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::CharacterAdd);
    refresh.header.object_id = player_id;
    refresh.payload.resize(288, 0);
    std::memcpy(refresh.payload.data(), &player_id, sizeof(player_id));
    const std::uint32_t life = 77u;
    const std::uint32_t max_life = 155u;
    std::memcpy(refresh.payload.data() + 35, &life, sizeof(life));
    std::memcpy(refresh.payload.data() + 39, &max_life, sizeof(max_life));
    const std::uint16_t equipped = 777u;
    std::memcpy(refresh.payload.data() + 54, &equipped, sizeof(equipped));
    refresh.payload[51] = 1;
    refresh.payload[52] = 2;
    refresh.payload[53] = 3;
    state.on_message({}, refresh);

    EXPECT_EQ(state.game_info().life, 77u);
    EXPECT_EQ(state.game_info().max_life, 155u);
    EXPECT_EQ(state.game_info().gender, 1u);
    EXPECT_EQ(state.game_info().face_type, 2u);
    EXPECT_EQ(state.game_info().hair_type, 3u);
    EXPECT_EQ(state.game_info().weared_item_idx.front(), equipped);
    EXPECT_TRUE(state.remote_players().empty());
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

TEST(InGameEntityLifecycle, RepeatedAddReplacesAndRemoveUsesStableObjectId) {
    mxh::client::CInGameState state;

    mxh::net::Message monsterAdd;
    monsterAdd.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    monsterAdd.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::MonsterAdd);
    monsterAdd.payload.resize(64, 0);
    const std::uint32_t monsterId = 700u;
    std::memcpy(monsterAdd.payload.data(), &monsterId, sizeof(monsterId));
    monsterAdd.payload[43] = 1;
    state.on_message({}, monsterAdd);
    monsterAdd.payload[43] = 2;
    state.on_message({}, monsterAdd);
    ASSERT_EQ(state.monsters().size(), 1u);
    EXPECT_EQ(state.monsters().front().monster_kind, 2u);

    mxh::net::Message npcAdd;
    npcAdd.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    npcAdd.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::NpcAdd);
    npcAdd.payload.resize(64, 0);
    const std::uint32_t npcId = 800u;
    std::memcpy(npcAdd.payload.data(), &npcId, sizeof(npcId));
    npcAdd.payload[35] = 3;
    state.on_message({}, npcAdd);
    npcAdd.payload[35] = 4;
    state.on_message({}, npcAdd);
    ASSERT_EQ(state.npcs().size(), 1u);
    EXPECT_EQ(state.npcs().front().npc_kind, 4u);

    const auto remove = [&](std::uint32_t objectId) {
        mxh::net::Message message;
        message.header.category = static_cast<std::uint8_t>(
            mxh::proto::Category::UserConn);
        message.header.protocol = static_cast<std::uint8_t>(
            mxh::proto::UserConnProtocol::ObjectRemove);
        message.payload.resize(4);
        std::memcpy(message.payload.data(), &objectId, sizeof(objectId));
        state.on_message({}, message);
    };
    remove(monsterId);
    remove(npcId);
    EXPECT_TRUE(state.monsters().empty());
    EXPECT_TRUE(state.npcs().empty());
}

TEST(InGameEntityLifecycle, ReleaseClearsWorldOwnedStateBeforeMapChange) {
    mxh::client::CInGameState state;

    mxh::net::Message monster;
    monster.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    monster.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::MonsterAdd);
    monster.payload.resize(64, 0);
    const std::uint32_t monster_id = 701u;
    std::memcpy(monster.payload.data(), &monster_id, sizeof(monster_id));
    state.on_message({}, monster);

    mxh::net::Message npc;
    npc.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    npc.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::NpcAdd);
    npc.payload.resize(64, 0);
    const std::uint32_t npc_id = 801u;
    std::memcpy(npc.payload.data(), &npc_id, sizeof(npc_id));
    state.on_message({}, npc);

    ASSERT_FALSE(state.monsters().empty());
    ASSERT_FALSE(state.npcs().empty());
    state.Release();
    EXPECT_TRUE(state.monsters().empty());
    EXPECT_TRUE(state.npcs().empty());
    EXPECT_TRUE(state.ground_drops().empty());
}
