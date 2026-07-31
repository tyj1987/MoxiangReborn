#include "mxh/server/mh_time_manager.hpp"
#include <gtest/gtest.h>
using namespace mxh::server;
TEST(MhTimeManager, NumericConstantsMatchLegacy){EXPECT_EQ(MXH_MH_TICK_PER_DAY,8640000u);EXPECT_EQ(MXH_MH_TICK_PER_HOUR,3600000u);EXPECT_EQ(MXH_MH_TICK_PER_MINUTE,60000u);EXPECT_EQ(MXH_MH_DAY_PER_YEAR,360u);EXPECT_EQ(MXH_MH_DAY_PER_MONTH,30u);}
TEST(MhTimeManager, InitSetsInitialDateAndTime){MhTimeManager t;t.init(7,12345);EXPECT_EQ(t.mh_date(),7u);EXPECT_EQ(t.mh_time(),12345u);}
TEST(MhTimeManager, FirstProcessEstablishesBaselineWithoutAdvancing){MhTimeManager t;t.init(0,1000);t.process(5000);EXPECT_EQ(t.mh_date(),0u);EXPECT_EQ(t.mh_time(),1000u);}
TEST(MhTimeManager, ProcessAdvancesMhTimeByDelta){MhTimeManager t;t.init(0,0);t.process(1000);t.process(2500);EXPECT_EQ(t.mh_time(),1500u);EXPECT_EQ(t.mh_date(),0u);}
TEST(MhTimeManager, ProcessWrapsDayWhenCrossingMidnight){MhTimeManager t;t.init(0,MXH_MH_TICK_PER_DAY-500);t.process(1000);t.process(2000);EXPECT_EQ(t.mh_date(),1u);EXPECT_EQ(t.mh_time(),500u);}
TEST(MhTimeManager, ProcessWrapsMultipleDays){MhTimeManager t;t.init(0,MXH_MH_TICK_PER_DAY*2-100);t.process(1000);t.process(2000);EXPECT_EQ(t.mh_date(),2u);EXPECT_EQ(t.mh_time(),900u);}
TEST(MhTimeManager, ProcessHandlesClockWrap){MhTimeManager t;t.init(0,0);t.process(0xFFFFFFF0u);t.process(0x00000010u);EXPECT_EQ(t.mh_time(),32u);}
TEST(MhTimeManager, SplitDateMatchesYearMonthDay){MhTimeManager t;t.init(0,0);std::uint8_t y=0,m=0,d=0;t.mh_date(y,m,d);EXPECT_EQ(y,1);EXPECT_EQ(m,1);EXPECT_EQ(d,1);t.init(29,0);t.mh_date(y,m,d);EXPECT_EQ(y,1);EXPECT_EQ(m,1);EXPECT_EQ(d,30);t.init(30,0);t.mh_date(y,m,d);EXPECT_EQ(y,1);EXPECT_EQ(m,2);EXPECT_EQ(d,1);t.init(360,0);t.mh_date(y,m,d);EXPECT_EQ(y,2);EXPECT_EQ(m,1);EXPECT_EQ(d,1);}
TEST(MhTimeManager, SplitTimeMatchesHourMinute){MhTimeManager t;t.init(0,0);std::uint8_t h=99,mi=99;t.mh_time(h,mi);EXPECT_EQ(h,0);EXPECT_EQ(mi,0);t.init(0,3600000);t.mh_time(h,mi);EXPECT_EQ(h,1);EXPECT_EQ(mi,0);t.init(0,3660000);t.mh_time(h,mi);EXPECT_EQ(h,1);EXPECT_EQ(mi,1);t.init(0,MXH_MH_TICK_PER_DAY-1);t.mh_time(h,mi);EXPECT_EQ(h,2);EXPECT_EQ(mi,23);}
TEST(MhTimeManager, ResetForTestClearsState){MhTimeManager t;t.init(100,2000);t.process(5000);t.reset_for_test();EXPECT_EQ(t.mh_date(),0u);EXPECT_EQ(t.mh_time(),0u);std::uint8_t h=99,mi=99;t.mh_time(h,mi);EXPECT_EQ(h,0);EXPECT_EQ(mi,0);}
