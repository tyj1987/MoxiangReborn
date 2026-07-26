#include "mxh/server/plustime_manager.hpp"
#include <gtest/gtest.h>
using namespace mxh::server;
static PlusTimeInfo e(std::uint16_t i,std::uint16_t si=1,std::uint16_t ei=2){PlusTimeInfo x;x.index=i;x.event_index=i+10;x.start_day=si;x.end_day=ei;x.start_hour=10;x.end_hour=12;x.context="event"+std::to_string(i);x.title="title";x.rate=150;return x;}
TEST(PlusTime, AppliesOnlyInsideWindow){PlusTimeState s;s.entries={e(1)};EXPECT_TRUE(plustime_process(s,1,9,0).empty());auto a=plustime_process(s,1,10,0);EXPECT_EQ(s.applied.size(),1u);EXPECT_EQ(a[1].kind,PlusTimeActionKind::on);}
TEST(PlusTime, DoesNotApplyAfterEnd){PlusTimeState s;s.entries={e(1)};EXPECT_TRUE(plustime_process(s,2,12,0).empty());}
TEST(PlusTime, ExpiresAtEndMinute){PlusTimeState s;s.entries={e(1)};plustime_process(s,1,10,0);auto a=plustime_process(s,2,12,0);EXPECT_TRUE(s.applied.empty());EXPECT_EQ(a[0].kind,PlusTimeActionKind::off);}
TEST(PlusTime, ContextsAreCommaJoined){PlusTimeState s;s.entries={e(1),e(2)};s.entries[1].start_day=1;s.entries[1].end_day=2;plustime_process(s,1,10,0);plustime_process(s,1,10,1);EXPECT_EQ(s.context,"event1, event2");}
TEST(PlusTime, OffDisablesAndClears){PlusTimeState s;s.entries={e(1)};plustime_process(s,1,10,0);auto a=plustime_off(s);EXPECT_FALSE(s.toggle_on);EXPECT_TRUE(s.applied.empty());EXPECT_EQ(a.size(),2u);}
TEST(PlusTime, ConnectingReplaysAppliedOn){PlusTimeState s;s.entries={e(1)};plustime_process(s,1,10,0);auto a=plustime_connecting(s);ASSERT_EQ(a.size(),1u);EXPECT_EQ(a[0].event_index,11);EXPECT_EQ(a[0].rate,150);}
TEST(PlusTime, ResetClearsTables){PlusTimeState s;s.entries={e(1)};s.applied={1};s.context="x";plustime_reset(s);EXPECT_TRUE(s.entries.empty());EXPECT_TRUE(s.applied.empty());EXPECT_TRUE(s.context.empty());}