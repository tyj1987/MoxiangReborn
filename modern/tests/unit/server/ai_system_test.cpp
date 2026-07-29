#include <gtest/gtest.h>

#include "mxh/server/ai_system.hpp"
#include "mxh/server/object.hpp"

#include <set>

using namespace mxh::server;

namespace {

class TestObject : public Object {
public:
    TestObject() = default;
    std::uint32_t ping_count = 0;
    void ping() { ping_count++; }
};

TestObject* make_test_object(std::uint32_t id) {
    auto* obj = new TestObject();
    BaseObjectInfo info{};
    info.dw_object_id = id;
    EXPECT_TRUE(obj->init(ObjectKind::Monster, 0u, &info));
    return obj;
}

}  // namespace

// -----------------------------------------------------------------------------
// AiState enum (7 states + Max) matches legacy CStateMachinen transitions.
// -----------------------------------------------------------------------------
TEST(AISystemTest, AiStateEnumMirrorsLegacy) {
    EXPECT_EQ(static_cast<std::uint8_t>(AiState::Stand),      0u);
    EXPECT_EQ(static_cast<std::uint8_t>(AiState::WalkAround), 1u);
    EXPECT_EQ(static_cast<std::uint8_t>(AiState::Pursuit),    2u);
    EXPECT_EQ(static_cast<std::uint8_t>(AiState::Attack),     3u);
    EXPECT_EQ(static_cast<std::uint8_t>(AiState::RunAway),    4u);
    EXPECT_EQ(static_cast<std::uint8_t>(AiState::Rest),       5u);
    EXPECT_EQ(static_cast<std::uint8_t>(AiState::Dead),       6u);
    EXPECT_EQ(static_cast<std::uint8_t>(AiState::Max),        7u);
}

// -----------------------------------------------------------------------------
// AddObject / RemoveObject / tracked_count lifecycle.
// -----------------------------------------------------------------------------
TEST(AISystemTest, AddObjectTracksAndReturnsTrue) {
    AISystem& ai = AISystem::instance();
    ai.remove_all_list();
    auto* obj = make_test_object(101u);

    EXPECT_FALSE(ai.is_tracked(obj));
    EXPECT_TRUE(ai.add_object(obj));
    EXPECT_TRUE(ai.is_tracked(obj));
    EXPECT_EQ(ai.tracked_count(), 1u);

    Object* removed = ai.remove_object(101u);
    EXPECT_EQ(removed, obj);
    EXPECT_FALSE(ai.is_tracked(obj));
    EXPECT_EQ(ai.tracked_count(), 0u);
    delete obj;
}

TEST(AISystemTest, AddObjectRejectsDuplicate) {
    AISystem& ai = AISystem::instance();
    ai.remove_all_list();
    auto* obj = make_test_object(200u);

    EXPECT_TRUE(ai.add_object(obj));
    EXPECT_FALSE(ai.add_object(obj));
    EXPECT_EQ(ai.tracked_count(), 1u);
    delete obj;
}

TEST(AISystemTest, AddObjectRejectsNullPtr) {
    AISystem& ai = AISystem::instance();
    ai.remove_all_list();
    EXPECT_FALSE(ai.add_object(nullptr));
    EXPECT_EQ(ai.tracked_count(), 0u);
}

TEST(AISystemTest, RemoveObjectUnknownIdReturnsNull) {
    AISystem& ai = AISystem::instance();
    ai.remove_all_list();
    EXPECT_EQ(ai.remove_object(99999u), nullptr);
}

// -----------------------------------------------------------------------------
// Monster ID generator + release semantics.
// -----------------------------------------------------------------------------
TEST(AISystemTest, GenerateMonsterIdMonotonic) {
    AISystem& ai = AISystem::instance();
    ai.remove_all_list();
    std::uint32_t a = ai.generate_monster_id();
    std::uint32_t b = ai.generate_monster_id();
    std::uint32_t c = ai.generate_monster_id();
    EXPECT_EQ(a + 1u, b);
    EXPECT_EQ(b + 1u, c);
}

TEST(AISystemTest, ReleaseMonsterIdReusesOnNextGenerate) {
    AISystem& ai = AISystem::instance();
    ai.remove_all_list();
    std::uint32_t first = ai.generate_monster_id();  // 1
    std::uint32_t second = ai.generate_monster_id(); // 2
    ai.release_monster_id(first);
    EXPECT_EQ(ai.generate_monster_id(), first);
    EXPECT_NE(ai.generate_monster_id(), second);
}

TEST(AISystemTest, ReleaseMonsterIdDuplicateIsNoOp) {
    AISystem& ai = AISystem::instance();
    ai.remove_all_list();
    std::uint32_t id = ai.generate_monster_id();
    ai.release_monster_id(id);
    ai.release_monster_id(id);  // duplicate: should not crash
    ai.generate_monster_id();    // returns id (only one entry)
    std::uint32_t peek = ai.next_monster_id();
    EXPECT_GT(peek, id);
}

// -----------------------------------------------------------------------------
// SetTransition dispatches into the per-object transition record.
// -----------------------------------------------------------------------------
TEST(AISystemTest, SetTransitionUpdatesPerObject) {
    AISystem& ai = AISystem::instance();
    ai.remove_all_list();
    auto* obj = make_test_object(500u);
    ai.add_object(obj);

    EXPECT_EQ(ai.last_transition_for(obj), AiState::Stand);
    ai.set_transition(obj, AiState::Pursuit);
    EXPECT_EQ(ai.last_transition_for(obj), AiState::Pursuit);
    ai.set_transition(obj, AiState::Attack);
    EXPECT_EQ(ai.last_transition_for(obj), AiState::Attack);
    ai.set_transition(obj, AiState::Dead);
    EXPECT_EQ(ai.last_transition_for(obj), AiState::Dead);

    delete obj;
}

TEST(AISystemTest, SetTransitionOnUntrackedIsNoOp) {
    AISystem& ai = AISystem::instance();
    ai.remove_all_list();
    auto* obj = make_test_object(600u);
    // Not added.
    ai.set_transition(obj, AiState::Rest);
    EXPECT_EQ(ai.last_transition_for(obj), AiState::Dead);
    delete obj;
}

// -----------------------------------------------------------------------------
// 7-state transition graph is pinned (the order must match legacy).
// -----------------------------------------------------------------------------
TEST(AISystemTest, AiSystemTransitionsArePinned) {
    // Pinned: the order Stand < WalkAround < Pursuit < Attack <
    //                  RunAway < Rest < Dead < Max is fixed.
    EXPECT_LT(static_cast<std::uint8_t>(AiState::Stand),
              static_cast<std::uint8_t>(AiState::WalkAround));
    EXPECT_LT(static_cast<std::uint8_t>(AiState::WalkAround),
              static_cast<std::uint8_t>(AiState::Pursuit));
    EXPECT_LT(static_cast<std::uint8_t>(AiState::Pursuit),
              static_cast<std::uint8_t>(AiState::Attack));
    EXPECT_LT(static_cast<std::uint8_t>(AiState::Attack),
              static_cast<std::uint8_t>(AiState::RunAway));
    EXPECT_LT(static_cast<std::uint8_t>(AiState::RunAway),
              static_cast<std::uint8_t>(AiState::Rest));
    EXPECT_LT(static_cast<std::uint8_t>(AiState::Rest),
              static_cast<std::uint8_t>(AiState::Dead));
    EXPECT_LT(static_cast<std::uint8_t>(AiState::Dead),
              static_cast<std::uint8_t>(AiState::Max));
    EXPECT_EQ(static_cast<std::uint8_t>(AiState::Max), 7u);
}

// -----------------------------------------------------------------------------
// Process() does not crash and tolerates empty + non-empty trackings.
// -----------------------------------------------------------------------------
TEST(AISystemTest, ProcessDoesNotCrashOnEmpty) {
    AISystem& ai = AISystem::instance();
    ai.remove_all_list();
    ai.process(0u);
    ai.process(12345u);
}

TEST(AISystemTest, ProcessDoesNotCrashOnTrackedObjects) {
    AISystem& ai = AISystem::instance();
    ai.remove_all_list();
    auto* obj = make_test_object(700u);
    ai.add_object(obj);
    ai.process(12345u);
    EXPECT_TRUE(ai.is_tracked(obj));
    delete obj;
}

// -----------------------------------------------------------------------------
// SendMsg is a no-op stub (router is owned by a later commit).
// -----------------------------------------------------------------------------
TEST(AISystemTest, SendMsgIsNoOpStub) {
    AISystem& ai = AISystem::instance();
    ai.remove_all_list();
    ai.send_msg(1u, 2u, 3u, 4u, 5u);
    EXPECT_EQ(ai.tracked_count(), 0u);
}

// -----------------------------------------------------------------------------
// load_ai_group_list / remove_all_list clean bookkeeping.
// -----------------------------------------------------------------------------
TEST(AISystemTest, RemoveAllListDropsTrackedObjects) {
    AISystem& ai = AISystem::instance();
    ai.remove_all_list();
    auto* a = make_test_object(801u);
    auto* b = make_test_object(802u);
    ai.add_object(a);
    ai.add_object(b);
    EXPECT_EQ(ai.tracked_count(), 2u);

    ai.remove_all_list();
    EXPECT_EQ(ai.tracked_count(), 0u);
    delete a;
    delete b;
}

TEST(AISystemTest, LoadAIGroupListClearsState) {
    AISystem& ai = AISystem::instance();
    ai.remove_all_list();
    auto* a = make_test_object(901u);
    ai.add_object(a);
    ai.generate_monster_id();

    ai.load_ai_group_list();
    EXPECT_EQ(ai.tracked_count(), 0u);
    delete a;
}
