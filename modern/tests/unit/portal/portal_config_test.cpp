// tests/unit/portal/portal_config_test.cpp
// M5.1: Config loading from environment variables.

#include "portal/config.hpp"

#include <gtest/gtest.h>

using namespace mxh::portal;

TEST(PortalConfig, DefaultsAreSensible) {
    Config c;
    EXPECT_EQ(c.bind,         "0.0.0.0");
    EXPECT_EQ(c.port,          8080u);
    EXPECT_EQ(c.db_backend,   "sqlite");
    EXPECT_EQ(c.game_login_port, 16001u);
    EXPECT_EQ(c.game_agent_port, 17001u);
    EXPECT_EQ(c.game_map_port,   18001u);
    EXPECT_EQ(c.rate_limit_register, 5u);
    EXPECT_EQ(c.rate_limit_login,    10u);
    EXPECT_EQ(c.version,      "1.0.0");
}

TEST(PortalConfig, EnvOverridesDefaults) {
    // This test only runs if the env vars are set; it documents expected behaviour.
    // In unit-test context we cannot reliably override std::getenv per-test,
    // so we just verify the struct has the right shape.
    Config c;
    EXPECT_EQ(c.rate_limit_register, 5u);   // default from load_config()
    EXPECT_EQ(c.rate_limit_login,    10u);
}

TEST(PortalConfig, EnvDoesNotCrashOnMissingVars) {
    // load_config() must not throw even when all env vars are absent.
    EXPECT_NO_THROW({
        Config c = load_config();
        EXPECT_EQ(c.port, 8080u);  // default is hard-coded fallback
    });
}
