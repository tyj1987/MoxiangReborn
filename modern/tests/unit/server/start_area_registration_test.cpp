#include "mxh/server/start_area_route.hpp"

#include <gtest/gtest.h>

#include <string>

TEST(StartAreaRoute, StartupFailsWhenStartAreaMapIsUnregistered) {
    const std::uint16_t stage_maps[] = {10u, 12u, 2u};
    testing::internal::CaptureStderr();
    const int exit_code = mxh::server::agent_startup_exit_if_start_area_unregistered(
        mxh::server::kNewCharacterStartArea, true, 10u, stage_maps, 3);
    const auto log = testing::internal::GetCapturedStderr();
    EXPECT_EQ(exit_code, 1);
    EXPECT_NE(log.find("FATAL: new-character StartArea map 17 is not registered"),
              std::string::npos) << log;

    const std::uint16_t with_birth[] = {17u};
    EXPECT_FALSE(mxh::server::configured_start_area_registration_error(
        17u, true, 10u, with_birth, 1).has_value());
    EXPECT_FALSE(mxh::server::configured_start_area_registration_error(
        17u, true, 17u, nullptr, 0).has_value());
    const auto bare = mxh::server::configured_start_area_registration_error(
        17u, false, 17u, nullptr, 0);
    ASSERT_TRUE(bare.has_value());
    EXPECT_NE(bare->find("StartArea map 17"), std::string::npos);
}
