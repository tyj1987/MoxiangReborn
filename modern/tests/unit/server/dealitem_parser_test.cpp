#include "mxh/compat/mh_file_ex.hpp"
#include "mxh/server/dealitem_parser.hpp"
#include "mxh/server/npc_shop.hpp"
#include <gtest/gtest.h>
#include <algorithm>
#include <cstring>
#include <limits>
#include <filesystem>

namespace {
std::filesystem::path canonical_playdh() {
    for (const auto& root : {
             std::filesystem::current_path() / "modern" / "data" / "PlayDH",
             std::filesystem::current_path() / "data" / "PlayDH"}) {
        if (std::filesystem::exists(root / "Resource" / "Dealitem.bin")) {
            return root;
        }
    }
    return {};
}
}

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

TEST(DealItemParser, CanonicalProfileExposesDealerCatalogs) {
    const auto root = canonical_playdh();
    if (root.empty()) GTEST_SKIP() << "canonical PlayDH root not found";
    const auto parsed = mxh::server::load_dealitem(
        root / "Resource" / "Dealitem.bin");
    ASSERT_TRUE(parsed.error_message.empty()) << parsed.error_message;
    ASSERT_GT(parsed.npcs.size(), 0u);
    const auto first = mxh::server::catalog_for_npc(
        parsed, parsed.npcs.front().npc_index);
    ASSERT_TRUE(first.has_value());
    EXPECT_FALSE(first->entries.empty());
}
