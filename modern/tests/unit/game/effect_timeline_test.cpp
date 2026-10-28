#include "mxh/game/effect_timeline.hpp"

#include <gtest/gtest.h>

using mxh::game::EffectScriptSummary;
using mxh::game::EffectTimeline;

namespace {
EffectScriptSummary script() {
    EffectScriptSummary s;
    s.decoded = true;
    s.effect_unit_count = 1;
    s.trigger_count = 3;
    s.units.push_back({0, "LIGHT"});
    s.triggers.push_back({"f0", 0, "ON"});
    s.triggers.push_back({"f2", 0, "OFF"});
    s.triggers.push_back({"120", 0, "LINK"});
    return s;
}
}

TEST(EffectTimeline, ConvertsFramesOnlyWithExplicitTick) {
    EffectTimeline timeline;
    ASSERT_TRUE(timeline.start(script(), 1000, 16));
    EXPECT_TRUE(timeline.advance(999).empty());
    auto first = timeline.advance(1000);
    ASSERT_EQ(first.size(), 1u);
    EXPECT_EQ(first[0].trigger.kind, "ON");
    auto second = timeline.advance(1032);
    ASSERT_EQ(second.size(), 1u);
    EXPECT_EQ(second[0].trigger.kind, "OFF");
    auto third = timeline.advance(1120);
    ASSERT_EQ(third.size(), 1u);
    EXPECT_EQ(third[0].trigger.kind, "LINK");
    EXPECT_FALSE(timeline.active());
}

TEST(EffectTimeline, EmitsDueTriggersOnceAndClampsBackwardClock) {
    EffectTimeline timeline;
    ASSERT_TRUE(timeline.start(script(), 500, 10));
    auto all = timeline.advance(700);
    ASSERT_EQ(all.size(), 3u);
    EXPECT_TRUE(timeline.advance(100).empty());
}

TEST(EffectTimeline, RejectsUnknownTimeTokenOrMissingTick) {
    auto bad = script();
    bad.triggers[1].time_token = "frame?";
    EffectTimeline timeline;
    EXPECT_FALSE(timeline.start(bad, 0, 16));
    EXPECT_FALSE(timeline.start(script(), 0, 0));
}
