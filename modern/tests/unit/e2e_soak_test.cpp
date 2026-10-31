#include "e2e_soak.hpp"
#include "mxh/net/abortive_close.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace {

std::string parse_error(const std::vector<std::string>& args) {
    mxh::e2e::SoakOptions opt;
    std::string error;
    EXPECT_FALSE(mxh::e2e::parse_soak_args(args, opt, error));
    return error;
}

mxh::e2e::SoakOptions parse_ok(const std::vector<std::string>& args) {
    mxh::e2e::SoakOptions opt;
    std::string error;
    EXPECT_TRUE(mxh::e2e::parse_soak_args(args, opt, error)) << error;
    return opt;
}

}  // namespace

TEST(E2eSoakArgs, ExistingFlagsAreNotSoakFlags) {
    EXPECT_FALSE(mxh::e2e::is_soak_flag("--exercise-position"));
    EXPECT_FALSE(mxh::e2e::is_soak_flag("--character-name"));
    EXPECT_FALSE(mxh::e2e::is_soak_flag("--login-port"));
    EXPECT_FALSE(mxh::e2e::is_soak_flag("--agent-port"));
    EXPECT_FALSE(mxh::e2e::is_soak_flag("--map-port"));
    EXPECT_FALSE(mxh::e2e::is_soak_flag("--map-number"));
    EXPECT_FALSE(mxh::e2e::is_soak_flag("--no-spawn"));
    EXPECT_FALSE(mxh::e2e::is_soak_flag("--timeout"));
}

TEST(E2eSoakArgs, ReuseCharacterRequiresName) {
    const auto opt = parse_ok({"--reuse-character", "soakA"});
    EXPECT_TRUE(opt.reuse);
    EXPECT_EQ(opt.reuse_character, "soakA");
    EXPECT_FALSE(mxh::e2e::in_map_soak_requested(opt));
    EXPECT_NE(parse_error({"--reuse-character"}).find("requires a value"), std::string::npos);
    EXPECT_NE(parse_error({"--reuse-character", ""}).find("empty"), std::string::npos);
    EXPECT_NE(parse_error({"--reuse-character", "this-name-is-too-long"}).find("16"),
              std::string::npos);
}

TEST(E2eSoakArgs, HoldSecondsParsesNonNegative) {
    const auto opt = parse_ok({"--hold-seconds", "20"});
    EXPECT_TRUE(opt.hold);
    EXPECT_EQ(opt.hold_seconds, 20);
    EXPECT_TRUE(mxh::e2e::in_map_soak_requested(opt));
    const auto zero = parse_ok({"--hold-seconds", "0"});
    EXPECT_TRUE(zero.hold);
    EXPECT_EQ(zero.hold_seconds, 0);
    EXPECT_NE(parse_error({"--hold-seconds", "-1"}).find("non-negative"), std::string::npos);
    EXPECT_NE(parse_error({"--hold-seconds", "12s"}).find("non-negative"), std::string::npos);
    EXPECT_NE(parse_error({"--hold-seconds"}).find("requires a value"), std::string::npos);
}

TEST(E2eSoakArgs, ExpectPeerDefaultsTimeoutTo30) {
    const auto opt = parse_ok({"--expect-peer", "other"});
    EXPECT_TRUE(opt.expect_peer);
    EXPECT_EQ(opt.expect_peer_name, "other");
    EXPECT_EQ(opt.expect_peer_timeout_s, 30);
    EXPECT_EQ(opt.expect_peer_timeout_s, mxh::e2e::kDefaultExpectPeerTimeoutSeconds);
    const auto overridden = parse_ok(
        {"--expect-peer-timeout", "45", "--expect-peer", "other"});
    EXPECT_EQ(overridden.expect_peer_timeout_s, 45);
    EXPECT_EQ(overridden.expect_peer_name, "other");
    EXPECT_NE(parse_error({"--expect-peer-timeout", "0"}).find(">= 1"), std::string::npos);
}

TEST(E2eSoakArgs, StateDumpAndDropParse) {
    const auto opt = parse_ok(
        {"--state-dump", "out/state.json", "--drop-socket-after", "5"});
    EXPECT_TRUE(opt.state_dump);
    EXPECT_EQ(opt.state_dump_path, "out/state.json");
    EXPECT_TRUE(opt.drop);
    EXPECT_EQ(opt.drop_socket_after_s, 5);
    EXPECT_EQ(mxh::e2e::kSimulatedDisconnectExitCode, 4);
    EXPECT_NE(parse_error({"--state-dump"}).find("requires a value"), std::string::npos);
    EXPECT_NE(parse_error({"--drop-socket-after", "-3"}).find("non-negative"), std::string::npos);
}

TEST(E2eSoakArgs, UnknownTokenIsRejected) {
    const auto error = parse_error({"--not-a-soak-flag"});
    EXPECT_NE(error.find("unknown arg:"), std::string::npos);
}

TEST(E2eSoakArgs, CombinedFlagsKeepIndependentValues) {
    const auto opt = parse_ok({
        "--reuse-character", "soakA",
        "--hold-seconds", "15",
        "--expect-peer", "soakB",
        "--expect-peer-timeout", "12",
        "--state-dump", "a.json",
        "--drop-socket-after", "9",
    });
    EXPECT_EQ(opt.reuse_character, "soakA");
    EXPECT_EQ(opt.hold_seconds, 15);
    EXPECT_EQ(opt.expect_peer_name, "soakB");
    EXPECT_EQ(opt.expect_peer_timeout_s, 12);
    EXPECT_EQ(opt.state_dump_path, "a.json");
    EXPECT_EQ(opt.drop_socket_after_s, 9);
    EXPECT_EQ(mxh::e2e::kKeepAliveIntervalSeconds, 10);
}

TEST(E2eSoakStateDump, SortsInventoryAndKeepsStableKeyOrder) {
    mxh::e2e::SessionDump dump;
    dump.entry.x = 25000;
    dump.entry.y = 25001;
    dump.entry.map = 10;
    dump.entry.hp = 100;
    dump.entry.mp = 40;
    dump.entry.shield = 7;
    dump.entry.money = 50;
    dump.entry.inventory.push_back({3, 11000, 1});
    dump.entry.inventory.push_back({1, 50108, 2});
    dump.exit = dump.entry;
    dump.exit.hp = 90;
    dump.exit.x = 25032;
    dump.exit.inventory = {{1, 50108, 2}};

    const auto json = mxh::e2e::format_session_dump_json(dump);
    const char* expected =
        "{\n"
        "  \"entry\": {\n"
        "    \"hp\": 100,\n"
        "    \"inventory\": [\n"
        "      {\n"
        "        \"count\": 2,\n"
        "        \"item_index\": 50108,\n"
        "        \"slot\": 1\n"
        "      },\n"
        "      {\n"
        "        \"count\": 1,\n"
        "        \"item_index\": 11000,\n"
        "        \"slot\": 3\n"
        "      }\n"
        "    ],\n"
        "    \"money\": 50,\n"
        "    \"mp\": 40,\n"
        "    \"position\": {\n"
        "      \"map\": 10,\n"
        "      \"x\": 25000,\n"
        "      \"y\": 25001\n"
        "    },\n"
        "    \"shield\": 7\n"
        "  },\n"
        "  \"exit\": {\n"
        "    \"hp\": 90,\n"
        "    \"inventory\": [\n"
        "      {\n"
        "        \"count\": 2,\n"
        "        \"item_index\": 50108,\n"
        "        \"slot\": 1\n"
        "      }\n"
        "    ],\n"
        "    \"money\": 50,\n"
        "    \"mp\": 40,\n"
        "    \"position\": {\n"
        "      \"map\": 10,\n"
        "      \"x\": 25032,\n"
        "      \"y\": 25001\n"
        "    },\n"
        "    \"shield\": 7\n"
        "  }\n"
        "}\n";
    EXPECT_EQ(json, expected);
    EXPECT_EQ(mxh::e2e::format_session_dump_json(dump), json);

    const auto hp = json.find("\"hp\"");
    const auto inventory = json.find("\"inventory\"");
    const auto money = json.find("\"money\"");
    const auto mp = json.find("\"mp\"");
    const auto position = json.find("\"position\"");
    const auto shield = json.find("\"shield\"");
    EXPECT_LT(hp, inventory);
    EXPECT_LT(inventory, money);
    EXPECT_LT(money, mp);
    EXPECT_LT(mp, position);
    EXPECT_LT(position, shield);
}

TEST(E2eSoakStateDump, WritesSyntheticJsonFile) {
    mxh::e2e::SessionDump dump;
    dump.entry.map = 10;
    dump.entry.hp = 1;
    dump.entry.money = 2;
    dump.exit = dump.entry;
    const auto dir = std::filesystem::temp_directory_path() / "mxh-e2e-soak-test";
    const auto path = dir / "state.json";
    std::filesystem::remove_all(dir);
    std::string error;
    ASSERT_TRUE(mxh::e2e::write_session_dump_file(path, dump, error)) << error;
    std::ifstream input(path, std::ios::binary);
    const std::string body((std::istreambuf_iterator<char>(input)),
                           std::istreambuf_iterator<char>());
    EXPECT_EQ(body, mxh::e2e::format_session_dump_json(dump));
    std::filesystem::remove_all(dir);
}

TEST(E2eSoakAbort, LingerOneZeroIsSetBeforeCloseAndShutdownIsAbsent) {
    std::vector<std::string> ops;
    const bool closed = mxh::net::abortive_rst_close(
        [&](mxh::net::LingerOption opt) {
            ops.push_back("linger " + std::to_string(opt.l_onoff) + " " +
                          std::to_string(opt.l_linger));
            return true;
        },
        [&]() { ops.push_back("close"); });
    ASSERT_TRUE(closed);
    ASSERT_EQ(ops.size(), 2u);
    EXPECT_EQ(ops[0], "linger 1 0");
    EXPECT_EQ(ops[1], "close");
    EXPECT_EQ(mxh::net::kAbortiveLinger.l_onoff, 1);
    EXPECT_EQ(mxh::net::kAbortiveLinger.l_linger, 0);
    for (const auto& op : ops) EXPECT_EQ(op.find("shutdown"), std::string::npos);
}

TEST(E2eSoakAbort, CloseDoesNotRunWhenLingerFails) {
    int closes = 0;
    int shutdowns = 0;
    const bool closed = mxh::net::abortive_rst_close(
        [](mxh::net::LingerOption opt) {
            EXPECT_EQ(opt.l_onoff, 1);
            EXPECT_EQ(opt.l_linger, 0);
            return false;
        },
        [&]() { ++closes; });
    EXPECT_FALSE(closed);
    EXPECT_EQ(closes, 0);
    EXPECT_EQ(shutdowns, 0);
}
