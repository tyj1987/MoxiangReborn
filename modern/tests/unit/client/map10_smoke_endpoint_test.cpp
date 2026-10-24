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

TEST(Map10SmokeEndpoint, ExplicitCt930PortsAreHonoredWhenSet) {
    FakeEnv env;
    env.values[mxh::test::kMap10SmokeHostEnv] = "192.168.2.244";
    env.values[mxh::test::kMap10SmokeLoginPortEnv] = "16001";
    env.values[mxh::test::kMap10SmokeAgentPortEnv] = "17001";
    env.values[mxh::test::kMap10SmokeMapPortEnv] = "18001";
    const auto endpoint = mxh::test::resolve_map10_smoke_endpoint(env);
    ASSERT_TRUE(endpoint.ready);
    EXPECT_EQ(endpoint.login_port, 16001);
    EXPECT_EQ(endpoint.agent_port, 17001);
    EXPECT_EQ(endpoint.map_port, 18001);
    EXPECT_NE(endpoint.e2e_arguments().find("--login-port 16001"), std::string::npos);
    EXPECT_NE(endpoint.log_line().find("host=192.168.2.244"), std::string::npos);
}
