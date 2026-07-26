#include "mxh/server/mh_time_manager.hpp"
#include <gtest/gtest.h>
using namespace mxh::server;
TEST(MhTime, FirstProcessOnlyPrimesClock){MhTimeState s;mh_time_init(s,4,5);EXPECT_EQ(mh_time_process(s,100),0u);EXPECT_EQ(s.mh_date,4u);EXPECT_EQ(s.mh_time,5u);}
TEST(MhTime, AdvancesCurAndGameTime){MhTimeState s;mh_time_init(s,4,5);mh_time_process(s,100);EXPECT_EQ(mh_time_process(s,250),150u);EXPECT_EQ(s.cur_time,150u);EXPECT_EQ(s.mh_time,155u);}
TEST(MhTime, WrapsDay){MhTimeState s;mh_time_init(s,4,tick_per_day-10);mh_time_process(s,1);mh_time_process(s,21);EXPECT_EQ(s.mh_date,5u);EXPECT_EQ(s.mh_time,10u);}
TEST(MhTime, UsesUnsignedTickWrap){MhTimeState s;mh_time_init(s,0,0);mh_time_process(s,0xffffff00u);EXPECT_EQ(mh_time_process(s,0x00000010u),0x110u);}
TEST(MhTime, NewCalcDoesNotMutate){MhTimeState s;mh_time_init(s,0,0);mh_time_process(s,100);EXPECT_EQ(mh_new_calc_cur_time(s,140),40u);EXPECT_EQ(s.cur_time,0u);}
TEST(MhTime, LegacyDateParts){MhTimeState s;mh_time_init(s,360u+30u+29u,0);std::uint8_t y,m,d;mh_date_parts(s,y,m,d);EXPECT_EQ(y,2);EXPECT_EQ(m,14);EXPECT_EQ(d,30);}
TEST(MhTime, LegacyTimeParts){MhTimeState s;mh_time_init(s,0,2*tick_per_hour+17*tick_per_minute);std::uint8_t h,m;mh_time_parts(s,h,m);EXPECT_EQ(h,2);EXPECT_EQ(m,136);}