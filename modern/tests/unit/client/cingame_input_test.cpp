#include "CInGameState.hpp"
#include "mxh/game/hero_total_layout.hpp"
#include "mxh/net/net.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <vector>

using mxh::client::key_mask_for_vk;
using mxh::client::make_attack_message;
using mxh::client::make_buy_message;
using mxh::client::make_chat_message;
using mxh::client::make_move_message;
using mxh::client::make_pickup_message;
using mxh::client::parse_legacy_ground_drop;
using mxh::client::MoveKey;
using mxh::client::parse_monster_life_payload;
using mxh::client::parse_shop_list;
using mxh::client::parse_chat_payload;
using mxh::client::parse_move_payload;
using mxh::client::pick_attack_target;
using mxh::client::project_npc_to_screen;
using mxh::client::unproject_screen_to_world;
using mxh::client::quick_skill_for_slot;
using mxh::client::step_movement;
using mxh::client::kMoveSpeed;
using mxh::client::kWorldLimit;

namespace {
constexpr std::uint32_t kForward = static_cast<std::uint32_t>(MoveKey::Forward);
constexpr std::uint32_t kBack = static_cast<std::uint32_t>(MoveKey::Back);
constexpr std::uint32_t kStrafeLeft = static_cast<std::uint32_t>(MoveKey::StrafeLeft);
constexpr std::uint32_t kStrafeRight = static_cast<std::uint32_t>(MoveKey::StrafeRight);
constexpr std::uint32_t kRotateLeft = static_cast<std::uint32_t>(MoveKey::RotateLeft);
constexpr std::uint32_t kRotateRight = static_cast<std::uint32_t>(MoveKey::RotateRight);

mxh::net::Message make_gamein_ack_at(std::uint16_t x, std::uint16_t z);
}  // namespace

TEST(InGameInput, VkMappingMatchesLegacyBindings) {
    EXPECT_EQ(key_mask_for_vk(mxh::client::kVkW), kForward);
    EXPECT_EQ(key_mask_for_vk(mxh::client::kVkUp), kForward);
    EXPECT_EQ(key_mask_for_vk(mxh::client::kVkS), kBack);
    EXPECT_EQ(key_mask_for_vk(mxh::client::kVkDown), kBack);
    EXPECT_EQ(key_mask_for_vk(mxh::client::kVkQ), kStrafeLeft);
    EXPECT_EQ(key_mask_for_vk(mxh::client::kVkE), kStrafeRight);
    EXPECT_EQ(key_mask_for_vk(mxh::client::kVkA), kRotateLeft);
    EXPECT_EQ(key_mask_for_vk(mxh::client::kVkLeft), kRotateLeft);
    EXPECT_EQ(key_mask_for_vk(mxh::client::kVkD), kRotateRight);
    EXPECT_EQ(key_mask_for_vk(mxh::client::kVkRight), kRotateRight);
    EXPECT_EQ(key_mask_for_vk(0x20), 0u);  // space is not a move key
}

TEST(InGameMovement, NoKeysMeansNoMovement) {
    const auto r = step_movement(0, 1.0f, 100.0f, 200.0f, 0.1f);
    EXPECT_EQ(r.x, 100.0f);
    EXPECT_EQ(r.z, 200.0f);
    EXPECT_EQ(r.yaw, 1.0f);
    EXPECT_FALSE(r.moving);
}

TEST(InGameMovement, ForwardAtDefaultYawMovesAlongPlusZ) {
    const auto r = step_movement(kForward, 0.0f, 0.0f, 0.0f, 1.0f);
    EXPECT_NEAR(r.x, 0.0f, 0.001f);
    EXPECT_NEAR(r.z, kMoveSpeed, 0.001f);
    EXPECT_TRUE(r.moving);
}

TEST(InGameMovement, ForwardPlusStrafeRightIsNormalizedDiagonal) {
    const auto r = step_movement(kForward | kStrafeRight, 0.0f,
                                 0.0f, 0.0f, 1.0f);
    const float expected = kMoveSpeed * 0.70710678f;
    EXPECT_NEAR(r.x, expected, 0.01f);
    EXPECT_NEAR(r.z, expected, 0.01f);
}

TEST(InGameMovement, RotateKeysTurnYawWithoutMoving) {
    const auto r = step_movement(kRotateLeft, 1.0f, 5.0f, 6.0f, 1.0f);
    EXPECT_LT(r.yaw, 1.0f);
    EXPECT_EQ(r.x, 5.0f);
    EXPECT_EQ(r.z, 6.0f);
    EXPECT_FALSE(r.moving);
}

TEST(InGameMovement, PositionClampsToWorldLimit) {
    const auto r = step_movement(kForward, 0.0f,
                                 0.0f, kWorldLimit - 10.0f, 10.0f);
    EXPECT_EQ(r.x, 0.0f);
    EXPECT_EQ(r.z, kWorldLimit);
}

TEST(InGameMovement, UsesMapSpecificWorldBounds) {
    const auto r = step_movement(kForward, 0.0f,
                                 10.0f, 90.0f, 1.0f, 120.0f, 100.0f);
    EXPECT_EQ(r.x, 10.0f);
    EXPECT_EQ(r.z, 100.0f);
}

TEST(InGamePlayable, StaticCollisionQueryRejectsCandidateStep) {
    mxh::client::CInGameState state;
    state.Init(nullptr);
    mxh::client::GameInInfo info;
    info.player_id = 42;
    info.position_x = 1000;
    info.position_z = 1000;
    info.map_num = 10;
    state.dispatch_gamein_ack(info);
    state.set_collision_query([](float x, float z, float) {
        return x >= 900.0f && z >= 900.0f;
    });
    state.OnKeyEvent(true, mxh::client::kVkW);
    state.Process();
    EXPECT_EQ(state.local_x(), 1000u);
    EXPECT_EQ(state.local_z(), 1000u);
}

TEST(InGamePlayable, MKeyTogglesLoadedBigMapDialogState) {
    mxh::client::CInGameState state;
    state.Init(nullptr);
    mxh::client::GameInInfo info;
    info.player_id = 42;
    info.map_num = 10;
    state.dispatch_gamein_ack(info);
    EXPECT_FALSE(state.map_open());
    state.OnKeyEvent(true, 0x4D); // M
    EXPECT_TRUE(state.map_open());
    state.OnKeyEvent(true, 0x4D);
    EXPECT_FALSE(state.map_open());
}

TEST(InGamePlayable, EffectAudioDistanceUsesAuthoritativeObjectIdentity) {
    mxh::client::CInGameState state;
    state.Init(nullptr);
    mxh::client::GameInInfo info;
    info.player_id = 42;
    info.position_x = 1000;
    info.position_z = 1000;
    info.map_num = 10;
    state.dispatch_gamein_ack(info);

    const auto local = state.distance_to_object(42);
    ASSERT_TRUE(local.has_value());
    EXPECT_FLOAT_EQ(*local, 0.0f);
    EXPECT_FALSE(state.distance_to_object(999999u).has_value());
}

TEST(InGameMovement, ZeroDtDoesNotMove) {
    const auto r = step_movement(kForward, 0.0f, 1.0f, 2.0f, 0.0f);
    EXPECT_EQ(r.x, 1.0f);
    EXPECT_EQ(r.z, 2.0f);
    EXPECT_FALSE(r.moving);
}

TEST(InGameAttack, PicksNearestAliveMonsterInRange) {
    std::vector<mxh::client::MonsterAddInfo> monsters;
    monsters.push_back({});
    monsters.back().object_id = 50000;
    monsters.back().current_life = 80;
    monsters.back().position_x = 100;
    monsters.back().position_z = 100;

    monsters.push_back({});
    monsters.back().object_id = 50001;
    monsters.back().current_life = 150;
    monsters.back().position_x = 400;
    monsters.back().position_z = 0;

    monsters.push_back({});
    monsters.back().object_id = 50002;
    monsters.back().current_life = 0;  // dead: never selected
    monsters.back().position_x = 30;
    monsters.back().position_z = 0;

    const auto target = pick_attack_target(monsters, 0.0f, 0.0f, 500.0f);
    ASSERT_TRUE(target.has_value());
    EXPECT_EQ(*target, 50000u);
}

TEST(InGameAttack, ReturnsNulloptWhenOutOfRange) {
    std::vector<mxh::client::MonsterAddInfo> monsters;
    monsters.push_back({});
    monsters.back().object_id = 50000;
    monsters.back().current_life = 80;
    monsters.back().position_x = 1000;
    monsters.back().position_z = 1000;

    EXPECT_FALSE(pick_attack_target(monsters, 0.0f, 0.0f, 500.0f).has_value());
}

TEST(InGameAttack, ReturnsNulloptForEmptyList) {
    EXPECT_FALSE(pick_attack_target({}, 0.0f, 0.0f, 500.0f).has_value());
}

TEST(InGameQuickSlot, StarterSetWhenMugongEmpty) {
    mxh::client::GameInInfo info;
    EXPECT_EQ(quick_skill_for_slot(info, 0), 0u);
    EXPECT_EQ(quick_skill_for_slot(info, 1), 0u);
    EXPECT_EQ(quick_skill_for_slot(info, 2), 0u);
    EXPECT_EQ(quick_skill_for_slot(info, 3), 0u);
    EXPECT_EQ(quick_skill_for_slot(info, 4), 0u);
    EXPECT_EQ(quick_skill_for_slot(info, 8), 0u);  // out of range
}

TEST(InGameQuickSlot, ParsedMugongWinsOverStarter) {
    mxh::client::GameInInfo info;
    info.mugong[0].icon_idx = 42;
    EXPECT_EQ(quick_skill_for_slot(info, 0), 42u);
}

TEST(InGameNpcProjection, ForwardNpcIsOnScreenCenter) {
    float sx = 0;
    float sy = 0;
    // NPC 500 world units in front of the player at yaw 0 -> screen center,
    // 400px up.
    EXPECT_TRUE(project_npc_to_screen(0.0f, 0.0f, 0.0f,
                                      0.0f, 500.0f, sx, sy));
    EXPECT_NEAR(sx, 400.0f, 0.01f);
    EXPECT_NEAR(sy, 300.0f - 500.0f * 0.8f, 0.01f);
}

TEST(InGameNpcProjection, BehindNpcIsRejected) {
    float sx = 0;
    float sy = 0;
    EXPECT_FALSE(project_npc_to_screen(0.0f, 0.0f, 0.0f,
                                       0.0f, -100.0f, sx, sy));
}

TEST(InGameNpcProjection, ScreenUnprojectionRoundTripsWorldPoint) {
    constexpr float yaw = 0.65f;
    float sx = 0;
    float sy = 0;
    ASSERT_TRUE(project_npc_to_screen(1200.0f, 800.0f, yaw,
                                      1450.0f, 1225.0f, sx, sy));
    float wx = 0;
    float wz = 0;
    ASSERT_TRUE(unproject_screen_to_world(1200.0f, 800.0f, yaw,
                                          sx, sy, wx, wz));
    EXPECT_NEAR(wx, 1450.0f, 0.01f);
    EXPECT_NEAR(wz, 1225.0f, 0.01f);
}

TEST(InGameNpcProjection, ScreenUnprojectionRejectsBehindCamera) {
    float wx = 0;
    float wz = 0;
    EXPECT_FALSE(unproject_screen_to_world(0.0f, 0.0f, 0.0f,
                                           400.0f, 400.0f, wx, wz));
}

TEST(InGamePlayable, PillarboxClickDoesNotMoveWorld) {
    mxh::client::CInGameState state;
    state.Init(nullptr);
    state.on_message(mxh::net::make_connection_id(1),
                     make_gamein_ack_at(25000, 25000));
    const auto before_x = state.local_x();
    const auto before_z = state.local_z();
    state.OnMouseButton(true, true, -20, 300);
    EXPECT_EQ(state.local_x(), before_x);
    EXPECT_EQ(state.local_z(), before_z);
    EXPECT_EQ(state.last_attack_target(), 0u);
}

TEST(InGameWire, MoveMessageMatchesModernServerLayout) {
    const auto m = make_move_message(
        240366u, mxh::proto::MoveProtocol::OneTarget, 0x1234u, 0x5678u);
    EXPECT_EQ(m.header.category,
              static_cast<std::uint8_t>(mxh::proto::Category::Move));
    EXPECT_EQ(m.header.protocol,
              static_cast<std::uint8_t>(mxh::proto::MoveProtocol::OneTarget));
    EXPECT_EQ(m.header.object_id, 240366u);
    ASSERT_EQ(m.payload.size(), 4u);
    EXPECT_EQ(m.payload[0], 0x34);
    EXPECT_EQ(m.payload[1], 0x12);
    EXPECT_EQ(m.payload[2], 0x78);
    EXPECT_EQ(m.payload[3], 0x56);
}

TEST(InGameWire, AttackMessageMatchesModernServerLayout) {
    const auto m = make_attack_message(
        240366u, 1u, 50000u, 27492.0f, 27358.0f);
    EXPECT_EQ(m.header.category,
              static_cast<std::uint8_t>(mxh::proto::Category::Skill));
    EXPECT_EQ(m.header.protocol,
              static_cast<std::uint8_t>(mxh::proto::SkillProtocol::StartSyn));
    EXPECT_EQ(m.header.object_id, 240366u);
    ASSERT_EQ(m.payload.size(), 16u);
    EXPECT_EQ(m.payload[0], 1u);
    EXPECT_EQ(m.payload[4], 0x50);
    EXPECT_EQ(m.payload[5], 0xC3);
    EXPECT_EQ(m.payload[6], 0x00);
    EXPECT_EQ(m.payload[7], 0x00);

    float tx = 0;
    float tz = 0;
    std::memcpy(&tx, m.payload.data() + 8, sizeof(tx));
    std::memcpy(&tz, m.payload.data() + 12, sizeof(tz));
    EXPECT_FLOAT_EQ(tx, 27492.0f);
    EXPECT_FLOAT_EQ(tz, 27358.0f);
}

TEST(InGameWire, ParseMovePayloadDecodesLittleEndian) {
    const std::array<std::uint8_t, 4> bytes{0x34, 0x12, 0x78, 0x56};
    const auto pos = parse_move_payload(bytes);
    ASSERT_TRUE(pos.has_value());
    EXPECT_EQ(pos->first, 0x1234u);
    EXPECT_EQ(pos->second, 0x5678u);
}

TEST(InGameWire, ParseMovePayloadRejectsShort) {
    const std::array<std::uint8_t, 3> bytes{0, 0, 0};
    EXPECT_FALSE(parse_move_payload(bytes).has_value());
}

TEST(InGameWire, ParseMonsterLifePayloadDecodes) {
    const std::array<std::uint8_t, 8> bytes{
        0x64, 0, 0, 0, 0x32, 0, 0, 0};
    const auto life = parse_monster_life_payload(bytes);
    ASSERT_TRUE(life.has_value());
    EXPECT_EQ(life->first, 100u);
    EXPECT_EQ(life->second, 50u);
}

TEST(InGameWire, ParseMonsterLifePayloadRejectsShort) {
    const std::array<std::uint8_t, 7> bytes{};
    EXPECT_FALSE(parse_monster_life_payload(bytes).has_value());
}

TEST(InGameWire, ChatMessageMatchesModernServerLayout) {
    const auto m = make_chat_message(240366u, "hello world");
    EXPECT_EQ(m.header.category,
              static_cast<std::uint8_t>(mxh::proto::Category::Chat));
    EXPECT_EQ(m.header.protocol,
              static_cast<std::uint8_t>(mxh::proto::ChatProtocol::All));
    EXPECT_EQ(m.header.object_id, 240366u);
    EXPECT_EQ(m.payload.size(), 11u);
    EXPECT_EQ(parse_chat_payload(m.payload), "hello world");
}

TEST(InGameWire, ParseChatPayloadStopsAtNul) {
    const std::array<std::uint8_t, 6> bytes{
        'a', 'b', 'c', 0, 'x', 'y'};
    EXPECT_EQ(parse_chat_payload(bytes), "abc");
}

TEST(InGameWire, ParseChatPayloadEmpty) {
    EXPECT_EQ(parse_chat_payload(std::span<const std::uint8_t>{}), "");
}

TEST(InGameShop, ParseShopListDecodesRows) {
    std::vector<std::uint8_t> payload(6 + 2 * 6, 0);
    payload[0] = 7;                       // npc_id
    payload[4] = 2;                       // count
    payload[6] = 0x10; payload[7] = 0;    // item 0x0010
    payload[8] = 0x64; payload[9] = 0;
    payload[10] = 0; payload[11] = 0;     // price 100
    payload[12] = 0x34; payload[13] = 0x12;  // item 0x1234
    payload[14] = 0xE8; payload[15] = 0x03;
    payload[16] = 0; payload[17] = 0;     // price 1000

    const auto items = parse_shop_list(payload);
    ASSERT_EQ(items.size(), 2u);
    EXPECT_EQ(items[0].item_id, 0x0010u);
    EXPECT_EQ(items[0].price, 100u);
    EXPECT_EQ(items[1].item_id, 0x1234u);
    EXPECT_EQ(items[1].price, 1000u);
}

TEST(InGameShop, ParseShopListShortPayloadIsEmpty) {
    std::array<std::uint8_t, 4> payload{};
    EXPECT_TRUE(parse_shop_list(payload).empty());
}

TEST(InGameShop, ParseShopListRejectsTruncatedCatalog) {
    std::array<std::uint8_t, 12> payload{};
    payload[4] = 2; // two entries declared, only one entry fits
    payload[6] = 0x10;
    payload[8] = 100;
    EXPECT_TRUE(parse_shop_list(payload).empty());
}

TEST(InGameShop, BuyMessageMatchesModernServerLayout) {
    const auto m = make_buy_message(240366u, 0x1234u, 1u);
    EXPECT_EQ(m.header.category,
              static_cast<std::uint8_t>(mxh::proto::Category::Item));
    EXPECT_EQ(m.header.protocol,
              static_cast<std::uint8_t>(mxh::proto::ItemProtocol::BuySyn));
    EXPECT_EQ(m.header.object_id, 240366u);
    ASSERT_EQ(m.payload.size(), 4u);
    EXPECT_EQ(m.payload[0], 0x34);
    EXPECT_EQ(m.payload[1], 0x12);
    EXPECT_EQ(m.payload[2], 1u);
    EXPECT_EQ(m.payload[3], 0u);
}

TEST(InGameShop, StaleBuyAckDoesNotCloseFreshShop) {
    mxh::client::CInGameState state;
    state.Init(nullptr);
    mxh::net::Message shop;
    shop.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    shop.header.protocol = mxh::proto::kModernShopList;
    shop.payload = {7, 0, 0, 0, 1, 0, 0x2B, 0x02, 10, 0, 0, 0};
    state.on_message(mxh::net::make_connection_id(1), shop);
    ASSERT_TRUE(state.shop_open());
    mxh::net::Message ack;
    ack.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    ack.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::BuyAck);
    state.on_message(mxh::net::make_connection_id(1), ack);
    EXPECT_TRUE(state.shop_open());
}

namespace {

mxh::net::Message make_gamein_ack_at(std::uint16_t x, std::uint16_t z) {
    mxh::net::Message ack;
    ack.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    ack.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::GameInAck);
    ack.payload.assign(mxh::game::HERO_TOTAL_EMPTY_PAYLOAD_SIZE, 0);
    const std::uint32_t player_id = 42;
    std::memcpy(ack.payload.data(), &player_id, 4);
    std::memcpy(ack.payload.data() + mxh::game::HERO_TOTAL_MOVE_OFFSET, &x, 2);
    std::memcpy(ack.payload.data() + mxh::game::HERO_TOTAL_MOVE_OFFSET + 2, &z, 2);
    return ack;
}

mxh::net::Message make_monster_add_at(std::uint32_t object_id,
                                      std::uint16_t x, std::uint16_t z) {
    mxh::net::Message add;
    add.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    add.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::MonsterAdd);
    add.header.object_id = object_id;
    add.payload.assign(64, 0);
    std::memcpy(add.payload.data(), &object_id, 4);
    const std::uint32_t life = 100;
    std::memcpy(add.payload.data() + 35, &life, 4);
    std::memcpy(add.payload.data() + 49, &x, 2);
    std::memcpy(add.payload.data() + 51, &z, 2);
    return add;
}

}  // namespace

TEST(InGamePlayable, OffensiveQuickSlotRequiresExplicitCursorTarget) {
    mxh::client::CInGameState state;
    state.Init(nullptr);
    state.on_message(mxh::net::make_connection_id(1),
                     make_gamein_ack_at(25000, 25000));
    state.on_message(mxh::net::make_connection_id(1),
                     make_monster_add_at(50001u, 25020, 25000));
    state.OnKeyEvent(true, 0x70); // F1, offensive starter skill
    EXPECT_EQ(state.last_attack_target(), 0u);
}

TEST(InGamePlayable, HudDoesNotSwallowWasdAfterGameInAck) {
    mxh::client::CInGameState state;
    state.Init(nullptr);
    state.on_message(mxh::net::make_connection_id(1),
                     make_gamein_ack_at(25000, 25000));
    ASSERT_TRUE(state.is_in_game());
    EXPECT_EQ(state.local_x(), 25000u);
    EXPECT_EQ(state.local_z(), 25000u);

    state.OnKeyEvent(true, mxh::client::kVkW);
    state.Process();
    EXPECT_GT(state.local_z(), 25000u);
    EXPECT_EQ(state.local_x(), 25000u);
}

TEST(InGamePlayable, QStrafesInsteadOfOpeningQuestLog) {
    mxh::client::CInGameState state;
    state.Init(nullptr);
    state.on_message(mxh::net::make_connection_id(1),
                     make_gamein_ack_at(25000, 25000));
    state.OnKeyEvent(true, mxh::client::kVkQ);
    EXPECT_FALSE(state.quest_open());
    state.Process();
    EXPECT_LT(state.local_x(), 25000u);
}

TEST(InGamePlayable, SocialHotkeysToggleFriendAndGuildWindows) {
    mxh::client::CInGameState state;
    state.Init(nullptr);
    state.on_message(mxh::net::make_connection_id(1),
                     make_gamein_ack_at(25000, 25000));
    state.OnKeyEvent(true, 0x48); // H
    EXPECT_TRUE(state.friend_open());
    state.OnKeyEvent(true, mxh::client::kVkEscape);
    EXPECT_FALSE(state.friend_open());
    state.OnKeyEvent(true, 0x47); // G
    EXPECT_TRUE(state.guild_open());
    state.OnKeyEvent(true, mxh::client::kVkEscape);
    EXPECT_FALSE(state.guild_open());
}

TEST(InGamePlayable, ServerMovementCorrectionRollsBackLocalPrediction) {
    mxh::client::CInGameState state;
    state.Init(nullptr);
    state.on_message(mxh::net::make_connection_id(1),
                     make_gamein_ack_at(25000, 25000));
    state.OnKeyEvent(true, mxh::client::kVkW);
    state.Process();
    ASSERT_NE(state.local_z(), 25000u);

    mxh::net::Message correction;
    correction.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::Move);
    correction.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::MoveProtocol::Correction);
    correction.header.object_id = state.player_id();
    correction.payload.resize(4);
    const std::uint16_t x = 24900u;
    const std::uint16_t z = 24800u;
    std::memcpy(correction.payload.data(), &x, sizeof(x));
    std::memcpy(correction.payload.data() + 2, &z, sizeof(z));
    state.on_message(mxh::net::make_connection_id(1), correction);
    EXPECT_EQ(state.local_x(), x);
    EXPECT_EQ(state.local_z(), z);
}

TEST(InGamePlayable, SkillNackBecomesVisiblePlayerFeedback) {
    mxh::client::CInGameState state;
    state.Init(nullptr);
    state.on_message(mxh::net::make_connection_id(1),
                     make_gamein_ack_at(25000, 25000));
    mxh::net::Message nack;
    nack.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Skill);
    nack.header.protocol = static_cast<std::uint8_t>(mxh::proto::SkillProtocol::StartNack);
    nack.header.object_id = state.player_id();
    nack.payload = {4u};
    state.on_message(mxh::net::make_connection_id(1), nack);
    EXPECT_EQ(state.last_skill_error(), "Target is out of range.");
}

TEST(InGamePlayable, SkillResultPublishesCombatFeedbackForHud) {
    mxh::client::CInGameState state;
    state.Init(nullptr);
    state.on_message(mxh::net::make_connection_id(1),
                     make_gamein_ack_at(25000, 25000));
    mxh::net::Message result;
    result.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Skill);
    result.header.protocol = static_cast<std::uint8_t>(mxh::proto::SkillProtocol::SingleResult);
    result.header.object_id = state.player_id();
    result.payload.resize(9);
    const std::uint32_t target = 50001u;
    const std::int32_t damage = 37;
    const std::uint8_t hit = 1u;
    std::memcpy(result.payload.data(), &target, sizeof(target));
    std::memcpy(result.payload.data() + 4, &damage, sizeof(damage));
    result.payload[8] = hit;
    state.on_message(mxh::net::make_connection_id(1), result);
    EXPECT_EQ(state.last_damage(), damage);
    EXPECT_EQ(state.last_hit_target(), target);
    EXPECT_EQ(state.last_hit_result(), hit);
    EXPECT_GT(state.last_damage_timestamp_ms(), 0u);
}

TEST(InGamePlayable, SkillResultImmediatelyUpdatesTargetLifeBar) {
    mxh::client::CInGameState state;
    state.Init(nullptr);
    state.on_message(mxh::net::make_connection_id(1),
                     make_gamein_ack_at(25000, 25000));
    mxh::net::Message monster;
    monster.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    monster.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::MonsterAdd);
    monster.payload.resize(64, 0);
    const std::uint32_t target = 50002u;
    const std::uint32_t life = 100u;
    std::memcpy(monster.payload.data(), &target, 4);
    std::memcpy(monster.payload.data() + 35, &life, 4);
    state.on_message(mxh::net::make_connection_id(1), monster);
    ASSERT_EQ(state.monsters().size(), 1u);
    mxh::net::Message result;
    result.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Skill);
    result.header.protocol = static_cast<std::uint8_t>(mxh::proto::SkillProtocol::SingleResult);
    result.payload.resize(9);
    const std::int32_t damage = 35;
    std::memcpy(result.payload.data(), &target, 4);
    std::memcpy(result.payload.data() + 4, &damage, 4);
    result.payload[8] = 1;
    state.on_message(mxh::net::make_connection_id(1), result);
    EXPECT_EQ(state.monsters().front().current_life, 65u);
}

TEST(InGamePlayable, DuplicateMonsterDeathNotifyEmitsOneDeathEvent) {
    mxh::client::CInGameState state;
    state.Init(nullptr);
    mxh::net::Message monster;
    monster.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    monster.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::MonsterAdd);
    monster.payload.resize(64, 0);
    const std::uint32_t target = 50003u;
    const std::uint32_t life = 25u;
    std::memcpy(monster.payload.data(), &target, 4);
    std::memcpy(monster.payload.data() + 35, &life, 4);
    state.on_message({}, monster);
    mxh::net::Message death;
    death.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Monster);
    death.header.protocol = static_cast<std::uint8_t>(mxh::proto::MonsterProtocol::LifeNotify);
    death.header.object_id = target;
    death.payload.resize(8, 0);
    state.on_message({}, death);
    state.on_message({}, death);
    const auto events = state.drain_effect_events();
    EXPECT_EQ(std::count_if(events.begin(), events.end(), [](const auto& event) {
        return event.kind == mxh::client::EffectEventKind::Death;
    }), 1);
}

TEST(InGamePlayable, BlockedMovementStillRotatesCamera) {
    mxh::client::CInGameState state;
    state.Init(nullptr);
    state.on_message(mxh::net::make_connection_id(1),
                     make_gamein_ack_at(25000, 25000));
    state.set_collision_query([](float, float, float) { return true; });
    const auto before_yaw = state.camera_yaw();
    state.OnKeyEvent(true, mxh::client::kVkW);
    state.OnKeyEvent(true, mxh::client::kVkD);
    state.Process();
    EXPECT_EQ(state.local_x(), 25000u);
    EXPECT_EQ(state.local_z(), 25000u);
    EXPECT_GT(state.camera_yaw(), before_yaw);
    EXPECT_FALSE(state.is_moving());
}

TEST(InGamePlayable, DiagonalCollisionSlidesAlongClearAxis) {
    mxh::client::CInGameState state;
    state.Init(nullptr);
    state.on_message(mxh::net::make_connection_id(1),
                     make_gamein_ack_at(25000, 25000));
    state.set_collision_query([](float x, float z, float) {
        // A diagonal destination is blocked, but either axis-aligned
        // candidate is traversable.
        return x != 25000.0f && z != 25000.0f;
    });
    state.OnKeyEvent(true, mxh::client::kVkW);
    state.OnKeyEvent(true, mxh::client::kVkQ);
    state.Process();
    EXPECT_TRUE(state.local_x() != 25000u || state.local_z() != 25000u);
    EXPECT_TRUE(state.local_x() == 25000u || state.local_z() == 25000u);
}

TEST(InGamePlayable, ClickMoveSlidesAlongClearAxis) {
    mxh::client::CInGameState state;
    state.Init(nullptr);
    state.on_message(mxh::net::make_connection_id(1),
                     make_gamein_ack_at(25000, 25000));
    state.set_collision_query([](float x, float z, float) {
        return x != 25000.0f && z != 25000.0f;
    });
    state.OnMouseButton(true, true, 320, 220);
    EXPECT_TRUE(state.local_x() != 25000u || state.local_z() != 25000u);
    EXPECT_TRUE(state.local_x() == 25000u || state.local_z() == 25000u);
}

TEST(InGamePlayable, FullyBlockedClickDoesNotAutoAttack) {
    mxh::client::CInGameState state;
    state.Init(nullptr);
    state.on_message(mxh::net::make_connection_id(1),
                     make_gamein_ack_at(25000, 25000));
    state.on_message(mxh::net::make_connection_id(1),
                     make_monster_add_at(50001u, 25000, 25000));
    state.set_collision_query([](float, float, float) { return true; });
    state.OnMouseButton(true, true, 320, 220);
    EXPECT_EQ(state.last_attack_target(), 0u);
}

TEST(InGamePlayable, LeftClickMovesOnEmptyWorldInsteadOfAutoAttacking) {
    mxh::client::CInGameState state;
    state.Init(nullptr);
    state.on_message(mxh::net::make_connection_id(1),
                     make_gamein_ack_at(25000, 25000));
    state.on_message(mxh::net::make_connection_id(1),
                     make_monster_add_at(50001u, 25000, 25000));
    ASSERT_EQ(state.monsters().size(), 1u);

    int miss_x = 400;
    int miss_y = 300;
    for (int y = 8; y < 600 && miss_x == 400; y += 16) {
        for (int x = 8; x < 800; x += 16) {
            bool covered = false;
            for (const auto& dialog : state.ui_runtime().dialogs()) {
                if (!dialog || !dialog->isActive()) continue;
                if (dialog->PtInWindow(x, y)) {
                    covered = true;
                    break;
                }
            }
            if (!covered) {
                miss_x = x;
                miss_y = y;
                break;
            }
        }
    }
    const auto before_x = state.local_x();
    const auto before_z = state.local_z();
    state.OnMouseButton(true, true, miss_x, miss_y);
    EXPECT_EQ(state.last_attack_target(), 0u);
    EXPECT_TRUE(state.local_x() != before_x || state.local_z() != before_z);
}

TEST(InGamePlayable, LeftClickPrefersMonsterUnderCursor) {
    mxh::client::CInGameState state;
    state.Init(nullptr);
    state.on_message(mxh::net::make_connection_id(1),
                     make_gamein_ack_at(25000, 25000));
    // At yaw 0, a monster 100 world units in front projects to the logical
    // centre line.  A second, closer monster is deliberately off-screen so
    // nearest-target fallback would choose the wrong object.
    state.on_message(mxh::net::make_connection_id(1),
                     make_monster_add_at(50001u, 25000, 25100));
    state.on_message(mxh::net::make_connection_id(1),
                     make_monster_add_at(50002u, 25100, 25000));

    state.OnMouseButton(true, true, 400, 220);
    EXPECT_EQ(state.last_attack_target(), 50001u);
}

TEST(InGamePlayable, ExplicitPointedTargetStillHonorsAttackRange) {
    mxh::client::CInGameState state;
    state.Init(nullptr);
    state.on_message(mxh::net::make_connection_id(1),
                     make_gamein_ack_at(25000, 25000));
    state.on_message(mxh::net::make_connection_id(1),
                     make_monster_add_at(50003u, 26000, 25000));

    float screen_x = 0.0f;
    float screen_y = 0.0f;
    ASSERT_TRUE(mxh::client::project_npc_to_screen(
        25000.0f, 25000.0f, 0.0f, 26000.0f, 25000.0f,
        screen_x, screen_y));
    state.OnMouseButton(true, true,
                        static_cast<std::int32_t>(screen_x),
                        static_cast<std::int32_t>(screen_y));
    EXPECT_EQ(state.last_attack_target(), 0u);
}

TEST(InGamePlayable, MouseWheelZoomIsBounded) {
    mxh::client::CInGameState state;
    state.Init(nullptr);
    state.on_message(mxh::net::make_connection_id(1),
                     make_gamein_ack_at(25000, 25000));
    EXPECT_FLOAT_EQ(state.camera_distance(), 6.0f);
    for (int i = 0; i < 20; ++i) state.OnMouseWheel(120);
    EXPECT_FLOAT_EQ(state.camera_distance(), 3.0f);
    for (int i = 0; i < 40; ++i) state.OnMouseWheel(-120);
    EXPECT_FLOAT_EQ(state.camera_distance(), 12.0f);
    state.OnMouseWheel(0);
    EXPECT_FLOAT_EQ(state.camera_distance(), 12.0f);
}

TEST(InGamePlayable, RightDragRotatesCameraThroughHudCoverage) {
    mxh::client::CInGameState state;
    state.Init(nullptr);
    const auto initial = state.camera_yaw();
    state.OnMouseButton(false, true, 400, 300);
    state.OnMouseMove(450, 300);
    EXPECT_NE(state.camera_yaw(), initial);
    state.OnMouseButton(false, false, 450, 300);
    const auto settled = state.camera_yaw();
    state.OnMouseMove(500, 300);
    EXPECT_EQ(state.camera_yaw(), settled);
}

TEST(InGamePlayable, MonsterObtainNotifyBecomesGroundDropAndPickupAckClearsIt) {
    mxh::client::CInGameState state;
    state.Init(nullptr);
    state.on_message(mxh::net::make_connection_id(1),
                     make_gamein_ack_at(25000, 25000));

    mxh::net::Message notify;
    notify.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::Item);
    notify.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::ItemProtocol::MonsterObtainNotify);
    notify.payload.resize(20, 0);
    const std::uint32_t drop_id = 9001u;
    const std::uint32_t source = 50001u;
    const std::uint16_t item_id = 501u;
    const std::uint16_t count = 1u;
    const float px = 25000.0f;
    const float pz = 25000.0f;
    std::memcpy(notify.payload.data(), &drop_id, 4);
    std::memcpy(notify.payload.data() + 4, &source, 4);
    std::memcpy(notify.payload.data() + 8, &item_id, 2);
    std::memcpy(notify.payload.data() + 10, &count, 2);
    std::memcpy(notify.payload.data() + 12, &px, 4);
    std::memcpy(notify.payload.data() + 16, &pz, 4);
    state.on_message(mxh::net::make_connection_id(1), notify);
    ASSERT_EQ(state.ground_drops().size(), 1u);
    EXPECT_EQ(state.ground_drops()[0].object_id, drop_id);
    EXPECT_EQ(state.ground_drops()[0].item_id, item_id);

    const auto pickup = make_pickup_message(42u, drop_id);
    EXPECT_EQ(pickup.header.protocol,
              static_cast<std::uint8_t>(mxh::proto::ItemProtocol::PickupSyn));

    mxh::net::Message ack;
    ack.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::Item);
    ack.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::ItemProtocol::PickupAck);
    ack.payload.resize(8, 0);
    std::memcpy(ack.payload.data(), &drop_id, 4);
    std::memcpy(ack.payload.data() + 4, &item_id, 2);
    std::memcpy(ack.payload.data() + 6, &count, 2);
    state.on_message(mxh::net::make_connection_id(1), ack);
    EXPECT_TRUE(state.ground_drops().empty());
    EXPECT_EQ(state.game_info().items.Inventory[0].dwDBIdx, drop_id);
    EXPECT_EQ(state.game_info().items.Inventory[0].wIconIdx, item_id);
    EXPECT_EQ(state.game_info().items.Inventory[0].ItemParam, count);

    // A repeated acknowledgement for the already-consumed drop must not
    // create a second inventory entry.
    state.on_message(mxh::net::make_connection_id(1), ack);
    EXPECT_TRUE(mxh::game::is_empty_slot(state.game_info().items.Inventory[1]));
}

TEST(InGamePlayable, ItemNacksBecomeVisiblePlayerFeedback) {
    mxh::client::CInGameState state;
    state.Init(nullptr);
    mxh::net::Message nack;
    nack.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);

    nack.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::PickupNack);
    state.on_message(mxh::net::make_connection_id(1), nack);
    EXPECT_EQ(state.last_item_error(), "Cannot pick up this item.");

    state.ui_runtime().clear();
    nack.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::BuyNack);
    state.on_message(mxh::net::make_connection_id(1), nack);
    EXPECT_EQ(state.last_item_error(), "Purchase failed.");

    state.ui_runtime().clear();
    nack.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::SellNack);
    state.on_message(mxh::net::make_connection_id(1), nack);
    EXPECT_EQ(state.last_item_error(), "Sale failed.");
}

TEST(InGamePlayable, InvalidPickupAckDoesNotConsumeGroundDrop) {
    mxh::client::CInGameState state;
    state.Init(nullptr);
    state.on_message(mxh::net::make_connection_id(1),
                     make_gamein_ack_at(25000, 25000));
    mxh::net::Message notify;
    notify.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    notify.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::MonsterObtainNotify);
    notify.payload.resize(20, 0);
    const std::uint32_t drop_id = 9020u;
    const std::uint16_t item_id = 502u;
    std::memcpy(notify.payload.data(), &drop_id, 4);
    std::memcpy(notify.payload.data() + 8, &item_id, 2);
    state.on_message(mxh::net::make_connection_id(1), notify);
    ASSERT_EQ(state.ground_drops().size(), 1u);

    mxh::net::Message ack;
    ack.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    ack.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::PickupAck);
    ack.payload.resize(8, 0);
    std::memcpy(ack.payload.data(), &drop_id, 4);
    std::memcpy(ack.payload.data() + 4, &item_id, 2);
    state.on_message(mxh::net::make_connection_id(1), ack);
    EXPECT_EQ(state.ground_drops().size(), 1u);
    EXPECT_EQ(state.last_item_error(), "Invalid pickup confirmation.");
}

TEST(InGamePlayable, BKeyOpensNearestNpcShopThenBuyClickSelectsCatalogItem) {
    mxh::client::CInGameState state;
    state.Init(nullptr);
    state.on_message(mxh::net::make_connection_id(1),
                     make_gamein_ack_at(25000, 25000));

    mxh::net::Message npc;
    npc.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    npc.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::NpcAdd);
    npc.payload.assign(64, 0);
    const std::uint32_t npc_id = 7u;
    const std::uint16_t pos = 25000;
    std::memcpy(npc.payload.data(), &npc_id, 4);
    npc.payload[35] = 1;
    std::memcpy(npc.payload.data() + 45, &pos, 2);
    std::memcpy(npc.payload.data() + 47, &pos, 2);
    state.on_message(mxh::net::make_connection_id(1), npc);
    ASSERT_EQ(state.npcs().size(), 1u);

    state.OnKeyEvent(true, 0x42);  // B — nearest NPC shop
    EXPECT_EQ(state.shop_npc_id(), 7u);

    mxh::net::Message shop;
    shop.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::Item);
    shop.header.protocol = mxh::proto::kModernShopList;
    shop.payload.resize(12, 0);
    shop.payload[0] = 7;
    shop.payload[4] = 1;
    shop.payload[6] = 0x2B;
    shop.payload[7] = 0x02;  // item 0x022B
    shop.payload[8] = 0x39;
    shop.payload[9] = 0x30;  // price 12345
    state.on_message(mxh::net::make_connection_id(1), shop);
    ASSERT_TRUE(state.shop_open());
    ASSERT_EQ(state.shop_items().size(), 1u);
    EXPECT_EQ(state.shop_npc_id(), 7u);
    EXPECT_EQ(state.shop_items()[0].item_id, 0x022Bu);

    state.OnMouseButton(true, true,
                        static_cast<std::int32_t>(mxh::client::kShopPanelX + 8),
                        static_cast<std::int32_t>(mxh::client::kShopPanelY + 4));
    EXPECT_EQ(state.last_buy_item_id(), 0x022Bu);
}

TEST(InGamePlayable, BKeyDoesNotOpenShopForQuestNpc) {
    mxh::client::CInGameState state;
    state.Init(nullptr);
    state.on_message(mxh::net::make_connection_id(1),
                     make_gamein_ack_at(25000, 25000));

    mxh::net::Message npc;
    npc.header.category = static_cast<std::uint8_t>(
        mxh::proto::Category::UserConn);
    npc.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::NpcAdd);
    npc.payload.assign(64, 0);
    const std::uint32_t npc_id = 8u;
    const std::uint16_t pos = 25000;
    std::memcpy(npc.payload.data(), &npc_id, sizeof(npc_id));
    npc.payload[35] = 6;  // legacy TALKER_ROLE
    std::memcpy(npc.payload.data() + 45, &pos, sizeof(pos));
    std::memcpy(npc.payload.data() + 47, &pos, sizeof(pos));
    state.on_message(mxh::net::make_connection_id(1), npc);

    state.OnKeyEvent(true, 0x42);  // B must not treat a quest NPC as a shop.
    EXPECT_FALSE(state.shop_open());
    EXPECT_EQ(state.shop_npc_id(), 0u);
}

TEST(InGamePlayable, DistantNpcDoesNotTriggerInteraction) {
    mxh::client::CInGameState state;
    state.Init(nullptr);
    state.on_message(mxh::net::make_connection_id(1),
                     make_gamein_ack_at(25000, 25000));
    mxh::net::Message npc;
    npc.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    npc.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::NpcAdd);
    npc.payload.assign(64, 0);
    const std::uint32_t npc_id = 99u;
    const std::uint16_t far = 30000u;
    std::memcpy(npc.payload.data(), &npc_id, sizeof(npc_id));
    npc.payload[35] = 1; // dealer, but outside the authoritative interaction radius
    std::memcpy(npc.payload.data() + 45, &far, sizeof(far));
    std::memcpy(npc.payload.data() + 47, &far, sizeof(far));
    state.on_message(mxh::net::make_connection_id(1), npc);
    state.OnKeyEvent(true, 0x42);
    EXPECT_FALSE(state.shop_open());
    EXPECT_EQ(state.shop_npc_id(), 0u);
}

TEST(InGamePlayable, NpcSpeechNackBecomesVisiblePlayerFeedback) {
    mxh::client::CInGameState state;
    state.Init(nullptr);
    mxh::net::Message nack;
    nack.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Npc);
    nack.header.protocol = static_cast<std::uint8_t>(mxh::proto::NpcProtocol::SpeechNack);
    state.on_message(mxh::net::make_connection_id(1), nack);
    EXPECT_EQ(state.last_npc_error(), "NPC is too far away.");
}

TEST(InGamePlayable, MoneyUpdateFromShopAckFeedsLiveHudState) {
    mxh::client::CInGameState state;
    state.Init(nullptr);
    state.on_message(mxh::net::make_connection_id(1),
                     make_gamein_ack_at(25000, 25000));
    const std::uint32_t money = 4321u;
    mxh::net::Message update;
    update.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    update.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::Money);
    update.payload.resize(sizeof(money));
    std::memcpy(update.payload.data(), &money, sizeof(money));
    state.on_message(mxh::net::make_connection_id(1), update);
    EXPECT_EQ(state.game_info().money, money);
}

TEST(InGamePlayable, UseAckConsumesLiveInventorySlotAndUpdatesVitals) {
    mxh::client::CInGameState state;
    state.Init(nullptr);
    state.on_message(mxh::net::make_connection_id(1),
                     make_gamein_ack_at(25000, 25000));
    mxh::game::ItemTotalInfo items{};
    items.Inventory[3] = mxh::game::make_item(7001u, 101u, 3u, 100u, 1u);
    mxh::net::Message inventory;
    inventory.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    inventory.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::TotalInfoLocal);
    inventory.payload.resize(sizeof(items));
    std::memcpy(inventory.payload.data(), &items, sizeof(items));
    state.on_message(mxh::net::make_connection_id(1), inventory);
    mxh::net::Message ack;
    ack.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    ack.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::UseAck);
    ack.payload.resize(20, 0);
    const std::uint16_t pos = 3u;
    const std::uint32_t hp = 777u;
    const std::uint32_t mp = 555u;
    std::memcpy(ack.payload.data(), &pos, 2);
    std::memcpy(ack.payload.data() + 12, &hp, 4);
    std::memcpy(ack.payload.data() + 16, &mp, 4);
    state.on_message(mxh::net::make_connection_id(1), ack);
    EXPECT_TRUE(mxh::game::is_empty_slot(state.game_info().items.Inventory[3]));
    EXPECT_EQ(state.game_info().life, hp);
    EXPECT_EQ(state.game_info().mp, mp);
}

TEST(InGamePlayable, DiscardAckClearsLiveInventorySlot) {
    mxh::client::CInGameState state;
    state.Init(nullptr);
    state.on_message(mxh::net::make_connection_id(1),
                     make_gamein_ack_at(25000, 25000));
    mxh::game::ItemTotalInfo items{};
    items.Inventory[4] = mxh::game::make_item(7002u, 102u, 4u, 100u, 1u);
    mxh::net::Message inventory;
    inventory.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    inventory.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::TotalInfoLocal);
    inventory.payload.resize(sizeof(items));
    std::memcpy(inventory.payload.data(), &items, sizeof(items));
    state.on_message(mxh::net::make_connection_id(1), inventory);
    mxh::net::Message ack;
    ack.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    ack.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::DiscardAck);
    const std::uint16_t pos = 4u;
    ack.payload.resize(sizeof(pos));
    std::memcpy(ack.payload.data(), &pos, sizeof(pos));
    state.on_message(mxh::net::make_connection_id(1), ack);
    EXPECT_TRUE(mxh::game::is_empty_slot(state.game_info().items.Inventory[4]));
}

TEST(InGamePlayable, MoveAckReordersLiveInventorySlots) {
    mxh::client::CInGameState state;
    state.Init(nullptr);
    state.on_message(mxh::net::make_connection_id(1),
                     make_gamein_ack_at(25000, 25000));
    mxh::game::ItemTotalInfo items{};
    items.Inventory[1] = mxh::game::make_item(7101u, 201u, 1u, 100u, 1u);
    items.Inventory[5] = mxh::game::make_item(7102u, 202u, 5u, 100u, 1u);
    mxh::net::Message inventory;
    inventory.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    inventory.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::TotalInfoLocal);
    inventory.payload.resize(sizeof(items));
    std::memcpy(inventory.payload.data(), &items, sizeof(items));
    state.on_message(mxh::net::make_connection_id(1), inventory);
    mxh::net::Message ack;
    ack.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    ack.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::MoveAck);
    ack.payload.resize(24, 0);
    const std::uint32_t db_idx = 7101u;
    const std::uint16_t target = 5u;
    std::memcpy(ack.payload.data(), &db_idx, 4);
    std::memcpy(ack.payload.data() + 22, &target, 2);
    state.on_message(mxh::net::make_connection_id(1), ack);
    EXPECT_EQ(state.game_info().items.Inventory[1].dwDBIdx, 7102u);
    EXPECT_EQ(state.game_info().items.Inventory[5].dwDBIdx, 7101u);
    EXPECT_EQ(state.game_info().items.Inventory[5].Position, 5u);
}

TEST(InGamePlayable, MoveAckAppliesAuthoritativeItemFields) {
    mxh::client::CInGameState state;
    state.Init(nullptr);
    state.on_message(mxh::net::make_connection_id(1),
                     make_gamein_ack_at(25000, 25000));
    mxh::game::ItemTotalInfo items{};
    items.Inventory[3] = mxh::game::make_item(7201u, 301u, 3u, 40u, 2u);
    mxh::net::Message inventory;
    inventory.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    inventory.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::TotalInfoLocal);
    inventory.payload.resize(sizeof(items));
    std::memcpy(inventory.payload.data(), &items, sizeof(items));
    state.on_message(mxh::net::make_connection_id(1), inventory);

    mxh::net::Message ack;
    ack.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    ack.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::MoveAck);
    ack.payload.resize(24, 0);
    const auto authoritative = mxh::game::make_item(7201u, 301u, 0u, 77u, 9u);
    std::memcpy(ack.payload.data(), &authoritative, sizeof(authoritative));
    const std::uint16_t target = 6u;
    std::memcpy(ack.payload.data() + 22, &target, sizeof(target));
    state.on_message(mxh::net::make_connection_id(1), ack);

    EXPECT_EQ(state.game_info().items.Inventory[6].dwDBIdx, 7201u);
    EXPECT_EQ(state.game_info().items.Inventory[6].Durability, 77u);
    EXPECT_EQ(state.game_info().items.Inventory[6].ItemParam, 9u);
    EXPECT_EQ(state.game_info().items.Inventory[6].Position, 6u);
    EXPECT_TRUE(mxh::game::is_empty_slot(state.game_info().items.Inventory[3]));
}

TEST(InGamePlayable, MoveNackShowsVisibleItemMoveMessage) {
    mxh::client::CInGameState state;
    state.Init(nullptr);
    mxh::net::Message nack;
    nack.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    nack.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::MoveNack);
    state.on_message({}, nack);
    EXPECT_TRUE(state.ui_runtime().hasModal());
}

TEST(InGamePlayable, MoveAckUpdatesEquipmentAppearanceSlots) {
    mxh::client::CInGameState state;
    state.Init(nullptr);
    state.on_message(mxh::net::make_connection_id(1),
                     make_gamein_ack_at(25000, 25000));
    mxh::game::ItemTotalInfo items{};
    items.Inventory[2] = mxh::game::make_item(7301u, 401u, 2u, 100u, 1u);
    mxh::net::Message inventory;
    inventory.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    inventory.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::TotalInfoLocal);
    inventory.payload.resize(sizeof(items));
    std::memcpy(inventory.payload.data(), &items, sizeof(items));
    state.on_message(mxh::net::make_connection_id(1), inventory);

    mxh::net::Message ack;
    ack.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    ack.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::MoveAck);
    ack.payload.resize(24, 0);
    const std::uint32_t db_idx = 7301u;
    const std::uint16_t target = mxh::game::TP_WEAREDITEM_START + 2u;
    std::memcpy(ack.payload.data(), &db_idx, 4);
    std::memcpy(ack.payload.data() + 22, &target, 2);
    state.on_message(mxh::net::make_connection_id(1), ack);

    EXPECT_TRUE(mxh::game::is_empty_slot(state.game_info().items.Inventory[2]));
    EXPECT_EQ(state.game_info().items.WearedItem[2].dwDBIdx, db_idx);
    EXPECT_EQ(state.game_info().items.WearedItem[2].Position, target);
    EXPECT_EQ(state.game_info().weared_item_idx[2], 401u);
}

TEST(InGamePlayable, QuestChangeStateUpdatesLiveQuestStatus) {
    mxh::client::CInGameState state;
    state.Init(nullptr);
    state.on_message(mxh::net::make_connection_id(1),
                     make_gamein_ack_at(25000, 25000));
    mxh::net::Message change;
    change.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Quest);
    change.header.protocol = static_cast<std::uint8_t>(mxh::proto::QuestProtocol::ChangeState);
    change.payload.resize(8, 0);
    const std::uint32_t quest_id = 1u;
    const std::uint32_t complete = 2u;
    std::memcpy(change.payload.data(), &quest_id, 4);
    std::memcpy(change.payload.data() + 4, &complete, 4);
    state.on_message(mxh::net::make_connection_id(1), change);
    EXPECT_EQ(state.quest_id(), 1u);
    EXPECT_EQ(state.quest_status(), "Ready to claim");
    change.header.protocol = static_cast<std::uint8_t>(mxh::proto::QuestProtocol::TotalInfo);
    const std::uint32_t rewarded = 3u;
    std::memcpy(change.payload.data() + 4, &rewarded, 4);
    state.on_message(mxh::net::make_connection_id(1), change);
    EXPECT_EQ(state.quest_status(), "Reward claimed");
}

TEST(InGamePlayable, QuestRewardPacketsRefreshMoneyAndInventoryBeforeEndAck) {
    mxh::client::CInGameState state;
    state.Init(nullptr);
    state.on_message(mxh::net::make_connection_id(1),
                     make_gamein_ack_at(25000, 25000));
    mxh::net::Message money;
    money.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    money.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::Money);
    const std::uint32_t updated_money = 12345u;
    money.payload.resize(4);
    std::memcpy(money.payload.data(), &updated_money, 4);
    state.on_message(mxh::net::make_connection_id(1), money);

    mxh::game::ItemTotalInfo items{};
    items.Inventory[0] = mxh::game::make_item(7401u, 8000u, 0u, 100u, 2u);
    mxh::net::Message total;
    total.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    total.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::TotalInfoLocal);
    total.payload.resize(sizeof(items));
    std::memcpy(total.payload.data(), &items, sizeof(items));
    state.on_message(mxh::net::make_connection_id(1), total);

    mxh::net::Message end_ack;
    end_ack.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Quest);
    end_ack.header.protocol = static_cast<std::uint8_t>(mxh::proto::QuestProtocol::EndAck);
    end_ack.payload.resize(2, 0);
    state.on_message(mxh::net::make_connection_id(1), end_ack);
    EXPECT_EQ(state.game_info().money, updated_money);
    EXPECT_EQ(state.game_info().items.Inventory[0].dwDBIdx, 7401u);
    EXPECT_EQ(state.quest_status(), "Reward claimed");
}

TEST(InGamePlayable, SellAckRemovesSoldQuantityFromLiveInventory) {
    mxh::client::CInGameState state;
    state.Init(nullptr);
    state.on_message(mxh::net::make_connection_id(1),
                     make_gamein_ack_at(25000, 25000));
    mxh::game::ItemTotalInfo items{};
    items.Inventory[6] = mxh::game::make_item(7201u, 301u, 6u, 100u, 3u);
    mxh::net::Message inventory;
    inventory.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    inventory.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::TotalInfoLocal);
    inventory.payload.resize(sizeof(items));
    std::memcpy(inventory.payload.data(), &items, sizeof(items));
    state.on_message(mxh::net::make_connection_id(1), inventory);
    mxh::net::Message ack;
    ack.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    ack.header.protocol = static_cast<std::uint8_t>(mxh::proto::ItemProtocol::SellAck);
    ack.payload.resize(8, 0);
    const std::uint16_t pos = 6u, item_idx = 301u, quantity = 2u, dealer = 7u;
    std::memcpy(ack.payload.data(), &pos, 2);
    std::memcpy(ack.payload.data() + 2, &item_idx, 2);
    std::memcpy(ack.payload.data() + 4, &quantity, 2);
    std::memcpy(ack.payload.data() + 6, &dealer, 2);
    state.on_message(mxh::net::make_connection_id(1), ack);
    EXPECT_EQ(state.game_info().items.Inventory[6].ItemParam, 1u);
}

TEST(InGameWire, PickupMessageAndGroundDropPayloadRoundTrip) {
    const auto m = make_pickup_message(42u, 9001u);
    EXPECT_EQ(m.header.category,
              static_cast<std::uint8_t>(mxh::proto::Category::Item));
    EXPECT_EQ(m.header.protocol,
              static_cast<std::uint8_t>(mxh::proto::ItemProtocol::PickupSyn));
    ASSERT_EQ(m.payload.size(), 4u);
    EXPECT_EQ(m.payload[0], 0x29);
    EXPECT_EQ(m.payload[1], 0x23);

    std::vector<std::uint8_t> payload(20, 0);
    const std::uint32_t drop_id = 7u;
    const std::uint16_t item_id = 88u;
    const float x = 10.0f;
    const float z = 20.0f;
    std::memcpy(payload.data(), &drop_id, 4);
    std::memcpy(payload.data() + 8, &item_id, 2);
    payload[10] = 1;
    std::memcpy(payload.data() + 12, &x, 4);
    std::memcpy(payload.data() + 16, &z, 4);
    const auto drop = parse_legacy_ground_drop(payload);
    ASSERT_TRUE(drop.has_value());
    EXPECT_EQ(drop->object_id, 7u);
    EXPECT_EQ(drop->item_id, 88u);
    EXPECT_FLOAT_EQ(drop->position_x, 10.0f);
    EXPECT_FLOAT_EQ(drop->position_z, 20.0f);
}
