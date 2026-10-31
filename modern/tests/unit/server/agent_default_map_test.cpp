#include "agent_cli.hpp"

#include <gtest/gtest.h>

// No --default-map argument: the effective map is 17, and omitted
// --start-area copies that same value.
TEST(AgentCli, OmittedDefaultMapIs17) {
    char program[] = "mxh_agent_server";
    char* bare[] = {program};
    const auto omitted = mxh::agent_cli::parse_args(1, bare);
    EXPECT_EQ(mxh::agent_cli::kOmittedDefaultMapNum, 17);
    EXPECT_EQ(omitted.default_map_num, 17);
    EXPECT_EQ(omitted.start_area, 17);
    EXPECT_FALSE(omitted.start_area_explicit);

    char port_flag[] = "--port";
    char port_value[] = "7001";
    char legacy_flag[] = "--legacy";
    char* with_other_flags[] = {program, port_flag, port_value, legacy_flag};
    const auto still_omitted = mxh::agent_cli::parse_args(4, with_other_flags);
    EXPECT_EQ(still_omitted.port, 7001);
    EXPECT_TRUE(still_omitted.use_legacy);
    EXPECT_EQ(still_omitted.default_map_num, 17);
    EXPECT_EQ(still_omitted.start_area, 17);
}
