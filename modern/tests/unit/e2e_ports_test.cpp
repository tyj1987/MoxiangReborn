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
        EXPECT_TRUE(mxh::test::e2e_ports_reject_reason(
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

TEST(E2ePorts, Rejects36001Group) {
    const auto derived = mxh::test::e2e_ports_from_base_value("36001");
    EXPECT_EQ(derived.login, 36001);
    EXPECT_EQ(derived.agent, 37001);
    EXPECT_EQ(derived.map, 38001);
    EXPECT_EQ(derived.map12, 38002);
    const auto derived_message = mxh::test::e2e_ports_reject_reason(
        derived.login, derived.agent, derived.map, derived.map12);
    EXPECT_NE(derived_message.find("login=36001"), std::string::npos) << derived_message;
    EXPECT_NE(derived_message.find("agent=37001"), std::string::npos) << derived_message;
    EXPECT_NE(derived_message.find("map=38001"), std::string::npos) << derived_message;
    EXPECT_EQ(derived_message.find("map12=38002"), std::string::npos) << derived_message;

    const auto overridden = mxh::test::resolve_e2e_cli_ports(
        "26001", false, 0, true, 37001, false, 0, false, 0);
    EXPECT_EQ(overridden.login, 26001);
    EXPECT_EQ(overridden.agent, 37001);
    const auto override_message = mxh::test::e2e_ports_reject_reason(
        overridden.login, overridden.agent, overridden.map, overridden.map12);
    EXPECT_NE(override_message.find("agent=37001"), std::string::npos) << override_message;
    EXPECT_EQ(override_message.find("login=36001"), std::string::npos) << override_message;
}

TEST(E2ePorts, RejectsPortsOutside1024To65535) {
    const auto low = mxh::test::e2e_ports_from_base_value("1000");
    EXPECT_EQ(low.login, 1000);
    const auto low_message = mxh::test::e2e_ports_reject_reason(
        low.login, low.agent, low.map, low.map12);
    EXPECT_NE(low_message.find("login=1000"), std::string::npos) << low_message;
    EXPECT_NE(low_message.find("1024"), std::string::npos) << low_message;
    EXPECT_EQ(low_message.find("agent=2000"), std::string::npos) << low_message;

    const auto high = mxh::test::e2e_ports_from_base_value("64000");
    EXPECT_EQ(high.map, 66000);
    EXPECT_EQ(high.map12, 66001);
    const auto high_message = mxh::test::e2e_ports_reject_reason(
        high.login, high.agent, high.map, high.map12);
    EXPECT_NE(high_message.find("map12=66001"), std::string::npos) << high_message;
    EXPECT_NE(high_message.find("65535"), std::string::npos) << high_message;
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

TEST(E2ePorts, Map17Port28003IsAllowed) {
    EXPECT_EQ(mxh::test::kE2eDefaultMap17Port, 28003);
    EXPECT_FALSE(mxh::test::e2e_port_is_forbidden(28003));
    const auto message = mxh::test::e2e_forbidden_port_message(
        26001, 27001, 28001, mxh::test::kE2eDefaultMap17Port);
    EXPECT_TRUE(message.empty()) << message;
}

TEST(E2ePorts, StageAndR18PortsStayRejected) {
    for (const int port : {16001, 17001, 18001, 36001, 37001, 38001}) {
        EXPECT_TRUE(mxh::test::e2e_port_is_forbidden(port)) << port;
    }
    const auto message = mxh::test::e2e_forbidden_port_message(16001, 17001, 18001, 36001);
    EXPECT_NE(message.find("login=16001"), std::string::npos) << message;
    EXPECT_NE(message.find("agent=17001"), std::string::npos) << message;
    EXPECT_NE(message.find("map=18001"), std::string::npos) << message;
    EXPECT_NE(message.find("map12=36001"), std::string::npos) << message;
}
