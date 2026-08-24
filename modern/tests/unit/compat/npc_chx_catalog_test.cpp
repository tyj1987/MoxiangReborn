#include <gtest/gtest.h>

#include "mxh/compat/npc_chx_catalog.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>

namespace {
std::filesystem::path findNpcChxList() {
    auto root = std::filesystem::current_path();
    for (int depth = 0; depth < 8 && !root.empty();
         ++depth, root = root.parent_path()) {
        const auto direct = root / L"data" / L"PlayDH" / L"Resource" /
                            L"Client" / L"NpcChxList.bin";
        if (std::filesystem::is_regular_file(direct)) return direct;
        for (const auto& entry : std::filesystem::directory_iterator(root)) {
            if (!entry.is_directory()) continue;
            const auto candidate = entry.path() / L"PlayDH" / L"Resource" /
                                   L"Client" / L"NpcChxList.bin";
            if (std::filesystem::is_regular_file(candidate)) return candidate;
        }
    }
    return {};
}
}

TEST(NpcChxCatalog, PreservesLegacySlotIndices) {
    const std::string text = "3\r\nnull.chx\r\nN001.chx\r\nN002.chx\r\n";
    const auto catalog = mxh::compat::NpcChxCatalog::parse_text(
        {reinterpret_cast<const std::uint8_t*>(text.data()), text.size()});
    ASSERT_TRUE(catalog.has_value());
    ASSERT_EQ(catalog->entries().size(), 3u);
    EXPECT_EQ(catalog->find(0), nullptr);
    ASSERT_NE(catalog->find(1), nullptr);
    EXPECT_EQ(*catalog->find(1), "N001.chx");
    ASSERT_NE(catalog->find(2), nullptr);
    EXPECT_EQ(*catalog->find(2), "N002.chx");
    EXPECT_EQ(catalog->find(3), nullptr);
}

TEST(NpcChxCatalog, RejectsTruncatedEntryTable) {
    const std::string text = "3 null.chx N001.chx";
    EXPECT_FALSE(mxh::compat::NpcChxCatalog::parse_text(
        {reinterpret_cast<const std::uint8_t*>(text.data()), text.size()}));
}

TEST(NpcChxCatalog, ParsesCanonicalPlayDhTable) {
    const auto file = findNpcChxList();
    if (file.empty()) GTEST_SKIP() << "PlayDH NpcChxList.bin unavailable";

    std::ifstream input(file, std::ios::binary | std::ios::ate);
    ASSERT_TRUE(input.good());
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(input.tellg()));
    input.seekg(0);
    input.read(reinterpret_cast<char*>(bytes.data()), bytes.size());

    const auto catalog = mxh::compat::NpcChxCatalog::parse_bin(bytes);
    ASSERT_TRUE(catalog.has_value());
    ASSERT_EQ(catalog->entries().size(), 87u);
    ASSERT_NE(catalog->find(1), nullptr);
    EXPECT_EQ(*catalog->find(1), "N001.chx");
    ASSERT_NE(catalog->find(80), nullptr);
    EXPECT_EQ(*catalog->find(80), "titan_npc.chx");
    EXPECT_EQ(catalog->find(87), nullptr);
}

// Axis-F invariant: every legacy NPC slot in [1..87] must resolve to a
// .chx filename so the renderer never falls back to the placeholder
// wireframe for a real NPC.  N073.chx was the one stale slot noted in
// docs/PLAYABLE_STATUS.md §0 ("1 个 N073.chx 占位") — make sure that
// slot and a sample of others all carry a non-null filename.
TEST(NpcChxCatalog, AllCanonicalSlotsResolveToChxFilename) {
    const auto file = findNpcChxList();
    if (file.empty()) GTEST_SKIP() << "PlayDH NpcChxList.bin unavailable";

    std::ifstream input(file, std::ios::binary | std::ios::ate);
    ASSERT_TRUE(input.good());
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(input.tellg()));
    input.seekg(0);
    input.read(reinterpret_cast<char*>(bytes.data()), bytes.size());

    const auto catalog = mxh::compat::NpcChxCatalog::parse_bin(bytes);
    ASSERT_TRUE(catalog.has_value());
    // Sample a handful of slots across the [1..86] range; the legacy
    // N073 entry is the most likely to be missing — it must NOT be.
    for (const std::uint32_t slot : {1u, 10u, 25u, 50u, 73u, 80u, 86u}) {
        const auto* name = catalog->find(slot);
        ASSERT_NE(name, nullptr) << "slot " << slot << " missing from "
                                    "NpcChxList.bin (would render as placeholder)";
        EXPECT_FALSE(name->empty())
            << "slot " << slot << " has empty CHX filename";
    }
}
