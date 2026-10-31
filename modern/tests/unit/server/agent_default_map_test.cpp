#include "agent_cli.hpp"

#include <gtest/gtest.h>

#include <string>

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

TEST(AgentCli, ExtraMapEqualsExplicitDefaultRejected) {
    char program[] = "mxh_agent_server";
    char default_flag[] = "--default-map";
    char default_value[] = "17";
    char route_flag[] = "--map-server-map";
    char route_value[] = "17=127.0.0.1:28003";
    char* argv[] = {program, default_flag, default_value, route_flag, route_value};
    const auto args = mxh::agent_cli::parse_args(5, argv);
    const auto error = mxh::agent_cli::extra_map_equals_default_error(args);
    EXPECT_FALSE(error.empty());
    EXPECT_NE(error.find("17"), std::string::npos);
    EXPECT_EQ(args.default_map_num, 17);
}

TEST(AgentCli, ExtraMapEqualsOmittedDefaultRejected) {
    char program[] = "mxh_agent_server";
    char route_flag[] = "--map-server-map";
    char route_value[] = "17=127.0.0.1:28003";
    char* argv[] = {program, route_flag, route_value};
    const auto args = mxh::agent_cli::parse_args(3, argv);
    EXPECT_EQ(args.default_map_num, 17);
    const auto error = mxh::agent_cli::extra_map_equals_default_error(args);
    EXPECT_FALSE(error.empty());
    EXPECT_NE(error.find("equals default map 17"), std::string::npos);
}

TEST(AgentCli, MapServerMapBeforeDefaultMapRejected) {
    char program[] = "mxh_agent_server";
    char route_flag[] = "--map-server-map";
    char route_value[] = "12=127.0.0.1:28002";
    char default_flag[] = "--default-map";
    char default_value[] = "12";
    char* argv[] = {program, route_flag, route_value, default_flag, default_value};
    const auto args = mxh::agent_cli::parse_args(5, argv);
    EXPECT_EQ(args.default_map_num, 12);
    ASSERT_EQ(args.map_routes.size(), 1u);
    EXPECT_EQ(args.map_routes[0].map_num, 12);
    EXPECT_FALSE(mxh::agent_cli::extra_map_equals_default_error(args).empty());
}

TEST(AgentCli, ExtraMapDifferentFromDefaultAccepted) {
    char program[] = "mxh_agent_server";
    char default_flag[] = "--default-map";
    char default_value[] = "12";
    char route_flag[] = "--map-server-map";
    char route_value[] = "17=127.0.0.1:28003";
    char* argv[] = {program, default_flag, default_value, route_flag, route_value};
    const auto args = mxh::agent_cli::parse_args(5, argv);
    EXPECT_EQ(args.default_map_num, 12);
    ASSERT_EQ(args.map_routes.size(), 1u);
    EXPECT_EQ(args.map_routes[0].map_num, 17);
    EXPECT_TRUE(mxh::agent_cli::extra_map_equals_default_error(args).empty());
}
