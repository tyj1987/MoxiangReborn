#include "CInGameState.hpp"
#include <gtest/gtest.h>
#include <algorithm>
#include <cstring>

namespace {
constexpr std::uint32_t target = 50030u;

mxh::net::Message monster_add() {
    mxh::net::Message msg;
    msg.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    msg.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::MonsterAdd);
    msg.header.object_id = target;
    msg.payload.assign(64, 0);
    const std::uint32_t hp = 53;
    std::memcpy(msg.payload.data(), &target, 4);
    std::memcpy(msg.payload.data() + 35, &hp, 4);
    return msg;
}

mxh::net::Message life(std::uint32_t hp) {
    mxh::net::Message msg;
    msg.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Monster);
    msg.header.protocol = static_cast<std::uint8_t>(mxh::proto::MonsterProtocol::LifeNotify);
    msg.header.object_id = target;
    msg.payload.assign(8, 0);
    std::memcpy(msg.payload.data(), &hp, 4);
    return msg;
}

mxh::net::Message damage(std::int32_t amount = 6) {
    mxh::net::Message msg;
    msg.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Skill);
    msg.header.protocol = static_cast<std::uint8_t>(mxh::proto::SkillProtocol::SingleResult);
    msg.header.object_id = 42;
    msg.payload.assign(9, 0);
    std::memcpy(msg.payload.data(), &target, 4);
    std::memcpy(msg.payload.data() + 4, &amount, 4);
    msg.payload[8] = 1;
    return msg;
}

class MonsterLifeAuthority : public testing::Test {
protected:
    mxh::client::CInGameState state;
    void SetUp() override {
        state.Init(nullptr);
        state.on_message({}, monster_add());
    }
    void deliver(const mxh::net::Message& msg) { state.on_message({}, msg); }
    void expect_life(std::uint32_t hp) {
        ASSERT_EQ(state.monsters().size(), 1u);
        EXPECT_EQ(state.monsters().front().current_life, hp);
        const auto selected = mxh::client::pick_attack_target(state.monsters(), 0, 0, 100);
        if (hp) EXPECT_EQ(selected, target);
        else EXPECT_FALSE(selected.has_value());
    }
};
} // namespace

TEST_F(MonsterLifeAuthority, ServerLifeThenDamageKeepsLivingTargetAndFeedback) {
    // Actual MapHandler order: apply_monster_damage broadcasts LifeNotify,
    // then its caller sends SingleResult. Five remaining HP is not a kill.
    deliver(life(5));
    deliver(damage());
    expect_life(5);
    EXPECT_EQ(state.last_damage(), 6);
    EXPECT_EQ(state.last_hit_target(), target);
    EXPECT_EQ(state.last_hit_result(), 1);
    const auto events = state.drain_effect_events();
    EXPECT_EQ(std::count_if(events.begin(), events.end(), [](const auto& e) {
        return e.kind == mxh::client::EffectEventKind::Hit &&
               e.target_object_id == target && e.damage == 6 && e.hit_result == 1;
    }), 1);
    EXPECT_EQ(std::count_if(events.begin(), events.end(), [](const auto& e) {
        return e.kind == mxh::client::EffectEventKind::End && e.target_object_id == target;
    }), 1);
    EXPECT_EQ(std::count_if(events.begin(), events.end(), [](const auto& e) {
        return e.kind == mxh::client::EffectEventKind::Death;
    }), 0);
    EXPECT_TRUE(state.ground_drops().empty());
}

TEST_F(MonsterLifeAuthority, DuplicateAndOldDamageCannotReduceAuthoritativeLife) {
    deliver(life(5));
    deliver(damage());
    deliver(damage());
    deliver(life(5));
    deliver(damage(999));
    expect_life(5);
    EXPECT_EQ(state.last_damage(), 999);
    const auto events = state.drain_effect_events();
    // There is no result sequence ID: keep each damage feedback event, but
    // never replay it as a mutation of the authoritative HP snapshot.
    EXPECT_EQ(std::count_if(events.begin(), events.end(), [](const auto& e) {
        return e.kind == mxh::client::EffectEventKind::Hit;
    }), 3);
}

TEST_F(MonsterLifeAuthority, ResultBeforeLifeWaitsForAuthority) {
    deliver(damage(999));
    expect_life(53);
    deliver(life(5));
    expect_life(5);
}

TEST_F(MonsterLifeAuthority, ActualDeathRemainsDeadWithDuplicateAndLateResults) {
    deliver(life(0));
    deliver(damage());
    deliver(life(0));
    deliver(damage(-20));
    expect_life(0);
    const auto events = state.drain_effect_events();
    EXPECT_EQ(std::count_if(events.begin(), events.end(), [](const auto& e) {
        return e.kind == mxh::client::EffectEventKind::Death && e.target_object_id == target;
    }), 1);
    EXPECT_EQ(std::count_if(events.begin(), events.end(), [](const auto& e) {
        return e.kind == mxh::client::EffectEventKind::Hit;
    }), 2);
}

TEST_F(MonsterLifeAuthority, RemovalAndLateMessagesDoNotRecreateMonsterOrLoot) {
    deliver(life(0));
    mxh::net::Message remove;
    remove.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    remove.header.protocol = static_cast<std::uint8_t>(mxh::proto::UserConnProtocol::ObjectRemove);
    remove.payload.resize(4);
    std::memcpy(remove.payload.data(), &target, 4);
    deliver(remove);
    deliver(remove);
    deliver(damage());
    deliver(life(5));
    EXPECT_TRUE(state.monsters().empty());
    EXPECT_TRUE(state.ground_drops().empty());
    // A genuine server MonsterAdd may respawn the same object ID. A late
    // damage result must not hide that newly authoritative live instance.
    deliver(monster_add());
    deliver(damage(999));
    expect_life(53);
}
