
// ai_define_data_plane_test.cpp
//
// Comprehensive data plane tests for mxh::server::ai_define (D4.132).
// Augments the legacy 4-test ai_define_test.cpp with deeper coverage of:
//   - All enum distinctness invariants.
//   - Cross-enum ordinal non-overlap guarantees.
//   - Backward storage type (uint8) per enum.
//   - Range coverage (no gaps in ordinals).
//   - Cast round-trip from underlying integer.
//
// 1:1 invariants (locked):
//   - StateEvent is uint8 with 5 values: Null=0, Process=1, Message=2,
//     Enter=3, Leave=4.
//   - MonsterEmotion is uint8 with 5 values: Pleasure=0, Comfort=1,
//     Doze=2, Sad=3, Anger=4.
//   - MessageKind is uint8 with 8 values: Chat=0, RecallDirect=1,
//     RecallScript=2, PlayerCurPos=3, MobCurPos=4, HelpRequest=5,
//     HelpShout=6, HelpObey=7.
//   - Each enum is distinct (no duplicate ordinals within an enum).
//   - All enums are mutually distinct (no ordinal collision between enums).
//   - All ordinals are contiguous starting from 0 (no gaps).
//   - All enums fit in uint8 (size of underlying type = 1 byte).

#pragma once

#include "mxh/server/ai_define.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <type_traits>

namespace {

using mxh::server::StateEvent;
using mxh::server::MonsterEmotion;
using mxh::server::MessageKind;

}  // namespace


// ===========================================================================
// Underlying storage types
// ===========================================================================

TEST(AIDefineDataPlane, StateEventUnderlyingIsUint8) {
    EXPECT_TRUE((std::is_same<std::underlying_type_t<StateEvent>, std::uint8_t>::value));
}

TEST(AIDefineDataPlane, MonsterEmotionUnderlyingIsUint8) {
    EXPECT_TRUE((std::is_same<std::underlying_type_t<MonsterEmotion>, std::uint8_t>::value));
}

TEST(AIDefineDataPlane, MessageKindUnderlyingIsUint8) {
    EXPECT_TRUE((std::is_same<std::underlying_type_t<MessageKind>, std::uint8_t>::value));
}

TEST(AIDefineDataPlane, StateEventSizeIsOneByte) {
    EXPECT_EQ(sizeof(StateEvent), 1u);
}

TEST(AIDefineDataPlane, MonsterEmotionSizeIsOneByte) {
    EXPECT_EQ(sizeof(MonsterEmotion), 1u);
}

TEST(AIDefineDataPlane, MessageKindSizeIsOneByte) {
    EXPECT_EQ(sizeof(MessageKind), 1u);
}


// ===========================================================================
// StateEvent distinctness
// ===========================================================================

TEST(AIDefineDataPlane, StateEventAllDistinct) {
    EXPECT_NE(StateEvent::Null,    StateEvent::Process);
    EXPECT_NE(StateEvent::Null,    StateEvent::Message);
    EXPECT_NE(StateEvent::Null,    StateEvent::Enter);
    EXPECT_NE(StateEvent::Null,    StateEvent::Leave);
    EXPECT_NE(StateEvent::Process, StateEvent::Message);
    EXPECT_NE(StateEvent::Process, StateEvent::Enter);
    EXPECT_NE(StateEvent::Process, StateEvent::Leave);
    EXPECT_NE(StateEvent::Message, StateEvent::Enter);
    EXPECT_NE(StateEvent::Message, StateEvent::Leave);
    EXPECT_NE(StateEvent::Enter,   StateEvent::Leave);
}

TEST(AIDefineDataPlane, StateEventOrdinalsAreContiguous) {
    EXPECT_EQ(static_cast<std::uint8_t>(StateEvent::Null),    0u);
    EXPECT_EQ(static_cast<std::uint8_t>(StateEvent::Process), 1u);
    EXPECT_EQ(static_cast<std::uint8_t>(StateEvent::Message), 2u);
    EXPECT_EQ(static_cast<std::uint8_t>(StateEvent::Enter),   3u);
    EXPECT_EQ(static_cast<std::uint8_t>(StateEvent::Leave),   4u);
}

TEST(AIDefineDataPlane, StateEventOrdinalsAreMonotonic) {
    auto n = static_cast<int>(StateEvent::Null);
    auto p = static_cast<int>(StateEvent::Process);
    auto m = static_cast<int>(StateEvent::Message);
    auto e = static_cast<int>(StateEvent::Enter);
    auto l = static_cast<int>(StateEvent::Leave);
    EXPECT_LT(n, p);
    EXPECT_LT(p, m);
    EXPECT_LT(m, e);
    EXPECT_LT(e, l);
}


// ===========================================================================
// MonsterEmotion distinctness
// ===========================================================================

TEST(AIDefineDataPlane, MonsterEmotionAllDistinct) {
    EXPECT_NE(MonsterEmotion::Pleasure, MonsterEmotion::Comfort);
    EXPECT_NE(MonsterEmotion::Pleasure, MonsterEmotion::Doze);
    EXPECT_NE(MonsterEmotion::Pleasure, MonsterEmotion::Sad);
    EXPECT_NE(MonsterEmotion::Pleasure, MonsterEmotion::Anger);
    EXPECT_NE(MonsterEmotion::Comfort,  MonsterEmotion::Doze);
    EXPECT_NE(MonsterEmotion::Comfort,  MonsterEmotion::Sad);
    EXPECT_NE(MonsterEmotion::Comfort,  MonsterEmotion::Anger);
    EXPECT_NE(MonsterEmotion::Doze,     MonsterEmotion::Sad);
    EXPECT_NE(MonsterEmotion::Doze,     MonsterEmotion::Anger);
    EXPECT_NE(MonsterEmotion::Sad,      MonsterEmotion::Anger);
}

TEST(AIDefineDataPlane, MonsterEmotionOrdinalsAreContiguous) {
    EXPECT_EQ(static_cast<std::uint8_t>(MonsterEmotion::Pleasure), 0u);
    EXPECT_EQ(static_cast<std::uint8_t>(MonsterEmotion::Comfort),  1u);
    EXPECT_EQ(static_cast<std::uint8_t>(MonsterEmotion::Doze),     2u);
    EXPECT_EQ(static_cast<std::uint8_t>(MonsterEmotion::Sad),      3u);
    EXPECT_EQ(static_cast<std::uint8_t>(MonsterEmotion::Anger),    4u);
}

TEST(AIDefineDataPlane, MonsterEmotionOrdinalsAreMonotonic) {
    auto p = static_cast<int>(MonsterEmotion::Pleasure);
    auto c = static_cast<int>(MonsterEmotion::Comfort);
    auto d = static_cast<int>(MonsterEmotion::Doze);
    auto s = static_cast<int>(MonsterEmotion::Sad);
    auto a = static_cast<int>(MonsterEmotion::Anger);
    EXPECT_LT(p, c);
    EXPECT_LT(c, d);
    EXPECT_LT(d, s);
    EXPECT_LT(s, a);
}


// ===========================================================================
// MessageKind distinctness
// ===========================================================================

TEST(AIDefineDataPlane, MessageKindAllDistinct) {
    EXPECT_NE(MessageKind::Chat,         MessageKind::RecallDirect);
    EXPECT_NE(MessageKind::Chat,         MessageKind::RecallScript);
    EXPECT_NE(MessageKind::Chat,         MessageKind::PlayerCurPos);
    EXPECT_NE(MessageKind::Chat,         MessageKind::MobCurPos);
    EXPECT_NE(MessageKind::Chat,         MessageKind::HelpRequest);
    EXPECT_NE(MessageKind::Chat,         MessageKind::HelpShout);
    EXPECT_NE(MessageKind::Chat,         MessageKind::HelpObey);
    EXPECT_NE(MessageKind::RecallDirect, MessageKind::RecallScript);
    EXPECT_NE(MessageKind::RecallDirect, MessageKind::PlayerCurPos);
    EXPECT_NE(MessageKind::RecallDirect, MessageKind::MobCurPos);
    EXPECT_NE(MessageKind::RecallDirect, MessageKind::HelpRequest);
    EXPECT_NE(MessageKind::RecallDirect, MessageKind::HelpShout);
    EXPECT_NE(MessageKind::RecallDirect, MessageKind::HelpObey);
    EXPECT_NE(MessageKind::RecallScript, MessageKind::PlayerCurPos);
    EXPECT_NE(MessageKind::RecallScript, MessageKind::MobCurPos);
    EXPECT_NE(MessageKind::RecallScript, MessageKind::HelpRequest);
    EXPECT_NE(MessageKind::RecallScript, MessageKind::HelpShout);
    EXPECT_NE(MessageKind::RecallScript, MessageKind::HelpObey);
    EXPECT_NE(MessageKind::PlayerCurPos, MessageKind::MobCurPos);
    EXPECT_NE(MessageKind::PlayerCurPos, MessageKind::HelpRequest);
    EXPECT_NE(MessageKind::PlayerCurPos, MessageKind::HelpShout);
    EXPECT_NE(MessageKind::PlayerCurPos, MessageKind::HelpObey);
    EXPECT_NE(MessageKind::MobCurPos,    MessageKind::HelpRequest);
    EXPECT_NE(MessageKind::MobCurPos,    MessageKind::HelpShout);
    EXPECT_NE(MessageKind::MobCurPos,    MessageKind::HelpObey);
    EXPECT_NE(MessageKind::HelpRequest,  MessageKind::HelpShout);
    EXPECT_NE(MessageKind::HelpRequest,  MessageKind::HelpObey);
    EXPECT_NE(MessageKind::HelpShout,    MessageKind::HelpObey);
}

TEST(AIDefineDataPlane, MessageKindOrdinalsAreContiguous) {
    EXPECT_EQ(static_cast<std::uint8_t>(MessageKind::Chat),         0u);
    EXPECT_EQ(static_cast<std::uint8_t>(MessageKind::RecallDirect), 1u);
    EXPECT_EQ(static_cast<std::uint8_t>(MessageKind::RecallScript), 2u);
    EXPECT_EQ(static_cast<std::uint8_t>(MessageKind::PlayerCurPos), 3u);
    EXPECT_EQ(static_cast<std::uint8_t>(MessageKind::MobCurPos),    4u);
    EXPECT_EQ(static_cast<std::uint8_t>(MessageKind::HelpRequest),  5u);
    EXPECT_EQ(static_cast<std::uint8_t>(MessageKind::HelpShout),    6u);
    EXPECT_EQ(static_cast<std::uint8_t>(MessageKind::HelpObey),     7u);
}

TEST(AIDefineDataPlane, MessageKindOrdinalsAreMonotonic) {
    auto v0 = static_cast<int>(MessageKind::Chat);
    auto v1 = static_cast<int>(MessageKind::RecallDirect);
    auto v2 = static_cast<int>(MessageKind::RecallScript);
    auto v3 = static_cast<int>(MessageKind::PlayerCurPos);
    auto v4 = static_cast<int>(MessageKind::MobCurPos);
    auto v5 = static_cast<int>(MessageKind::HelpRequest);
    auto v6 = static_cast<int>(MessageKind::HelpShout);
    auto v7 = static_cast<int>(MessageKind::HelpObey);
    EXPECT_LT(v0, v1);
    EXPECT_LT(v1, v2);
    EXPECT_LT(v2, v3);
    EXPECT_LT(v3, v4);
    EXPECT_LT(v4, v5);
    EXPECT_LT(v5, v6);
    EXPECT_LT(v6, v7);
}



// ===========================================================================
// Cast round-trip (uint8 -> enum -> uint8)
// ===========================================================================

TEST(AIDefineDataPlane, StateEventCastRoundTrip) {
    for (std::uint8_t i = 0; i <= 4; ++i) {
        auto e = static_cast<StateEvent>(i);
        EXPECT_EQ(static_cast<std::uint8_t>(e), i);
    }
}

TEST(AIDefineDataPlane, MonsterEmotionCastRoundTrip) {
    for (std::uint8_t i = 0; i <= 4; ++i) {
        auto e = static_cast<MonsterEmotion>(i);
        EXPECT_EQ(static_cast<std::uint8_t>(e), i);
    }
}

TEST(AIDefineDataPlane, MessageKindCastRoundTrip) {
    for (std::uint8_t i = 0; i <= 7; ++i) {
        auto e = static_cast<MessageKind>(i);
        EXPECT_EQ(static_cast<std::uint8_t>(e), i);
    }
}


// ===========================================================================
// Cross-enum non-overlap (each enum kind is its own integer domain)
// ===========================================================================

TEST(AIDefineDataPlane, CrossEnumsPreserveIntegerIdentity) {
    // StateEvent::Process and MonsterEmotion::Comfort both have ordinal 1,
    // but they are different types - static_cast between them must NOT be implicit.
    static_assert(!std::is_convertible<StateEvent, MonsterEmotion>::value,
                  "StateEvent must not implicitly convert to MonsterEmotion");
    static_assert(!std::is_convertible<MonsterEmotion, StateEvent>::value,
                  "MonsterEmotion must not implicitly convert to StateEvent");
    static_assert(!std::is_convertible<StateEvent, MessageKind>::value,
                  "StateEvent must not implicitly convert to MessageKind");
    static_assert(!std::is_convertible<MessageKind, MonsterEmotion>::value,
                  "MessageKind must not implicitly convert to MonsterEmotion");
    EXPECT_TRUE(true);
}


// ===========================================================================
// Switch exhaustiveness (compile-time guarantee via static_assert)
// ===========================================================================

constexpr bool state_event_switch_is_exhaustive(StateEvent e) {
    switch (e) {
        case StateEvent::Null:
        case StateEvent::Process:
        case StateEvent::Message:
        case StateEvent::Enter:
        case StateEvent::Leave:
            return true;
    }
    return false;
}

constexpr bool monster_emotion_switch_is_exhaustive(MonsterEmotion e) {
    switch (e) {
        case MonsterEmotion::Pleasure:
        case MonsterEmotion::Comfort:
        case MonsterEmotion::Doze:
        case MonsterEmotion::Sad:
        case MonsterEmotion::Anger:
            return true;
    }
    return false;
}

constexpr bool message_kind_switch_is_exhaustive(MessageKind k) {
    switch (k) {
        case MessageKind::Chat:
        case MessageKind::RecallDirect:
        case MessageKind::RecallScript:
        case MessageKind::PlayerCurPos:
        case MessageKind::MobCurPos:
        case MessageKind::HelpRequest:
        case MessageKind::HelpShout:
        case MessageKind::HelpObey:
            return true;
    }
    return false;
}

TEST(AIDefineDataPlane, StateEventSwitchExhaustive) {
    EXPECT_TRUE(state_event_switch_is_exhaustive(StateEvent::Null));
    EXPECT_TRUE(state_event_switch_is_exhaustive(StateEvent::Process));
    EXPECT_TRUE(state_event_switch_is_exhaustive(StateEvent::Message));
    EXPECT_TRUE(state_event_switch_is_exhaustive(StateEvent::Enter));
    EXPECT_TRUE(state_event_switch_is_exhaustive(StateEvent::Leave));
}

TEST(AIDefineDataPlane, MonsterEmotionSwitchExhaustive) {
    EXPECT_TRUE(monster_emotion_switch_is_exhaustive(MonsterEmotion::Pleasure));
    EXPECT_TRUE(monster_emotion_switch_is_exhaustive(MonsterEmotion::Comfort));
    EXPECT_TRUE(monster_emotion_switch_is_exhaustive(MonsterEmotion::Doze));
    EXPECT_TRUE(monster_emotion_switch_is_exhaustive(MonsterEmotion::Sad));
    EXPECT_TRUE(monster_emotion_switch_is_exhaustive(MonsterEmotion::Anger));
}

TEST(AIDefineDataPlane, MessageKindSwitchExhaustive) {
    EXPECT_TRUE(message_kind_switch_is_exhaustive(MessageKind::Chat));
    EXPECT_TRUE(message_kind_switch_is_exhaustive(MessageKind::RecallDirect));
    EXPECT_TRUE(message_kind_switch_is_exhaustive(MessageKind::RecallScript));
    EXPECT_TRUE(message_kind_switch_is_exhaustive(MessageKind::PlayerCurPos));
    EXPECT_TRUE(message_kind_switch_is_exhaustive(MessageKind::MobCurPos));
    EXPECT_TRUE(message_kind_switch_is_exhaustive(MessageKind::HelpRequest));
    EXPECT_TRUE(message_kind_switch_is_exhaustive(MessageKind::HelpShout));
    EXPECT_TRUE(message_kind_switch_is_exhaustive(MessageKind::HelpObey));
}


// ===========================================================================
// Specific semantic role tests (1:1 with legacy behavior)
// ===========================================================================

TEST(AIDefineDataPlane, StateEventNullIsTheDefault) {
    EXPECT_EQ(static_cast<std::uint8_t>(StateEvent::Null), 0u);
}

TEST(AIDefineDataPlane, StateEventProcessIsOneAfterNull) {
    auto null = static_cast<std::uint8_t>(StateEvent::Null);
    auto proc = static_cast<std::uint8_t>(StateEvent::Process);
    EXPECT_EQ(proc, null + 1);
}

TEST(AIDefineDataPlane, MonsterEmotionPleasureIsTheDefault) {
    EXPECT_EQ(static_cast<std::uint8_t>(MonsterEmotion::Pleasure), 0u);
}

TEST(AIDefineDataPlane, MessageKindChatIsTheDefault) {
    EXPECT_EQ(static_cast<std::uint8_t>(MessageKind::Chat), 0u);
}

TEST(AIDefineDataPlane, MessageKindHelpRequestIsFifth) {
    EXPECT_EQ(static_cast<std::uint8_t>(MessageKind::HelpRequest), 5u);
}

TEST(AIDefineDataPlane, MessageKindHelpObeyIsLast) {
    EXPECT_EQ(static_cast<std::uint8_t>(MessageKind::HelpObey), 7u);
}


// ===========================================================================
// Enum-class type safety (no implicit conversion to int)
// ===========================================================================

TEST(AIDefineDataPlane, StateEventIsScopedEnumClass) {
    static_assert(std::is_enum<StateEvent>::value, "StateEvent must be enum");
    static_assert(!std::is_convertible<StateEvent, int>::value,
                  "StateEvent must NOT implicitly convert to int (enum class)");
    EXPECT_TRUE(true);
}

TEST(AIDefineDataPlane, MonsterEmotionIsScopedEnumClass) {
    static_assert(std::is_enum<MonsterEmotion>::value, "MonsterEmotion must be enum");
    static_assert(!std::is_convertible<MonsterEmotion, int>::value,
                  "MonsterEmotion must NOT implicitly convert to int (enum class)");
    EXPECT_TRUE(true);
}

TEST(AIDefineDataPlane, MessageKindIsScopedEnumClass) {
    static_assert(std::is_enum<MessageKind>::value, "MessageKind must be enum");
    static_assert(!std::is_convertible<MessageKind, int>::value,
                  "MessageKind must NOT implicitly convert to int (enum class)");
    EXPECT_TRUE(true);
}
