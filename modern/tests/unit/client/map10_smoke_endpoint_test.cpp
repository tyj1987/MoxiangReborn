#include "map10_smoke_endpoint.hpp"

#include <gtest/gtest.h>

#include <map>
#include <string>

namespace {

struct FakeEnv {
    std::map<std::string, const char*> values;
    const char* operator()(const char* name) const {
        const auto it = values.find(name ? name : "");
        if (it == values.end()) return nullptr;
        return it->second;
    }
};

FakeEnv empty_env() { return {}; }

}  // namespace

TEST(Map10SmokeEndpoint, SkipsWhenHostAndPortsAreUnset) {
    const auto endpoint = mxh::test::resolve_map10_smoke_endpoint(empty_env());
    EXPECT_FALSE(endpoint.ready);
    EXPECT_TRUE(endpoint.host.empty());
    EXPECT_EQ(endpoint.login_port, 0);
    EXPECT_EQ(endpoint.agent_port, 0);
    EXPECT_EQ(endpoint.map_port, 0);
    EXPECT_TRUE(endpoint.e2e_arguments().empty());
    EXPECT_TRUE(endpoint.log_line().empty());
    EXPECT_NE(endpoint.skip_reason.find("writes the game database"), std::string::npos);
    EXPECT_NE(endpoint.skip_reason.find("skipped"), std::string::npos);
    EXPECT_NE(endpoint.skip_reason.find("16001/17001/18001"), std::string::npos);
    for (const char* name : {mxh::test::kMap10SmokeHostEnv,
                             mxh::test::kMap10SmokeLoginPortEnv,
                             mxh::test::kMap10SmokeAgentPortEnv,
                             mxh::test::kMap10SmokeMapPortEnv}) {
        EXPECT_NE(endpoint.skip_reason.find(name), std::string::npos) << name;
        EXPECT_NE(endpoint.skip_reason.find(std::string(name) + " is unset"),
                  std::string::npos) << name;
    }
}

TEST(Map10SmokeEndpoint, SkipsWhenAnyPortIsMissing) {
    FakeEnv env;
    env.values[mxh::test::kMap10SmokeHostEnv] = "10.1.2.3";
    env.values[mxh::test::kMap10SmokeLoginPortEnv] = "21001";
    env.values[mxh::test::kMap10SmokeAgentPortEnv] = "21002";
    const auto endpoint = mxh::test::resolve_map10_smoke_endpoint(env);
    EXPECT_FALSE(endpoint.ready);
    EXPECT_TRUE(endpoint.host.empty());
    EXPECT_EQ(endpoint.login_port, 0);
    EXPECT_EQ(endpoint.map_port, 0);
    EXPECT_TRUE(endpoint.e2e_arguments().empty());
    EXPECT_NE(endpoint.skip_reason.find("MXH_MAP10_SMOKE_MAP_PORT is unset"),
              std::string::npos);
    EXPECT_EQ(endpoint.skip_reason.find("MXH_MAP10_SMOKE_LOGIN_PORT is unset"),
              std::string::npos);
}

TEST(Map10SmokeEndpoint, SkipsWhenAValueIsBlankOrInvalid) {
    FakeEnv env;
    env.values[mxh::test::kMap10SmokeHostEnv] = "   ";
    env.values[mxh::test::kMap10SmokeLoginPortEnv] = "0";
    env.values[mxh::test::kMap10SmokeAgentPortEnv] = "65536";
    env.values[mxh::test::kMap10SmokeMapPortEnv] = "18001abc";
    const auto endpoint = mxh::test::resolve_map10_smoke_endpoint(env);
    EXPECT_FALSE(endpoint.ready);
    EXPECT_EQ(endpoint.login_port, 0);
    EXPECT_EQ(endpoint.agent_port, 0);
    EXPECT_EQ(endpoint.map_port, 0);
    EXPECT_NE(endpoint.skip_reason.find("MXH_MAP10_SMOKE_HOST is unset"),
              std::string::npos);
    EXPECT_NE(endpoint.skip_reason.find("MXH_MAP10_SMOKE_LOGIN_PORT is not a TCP port"),
              std::string::npos);
    EXPECT_NE(endpoint.skip_reason.find("MXH_MAP10_SMOKE_AGENT_PORT is not a TCP port"),
              std::string::npos);
    EXPECT_NE(endpoint.skip_reason.find("MXH_MAP10_SMOKE_MAP_PORT is not a TCP port"),
              std::string::npos);
}

TEST(Map10SmokeEndpoint, SkipsWhenPortsCollide) {
    FakeEnv env;
    env.values[mxh::test::kMap10SmokeHostEnv] = "vm112.example";
    env.values[mxh::test::kMap10SmokeLoginPortEnv] = "21001";
    env.values[mxh::test::kMap10SmokeAgentPortEnv] = "21001";
    env.values[mxh::test::kMap10SmokeMapPortEnv] = "21003";
    const auto endpoint = mxh::test::resolve_map10_smoke_endpoint(env);
    EXPECT_FALSE(endpoint.ready);
    EXPECT_EQ(endpoint.login_port, 0);
    EXPECT_NE(endpoint.skip_reason.find("must be distinct"), std::string::npos);
}

TEST(Map10SmokeEndpoint, ExplicitEndpointIsLoggedAndPassedThrough) {
    FakeEnv env;
    env.values[mxh::test::kMap10SmokeHostEnv] = "10.1.2.3";
    env.values[mxh::test::kMap10SmokeLoginPortEnv] = "21001";
    env.values[mxh::test::kMap10SmokeAgentPortEnv] = "21002";
    env.values[mxh::test::kMap10SmokeMapPortEnv] = "21003";
    const auto endpoint = mxh::test::resolve_map10_smoke_endpoint(env);
    ASSERT_TRUE(endpoint.ready);
    EXPECT_TRUE(endpoint.skip_reason.empty());
    EXPECT_EQ(endpoint.host, "10.1.2.3");
    EXPECT_EQ(endpoint.login_port, 21001);
    EXPECT_EQ(endpoint.agent_port, 21002);
    EXPECT_EQ(endpoint.map_port, 21003);
    EXPECT_EQ(endpoint.log_line(),
              "Map10InGameSmoke resolved endpoint host=10.1.2.3 "
              "login_port=21001 agent_port=21002 map_port=21003");
    const std::string args = endpoint.e2e_arguments();
    EXPECT_NE(args.find("--login-host 10.1.2.3"), std::string::npos);
    EXPECT_NE(args.find("--agent-host 10.1.2.3"), std::string::npos);
    EXPECT_NE(args.find("--login-port 21001"), std::string::npos);
    EXPECT_NE(args.find("--agent-port 21002"), std::string::npos);
    EXPECT_NE(args.find("--map-port 21003"), std::string::npos);
    EXPECT_NE(args.find("--no-spawn"), std::string::npos);
    EXPECT_EQ(args.find("16001"), std::string::npos);
    EXPECT_EQ(args.find("17001"), std::string::npos);
    EXPECT_EQ(args.find("18001"), std::string::npos);
    EXPECT_EQ(args.find("192.168.2.244"), std::string::npos);
}

FakeEnv stage_env(const char* login, const char* agent, const char* map) {
    FakeEnv env;
    env.values[mxh::test::kMap10SmokeHostEnv] = "192.168.2.244";
    env.values[mxh::test::kMap10SmokeLoginPortEnv] = login;
    env.values[mxh::test::kMap10SmokeAgentPortEnv] = agent;
    env.values[mxh::test::kMap10SmokeMapPortEnv] = map;
    return env;
}

TEST(Map10SmokeEndpoint, DefaultStagePortsSkipWithoutAllow) {
    const auto endpoint = mxh::test::resolve_map10_smoke_endpoint(
        stage_env("16001", "17001", "18001"));
    EXPECT_FALSE(endpoint.ready);
    EXPECT_TRUE(endpoint.host.empty());
    EXPECT_EQ(endpoint.login_port, 0);
    EXPECT_EQ(endpoint.agent_port, 0);
    EXPECT_EQ(endpoint.map_port, 0);
    EXPECT_TRUE(endpoint.e2e_arguments().empty());
    EXPECT_TRUE(endpoint.log_line().empty());
    EXPECT_NE(endpoint.skip_reason.find("default stage"), std::string::npos);
    EXPECT_NE(endpoint.skip_reason.find("MXH_MAP10_SMOKE_ALLOW_DEFAULT_STAGE=1"),
              std::string::npos);
    EXPECT_NE(endpoint.skip_reason.find("MXH_MAP10_SMOKE_LOGIN_PORT=16001"),
              std::string::npos);
    EXPECT_NE(endpoint.skip_reason.find("MXH_MAP10_SMOKE_AGENT_PORT=17001"),
              std::string::npos);
    EXPECT_NE(endpoint.skip_reason.find("MXH_MAP10_SMOKE_MAP_PORT=18001"),
              std::string::npos);

    FakeEnv not_one = stage_env("16001", "17001", "18001");
    not_one.values[mxh::test::kMap10SmokeAllowDefaultStageEnv] = "true";
    const auto still_skipped = mxh::test::resolve_map10_smoke_endpoint(not_one);
    EXPECT_FALSE(still_skipped.ready);

    for (const char* map_port : {"18002", "18003"}) {
        const auto blocked = mxh::test::resolve_map10_smoke_endpoint(
            stage_env("21001", "21002", map_port));
        EXPECT_FALSE(blocked.ready) << map_port;
        EXPECT_NE(blocked.skip_reason.find(std::string("MXH_MAP10_SMOKE_MAP_PORT=") + map_port),
                  std::string::npos) << map_port;
        EXPECT_TRUE(blocked.log_line().empty()) << map_port;
    }
}

TEST(Map10SmokeEndpoint, DefaultStagePortsProceedWhenAllowIs1) {
    FakeEnv env = stage_env("16001", "17001", "18001");
    env.values[mxh::test::kMap10SmokeAllowDefaultStageEnv] = "1";
    const auto endpoint = mxh::test::resolve_map10_smoke_endpoint(env);
    ASSERT_TRUE(endpoint.ready);
    EXPECT_TRUE(endpoint.skip_reason.empty());
    EXPECT_EQ(endpoint.login_port, 16001);
    EXPECT_EQ(endpoint.agent_port, 17001);
    EXPECT_EQ(endpoint.map_port, 18001);
    EXPECT_EQ(endpoint.log_line(),
              "Map10InGameSmoke resolved endpoint host=192.168.2.244 "
              "login_port=16001 agent_port=17001 map_port=18001");
    EXPECT_NE(endpoint.e2e_arguments().find("--login-port 16001"), std::string::npos);
    EXPECT_NE(endpoint.e2e_arguments().find("--agent-port 17001"), std::string::npos);
    EXPECT_NE(endpoint.e2e_arguments().find("--map-port 18001"), std::string::npos);
}

TEST(Map10SmokeEndpoint, R2PortsProceedWithoutAllow) {
    FakeEnv env;
    env.values[mxh::test::kMap10SmokeHostEnv] = "10.8.8.8";
    env.values[mxh::test::kMap10SmokeLoginPortEnv] = "36101";
    env.values[mxh::test::kMap10SmokeAgentPortEnv] = "37101";
    env.values[mxh::test::kMap10SmokeMapPortEnv] = "38101";
    const auto endpoint = mxh::test::resolve_map10_smoke_endpoint(env);
    ASSERT_TRUE(endpoint.ready);
    EXPECT_TRUE(endpoint.skip_reason.empty());
    EXPECT_EQ(endpoint.log_line(),
              "Map10InGameSmoke resolved endpoint host=10.8.8.8 "
              "login_port=36101 agent_port=37101 map_port=38101");
    EXPECT_NE(endpoint.e2e_arguments().find("--login-port 36101"), std::string::npos);
    EXPECT_NE(endpoint.e2e_arguments().find("--agent-port 37101"), std::string::npos);
    EXPECT_NE(endpoint.e2e_arguments().find("--map-port 38101"), std::string::npos);
    EXPECT_EQ(endpoint.e2e_arguments().find("16001"), std::string::npos);
}
