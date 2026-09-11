#include "mxh/game/movement_timeline.hpp"
#include <gtest/gtest.h>
#include <limits>
using namespace mxh::game;

TEST(MovementTimeline, RunUsesServerTimeAndStrictOriginalArrivalBoundary) {
    MovementTimeline move;
    ASSERT_TRUE(move.reset({1000,1000},100));
    ASSERT_TRUE(move.start({1400,1000},400,100));
    EXPECT_FLOAT_EQ(move.advance(100).x,1000);
    EXPECT_FLOAT_EQ(move.advance(600).x,1200);
    EXPECT_FLOAT_EQ(move.advance(1100).x,1400);
    EXPECT_TRUE(move.moving()); // Original duration < elapsed, not <=.
    EXPECT_FLOAT_EQ(move.advance(1101).x,1400);
    EXPECT_FALSE(move.moving());
    EXPECT_FLOAT_EQ(move.advance(100000).x,1400);
}
TEST(MovementTimeline, DiagonalSpeedIsNormalizedAndWalkIsNotRun) {
    MovementTimeline move; ASSERT_TRUE(move.reset({1000,1000},0));
    ASSERT_TRUE(move.start({1300,1400},200,0));
    auto p=move.advance(1000);
    EXPECT_FLOAT_EQ(p.x,1120); EXPECT_FLOAT_EQ(p.z,1160);
}
TEST(MovementTimeline, RedirectStartsAtMaterializedPositionWithoutTeleport) {
    MovementTimeline move; ASSERT_TRUE(move.reset({1000,1000},0));
    ASSERT_TRUE(move.start({1800,1000},400,0));
    ASSERT_TRUE(move.start({1200,1400},400,500));
    EXPECT_FLOAT_EQ(move.position().x,1200); EXPECT_FLOAT_EQ(move.position().z,1000);
    EXPECT_FLOAT_EQ(move.advance(1000).z,1200);
    EXPECT_FLOAT_EQ(move.advance(900).z,1200); // Never rewind on stale clock input.
}
TEST(MovementTimeline, RepeatedSameTimestampCommandsCannotAccumulateDistance) {
    MovementTimeline move; ASSERT_TRUE(move.reset({1000,1000},0));
    for(int i=0;i<1000;++i) ASSERT_TRUE(move.start({5000,1000},400,0));
    EXPECT_FLOAT_EQ(move.position().x,1000);
    EXPECT_FLOAT_EQ(move.advance(1000).x,1400);
}
TEST(MovementTimeline, StopToleranceComparesAgainstCurrentInterpolatedPosition) {
    for(float offset : {999.0f,1000.0f,1000.25f}) {
        MovementTimeline move; ASSERT_TRUE(move.reset({1000,1000},0));
        ASSERT_TRUE(move.start({5000,1000},400,0));
        const auto result=move.stop({1400+offset,1000},1000);
        EXPECT_EQ(result,offset>1000 ? MovementStopResult::CorrectionRequired : MovementStopResult::Accepted);
        EXPECT_FALSE(move.moving());
        EXPECT_FLOAT_EQ(move.position().x,offset>1000 ? 1400 : 1400+offset);
    }
}
TEST(MovementTimeline, InvalidInputsDoNotReplaceLiveTrajectory) {
    MovementTimeline move;
    EXPECT_FALSE(move.start({1,1},400,0));
    ASSERT_TRUE(move.reset({1000,1000},0)); ASSERT_TRUE(move.start({2000,1000},400,0));
    const float nan=std::numeric_limits<float>::quiet_NaN();
    EXPECT_FALSE(move.start({nan,0},400,100));
    EXPECT_FALSE(move.start({1200,1000},nan,100));
    EXPECT_FALSE(move.start({1200,1000},-1,100));
    EXPECT_FALSE(move.reset({51200,0},100));
    EXPECT_EQ(move.stop({-1,1000},100),MovementStopResult::InvalidInput);
    EXPECT_FLOAT_EQ(move.advance(1000).x,1400);
}
TEST(MovementTimeline, ZeroSpeedHaltsAndLongClockDoesNotWrap) {
    MovementTimeline move;
    constexpr std::uint64_t origin=0xffffffffULL-500;
    ASSERT_TRUE(move.reset({1000,1000},origin));
    ASSERT_TRUE(move.start({5000,1000},400,origin));
    EXPECT_FLOAT_EQ(move.advance(origin+1000).x,1400);
    ASSERT_TRUE(move.start({5000,1000},0,origin+1500));
    EXPECT_FLOAT_EQ(move.position().x,1600); EXPECT_FALSE(move.moving());
    EXPECT_FLOAT_EQ(move.advance(origin+100000).x,1600);
}
TEST(MovementTimeline, LegacyWorldClampAndZeroDistanceRemainDefined) {
    MovementTimeline move; ASSERT_TRUE(move.reset({51000,51000},0));
    ASSERT_TRUE(move.start({51199,51199},400,0));
    auto p=move.advance(1000); EXPECT_FLOAT_EQ(p.x,51100); EXPECT_FLOAT_EQ(p.z,51100);
    ASSERT_TRUE(move.start(p,400,1000));
    EXPECT_TRUE(move.moving()); move.advance(1001); EXPECT_FALSE(move.moving());
}
TEST(MovementTimeline, FiniteInputsThatOverflowDerivedMotionAreRejected) {
    MovementTimeline move; ASSERT_TRUE(move.reset({1000,1000},0));
    ASSERT_TRUE(move.start({2000,1000},400,0));
    EXPECT_FALSE(move.start({2000,1000},std::numeric_limits<float>::denorm_min(),0));
    EXPECT_FLOAT_EQ(move.target().x,2000); EXPECT_FLOAT_EQ(move.speed(),400);
    EXPECT_FALSE(move.start({1000.001f,1000},std::numeric_limits<float>::max(),0));
    EXPECT_FALSE(move.start({2000,1000},std::numeric_limits<float>::denorm_min(),500));
    EXPECT_FLOAT_EQ(move.position().x,1200); // Failed retarget still materializes the old segment.
    EXPECT_FLOAT_EQ(move.advance(1000).x,1400);
}
TEST(MovementTimeline, ResetAndStaleStopUseExplicitClockAndClampContract) {
    MovementTimeline move; ASSERT_TRUE(move.reset({51199,51199},100));
    EXPECT_FLOAT_EQ(move.position().x,51199);
    EXPECT_FLOAT_EQ(move.advance(100).x,51100);
    ASSERT_TRUE(move.reset({1000,1000},200)); ASSERT_TRUE(move.start({2000,1000},400,200));
    EXPECT_FLOAT_EQ(move.advance(1200).x,1400);
    EXPECT_EQ(move.stop({1400,1000},1100),MovementStopResult::Accepted);
    EXPECT_FLOAT_EQ(move.advance(1200).x,1400); EXPECT_FALSE(move.moving());
    ASSERT_TRUE(move.start({1400.125f,1000},400,1200));
    EXPECT_FLOAT_EQ(move.advance(1201).x,1400.125f); EXPECT_FALSE(move.moving());
    ASSERT_TRUE(move.start({3000,1000},400,1201));
    EXPECT_FLOAT_EQ(move.advance(std::numeric_limits<std::uint64_t>::max()).x,3000);
    EXPECT_FALSE(move.moving());
}
