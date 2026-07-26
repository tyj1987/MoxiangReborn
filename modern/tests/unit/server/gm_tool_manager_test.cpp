#include "mxh/server/gm_tool_manager.hpp"
#include <gtest/gtest.h>
using namespace mxh::server;
TEST(GmToolManager, LegacyImplementationIsNoOpBeforeInit){GmToolManagerState s;gm_tool_process_permit(s);EXPECT_FALSE(s.initialized);}
TEST(GmToolManager, InitAndReleaseAreIdempotent){GmToolManagerState s;gm_tool_init(s);gm_tool_init(s);EXPECT_TRUE(s.initialized);gm_tool_release(s);gm_tool_release(s);EXPECT_FALSE(s.initialized);}