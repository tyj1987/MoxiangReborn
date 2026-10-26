#include "mxh/render/EntityFrameStats.hpp"

#include <gtest/gtest.h>

using namespace mxh::gx;

TEST(EntityFrameStats, CountsPerKindAndSum) {
    EntityFrameStats s;
    s.drawn(EntityFrameKind::Monster);
    s.drawn(EntityFrameKind::Monster);
    s.culled(EntityFrameKind::Monster);
    s.skipped(EntityFrameKind::Npc);
    s.drawn(EntityFrameKind::Player);
    const auto m = s.at(EntityFrameKind::Monster);
    EXPECT_EQ(m.total, 3u); EXPECT_EQ(m.drawn, 2u); EXPECT_EQ(m.culled, 1u);
    const auto t = s.sum();
    EXPECT_EQ(t.total, 5u); EXPECT_EQ(t.drawn, 3u);
    EXPECT_EQ(t.culled, 1u); EXPECT_EQ(t.skipped, 1u);
}

TEST(EntityFrameStats, PlaceholderUnskipConvertsToDrawn) {
    EntityFrameStats s;
    s.skipped(EntityFrameKind::Npc);
    s.unskip(EntityFrameKind::Npc);
    s.drawn(EntityFrameKind::Npc);
    EXPECT_EQ(s.at(EntityFrameKind::Npc).skipped, 0u);
    EXPECT_EQ(s.at(EntityFrameKind::Npc).drawn, 1u);
    EXPECT_EQ(s.at(EntityFrameKind::Npc).total, 1u);
    s.unskip(EntityFrameKind::Npc);  // no-op when nothing skipped
    EXPECT_EQ(s.at(EntityFrameKind::Npc).total, 1u);
}

TEST(EntityFrameStats, FormatLine) {
    EntityFrameStats s;
    s.drawn(EntityFrameKind::Monster);
    s.culled(EntityFrameKind::Npc);
    s.skipped(EntityFrameKind::Player);
    EXPECT_EQ(formatEntityFrameStats(s),
              "[entity] frame drawn=1/3 culled=1 skipped=1 monster=1/1/0/0 "
              "npc=0/1/1/0 player=0/1/0/1 effect=0/0/0/0");
}

TEST(EntityFrameStats, ThrottleOnChangeAndHeartbeat) {
    EntityFrameStatsThrottle th(1000, 10000);
    EntityFrameStats s;
    EXPECT_TRUE(th.shouldLog(s, 0));      // first frame
    EXPECT_FALSE(th.shouldLog(s, 500));   // unchanged
    s.drawn(EntityFrameKind::Monster);
    EXPECT_FALSE(th.shouldLog(s, 900));   // changed but within min interval
    EXPECT_TRUE(th.shouldLog(s, 1000));   // changed, interval elapsed
    EXPECT_FALSE(th.shouldLog(s, 5000));  // stable
    EXPECT_TRUE(th.shouldLog(s, 11000));  // heartbeat
}

TEST(EntityFrameStats, SmokeExitEnvFailsOnThrow) {
    EXPECT_TRUE(entitySceneThrowFailsSmoke("1"));
    EXPECT_FALSE(entitySceneThrowFailsSmoke(nullptr));
    EXPECT_FALSE(entitySceneThrowFailsSmoke("0"));
    EXPECT_FALSE(entitySceneThrowFailsSmoke(""));
}
