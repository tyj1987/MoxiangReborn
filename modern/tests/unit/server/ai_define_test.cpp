// ai_define_test.cpp - Phase D6 AIDefine 1:1 port tests.

#include "mxh/server/ai_define.hpp"

#include <gtest/gtest.h>

namespace {

using mxh::server::StateEvent;
using mxh::server::MonsterEmotion;
using mxh::server::MessageKind;

TEST(AIDefine, StateEventOrderingMatchesLegacy) {
    EXPECT_EQ(static_cast<std::uint8_t>(StateEvent::Null),    0u);
    EXPECT_EQ(static_cast<std::uint8_t>(StateEvent::Process), 1u);
    EXPECT_EQ(static_cast<std::uint8_t>(StateEvent::Message), 2u);
    EXPECT_EQ(static_cast<std::uint8_t>(StateEvent::Enter),   3u);
    EXPECT_EQ(static_cast<std::uint8_t>(StateEvent::Leave),   4u);
}

TEST(AIDefine, MonsterEmotionOrderingMatchesLegacy) {
    EXPECT_EQ(static_cast<std::uint8_t>(MonsterEmotion::Pleasure), 0u);
    EXPECT_EQ(static_cast<std::uint8_t>(MonsterEmotion::Comfort),  1u);
    EXPECT_EQ(static_cast<std::uint8_t>(MonsterEmotion::Doze),     2u);
    EXPECT_EQ(static_cast<std::uint8_t>(MonsterEmotion::Sad),      3u);
    EXPECT_EQ(static_cast<std::uint8_t>(MonsterEmotion::Anger),    4u);
}

TEST(AIDefine, MessageKindOrderingMatchesLegacy) {
    EXPECT_EQ(static_cast<std::uint8_t>(MessageKind::Chat),         0u);
    EXPECT_EQ(static_cast<std::uint8_t>(MessageKind::RecallDirect), 1u);
    EXPECT_EQ(static_cast<std::uint8_t>(MessageKind::RecallScript), 2u);
    EXPECT_EQ(static_cast<std::uint8_t>(MessageKind::PlayerCurPos), 3u);
    EXPECT_EQ(static_cast<std::uint8_t>(MessageKind::MobCurPos),    4u);
    EXPECT_EQ(static_cast<std::uint8_t>(MessageKind::HelpRequest),  5u);
    EXPECT_EQ(static_cast<std::uint8_t>(MessageKind::HelpShout),    6u);
    EXPECT_EQ(static_cast<std::uint8_t>(MessageKind::HelpObey),     7u);
}

TEST(AIDefine, DistinctValuesAreNotAmbiguous) {
    // Sanity: every enum value is unique within its kind.
    EXPECT_NE(MonsterEmotion::Pleasure, MonsterEmotion::Comfort);
    EXPECT_NE(MonsterEmotion::Comfort,  MonsterEmotion::Doze);
    EXPECT_NE(MessageKind::Chat,        MessageKind::HelpObey);
}

}  // namespace
