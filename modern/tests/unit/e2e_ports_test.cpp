#include "../support/e2e_ports.hpp"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

namespace {

std::string message_of(const char* value) {
    try {
        mxh::test::e2e_ports_from_base_value(value);
    } catch (const std::runtime_error& error) {
        return error.what();
    }
    return {};
}

}  // namespace

TEST(E2ePorts, UnsetValueUsesDefaultPorts) {
    for (const char* value : {static_cast<const char*>(nullptr), ""}) {
        const auto ports = mxh::test::e2e_ports_from_base_value(value);
        EXPECT_EQ(ports.login, 26001);
        EXPECT_EQ(ports.agent, 27001);
        EXPECT_EQ(ports.map, 28001);
        EXPECT_EQ(ports.map12, 28002);
        EXPECT_TRUE(mxh::test::e2e_forbidden_port_message(
                        ports.login, ports.agent, ports.map, ports.map12)
                        .empty());
    }
}

TEST(E2ePorts, Base26001MatchesDefaults) {
    const auto ports = mxh::test::e2e_ports_from_base_value("26001");
    const auto defaults = mxh::test::e2e_ports_from_base_value(nullptr);
    EXPECT_EQ(ports.login, defaults.login);
    EXPECT_EQ(ports.agent, defaults.agent);
    EXPECT_EQ(ports.map, defaults.map);
    EXPECT_EQ(ports.map12, defaults.map12);
    EXPECT_EQ(ports.login, mxh::test::kE2eDefaultLoginPort);
    EXPECT_EQ(ports.agent, mxh::test::kE2eDefaultAgentPort);
    EXPECT_EQ(ports.map, mxh::test::kE2eDefaultMapPort);
    EXPECT_EQ(ports.map12, mxh::test::kE2eDefaultMap12Port);
}

TEST(E2ePorts, BaseDerivesFourPorts) {
    const auto ports = mxh::test::e2e_ports_from_base_value("26111");
    EXPECT_EQ(ports.login, 26111);
    EXPECT_EQ(ports.agent, 27111);
    EXPECT_EQ(ports.map, 28111);
    EXPECT_EQ(ports.map12, 28112);
}

TEST(E2ePorts, RejectsNonIntegerBase) {
    for (const char* value : {"abc", "26001x", " 26001", "26001 ", "-", "+"}) {
        const auto message = message_of(value);
        EXPECT_NE(message.find("MXH_E2E_PORT_BASE"), std::string::npos) << value;
        EXPECT_NE(message.find(value), std::string::npos) << message;
    }
}

TEST(E2ePorts, RejectsDerivedPortOutsideRange) {
    const auto message = message_of("64000");
    EXPECT_NE(message.find("MXH_E2E_PORT_BASE"), std::string::npos) << message;
    EXPECT_NE(message.find("64000"), std::string::npos) << message;
    EXPECT_NE(message.find("66000"), std::string::npos) << message;
}

TEST(E2ePorts, ExplicitPortsOverrideTheBase) {
    const auto ports = mxh::test::resolve_e2e_cli_ports(
        "26111", true, 26222, false, 0, false, 0, false, 0);
    EXPECT_EQ(ports.login, 26222);
    EXPECT_EQ(ports.agent, 27111);
    EXPECT_EQ(ports.map, 28111);
    EXPECT_EQ(ports.map12, 28112);
}

TEST(E2ePorts, ExplicitPortsIgnoreAnInvalidBase) {
    const auto ports = mxh::test::resolve_e2e_cli_ports(
        "not-a-base", true, 26222, true, 27222, true, 28222, true, 28223);
    EXPECT_EQ(ports.login, 26222);
    EXPECT_EQ(ports.agent, 27222);
    EXPECT_EQ(ports.map, 28222);
    EXPECT_EQ(ports.map12, 28223);
}

TEST(E2ePorts, ForbiddenPortsNameTheRole) {
    const auto derived = mxh::test::e2e_ports_from_base_value("15001");
    EXPECT_EQ(derived.agent, 16001);
    const auto message = mxh::test::e2e_forbidden_port_message(
        derived.login, derived.agent, derived.map, derived.map12);
    EXPECT_NE(message.find("agent=16001"), std::string::npos) << message;
    EXPECT_EQ(message.find("login=16001"), std::string::npos) << message;

    const auto login_hit = mxh::test::e2e_forbidden_port_message(16001, 27001, 28001, 28002);
    EXPECT_NE(login_hit.find("login=16001"), std::string::npos) << login_hit;
    const auto map_hit = mxh::test::e2e_forbidden_port_message(26001, 27001, 18001, 28002);
    EXPECT_NE(map_hit.find("map=18001"), std::string::npos) << map_hit;
    const auto agent_hit = mxh::test::e2e_forbidden_port_message(26001, 17001, 28001, 28002);
    EXPECT_NE(agent_hit.find("agent=17001"), std::string::npos) << agent_hit;
}

TEST(E2ePorts, Map12OffsetIsNotAForbiddenPort) {
    const auto message = mxh::test::e2e_forbidden_port_message(26001, 27001, 28001, 18002);
    EXPECT_TRUE(message.empty()) << message;
}
