#include "mxh/compat/gx_map_script.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

using mxh::compat::GxMapReject;
using mxh::compat::parse_gx_map_script;

namespace {

std::vector<std::uint8_t> bytes_of(std::string_view text) {
    return std::vector<std::uint8_t>(text.begin(), text.end());
}

std::vector<std::uint8_t> obfuscated_map(std::size_t body_len) {
    const std::size_t size = 4 + 20 + body_len;
    std::vector<std::uint8_t> bytes(size);
    bytes[0] = static_cast<std::uint8_t>(size & 0xffu);
    bytes[1] = static_cast<std::uint8_t>((size >> 8) & 0xffu);
    bytes[2] = static_cast<std::uint8_t>((size >> 16) & 0xffu);
    bytes[3] = static_cast<std::uint8_t>((size >> 24) & 0xffu);
    for (std::size_t i = 4; i < 24; ++i)
        bytes[i] = static_cast<std::uint8_t>(0xA0 + (i - 4));
    // Plaintext bait sits in the XOR region. A parser that ignores the
    // length prefix and keeps scanning would record should-not-parse.stm.
    const std::string bait = "GX_MAP { STATIC_MODEL should-not-parse.stm HEIGHT_FIELD no.hfl }";
    for (std::size_t i = 0; i < body_len; ++i)
        bytes[24 + i] = static_cast<std::uint8_t>(bait[i % bait.size()]);
    return bytes;
}

}  // namespace

TEST(GxMapScript, ParsesPlaintextGxMap) {
    const auto text = std::string(
        "GX_METADATA\r\n"
        "{\r\n"
        "\tBOX_MAX 0.000000 11751.000000 0.000000\r\n"
        "\tBOX_MIN 0.000000 -1521.000000 0.000000\r\n"
        "}\r\n"
        "GX_MAP\r\n"
        "{ \r\n"
        "\t STATIC_MODEL 10.stm \r\n"
        "\t HEIGHT_FIELD 10.hfl \r\n"
        "} \r\n"
        "GX_OBJECT \t 1\r\n"
        "{ \r\n"
        "\t 106_fire.chr \t 4294967280 \t 2.0 2.0 2.0 \t 1 2 3 \t 0 1 0 0 \t 0 \r\n"
        "} \r\n"
        "GX_LIGHT \t 1\r\n"
        "{ \r\n"
        "\t FFFFE6BE \t 8291.230469 10000.000000 31573.900391 \t 1000.000000 \t 1000 \r\n"
        "} \r\n"
        "GX_TRIGGER \t 0\r\n"
        "{ \r\n"
        "} \r\n");
    testing::internal::CaptureStderr();
    const auto parsed = parse_gx_map_script(bytes_of(text), "10.map");
    const auto logged = testing::internal::GetCapturedStderr();

    EXPECT_TRUE(parsed.ok);
    EXPECT_EQ(parsed.reject, GxMapReject::None);
    EXPECT_TRUE(parsed.message.empty());
    EXPECT_EQ(logged.find("ERROR [map]"), std::string::npos) << logged;
    EXPECT_TRUE(parsed.script.has_metadata);
    EXPECT_FLOAT_EQ(parsed.script.box_max[1], 11751.0f);
    EXPECT_FLOAT_EQ(parsed.script.box_min[1], -1521.0f);
    EXPECT_EQ(parsed.script.static_model, "10.stm");
    EXPECT_EQ(parsed.script.height_field, "10.hfl");
}

TEST(GxMapScript, RejectsObfuscatedBufferWithExactError) {
    const auto bytes = obfuscated_map(80);
    ASSERT_GE(bytes.size(), 24u);
    const std::string raw(bytes.begin(), bytes.end());
    ASSERT_NE(raw.find("should-not-parse.stm"), std::string::npos);
    testing::internal::CaptureStderr();
    const auto parsed = parse_gx_map_script(bytes, "10.map");
    const auto logged = testing::internal::GetCapturedStderr();

    const std::string expected =
        "ERROR [map] obfuscated .map not supported: 10.map (bytes=" +
        std::to_string(bytes.size()) + ")";
    EXPECT_FALSE(parsed.ok);
    EXPECT_EQ(parsed.reject, GxMapReject::Obfuscated);
    EXPECT_EQ(parsed.message, expected);
    EXPECT_NE(logged.find(expected), std::string::npos) << logged;
    EXPECT_TRUE(parsed.script.static_model.empty());
    EXPECT_TRUE(parsed.script.height_field.empty());
}

TEST(GxMapScript, RejectsPrefixMismatchAsMalformed) {
    std::vector<std::uint8_t> bytes(40, 0x11);
    bytes[0] = 7;
    bytes[1] = 0;
    bytes[2] = 0;
    bytes[3] = 0;
    const std::string bait = "GX_MAP { STATIC_MODEL leaked.stm }";
    for (std::size_t i = 0; i < bait.size() && 8 + i < bytes.size(); ++i)
        bytes[8 + i] = static_cast<std::uint8_t>(bait[i]);

    testing::internal::CaptureStderr();
    const auto parsed = parse_gx_map_script(bytes, "broken.map");
    const auto logged = testing::internal::GetCapturedStderr();

    const std::string expected =
        "ERROR [map] malformed .map: broken.map (bytes=40)";
    EXPECT_FALSE(parsed.ok);
    EXPECT_EQ(parsed.reject, GxMapReject::Malformed);
    EXPECT_EQ(parsed.message, expected);
    EXPECT_NE(logged.find(expected), std::string::npos) << logged;
    EXPECT_EQ(logged.find("obfuscated"), std::string::npos) << logged;
    EXPECT_TRUE(parsed.script.static_model.empty());
}

TEST(GxMapScript, HandlesTinyBuffer) {
    std::vector<std::uint8_t> prefixed(10, 0x3c);
    prefixed[0] = 10;
    prefixed[1] = 0;
    prefixed[2] = 0;
    prefixed[3] = 0;
    testing::internal::CaptureStderr();
    const auto rejected = parse_gx_map_script(prefixed, "tiny.map");
    const auto rejected_log = testing::internal::GetCapturedStderr();
    EXPECT_FALSE(rejected.ok);
    EXPECT_EQ(rejected.reject, GxMapReject::Malformed);
    EXPECT_EQ(rejected.message, "ERROR [map] malformed .map: tiny.map (bytes=10)");
    EXPECT_NE(rejected_log.find(rejected.message), std::string::npos) << rejected_log;
    EXPECT_EQ(rejected_log.find("obfuscated"), std::string::npos) << rejected_log;

    const auto tiny_ok = bytes_of("GX_MAP{STATIC_MODEL a}");
    ASSERT_LT(tiny_ok.size(), 24u);
    testing::internal::CaptureStderr();
    const auto parsed = parse_gx_map_script(tiny_ok, "9.map");
    const auto parsed_log = testing::internal::GetCapturedStderr();
    EXPECT_TRUE(parsed.ok);
    EXPECT_EQ(parsed.script.static_model, "a");
    EXPECT_EQ(parsed_log.find("ERROR [map]"), std::string::npos) << parsed_log;

    const auto boundary = obfuscated_map(0);
    ASSERT_EQ(boundary.size(), 24u);
    testing::internal::CaptureStderr();
    const auto at_24 = parse_gx_map_script(boundary, "edge.map");
    const auto at_24_log = testing::internal::GetCapturedStderr();
    EXPECT_NE(at_24_log.find(at_24.message), std::string::npos) << at_24_log;
    EXPECT_FALSE(at_24.ok);
    EXPECT_EQ(at_24.reject, GxMapReject::Obfuscated);
    EXPECT_EQ(at_24.message,
              "ERROR [map] obfuscated .map not supported: edge.map (bytes=24)");
}
