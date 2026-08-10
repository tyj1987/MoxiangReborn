#include "mxh/compat/mh_file_ex.hpp"
#include "mxh/server/dealitem_parser.hpp"
#include <gtest/gtest.h>
#include <algorithm>
#include <cstring>
#include <limits>

TEST(DealItemParser, RejectsShortRows) {
    const std::string text = "1 map 2 npc 7\n";
    std::vector<std::uint8_t> payload(text.begin(), text.end());
    const auto encrypted = mxh::compat::encrypt_bin_payload(payload, 42);
    mxh::compat::MhFileHeader header{1, 42, static_cast<std::uint32_t>(payload.size())};
    std::vector<std::uint8_t> raw(sizeof(header) + 1 + encrypted.size() + 1);
    std::memcpy(raw.data(), &header, sizeof(header));
    std::copy(encrypted.begin(), encrypted.end(), raw.begin() + sizeof(header) + 1);
    const auto parsed = mxh::server::parse_dealitem_bytes(raw);
    EXPECT_EQ(parsed.rows_seen, 1u);
    EXPECT_EQ(parsed.parse_errors, 1u);
}
