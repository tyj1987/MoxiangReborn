// object_event_test.cpp - Phase D6 ObjectEvent 1:1 port tests.

#include "mxh/server/object_event.hpp"

#include <gtest/gtest.h>

namespace {

using mxh::server::object_event_dispatch;
using mxh::server::ObjectEventCode;
using mxh::server::ObjectEventSink;
using mxh::server::get_object_event_sink;
using mxh::server::set_object_event_sink;

struct Obj {};

bool record_levelup(mxh::server::Object*, void*) { return true; }
bool record_die(mxh::server::Object*, void*) { return false; }
bool record_life(mxh::server::Object*, void*) { return true; }

class ObjectEventFixture : public ::testing::Test {
protected:
    void SetUp() override { set_object_event_sink(ObjectEventSink{}); }
    void TearDown() override { set_object_event_sink(ObjectEventSink{}); }
};

TEST_F(ObjectEventFixture, NoSinkReturnsFalse) {
    Obj o;
    EXPECT_FALSE(object_event_dispatch(ObjectEventCode::LevelUp, nullptr));
    EXPECT_FALSE(object_event_dispatch(ObjectEventCode::Die, nullptr));
}

TEST_F(ObjectEventFixture, LevelUpCallsHandler) {
    Obj o;
    set_object_event_sink(ObjectEventSink{&record_levelup, nullptr, nullptr, nullptr});
    EXPECT_TRUE(object_event_dispatch(ObjectEventCode::LevelUp, nullptr));
}

TEST_F(ObjectEventFixture, DieCallsHandler) {
    Obj o;
    set_object_event_sink(ObjectEventSink{nullptr, &record_die, nullptr, nullptr});
    EXPECT_FALSE(object_event_dispatch(ObjectEventCode::Die, nullptr));
}

TEST_F(ObjectEventFixture, LifeRecoverCompletedCallsHandler) {
    Obj o;
    set_object_event_sink(ObjectEventSink{nullptr, nullptr, &record_life, nullptr});
    EXPECT_TRUE(object_event_dispatch(ObjectEventCode::LifeRecoverCompleted, nullptr));
}

TEST_F(ObjectEventFixture, SinkRoundTrips) {
    ObjectEventSink s{&record_levelup, &record_die, &record_life, reinterpret_cast<void*>(0x1234)};
    set_object_event_sink(s);
    auto got = get_object_event_sink();
    EXPECT_EQ(got.on_levelup, &record_levelup);
    EXPECT_EQ(got.on_die, &record_die);
    EXPECT_EQ(got.on_life_recover_completed, &record_life);
    EXPECT_EQ(got.user_data, reinterpret_cast<void*>(0x1234));
}

TEST(ObjectEventEnum, ValuesMatchLegacy) {
    EXPECT_EQ(static_cast<std::uint8_t>(ObjectEventCode::LevelUp), 0u);
    EXPECT_EQ(static_cast<std::uint8_t>(ObjectEventCode::Die), 1u);
    EXPECT_EQ(static_cast<std::uint8_t>(ObjectEventCode::LifeRecoverCompleted), 2u);
}

}  // namespace
