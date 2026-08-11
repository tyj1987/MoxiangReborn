#include "CInGameState.hpp"

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
using mxh::client::MoveKey;
using mxh::client::parse_monster_life_payload;
using mxh::client::parse_shop_list;
using mxh::client::parse_chat_payload;
using mxh::client::parse_move_payload;
using mxh::client::pick_attack_target;
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
    EXPECT_EQ(quick_skill_for_slot(info, 0), 1u);
    EXPECT_EQ(quick_skill_for_slot(info, 1), 2u);
    EXPECT_EQ(quick_skill_for_slot(info, 2), 3u);
    EXPECT_EQ(quick_skill_for_slot(info, 3), 10u);
    EXPECT_EQ(quick_skill_for_slot(info, 4), 0u);
    EXPECT_EQ(quick_skill_for_slot(info, 8), 0u);  // out of range
}

TEST(InGameQuickSlot, ParsedMugongWinsOverStarter) {
    mxh::client::GameInInfo info;
    info.mugong[0].icon_idx = 42;
    EXPECT_EQ(quick_skill_for_slot(info, 0), 42u);
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
